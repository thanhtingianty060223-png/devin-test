// Copyright Void Interactive, 2021

#include "ReadyOrNotSignificanceManager.h"

#include "Engine/DemoNetDriver.h"
#include "Actors/PlayerViewActor.h"
#include "Characters/CyberneticController.h"
#include "Components/InteractableComponent.h"
#include "Characters/AI/SWATCharacter.h"
#include "Characters/PlayerCharacter.h"
#include "Characters/AI/TrailerSWATCharacter.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Tick"), STAT_SignificanceManagerTick, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Get Relevant Viewpoints"), STAT_SignificanceManagerGetRelevantViewPoints, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Perform Relevancy"), STAT_SignificanceManagerPerformRelevancy, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Is Relevant For Viewpoint"), STAT_SignificanceManagerIsRelevant, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Actor Relevant"), STAT_SignificanceManagerActorRelevant, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Actor Not Relevant"), STAT_SignificanceManagerActorNotRelevant, STATGROUP_SignificanceManager);

DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Actor Relevant ~ Tick Instantly"), STAT_SignificanceManagerActorRelevant_TickInstantly, STATGROUP_SignificanceManager);

DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Optimize Skeletal Mesh"), STAT_SignificanceManagerOptimizeSkeletalMesh, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Optimize Scene Component"), STAT_SignificanceManagerOptimizeSceneComponent, STATGROUP_SignificanceManager);

DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Recover Skeletal Mesh"), STAT_SignificanceManagerRecoverSkeletalMesh, STATGROUP_SignificanceManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Significance Manager ~ Recover Scene Component"), STAT_SignificanceManagerRecoverSceneComponent, STATGROUP_SignificanceManager);

static float GTickInterval = 0.2f;

TAutoConsoleVariable<int32> CVarRonPauseSignificance(TEXT("a.RonPauseSignificance"), 0, TEXT("PPause the significance manager"));
TAutoConsoleVariable<int32> CVarRonMakeEveryActorIrrelevant(TEXT("a.RonMakeEveryActorIrrelevant"), 0, TEXT("Make all actors insignificant in the significance manager"));

UReadyOrNotSignificanceManager* UReadyOrNotSignificanceManager::Get(const UObject* WorldContext)
{
	return WorldContext->GetWorld()->GetSubsystem<UReadyOrNotSignificanceManager>();
}

void UReadyOrNotSignificanceManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastTick += DeltaTime;
	if (TimeSinceLastTick > GTickInterval)
	{
		TimeSinceLastTick = 0.0f;
		
		SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerTick)

		#if !UE_BUILD_SHIPPING
		if (CVarRonPauseSignificance.GetValueOnAnyThread() == 1)
		{
			for (int32 i = 0; i < IrrelevantPlayerCharacters.Num(); i++)
			{
				if (IrrelevantPlayerCharacters[i])
				{
					ActorRelevant(IrrelevantPlayerCharacters[i], true);
				}
			}
			
			return;
		}

		if (CVarRonMakeEveryActorIrrelevant.GetValueOnAnyThread() == 1)
		{
			for (int32 i = 0 ; i < CharactersRelevantToSignificance.Num(); i++)
			{
				if (CharactersRelevantToSignificance[i])
				{
					ActorNotRelevant(CharactersRelevantToSignificance[i]);
				}
			}
			return;
		}
		#endif

		ExpensiveTracesThisFrame = 0;
		CheapTracesThisFrame = 0;
		ActorsMadeIrrelevantThisFrame.Empty();
		ActorsMadeRelevantThisFrame.Empty();
		RelevantViewpoints.Empty();
		GetRelevantViewpoints(RelevantViewpoints);

		int32 MaxRelevancyChecksPerFrame = 1;
		for (int32 i = 0; i < MaxRelevancyChecksPerFrame; i++)
		{
			if (MaxRelevancyChecksPerFrame > CharactersRelevantToSignificance.Num() - 1)
				MaxRelevancyChecksPerFrame = CharactersRelevantToSignificance.Num() - 1;
			
			if (!CharactersRelevantToSignificance.IsValidIndex(CurrentRelevancyIdx))
			{
				CurrentRelevancyIdx = 0;
				break;
			}
			AActor* TestActor = CharactersRelevantToSignificance[CurrentRelevancyIdx];
			CurrentRelevancyIdx++;
			
			PerformRelevancy(TestActor);
		}

		for (int32 i = 0; i < CharactersRelevantToSignificance.Num(); i++)
		{
			if (!IrrelevantPlayerCharacters.Contains(CharactersRelevantToSignificance[i]))
			{
				ACyberneticCharacter* AiCharacter = Cast<ACyberneticCharacter>(CharactersRelevantToSignificance[i]);
				if (AiCharacter)
				{
					float Dist = (GetLocalViewPoint().GetLocation() - AiCharacter->GetActorLocation()).Size();
					float TickRate = 0.0f;
					if (Dist > 3500.0f)
					{
						TickRate = 1.0f/30.0f;
					}
					else if (Dist > 2000.0f)
					{
						TickRate = 1.0f/60.0f;
					}
					AiCharacter->SetActorTickInterval(TickRate);
					//GEngine->AddOnScreenDebugMessage((int32)RelevantCharacter->GetUniqueID() + 99999, 1.0f, FColor::White, "Dist: " + FString::SanitizeFloat(Dist) + " Running " + RelevantCharacter->GetName() + " at " + FString::SanitizeFloat(1.0f / TickRate));
				}
			}
		}

		TArray<AReadyOrNotCharacter*> CachedIrrelevantCharacters = IrrelevantPlayerCharacters;
		for (int32 i = 0 ; i < CachedIrrelevantCharacters.Num(); i++)
		{
			AReadyOrNotCharacter* IrrelevantCharacter = CachedIrrelevantCharacters[i];
			if (IrrelevantCharacter)
			{
				IrrelevantCharacter->GetMesh()->bNoSkeletonUpdate = !IrrelevantCharacter->IsAny3PMontageActive();
				PerformRelevancy(IrrelevantCharacter);
			}
		}

		#if !UE_BUILD_SHIPPING
		FString ClientServerStr = GetNetMode() == NM_Client ? "[Client] " : "[Server] " ;
		GEngine->AddOnScreenDebugMessage(10069, 1.0f, FColor::Yellow, ClientServerStr + "SignfManager: CheapTraces "
			+ FString::FromInt(CheapTracesThisFrame)
			+ " ExpensiveTraces "
			+ FString::FromInt(ExpensiveTracesThisFrame));

		GEngine->AddOnScreenDebugMessage(10070, 1.0f, FColor::Yellow, ClientServerStr + "SignfManager: Characters "
			+ FString::FromInt(CharactersRelevantToSignificance.Num())
			+ " IrrelevantChars "
			+ FString::FromInt(IrrelevantPlayerCharacters.Num()));
		#endif
	}
}

TStatId UReadyOrNotSignificanceManager::GetStatId() const
{
	return GetStatID();
}

void UReadyOrNotSignificanceManager::GetRelevantViewpoints(TArray<FTransform>& OutViewpoints)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerGetRelevantViewPoints)
	
	FString ClientServerStr = GetNetMode() == NM_Client ? "[Client] " : "[Server] ";
	
	for (TActorIterator<AReadyOrNotPlayerController>It(GetWorld()); It; ++It)
	{
		AReadyOrNotPlayerController* Player = *It;
		if (!Player->GetPawnOrSpectator() || Player->bIsReplaySpectator)
			continue;
			
		FTransform t;
		FVector OutCamLocPt;
		FRotator OutCamRotPt;
		
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Player->GetViewTarget());
		if (PlayerCharacter)
		{

			
			GEngine->AddOnScreenDebugMessage(10071, 1.0f, FColor::Yellow, ClientServerStr + "Using PlayerCharacter RelevantViewPoint: " + PlayerCharacter->GetName() + " (Loc: " + PlayerCharacter->GetActorLocation().ToCompactString() + " Rot: " + PlayerCharacter->GetActorRotation().ToCompactString() + ")");
			OutCamLocPt = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
			OutCamRotPt = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentRotation();
			if (CharactersRelevantToSignificance.Contains(PlayerCharacter))
			{
				UnregisterActorWithSignificanceManager(PlayerCharacter);
			}
		else if (ASpectatePawn* SpectatePawn = Cast<ASpectatePawn>(Player->GetPawnOrSpectator()))
		{
				GEngine->AddOnScreenDebugMessage(10072, 1.0f, FColor::Yellow, ClientServerStr + "Using Spectate Pawn RelevantViewPoint: " + SpectatePawn->GetName() + " (Loc: " + SpectatePawn->GetActorLocation().ToCompactString() + " Rot: " + SpectatePawn->GetActorRotation().ToCompactString() + ")");
			OutCamLocPt = SpectatePawn->GetActorLocation();
			OutCamRotPt= Player->GetControlRotation();
		}
		} else if (Player->GetViewTarget())
		{
			GEngine->AddOnScreenDebugMessage(10073, 1.0f, FColor::Yellow, ClientServerStr + "Using View Target RelevantViewPoint: " + Player->GetViewTarget()->GetName() + " (Loc: " + Player->GetViewTarget()->GetActorLocation().ToCompactString() + " Rot: " + Player->GetViewTarget()->GetActorRotation().ToCompactString() + ")");
			OutCamLocPt = Player->GetViewTarget()->GetActorLocation();
			OutCamRotPt = Player->GetViewTarget()->GetActorRotation();
		}
		t.SetLocation(OutCamLocPt);
		t.SetRotation(OutCamRotPt.Quaternion());
		if (IsViewpointUnique(OutViewpoints, t))
		{
			OutViewpoints.Add(t);
		}
	}

	for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Swat = *It;
		if (Cast<ATrailerSWATCharacter>(Swat))
			continue;

		FTransform t;
		FVector OutCamLocPt;
		FRotator OutCamRotPt;

		Swat->GetActorEyesViewPoint(OutCamLocPt, OutCamRotPt);
		t.SetLocation(OutCamLocPt);
		t.SetRotation(OutCamRotPt.Quaternion());
		if (IsViewpointUnique(OutViewpoints, t))
		{
			OutViewpoints.Add(t);
		}
	}

	if ((GetWorld()->GetDemoNetDriver() && GetWorld()->GetDemoNetDriver()->IsPlaying()) || bWasDemoPlaying)
	{
		bWasDemoPlaying = true;
		for (TActorIterator<ASpectatorPawn>It(GetWorld()); It; ++It)
		{
			ASpectatorPawn* SpectatorPawn = *It;
			FTransform t;
			t.SetLocation(SpectatorPawn->GetActorLocation());
			t.SetRotation(SpectatorPawn->GetActorRotation().Quaternion());
			if (IsViewpointUnique(OutViewpoints, t))
			{
				GEngine->AddOnScreenDebugMessage(10073, 1.0f, FColor::Yellow, ClientServerStr + "Using View Target RelevantViewPoint: " + SpectatorPawn->GetName() + " (Loc: " + SpectatorPawn->GetActorLocation().ToCompactString() + " Rot: " + SpectatorPawn->GetActorRotation().ToCompactString() + ")");
				OutViewpoints.Add(t);
			}
		}
	}

	for (TActorIterator<APlayerViewActor>It(GetWorld()); It; ++It)
	{
		APlayerViewActor* ViewActor = *It;
		FTransform ViewPoint;
		if (ViewActor->IsViewInuse(ViewPoint))
		{
			GEngine->AddOnScreenDebugMessage(10074, 1.0f, FColor::Yellow, ClientServerStr + "Using View Camera RelevantViewPoint: " + ViewActor->GetName() + " (Loc: " + ViewActor->GetActorLocation().ToCompactString() + " Rot: " + ViewActor->GetActorRotation().ToCompactString() + ")");
			OutViewpoints.Add(ViewPoint);
		}
	}
}

bool UReadyOrNotSignificanceManager::IsViewpointUnique(TArray<FTransform>& Viewpoints, FTransform TestViewpoint)
{
	for (FTransform Viewpoint : Viewpoints)
	{
		if (Viewpoint.GetLocation() == TestViewpoint.GetLocation() && Viewpoint.GetRotation() == TestViewpoint.GetRotation())
		{
			return false;
		}
	}
	return true;
}

bool UReadyOrNotSignificanceManager::IsRelevantForViewpoint(FTransform ViewPoint, AActor* TestActor)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerIsRelevant)
	FVector Dir = ViewPoint.GetRotation().GetForwardVector();
	FVector Offset = TestActor->GetActorLocation() - ViewPoint.GetLocation();
	Offset = Offset.GetSafeNormal();
	float DotProduct = FVector::DotProduct(Dir, Offset);
	float MinDistanceAlwaysShow = 1400.0f;	
	float DistanceFromViewpoint = (ViewPoint.GetLocation() - TestActor->GetActorLocation()).Size();
	if  (DistanceFromViewpoint > MinDistanceAlwaysShow && DotProduct > 0.0f)
	{
		TArray<FVector> TestLocations;
		TestLocations.Add(TestActor->GetActorLocation() + UKismetMathLibrary::FindLookAtRotation(ViewPoint.GetLocation(), TestActor->GetActorLocation()).Vector() * -500.0f);
		TestLocations.Add(TestActor->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f) + UKismetMathLibrary::FindLookAtRotation(ViewPoint.GetLocation(), TestActor->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f)).Vector() * -500.0f);
		if (DistanceFromViewpoint < 3500.0f)
		{
			ExpensiveTracesThisFrame++;
			TestLocations.Add(TestActor->GetActorLocation() + TestActor->GetActorForwardVector() * 200.0f);
			TestLocations.Add(TestActor->GetActorLocation() + TestActor->GetActorForwardVector() * -200.0f);
			TestLocations.Add(TestActor->GetActorLocation() + TestActor->GetActorRightVector() * 200.0f);
			TestLocations.Add(TestActor->GetActorLocation() + TestActor->GetActorRightVector() * -200.0f);
			TestLocations.Add(TestActor->GetActorLocation() + TestActor->GetActorUpVector() * 100.0f);
		} else
		{
			CheapTracesThisFrame++;
		}
		
		FHitResult HitResult;
		
		FCollisionResponseParams ResponseParams;
		for (FVector loc : TestLocations)
		{
			// add a small buffer so that if we're up against a wall we become visible
			FVector ViewPointBuffer = ViewPoint.GetLocation() + UKismetMathLibrary::FindLookAtRotation(ViewPoint.GetLocation(), loc).Vector() * MinDistanceAlwaysShow*0.5f;
			FVector LocBuffer = loc + UKismetMathLibrary::FindLookAtRotation(ViewPoint.GetLocation(), loc).Vector() * MinDistanceAlwaysShow*-0.5f;
			GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointBuffer, LocBuffer, ECC_Visibility, QueryParams, ResponseParams);
			//DrawDebugLine(GetWorld(), ViewPoint.GetLocation(), loc, FColor::Orange);
			if (!HitResult.bBlockingHit)
			{
				return true;
			}
		}
		return false;
	}
	return (DotProduct > 0.0f || DistanceFromViewpoint < MinDistanceAlwaysShow) ;
}

FTransform UReadyOrNotSignificanceManager::GetLocalViewPoint()
{
	AReadyOrNotPlayerController* Player = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	if (!Player)
		return FTransform();
	if (!Player->GetPawnOrSpectator())
		return FTransform();
			
	FTransform t;
	FVector OutCamLocPt;
	FRotator OutCamRotPt;
	
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Player->GetViewTarget());
	if (PlayerCharacter)
	{
		OutCamLocPt = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
		OutCamRotPt = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentRotation();
	}
	else if (ASpectatePawn* SpectatePawn = Cast<ASpectatePawn>(Player->GetPawnOrSpectator()))
	{
		OutCamLocPt = SpectatePawn->GetActorLocation();
		OutCamRotPt= Player->GetControlRotation();
	} else if (Player->GetViewTarget())
	{
		OutCamLocPt = Player->GetViewTarget()->GetActorLocation();
		OutCamRotPt = Player->GetViewTarget()->GetActorRotation();
	}
	t.SetLocation(OutCamLocPt);
	t.SetRotation(OutCamRotPt.Quaternion());
	return t;
}

void UReadyOrNotSignificanceManager::ForceActorRelevant(AActor* Actor)
{
	Get(Actor)->ActorRelevant(Actor, true);
}

void UReadyOrNotSignificanceManager::ForceActorNotRelevant(AActor* Actor)
{
	Get(Actor)->ActorNotRelevant(Actor);
}

bool UReadyOrNotSignificanceManager::IsActorRelevant(const AActor* Actor)
{
	if (const AReadyOrNotCharacter* PlayerCharacter = Cast<AReadyOrNotCharacter>(Actor))
	{
		return PlayerCharacter->bIsRelevant;
	}
	
	return !Get(Actor)->ActorsMadeIrrelevant.Contains(Actor);
}

void UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(AActor* Actor)
{
	if (!Actor)
		return;

	#if !UE_BUILD_SHIPPING
	if (CVarRonPauseSignificance.GetValueOnAnyThread() == 1)
	{
		return;
	}
	#endif

	AReadyOrNotCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor);
	if (PlayerCharacter && PlayerCharacter->IsLocalPlayer())
		return;

	UReadyOrNotSignificanceManager* s = Get(Actor);
	
	if (!Actor->Tags.Contains("NoRelevancy"))
	{
		if (Cast<AReadyOrNotCharacter>(Actor))
		{
			s->CharactersRelevantToSignificance.AddUnique(Cast<AReadyOrNotCharacter>(Actor));
		}
		else
		{
			s->ActorsRelevantToSignificance.AddUnique(Actor);
		}
	}
	
	TArray<AActor*> IgnoredActors;
	Actor->GetAttachedActors(IgnoredActors);
	
	s->QueryParams.AddIgnoredActor(Actor);
	s->QueryParams.AddIgnoredActors(IgnoredActors);
	s->ActorsMadeIrrelevant.Add(Actor);
	s->ActorRelevant(Actor, true);
}

void UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(AActor* Actor)
{
	if (!Actor)
		return;

	UReadyOrNotSignificanceManager* s = Get(Actor);
	
	if (s->ActorsRelevantToSignificance.Contains(Actor))
	{
		s->ActorRelevant(Actor, true);
		s->ActorsRelevantToSignificance.Remove(Actor);
	}
	
	if (s->CharactersRelevantToSignificance.Contains(Actor))
	{
		s->ActorRelevant(Actor, true);
		s->CharactersRelevantToSignificance.Remove(Cast<AReadyOrNotCharacter>(Actor));
	}
}

void UReadyOrNotSignificanceManager::PerformRelevancy(AActor* TestActor)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerPerformRelevancy)
	
	if (!TestActor)
		return;

	// Actor can't be tested for relevancy more than once per frame
	if (ActorsMadeIrrelevantThisFrame.Contains(TestActor))
		return;

	if (ActorsMadeRelevantThisFrame.Contains(TestActor))
		return;
	
	bool bRelevantToAnyViewpoint = false;
	for (const FTransform& View : RelevantViewpoints)
	{
		if (IsRelevantForViewpoint(View, TestActor))
		{
			bRelevantToAnyViewpoint = true;
			break;
		}
	}

	// is below the kill Z?
	const bool bBelowKillZ = TestActor->GetActorLocation().Z < GetWorld()->GetWorldSettings()->KillZ;

	bool bDeactivated = false;
	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(TestActor))
		bDeactivated = AI->bDeactivated;
	
	if (!bRelevantToAnyViewpoint || bBelowKillZ || bDeactivated)
	{
		ActorNotRelevant(TestActor);
		ActorsMadeIrrelevantThisFrame.Add(TestActor);
	}
	else
	{
		ActorRelevant(TestActor, false);
		ActorsMadeRelevantThisFrame.Add(TestActor);
	}
}

void UReadyOrNotSignificanceManager::ActorRelevant(AActor* InActor, const bool bUnregister)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerActorRelevant)
	
	if (!InActor)
		return;

	// Can't make actors more relevant
	if (ActorsMadeRelevantThisFrame.Contains(InActor))
	{
		return;
	}

	OnActorRelevancyChanged.Broadcast(InActor, true);
	
	ActorsMadeIrrelevant.Remove(InActor);

	AReadyOrNotCharacter* RelevantCharacter = Cast<AReadyOrNotCharacter>(InActor);

	float TickRate = 0.0f;

	if (RelevantCharacter)
	{
		RelevantCharacter->bDisableInteraction = false;
		RelevantCharacter->GetMesh()->bNoSkeletonUpdate = false;
		for (ABaseItem* Item : RelevantCharacter->GetInventoryComponent()->GetInventoryItems())
		{
			if (Item)
			{
				ActorRelevant(Item, bUnregister);
			}
		}

		RelevantCharacter->bIsRelevant = true;
		IrrelevantPlayerCharacters.Remove(RelevantCharacter);

		ACyberneticController* CyberneticController = Cast<ACyberneticController>(RelevantCharacter->GetController());
		if (CyberneticController)
		{
			CyberneticController->SetActorTickEnabled(false);
			CyberneticController->SetActorTickInterval(TickRate);
			CyberneticController->SetActorTickEnabled(true);
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerActorRelevant_TickInstantly)

		TArray<UActorComponent*> ActorComps;
		InActor->GetComponents(USkeletalMeshComponent::StaticClass(), ActorComps);

		// Tick Instantly!
		InActor->SetActorTickEnabled(false);
		InActor->SetActorTickInterval(TickRate);
		InActor->SetActorTickEnabled(true);

		if (UCharacterMovementComponent* MovementComponent =  Cast<UCharacterMovementComponent>(InActor->GetComponentByClass(UCharacterMovementComponent::StaticClass())))
		{
			MovementComponent->SetComponentTickEnabled(false);
			MovementComponent->SetComponentTickInterval(TickRate);
			MovementComponent->SetComponentTickEnabled(true);
			MovementComponent->SetGroundMovementMode(MOVE_Walking);
		}
		
		for (UActorComponent* a : ActorComps)
		{
			a->SetComponentTickEnabled(false);
			a->SetComponentTickInterval(TickRate);
			a->SetComponentTickEnabled(true);
			USkeletalMeshComponent* sk = Cast<USkeletalMeshComponent>(a);
			if (sk)
			{
				if (!a->ComponentTags.Contains("NoRelevancy"))
				{
					RecoverSkeletalMesh(sk);
				}
			}
		}
		
		TInlineComponentArray<UInteractableComponent*> InteractableComponents;
		InActor->GetComponents(InteractableComponents);

		for (UInteractableComponent* I : InteractableComponents)
		{
			I->EnableInteractable();
		}
	}
}

void UReadyOrNotSignificanceManager::ActorNotRelevant(AActor* InActor)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerActorNotRelevant)
	
	if (!InActor)
		return;

	// can't make an irrelevant actor more irrelevant
	if (ActorsMadeIrrelevant.Contains(InActor))
		return;

	ActorsMadeIrrelevant.AddUnique(InActor);

	AReadyOrNotCharacter* IrrelevantCharacter = Cast<AReadyOrNotCharacter>(InActor);
	if (IrrelevantCharacter)
	{
		IrrelevantCharacter->bDisableInteraction = true;
		IrrelevantCharacter->GetMesh()->bNoSkeletonUpdate = true;
		IrrelevantCharacter->bIsRelevant = false;
		
		for (ABaseItem* Item : IrrelevantCharacter->GetInventoryComponent()->GetInventoryItems())
		{
			if (Item)
			{
				ActorNotRelevant(Item);
			}
		}
		
		IrrelevantPlayerCharacters.Add(IrrelevantCharacter);
		ACyberneticController* CyberneticController = Cast<ACyberneticController>(IrrelevantCharacter->GetController());
		if (CyberneticController)
		{
			CyberneticController->SetActorTickInterval(1.0f);
		}
	}

	// Tick Instantly!
	InActor->SetActorTickInterval(1.0f);

	if (UCharacterMovementComponent* MovementComponent =  Cast<UCharacterMovementComponent>(InActor->GetComponentByClass(UCharacterMovementComponent::StaticClass())))
	{
		MovementComponent->SetComponentTickInterval(1.0f);
		MovementComponent->SetGroundMovementMode(MOVE_Walking);
	}
	
	TInlineComponentArray<UInteractableComponent*> InteractableComponents;
	InActor->GetComponents(InteractableComponents);

	for (UInteractableComponent* I : InteractableComponents)
	{
		I->DisableInteractable();
	}
		
	TInlineComponentArray<USkeletalMeshComponent*> ActorComps;
	InActor->GetComponents(ActorComps);

	for (USkeletalMeshComponent* a : ActorComps)
	{
		a->SetComponentTickInterval(1.0f);
		if (!a->ComponentTags.Contains("NoRelevancy"))
		{
			OptimizeSkeletalMesh(a);
		}
	}

	OnActorRelevancyChanged.Broadcast(InActor, false);
}

void UReadyOrNotSignificanceManager::OptimizeSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerOptimizeSkeletalMesh)

	if (SkeletalMeshComp)
	{
		SkeletalMeshComp->bEnableUpdateRateOptimizations = true;
		SkeletalMeshComp->SetCastCapsuleIndirectShadow(false);
		SkeletalMeshComp->SetCachedMaxDrawDistance(1.0f);
		SkeletalMeshComp->SuspendClothingSimulation();
	}
}

void UReadyOrNotSignificanceManager::RecoverSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerRecoverSkeletalMesh)

	if (SkeletalMeshComp)
	{
		SkeletalMeshComp->bEnableUpdateRateOptimizations = false;
		SkeletalMeshComp->bNoSkeletonUpdate = false;
		SkeletalMeshComp->SetCastCapsuleIndirectShadow(true);
		if (SkeletalMeshComp->MasterPoseComponent.Get())
		{
			SkeletalMeshComp->SetMasterPoseComponent(SkeletalMeshComp->MasterPoseComponent.Get());
		}
		SkeletalMeshComp->SetCachedMaxDrawDistance(0.0f);
		SkeletalMeshComp->ResumeClothingSimulation();
	}
}

void UReadyOrNotSignificanceManager::OptimizeSceneComponent(USceneComponent* SceneComponent)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerOptimizeSceneComponent)

	if (SceneComponent)
	{
		FOptimizationAttachmentData AttachData;
		AttachData.AttachedComponent = SceneComponent->GetAttachParent();
		AttachData.RelativeTransform = SceneComponent->GetRelativeTransform();
		AttachData.AttachedSocketName = SceneComponent->GetAttachSocketName();
		SceneCompAttachmentData.Add(SceneComponent, AttachData);
	}
}

void UReadyOrNotSignificanceManager::RecoverSceneComponent(USceneComponent* SceneComponent)
{
	SCOPE_CYCLE_COUNTER(STAT_SignificanceManagerRecoverSceneComponent)

	if (SceneComponent)
	{
		FOptimizationAttachmentData* AttachData = SceneCompAttachmentData.Find(SceneComponent);
		if (AttachData)
		{
			if (AttachData->AttachedComponent)
			{
				SceneComponent->AttachToComponent(AttachData->AttachedComponent,
					FAttachmentTransformRules::SnapToTargetIncludingScale, AttachData->AttachedSocketName);
				if (!SceneComponent->IsSimulatingPhysics())
				{
					SceneComponent->SetRelativeTransform(AttachData->RelativeTransform);
				}
				ABaseItem* BaseItem = Cast<ABaseItem>(SceneComponent->GetOwner());
				if (BaseItem)
				{
					BaseItem->OnRep_AttachmentRep();
				}
			}
			SceneCompAttachmentData.Remove(SceneComponent);
		}
	}
}

ENetMode UReadyOrNotSignificanceManager::GetNetMode() const
{
	UWorld* World = GetWorld();
	UNetDriver* NetDriver = World->GetNetDriver();
	
	if (NetDriver != nullptr)
	{
		return NetDriver->GetNetMode();
	}
	
	if (UDemoNetDriver* DemoNetDriver = World ? World->GetDemoNetDriver() : nullptr)
	{
		return DemoNetDriver->GetNetMode();
	}

	return NM_Standalone;
}
