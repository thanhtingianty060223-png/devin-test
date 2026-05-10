// Copyright Void Interactive, 2018

#include "InteractionsData.h"
#include "Actors/PairedInteractionDriver.h"

#define DEBUG_INTERACTION_POSITIONS 0

UInteractionsData::UInteractionsData()
{
	InteractionName = "Default";

	bUpdateSlaveTransform = true;
	bEquipLastItemAfterPlaying = true;
}

APairedInteractionDriver* UInteractionsData::PlayPairedInteraction(AActor* Driver, AActor*  Slave, ABaseItem* OptionalItem)
{
	if (!Driver || !Slave)
	{
		// No driver or slave makes it impossible to play a paired animation
		return nullptr;
	}

	if (!Driver->HasAuthority())
	{
		V_LOG(LogReadyOrNot, "PlayPairedInteraction called on client. Don't do this!");
		return nullptr;
	}

	return APairedInteractionDriver::CreateAndPlayInteraction(Driver->GetWorld(), this, Driver, Slave, OptionalItem);
}

APairedInteractionDriver* UInteractionsData::IsPairedInteractionPlayingOn(AActor* Target)
{
	if (!IsValid(Target) || !Target->GetWorld())
		return nullptr;

	AReadyOrNotGameState* GameState = Target->GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return nullptr;
	
	for (APairedInteractionDriver* PairedInteractionDriver : GameState->AllPairedInteractionActors)
	{
		if (!IsValid(PairedInteractionDriver))
			continue;
		
		if (PairedInteractionDriver->bInteractionRunning)
		{
			if (PairedInteractionDriver->IsDriver(Target))
			{
				if (PairedInteractionDriver->IsDriverFinished())
					return nullptr;
				
				return PairedInteractionDriver;
			}
			
			if (PairedInteractionDriver->IsSlave(Target))
			{
				if (PairedInteractionDriver->IsSlaveFinished())
					return nullptr;
			
				return PairedInteractionDriver;
			}
		}
	}
	
	return nullptr;
}

APairedInteractionDriver* UInteractionsData::IsPairedInteractionPlayingOn(const AActor* Target)
{
	if (!Target)
		return nullptr;
	
	return IsPairedInteractionPlayingOn((AActor*)Target);
}

bool UInteractionsData::IsPairedInteractionPlayingOn(UInteractionsData* InteractionData, AActor* Target)
{
	if (!Target || !InteractionData)
		return false;
	
	AReadyOrNotGameState* GameState = Target->GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return false;

	for (APairedInteractionDriver* PairedInteractionDriver : GameState->AllPairedInteractionActors)
	{
		if (IsValid(PairedInteractionDriver))
		{
			if (PairedInteractionDriver->GetInteractionData() == InteractionData && PairedInteractionDriver->bInteractionRunning)
			{
				if (PairedInteractionDriver->IsDriver(Target) && !PairedInteractionDriver->IsDriverFinished())
					return true;
				
				if (PairedInteractionDriver->IsSlave(Target) && !PairedInteractionDriver->IsSlaveFinished())
					return true;
			}
		}
	}
	
	return false;
}

bool UInteractionsData::IsPairedInteractionPlayingOn(UInteractionsData* InteractionData, const AActor* Target)
{
	if (!Target)
	{
		return false;
	}
	
	return IsPairedInteractionPlayingOn(InteractionData, (AActor*)Target);
}

UInteractionsData* UInteractionsData::GetInteraction(const FName& InteractionName)
{
	if (const UDataTable* InteractionsTable = UBpGameplayHelperLib::GetPairedInteractionDataTable())
	{
		TArray<FPairedInteractionTable*> Rows;
		InteractionsTable->GetAllRows(__FUNCTION__, Rows);
		
		for (FPairedInteractionTable* Row : Rows)
		{
			if (UInteractionsData** Data = Row->Interactions.Find(InteractionName))
			{
				return *Data;
			}
		}
	}

	return nullptr;
}

TMap<FName, UInteractionsData*> UInteractionsData::GetInteractionCollection(const FName& CollectionName)
{
	if (const UDataTable* InteractionsTable = UBpGameplayHelperLib::GetPairedInteractionDataTable())
	{
		if (FPairedInteractionTable* Row = InteractionsTable->FindRow<FPairedInteractionTable>(CollectionName, __FUNCTION__))
		{
			return Row->Interactions;
		}
	}

	return TMap<FName, UInteractionsData*>();
}
