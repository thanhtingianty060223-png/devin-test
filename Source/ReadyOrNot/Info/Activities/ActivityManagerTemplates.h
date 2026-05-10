// Copyright Void Interactive, 2023

#pragma once 

#include "Info/ActivityManager.h"
#include "Characters/CyberneticController.h" // Clang is picky about this one
#include "Team/TeamBaseActivity.h"

template <typename T>
bool UActivityManager::AnyAIHasActivity(const TFunctionRef<bool(const T*)> Predicate)
{
	static_assert(TIsDerivedFrom<T, UBaseActivity>::Value, "T must be derived from UBaseActivity");

	if (const AReadyOrNotGameState* GS = UBpGameplayHelperLib::GetWorldStatic()->GetGameState<AReadyOrNotGameState>())
	{
		for (const ACyberneticCharacter* AI : GS->AllAICharacters)
		{
			if (ACyberneticController* Controller = AI->GetCyberneticsController())
			{
				if (Controller->GetCurrentActivity<T>() && Predicate(Controller->GetCurrentActivity<T>()))
				{
					return true;
				}
			}
		}
	}

	return false;
}

template <typename T>
void UActivityManager::IterateAllActivitiesOfType(TFunctionRef<bool(T*)> Predicate)
{
	static_assert(TIsDerivedFrom<T, UBaseActivity>::Value, "T must be derived from UBaseActivity");

	if (UBpGameplayHelperLib::GetWorldStatic())
	{
		for (UBaseActivity* Activity : Get(UBpGameplayHelperLib::GetWorldStatic())->AllActivities)
		{
			if (IsValid(Activity) && Activity->GetOwningController() && Activity->GetOwningController()->GetCharacter())
			{
				if (T* TBA = Cast<T>(Activity))
				{
					if (!Predicate(TBA))
					{
						break;
					}
				}
			}
		}
	}
}

template <typename T>
T* UTeamBaseActivity::GetSharedData() const
{
	static_assert(TIsDerivedFrom<T, FSharedTeamData>::Value, "T must be derived from FSharedTeamData");
	return (T*)SharedData;
}
