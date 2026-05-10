// Copyright Void Interactive, 2022

#include "BaseGrenade.h"

#include "ReadyOrNotGameMode.h"

#include "Gameplay/ReadyOrNotPlayerState.h"
#include "Gameplay/ThrownGrenade.h"

#include "ReadyOrNotDebugSubsystem.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/AI/SWATCharacter.h"

#include "FMODBlueprintStatics.h"
#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticController.h"
#include "Components/InteractableComponent.h"
#include "Components/MoraleComponent.h"
#include "Environment/BreakableGlass.h"

#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/SWATManager.h"
#include "Perception/AISense_Hearing.h"
#include "Senses/ReadyOrNotAISense_Sight.h"

static TAutoConsoleVariable<int32> CVarDrawGrenadeDebug(TEXT("a.RonDrawGrenadeDebug"), 0, TEXT("Visualize grenade inner and outer damage radii"));
static TAutoConsoleVariable<int32> CVarClientModifyGrenadeLocation(TEXT("a.ClientModifyGrenadeLocation"), 0, TEXT("Modify grenade location on client after throwing, see how server handles it"));

ABaseGrenade::ABaseGrenade()
{
	SetCanBeDamaged(false);

	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 30.0f;
	NetPriority = 2.0f;

	bDisableTickWhenNotEquipped = false;
	DetonationLight = CreateDefaultSubobject<UPointLightComponent>("DetonationLight");
	DetonationLight->SetupAttachment(ItemMesh);
	DetonationLight->SetCastShadows(false);
	DetonationLight->Intensity = 0.0f;

	DetonationRadialForce = CreateDefaultSubobject<URadialForceComponent>("RadialForceComponent");
	DetonationRadialForce->SetupAttachment(ItemMesh);
	DetonationRadialForce->bAutoActivate = false;
	DetonationRadialForce->Radius = 1000.0f;
	DetonationRadialForce->ImpulseStrength = 1000.0f;

	DetonationStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));

	ItemMesh->bReplicatePhysicsToAutonomousProxy = false;
	ItemMesh->SetIsReplicated(false);
	ItemMesh->SetCanEverAffectNavigation(false);
	ItemMesh->bNavigationRelevant = false;
	AActor::SetReplicateMovement(false);
}

bool ABaseGrenade::ShouldEnableWeaponFovShader() const
{
	if (!GetAttachParentActor() || bUsed)
		return false;
	
	return Super::ShouldEnableWeaponFovShader();
}

bool ABaseGrenade::ShouldAttachToOwner() const
{
	if (bUsed || bHasEverDetonated)
		return false;
	
	return Super::ShouldAttachToOwner();
}

void ABaseGrenade::SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation)
{
	if (!GetOwnerCharacter())
		return;

	if (!ThrownItemClass)
		return;

	// Spawn thrown grenade
	Thrown = GetWorld()->SpawnActorDeferred<AThrownGrenade>(ThrownItemClass.Get(), Transform, GetOwner(), GetOwnerCharacter(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Thrown)
	{
		Thrown->FinishSpawning(Transform);
		
		Thrown->GetStaticMesh()->SetCollisionProfileName("Item");
		Thrown->GetStaticMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		
		Thrown->FullySimulateThrowPath(ThrowDirection, 1.0f, ThrowLocation);

		bUsed = true;
		bGrenadeReleased = true;
		bThrowStarted = true;
		bStartedThorwingGrenade = true;
		CurrentDetonations = 0;
		
		SetInstigator(GetOwnerCharacter());
		
		bStartedDetonating = true;
		if (DetonationTime <= 0.0f)
		{
			Detonate();
		}
		else
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(DetonationTime_Handle, this, &ABaseGrenade::Detonate, DetonationTime, false);
		}

		DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
		AttachToComponent(Thrown->GetStaticMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ItemMesh->AttachToComponent(Thrown->GetStaticMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetItemLocation(), 1.0f, this, 1000.0f, ThrownTag);
		
		OnGrenadeThrown.Broadcast(this);
	}
}

void ABaseGrenade::SetDetonateAtEndOfPath(bool bShouldDetonateAtEndOfPath)
{
	bDetonateAtEndofPath = bShouldDetonateAtEndOfPath;
}

void ABaseGrenade::AIThrow()
{
	if (!GetOwningAIController() || !GetOwningAICharacter())
		return;

	bForceNoSimulateGrenadePathOnThrow = false;

	//const FVector ThrowDirection = GetOwnerCharacter()->GetControlRotation().Vector();
	const FVector ThrowDirection = (GetOwningAICharacter()->Rep_FocalPoint - FVector(GetOwningAICharacter()->GetActorLocation().X, GetOwningAICharacter()->GetActorLocation().Y, GetOwningAICharacter()->Rep_FocalPoint.Z)).GetSafeNormal();
	
	bDropping = true;
	bUsed = true;
	bGrenadeReleased = true;
	bThrowStarted = true;
	bStartedThorwingGrenade = true;
	bAIThrowComplete = true;
	bHideGrenadeOnDetonate = false;
	ThrownBy = GetOwnerCharacter();
	SetInstigator(GetOwnerCharacter());
	
	FullySimulateGrenadePath(ThrowDirection, GetOwnerCharacter()->GetMesh()->GetBoneLocation("hand_LE"));
	
	GetOwnerCharacter()->GetInventoryComponent()->RemoveInventoryItem(this, false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	StartDetonationTimer();

	OnGrenadeThrown.Broadcast(this);
}

void ABaseGrenade::OnRep_GrenadePath()
{
	//for (int32 i = 0; i < CompletePath.Num(); i++)
	//{
	//	DrawDebugSphere(GetWorld(), CompletePath[i], 3.0f, 12.0f, FColor::Yellow, false, 10.0f, 0, 2.0f);
	//}
}

void ABaseGrenade::SetItemVisibility(const bool bNewVisibility)
{
	if (GetOwnerCharacter() && GetOwnerCharacter()->bPrimed && GetOwnerCharacter()->GetEquippedItem() == this)
	{
		Super::SetItemVisibility(bNewVisibility);
		return;
	}
	if (bGrenadeReleased && !bHideGrenadeOnDetonate)
	{
		Super::SetItemVisibility(true);
		return;
	}
	
	if (bHasEverDetonated && bHideGrenadeOnDetonate)
	{
		Super::SetItemVisibility(false);
		return;
	}

	if (bUsed)
	{
		Super::SetItemVisibility(true);
		return;
	}
	
	Super::SetItemVisibility(bNewVisibility);
}

void ABaseGrenade::OnRep_GrenadeUsed()
{
	if (APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner()))
	{
		OwnerCharacter->UpdatePictureInPictureVisibility();
	}
}

void ABaseGrenade::Multicast_AddImpulse_Implementation(const FVector Impulse, FVector FromLocation)
{
 	//ItemMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
 	//ItemMesh->AddImpulse(Impulse);
	//ULog::Info("Multicast Impulse");
}

void ABaseGrenade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABaseGrenade, CompletePath, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseGrenade, BouncePt1, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseGrenade, BouncePt2, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseGrenade, BouncePt3, COND_SkipOwner);
	
	DOREPLIFETIME(ABaseGrenade, bUsed);
	DOREPLIFETIME(ABaseGrenade, bGrenadeReleased);
	DOREPLIFETIME(ABaseGrenade, bThrowAsQuickThrow);
	DOREPLIFETIME(ABaseGrenade, bFastThrowing);
	
	DOREPLIFETIME(ABaseGrenade, ThrownBy);
}

void ABaseGrenade::BeginPlay()
{
	Super::BeginPlay();
	
	ActivationElapsedTime = 0.0f;
	MaxGrenadeSpeed = GrenadeSpeed;

	bCanPlayBounceSound = true;
}

void ABaseGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	for (int32 i = 0 ; i < SpawnedParticles.Num(); i++)
	{
		UParticleSystemComponent* ParticleSystem = SpawnedParticles[i];
		if (ParticleSystem)
		{
			ParticleSystem->DestroyComponent();
		}
	}
}

void ABaseGrenade::Tick(const float DeltaSeconds)
{
	#if !UE_BUILD_SHIPPING
	if (GIsAutomationTesting)
	{
		ItemMesh->bNoSkeletonUpdate = true;

		if (bHasEverDetonated)
		{
			if (GetOwnerCharacter())
				GetOwnerCharacter()->GetInventoryComponent()->RemoveInventoryItem(this);
			ItemMesh->SetSkeletalMesh(nullptr);
			SetActorTickEnabled(false);
			SetLifeSpan(1.0f);
			PrimaryActorTick.UnRegisterTickFunction();
			Destroy();
			if (InteractableComponent)
				InteractableComponent->DestroyComponent();
			return;
		}
	}
	#endif
	
	Super::Tick(DeltaSeconds);

	if (bUsed)
		TimeSinceUsed += DeltaSeconds;
	
	if (bHasEverDetonated || CurrentDetonations > 0)
		TimeSinceDetonate += DeltaSeconds;

	if (!GetOwnerCharacter() || bHasEverDetonated)
	{
		if (Thrown)
		{
			GetItemMesh()->AttachToComponent(Thrown->GetStaticMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);	
		}
		else
		{
			if (bHasEverDetonated)
			{
				ItemMesh->SetSimulatePhysics(true);
			}
		}

		SetActorTickInterval(0.1f);
		
		return;
	}

	//DrawDebugSphere(GetWorld(), GetActorLocation(), 10.0f, 5, FColor::Blue, false, 0.05f, 0, 0.0f);
	
	if (bUsed)
	{
		SetActorLocation(GetItemLocation(), false, nullptr, ETeleportType::TeleportPhysics);

		//ULog::ObjectName(GetAttachParentActor());
		//ULog::Info(GetAttachParentSocketName().ToString());
		// Apply glass damage, breakable glass doesn't respond well to physics collision. so do a trace
		if (ItemMesh->GetComponentVelocity().Size() > 75.0f)
		{
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Destructible);

			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(this);
			if (AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(GetOwner()))
				ActorsToIgnore.Append(OwnerCharacter->GetCollisionIgnoredActors());
			else
				ActorsToIgnore.Add(GetOwner());

			FHitResult HitResult_Forward, HitResult_Backward;
			float Distance = 100.0f;
			FVector Start = ItemMesh->GetComponentLocation();
			FVector End = ItemMesh->GetComponentVelocity().GetSafeNormal();
		
			GetWorld()->LineTraceSingleByObjectType(HitResult_Forward, Start, Start + End * Distance, CollisionObjectQueryParams);
			GetWorld()->LineTraceSingleByObjectType(HitResult_Backward, Start, Start - End * Distance, CollisionObjectQueryParams);

			//DrawDebugLine(GetWorld(), HitResult_Forward.TraceStart, HitResult_Forward.TraceEnd, FColor::Red);
			//DrawDebugLine(GetWorld(), HitResult_Backward.TraceStart, HitResult_Backward.TraceEnd, FColor::Green);

			if (HitResult_Forward.IsValidBlockingHit())
			{
				if (ABreakableGlass* Glass = Cast<ABreakableGlass>(HitResult_Forward.GetActor()))
				{
					Glass->ConvertHitAndExecute(HitResult_Forward, 1000.0f);
				}
			}
			else if (HitResult_Backward.IsValidBlockingHit())
			{
				if (ABreakableGlass* Glass = Cast<ABreakableGlass>(HitResult_Backward.GetActor()))
				{
					Glass->ConvertHitAndExecute(HitResult_Backward, 1000.0f);
				}
			}
		}
		
		if (GetOwnerCharacter() && GetOwnerCharacter()->IsLocalPlayer())
		{
		}
		else if (TargetLocation != FVector::ZeroVector && IsReplicatingMovement())
		{
			// Interpolate faster if we're a server, since the server sends the physics state to other clients who then interpolate as well. Don't wanna lag behind too much
			float InterpolationSpeed = HasAuthority() ? InterpStrength * 2.5f : InterpStrength;
			ItemMesh->SetWorldLocation(FMath::VInterpTo(ItemMesh->GetComponentLocation(), TargetLocation, DeltaSeconds, InterpolationSpeed), false, nullptr, ETeleportType::TeleportPhysics);
			ItemMesh->SetWorldRotation(FMath::RInterpTo(ItemMesh->GetComponentRotation(), TargetRotation, DeltaSeconds, InterpolationSpeed), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	DetonationLight->SetIntensity(FMath::FInterpTo(DetonationLight->Intensity, 0.0f, DeltaSeconds, DetonationFlashInterp));
	
	if (bUsed)
	{
		for (const FName& Bone : HideBonesOnUsed)
		{
			GetItemMesh()->HideBoneByName(Bone, PBO_None);
		}

		if (bEnabledWeaponFovShader)
		{
			DisableWeaponFovShader();
		}

		if (!bForceNoSimulateGrenadePathOnThrow)
		{
 			if (CompletePath.Num() > 0)
 			{
				if (CompletePath.IsValidIndex(PathIdx) && !bHasEverDetonated)
				{
					ItemMesh->SetSimulatePhysics(false);
					ItemMesh->SetVisibility(true);
					
					SetActorLocation(UKismetMathLibrary::VInterpTo_Constant(GetActorLocation(), CompletePath[PathIdx], DeltaSeconds, 1500.0f), false, nullptr, ETeleportType::ResetPhysics); // todo use grenade speed
					AddActorWorldRotation(FRotator(25.0f * DeltaSeconds));
					
					if (FVector::Distance(ItemMesh->GetComponentLocation(), CompletePath[PathIdx]) < 25.0f)
					{
						if (PathIdx == BouncePt1 || PathIdx == BouncePt2 || PathIdx == BouncePt3)
						{
							ItemMesh->AddWorldRotation(FRotator(FMath::RandRange(0.0f, 360.0f)));
							MaxGrenadeSpeed *= 0.77f;
							GrenadeSpeed = MaxGrenadeSpeed * 0.25f;
							PlayFMODBounceSound();
						}

						if (PathIdx == CompletePath.Num()-1)
						{
							if (bDetonateAtEndofPath && CurrentDetonations == 0)
							{
								Detonate();
							}
							
							if (ThirdBounceHit.ImpactPoint != FVector::ZeroVector)
							{
								if (GrenadeBounceEffect)
								{
									UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, ThirdBounceHit.ImpactPoint, ThirdBounceHit.ImpactNormal.Rotation());
								}
							}
						}
						else if (PathIdx == BouncePt1)
		                {
							if (FirstBounceHit.ImpactPoint != FVector::ZeroVector)
							{
								if (GrenadeBounceEffect)
								{
									UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, FirstBounceHit.ImpactPoint, FirstBounceHit.ImpactNormal.Rotation());
								}
							}
		                }
						else if (PathIdx == BouncePt2)
		                {
                			if (SecondBounceHit.ImpactPoint != FVector::ZeroVector)
                			{
                				if (GrenadeBounceEffect)
                				{
                					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, SecondBounceHit.ImpactPoint, SecondBounceHit.ImpactNormal.Rotation());
                				}
                			}
		                }
						
						PathIdx++;
					}
				}
				else if (PathIdx >= CompletePath.Num()-1 || bHasEverDetonated)
				{
					if (!ItemMesh->IsSimulatingPhysics())
					{
						SetActorEnableCollision(true);
						ItemMesh->SetCollisionProfileName("PhysicsItem");
						ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						ItemMesh->SetSimulatePhysics(true);
						ItemMesh->SetCollisionResponseToChannel(ECC_ITEM, ECR_Block);

						if (CompletePath.IsValidIndex(CompletePath.Num() - 1) && CompletePath.IsValidIndex(CompletePath.Num() - 2))
						{
							const FVector LastPathPoint = CompletePath[CompletePath.Num() - 1];
							const FVector SecondLastPathPoint = CompletePath[CompletePath.Num() - 2];
							const FVector ImpulseDirection = (LastPathPoint - SecondLastPathPoint).GetSafeNormal2D();
							ItemMesh->AddImpulse(ImpulseDirection * 75.0f);
						}
					}
				}
 				
 				/*
 				const bool bShouldWait = !GetOwnerCharacter()->IsA(ACyberneticCharacter::StaticClass()) || (GetOwnerCharacter()->IsA(ACyberneticCharacter::StaticClass()) && bAIThrowComplete);
				if (CompletePath.IsValidIndex(PathIdx) && !bHasEverDetonated && bShouldWait)
				{
					ItemMesh->SetSimulatePhysics(false);
					ItemMesh->SetVisibility(true);
		
					GrenadeSpeed = FMath::FInterpConstantTo(GrenadeSpeed, MaxGrenadeSpeed, DeltaSeconds, MaxGrenadeSpeed);
					ItemMesh->SetWorldLocation(UKismetMathLibrary::VInterpTo_Constant(ItemMesh->GetComponentLocation(), CompletePath[PathIdx], DeltaSeconds, GrenadeSpeed), false, nullptr, ETeleportType::TeleportPhysics);
					ItemMesh->AddWorldRotation(FRotator(15.0f * DeltaSeconds));
					
					if (FVector::Distance(ItemMesh->GetComponentLocation(), CompletePath[PathIdx]) < 10.0f)
					{
						if (PathIdx >= BouncePt1)
						{
							ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						}
						
						if (PathIdx == 0)
						{
							GrenadeSpeed = MaxGrenadeSpeed * 0.25f;
						}
						
						if (PathIdx == BouncePt1 || PathIdx == BouncePt2 || PathIdx == BouncePt3)
						{
							ItemMesh->AddWorldRotation(FRotator(FMath::RandRange(0.0f, 360.0f)));
							MaxGrenadeSpeed *= 0.77f;
							GrenadeSpeed = MaxGrenadeSpeed * 0.25f;
							PlayFMODBounceSound();
						}
		
						if (PathIdx == CompletePath.Num()-1)
						{
							if (bDetonateAtEndofPath && CurrentDetonations == 0)
							{
								Detonate();
							}
							
							if (ThirdBounceHit.ImpactPoint != FVector::ZeroVector)
							{
								if (GrenadeBounceEffect)
								{
									UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, ThirdBounceHit.ImpactPoint, ThirdBounceHit.ImpactNormal.Rotation());
								}
							}
						}
						else if (PathIdx == BouncePt1)
		                {
							if (FirstBounceHit.ImpactPoint != FVector::ZeroVector)
							{
								if (GrenadeBounceEffect)
								{
									UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, FirstBounceHit.ImpactPoint, FirstBounceHit.ImpactNormal.Rotation());
								}
							}
		                }
						else if (PathIdx == BouncePt2)
		                {
                			if (SecondBounceHit.ImpactPoint != FVector::ZeroVector)
                			{
                				if (GrenadeBounceEffect)
                				{
                					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeBounceEffect, SecondBounceHit.ImpactPoint, SecondBounceHit.ImpactNormal.Rotation());
                				}
                			}
                			
		                }
						
						//V_LOGM(LogReadyOrNot, "Grenade Hit pathIdx %s on [IsHost? %s]", *FString::FromInt(pathIdx), (GetLocalRole() == ROLE_Authority ? "true" : "false"));
						PathIdx++;
					}
				}
				else if (PathIdx >= CompletePath.Num()-1 || bHasEverDetonated)
				{
					if (!ItemMesh->IsSimulatingPhysics())
					{
						SetActorEnableCollision(true);
						ItemMesh->SetCollisionProfileName("PhysicsItem");
						ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
						ItemMesh->SetSimulatePhysics(true);
					}
				}
 				*/
 			}
		}
	}

	if (bGrenadeReleased && bThrowStarted)
	{
		ActivationElapsedTime += DeltaSeconds;

		if (ActivationElapsedTime > ActivationTime && !bActivated)
		{
			if (UFMODAudioComponent* comp = UFMODBlueprintStatics::PlayEventAttached(ActivationSound, ItemMesh, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, false, true, true))
			{
				comp->SetParameter("IsOutside", IsOutside() ? 1.0f : 0.0f);
			}
			
			bActivated = true;
		}
	}

	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter)
	{
		if (GetLocalRole() >= ROLE_Authority && (!OwnerCharacter || OwnerCharacter->IsDeadOrUnconscious()) && bPinPulled && !bStartedDetonating)
		{
			if (!bDeadDropped)
			{
				bDeadDropped = true;
				StartDetonationTimer();
				Multicast_OnDeadDropped();
			}
		}

		if (OwnerCharacter->bPrimed && AnimationData)
		{
			PrimeTime += DeltaSeconds;
			if (OwnerCharacter->bOverarmThrow)
			{
				if (PrimeTime > AnimationData->Throw.Body_FP->GetPlayLength())
				{
					SetFullyPrimed();
				}
			}
			else
			{
				if (PrimeTime > AnimationData->ThrowUnderarm.Body_FP->GetPlayLength())
				{
					SetFullyPrimed();
				}
			}
		}
		else
		{
			bFullyPrimed = false;
			PrimeTime = 0.0f;
		}

		if (bGrenadeReleased && !bThrowStarted)
		{
			if (OwnerCharacter && AnimationData)
			{
				if (bCanThrowGrenade)
				{
					bUsed = true;
					ThrownBy = OwnerCharacter;
					
					if (GetLocalRole() == ROLE_Authority)
					{
						OnRep_GrenadeUsed();
					}

					if (APlayerCharacter* pc = Cast<APlayerCharacter>(OwnerCharacter))
						pc->Client_AutoSelectNewQuickthrowItem(this);
					
					if (bFastThrowing)
					{
						DoThrowFast();
					}
					else
					{
						DoThrowOverarm();
					}
				}
			}
		}
	}
	
	for (UParticleSystemComponent* comp : SpawnedParticles)
	{
		if (comp)
		{
			comp->SetWorldLocation(ItemMesh->GetComponentLocation());
		}
	}
}

void ABaseGrenade::OnItemPrimaryUse()
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;

	if (OwnerCharacter->IsAnimationBlocking())
		return;
	
	if (bUsed)
		return;
	
	if (OwnerCharacter->bPrimed)
		return;

	if (bGrenadeReleased)
		return;

	bForceNoSimulateGrenadePathOnThrow = true;

	OwnerCharacter->bPrimed = true;
	
	bGrenadeReleased = false;

	GetItemMesh()->MoveIgnoreActors.Add(GetOwner());

	Server_SetThrowOverarm(true, false);
	
	if (AnimationData)
	{
		if (bFastThrowOnceEquipped)
		{
			if (!OwnerCharacter->bIsCrouched)
			{
				if (!AnimationData->QuickThrow_PinPull.Body_FP)
				{
					PlayItemAnimation(AnimationData->PullPin);
				}
				else
				{
					PlayItemAnimation(AnimationData->QuickThrow_PinPull);
				}
			}
			else
			{
				if (!AnimationData->Crouch_QuickThrow_PinPull.Body_FP)
				{
					PlayItemAnimation(AnimationData->Crouch_PullPin);
				}
				else
				{
					PlayItemAnimation(AnimationData->Crouch_QuickThrow_PinPull);
				}
			}
		}
		else
		{
			PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_PullPin : AnimationData->PullPin);
		}
	}
	Super::OnItemPrimaryUse();
}

void ABaseGrenade::OnItemPrimaryUseEnd()
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;
	
	if (!OwnerCharacter->bPrimed)
		return;
	
	Super::OnItemPrimaryUseEnd();
	
	bGrenadeReleased = true;
	bNoPickup = true;

	DetonationStimuliComp->RegisterWithPerceptionSystem();
	DetonationStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	DetonationStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
}

void ABaseGrenade::OnItemSecondaryUsed()
{
	if (IsBlockingAnimationPlaying())
		return;

	GetItemMesh()->MoveIgnoreActors.Add(GetOwner());
	bGrenadeReleased = false;

	if (bUsed)
		return;

	AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerCharacter->bPrimed = true;
		Server_SetThrowOverarm(false, bFastThrowOnceEquipped);

		if (AnimationData)
		{
			PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_PullPinUnderarm : AnimationData->PullPinUnderarm);
		}
		Super::OnItemSecondaryUsed();
	}
}

void ABaseGrenade::OnItemEndSecondaryUse()
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter->bPrimed)
		return;

	Super::OnItemEndSecondaryUse();
	
	bGrenadeReleased = true;
	bNoPickup = true;

	DetonationStimuliComp->RegisterWithPerceptionSystem();
	DetonationStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	DetonationStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
}

void ABaseGrenade::OnItemUseComplete()
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter)
	{
		AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(OwnerCharacter->GetPlayerState());
		if (ps)
		{
			ps->GrenadesThrown += 1;
		}
		
		bForceNoSimulateGrenadePathOnThrow = !OwnerCharacter->IsA(ACyberneticCharacter::StaticClass());

		// Holster the last equipped item.
		OwnerCharacter->OnQuickThrowEnd.Broadcast(this);

		if (!GetOwnerPlayerCharacter())
		{
			Server_ThrowGrenade(OwnerCharacter->bOverarmThrow, OwnerCharacter->GetControlRotation().Vector());
			Throw(true, OwnerCharacter->bOverarmThrow, OwnerCharacter->GetControlRotation().Vector());
		}
		else
		{
			FVector ThrowDirection = OwnerCharacter->GetControlRotation().Vector();
			FVector ThrowStart = ItemMesh->GetComponentLocation();
			if (const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OwnerCharacter))
			{
				FVector CameraDirection = PlayerCharacter->GetControlRotation().Vector();

				FVector CameraLocation = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
				FVector GrenadeLocation = ItemMesh->GetComponentLocation();

				// Our throw start should line up with the player's view precisely to avoid it veering in a way unexpected to the player
				ThrowStart = FMath::ClosestPointOnInfiniteLine(CameraLocation, CameraLocation + CameraDirection, ThrowStart);
				ThrowDirection = CameraDirection;
			}
			
			Server_ThrowGrenade(OwnerCharacter->bOverarmThrow, ThrowDirection, ThrowStart);
			Throw(true, OwnerCharacter->bOverarmThrow, ThrowDirection, ThrowStart);
		}

		APlayerCharacter* OwnerPlayer = Cast<APlayerCharacter>(OwnerCharacter);
		if (OwnerPlayer)
		{
			OwnerPlayer->bStartedQuickThrow = false;
			OwnerPlayer->bTryEndQuickThrow = false;
			OwnerPlayer->bQuickThrowing = false;
		}
	}

	// Return bounds back to standard now
	ItemMesh->SetBoundsScale(1.f);
}

void ABaseGrenade::Server_ThrowGrenade_Implementation(const bool bOverarmThrow, const FVector ThrowDirection, FVector ThrowStart)
{
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		OwnerCharacter->bPrimed = false;
		
		SetInstigator(GetOwnerCharacter());

		bForceNoSimulateGrenadePathOnThrow = !OwnerCharacter->IsA(ACyberneticCharacter::StaticClass());
		Throw(false, bOverarmThrow, ThrowDirection, ThrowStart);

		//Multicast_GrenadeThrow(bOverarmThrow, ThrowDirection);

		StartDetonationTimer();
	}
}

void ABaseGrenade::Multicast_GrenadeThrow_Implementation(bool bOverarmThrow, FVector ThrowDirection, FVector ThrowStart)
{
	Throw(false, bOverarmThrow, ThrowDirection, ThrowStart);
}

bool ABaseGrenade::PlayDraw(bool bDrawFirst)
{
	if (Super::PlayDraw(false))
	{
		// Increase the bounds slightly so the mesh isn't culled while we're holding it and close to a wall
		ItemMesh->SetBoundsScale(3.f);
		
		return true;
	}
	
	return false;
}

void ABaseGrenade::Multicast_OnDeadDropped_Implementation()
{
	bDropping = true;
	bUsed = true;
	bGrenadeReleased = true;
	bThrowStarted = true;

	DetonationStimuliComp->RegisterWithPerceptionSystem();
	DetonationStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	DetonationStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	GetItemMesh()->MoveIgnoreActors.Add(GetOwner());
	GetItemMesh()->MoveIgnoreActors.Add(GetOwner());

	ItemMesh->SetCollisionProfileName("Item");
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	ItemMesh->IgnoreActorWhenMoving(GetOwner(), true);

	ItemMesh->SetCollisionProfileName("Item");
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	ItemMesh->IgnoreActorWhenMoving(GetOwner(), true);
}

void ABaseGrenade::Throw(const bool bLocalOnly, const bool bOverarmThrow, const FVector& ThrowDirection, const FVector& ThrowStart)
{
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
	
	if (!bLocalOnly && IsLocallyControlled())
		return;

	if (bLocalOnly && !IsLocallyControlled())
		return;

	if (!GetOwnerCharacter())
		return;

	bDropping = true;
	bUsed = true;

	ThrownBy = GetOwnerCharacter();

	if (!bForceNoSimulateGrenadePathOnThrow)
	{
		if (bOverarmThrow)
		{
			FullySimulateGrenadePath(ThrowDirection + FVector(0, 0, 0.175));
		}
		else
		{
			FullySimulateGrenadePath(ThrowDirection);
		} 
	}
	else
	{
		// Prevent grenade from going through geometry
		FVector AdjustedThrowStart = ThrowStart;
		{
			const FVector StartTrace = GetItemMesh()->GetComponentLocation();
			
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);

			FHitResult WallHitTest;
			if (GetWorld()->LineTraceSingleByObjectType(WallHitTest, StartTrace - ThrowDirection.GetSafeNormal() * 100.0f, StartTrace + ThrowDirection.GetSafeNormal() * 100.0f, CollisionObjectQueryParams, GetOwnerCharacter()->GetCollisionQueryParameters()))
			{
				AdjustedThrowStart = WallHitTest.Location;
			}
		}

		EnableGrenadePhysics();

		//DrawDebugSphere(GetWorld(), AdjustedThrowStart, 10.0f, 4, FColor::Yellow, false, 1.0f);
		
		// Force our world location for consistency and ensure we keep no physics state
		ItemMesh->SetWorldLocation(AdjustedThrowStart, false, nullptr, ETeleportType::TeleportPhysics); // DO NOT CHANGE THIS, keep it TeleportPhysics or else grenades will go through walls and people will be sad
		ItemMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		ItemMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

		// Calculate and apply final impulse including vertical compensation
		FVector FinalImpulse = ThrowDirection * ThrowImpulse + FVector::UpVector * UpImpulse;
		ItemMesh->AddImpulse(FinalImpulse);

#if !UE_BUILD_SHIPPING
		if (CVarDrawGrenadeDebug.GetValueOnGameThread() != 0)
		{
			// Draw both the player perceived throw direction and the actual (with up impulse) impulse
			FinalImpulse.Normalize();
			DrawDebugLine(GetWorld(), ThrowStart, ThrowStart + ThrowDirection * 150.0f, FColor::Magenta, false, 10.0f);
			DrawDebugLine(GetWorld(), ThrowStart, ThrowStart + FinalImpulse * 150.0f, FColor::Green, false, 10.0f);
		}
#endif
	}

	GetOwnerCharacter()->GetInventoryComponent()->RemoveInventoryItem(this, false);
	
	OnRep_GrenadeUsed();

	DetonationStimuliComp->RegisterWithPerceptionSystem();
	DetonationStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	DetonationStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
	
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 1000.0f, ThrownTag);

	OnGrenadeThrown.Broadcast(this);

	SetRootComponent(ItemMesh);
	ItemMesh->bReplicatePhysicsToAutonomousProxy = true;
	SetReplicateMovement(true);

	// If we're a client that has thrown the grenade, setup a timer to send position data at a set rate, not from tick
	if (bLocalOnly && !HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(TH_SendPositionUpdatesToServer, this, &ABaseGrenade::SendLocationToServer, ClientReplicationFrequency, true);
	}

	LOG_CLASS_FUNC
}

void ABaseGrenade::SendLocationToServer()
{
	Server_UpdateThrowPosition(ItemMesh->GetComponentLocation(), ItemMesh->GetComponentRotation(), ItemMesh->GetPhysicsLinearVelocity());

	// if linear velocity is sufficiently low, assume it's come to rest and stop sending updates to server
	if (ItemMesh->GetPhysicsLinearVelocity().Size() <= 50.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(TH_SendPositionUpdatesToServer);
	}
}

void ABaseGrenade::EnableGrenadePhysics()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorEnableCollision(true);
	DisableWeaponFovShader();
	
	ItemMesh->SetCollisionProfileName("PhysicsItem");
	ItemMesh->SetCollisionResponseToChannel(ECC_ITEM, ECR_Ignore);
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetMobility(EComponentMobility::Movable);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	
	ItemMesh->IgnoreActorWhenMoving(this, true);
	ItemMesh->IgnoreActorWhenMoving(GetOwner(), true);

	if (GetOwnerCharacter())
	{
		for (AActor* IgnoredActor : GetOwnerCharacter()->GetCollisionIgnoredActors())
			ItemMesh->IgnoreActorWhenMoving(IgnoredActor, true);
	}

	ItemMesh->SetCastShadow(true);
	ItemMesh->SetUseCCD(true);
	ItemMesh->SetNotifyRigidBodyCollision(true);
	
	// We don't have a feel for weight in-game, all grenades should have the same weight so they behave as expected
	ItemMesh->SetMassOverrideInKg(NAME_None, 0.16f);
}

// Should quantize the inputs at some point
void ABaseGrenade::Server_UpdateThrowPosition_Implementation(FVector Position, FRotator Rotation, FVector Velocity)
{
	TargetLocation = Position;
	TargetRotation = Rotation;

	if (FVector::Distance(ItemMesh->GetComponentLocation(), Position) > MaxDistanceDifference)
	{
		ItemMesh->SetWorldLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		ItemMesh->SetWorldRotation(TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	ItemMesh->SetPhysicsLinearVelocity(Velocity);

	// if linear velocity received from client is sufficiently low, assume it's come to rest and stop replicating movement
	if (ItemMesh->GetPhysicsLinearVelocity().Size() <= 50.f)
	{
		SetReplicateMovement(false);
	}
}

bool ABaseGrenade::Server_UpdateThrowPosition_Validate(FVector Position, FRotator Rotation, FVector Velocity)
{
	return true;
}

void ABaseGrenade::FullySimulateGrenadePath(FVector ThrowDirection, FVector ForcedStartPoint)
{
	if (!GetOwnerCharacter())
		return;
	
	FirstBouncePath.Empty(10);
	SecondBouncePath.Empty(10);
	ThirdBouncePath.Empty(10);
	
	// fully simulate the grenade path as grenades are known to go through walls
	TArray<AActor*> IgnoredActors = GetIgnoredActorsForThrow();

	EDrawDebugTrace::Type DebugTrace = EDrawDebugTrace::None;
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bDrawGrenadePath)
			DebugTrace = EDrawDebugTrace::ForDuration;
		else
			DebugTrace = EDrawDebugTrace::None;
	}
			DebugTrace = EDrawDebugTrace::ForDuration;
	#endif
	
	FVector OutLastTraceDestination;
	FVector StartTrace = ForcedStartPoint == FVector::ZeroVector ? ItemMesh->GetComponentLocation() : ForcedStartPoint;
	UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), FirstBounceHit, FirstBouncePath, OutLastTraceDestination, StartTrace, ThrowDirection * ThrowDistance, true, 5.0f, ECC_PROJECTILE, true, IgnoredActors, DebugTrace, 3.0f);

	#if !UE_BUILD_SHIPPING
	if (DebugTrace != EDrawDebugTrace::None)
	{
		DrawDebugBox(GetWorld(), StartTrace, FVector(10.0f), FColor::Yellow, false, 4.0f);
		DrawDebugLine(GetWorld(), StartTrace - ThrowDirection.GetSafeNormal() * 100.0f, StartTrace + ThrowDirection.GetSafeNormal() * 100.0f, FColor::Yellow, false, 4.0f);
	}
	#endif
	
	if (!GetOwnerCharacter()->IsA(ACyberneticCharacter::StaticClass()))
	{
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);

		FHitResult WallHitTest;
		if (GetWorld()->LineTraceSingleByObjectType(WallHitTest, StartTrace - ThrowDirection.GetSafeNormal() * 100.0f, StartTrace + ThrowDirection.GetSafeNormal() * 100.0f, CollisionObjectQueryParams, GetOwnerCharacter()->GetCollisionQueryParameters()))
		{
			FirstBouncePath.Empty(1);
			FirstBouncePath.Add(WallHitTest.Location);
			
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
				UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), SecondBounceHit, SecondBouncePath, OutLastTraceDestinationBounce, FirstBouncePath.Last(), (ThrowDirection * 150) * GrenadeBounciness, true, 5.0f, ECC_PROJECTILE, true, IgnoredActors, DebugTrace, 3.0f);

				FVector OutLastTraceDestinationBounce2;
				//FVector MirroredVector2 = UKismetMathLibrary::MirrorVectorByNormal(SecondBounceHit.TraceEnd - SecondBounceHit.TraceStart, SecondBounceHit.ImpactNormal);
				UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(GetWorld(), ThirdBounceHit, ThirdBouncePath, OutLastTraceDestinationBounce2, SecondBouncePath.Last(), (ThrowDirection * 150) * GrenadeBounciness, true, 5.0f, ECC_PROJECTILE, true, IgnoredActors, DebugTrace, 3.0f);
			}
		}
	}
    
    CompletePath.Empty(30);
    CompletePath.Append(FirstBouncePath);
    CompletePath.Append(SecondBouncePath);
    CompletePath.Append(ThirdBouncePath);

	BouncePt1 = FirstBouncePath.Num() - 1;
	BouncePt2 = BouncePt1 + FMath::Clamp(BouncePt1 + SecondBouncePath.Num() - 1, 0, SecondBouncePath.Num());
	BouncePt3 = BouncePt2 + FMath::Clamp(BouncePt2 + ThirdBouncePath.Num() - 1, 0, ThirdBouncePath.Num());

	if (IsLocallyControlled())
	{
		UpdateServerPath(CompletePath, BouncePt1, BouncePt2, BouncePt3);
	}
}

void ABaseGrenade::UpdateServerPath_Implementation(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3)
{
	bUsed = true;
	CompletePath = Path;
	BouncePt1 = Bounce1;
	BouncePt2 = Bounce2;
	BouncePt3 = Bounce3;

 	//for (int32 i = 0; i < CompletePath.Num(); i++)
 	//{
 	//	DrawDebugSphere(GetWorld(), CompletePath[i], 3.0f, 12.0f, FColor::Red, false, 10.0f, 0, 2.0f);
 	//}
}

void ABaseGrenade::Server_SetThrowOverarm_Implementation(bool bThrowOverarm, bool bQuickThrowing)
{
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		bPinPulled = true;
		OwnerCharacter->bOverarmThrow = bThrowOverarm;
	}

	
	bThrowAsQuickThrow = bQuickThrowing;
}

void ABaseGrenade::OnOverarmThrowFinished()
{
	if (GetOwnerCharacter())
		GetOwnerCharacter()->GetInventoryComponent()->EquipLastEquippedWeapon();
}

void ABaseGrenade::StunTick_Implementation(EStunType StunType)
{
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		OwnerCharacter->bPrimed = false;
	}
	bThrowStarted = false;
	bGrenadeReleased = false;
	bPinPulled = false;
	Super::StunTick_Implementation(StunType);
}

void ABaseGrenade::DoThrowUnderarm()
{
	bThrowStarted = true;

	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		OwnerCharacter->bPrimed = false;
		OwnerCharacter->bOverarmThrow = false;
		
		if (AnimationData)
		{
			PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_ThrowUnderarm : AnimationData->ThrowUnderarm);
		}
	}
}

void ABaseGrenade::DoThrowOverarm()
{
	bThrowStarted = true;
	
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		OwnerCharacter->bPrimed = false;
		OwnerCharacter->bOverarmThrow = true;
		
		if (AnimationData)
		{
			if (bThrowAsQuickThrow && AnimationData->QuickThrow_Throw.Body_FP)
			{
				PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_QuickThrow_Throw : AnimationData->QuickThrow_Throw);
			}
			else
			{
				PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_Throw : AnimationData->Throw);
			}

			if (AnimationData->Throw.Body_TP)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ABaseGrenade::OnOverarmThrowFinished, AnimationData->Throw.Body_TP->GetPlayLength() - 0.1f, false);
			}
			else
			{
				OnOverarmThrowFinished();
			}
		}
	}
}

bool ABaseGrenade::IsBlockingAnimationPlaying(const TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (!AnimationData)
		return false;
	
	if (const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		if (const APlayerCharacter* pc = GetOwnerPlayerCharacter())
		{
			if (pc->Is1PMontagePlaying(AnimationData->Throw.Body_FP) || pc->Is1PMontagePlaying(AnimationData->ThrowUnderarm.Body_FP))
			{
				if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Throw))
				{
		 			return true;
				}
			}
			
			if (pc->Is1PMontagePlaying(AnimationData->PullPin.Body_FP) || pc->Is1PMontagePlaying(AnimationData->PullPinUnderarm.Body_FP))
			{
				if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_PullPin))
				{
					return true;
				}
			}
		}

		if (OwnerCharacter->Is3PMontagePlaying(AnimationData->Throw.Body_TP) || OwnerCharacter->Is3PMontagePlaying(AnimationData->ThrowUnderarm.Body_TP))
		{
			if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Throw))
			{
				return true;
			}
		}
	}
	
	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void ABaseGrenade::OnGrenadeBounceSoundStopped()
{
	bCanPlayBounceSound = true;
}

void ABaseGrenade::StartDetonationTimer()
{
	if (CurrentDetonations > 0)
	{
		DetonationTime = ReDetonationTime;
	}
	
	bStartedDetonating = true;

	if (DetonationTime <= 0.0f)
	{
		Detonate();
	}
	else
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(DetonationTime_Handle, this, &ABaseGrenade::Detonate, DetonationTime, false);
	}
}

// DO NOT call this every frame, it is very expensive!!
bool ABaseGrenade::IsOutside()
{
	FVector StartTrace = ItemMesh->GetComponentLocation();
	FVector EndTrace = StartTrace + GetActorUpVector() * 5000.0f;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	if (GetOwnerCharacter())
	{
		CollisionParams.AddIgnoredActor(GetOwnerCharacter());
		CollisionParams.AddIgnoredActors(GetOwnerCharacter()->GetCollisionIgnoredActors());
	}

	FHitResult HitResult;
	if (!GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, CollisionParams))
	{
		return true;
	}
	
	if (AActor* HitActor = HitResult.GetActor())
	{
		if (HitActor->Tags.Contains("IsOutside") && !HitActor->IsA(AReadyOrNotCharacter::StaticClass()))
		{
			return true;
		}
	}
	
	return false;
}

void ABaseGrenade::Multicast_DetonationEffects_Implementation(FVector CalculatedForce)
{
	bHasEverDetonated = true;
	
	for (UParticleSystem* ParticleSystem : DetonationParticles)
	{
		if (ParticleSystem)
		{
			if (UParticleSystemComponent* ParticleComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, ItemMesh))
			{
				ParticleComponent->SetBoundsScale(500.0f);
				SpawnedParticles.AddUnique(ParticleComponent);
			}
		}
	}

	USoundSource* GrenadeSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DetonationFMODEvent, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
	if(GrenadeSoundSource)
	{
		GrenadeSoundSource->Attach(ItemMesh, NAME_None);
		GrenadeSoundSource->SetParameter("IsOutside", IsOutside() ? 1.0f : 0.0f);
		GrenadeSoundSource->Play();
	}
	
	// Hide the grenade if we are supposed to
	if (bHideGrenadeOnDetonate)
	{
		ItemMesh->SetCastShadow(false);
		SetItemVisibility(false);
	}

	// Trace downwards and do a decal when we hit stuff
	if (bUseDetonationDecal)
	{
		FHitResult Hit;
		FVector Start = ItemMesh->GetComponentLocation();
		//ULog::Vector(Start, false, "Item Mesh Loc: ");
		FVector End = Start - FVector::UpVector * DetonationDecalTraceDistance;

		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(this);
		CollisionQueryParams.AddIgnoredActor(Thrown);
		
		if (GetOwnerCharacter())
		{
			CollisionQueryParams.AddIgnoredActor(GetOwnerCharacter());
			CollisionQueryParams.AddIgnoredActors(GetOwnerCharacter()->GetCollisionIgnoredActors());

			TArray<UPrimitiveComponent*> PrimitiveComponents;
			GetComponents(PrimitiveComponents);
			CollisionQueryParams.AddIgnoredComponents(PrimitiveComponents);
		}

		GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionQueryParams);
		
		// Don't spawn decals on characters or grenades
		if (Hit.GetActor() && (Hit.GetActor()->IsA(AReadyOrNotCharacter::StaticClass()) || Hit.GetActor()->IsA(StaticClass())))
		{
			return;
		}

		const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		FRotator RandomRotation = Hit.Normal.Rotation();
		RandomRotation = RandomRotation.Vector().RotateAngleAxis(RandomAngle, Hit.Normal).ToOrientationRotator();

		UGameplayStatics::SpawnDecalAttached(DetonationDecal, DetonationDecalSize, Hit.GetComponent(), NAME_None, Hit.Location, RandomRotation, EAttachLocation::KeepWorldPosition);
	}
}

void ABaseGrenade::Detonate()
{
	CurrentDetonations++;
	V_LOGM(LogReadyOrNot, "Detonations: %i", CurrentDetonations);

	if (CurrentDetonations <= RedotonateCount)
	{
		StartDetonationTimer();
	}
	else if (CurrentDetonations > 1)
	{
		return;
	}

	if (CurrentDetonations == 1)
	{
		TimeSinceDetonate = 0.0f;
		
		if (!bNoMoraleDamage)
		{
			const float Damage = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.Damage");
			const float InnerRadius = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.DamageInnerRadius");
			const float OuterRadius = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.DamageOuterRadius");
			const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("GrenadeDetonateMorale.DamageFalloffCurve"));

			UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, ItemMesh->GetComponentLocation(), Damage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT}, Curve, "Grenade Detonation");
		
			//UMoraleComponent::ChangeMoraleInArea(ItemMesh->GetComponentLocation(), AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale"), 1100.0f, false, {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT});
		}
	}

	if ((CurrentDetonations >= 1 && bTriggerSFXOnRedetonate) || CurrentDetonations == 1 || (bPlayDetonationEffectsExactlyOnce && !bDetonationEffectsPlayed))
	{
		//UMoraleComponent::ChangeMoraleInArea(ItemMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 100.0f), -0.25f, 1100.0f, true, {ETeamType::TT_SUSPECT, ETeamType::TT_CIVILIAN});
		FVector CalculatedForce;

		CalculatedForce.X = FMath::FRandRange(-MaxRandomizedForceOnDetonation.X, MaxRandomizedForceOnDetonation.X);
		CalculatedForce.Y = FMath::FRandRange(-MaxRandomizedForceOnDetonation.Y, MaxRandomizedForceOnDetonation.Y);
		CalculatedForce.Z = FMath::FRandRange(-MaxRandomizedForceOnDetonation.Z, MaxRandomizedForceOnDetonation.Z);

		Multicast_DetonationEffects(CalculatedForce);

		DetonationLight->SetIntensity(DetonationFlashIntensitiy);

		bDetonationEffectsPlayed = true;
	}
	
	// Apply screen shake to all actors in radius --eez
	if (bUseScreenShake)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionScreenShake, ItemMesh->GetComponentLocation(), 0.0f, CameraShakeRadius);
	}
	
	for (int32 i = 0; i < DetonationDamage.Num(); i++)
	{
		float DamageInnerRadius;
		float DamageOuterRadius;
		if (bIncreaseDamageRadiusOverTime && (RedotonateCount - CurrentDetonations) > 0)
		{
			DamageInnerRadius = DetonationDamage[i].DamageInnerRadius / (RedotonateCount - CurrentDetonations);
			DamageOuterRadius = DetonationDamage[i].DamageOuterRadius / (RedotonateCount - CurrentDetonations);
		}
		else
		{
			DamageInnerRadius = DetonationDamage[i].DamageInnerRadius;
			DamageOuterRadius = DetonationDamage[i].DamageOuterRadius;
		}

		#if !UE_BUILD_SHIPPING
		if (CVarDrawGrenadeDebug.GetValueOnGameThread() != 0)
		{
			DrawDebugSphere(GetWorld(), GetItemMesh()->GetComponentLocation(), DamageOuterRadius, 32, FColor::Yellow, false, 30.0f);
			DrawDebugSphere(GetWorld(), GetItemMesh()->GetComponentLocation(), DamageInnerRadius, 32, FColor::Blue, false, 30.0f);
		}
		#endif
		
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		if (GetOwningAIController())
			IgnoredActors.Add(GetOwnerCharacter());

		#if !UE_BUILD_SHIPPING
		const float DamageToApply = CHECK_DEBUG_SUBSYSTEM ? DetonationDamage[i].MaxDamageOnDetonation * (DEBUG_SUBSYSTEM->bApplyGlobalDamageMultiplier_Grenades ? DEBUG_SUBSYSTEM->GlobalDamageMultiplier_Grenades : 1.0f) : 0.0f;
		#else
		const float DamageToApply = DetonationDamage[i].MaxDamageOnDetonation;
		#endif

        if (DetonationRadialForce->ImpulseStrength > 0.0f)
        {
			DetonationRadialForce->FireImpulse();
        }

		if (!DetonationDamage[i].bUseSecondTrace)
		{
			// todo(killo): i think ideally we completely redo this ourselves for better grenade behavior in general
			UGameplayStatics::ApplyRadialDamageWithFalloff(this, DamageToApply, DetonationDamage[i].MinDamageOnDetonation, GetItemLocation(), DamageInnerRadius, DamageOuterRadius, 1.0f, DetonationDamage[i].DamageType, IgnoredActors, this, GetInstigatorController(), ECC_Visibility);
		}
		else
		{
			ApplyRadialDamageWithSecondTrace(this, DetonationDamage[i].SecondTraceStartDistance, DetonationDamage[i].SecondTraceRadiusFactor, DamageToApply, DetonationDamage[i].MinDamageOnDetonation, GetItemLocation(), DamageInnerRadius, DamageOuterRadius, 1.0f, DetonationDamage[i].DamageType, IgnoredActors, this, GetInstigatorController(), ECC_Visibility);
		}
	}

	if (bDetonationTriggersStimuli)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetItemLocation(), 2.0f, this, DetonationSoundMaxRange, DetonationTag);
	}
	
	// detonation removes the hearing event
	DetonationStimuliComp->UnregisterFromSense(UAISense_Hearing::StaticClass());

	OnGrenadeDetonated.Broadcast(this);
}

void ABaseGrenade::PlayFMODBounceSound()
{
	if (!bCanPlayBounceSound)
		return;
	
	FMODBounceSoundComponent = UFMODBlueprintStatics::PlayEventAttached(FMODGrenadeBounce, GetItemMesh(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, false, true, true);

	if (FMODBounceSoundComponent)
	{
		FMODBounceSoundComponent->SetParameter("Bounce", BounceCount);
		FMODBounceSoundComponent->Play();
		FMODBounceSoundComponent->OnEventStopped.AddDynamic(this, &ABaseGrenade::OnGrenadeBounceSoundStopped);

		bCanPlayBounceSound = false;
		
		BounceCount++;

		if (BounceTag != NAME_None)
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetItemLocation(), 1.0f, this, 600.0f, BounceTag);
	}
}

bool ABaseGrenade::CanEquip(AReadyOrNotCharacter* ToCharacter) const
{
	return !bUsed && !bHasEverDetonated;
}

void ABaseGrenade::OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnPhysicsImpact(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
	
	if (!bUsed)
	{
		// Don't worry about this if the grenade hasn't been thrown.
		return;
	}

	if (ItemMesh->GetComponentVelocity().Size() < 300.0f && bHasEverDetonated)
		return;

	if (ItemMesh->GetComponentVelocity().Size() > 300.0f)
		PlayFMODBounceSound();

	//ULog::ObjectName(OtherActor);
	//DrawDebugBox(GetWorld(), Hit.Location, FVector(15.0f), FColor::Red, false, 5.0f);
	//LOG_NUMBER(ItemMesh->GetComponentVelocity().Size());
}

void ABaseGrenade::OnRep_AttachmentRep()
{
	if (bUsed)
		return;
	
	Super::OnRep_AttachmentRep();
}

TArray<AActor*> ABaseGrenade::GetIgnoredActorsForThrow() const
{
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Reserve(100);
	
	if (GetOwnerCharacter())
		IgnoredActors.Append(GetOwnerCharacter()->GetCollisionIgnoredActors());
	
	IgnoredActors.AddUnique(GetOwner());

	if (const AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		IgnoredActors += (TArray<AActor*>)GS->AllItems;
	}

	IgnoredActors += (TArray<AActor*>)USWATManager::Get(this)->SwatAI;

	return IgnoredActors;
}

void ABaseGrenade::Server_StartFastThrow_Implementation()
{
	bFastThrowing = true;
	bGrenadeReleased = true;
	bCanThrowGrenade = true;

	DetonationStimuliComp->RegisterWithPerceptionSystem();
	DetonationStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	DetonationStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
}

void ABaseGrenade::DoThrowFast()
{
	bThrowStarted = true;

	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		OwnerCharacter->bPrimed = false;
		OwnerCharacter->bOverarmThrow = true;
		
		if (AnimationData)
		{
			PlayItemAnimation(OwnerCharacter->bIsCrouched ? AnimationData->Crouch_QuickThrow_Fast : AnimationData->QuickThrow_Fast);
		}
	}
}

// Checks if component is damageable, performing extra traces if not visible from origin
// Based off GameplayStatics code
static bool ComponentIsDamageableFrom(UPrimitiveComponent* VictimComp, float SecondTraceDistance, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, ECollisionChannel TraceChannel, FHitResult& OutHitResult)
{
	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(ComponentIsVisibleFrom), true, IgnoredActor);
	LineParams.AddIgnoredActors(IgnoreActors);

	// Do a trace from origin to middle of box
	UWorld* const World = VictimComp->GetWorld();
	check(World);

	FVector const TraceEnd = VictimComp->Bounds.Origin;
	FVector TraceStart = Origin;
	if (Origin == TraceEnd)
	{
		// Tiny nudge so LineTraceSingle doesn't early out with no hits
		TraceStart.Z += 0.01f;
	}

	FVector const FakeHitLoc = VictimComp->GetComponentLocation();
	FVector const FakeHitNorm = (Origin - FakeHitLoc).GetSafeNormal(); // normal points back toward the epicenter
	FHitResult FakeHitResult = FHitResult(VictimComp->GetOwner(), VictimComp, FakeHitLoc, FakeHitNorm);

	// Only do a line trace if there is a valid channel, if it is invalid then result will have no fall off
	if (TraceChannel != ECollisionChannel::ECC_MAX)
	{
		bool bHadBlockingHit = World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, LineParams);

		// If there was a blocking hit, it will be the last one
		if (bHadBlockingHit)
		{
			if (OutHitResult.Component == VictimComp)
			{
				#if !UE_BUILD_SHIPPING
				DrawDebugLine(VictimComp->GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 4.0f);
				#endif

				// If blocking hit was the victim component, it is visible
				return true;
			}
		}
		else
		{
			#if !UE_BUILD_SHIPPING
			DrawDebugLine(VictimComp->GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 4.0f);
			#endif

			// Nothing blocking, assume victim is visible
			OutHitResult = FakeHitResult;
			return true;
		}

		// No second trace and all above hits must have been blocking but not by our victim component, fail
		if (SecondTraceDistance <= 0.0f)
			return false;

		// Our array of second trace points to check against
		const TArray<FVector> SecondTracePoints =
		{
			Origin + FVector::UpVector * SecondTraceDistance,
			Origin + FVector(1, 0, 1).GetSafeNormal() * SecondTraceDistance,
			Origin + FVector(-1, 0, 1).GetSafeNormal() * SecondTraceDistance,
			Origin + FVector(0, 1, 1).GetSafeNormal() * SecondTraceDistance,
			Origin + FVector(0, -1, 1).GetSafeNormal() * SecondTraceDistance
		};

		#if !UE_BUILD_SHIPPING
		for (const FVector& Start : SecondTracePoints)
		{
			DrawDebugLine(VictimComp->GetWorld(), Origin, Start, FColor::Green, false, 4.0f);
		}
		#endif

		for (const FVector& Start : SecondTracePoints)
		{
			// First trace from origin to our second trace point
			FHitResult SecondTraceHitResult;
			bHadBlockingHit = World->LineTraceSingleByChannel(SecondTraceHitResult, Origin, Start, TraceChannel, LineParams);

			// Start to second trace start was blocked, skip
			if (bHadBlockingHit)
				continue;

			bHadBlockingHit = World->LineTraceSingleByChannel(OutHitResult, Start, TraceEnd, TraceChannel, LineParams);
			if (bHadBlockingHit)
			{
				if (OutHitResult.Component == VictimComp)
				{
					#if !UE_BUILD_SHIPPING
					DrawDebugLine(VictimComp->GetWorld(), Start, TraceEnd, FColor::Red, false, 4.0f);
					#endif

					// If blocking hit was the victim component, it is visible
					return true;
				}
				else
				{
					// If we hit something else blocking, it's not
					continue;
				}
			}

			#if !UE_BUILD_SHIPPING
			DrawDebugLine(VictimComp->GetWorld(), Start, TraceEnd, FColor::Red, false, 4.0f);
			#endif

			// Nothing blocking, assume victim is visible
			OutHitResult = FakeHitResult;
			return true;
		}

		// All above hits must have been blocking and not by our victim component, fail
		return false;
	}
	else
	{
		UE_LOG(LogDamage, Warning, TEXT("ECollisionChannel::ECC_MAX is not valid! No falloff is added to damage"));
	}

	// Nothing blocking, assume victim is visible
	OutHitResult = FakeHitResult;
	return true;
}

// This code is mostly the same as the one in gameplay statics, but it uses our IsComponentDamageable function instead
bool ABaseGrenade::ApplyRadialDamageWithSecondTrace(const UObject* WorldContextObject, float SecondTraceDistance, float SecondTraceRadiusFactor, float BaseDamage, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, ECollisionChannel DamagePreventionChannel)
{
	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(ApplyRadialDamage), false, DamageCauser);

	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(DamageOuterRadius), SphereParams);
	}

	// Second trace max distance
	const float SecondTraceRadiusSquared = FMath::Pow(DamageOuterRadius * SecondTraceRadiusFactor, 2);

	// Collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AActor* const OverlapActor = Overlap.GetActor();

		if (OverlapActor &&
			OverlapActor->CanBeDamaged() &&
			OverlapActor != DamageCauser &&
			Overlap.Component.IsValid())
		{
			// Check if this component is too far away for the second trace
			UPrimitiveComponent* Component = Overlap.Component.Get();
			float ThisSecondTraceDistance = SecondTraceDistance;
			if (FVector::DistSquared(Component->Bounds.Origin, Origin) > SecondTraceRadiusSquared)
				ThisSecondTraceDistance = 0.0f;

			FHitResult Hit;
			if (ComponentIsDamageableFrom(Component, ThisSecondTraceDistance, Origin, DamageCauser, IgnoreActors, DamagePreventionChannel, Hit))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
				HitList.Add(Hit);
			}
		}
	}

	bool bAppliedDamage = false;

	if (OverlapComponentMap.Num() > 0)
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UDamageType>(UDamageType::StaticClass());

		FRadialDamageEvent DmgEvent;
		DmgEvent.DamageTypeClass = ValidDamageTypeClass;
		DmgEvent.Origin = Origin;
		DmgEvent.Params = FRadialDamageParams(BaseDamage, MinimumDamage, DamageInnerRadius, DamageOuterRadius, DamageFalloff);

		// call damage function on each affected actors
		for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
		{
			AActor* const Victim = It.Key();
			TArray<FHitResult> const& ComponentHits = It.Value();
			DmgEvent.ComponentHits = ComponentHits;

			Victim->TakeDamage(BaseDamage, DmgEvent, InstigatedByController, DamageCauser);

			bAppliedDamage = true;
		}
	}

	return bAppliedDamage;
}

void ABaseGrenade::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	//if (ThrownBy)
	//	OutOverrideSensedActor = ThrownBy;

	// Only care about used grenades
	if (!bUsed)
	{
		InSenseController->RemoveActorSoundSense(this, Stimulus.Tag);
		return;
	}

	const float DistanceToGrenade = FVector::Distance(Stimulus.ReceiverLocation, Stimulus.StimulusLocation);

	const FString ReactionTimeConfig = Stimulus.Tag == DetonationTag ? "GrenadeDetonateReactionTime" : "GrenadeHeardReactionTime";
	
	const float GrenadePerceptionRange = FMath::Clamp(AI_CONFIG_GET_FLOAT("GrenadePerceptionRange", 1500.0f), 100.0f, 10000.0f);
	const float GrenadeHeardReactionTime = FMath::Clamp(AI_CONFIG_GET_FLOAT(ReactionTimeConfig, 0.25f), 0.0f, 60.0f);
	const float GrenadeForgetTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("GrenadeForgetTime", 0.5f), 0.0f, 9999.0f);

	if (DistanceToGrenade <= GrenadePerceptionRange)
	{
		if (!InSenseController->IsSoundReactingToActor(this))
		{
			FActorSense GrenadeSoundSense;
			GrenadeSoundSense.Actor = this;
			GrenadeSoundSense.Tag = Stimulus.Tag;
			GrenadeSoundSense.Stimulus = Stimulus;
			GrenadeSoundSense.SenseReactionTime = InSenseController->GetReactionTime(EActorSenseType::Sound);
			GrenadeSoundSense.SenseForgetTime = GrenadeForgetTime;
			
			InSenseController->AddActorSoundSense(GrenadeSoundSense);
		}
	}
	// Out of range, forget the grenade
	else
	{
		if (InSenseController->GetSoundSenseTimeForActor(this, Stimulus.Tag) > GrenadeForgetTime)
		{
			InSenseController->RemoveActorSoundSense(this, Stimulus.Tag);
		}
	}
}

void ABaseGrenade::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	//if (ThrownBy)
	//	OutOverrideSensedActor = ThrownBy;

	// Only care about used grenades
	if (!bUsed || bHasEverDetonated)
	{
		InSenseController->RemoveActorSightSense(this, "GrenadeSeen");
		return;
	}

	const float DistanceToGrenade = FVector::Distance(Stimulus.ReceiverLocation, Stimulus.StimulusLocation);

	const float GrenadePerceptionRange = FMath::Clamp(AI_CONFIG_GET_FLOAT("GrenadePerceptionRange", 1500.0f), 100.0f, 10000.0f);
	const float GrenadeSeenReactionTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("GrenadeSeenReactionTime", 0.25f), 0.0f, 60.0f);
	const float GrenadeForgetTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("GrenadeForgetTime", 0.5f), 0.0f, 9999.0f);

	if (DistanceToGrenade <= GrenadePerceptionRange && !InSenseController->IsSightReactingToActor(this))
	{
		FActorSense GrenadeSightSense;
		GrenadeSightSense.Actor = this;
		GrenadeSightSense.Tag = "GrenadeSeen";
		GrenadeSightSense.SenseReactionTime = GrenadeSeenReactionTime;
		GrenadeSightSense.SenseForgetTime = GrenadeForgetTime;
		
		InSenseController->AddActorSightSense(GrenadeSightSense);
	}
	// Out of range, forget the Grenade
	else
	{
		if (InSenseController->GetSightSenseTimeForActor(this, "GrenadeSeen") > GrenadeForgetTime)
		{
			InSenseController->RemoveActorSightSense(this, "GrenadeSeen");
		}
	}
}

void ABaseGrenade::PostNetReceivePhysicState()
{
	// If we're the thrower and we're a client, we don't care about any of this
	APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		return;

	if (!ItemMesh->IsSimulatingPhysics())
	{
		SetRootComponent(ItemMesh);
		EnableGrenadePhysics();
	}

	const FRepMovement& ReppedMovement = GetReplicatedMovement();

	TargetLocation = ReppedMovement.Location;
	TargetRotation = ReppedMovement.Rotation;

	if (FVector::Distance(ItemMesh->GetComponentLocation(), ReppedMovement.Location) > MaxDistanceDifference)
	{
		ItemMesh->SetWorldLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		ItemMesh->SetWorldRotation(TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	ItemMesh->SetPhysicsLinearVelocity(ReppedMovement.LinearVelocity);
}

void ABaseGrenade::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	bool bEvidence = true;
	
	if (Thrower->bPrimed && !bGrenadeReleased)
	{
		Thrower->bPrimed = false;
		ThrowDistance = 0.0f;
		ThrowImpulse = 0.0f;
		OnItemUseComplete();
		bEvidence = false;
		bMarkAsEvidenceWhenNoOwner = false;
	}

	Super::OnThrownFromInventory(Thrower, bEvidence);
}
