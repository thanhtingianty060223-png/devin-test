// Copyright Void Interactive, 2023

#include "ActivityData.h"
#include "ReadyOrNotGameMode.h"
#include "Actors/BaseGrenade.h"
#include "Actors/Door.h"
#include "Actors/TrainingTarget.h"
#include "Actors/Attachments/LaserAttachment.h"
#include "Actors/Environment/BaseTriggerable.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Items/Detonator.h"
#include "Actors/Items/DoorJam.h"
#include "Actors/Items/DoorRam.h"
#include "Actors/Items/LockpickGun.h"
#include "Actors/Items/Multitool.h"
#include "Actors/Items/Optiwand.h"
#include "Characters/AI/SWATCharacter.h"
#include "Components/InteractableComponent.h"
#include "HUD/Widgets/SwatCommandWidget.h"

void UActivityData::BindPlayerToActivity(APlayerCharacter* Player)
{
	if (!Player)
		return;
	PlayerCharacter = Player;

	// Setup bindings/checks according to the selected activity
	switch(Activity)
	{
	case EActivity::A_GoToLocation:
		CompleteActivity();
		return;
	case EActivity::A_Delay:
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UActivityData::CompleteActivity, TimeRequired);
		return;
	case EActivity::A_UIOnly:
		return;
	case EActivity::A_MoveForward:
	case EActivity::A_MoveBackward:
	case EActivity::A_MoveRight:
	case EActivity::A_MoveLeft:
	case EActivity::A_MoveForwardLowReady:
		BIND_DELEGATE(this, PlayerCharacter->OnCharacterMovementUpdated, &UActivityData::OnCharacterMovementUpdated)
		return;
	case EActivity::A_Interact:
	case EActivity::A_InteractTriggerable:
	case EActivity::A_OpenDoor:
		BIND_DELEGATE(this, PlayerCharacter->OnInteract, &UActivityData::OnInteract)
		return;
	case EActivity::A_SecureEvidence:
		BIND_DELEGATE(this, PlayerCharacter->OnEvidenceCollected, &UActivityData::OnEvidenceCollected);
		for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
		{
			BIND_DELEGATE(this, It->OnEvidenceCollected, &UActivityData::OnEvidenceCollected)
		}
		return;
	case EActivity::A_EquipPrimary:
	case EActivity::A_EquipSecondary:
		BIND_DELEGATE(this, PlayerCharacter->GetInventoryComponent()->OnItemEquipped, &UActivityData::OnItemEquipped)
		return;
	case EActivity::A_ShootPrimaryHip:
	case EActivity::A_ShootPrimaryADS:
	case EActivity::A_ShootSecondaryHip:
	case EActivity::A_ShootSecondaryADS:
		BIND_DELEGATE(this, PlayerCharacter->OnWeaponFire, &UActivityData::OnWeaponFire)
		return;
	case EActivity::A_Reload:
	case EActivity::A_ReloadAiming:
		BIND_DELEGATE(this, PlayerCharacter->OnWeaponTacticalReload, &UActivityData::OnWeaponTacticalReload)
		return;
	case EActivity::A_ReloadEmpty:
	case EActivity::A_ReloadQuick:
		BIND_DELEGATE(this, PlayerCharacter->OnWeaponReload, &UActivityData::OnWeaponReload)
		return;
	case EActivity::A_SwitchAmmoType:
		BIND_DELEGATE(this, PlayerCharacter->OnWeaponSwitchAmmoType, &UActivityData::OnWeaponSwitchAmmoType)
		return;
	case EActivity::A_SwitchFireMode:
		BIND_DELEGATE(this, PlayerCharacter->OnWeaponFireModeChanged, &UActivityData::OnWeaponFireModeChanged)
		return;
	case EActivity::A_ToggleTacticalLight:
		BIND_DELEGATE(this, PlayerCharacter->OnAttachmentLightToggled, &UActivityData::OnAttachmentLightToggled)
		return;
	case EActivity::A_ToggleCantedSight:
		BIND_DELEGATE(this, PlayerCharacter->OnCantedSightToggled, &UActivityData::OnCantedSightToggled)
		return;
	case EActivity::A_UseChemlight:
		BIND_DELEGATE(this, PlayerCharacter->OnChemlightThrown, &UActivityData::OnChemlightThrown)
		return;
	case EActivity::A_ThrowFlashbangGrenade:
	case EActivity::A_ThrowCSGasGrenade:
	case EActivity::A_ThrowStingerGrenade:
	case EActivity::A_ThrowNineBangerGrenade:
	case EActivity::A_UseOptiwand:
	case EActivity::A_UseDoorjam:
	case EActivity::A_UseBatteringRam:
		BIND_DELEGATE(this, PlayerCharacter->OnItemUseStart, &UActivityData::OnItemUseStart)
		return;
	case EActivity::A_UseC2Explosive:
	case EActivity::A_UseLockpick:
		BIND_DELEGATE(this, PlayerCharacter->OnItemUseCompleted, &UActivityData::OnItemUseCompleted)
		return;
	case EActivity::A_UseNVGs:
		BIND_DELEGATE(this, PlayerCharacter->OnNightVisionGogglesToggled, &UActivityData::OnNightVisionGogglesToggled)
		return;
	case EActivity::A_SwitchSwatElement:
		// If the SWAT command widget is not valid, try again in 0.25 seconds (to allow the widget to be created)
		if (!PlayerCharacter->SwatCommandWidget)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &UActivityData::BindPlayerToActivity, PlayerCharacter), 0.25f);
			return;
		}

		BIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatElementChanged, &UActivityData::OnSwatElementChanged)
		return;
	case EActivity::A_IssueSwatCommand:
		// If the SWAT command widget is not valid, try again in 0.25 seconds (to allow the widget to be created)
		if (!PlayerCharacter->SwatCommandWidget)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &UActivityData::BindPlayerToActivity, PlayerCharacter), 0.25f);
			return;
		}

		if (SwatCommandData.bQueue)
			BIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandQueued, &UActivityData::OnSwatCommandQueued)
		else
			BIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandIssued, &UActivityData::OnSwatCommandIssued)
		return;
	case EActivity::A_ArrestOrKillAi:
		if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			BIND_DELEGATE(this, GameState->OnCharacterArrested, &UActivityData::OnCharacterArrested)
			BIND_DELEGATE(this, GameState->OnCharacterKilled, &UActivityData::OnCharacterKilled)
		}
		return;
	case EActivity::A_ShootTarget:
	case EActivity::A_ShootTargetADS:
	case EActivity::A_ShootTargetCanted:
	case EActivity::A_ShootTargetLaser:
		for (TActorIterator<ATrainingTarget> It(GetWorld()); It; ++It)
			BIND_DELEGATE(this, It->OnSuccessfulShot, &UActivityData::OnTargetHit)
		return;
	case EActivity::A_GrenadeTarget:
		for (TActorIterator<ATrainingTarget> It(GetWorld()); It; ++It)
			BIND_DELEGATE(this, It->OnGrenadeHit, &UActivityData::OnTargetHit)
		return;
	case EActivity::A_SwitchTeamCamera:
		BIND_DELEGATE(this, PlayerCharacter->OnTeamViewSet, &UActivityData::OnTeamViewSet)
		return;
	case EActivity::A_Exfiltrate:
		if (AReadyOrNotGameMode* GameMode = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>())
			BIND_DELEGATE(this, GameMode->OnExfiltrateMission, &UActivityData::OnExfiltrateMission)
		return;
	}
	ensureMsgf(false, TEXT("Activity not handled!"));
}

void UActivityData::UnbindPlayerFromActivity()
{
	ensureMsgf(PlayerCharacter != nullptr, TEXT("PlayerCharacter is already null!"));
	if (!PlayerCharacter)
		return;

	/** Note: Ensure that all delegates are unbound here!
	 * As when the player leaves the trigger volume, the
	 * delegates should no longer trigger activities. */
	UNBIND_DELEGATE(this, PlayerCharacter->OnCharacterMovementUpdated, &UActivityData::OnCharacterMovementUpdated);
	UNBIND_DELEGATE(this, PlayerCharacter->OnInteract, &UActivityData::OnInteract);
	UNBIND_DELEGATE(this, PlayerCharacter->OnEvidenceCollected, &UActivityData::OnEvidenceCollected);
	for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
	{
		UNBIND_DELEGATE(this, It->OnEvidenceCollected, &UActivityData::OnEvidenceCollected);
	}
	UNBIND_DELEGATE(this, PlayerCharacter->GetInventoryComponent()->OnItemEquipped, &UActivityData::OnItemEquipped)
	UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponFire, &UActivityData::OnWeaponFire);
	UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponReload, &UActivityData::OnWeaponReload);
	UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponTacticalReload, &UActivityData::OnWeaponTacticalReload);
	UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponSwitchAmmoType, &UActivityData::OnWeaponSwitchAmmoType);
	UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponFireModeChanged, &UActivityData::OnWeaponFireModeChanged);
	UNBIND_DELEGATE(this, PlayerCharacter->OnAttachmentLightToggled, &UActivityData::OnAttachmentLightToggled);
	UNBIND_DELEGATE(this, PlayerCharacter->OnCantedSightToggled, &UActivityData::OnCantedSightToggled);
	UNBIND_DELEGATE(this, PlayerCharacter->OnChemlightThrown, &UActivityData::OnChemlightThrown);
	UNBIND_DELEGATE(this, PlayerCharacter->OnNightVisionGogglesToggled, &UActivityData::OnNightVisionGogglesToggled);
	UNBIND_DELEGATE(this, PlayerCharacter->OnItemUseStart, &UActivityData::OnItemUseStart);
	UNBIND_DELEGATE(this, PlayerCharacter->OnItemUseCompleted, &UActivityData::OnItemUseCompleted);
	UNBIND_DELEGATE(this, PlayerCharacter->OnTeamViewSet, &UActivityData::OnTeamViewSet);
	if (AReadyOrNotGameMode* GameMode = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>())
	{
		UNBIND_DELEGATE(this, GameMode->OnExfiltrateMission, &UActivityData::OnExfiltrateMission);
	}
	if (PlayerCharacter->SwatCommandWidget)
	{
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandIssued, &UActivityData::OnSwatCommandIssued)
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandQueued, &UActivityData::OnSwatCommandQueued)
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatElementChanged, &UActivityData::OnSwatElementChanged)
	}
	for (TActorIterator<ATrainingTarget> It(GetWorld()); It; ++It)
	{
		UNBIND_DELEGATE(this, It->OnSuccessfulShot, &UActivityData::OnTargetHit)
		UNBIND_DELEGATE(this, It->OnGrenadeHit, &UActivityData::OnTargetHit)
	}
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		UNBIND_DELEGATE(this, GameState->OnCharacterArrested, &UActivityData::OnCharacterArrested)
		UNBIND_DELEGATE(this, GameState->OnCharacterKilled, &UActivityData::OnCharacterKilled)
	}

	ensureMsgf(!bDelegateBound, TEXT("A delegate function was not unbound!"));

	PlayerCharacter = nullptr;
}

void UActivityData::ResetProgress()
{
	ActionsCompleted = 0;
	TimeElapsed = 0.0f;
	bIsComplete = false;
}

bool UActivityData::IsComplete() const
{
	if (bIsComplete)
		return true;

	if (ActionsCompleted >= ActionsRequired)
		return true;

	if (TimeElapsed >= TimeRequired)
		return true;

	return false;
}

#if WITH_EDITOR
bool UActivityData::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	const FName PropertyName = InProperty->GetFName();

	// Specific logic associated with "ActionsRequired"
	if(PropertyName == GET_MEMBER_NAME_CHECKED(UActivityData, ActionsRequired))
	{
		return CanEditActionsRequired();
	}

	// Specific logic associated with "TimeRequired"
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UActivityData, TimeRequired))
	{
		return CanEditTimeRequired();
	}

	return true;
}

bool UActivityData::CanEditActionsRequired() const
{
	switch(Activity)
	{
	case EActivity::A_Delay:
	case EActivity::A_GoToLocation:
	case EActivity::A_MoveForward:
	case EActivity::A_MoveBackward:
	case EActivity::A_MoveRight:
	case EActivity::A_MoveLeft:
	case EActivity::A_MoveForwardLowReady:
	case EActivity::A_Exfiltrate:
		return false;
	case EActivity::A_UIOnly:
	case EActivity::A_Interact:
	case EActivity::A_InteractTriggerable:
	case EActivity::A_OpenDoor:
	case EActivity::A_SecureEvidence:
	case EActivity::A_EquipPrimary:
	case EActivity::A_EquipSecondary:
	case EActivity::A_ShootPrimaryHip:
	case EActivity::A_ShootPrimaryADS:
	case EActivity::A_ShootSecondaryHip:
	case EActivity::A_ShootSecondaryADS:
	case EActivity::A_Reload:
	case EActivity::A_ReloadAiming:
	case EActivity::A_ReloadEmpty:
	case EActivity::A_ReloadQuick:
	case EActivity::A_SwitchAmmoType:
	case EActivity::A_SwitchFireMode:
	case EActivity::A_ToggleTacticalLight:
	case EActivity::A_ToggleCantedSight:
	case EActivity::A_UseChemlight:
	case EActivity::A_ThrowFlashbangGrenade:
	case EActivity::A_ThrowCSGasGrenade:
	case EActivity::A_ThrowStingerGrenade:
	case EActivity::A_ThrowNineBangerGrenade:
	case EActivity::A_UseOptiwand:
	case EActivity::A_UseDoorjam:
	case EActivity::A_UseBatteringRam:
	case EActivity::A_UseC2Explosive:
	case EActivity::A_UseLockpick:
	case EActivity::A_UseNVGs:
	case EActivity::A_IssueSwatCommand:
	case EActivity::A_ArrestOrKillAi:
	case EActivity::A_ShootTarget:
	case EActivity::A_ShootTargetADS:
	case EActivity::A_ShootTargetCanted:
	case EActivity::A_ShootTargetLaser:
	case EActivity::A_GrenadeTarget:
	case EActivity::A_SwitchTeamCamera:
	case EActivity::A_SwitchSwatElement:
		return true;
	}
	ensureMsgf(false, TEXT("Activity not handled"));
	return true;
}

bool UActivityData::CanEditTimeRequired() const
{
	switch(Activity)
	{
	case EActivity::A_Delay:
	case EActivity::A_MoveForward:
	case EActivity::A_MoveBackward:
	case EActivity::A_MoveRight:
	case EActivity::A_MoveLeft:
	case EActivity::A_MoveForwardLowReady:
		return true;
	case EActivity::A_GoToLocation:
	case EActivity::A_UIOnly:
	case EActivity::A_Interact:
	case EActivity::A_InteractTriggerable:
	case EActivity::A_OpenDoor:
	case EActivity::A_SecureEvidence:
	case EActivity::A_EquipPrimary:
	case EActivity::A_EquipSecondary:
	case EActivity::A_ShootPrimaryHip:
	case EActivity::A_ShootPrimaryADS:
	case EActivity::A_ShootSecondaryHip:
	case EActivity::A_ShootSecondaryADS:
	case EActivity::A_Reload:
	case EActivity::A_ReloadAiming:
	case EActivity::A_ReloadEmpty:
	case EActivity::A_ReloadQuick:
	case EActivity::A_SwitchAmmoType:
	case EActivity::A_SwitchFireMode:
	case EActivity::A_ToggleTacticalLight:
	case EActivity::A_ToggleCantedSight:
	case EActivity::A_UseChemlight:
	case EActivity::A_ThrowFlashbangGrenade:
	case EActivity::A_ThrowCSGasGrenade:
	case EActivity::A_ThrowStingerGrenade:
	case EActivity::A_ThrowNineBangerGrenade:
	case EActivity::A_UseOptiwand:
	case EActivity::A_UseDoorjam:
	case EActivity::A_UseBatteringRam:
	case EActivity::A_UseC2Explosive:
	case EActivity::A_UseLockpick:
	case EActivity::A_UseNVGs:
	case EActivity::A_IssueSwatCommand:
	case EActivity::A_ArrestOrKillAi:
	case EActivity::A_Exfiltrate:
	case EActivity::A_ShootTarget:
	case EActivity::A_ShootTargetADS:
	case EActivity::A_ShootTargetCanted:
	case EActivity::A_ShootTargetLaser:
	case EActivity::A_GrenadeTarget:
	case EActivity::A_SwitchTeamCamera:
	case EActivity::A_SwitchSwatElement:
		return false;
	}
	ensureMsgf(false, TEXT("Activity not handled"));
	return true;
}
#endif

void UActivityData::CompleteActivity()
{
	bIsComplete = true;
	OnComplete.Broadcast(this);
}

void UActivityData::OnCharacterMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	switch(Activity)
	{
	case EActivity::A_MoveForward:
		if (PlayerCharacter->MoveForwardInput > 0)
		{
			TimeElapsed += DeltaSeconds;
		}
		break;
	case EActivity::A_MoveBackward:
		if (PlayerCharacter->MoveForwardInput < 0)
		{
			TimeElapsed += DeltaSeconds;
		}
		break;
	case EActivity::A_MoveRight:
		if (PlayerCharacter->MoveRightInput > 0)
		{
			TimeElapsed += DeltaSeconds;
		}
		break;
	case EActivity::A_MoveLeft:
		if (PlayerCharacter->MoveRightInput < 0)
		{
			TimeElapsed += DeltaSeconds;
		}
		break;
	case EActivity::A_MoveForwardLowReady:
		if (PlayerCharacter->bUserLowReady && PlayerCharacter->MoveForwardInput > 0)
		{
			TimeElapsed += DeltaSeconds;
		}
		break;
	default:
		ensureMsgf(false, TEXT("Movement Activity not handled"));
		break;
	}
	
	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnCharacterMovementUpdated, &UActivityData::OnCharacterMovementUpdated)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnInteract(UInteractableComponent* InteractableComponent)
{
	switch(Activity)
	{
	case EActivity::A_Interact:
		ActionsCompleted++;
		break;
	case EActivity::A_InteractTriggerable:
		if (Cast<ABaseTriggerable>(InteractableComponent->GetUseActor()))
			ActionsCompleted++;
		break;
	case EActivity::A_OpenDoor:
		if (ADoor* Door = Cast<ADoor>(InteractableComponent->GetUseActor()))
		{
			const bool bOpenedOrPushed = InteractableComponent == Door->GetOpenInteractableComponent() || InteractableComponent == Door->GetPushInteractableComponent();
			if (bOpenedOrPushed)
				ActionsCompleted++;

			if (InteractableComponent == Door->GetKickInteractableComponent())
			{
				BIND_DELEGATE(this, Door->OnDoorKicked, &UActivityData::OnDoorKicked)
			}
		}
		break;
	default:
		ensureMsgf(false, TEXT("Interact Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnInteract, &UActivityData::OnInteract)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnEvidenceCollected(AActor* Evidence)
{
	switch(Activity)
	{
	case EActivity::A_SecureEvidence:
		if (Cast<ABaseItem>(Evidence))
			ActionsCompleted++;
		if (Cast<AEvidenceActor>(Evidence))
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Secure Evidence Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnEvidenceCollected, &UActivityData::OnEvidenceCollected)
		for (TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
		{
			UNBIND_DELEGATE(this, It->OnEvidenceCollected, &UActivityData::OnEvidenceCollected);
		}
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnItemEquipped(ABaseItem* Item)
{
	switch(Activity)
	{
	case EActivity::A_EquipPrimary:
		if (Item->ContainsItemCategory(EItemCategory::IC_Primary))
			ActionsCompleted++;
		break;
	case EActivity::A_EquipSecondary:
		if (Item->ContainsItemCategory(EItemCategory::IC_Secondary))
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Interact Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->GetInventoryComponent()->OnItemEquipped, &UActivityData::OnItemEquipped)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnDoorKicked(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter, bool bSuccess)
{
	if (bSuccess)
		ActionsCompleted++;

	// Always unbind after a kick as we have no way to store a reference to the door
	UNBIND_DELEGATE(this, Door->OnDoorKicked, &UActivityData::OnDoorKicked)

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnInteract, &UActivityData::OnInteract)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection)
{
	const bool bIsWeaponPrimary = PlayerCharacter->GetEquippedItem()->ContainsItemCategory(EItemCategory::IC_Primary);
	const bool bIsWeaponSecondary = PlayerCharacter->GetEquippedItem()->ContainsItemCategory(EItemCategory::IC_Secondary);

	switch(Activity)
	{
	case EActivity::A_ShootPrimaryHip:
		if (bIsWeaponPrimary && !PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	case EActivity::A_ShootPrimaryADS:
		if (bIsWeaponPrimary && PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	case EActivity::A_ShootSecondaryHip:
		if (bIsWeaponSecondary && !PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	case EActivity::A_ShootSecondaryADS:
		if (bIsWeaponSecondary && PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Shoot Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponFire, &UActivityData::OnWeaponFire)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnWeaponReload(APlayerCharacter* DelegatePlayerCharacter)
{
	switch(Activity)
	{
	case EActivity::A_ReloadEmpty:
		if (PlayerCharacter->GetEquippedWeapon()->GetAmmo() <= 0)
			ActionsCompleted++;
		break;
	case EActivity::A_ReloadQuick:
		if (PlayerCharacter->GetEquippedWeapon()->GetAmmo() > 0)
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Reload Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponReload, &UActivityData::OnWeaponReload)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnWeaponTacticalReload(APlayerCharacter* DelegatePlayerCharacter)
{
	switch(Activity)
	{
	case EActivity::A_Reload:
		if (PlayerCharacter->GetEquippedWeapon()->GetAmmo() > 0)
			ActionsCompleted++;
		break;
	case EActivity::A_ReloadAiming:
		if (PlayerCharacter->GetEquippedWeapon()->GetAmmo() > 0 && PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Tactical Reload Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponTacticalReload, &UActivityData::OnWeaponTacticalReload)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnWeaponSwitchAmmoType(APlayerCharacter* DelegatePlayerCharacter)
{
	ActionsCompleted++;
	
	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponSwitchAmmoType, &UActivityData::OnWeaponSwitchAmmoType)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnWeaponFireModeChanged(APlayerCharacter* DelegatePlayerCharacter, EFireMode NewFireMode, EFireMode LastFireMode)
{
	ActionsCompleted++;
	
	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnWeaponFireModeChanged, &UActivityData::OnWeaponFireModeChanged)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnAttachmentLightToggled()
{
	ActionsCompleted++;
	
	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnAttachmentLightToggled, &UActivityData::OnAttachmentLightToggled)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnCantedSightToggled(bool bUsingCantedSight)
{
	ActionsCompleted++;

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnCantedSightToggled, &UActivityData::OnCantedSightToggled)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnItemUseStart(ABaseItem* Item)
{
	switch(Activity)
	{
	case EActivity::A_ThrowFlashbangGrenade:
		if (Cast<ABaseGrenade>(Item)->Type == EGrenadeType::Flashbang)
			ActionsCompleted++;
		break;
	case EActivity::A_ThrowCSGasGrenade:
		if (Cast<ABaseGrenade>(Item)->Type == EGrenadeType::Gas)
			ActionsCompleted++;
		break;
	case EActivity::A_ThrowStingerGrenade:
		if (Cast<ABaseGrenade>(Item)->Type == EGrenadeType::Stinger)
			ActionsCompleted++;
		break;
	case EActivity::A_ThrowNineBangerGrenade:
		if (Cast<ABaseGrenade>(Item)->Type == EGrenadeType::Flashbang)
			ActionsCompleted++;
		break;
	case EActivity::A_UseOptiwand:
		if (Cast<AOptiwand>(Item))
		{
			if (PlayerCharacter->LastInteractableComponent)
			{
				const ADoor* Door = Cast<ADoor>(PlayerCharacter->LastInteractableComponent->GetUseActor());
				// If the player interacted with a Door's Optiwand Interactable Component
				if (Door && Door->GetOptiwandInteractableComponent() == PlayerCharacter->LastInteractableComponent)
					ActionsCompleted++;
			}
		}
		break;
	case EActivity::A_UseDoorjam:
		if (Cast<ADoorJam>(Item))
			ActionsCompleted++;
		break;
	case EActivity::A_UseBatteringRam:
		if (Cast<ADoorRam>(Item))
			ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Item Use Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, Item->OnItemPrimaryUseStart, &UActivityData::OnItemUseStart)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnItemUseCompleted(ABaseItem* Item)
{
	switch(Activity)
	{
	case EActivity::A_UseC2Explosive:
		if (Cast<ADetonator>(Item))
			ActionsCompleted++;
		break;
	case EActivity::A_UseLockpick:
		if (Cast<AMultitool>(Item) || Cast<ALockpickGun>(Item))
		{
			// If the last interactable component is a door and the used item is a multi-tool/lockpick gun
			if (PlayerCharacter->LastInteractableComponent && PlayerCharacter->LastInteractableComponent->CanInteract())
			{
				const ADoor* Door = Cast<ADoor>(PlayerCharacter->LastInteractableComponent->GetUseActor());
				if (Door && Door->GetLockpickInteractableComponent() == PlayerCharacter->LastInteractableComponent)
				{	
					ActionsCompleted++;
				}
			}
		}
		break;
	default:
		ensureMsgf(false, TEXT("Item Use Completed Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, Item->OnItemUseCompleted, &UActivityData::OnItemUseCompleted)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnChemlightThrown(APlayerCharacter* DelegatePlayerCharacter)
{
	ActionsCompleted++;

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnChemlightThrown, &UActivityData::OnChemlightThrown)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnNightVisionGogglesToggled(AReadyOrNotCharacter* Character, bool bNVGOn)
{
	ActionsCompleted++;

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnNightVisionGogglesToggled, &UActivityData::OnNightVisionGogglesToggled)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnSwatCommandIssued(const ESwatCommand SwatCommand, const ETeamType TeamType, AActor* ContextActor)
{
	if (SwatCommandData.Command == SwatCommand)
	{
		if (SwatCommandData.Team == TeamType || SwatCommandData.Team == ETeamType::TT_NONE)
		{
			const AActor* TargetActor = SwatCommandData.Target.Get();
			if (TargetActor == nullptr || TargetActor == ContextActor)
			{
				ActionsCompleted++;
			}
		}
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandIssued, &UActivityData::OnSwatCommandIssued)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnSwatCommandQueued(const FQueuedSwatCommand QueuedSwatCommand, const ETeamType TeamType)
{
	if (QueuedSwatCommand.Command.CommandType == SwatCommandData.Command)
	{
		if (SwatCommandData.Team == TeamType || SwatCommandData.Team == ETeamType::TT_NONE)
		{
			const AActor* TargetActor = SwatCommandData.Target.Get();
			if (TargetActor == nullptr || TargetActor == QueuedSwatCommand.ContextualData.GetActor())
			{
				ActionsCompleted++;
			}
		}
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatCommandQueued, &UActivityData::OnSwatCommandQueued)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnCharacterArrested(AReadyOrNotCharacter* Character, AReadyOrNotCharacter* ArrestedBy)
{
	// If the character has not been killed during the activity - prevents double ups (player can arrest and kill the same character)
	if (!CharactersKilled.Contains(Character))
	{
		ActionsCompleted++;
	}

	if (IsComplete())
	{
		if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
			UNBIND_DELEGATE(this, GameState->OnCharacterArrested, &UActivityData::OnCharacterArrested)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnCharacterKilled(AReadyOrNotCharacter* Character, AReadyOrNotCharacter* KilledBy)
{
	ActionsCompleted++;

	// Add the character to the list of killed characters for this activity
	CharactersKilled.Add(Character);

	if (IsComplete())
	{
		if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
			UNBIND_DELEGATE(this, GameState->OnCharacterKilled, &UActivityData::OnCharacterKilled)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnTargetHit(ATrainingTarget* Target)
{
	switch(Activity)
	{
	case EActivity::A_ShootTarget:
		ActionsCompleted++;
		break;
	case EActivity::A_ShootTargetADS:
		if (PlayerCharacter->bAiming)
			ActionsCompleted++;
		break;
	case EActivity::A_GrenadeTarget:
		ActionsCompleted++;
		break;
	case EActivity::A_ShootTargetCanted:
		if (PlayerCharacter->bCantedSightEnabled)
			ActionsCompleted++;
		break;
	case EActivity::A_ShootTargetLaser:
		if (ABaseMagazineWeapon* Weapon = PlayerCharacter->GetEquippedWeapon())
			if (Weapon->GetLaserAttachment() && Weapon->GetLaserAttachment()->IsLaserOn())
				ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Arrest Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		for (TActorIterator<ATrainingTarget> It(GetWorld()); It; ++It)
		{
			UNBIND_DELEGATE(this, It->OnSuccessfulShot, &UActivityData::OnTargetHit)
			UNBIND_DELEGATE(this, It->OnGrenadeHit, &UActivityData::OnTargetHit)
		}
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnTeamViewSet(AReadyOrNotCharacter* NewViewCharacter)
{
	switch(Activity)
	{
	case EActivity::A_SwitchTeamCamera:
		ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Team Camera Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->OnTeamViewSet, &UActivityData::OnTeamViewSet)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnSwatElementChanged(ETeamType TeamType)
{
	switch(Activity)
	{
	case EActivity::A_SwitchSwatElement:
		ActionsCompleted++;
		break;
	default:
		ensureMsgf(false, TEXT("Activity not handled"));
		break;
	}

	if (IsComplete())
	{
		UNBIND_DELEGATE(this, PlayerCharacter->SwatCommandWidget->OnSwatElementChanged, &UActivityData::OnSwatElementChanged)
		OnComplete.Broadcast(this);
	}
}

void UActivityData::OnExfiltrateMission()
{
	bIsComplete = true;

	if (AReadyOrNotGameMode* GameMode = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>())
	{
		UNBIND_DELEGATE(this, GameMode->OnExfiltrateMission, &UActivityData::OnExfiltrateMission)
		OnComplete.Broadcast(this);
	}
}
