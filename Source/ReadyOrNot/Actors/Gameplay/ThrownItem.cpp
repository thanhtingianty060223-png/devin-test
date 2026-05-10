// Void Interactive, 2020

#include "Actors/Gameplay/ThrownItem.h"

#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "Perception/AISense_Sight.h"

AThrownItem::AThrownItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	bReplicates = true;
	AActor::SetReplicateMovement(true);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(SceneComponent);
	StaticMesh->bNavigationRelevant = false;
	StaticMesh->SetCanEverAffectNavigation(false);
	StaticMesh->SetCollisionProfileName("PhysicsItem");

	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));

	TurnPhysicsOffDelay = 2.0f;
}

void AThrownItem::BeginPlay()
{
	Super::BeginPlay();

	ThrowInstigator = Cast<AReadyOrNotCharacter>(GetInstigator());
	
	PerceptionStimuliComp->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuliComp->RegisterWithPerceptionSystem();
	
	StaticMesh->SetNotifyRigidBodyCollision(true);
	StaticMesh->OnComponentHit.AddDynamic(this, &AThrownItem::OnHit);

	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->AllThrownItems.Add(this);
	}
}

void AThrownItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->AllThrownItems.Remove(this);
	}
}

void AThrownItem::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	/*if (StaticMesh->IsSimulatingPhysics())
	{
		TurnPhysicsOffDelay = TurnPhysicsOffDelay - DeltaTime;
		
		if (TurnPhysicsOffDelay <= 0.0f)
		{
			StaticMesh->SetSimulatePhysics(false);
		}
	}*/

	TravelAlongSimulatedPath(DeltaTime);
}

void AThrownItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AThrownItem, CompletePath, COND_SkipOwner);
	
	DOREPLIFETIME_CONDITION(AThrownItem, BouncePt1, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AThrownItem, BouncePt2, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AThrownItem, BouncePt3, COND_SkipOwner);
}

void AThrownItem::OnRep_ThrowPath()
{
}

void AThrownItem::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
}

bool AThrownItem::IsLocallyControlled() const
{
	return false;
}

void AThrownItem::UpdateServerPath_Implementation(const TArray<FVector_NetQuantize>& Path, int32 Bounce1,int32 Bounce2, int32 Bounce3)
{
	CompletePath = Path;
	BouncePt1 = Bounce1;
	BouncePt2 = Bounce2;
	BouncePt3 = Bounce3;
}

// fully simulate the throw path as throws are known to go through walls
void AThrownItem::FullySimulateThrowPath(FVector ThrowDirection, float DistMultiplier, FVector ForcedStartPoint)
{
	if (!ThrowInstigator)
		return;

	EDrawDebugTrace::Type DebugTrace = EDrawDebugTrace::None;
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		DebugTrace = EDrawDebugTrace::ForDuration;
	}
	#endif

	CompletePath.Empty(30);
	FirstBouncePath.Empty(10);
	SecondBouncePath.Empty(10);
	ThirdBouncePath.Empty(10);

	FVector StartTrace = ForcedStartPoint == FVector::ZeroVector ? StaticMesh->GetComponentLocation() : ForcedStartPoint;

	if (ForcedStartPoint != FVector::ZeroVector)
	{
		// Control points
		const FVector C1 = StaticMesh->GetComponentLocation();
		const FVector C2 = StartTrace - ThrowDirection.GetSafeNormal() * 50.0f;
		const FVector C3 = StartTrace;
		
		DrawDebugSphere(GetWorld(), StartTrace, 10.0f, 8, FColor::Turquoise, false, 5.0f);

		// Bezier curve baby
		for (float i = 0; i <= 1.0f; i += 0.1f)
		{
			FVector P1 = FMath::Lerp(C1, C2, i);
			FVector P2 = FMath::Lerp(C2, C3, i);
			FVector P3 = FMath::Lerp(P1, P2, i);

			#if !UE_BUILD_SHIPPING
			//if (DebugTrace != EDrawDebugTrace::None)
			{
				DrawDebugSphere(GetWorld(), P3, 5.0f, 4, FColor::Green, false, 5.0f);
			}
			#endif
			
			CompletePath.Add(P3);
		}
		
		CompletePath.Add(StartTrace);
		
		#if !UE_BUILD_SHIPPING
		//if (DebugTrace != EDrawDebugTrace::None)
		{
			DrawDebugSphere(GetWorld(), C1, 2.5f, 4, FColor::Red, false, 5.0f);
			DrawDebugSphere(GetWorld(), C2, 2.5f, 4, FColor::Red, false, 5.0f);
			DrawDebugSphere(GetWorld(), C3, 2.5f, 4, FColor::Red, false, 5.0f);
		}
		#endif
	}
	else
	{
		// TODO: refactor this code into a static function so anyone can generate a throw path and not have to type all this again
		
		if (DistMultiplier > 0.0f)
		{
			TArray<AActor*> IgnoredActors = GetIgnoredActorsForThrow();
			
			FVector OutLastTraceDestination;
			UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), FirstBounceHit, FirstBouncePath, OutLastTraceDestination, StartTrace, (ThrowDirection * ThrowDistance) * DistMultiplier, true, 5.0f, ECC_WorldStatic, false, IgnoredActors, DebugTrace, 3.0f);

			#if !UE_BUILD_SHIPPING
			if (DebugTrace != EDrawDebugTrace::None)
			{
				DrawDebugBox(GetWorld(), StartTrace, FVector(10.0f), FColor::Yellow, false, 4.0f);
				DrawDebugLine(GetWorld(), StartTrace - ThrowDirection.GetSafeNormal() * 100.0f, StartTrace + ThrowDirection.GetSafeNormal() * 100.0f, FColor::Yellow, false, 4.0f);
			}
			#endif
			
			if (!ThrowInstigator->IsA(ACyberneticCharacter::StaticClass()))
			{
				FCollisionObjectQueryParams CollisionObjectQueryParams;
				CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

				FHitResult WallHitTest;
				if (GetWorld()->LineTraceSingleByObjectType(WallHitTest, StartTrace - ThrowDirection.GetSafeNormal() * 100.0f, StartTrace + ThrowDirection.GetSafeNormal() * 100.0f, CollisionObjectQueryParams, ThrowInstigator->GetCollisionQueryParameters()))
				{
					FirstBouncePath.Empty(1);
					FirstBouncePath.Add(WallHitTest.Location);
					SetActorLocation(WallHitTest.Location, false, nullptr, ETeleportType::TeleportPhysics);
					
					#if !UE_BUILD_SHIPPING
					if (DebugTrace != EDrawDebugTrace::None)
					{
						DrawDebugBox(GetWorld(), WallHitTest.Location, FVector(5.0f), FColor::Magenta, false, 4.0f);
						ULog::ObjectName(WallHitTest.GetActor());
					}
					#endif
				}
				else
				{
					// Can we bounce? (Only bounce if on ground)
					FHitResult GroundHitTest;
					if (GetWorld()->LineTraceSingleByObjectType(GroundHitTest, FirstBouncePath.Last(), FirstBouncePath.Last() - FVector::UpVector * 25.0f, CollisionObjectQueryParams))
					{
						FVector OutLastTraceDestinationBounce;
						//FVector MirroredVector = UKismetMathLibrary::MirrorVectorByNormal(FirstBounceHit.TraceEnd - FirstBounceHit.TraceStart, FirstBounceHit.ImpactNormal);
						UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), SecondBounceHit, SecondBouncePath, OutLastTraceDestinationBounce, FirstBouncePath.Last(), (ThrowDirection * 150) * ThrowBounciness, true, 5.0f, ECC_PROJECTILE, true, IgnoredActors, DebugTrace, 3.0f);

						FVector OutLastTraceDestinationBounce2;
						//FVector MirroredVector2 = UKismetMathLibrary::MirrorVectorByNormal(SecondBounceHit.TraceEnd - SecondBounceHit.TraceStart, SecondBounceHit.ImpactNormal);
						UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), ThirdBounceHit, ThirdBouncePath, OutLastTraceDestinationBounce2, SecondBouncePath.Last(), (ThrowDirection * 150) * ThrowBounciness, true, 5.0f, ECC_PROJECTILE, true, IgnoredActors, DebugTrace, 3.0f);
					}
				}
			}
		}
		else
		{
			FirstBouncePath.Add(GetActorLocation());
			SecondBouncePath.Empty();
			ThirdBouncePath.Empty();
		}

	    CompletePath.Append(FirstBouncePath);
	    CompletePath.Append(SecondBouncePath);
		CompletePath.Append(ThirdBouncePath);
	}

	BouncePt1 = FirstBouncePath.Num() - 1;
	BouncePt2 = BouncePt1 + FMath::Clamp(BouncePt1 + SecondBouncePath.Num() - 1, 0, SecondBouncePath.Num());
	BouncePt3 = BouncePt2 + FMath::Clamp(BouncePt2 + ThirdBouncePath.Num() - 1, 0, ThirdBouncePath.Num());

	if (IsLocallyControlled())
	{
		UpdateServerPath(CompletePath, BouncePt1, BouncePt2, BouncePt3);
	}
}

void AThrownItem::TravelAlongSimulatedPath(const float DeltaTime)
{
	if (CompletePath.IsValidIndex(pathIdx))
	{
		StaticMesh->SetSimulatePhysics(false);

		ThrowSpeed = FMath::FInterpConstantTo(ThrowSpeed, MaxThrowSpeed, DeltaTime, MaxThrowSpeed);
		StaticMesh->SetWorldLocation(UKismetMathLibrary::VInterpTo_Constant(StaticMesh->GetComponentLocation(), CompletePath[pathIdx], DeltaTime, ThrowSpeed), false, nullptr, ETeleportType::TeleportPhysics);
		StaticMesh->AddWorldRotation(FRotator(36.0f * DeltaTime, 36.0f * DeltaTime, 36.f * DeltaTime));
		
		if (FVector::Distance(StaticMesh->GetComponentLocation(), CompletePath[pathIdx]) < 10.0f)
		{
			//if (pathIdx >= BouncePt1)
			//{
			//	StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			//}
			/*if (pathIdx == 0)
			{
				ThrowSpeed = MaxThrowSpeed * 0.25f;
			}
			
			if ( pathIdx == BouncePt1 || pathIdx == BouncePt2 || pathIdx == BouncePt3) //   pathIdx <= BouncePt3
			{
				StaticMesh->AddWorldRotation(FRotator(FMath::RandRange(0.0f, 360.0f)));
				MaxThrowSpeed *= 0.77f;
				ThrowSpeed = MaxThrowSpeed * 0.25f;
				//PlayFMODBounceSound();
			}
			
			if (pathIdx == CompletePath.Num()-1)
			{
				/*if (bDetonateAtEndofPath && CurrentDetonations == 0)
				{
					Detonate();
				}#1#
				
				if (ThirdBounceHit.ImpactPoint != FVector::ZeroVector)
				{
					/*if (ThrowBounceEffect)
					{
						UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ThrowBounceEffect, ThirdBounceHit.ImpactPoint, ThirdBounceHit.ImpactNormal.Rotation());
					}#1#
				}
			}
			else if (pathIdx == BouncePt1)
            {
				if (FirstBounceHit.ImpactPoint != FVector::ZeroVector)
				{
					/*if (ThrowBounceEffect)
					{
						UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ThrowBounceEffect, FirstBounceHit.ImpactPoint, FirstBounceHit.ImpactNormal.Rotation());
					}#1#
				}
            } else if (pathIdx == BouncePt2)
            {
                if (SecondBounceHit.ImpactPoint != FVector::ZeroVector)
                {
                	/*if (ThrowBounceEffect)
                	{
                		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ThrowBounceEffect, SecondBounceHit.ImpactPoint, SecondBounceHit.ImpactNormal.Rotation());
                	}#1#
                }
                
            }*/
			
			//V_LOGM(LogReadyOrNot, "Throw Hit pathIdx %s on [IsHost? %s]", *FString::FromInt(pathIdx), (GetLocalRole() == ROLE_Authority ? "true" : "false"));
			pathIdx++;
		}
	}
	else if (pathIdx >= CompletePath.Num()-1)
	{
		if (!StaticMesh->IsSimulatingPhysics())
		{
			SetActorEnableCollision(true);
			StaticMesh->SetCollisionProfileName("PhysicsItem");
			StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			StaticMesh->SetSimulatePhysics(true);

			if (CompletePath.IsValidIndex(CompletePath.Num() - 1) && CompletePath.IsValidIndex(CompletePath.Num() - 2))
			{
				const FVector LastPathPoint = CompletePath[CompletePath.Num() - 1];
				const FVector SecondLastPathPoint = CompletePath[CompletePath.Num() - 2];
				const FVector ImpulseDirection = (LastPathPoint - SecondLastPathPoint).GetSafeNormal2D();
				StaticMesh->AddImpulse(ImpulseDirection * 5000.0f);
			}
		}
	}
}

TArray<AActor*> AThrownItem::GetIgnoredActorsForThrow()
{
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Reserve(100);

	if (ThrowInstigator)
		IgnoredActors.Append(ThrowInstigator->GetCollisionIgnoredActors());
	
	IgnoredActors.Add(GetOwner());
	IgnoredActors.Add(this);

	IgnoredActors += (TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters;
	IgnoredActors += (TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems;

	return IgnoredActors;
}
