// Copyright Void Interactive, 2021

#include "TeamBaseActivity.h"

#include "ReadyOrNotAISystem.h"
#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"
#include "Info/Activities/ActivityManagerTemplates.h"

#include "Info/SWATManager.h"
#include "Misc/RuntimeErrors.h"

UTeamBaseActivity::UTeamBaseActivity()
{
	bIsProgressActivity = false;
	bAbortActivityIfCannotReachLocation = false;
	bFinishActivityWhenOverriden = false;
	bAbortIfNotMovingForAWhile = false;
}

bool UTeamBaseActivity::OverrideAvoidanceLocation() const
{
	return false;
}

FVector UTeamBaseActivity::GetBestAvoidanceLocation(ACyberneticCharacter* OverlappingAI) const
{
	return FVector::ZeroVector;
}

void UTeamBaseActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!USWATManager::Get(this))
	{
		ACTIVITY_FAILED("No swat manager was specified");
		return;
	}

	bIsSwapping = false;

	// If was given a "hold" command, force empty this list
	GetCharacter()->ReasonsToStandStill.Empty();

	#if WITH_EDITOR
	ensureAlways(SharedData != nullptr);
	#endif
}

bool UTeamBaseActivity::HasTeamReachedPosition(float Tolerance) const
{
	bool bHasAnyNotReachedLocation = false;

	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (FMath::IsNearlyZero(Tolerance, 0.0001f))
				Tolerance = Activity->GetDestinationTolerance();

			Tolerance += 2.0f; // extra error tolerance, just incase of instances where the distance to location is like 10.091547 or something (if the tolerance was 10 for example)
			
			if (!Activity->HasReachedLocation(Tolerance))
			{
				#if !UE_BUILD_SHIPPING
				if (CVarRonToggleActivityDebugLines.GetValueOnAnyThread() == 2)
				{
					DrawDebugBox(GetWorld(), Activity->GetLocation(), FVector(20.0f), FColor::Red, false, 1.0f, 0, 1);
				}
				#endif
				
				bHasAnyNotReachedLocation = true;
				return false;
			}
		}

		return true;
	});
	
	return !bHasAnyNotReachedLocation;
}

ACyberneticCharacter* UTeamBaseActivity::GetCharacterAtSquadPosition(const ESquadPosition SquadPosition) const
{
	ACyberneticCharacter* FoundCharacter = nullptr;
	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity->IsActivityComplete())
			return true;
		
		if (Activity->OverrideSquadPosition == SquadPosition && Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			FoundCharacter = Activity->GetCharacter();
			return false;
		}

		return true;
	});
	
	return FoundCharacter;
}

ACyberneticCharacter* UTeamBaseActivity::GetCharacterWithItem(TSubclassOf<ABaseItem> ItemClass) const
{
	ACyberneticCharacter* FoundCharacter = nullptr;
	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (Activity->OwningController && Activity->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ItemClass))
			{
				FoundCharacter = Activity->GetCharacter();
				return false;
			}
		}

		return true;
	});
	
	return FoundCharacter;
}

ACyberneticCharacter* UTeamBaseActivity::GetCharacterClosestToLocation(const FVector& TestLocation) const
{
	if (TestLocation == FVector::ZeroVector)
		return nullptr;
	
	float ClosestDist = FLT_MAX;
	ACyberneticCharacter* Closest = nullptr;
	
	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (Activity->GetCharacter())
			{
				const float TestDistance = FVector::DistSquared(Activity->GetCharacter()->GetActorLocation(), TestLocation);
				if (TestDistance < ClosestDist)
				{
					ClosestDist = TestDistance;
					Closest = Activity->GetCharacter();
				}
			}
		}

		return true;
	});
	
	return Closest;
}

ACyberneticCharacter* UTeamBaseActivity::GetCharacterClosestToCharacter(ACyberneticCharacter* InCharacter) const
{
	if (!InCharacter)
		return nullptr;
	
	float ClosestDist = FLT_MAX;
	ACyberneticCharacter* Closest = nullptr;
	
	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (Activity->GetCharacter() && InCharacter != Activity->GetCharacter())
			{
				const float TestDistance = (Activity->GetCharacter()->GetActorLocation() - InCharacter->GetActorLocation()).SizeSquared();
				if (TestDistance < ClosestDist)
				{
					ClosestDist = TestDistance;
					Closest = Activity->GetCharacter();
				}
			}
		}
		
		return true;
	});
	
	return Closest;
}

AReadyOrNotCharacter* UTeamBaseActivity::GetSquadLeader() const
{
	return USWATManager::Get(this)->GetSquadLeader();
}

bool UTeamBaseActivity::CanShoot() const
{
	return true;
}

void UTeamBaseActivity::SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated)
{
	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return;
	
	// can't swap with yourself :p
	if (SquadPosition == OverrideSquadPosition)
		return;

	UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
	{
		if (Activity != this && Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (Activity->OverrideSquadPosition == SquadPosition)
			{
				Swap(OverrideSquadPosition, Activity->OverrideSquadPosition);
				SetLocation(Activity->GetCharacter()->GetNavAgentLocation(), true);
				Activity->SetLocation(GetCharacter()->GetNavAgentLocation(), true);
				bIsSwapping = true;
				Activity->bIsSwapping = true;
				return false;
			}
		}
		
		return true;
	});
}

bool UTeamBaseActivity::CanSwapSquadPositions() const
{
	return true;
}

bool UTeamBaseActivity::ShouldForceNoStrafe() const
{
	if (bIsSwapping)
		return true;
	
	return false;
}

bool UTeamBaseActivity::IsAnyoneSwapping() const
{
    bool bAnyoneSwapping = false;
    UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](const UTeamBaseActivity* Activity)
    {
        if (Activity->SharedData->ActivityId == SharedData->ActivityId && Activity->bIsSwapping)
        {
            bAnyoneSwapping = true;
            return false;
        }

        return true;
    });

	return bAnyoneSwapping;
}

bool UTeamBaseActivity::CanFinishActivity() const
{
	return false;
}

void UTeamBaseActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (bIsSwapping)
	{
		FVector Blah = FVector::ZeroVector;
		const bool bSuccess = UReadyOrNotAISystem::ProjectPointToNav(Location, Blah, FVector(75.0f, 75.0f, 150.0f));
		
		if (Location == FVector::ZeroVector ||
			!bSuccess ||
			(Location != FVector::ZeroVector && HasReachedLocation(GetDestinationTolerance())))
		{
			bIsSwapping = false;
		}
	}
}

void UTeamBaseActivity::ResetData()
{
	Super::ResetData();
	
	bIsSwapping = false;
}

float UTeamBaseActivity::GetMoveAcceptanceRadiusOverride() const
{
	if (bIsSwapping)
		return 1.0f;

	return MoveAcceptanceRadius;
}
