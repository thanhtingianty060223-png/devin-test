// Copyright Void Interactive, 2021

#include "EvidenceActor.h"

#include "CollectedEvidenceActor.h"
#include "Actors/Door.h"
#include "GameModes/CoopGS.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/ScoringComponent.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#if !UE_BUILD_SHIPPING
#include "ReadyOrNotDebugSubsystem.h"
#endif

AEvidenceActor::AEvidenceActor()
{
	SkeletalMesh->SetVisibility(true);
	SkeletalMesh->SetHiddenInGame(false);
	StaticMesh->SetVisibility(true);
	StaticMesh->SetHiddenInGame(false);
	
	ObjectiveMarkerComponent->SetNewFadeDistance(165.0f); // 1.65m
	ObjectiveMarkerComponent->bEnabled = false;
	ObjectiveMarkerComponent->bStartHidden = true;
	
	PickupName = "Evidence";

	InteractableComponent->bHideUponPlayerMovement = false;
	InteractableComponent->RequiredLookAtPercentage = 0.98f;
	InteractableComponent->bImprintIconOnHUDUponInteraction = false;
	InteractableComponent->bShowIconWhenActionsLocked = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Repeat, FText::Format(FText::FromStringTable("ActionPromptTable", "SecureName"), EvidenceName));
	InteractableComponent->ActionSlot1.bUseCustomDisallowedActionText = true;
	InteractableComponent->ActionSlot2.bUseCustomActionText = true;
	InteractableComponent->ActionSlot2.bAnimate = true;
	InteractableComponent->ActionSlot2.bLoopAnimation = true;
	InteractableComponent->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "SecuringEvidence");

	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->ScoreGroupName = "EvidenceSecured";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;
}

void AEvidenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEvidenceActor, EvidenceState);
	DOREPLIFETIME(AEvidenceActor, PreviousEvidenceState);
	DOREPLIFETIME(AEvidenceActor, bIsBeingCollected);
	DOREPLIFETIME(AEvidenceActor, bEvidenceExtracted);
	DOREPLIFETIME(AEvidenceActor, CurrentCollectionTime);
	DOREPLIFETIME(AEvidenceActor, MaxCollectionTime);
}

void AEvidenceActor::BeginPlay()
{
	Super::BeginPlay();

	EvidenceState = EEvidenceActorState::Unclaimed;

	if (!EvidenceName.IsEmpty())
	{
		Tags.AddUnique(FName(*EvidenceName.ToString()));
	}
	Tags.AddUnique(PickupName);

	ObjectiveMarkerComponent->DisableObjectiveMarker();

	SkeletalMesh->SetVisibility(true);
	SkeletalMesh->SetHiddenInGame(false);
	StaticMesh->SetVisibility(true);
	StaticMesh->SetHiddenInGame(false);

	StaticMesh->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllEvidenceActors.AddUnique(this);
	}
}

void AEvidenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// need to fix memory error with templating this scoring comp
	if (IsValid(ScoringComponent))
	{
		ScoringComponent->RemoveFromScorePool();
		ScoringComponent->DestroyComponent();
		ScoringComponent = nullptr;
	}
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllEvidenceActors.Remove(this);
	}
}

void AEvidenceActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bShowAllEvidenceActors)
		{
			ShowObjectiveMarker();
		}
		else if (EvidenceState == EEvidenceActorState::Collected)
		{
			HideObjectiveMarker();
		}
	}
	#endif

	InteractableComponent->SetAnimatedIconName(DetermineAnimatedIcon_Implementation());
	InteractableComponent->ActionSlot1.InputEvent = IE_Repeat;
	// TODO (Max): Should Evidence Names be localised?
	InteractableComponent->ActionSlot1.ActionText = FText::Format(FText::FromStringTable("ActionPromptTable", "SecureEvidenceName"), EvidenceName);
	InteractableComponent->ActionSlot1.bCondition = EvidenceState != EEvidenceActorState::Extraction && EvidenceState != EEvidenceActorState::Collected && !bIsBeingCollected;
	InteractableComponent->ActionSlot2.bCondition = bIsBeingCollected;
	
	InteractableComponent->CurrentProgress = bIsBeingCollected ? FMath::Clamp(CurrentCollectionTime/MaxCollectionTime, 0.0f, 1.0f) : 0.0f;

	// Failsafe, dont want the static mesh to be detached too far 0, 0, 0
	//if (FVector::Distance(FVector::ZeroVector, StaticMesh->GetRelativeLocation()) > 100.0f)
		//StaticMesh->SetRelativeLocation(FVector::ZeroVector);
		
	// Failsafe, dont want the interactable to be detached too far from the mesh
	if (FVector::Distance(FVector::ZeroVector, InteractableComponent->GetRelativeLocation()) > 45.0f)
		InteractableComponent->SetRelativeLocation(FVector::ZeroVector);

	if (bIsBeingCollected)
	{
		UpdateEvidenceCollection_COOP(DeltaSeconds);
	}

	if (EvidenceState == EEvidenceActorState::Collected)
	{
		HideEvidenceActor();
	}

	if (PickupInstigator)
	{
		if (EvidenceState == EEvidenceActorState::Collected)
		{
			//SetActorLocation(PickupInstigator->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			
			//SetActorEnableCollision(false);
			SkeletalMesh->SetSimulatePhysics(false);
			StaticMesh->SetSimulatePhysics(false);
		}
		else if (EvidenceState != EEvidenceActorState::Extraction)
		{
			ShowObjectiveMarker();
			
			//SetActorEnableCollision(false);
			SkeletalMesh->SetSimulatePhysics(false);
			StaticMesh->SetSimulatePhysics(false);
		}
		else
		{
			//SetActorEnableCollision(true);
			SkeletalMesh->SetSimulatePhysics(true);
			//StaticMesh->SetSimulatePhysics(true);
		}
	}
	else
	{
		EvidenceState = EEvidenceActorState::Unclaimed;

		//SetActorEnableCollision(true);
		SkeletalMesh->SetSimulatePhysics(true);
		StaticMesh->SetSimulatePhysics(false);
	}
}

void AEvidenceActor::Reset()
{
	Super::Reset();

	EvidenceState = EEvidenceActorState::Unclaimed;
}

void AEvidenceActor::ActorPickedUp(AActor* InPickupInstigator)
{
	if (!InPickupInstigator)
		return;

	if (EvidenceState != EEvidenceActorState::Collected)
	{
		Super::ActorPickedUp(InPickupInstigator);

		if (ACharacter* Character = Cast<ACharacter>(InPickupInstigator))
		{
			if (AReadyOrNotPlayerState* RON_PS = Character->GetPlayerState<AReadyOrNotPlayerState>())
			{
				RON_PS->Evidence++;
				RON_PS->EvidenceActorsInPossession.Add(this);
			}

			if (APlayerCharacter* PC = Cast<APlayerCharacter>(InPickupInstigator))
				PC->PlayRawVO(VO_SWAT_GENERAL::CALL_COLLECT_EVIDENCE);
		}

		PreviousEvidenceState = EvidenceState;
		EvidenceState = EEvidenceActorState::Collected;

		if (HasAuthority())
		{
			FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "EvidenceSecuredWithName"), EvidenceName);
			ScoringComponent->GiveAllScores(true, true, ScoreText);
			
			OnRep_EvidenceStateChanged();
		}
	}
}

void AEvidenceActor::ActorDropped(AActor* InDroppedInstigator)
{
	Super::ActorDropped(InDroppedInstigator);

	if (EvidenceState != EEvidenceActorState::Collected || EvidenceState == EEvidenceActorState::Extraction)
		return;

	if (ACharacter* Character = Cast<ACharacter>(InDroppedInstigator))
	{
		if (AReadyOrNotPlayerState* RON_PS = Character->GetPlayerState<AReadyOrNotPlayerState>())
		{
			RON_PS->EvidenceActorsInPossession.RemoveSingle(this);
		}
	}

	PreviousEvidenceState = EvidenceState;
	EvidenceState = EEvidenceActorState::Dropped;

	if (HasAuthority())
	{
		OnRep_EvidenceStateChanged();
	}
}

void AEvidenceActor::StartExtractingEvidence()
{
	if (EvidenceState == EEvidenceActorState::Extraction)
		return;

	PreviousEvidenceState = EvidenceState;
	EvidenceState = EEvidenceActorState::Extraction;

	if (HasAuthority())
	{
		OnRep_EvidenceStateChanged();
	}
	
	bEvidenceExtracted = false;
}

void AEvidenceActor::FinishExtractingEvidence()
{
	if (EvidenceState != EEvidenceActorState::Extraction)
		return;

	bEvidenceExtracted = true;

	ReportExtractedToInGameLog();
}

bool AEvidenceActor::IsEvidenceCollected() const
{
	return EvidenceState == EEvidenceActorState::Collected;
}

void AEvidenceActor::StartEvidenceCollection_COOP()
{
	if (bIsBeingCollected)
		return;
	
	if (APlayerCharacter* Collector = Cast<APlayerCharacter>(PickupInstigator))
	{
		CurrentCollectionTime = 0.0f;
		MaxCollectionTime = Collector->GetEvidenceCollectionTime();
		
		Collector->BeginEvidenceCollection_COOP(this, InteractableComponent, MaxCollectionTime);
		
		bIsBeingCollected = true;
	}
}

void AEvidenceActor::StopEvidenceCollection_COOP()
{
	if (!bIsBeingCollected)
		return;

	bIsBeingCollected = false;

	if (APlayerCharacter* CollectingCharacter = Cast<APlayerCharacter>(PickupInstigator))
	{
		CollectingCharacter->UnlockAllActions();
		
		CollectingCharacter->StopEvidenceCollectingAnims();
		
        CollectingCharacter->EndEvidenceCollection_COOP(InteractableComponent);
	}
}

void AEvidenceActor::CompleteEvidenceCollection_COOP()
{
	if (AReadyOrNotCharacter* Collector = Cast<AReadyOrNotCharacter>(PickupInstigator))
	{
		if (ACollectedEvidenceActor* EvidenceBag = Collector->SpawnEvidenceCollectionBag(GetActorTransform()))
		{
			UScoringComponent* NewScoringComp = NewObject<UScoringComponent>(EvidenceBag, FName("ScoringComp"), RF_NoFlags, ScoringComponent);
			NewScoringComp->RegisterComponent();
			NewScoringComp->SetIsReplicated(true);
			NewScoringComp->AddToScorePool();
			NewScoringComp->GiveAllScores();
			
			EvidenceBag->Tags = Tags;
		}

		Collector->PickupEvidence(this);
    }
}

void AEvidenceActor::UpdateEvidenceCollection_COOP(const float DeltaTime)
{
	if (APlayerCharacter* CollectingCharacter = Cast<APlayerCharacter>(PickupInstigator))
	{
		if (!CollectingCharacter->HasCollectionAnimTriggered())
		{
			CollectingCharacter->TriggerCollectionAnim();
		}
		
		CurrentCollectionTime += DeltaTime;
	}
}

void AEvidenceActor::OnRep_EvidenceStateChanged()
{
	switch (EvidenceState)
	{
		case EEvidenceActorState::Unclaimed:
		break;
		
		case EEvidenceActorState::Collected:
			HideEvidenceActor();
		break;
		
		case EEvidenceActorState::Extraction:
			HideEvidenceActor();
			ReportExtractingToInGameLog(PickupInstigator);
		break;

		case EEvidenceActorState::Dropped:
			ShowEvidenceActor(true);
		break;
		
		default:
		break;
	}
}

bool AEvidenceActor::CanPickUpNow(APlayerCharacter* PickerUpper)
{
	return EvidenceState != EEvidenceActorState::Extraction && EvidenceState != EEvidenceActorState::Collected;
}

void AEvidenceActor::ReportPickupToInGameLog(AActor* InPickupInstigator)
{
	if (EvidenceState == EEvidenceActorState::Collected)
	{
		if (AReadyOrNotGameState* RON_GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (RON_GS->bPvPMode)
			{
				if (APlayerCharacter* PC = Cast<APlayerCharacter>(InPickupInstigator))
				{
					if (PC->GetPlayerState())
					{
						if (PreviousEvidenceState == EEvidenceActorState::Extraction)
						{
							const FString InGameLogMessage = PC->GetPlayerState()->GetPlayerName() + FString(" has collected the extracted intel item.");
							ENQUEUE_INGAMELOG_MESSAGE_PVP({PC, EPVPEvent::IntelCollected, FText::FromString(InGameLogMessage)});
						}
						else
						{
							const FString InGameLogMessage = PC->GetPlayerState()->GetPlayerName() + FString(" has secured the intel item.");
							ENQUEUE_INGAMELOG_MESSAGE_PVP({PC, EPVPEvent::IntelCollected, FText::FromString(InGameLogMessage)});
						}
					}
				}
			}
			else
			{
				if (PreviousEvidenceState == EEvidenceActorState::Extraction)
				{
					ENQUEUE_INGAMELOG_MESSAGE({DL_Success, FText::FromString("[Evidence Extracted]: " + EvidenceName.ToString())});
				}
				else
				{
					ENQUEUE_INGAMELOG_MESSAGE({DL_Success, FText::FromString("[Evidence Collected]: " + EvidenceName.ToString())});
				}
			}
		}
	}
}

void AEvidenceActor::ReportDropToInGameLog(AActor* InDropInstigator)
{
	if (EvidenceState == EEvidenceActorState::Dropped)
	{
		if (AReadyOrNotGameState* RON_GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (RON_GS->bPvPMode)
			{
				if (APlayerCharacter* PC = Cast<APlayerCharacter>(InDropInstigator))
				{
					if (PC->GetPlayerState())
					{
						const FString InGameLogMessage = PC->GetPlayerState()->GetPlayerName() + FString(" has dropped the intel item.");
						ENQUEUE_INGAMELOG_MESSAGE_PVP({PC, EPVPEvent::IntelDropped, FText::FromString(InGameLogMessage)});
					}
				}
			}
			else
			{
				ENQUEUE_INGAMELOG_MESSAGE({DL_Error, FText::FromString("[Evidence Dropped]: " + EvidenceName.ToString())});
			}
		}
	}
}

void AEvidenceActor::ReportExtractingToInGameLog(AActor* InPickupInstigator)
{
	if (EvidenceState == EEvidenceActorState::Extraction)
	{
		if (AReadyOrNotGameState* RON_GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (RON_GS->bPvPMode)
			{
				if (APlayerCharacter* PC = Cast<APlayerCharacter>(InPickupInstigator))
				{
					if (PC->GetPlayerState())
					{
						const FString InGameLogMessage = PC->GetPlayerState()->GetPlayerName() + FString(" started the intel extraction process.");
						ENQUEUE_INGAMELOG_MESSAGE_PVP({PC, EPVPEvent::IntelExtracting, FText::FromString(InGameLogMessage)});
					}
				}
			}
			else
			{
				ENQUEUE_INGAMELOG_MESSAGE({DL_Info, FText::FromString("[Extracting Evidence]: " + EvidenceName.ToString())});
			}
		}
	}
}

void AEvidenceActor::ReportExtractedToInGameLog()
{
	if (EvidenceState == EEvidenceActorState::Extraction)
	{
		if (AReadyOrNotGameState* RON_GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (RON_GS->bPvPMode)
			{
				if (APlayerCharacter* PC = Cast<APlayerCharacter>(PickupInstigator))
				{
					if (PC->GetPlayerState())
					{
						ENQUEUE_INGAMELOG_MESSAGE_PVP({PC, EPVPEvent::IntelExtracted, FText::FromString("Intel has been extracted. Ready for collection.")});
					}
				}
			}
			else
			{
				ENQUEUE_INGAMELOG_MESSAGE({DL_Info, FText::FromString("[Evidence Extracted]: " + EvidenceName.ToString())});
			}
		}
	}
}

void AEvidenceActor::DestroyEvidence()
{
	Destroy(true);
}

FName AEvidenceActor::DetermineAnimatedIcon_Implementation() const
{
	return bIsBeingCollected ? "Empty" : "Pickup Evidence";
}

void AEvidenceActor::ShowEvidenceActor(const bool bReportToInGameLog)
{
	//if (ObjectiveMarkerComponent)
	//{
	//	ObjectiveMarkerComponent->ShowObjectiveMarker();
	//}
	
	//if (bReportToInGameLog)
	//	ReportDropToInGameLog(PickupInstigator);

	SetActorHiddenInGame(false);
	//SetActorEnableCollision(true);
	SkeletalMesh->SetSimulatePhysics(true);
	SkeletalMesh->SetVisibility(true);

	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetVisibility(true);
}

void AEvidenceActor::HideEvidenceActor(const bool bReportToInGameLog)
{
	//if (ObjectiveMarkerComponent)
	//{
	//	ObjectiveMarkerComponent->HideObjectiveMarker();
	//}
	
	//if (bReportToInGameLog)
	//	ReportPickupToInGameLog(PickupInstigator);

	TArray<UFMODAudioComponent*> FMODAudioComponents;
	GetComponents(FMODAudioComponents);
	for (UFMODAudioComponent* Comp : FMODAudioComponents)
	{
		if (Comp)
		{
			Comp->Stop();
		}
	}	

	SetActorHiddenInGame(true);
	//SetActorEnableCollision(false);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetVisibility(false);
	
	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->SetVisibility(false);
}

void AEvidenceActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	PickupInstigator = InteractInstigator;
	
	if (GET_GAME_STATE(ACoopGS))
	{
		StartEvidenceCollection_COOP();

		return;
	}

	Super::Interact_Implementation(InteractInstigator, InInteractableComponent);
}

void AEvidenceActor::EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;
	
	if (GET_GAME_STATE(ACoopGS))
	{
		StopEvidenceCollection_COOP();

		return;
	}

	Super::EndInteract_Implementation(InteractInstigator, InInteractableComponent);
}

void AEvidenceActor::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	//EndInteract_Implementation(InteractInstigator, InInteractableComponent);
}

bool AEvidenceActor::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (Hit.GetActor())
	{
		if (const ADoor* Door = Cast<ADoor>(Hit.GetActor()))
		{
			if (Door->IsDoorBroken() || !Door->IsAttachedToRoot() || Door->AllMajorDoorChunksDestroyed())
				return true;
		}
		
		if (Hit.GetActor()->IsA(ABaseItem::StaticClass()))
			return true;
		
		if (Hit.GetActor()->IsA(APawn::StaticClass()))
			return true;
		
		if (Hit.GetActor()->IsA(ACollectedEvidenceActor::StaticClass()))
			return true;
	}
	
	return false;
}

// ICanIssueCommandOn implementation
bool AEvidenceActor::CanIssueCommand_Implementation() const
{
	return true;
}

AActor* AEvidenceActor::GetCommandActor_Implementation() const
{
	return const_cast<AEvidenceActor*>(this);
}

// IScoringInterface implementation
UScoringComponent* AEvidenceActor::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

void AEvidenceActor::Secure_Implementation(AReadyOrNotCharacter* InInstigator)
{
	InInstigator->PickupEvidence(this);
}

bool AEvidenceActor::IsSecured_Implementation() const
{
	return EvidenceState == EEvidenceActorState::Collected;
}

FVector AEvidenceActor::GetLocation_Implementation() const
{
	return GetActorLocation() + FVector::UpVector * 50.0f;
}

bool AEvidenceActor::CanBeSecured_Implementation() const
{
	return EvidenceState != EEvidenceActorState::Collected;
}
