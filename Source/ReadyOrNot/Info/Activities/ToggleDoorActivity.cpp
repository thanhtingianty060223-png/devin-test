// Void Interactive, 2020

#include "ToggleDoorActivity.h"

#include "Actors/Door.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Team/TeamStackUpActivity.h"

#include "Info/SWATManager.h"

#include "ReadyOrNotAIConfig.h"

UToggleDoorActivity::UToggleDoorActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "ToggleDoor");
}

bool UToggleDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (Door)
	{
		FocalPoint = Door->GetDoorMidLocation();
		return true;
	}
    
	return false;
}

bool UToggleDoorActivity::CheckEdgeCases()
{
	if (!Super::CheckEdgeCases())
		return false;

	if (bOpenDoor)
	{
		if (!Door->CanOpenDoor(GetCharacter()))
		{
			ACTIVITY_FAILED("Door is already opened");
			return false;
		}
	}
	else
	{
		if (Door->IsClosed())
		{
			ACTIVITY_FAILED("Door is already closed");
			return false;
		}
	}

	return true;
}

void UToggleDoorActivity::EnterGetInPositionStage()
{
	Super::EnterGetInPositionStage();

	ProgressState = FText::FromStringTable("SwatCommandTable", "GettingInPosition");
}

void UToggleDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	if (!Door->IsOpening() && !Door->IsAnyAIOpening())
	{
		ToggleDoor();
	}
}

void UToggleDoorActivity::PerformGetInPositionStage(float DeltaTime, float Uptime)
{
	Super::PerformGetInPositionStage(DeltaTime, Uptime);
}

void UToggleDoorActivity::EnterInteractStage()
{
	if (CheckEdgeCases())
	{
		ToggleDoor();
	}
}

void UToggleDoorActivity::OnDoorOpened()
{
	OwningController->FinishActivity(this, true, true);
}

void UToggleDoorActivity::OnDoorClosed()
{
	OwningController->FinishActivity(this, true, true);
}

void UToggleDoorActivity::OnDoorMovementBlocked()
{
	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
	OwningController->FinishActivity(this, true, true);
}

float UToggleDoorActivity::GetInteractionDistance() const
{
	return 25.0f;
}

FVector UToggleDoorActivity::GetInteractionLocation() const
{
	if (OwningController->GetActivity<UTeamStackUpActivity>())
		return FVector::ZeroVector;
	
	if (Door)
	{
		const bool bIsCommandFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
		const bool bIsOnFrontSideOfDoor = Door->IsPointInFrontOfDoorway(GetCharacter()->GetActorLocation());
		const bool bIsOnTacticalSideOfDoor = Door->IsActorRightOfDoorway(GetCharacter());
		const bool bSameSideAsCommand = bIsCommandFrontOfDoor == bIsOnFrontSideOfDoor;

		EStackupGenArea CurrentStackUpArea = Door->FindStackUpAreaFromLocation(GetCharacter()->GetActorLocation());
		
		if (bOpenDoor)
		{
			// Flip over to the tactical side of the door, if closed
			if (Door->IsClosed() && !bIsOnTacticalSideOfDoor)
				ADoor::FlipStackUpArea(CurrentStackUpArea, true, !bSameSideAsCommand);
		}
		else
		{
			if (!bSameSideAsCommand)
			{
				ADoor::FlipStackUpArea(CurrentStackUpArea, bIsOnTacticalSideOfDoor, true);
			}
		}
		
		return Door->GetBestDoorInteraction_FromStackUpArea(CurrentStackUpArea);
	}
	
	return FVector::ZeroVector;
}

bool UToggleDoorActivity::CanInteract() const
{
	if (bOpenDoor)
		return Super::CanInteract() && !Door->CanCloseDoor(GetCharacter());

	return Super::CanInteract() && Door->CanCloseDoor(GetCharacter());
}

void UToggleDoorActivity::ToggleDoor()
{
	// todo: play check anim before trying to open. For now, just magically know if door is locked or not.
	if (Door->IsLocked())
	{
		Door->SetDoorLockKnowledge(GetCharacter()->IsSuspect(), true);
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCKED);
		ACTIVITY_FAILED("Door is locked", true);
		return;
	}
		
	GetCharacter()->ToggleDoor(Door, bOpenDoor);

	ProgressState = bOpenDoor ? FText::FromStringTable("SwatCommandTable", "OpeningDoor") : FText::FromStringTable("SwatCommandTable", "ClosingDoor");
}
