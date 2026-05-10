// Copyright Void Interactive, 2023


#include "InteractWithDoorObjective.h"


// Sets default values
AInteractWithDoorObjective::AInteractWithDoorObjective()
{
	bAllowCompletionWhileHidden = false;
}

void AInteractWithDoorObjective::TickObjective()
{
	if (!HasAuthority())
		return;

	if (bHiddenObjective && !bAllowCompletionWhileHidden)
		return;

	// Remove any invalid doors from the list
	// Should we instead fail the objective if a door no longer exists? Not sure when/how this could happen, just some thoughts for later
	Doors.RemoveAll([](const ADoor* Element) { return !IsValid(Element); });

	if (!Doors.Num())
		return;

	switch (DoorInteractionType)
	{
	case ODI_Open:
		for (ADoor* Door : Doors)
		{
			if (!Door->IsOpen())
				return;
		}
		break;

	case ODI_Close:
		for (ADoor* Door : Doors)
		{
			if (!Door->IsClosed())
				return;
		}
		break;

	default: return;
	}

	ObjectiveCompleted();

	for (ADoor* Door : Doors)
	{
		Door->Multicast_DisableDoorInteraction(DoorInteractionType == ODI_Close);
	}	
}

void AInteractWithDoorObjective::BeginPlay()
{
	Super::BeginPlay();

	// Only setup the bindings here if we're visible or allowed to complete while hidden, else set them up on unlock
	if (!bHiddenObjective || bAllowCompletionWhileHidden)
		SetupDoorBindings();
		
	
	ObjectiveName = GetFormattedName();
	ObjectiveDescription = GetFormattedDescription();
}

void AInteractWithDoorObjective::SetupDoorBindings()
{
	for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;

		if (Door->Tags.Contains(DoorTag))
		{
			Doors.Emplace(Door);
			
			if (DoorInteractionType == ODI_Kick)
			{
				Door->OnDoorKicked.RemoveDynamic(this, &AInteractWithDoorObjective::OnDoorKicked);
				Door->OnDoorKicked.AddDynamic(this, &AInteractWithDoorObjective::OnDoorKicked);
			}
		}
	}
}

void AInteractWithDoorObjective::OnDoorKicked(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter, bool bSuccess)
{
	// Double check we're wanting door kick
	if (DoorInteractionType == ODI_Kick && bSuccess)
		ObjectiveCompleted();
}

void AInteractWithDoorObjective::Multicast_UnlockObjective_Implementation()
{
	Super::Multicast_UnlockObjective_Implementation();

	// Only setup the bindings on the server
	if (HasAuthority())
		SetupDoorBindings();
}
