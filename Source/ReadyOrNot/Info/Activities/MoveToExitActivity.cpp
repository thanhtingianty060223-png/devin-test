// Copyright Void Interactive, 2023

#include "Info/Activities/MoveToExitActivity.h"

#include "Info/SWATManager.h"
#include "Navigation/ReadyOrNotNavQueries.h"

void UMoveToExitActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	OwningController->bStopUtilityTick = false;
}

void UMoveToExitActivity::PerformActivity(const float DeltaTime)
{
	if (GetCharacter()->IsExitingSurrender())
		return;
	
	if (const USWATManager* Manager = USWATManager::Get(this))
	{
		SetLocation(Manager->OriginalSpawnLocation);
	}
	else
	{
		if (LOCAL_PLAYER)
		{
			SetLocation(LocalPlayer->OriginalSpawnLocation);
		}
	}

	if (Location == FVector::ZeroVector)
	{
		ACTIVITY_FAILED("No spawn location found");
		return;
	}

	if (HasReachedLocation(200.0f))
	{
		AbortMove(true);
		if (LOCAL_PLAYER)
		{
			GetCharacter()->ArrestComplete(LocalPlayer, Cast<AZipcuffs>(LocalPlayer->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Zipcuffs)));
			LocalPlayer->Server_ReportToTOC_Implementation(GetCharacter(), false);
			OwningController->FinishActivity(this, true, true);
			return;
		}
	}

	OwningController->bStopDecisionMaking = true;
	OwningController->bStopUtilityTick = true;
	
	GetCharacter()->ReasonsToSprint.AddUnique("running to exit");
	
	GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Afraid, 15.0f, 1.0f, 0.1f, 50);
	
	Super::PerformActivity(DeltaTime);
}

bool UMoveToExitActivity::CanBeCleared()
{
	return false;
}

bool UMoveToExitActivity::CanBeOverridenBy(UBaseActivity* InOverridingActivity)
{
	return false;
}

bool UMoveToExitActivity::CanOverrideActivity() const
{
	return false;
}

TSubclassOf<UNavigationQueryFilter> UMoveToExitActivity::GetNavigationQueryOverride()
{
	return UNavigationQueryFilter::StaticClass();
}

FName UMoveToExitActivity::GetMoveStyleOverride_Implementation() const
{
	if (GetCharacter()->IsCivilian())
	{
		return "male01_civilian_cowering";
	}

	return NAME_None;
}
