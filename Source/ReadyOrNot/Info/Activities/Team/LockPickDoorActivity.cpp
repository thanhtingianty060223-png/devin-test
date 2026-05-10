// Void Interactive, 2020

#include "Info/Activities/Team/LockPickDoorActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/Door.h"

#include "ReadyOrNotAIConfig.h"

ULockPickDoorActivity::ULockPickDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "PickLock");
	bReturnToPositionAfterInteraction = true;
}

bool ULockPickDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (Door)
	{
		FocalPoint = Door->GetLockpickInteractableComponent()->GetComponentLocation();
		return true;
	}
	
	return Super::OverrideFocalPoint(FocalPoint);
}

void ULockPickDoorActivity::EnterGetInPositionStage()
{
	Super::EnterGetInPositionStage();

	ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForLockPicking");
}

void ULockPickDoorActivity::OnInteractionBegin()
{
	Super::OnInteractionBegin();

	ProgressState = FText::FromStringTable("SwatCommandTable", "LockPickingDoor");
}

void ULockPickDoorActivity::OnInteractionEnd()
{
	Super::OnInteractionEnd();
	
	if (CheckEdgeCases())
	{
		Door->UnlockDoor();

		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCK_PICKED);
		//GetCharacter()->PlaySpeechFromLookupTable(DialogueStringUtilities::CALL_DOOR_LOCK_PICKED);

		ProgressState = FText::FromStringTable("SwatCommandTable", "DoorUnlocked");
	}
}

void ULockPickDoorActivity::BindEvents()
{
	Super::BindEvents();

	GetCharacter()->OnDoorLockPickBegin_FromAnimNotify.AddDynamic(this, &ULockPickDoorActivity::OnInteractionBegin);
	GetCharacter()->OnDoorLockPickEnd_FromAnimNotify.AddDynamic(this, &ULockPickDoorActivity::OnInteractionEnd);
}

void ULockPickDoorActivity::UnbindEvents()
{
	Super::UnbindEvents();

	if (!GetCharacter())
		return;

	GetCharacter()->OnDoorLockPickBegin_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnDoorLockPickEnd_FromAnimNotify.RemoveAll(this);
}

bool ULockPickDoorActivity::CheckEdgeCases()
{
	if (!Super::CheckEdgeCases())
		return false;

	if (!Door->IsLocked())
	{
		ACTIVITY_FAILED("Door is already unlocked");
		return false;
	}
	
	return true;
}

bool ULockPickDoorActivity::CanInteract() const
{
	return Super::CanInteract() && Door->IsLocked();
}

float ULockPickDoorActivity::GetInteractionDistance() const
{
	return 110.0f;
}

FString ULockPickDoorActivity::GetInteractionAnimation() const
{
	return "tp_swt_lockpick";
}
