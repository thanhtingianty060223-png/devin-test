// Copyright Void Interactive, 2023

#include "ActivityTriggerVolume.h"

#include "BaseTriggerable.h"
#include "Actors/Door.h"
#include "Actors/Items/Chemlight.h"
#include "Components/AmmoComponent.h"
#include "Components/InteractableComponent.h"
#include "Data/ActivityData.h"
#include "GameModes/TrainingGM.h"
#include "HUD/Widgets/ScreenspaceMarkerWidget.h"
#include "HUD/Widgets/TutorialWidget.h"
#include "Info/LoadoutManager.h"
#include "Info/SWATManager.h"

AActivityTriggerVolume::AActivityTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AActivityTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	// Add dynamic delegates for when player enters and exits the trigger volume
	this->OnActorBeginOverlap.AddDynamic(this, &AActivityTriggerVolume::OnPlayerBeginOverlap);
	this->OnActorEndOverlap.AddDynamic(this, &AActivityTriggerVolume::OnPlayerEndOverlap);

	if (bStartActive)
		Activate();
}

void AActivityTriggerVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove dynamic delegates for when player enters and exits the trigger volume
	this->OnActorBeginOverlap.RemoveDynamic(this, &AActivityTriggerVolume::OnPlayerBeginOverlap);
	this->OnActorEndOverlap.RemoveDynamic(this, &AActivityTriggerVolume::OnPlayerEndOverlap);
}

void AActivityTriggerVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
#if WITH_EDITOR
	if (!bDrawDebug)
		return;
	
	if (bIsComplete)
	{
		DrawDebugBox(GetWorld(), GetActorLocation(), GetComponentsBoundingBox().GetExtent(), FColor::Emerald, false, 0.2f, 0, 5.0f);
	}
	else if (bIsActive && PlayerCharacter)
	{
		DrawDebugBox(GetWorld(), GetActorLocation(), GetComponentsBoundingBox().GetExtent(), FColor::Blue, false, 0.2f, 0, 5.0f);
	}
	else if (bIsActive)
	{
		DrawDebugBox(GetWorld(), GetActorLocation(), GetComponentsBoundingBox().GetExtent(), FColor::Yellow, false, 0.2f, 0, 5.0f);
	}
	else
	{
		DrawDebugBox(GetWorld(), GetActorLocation(), GetComponentsBoundingBox().GetExtent(), FColor::Red, false, 0.2f, 0, 3.0f);
	}
#endif
}

void AActivityTriggerVolume::Activate()
{
	ensureMsgf(bIsActive == false, TEXT("ActivityTriggerVolume is already active!"));
	if (bIsActive)
		return;

	bIsActive = true;

	if (ATrainingGM* GameMode = GetWorld()->GetAuthGameMode<ATrainingGM>())
		GameMode->ActiveTriggerVolumes.Add(this);

	// TODO KobeT: Show the screenspace marker on activation
	// ShowTooltipScreenspaceMarker(true);

	// Perform all activation events
	HandleEvents(ActivationEvents);

	// If player already in volume when activated
	if (IsLocalPlayer(PlayerCharacter))
		ActivateActivities();
}

void AActivityTriggerVolume::Deactivate()
{
	ensureMsgf(bIsActive == true, TEXT("ActivityTriggerVolume is already inactive!"));
	if (!bIsActive)
		return;

	bIsActive = false;

	// Remove the current transition volume from the game mode
	if (ATrainingGM* GameMode = GetWorld()->GetAuthGameMode<ATrainingGM>())
		GameMode->ActiveTriggerVolumes.Remove(this);

	// Unbind the player from this trigger's activities
	HandleActivities(true);
}

void AActivityTriggerVolume::Reactivate(const float ReactivateDelay)
{
	Deactivate();

	ResetAllProgress();

	if (ReactivateDelay <= 0.0f)
		Activate();
	else
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AActivityTriggerVolume::Activate, ReactivateDelay);
}

void AActivityTriggerVolume::ResetAllProgress()
{
	bIsComplete = false;

	ResetEvents(ActivationEvents);
	ResetEvents(OnEnterEvents);
	ResetEvents(OnLeaveEvents);
	ResetEvents(CompletionEvents);

	ResetActivities();
}

bool AActivityTriggerVolume::AllActivitiesComplete() const
{
	for (const UActivityData* Activity : Activities)
	{
		if (!Activity)
			continue;

		if (!Activity->IsComplete())
			return false;
	}
	return true;
}

void AActivityTriggerVolume::Reset()
{
	Super::Reset();
	ResetAllProgress();
}

bool AActivityTriggerVolume::ContainsAnyEvent(EStandaloneEvent StandaloneEvent) const
{
	for (const FActivityEvent& Event : ActivationEvents)
	{
		if (Event.StandaloneEvent == StandaloneEvent)
			return true;
	}
	for (const FActivityEvent& Event : OnEnterEvents)
	{
		if (Event.StandaloneEvent == StandaloneEvent)
			return true;
	}
	for (const FActivityEvent& Event : OnLeaveEvents)
	{
		if (Event.StandaloneEvent == StandaloneEvent)
			return true;
	}
	for (const FActivityEvent& Event : CompletionEvents)
	{
		if (Event.StandaloneEvent == StandaloneEvent)
			return true;
	}
	return false;
}

bool AActivityTriggerVolume::ContainsAnyEvent(ETargetEvent TargetEvent) const
{
	for (const FActivityEvent& Event : ActivationEvents)
	{
		if (Event.TargetEvent == TargetEvent)
			return true;
	}
	for (const FActivityEvent& Event : OnEnterEvents)
	{
		if (Event.TargetEvent == TargetEvent)
			return true;
	}
	for (const FActivityEvent& Event : OnLeaveEvents)
	{
		if (Event.TargetEvent == TargetEvent)
			return true;
	}
	for (const FActivityEvent& Event : CompletionEvents)
	{
		if (Event.TargetEvent == TargetEvent)
			return true;
	}
	return false;
}

void AActivityTriggerVolume::OnPlayerRespawned(APlayerCharacter* PlayerRespawned)
{
	if (!IsLocalPlayer(PlayerRespawned))
		return;

	PlayerCharacter = PlayerRespawned;
	ActivateActivities();
}

void AActivityTriggerVolume::OnPlayerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!IsLocalPlayer(OtherActor))
		return;

	PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
	ActivateActivities();
}

void AActivityTriggerVolume::ActivateActivities()
{
	if (!bIsActive)
	{
		if (bTriggerEventsWhileInactive)
			HandleEvents(OnEnterEvents);

		return;
	}

	// TODO KobeT: Hide the screenspace marker on player entering the volume
	// ShowTooltipScreenspaceMarker(false);

	// Perform all on-enter events
	HandleEvents(OnEnterEvents);

	// Bind the player to this trigger's activities
	if (ActivityDelay <= 0.0f)
		HandleActivities();
	else
		UReadyOrNotFunctionLibrary::StartTimerForCallback(ActivateDelayTimerHandle, this, FTimerDelegate::CreateUObject(this, &AActivityTriggerVolume::HandleActivities, false), ActivityDelay);
}

void AActivityTriggerVolume::OnPlayerEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!IsLocalPlayer(OtherActor))
		return;

	DeactivateActivities();
}

void AActivityTriggerVolume::DeactivateActivities()
{
	if (!bIsActive)
	{
		if (bTriggerEventsWhileInactive)
			HandleEvents(OnLeaveEvents);

		PlayerCharacter = nullptr;
		return;
	}

	// TODO KobeT: Show the screenspace marker on player leaving the volume
	// ShowTooltipScreenspaceMarker(true);

	// Perform all on-leave events
	HandleEvents(OnLeaveEvents);

	// Unbind the player from this trigger's activities
	HandleActivities(true);

	PlayerCharacter = nullptr;
}

void AActivityTriggerVolume::OnActivityComplete(UActivityData* Activity)
{
	// Set tooltip to next activity
	ShowTutorialWidget();

	if (AllActivitiesComplete())
	{
		// Perform all completion events
		HandleEvents(CompletionEvents);

		bIsComplete = true;
		OnAllActivitiesComplete.Broadcast(this);

		if (TransitionDelay <= 0.0f)
		{
			TransitionToNextVolume();
		}
		else
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AActivityTriggerVolume::TransitionToNextVolume, TransitionDelay);
		}
	}
}

void AActivityTriggerVolume::TransitionToNextVolume()
{
	Deactivate();

	if (AActivityTriggerVolume* NextVolume = NextTransitionVolume.Get())
	{
		NextVolume->Activate();
	}
}

bool AActivityTriggerVolume::IsLocalPlayer(const AActor* Actor) const
{
	if (!Actor)
		return false;

	// Activity volumes and associated events/activities are currently configured for local-only
	if (Actor == UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
		return true;

	return false;
}

void AActivityTriggerVolume::HandleActivities(const bool bUnbind)
{
	// If the player has left the volume before the ActivateDelay timer has finished
	// stop the timer to prevent the tutorial tooltip from showing once timer completes
	if (bUnbind && UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, ActivateDelayTimerHandle))
	{
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, ActivateDelayTimerHandle);
	}

	for (UActivityData* Activity : Activities)
	{
		if (Activity == nullptr)
			continue;

		if (bUnbind)
		{
			Activity->UnbindPlayerFromActivity();
			Activity->OnComplete.RemoveDynamic(this, &AActivityTriggerVolume::OnActivityComplete);
		}
		else
		{
			Activity->BindPlayerToActivity(PlayerCharacter);
			Activity->OnComplete.AddUniqueDynamic(this, &AActivityTriggerVolume::OnActivityComplete);
		}
	}

	if (bUnbind)
	{
		HideTutorialWidget();
	}
	else
	{
		ShowTutorialWidget();
	}
}

void AActivityTriggerVolume::ResetActivities()
{
	for (UActivityData* Activity : Activities)
	{
		if (Activity == nullptr)
			continue;

		Activity->ResetProgress();
	}
}

void AActivityTriggerVolume::HandleEvents(TArray<FActivityEvent>& Events)
{
	for (FActivityEvent& Event : Events)
	{
		// Skip if event has already been triggered and is set to only trigger once
		if (Event.bTriggerOnlyOnce && Event.bHasTriggered)
			continue;

		Event.bHasTriggered = true;

		switch(Event.EventType)
		{
		case EEventType::E_Standalone:
			HandleStandAloneEvent(Event);
			continue;
		case EEventType::E_Target:
			HandleTargetEvent(Event);
			continue;
		case EEventType::E_FmodAudio:
			HandleFmodAudioEvent(Event);
			continue;
		case EEventType::E_UnrealAudio:
			HandleUnrealAudioEvent(Event);
			continue;
		}
		ensureMsgf(false, TEXT("Event not handled!"));
	}
}

void AActivityTriggerVolume::ResetEvents(TArray<FActivityEvent>& Events)
{
	for (FActivityEvent& Event : Events)
	{
		Event.bHasTriggered = false;
	}
}

void AActivityTriggerVolume::HandleStandAloneEvent(const FActivityEvent& Event)
{
	switch(Event.StandaloneEvent)
	{
	case EStandaloneEvent::E_None:
		return;
	case EStandaloneEvent::E_LockPlayerMovement:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock movement!")))
		{
			PlayerCharacter->LockMovement();
		}
		return;
	case EStandaloneEvent::E_UnlockPlayerMovement:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock movement!")))
		{
			PlayerCharacter->UnlockMovement();
		}
		return;
	case EStandaloneEvent::E_LockPlayerItemSelection:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock item selection!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->LockItemSelection();
		}
		return;
	case EStandaloneEvent::E_UnlockPlayerItemSelection:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock item selection!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->UnlockItemSelection();
		}
		return;
	case EStandaloneEvent::E_LockAllPlayerActions:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock actions!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->LockAllActions();
		}
		return;
	case EStandaloneEvent::E_UnlockAllPlayerActions:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock actions!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->UnlockAllActions();
		}
		return;
	case EStandaloneEvent::E_LockPlayerCommandMenu:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock actions!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->LockCommandMenu();
		}
		return;
	case EStandaloneEvent::E_UnlockPlayerCommandMenu:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock actions!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->UnlockCommandMenu();
		}
		return;
	case EStandaloneEvent::E_LockWeaponAttachments:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock weapon attachments!")))
		{
			PlayerCharacter->LockWeaponAttachments();
		}
		return;
	case EStandaloneEvent::E_UnlockWeaponAttachments:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock weapon attachments!")))
		{
			PlayerCharacter->UnlockWeaponAttachments();
		}
		return;
	case EStandaloneEvent::E_LockCantedSights:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot lock canted sights!")))
		{
			PlayerCharacter->LockCantedSight();
		}
		return;
	case EStandaloneEvent::E_UnlockCantedSights:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unlock canted sights!")))
		{
			PlayerCharacter->UnlockCantedSight();
		}
		return;
	case EStandaloneEvent::E_SetPlayerLowReady:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot set player low ready!")))
		{
			PlayerCharacter->SetForceLowReady(true);
		}
		return;
	case EStandaloneEvent::E_SetPlayerNotLowReady:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot unset player low ready!")))
		{
			PlayerCharacter->SetForceLowReady(false);
		}
		return;
	case EStandaloneEvent::E_SpawnPolice:
		ensureAlwaysMsgf(GetWorld()->GetAuthGameMode<ATrainingGM>(), TEXT("Spawn Police is only valid in a Training GameMode!"));
		if (ATrainingGM* GameMode = GetWorld()->GetAuthGameMode<ATrainingGM>())
		{
			GameMode->SpawnPolice(false);
		}
		return;
	case EStandaloneEvent::E_SpawnPoliceAtPlayer:
		ensureAlwaysMsgf(GetWorld()->GetAuthGameMode<ATrainingGM>(), TEXT("Spawn Police is only valid in a Training GameMode!"));
		if (ATrainingGM* GameMode = GetWorld()->GetAuthGameMode<ATrainingGM>())
		{
			GameMode->SpawnPolice(true);
		}
		return;
	case EStandaloneEvent::E_HidePlayerWeapon:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change equipment!")))
		{
			// "Hide" the player weapon by manually equipping a chemlight. Using manual equip to avoid the weapon draw animation
			ABaseItem* ItemToEquip = PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Chemlight);
			PlayerCharacter->GetInventoryComponent()->ManuallySetEquippedItem(ItemToEquip);
		}
		return;
	case EStandaloneEvent::E_EquipTrainingLoadout:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!")))
		{
			AReadyOrNotPlayerState* PlayerState = PlayerCharacter->GetRONPlayerState();
			if (!ensureAlways(PlayerState))
				return;

			FSavedLoadout Loadout;
			UBpGameplayHelperLib::LoadDefaultLoadout(Loadout, "training");
			PlayerState->Server_SetLoadout(Loadout);

			// Destroy all equipped items to ensure the player is equipped with a fresh loadout
			PlayerCharacter->GetInventoryComponent()->DestroyAllEquippedItems();

			UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, PlayerCharacter, FLoadoutEquipOptions());
		}
		return;
	case EStandaloneEvent::E_RemoveAmmoFromLoadout:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!")))
		{
			if (UInventoryComponent* PlayerInventory = PlayerCharacter->GetInventoryComponent())
			{
				// Empty primary weapon ammo
				if (ABaseMagazineWeapon* PrimaryWeapon = Cast<ABaseMagazineWeapon>(PlayerInventory->GetSpawnedGear().Primary))
					for (FMagazine& Magazine : PrimaryWeapon->Magazines)
						Magazine.Ammo = 0;

				// Empty secondary weapon ammo
				if (ABaseMagazineWeapon* SecondaryWeapon = Cast<ABaseMagazineWeapon>(PlayerInventory->GetSpawnedGear().Secondary))
					for (FMagazine& Magazine : SecondaryWeapon->Magazines)
						Magazine.Ammo = 0;

				// Empty grenade ammo
				for (ABaseItem* Grenade : PlayerInventory->GetSpawnedGear().Grenades)
					PlayerInventory->DestroyInventoryItem(Grenade);

				// Empty tactical devices
				for (ABaseItem* TacticalDevice : PlayerInventory->GetSpawnedGear().TacticalDevices)
					PlayerInventory->DestroyInventoryItem(TacticalDevice);
			}
		}
		return;
	case EStandaloneEvent::E_AddAmmoToLoadout:
		ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!"));
		if (PlayerCharacter)
		{
			PlayerCharacter->ReplenishAllMagazineAmmo();
		}
		return;
	case EStandaloneEvent::E_AddGrenadesToLoadout:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!")))
		{
			PlayerCharacter->ReplenishAllGrenadeAmmo();
		}
		return;
	case EStandaloneEvent::E_AddChemlightsToLoadout:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!")))
		{
			const UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent();
			if (!ensureAlways(InventoryComponent))
				return;

			if (const AChemlight* Chemlight = InventoryComponent->GetInventoryItemOfClass_Native<AChemlight>(AChemlight::StaticClass()))
			{
				Chemlight->GetAmmoComponent()->SetResource(10);
			}
		}
		return;
	case EStandaloneEvent::E_AddEmptyMagPrimary:
		if (ensureAlwaysMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot change loadout!")))
		{
			const FSpawnedGear& SpawnedGear = PlayerCharacter->GetInventoryComponent()->GetSpawnedGear();

			if (ABaseMagazineWeapon* PrimaryWeapon = Cast<ABaseMagazineWeapon>(SpawnedGear.Primary))
				PrimaryWeapon->Magazines.Insert(FMagazine(), 0);
		}
		return;
	}
	ensureAlwaysMsgf(false, TEXT("Event: %s not handled!"), *UEnum::GetValueAsString(Event.StandaloneEvent));
}

void AActivityTriggerVolume::HandleTargetEvent(const FActivityEvent& Event) const
{
	AActor* TargetActor = Event.TargetActor.Get();
	ensureMsgf(TargetActor, TEXT("A valid target actor has not been selected!"));
	if (!TargetActor)
		return;

	switch(Event.TargetEvent)
	{
	case ETargetEvent::E_None:
		return;
	case ETargetEvent::E_LockDoor:
		if (ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->LockDoor();
		return;
	case ETargetEvent::E_UnlockDoor:
		if (ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->UnlockDoor();
		return;
	case ETargetEvent::E_DisableDoor:
		if (ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->bCanPlayerInteract = false;
		return;
	case ETargetEvent::E_EnableDoor:
		if (ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->bCanPlayerInteract = true;
		return;
	case ETargetEvent::E_DisableDoorInteraction:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot disable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
		{
			TArray<UInteractableComponent*> InteractableComponents;
			TargetDoor->GetComponents<UInteractableComponent>(InteractableComponents, true);
			for (UInteractableComponent* InteractableComponent : InteractableComponents)
				InteractableComponent->DisableInteractionFor(PlayerCharacter);
		}
		return;
	case ETargetEvent::E_EnableDoorInteraction:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot enable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
		{
			TArray<UInteractableComponent*> InteractableComponents;
			TargetDoor->GetComponents<UInteractableComponent>(InteractableComponents, true);
			for (UInteractableComponent* InteractableComponent : InteractableComponents)
				InteractableComponent->EnableInteractionFor(PlayerCharacter);
		}
		return;
	case ETargetEvent::E_EnableDoorOpen:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot enable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->GetOpenInteractableComponent()->EnableInteractionFor(PlayerCharacter);
		return;
	case ETargetEvent::E_EnableDoorPeek:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot enable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->GetPushInteractableComponent()->EnableInteractionFor(PlayerCharacter);
		return;
	case ETargetEvent::E_EnableDoorKick:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot enable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->GetKickInteractableComponent()->EnableInteractionFor(PlayerCharacter);
		return;
	case ETargetEvent::E_EnableDoorOptiwand:
		ensureMsgf(PlayerCharacter, TEXT("A Player is not in the volume! Cannot enable interaction!"));
		if (const ADoor* TargetDoor = Cast<ADoor>(TargetActor))
			TargetDoor->GetOptiwandInteractableComponent()->EnableInteractionFor(PlayerCharacter);
		return;
	case ETargetEvent::E_ActivateTriggerable:
		if (ABaseTriggerable* Triggerable = Cast<ABaseTriggerable>(TargetActor))
			Triggerable->Activate();
		return;
	case ETargetEvent::E_DeactivateTriggerable:
		if (ABaseTriggerable* Triggerable = Cast<ABaseTriggerable>(TargetActor))
			Triggerable->Deactivate();
		return;
	case ETargetEvent::E_SpawnAi:
		if (AAISpawn* TargetSpawner = Cast<AAISpawn>(TargetActor))
			if (TargetSpawner->DoSpawn())
				if (TargetSpawner->SpawnedCharacter && TargetSpawner->SpawnedCharacter->GetTeam() == ETeamType::TT_CIVILIAN)
					TargetSpawner->SpawnedCharacter->OnCharacterKilled.AddDynamic(this, &AActivityTriggerVolume::OnSpawnedAiKilled);
		return;
	case ETargetEvent::E_SetPlayerSpawn:
		if (const APlayerStart* TargetStart = Cast<APlayerStart>(TargetActor))
			if (const APlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
				if (AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(PlayerController->PlayerState))
					PlayerState->Server_UpdatePlayerSpawnTag(TargetStart->PlayerStartTag.ToString());
		return;
	}
	ensureMsgf(false, TEXT("Event not handled!"));
}

void AActivityTriggerVolume::HandleFmodAudioEvent(const FActivityEvent& Event) const
{
	ensureMsgf(Event.FmodAudioEvent, TEXT("A valid audio event has not been selected!"));
	if (!Event.FmodAudioEvent)
		return;

	UFMODBlueprintStatics::PlayEventAttached(Event.FmodAudioEvent, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
}

void AActivityTriggerVolume::HandleUnrealAudioEvent(const FActivityEvent& Event) const
{
#if !UE_BUILD_SHIPPING
	ensureMsgf(Event.UnrealAudioEvent, TEXT("A valid audio event has not been selected!"));
	if (!Event.UnrealAudioEvent)
		return;

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Event.UnrealAudioEvent, GetActorLocation());
#endif
}

UTutorialWidget* AActivityTriggerVolume::GetTutorialWidget() const
{
	if (const APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
	{
		if (LocalPlayer->HumanCharacterWidget_V2)
		{
			return LocalPlayer->HumanCharacterWidget_V2->GetTutorialWidget();
		}
	}
	return nullptr;
}

void AActivityTriggerVolume::ShowTutorialWidget() const
{
	FTutorialWidgetData WidgetData;
	if (SetTooltipToNextActivity(WidgetData))
	{
		UTutorialWidget* TutorialWidget = GetTutorialWidget();
		if (!TutorialWidget)
		{
			// If the tutorial widget is not valid, try again in 0.25 seconds (to allow the widget to be created)
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AActivityTriggerVolume::ShowTutorialWidget, 0.25f);
			return;
		}

		TutorialWidget->SetData(WidgetData);
		TutorialWidget->ShowMainWidget();
		return;
	}

	// If no activities are valid, hide the tooltip instead
	HideTutorialWidget();
}

void AActivityTriggerVolume::HideTutorialWidget() const
{
	UTutorialWidget* TutorialWidget = GetTutorialWidget();
	if (TutorialWidget)
	{
		TutorialWidget->HideMainWidget();
	}
}

bool AActivityTriggerVolume::SetTooltipToNextActivity(FTutorialWidgetData& OutWidgetData) const
{
	for (const UActivityData* Activity : Activities)
	{
		if (!Activity || Activity->IsComplete())
			continue;

		if (Activity->Activity == EActivity::A_Delay)
			break;

		OutWidgetData = Activity->WidgetData;
		return true;
	}
	return false;
}

UScreenspaceMarkerWidget* AActivityTriggerVolume::GetScreenspaceMarkerWidget() const
{
	if (const APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
	{
		if (LocalPlayer->HumanCharacterWidget_V2)
		{
			return LocalPlayer->HumanCharacterWidget_V2->GetScreenspaceMarkerWidget();
		}
	}
	return nullptr;
}

void AActivityTriggerVolume::ShowScreenspaceMarker() const
{
	UScreenspaceMarkerWidget* ScreenspaceMarkerWidget = GetScreenspaceMarkerWidget();
	if (ensureAlways(ScreenspaceMarkerWidget))
	{
		ScreenspaceMarkerWidget->ShowWidget();
	}
}

void AActivityTriggerVolume::HideScreenspaceMarker() const
{
	UScreenspaceMarkerWidget* ScreenspaceMarkerWidget = GetScreenspaceMarkerWidget();
	if (ensureAlways(ScreenspaceMarkerWidget))
	{
		ScreenspaceMarkerWidget->HideWidget();
	}
}

void AActivityTriggerVolume::OnSpawnedAiKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		SwatManager->RespondToPlayerTeamKill(InstigatorCharacter);
	}
}
