// Copyright Void Interactive, 2023

#include "ReadyOrNotAISystem.h"

#include "NavigationSystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Info/SuspectsAndCivilianManager.h"
#include "Info/SWATManager.h"
#include "Info/Activities/BaseCombatActivity.h"
#include "Navigation/NavLocalGridManager.h"

void UReadyOrNotAISystem::PostInitProperties()
{
	Super::Super::PostInitProperties(); // skip the base ai system class, we wanna not create some manager objects as we're not using them (to save on performance)

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* WorldOuter = GetOuterWorld();

		BehaviorTreeManager = nullptr;
		EnvironmentQueryManager = NewObject<UEnvQueryManager>(this);
		NavLocalGrids = NewObject<UNavLocalGridManager>(this);
		HotSpotManager = nullptr;

		TSubclassOf<UAIPerceptionSystem> PerceptionSystemClass = PerceptionSystemClassName.IsValid() ? LoadClass<UAIPerceptionSystem>(NULL, *PerceptionSystemClassName.ToString(), NULL, LOAD_None, NULL) : nullptr;
		if (PerceptionSystemClass)
		{
			PerceptionSystem = NewObject<UAIPerceptionSystem>(this, PerceptionSystemClass, TEXT("PerceptionSystem"));
		}

		if (WorldOuter)
		{
			FOnActorSpawned::FDelegate ActorSpawnedDelegate = FOnActorSpawned::FDelegate::CreateUObject(this, &UReadyOrNotAISystem::OnActorSpawned);
			ActorSpawnedDelegateHandle = WorldOuter->AddOnActorSpawnedHandler(ActorSpawnedDelegate);
		}

		ConditionalLoadDebuggerPlugin();
	}
}

bool UReadyOrNotAISystem::ProjectPointToNav(FVector Point, FVector& OutLocation, FVector Extent)
{
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(UBpGameplayHelperLib::GetWorldStatic()))
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(Point, NavLocation, Extent))
        {
            OutLocation = NavLocation.Location;
            return true;
        }

        return false;
    }

    return false;
}

bool UReadyOrNotAISystem::FindPath(FVector From, FVector To, float* OutLength, FNavigationPath* OutPath)
{
    if (From == FVector::ZeroVector || To == FVector::ZeroVector)
        return false;

	ProjectPointToNav(From, From);
	ProjectPointToNav(To, To);
    
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(UBpGameplayHelperLib::GetWorldStatic()))
    {
        FPathFindingQuery PathFindingQuery;
        PathFindingQuery.StartLocation = From;
        PathFindingQuery.EndLocation = To;
        PathFindingQuery.SetAllowPartialPaths(false);
        const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
        const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
        PathFindingQuery.QueryFilter = QueryFilter;

        const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
        if (PathFindingResult.IsSuccessful() && !PathFindingResult.IsPartial())
        {
	        if (OutLength)
	        {
		        *OutLength = PathFindingResult.Path->GetLength();
	        }

        	if (OutPath)
        	{
        		*OutPath = *PathFindingResult.Path.Get();
        	}
        	
            return true;
        }
    }

    return false;
}

bool UReadyOrNotAISystem::WasRecentlyInCombat(float SinceSeconds, bool bCivilianCheck)
{
	const TArray<ASWATCharacter*>& SwatAI = USWATManager::Get(UBpGameplayHelperLib::GetWorldStatic())->SwatAI;
	const TArray<ACyberneticCharacter*>& Suspects = USuspectsAndCivilianManager::Get(UBpGameplayHelperLib::GetWorldStatic())->Suspects;
	
	for (const ASWATCharacter* Swat : SwatAI)
	{
		if (IsValid(Swat))
		{
			if (Swat->IsActive() && Swat->bHasEverShot)
			{
				const float TimeSinceFiredGun = Swat->TimeSinceLastShot;
				
				if (TimeSinceFiredGun != 0.0f && TimeSinceFiredGun < SinceSeconds)
				{
					return true;
				}
			}
		}
	}

	// Were we recently in combat?
	for (const ACyberneticCharacter* Character : Suspects)
	{
		if (IsValid(Character))
		{
			if (Character->IsActive() && Character->bHasEverShot)
			{
				const float TimeSinceFiredGun = Character->TimeSinceLastShot;
				
				if (TimeSinceFiredGun < SinceSeconds)
				{
					return true;
				}
			}
		}
	}

	if (bCivilianCheck)
	{
		const TArray<ACyberneticCharacter*>& Civilians = USuspectsAndCivilianManager::Get(UBpGameplayHelperLib::GetWorldStatic())->Civilians;
		
		for (const ACyberneticCharacter* Character : Civilians)
		{
			if (IsValid(Character))
			{
				if (Character->IsActive())
				{
					//const float TimeSinceHeardYell = Character->TimeSinceHeardOfficerYell;

					// sometimes players dont know who is and isnt a civilian so in order to prevent the swat ai
					// from killing the player (if player had believed they killed a suspect),
					// we check if they're performaing a combat manuever,
					// since fleeing, taking cover and hiding are all things suspects do as well
					// and we can't expect players to know who is and isnt a suspect for real
					// and also if civilians are performing combat moves, then we are basically in combat :)
					if ((Character->GetCyberneticsController()->GetCombatActivity() &&
						Character->GetCyberneticsController()->GetCombatActivity()->TimeSincePerformingAnyCombatMove < SinceSeconds))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}
