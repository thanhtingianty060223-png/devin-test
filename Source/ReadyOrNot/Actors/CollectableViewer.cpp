// Copyright Void Interactive, 2023

#include "CollectableViewer.h"

#include "Collectable.h"
#include "CollectableViewController.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Components/InteractableComponent.h"
#include "Data/ProgressionData.h"

TAutoConsoleVariable<int32> CVarUnlockAllCollectables(TEXT("a.RonUnlockAllCollectables"), 0, TEXT("Unlock all collectables"));

ACollectableViewer::ACollectableViewer()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>("InteractableComponent");
	InteractableComponent->bClientInteract = true;
	InteractableComponent->bExecuteOnServer = false;
	InteractableComponent->AnimatedIconName = "PickupEvidence";
	InteractableComponent->ShowPromptAtDistance = 250.0f;
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->InteractCircleSize = 80.0f;
	InteractableComponent->InteractIconSize = 76.0f;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::AsCultureInvariant("Inspect Evidence"));
	InteractableComponent->ActionSlot1.bAnimate = true;
	InteractableComponent->ActionSlot1.bLoopAnimation = true;
	InteractableComponent->SetupAttachment(RootComponent);
}

void ACollectableViewer::BeginPlay()
{
	Super::BeginPlay();
	
	CheckUnlocked();

	if (CollectableClass)
	{
		FText InteractText = CollectableClass->GetDefaultObject<ACollectable>()->ItemName;
		FText FinalText = FText::Format(NSLOCTEXT("CollectableViewer", "InspectEvidence", "Inspect Evidence ({0})"), InteractText);
		InteractableComponent->ActionSlot1.ActionText = FinalText;
	}
	
	for (TActorIterator<ACollectableViewController> It(GetWorld()); It; ++It)
	{
		ViewController = *It;
		break;
	}
}

void ACollectableViewer::CheckUnlocked()
{
	// Disable if no blueprint to view or if we are not the host
	if (!CollectableClass || !HasAuthority())
	{
		InteractableComponent->DisableInteractable();
		return;
	}

	if (CVarUnlockAllCollectables.GetValueOnAnyThread() != 0)
		return;

	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	TSet<FName> ProgressionTags;
	
	if (UCommanderProfile* CommanderProfile = Cast<UCommanderProfile>(Profile))
	{
		ProgressionTags = CommanderProfile->ProgressionTags;
	}
	else if (UMetaGameProfile* MetaGameProfile = Cast<UMetaGameProfile>(Profile))
	{
		ProgressionTags = MetaGameProfile->GetProgressionTags();
	}
	
	TArray<UProgressionRequirement*>& RequiredProgression =
		CollectableClass->GetDefaultObject<ACollectable>()->RequiredProgression;

	bool bLocked = false;
	for (UProgressionRequirement* Requirement : RequiredProgression)
	{
		if (!IsValid(Requirement))
			continue;
		
		if (Requirement->IsLocked(ProgressionTags))
		{
			bLocked = true;
			break;
		}
	}

	if (bLocked)
		Destroy();
}

void ACollectableViewer::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!IsValid(InteractInstigator))
		return;
	
	AReadyOrNotPlayerController* PlayerController = InteractInstigator->GetRONPlayerController();
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
		return;

	if (IsValid(ViewController))
		ViewController->OpenViewer(this, PlayerController);
}
