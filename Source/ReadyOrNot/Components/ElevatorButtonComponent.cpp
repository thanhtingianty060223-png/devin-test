#include "ElevatorButtonComponent.h"
#include "ReadyOrNot.h"
#include "Actors/Gameplay/Elevator.h"

bool UElevatorButtonComponent::StartUse_Implementation(class APlayerCharacter* User)
{
	if (OwningElevator == nullptr)
	{
		return false; // doesn't function if we don't have an elevator attached to us
	}

	if (bDoorButton)
	{
		OwningElevator->Server_OpenCloseDoors(bDoorClose);
	}
	else
	{
		OwningElevator->Server_SetSelectedFloor(Floor);
	}

	return true; // always functions when we press a button
}

void UElevatorButtonComponent::EndUse_Implementation(class APlayerCharacter* User)
{
	return; // doesn't do anything
}

TArray<USceneComponent*> UElevatorButtonComponent::GetUseViewComponents_Implementation()
{
	TArray<USceneComponent*> ViewComponents;
	ViewComponents.Add(this);
	return ViewComponents;
}
