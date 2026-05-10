// Copyright Void Interactive, 2021

#include "SWATManager.h"

#include "ReadyOrNotGameState.h"

#include "Actors/Door.h"
#include "Actors/Items/Multitool.h"
#include "Actors/BaseGrenade.h"
#include "Actors/BaseItem.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Items/Optiwand.h"
#include "Actors/Items/Chemlight.h"
#include "Actors/Items/C2Explosive.h"
#include "Actors/Items/DoorRam.h"
#include "Actors/Items/GrenadeLauncher.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"

#include "Characters/AI/SWATCharacter.h"
#include "Characters/PlayerCharacter.h"
#include "Characters/CyberneticController.h"

#include "Activities/ActivityManagerTemplates.h"
#include "Activities/ToggleDoorActivity.h"
#include "Activities/PickupItemActivity.h"
#include "Activities/Team/ArrestTargetActivity.h"
#include "Activities/Team/DeployWedgeActivity.h"
#include "Activities/Team/DisarmDoorTrapActivity.h"
#include "Activities/Team/TeamStackUpActivity.h"
#include "Activities/Team/MirrorUnderDoorActivity.h"
#include "Activities/Team/TeamBreachAndClearActivity.h"
#include "Activities/Team/DeployItemAtLocationActivity.h"
#include "Activities/Team/TeamFallinActivity.h"
#include "Activities/Team/HoldActivity.h"
#include "Activities/Team/CollectEvidenceActivity.h"

#include "NavigationSystem.h"
#include "NavLinkCustomComponent.h"
#include "ReadyOrNotAISystem.h"
#include "ReadyOrNotSignificanceManager.h"
#include "ScoringManager.h"
#include "Activities/DeployChemlightActivity.h"
#include "Activities/DeployGrenadeAtLocationActivity.h"
#include "Activities/EngageTargetLessLethalActivity.h"
#include "Activities/MoveActivity.h"
#include "Activities/MoveToActivity.h"
#include "Activities/TrailerSearchAndSecureActivity.h"
#include "Activities/Team/DisarmStandaloneTrapActivity.h"
#include "Activities/Team/LockPickDoorActivity.h"
#include "Activities/Team/ReportTargetActivity.h"
#include "Activities/Team/SearchAndSecureActivity.h"
#include "Activities/Team/TeamCoverAreaActivity.h"
#include "Actors/StackUpActor.h"
#include "Actors/Gameplay/CollectedEvidenceActor.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/BreachingShotgun.h"
#include "Actors/Items/DoorJam.h"
#include "Characters/AI/SWATController.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "GameModes/TrainingGM.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "lib/ReadyOrNotMathLibrary.h"
#include "Navigation/ReadyOrNotNavQueries.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"
#include "Subsystems/AchievementSubsystem.h"

#include "Structs.h"
#include "GameModes/CoopGM.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ SWAT Manager Tick"), STAT_SWATManagerTick, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Fall In Sort"), STAT_FallInSort, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Speech Cooldown"), STAT_SpeechCooldown, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Find Evidence to Order Request"), STAT_EvidenceOrderRequest, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Find Traps to Order Request"), STAT_DisarmTrapOrderRequest, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Remove Unavailable SWAT"), STAT_RemoveUnavailableSwat, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Remove All Invalid Request Orders"), STAT_RemoveAllInvalidRequestOrders, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Remove Out of Range Request Orders"), STAT_RemoveOutOfRangeRequestOrders, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Trailer Update"), STAT_SwatTrailerUpdate, STATGROUP_SWATManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Automatic Room Clear Detection"), STAT_RoomDetection, STATGROUP_SWATManager);

TAutoConsoleVariable<int32> CVarSwatKillTeamKiller(TEXT("SWAT.KillTeamKiller"), 1, TEXT("Can swat AI kill the team killer? 0 = No, 1 = Yes"));
TAutoConsoleVariable<int32> CVarSwatAutomate(TEXT("SWAT.Automate"), 0, TEXT("Automate swat ai? 0 = No, 1 = Yes"));

USWATManager::USWATManager()
{
	TickInterval = 0.15f;
	GlobalYellDelay = 0.5f;
}

USWATManager* USWATManager::Get(const UObject* WorldContext)
{
	return WorldContext->GetWorld()->GetSubsystem<USWATManager>();
}

void USWATManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SetupSwatManager();
}

void USWATManager::Deinitialize()
{
	Super::Deinitialize();
	
	GetWorld()->OnWorldBeginPlay.RemoveAll(this);
}

TStatId USWATManager::GetStatId() const
{
	return GetStatID();
}

void USWATManager::SetupSwatManager()
{
	LOCAL_PLAYER;
	SquadLeader = LocalPlayer;
	
	WaitingReplyDelay = MaxWaitingReplyDelay;
	
	CreationTime = GetWorld()->GetTimeSeconds();

	GetWorld()->OnWorldBeginPlay.AddUObject(this, &USWATManager::OnWorldBeingPlay);
	
	//bWaitingOnPathTest = true;
	
	FlashbangClass = StaticLoadClass(ABaseGrenade::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprints/Items/WeaponsRevised/Grenade_Flashbang_V2.Grenade_Flashbang_V2_C'"));
	StingerClass = StaticLoadClass(ABaseGrenade::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprints/Items/WeaponsRevised/Grenade_Stinger_V2.Grenade_Stinger_V2_C'"));
	CSGasClass = StaticLoadClass(ABaseGrenade::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprints/Items/WeaponsRevised/Grenade_CSGas_V2.Grenade_CSGas_V2_C'"));
	
	SharedBreachData =
	{
		{ETeamType::TT_SQUAD, {}},
		{ETeamType::TT_SERT_RED, {}},
		{ETeamType::TT_SERT_BLUE, {}}
	};
	
	SharedStackUpData =
	{
		{ETeamType::TT_SQUAD, {}},
		{ETeamType::TT_SERT_RED, {}},
		{ETeamType::TT_SERT_BLUE, {}}
	};
	
	SharedFallInData =
	{
		{ETeamType::TT_SQUAD, {}},
		{ETeamType::TT_SERT_RED, {}},
		{ETeamType::TT_SERT_BLUE, {}}
	};
	
	SharedTeamData =
	{
		{ETeamType::TT_SQUAD, {}},
		{ETeamType::TT_SERT_RED, {}},
		{ETeamType::TT_SERT_BLUE, {}}
	};
	
	CurrentSharedTeamData =
	{
		{ETeamType::TT_SQUAD, nullptr},
		{ETeamType::TT_SERT_RED, nullptr},
		{ETeamType::TT_SERT_BLUE, nullptr}
	};
}

void USWATManager::OnWorldBeingPlay()
{
	UActivityManager::Get(this)->OnStartActivity.AddDynamic(this, &USWATManager::OnActivityStarted);
	// TODO: on resume activity

	if (SquadLeader)
	{
		SquadLeader->OnNightVisionGogglesToggled.AddDynamic(this, &USWATManager::OnLeaderToggledNightvision);
	}
}

TArray<AActor*> USWATManager::GetAvailableSecurables(bool bNoSecureCheck) const
{
	TArray<AActor*> SortedSecurables;
	SortedSecurables.Reserve(200);
	
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (!IsValid(AI))
			continue;
			
		if (ISecurable::Execute_IsSecured(AI))
			continue;
		
		if (!Cast<ATrailerSWATCharacter>(AI) && !AI->IsOnSWATTeam())
		{
			if (bNoSecureCheck || ISecurable::Execute_CanBeSecured(AI))
			{
				// quick test to see if they're on a nav mesh
				FVector Blah;
				bool bCanProject = UReadyOrNotAISystem::ProjectPointToNav(ISecurable::Execute_GetLocation(AI), Blah, FVector(40.0f, 40.0f, 150.0f));

				if (bCanProject)
				{
					SortedSecurables.AddUnique(AI);
				}
			}
		}
	}
	
	for (ABaseItem* Item : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems)
	{
		if (!IsValid(Item))
			continue;
		
		if (ISecurable::Execute_IsSecured(Item))
			continue;
		
		if (Item->IsEvidence())
		{
			if (bNoSecureCheck || ISecurable::Execute_CanBeSecured(Item))
			{
				// quick test to see if they're on a nav mesh
				FVector Blah;
				bool bCanProject = UReadyOrNotAISystem::ProjectPointToNav(ISecurable::Execute_GetLocation(Item), Blah, FVector(40.0f, 40.0f, 150.0f));
				//DrawDebugBox(GetWorld(), ISecurable::Execute_GetLocation(Item), FVector(15.0f), FColor::Cyan, false, 1.0f);

				if (bCanProject)
				{
					SortedSecurables.AddUnique(Item);
				}
			}
		}
	}
	
	for (AEvidenceActor* EvidenceActor : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllEvidenceActors)
	{
		if (!IsValid(EvidenceActor))
			continue;
		
		if (ISecurable::Execute_IsSecured(EvidenceActor))
			continue;
		
		if (bNoSecureCheck || ISecurable::Execute_CanBeSecured(EvidenceActor))
		{
			// quick test to see if they're on a nav mesh
			FVector Blah;
			bool bCanProject = UReadyOrNotAISystem::ProjectPointToNav(ISecurable::Execute_GetLocation(EvidenceActor), Blah, FVector(40.0f, 40.0f, 150.0f));

			if (bCanProject)
			{
				SortedSecurables.AddUnique(EvidenceActor);
			}
		}
	}

	// only allow this when the ai is a squad leader
	if (Cast<ASWATCharacter>(SquadLeader))
	{
		for (AReportableActor* ReportableActor : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReportableActors)
		{
			if (!IsValid(ReportableActor))
				continue;
			
			if (ISecurable::Execute_IsSecured(ReportableActor))
				continue;
			
			if (bNoSecureCheck || ISecurable::Execute_CanBeSecured(ReportableActor))
			{
				// quick test to see if they're on a nav mesh
				FVector Blah;
				bool bCanProject = UReadyOrNotAISystem::ProjectPointToNav(ISecurable::Execute_GetLocation(ReportableActor), Blah, FVector(40.0f, 40.0f, 150.0f));

				if (bCanProject)
				{
					SortedSecurables.AddUnique(ReportableActor);
				}
			}
		}
	}

	return SortedSecurables;
}

void USWATManager::OnActivityStarted(UBaseActivity* Activity, ACyberneticController* OwningController)
{
	if (SwatAI.Contains(OwningController->GetCharacter()))
	{
		if (!Cast<UMoveToActivity>(Activity))
			bGivenInitialFallInCommand = true;
		
		WaitingReplyDelay = MaxWaitingReplyDelay;

		// Clear custom focus location if a new team or door activity was given
		if (Cast<UDoorInteractionActivity>(Activity) || Cast<UScanDoorActivity>(Activity))
		{
			OwningController->GetTargetingComp()->CustomFocusActor = nullptr;
			OwningController->GetTargetingComp()->CustomFocusLocation = FVector::ZeroVector;
		}
	}
}

void USWATManager::OnLeaderToggledNightvision(AReadyOrNotCharacter* Character, bool bOn)
{
	for (ASWATCharacter* Swat : SwatAI)
	{
		if (Swat && Swat->IsActive())
		{
			Swat->ToggleNightvisionGoggles();
			if (bOn)
				Swat->EnableNightVisionGoggles();
			else
				Swat->DisableNightVisionGoggles();
		}
	}
}

bool USWATManager::Internal_FindPath(float& OutDistance, FVector From, FVector To, float MaxPathLength)
{
    if (From == FVector::ZeroVector || To == FVector::ZeroVector)
        return false;
    
    // if the distance is so short, we prob don't need to path anyway
	float Distance = FVector::Distance(From, To);
    if (Distance < 150.0f)
    {
		OutDistance = Distance;
		return true;
    }
	
	UReadyOrNotAISystem::ProjectPointToNav(From, From);
	UReadyOrNotAISystem::ProjectPointToNav(To, To);
    
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FPathFindingQuery PathFindingQuery;
        PathFindingQuery.StartLocation = From;
        PathFindingQuery.EndLocation = To;
        PathFindingQuery.SetAllowPartialPaths(false);
        const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
        const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
        PathFindingQuery.QueryFilter = QueryFilter;
        
        FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
        if (PathFindingResult.IsSuccessful() && PathFindingResult.Path.IsValid() && PathFindingResult.Path->IsValid())
        {
            if (PathFindingResult.Path->GetLength() > MaxPathLength)
                return false;

			OutDistance = PathFindingResult.Path->GetLength();
            return true;
        }
    }

    return false;
}

void USWATManager::OnSwatFinishedClearing(UTeamBreachAndClearActivity* BreachAndClearActivity, bool bAuto)
{
	// Don't auto clear if in training mode
	if (GetWorld()->GetAuthGameMode<ATrainingGM>())
		return;

	bool bAnyBreachAndClearActive = false;
	UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
	{
		if (Activity != BreachAndClearActivity &&
			Activity->SharedData->ActivityId == BreachAndClearActivity->SharedData->ActivityId && !Activity->IsActivityComplete())
		{
			bAnyBreachAndClearActive = true;
			return false;
		}

		return true;
	});

	if (!bAnyBreachAndClearActive)
	{
		if (!bAuto)
		{
			if (BreachAndClearActivity->SharedData->CommandTeam == ETeamType::TT_SQUAD)
				bGoldAutoClearing = true;
			else if (BreachAndClearActivity->SharedData->CommandTeam == ETeamType::TT_SERT_RED)
				bRedAutoClearing = true;
			else if (BreachAndClearActivity->SharedData->CommandTeam == ETeamType::TT_SERT_BLUE)
				bBlueAutoClearing = true;
		}

		if (const FRoom* Room = BreachAndClearActivity->GetSharedData<FSharedBreachData>()->Room)
		{
			for (ADoor* Door : Room->AdditionalRootDoors)
			{
				if (Door)
					Door->DeactivateDoorBlocker();
			}
			
			for (ASWATCharacter* swat : SwatAI)
			{
				if (CanGiveActivityToSWAT(swat, BreachAndClearActivity->SharedData->CommandTeam))
				{
					if (USearchAndSecureActivity* SearchAndSecureActivity = swat->GetCyberneticsController<ASWATController>()->GetSearchAndSecureActivity())
					{
						ETeamType PreviousTeam = BreachAndClearActivity->SharedData->CommandTeam;
						if (bAuto)
						{
							PreviousTeam = SearchAndSecureActivity->CommandTeam;
							SearchAndSecureActivity->ResetData();
						}
						
						SearchAndSecureActivity->SearchingRoom = Room;
						SearchAndSecureActivity->BreachDoor = BreachAndClearActivity->GetStackUpDoor();
						SearchAndSecureActivity->bBreachedFromFront = BreachAndClearActivity->GetSharedData<FSharedBreachData>()->StackUpDoor->IsPointInFrontOfDoorway(BreachAndClearActivity->SharedData->CommandLocation);
						SearchAndSecureActivity->CommandLocation = BreachAndClearActivity->SharedData->CommandLocation;
						SearchAndSecureActivity->bReturnToOriginalLocation = true;
						SearchAndSecureActivity->bNoResetDataOnFinish = true;
						SearchAndSecureActivity->CommandTeam = PreviousTeam;
						SearchAndSecureActivity->bAuto = true;
						
						if (UActivityManager::GiveActivityTo(SearchAndSecureActivity, swat, true, false))
						{
							SearchAndSecureActivity->OnSearchComplete.RemoveAll(this);
							SearchAndSecureActivity->OnSearchComplete.AddDynamic(this, &USWATManager::OnSwatFinishedRoomSearch);
						}
					}
				}
			}
		}
	}
}

void USWATManager::OnSwatFinishedRoomSearch(USearchAndSecureActivity* SearchAndSecureActivity, ADoor* BreachedDoor)
{
	if (!SearchAndSecureActivity->SearchingRoom)
		return;
	
	bool bAnySearchAndSecureActive = false;
	UActivityManager::IterateAllActivitiesOfType<USearchAndSecureActivity>([&](USearchAndSecureActivity* Activity)
	{
		if (Activity != SearchAndSecureActivity &&
			Activity->SearchingRoom == SearchAndSecureActivity->SearchingRoom && !Activity->IsActivityComplete())
		{
			bAnySearchAndSecureActive = true;
			return false;
		}

		return true;
	});

	if (!bAnySearchAndSecureActive)
	{
		ETeamType CommandTeam = SearchAndSecureActivity->CommandTeam;

		if (bRedAutoClearing && bBlueAutoClearing)
		{
			ClearingQueue.Remove(ETeamType::TT_SQUAD);

			if (CommandTeam == ETeamType::TT_SQUAD)
				bGoldAutoClearing = false;
		}
		
		if (!ClearingQueue.Find(CommandTeam))
		{
			int32 NumDoorways = 0;
			
			for (const ADoor* Door : SearchAndSecureActivity->SearchingRoom->AdditionalRootDoors)
			{
				if (Door && Door != BreachedDoor && Door != BreachedDoor->GetSubDoor())
				{
					if (Door->IsDoorwayOnly() && !Door->bNoAutomaticClearing)
					{
						NumDoorways++;
					}
				}
			}
			
			// Search connecting rooms (doorways only, open doors don't count)
			if (NumDoorways > 0)
			{
				const FVector CommandLocation = FVector(SearchAndSecureActivity->SearchingRoom->Location);

				// first clear all dead end doorways before trying to rabbit hole into deeply connected doorways
				
				TMap<ADoor*, AThreatAwarenessActor*> ConnectedDoorways, DeadEndDoorways;
				for (ADoor* Door : SearchAndSecureActivity->SearchingRoom->AdditionalRootDoors)
				{
					if (!Door)
						continue;
					
					if (Door->bNoAutomaticClearing)
						continue;
					
					if (Door->IsDoorwayOnly())
					{
						Door->DeactivateDoorBlocker();
						
						FName BreachingRoom = NAME_None;
						if (Door->GetFrontThreatOwningRoom() != SearchAndSecureActivity->SearchingRoom->Name)
						{
							BreachingRoom = Door->GetFrontThreatOwningRoom();
						}
						else if (Door->GetBackThreatOwningRoom() != SearchAndSecureActivity->SearchingRoom->Name)
						{
							BreachingRoom = Door->GetBackThreatOwningRoom();
						}
						
						AThreatAwarenessActor* TAA = nullptr;
						if (Door->GetFrontThreatOwningRoom() == SearchAndSecureActivity->SearchingRoom->Name)
						{
							TAA = Door->FrontThreat;
						}
						else if (Door->GetBackThreatOwningRoom() == SearchAndSecureActivity->SearchingRoom->Name)
						{
							TAA = Door->BackThreat;
						}

						// Does the breaching room have 1 or more connected rooms
						if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
						{
							if (const FRoom* TheRoomByTommyWiseau = GS->RoomData->Rooms.FindByPredicate([&](const FRoom& Room)
							{
								return Room.Name == BreachingRoom;
							}))
							{
								if (TheRoomByTommyWiseau->bClearedBySwat)
									continue;

								TArray<FName> ConnectingRooms = TheRoomByTommyWiseau->ConnectingRooms;
								ConnectingRooms.Remove(SearchAndSecureActivity->SearchingRoom->Name);
								
								if (TheRoomByTommyWiseau->ConnectingRooms.Num() < 1)
								{
									DeadEndDoorways.Add(Door, TAA);
								}
								else
								{
									ConnectedDoorways.Add(Door, TAA);
								}
							}
						}
					}
				}

				// sort all doorways from the breach point
				ConnectedDoorways.KeySort([&](const ADoor& Lhs, const ADoor& Rhs)
				{
					return (Lhs.GetDoorMidLocation() - CommandLocation).SizeSquared() < (Rhs.GetDoorMidLocation() - CommandLocation).SizeSquared();
				});
				
				DeadEndDoorways.KeySort([&](const ADoor& Lhs, const ADoor& Rhs)
				{
					return (Lhs.GetDoorMidLocation() - CommandLocation).SizeSquared() < (Rhs.GetDoorMidLocation() - CommandLocation).SizeSquared();
				});

				if (ConnectedDoorways.Num() + DeadEndDoorways.Num() > 0)
				{
					FClearQueueInfo QueueInfo;
					QueueInfo.Data = DeadEndDoorways;
					QueueInfo.Data.Append(ConnectedDoorways);
					QueueInfo.NumDoorways = NumDoorways;
					QueueInfo.TotalDoors = SearchAndSecureActivity->SearchingRoom->AdditionalRootDoors.Num();
					QueueInfo.ClearingRoom = SearchAndSecureActivity->SearchingRoom;
					
					ClearingQueue.Add(CommandTeam, QueueInfo);
				}
			}
		}
		
		if (FClearQueueInfo* QueueInfo = ClearingQueue.Find(CommandTeam))
		{
			uint8 NumCommandsGiven = 0;
			uint8 NumDoorways = QueueInfo->NumDoorways;
			TArray<ETeamType, TFixedAllocator<2>> AvailableTeams;

			bool bIsRedClearing = false;
			bool bIsBlueClearing = false;
			
			if (CommandTeam == ETeamType::TT_SQUAD)
			{
				UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
				{
					if (!Activity->IsActivityComplete() && Activity->GetSharedData<FSharedBreachData>()->CommandTeam == ETeamType::TT_SERT_RED)
					{
						bIsRedClearing = true;
					}
					
					if (!Activity->IsActivityComplete() && Activity->GetSharedData<FSharedBreachData>()->CommandTeam == ETeamType::TT_SERT_BLUE)
					{
						bIsBlueClearing = true;
					}

					return true;
				});
				
				UActivityManager::IterateAllActivitiesOfType<USearchAndSecureActivity>([&](USearchAndSecureActivity* Activity)
				{
					if (!Activity->IsActivityComplete() && Activity->CommandTeam == ETeamType::TT_SERT_RED)
					{
						bIsRedClearing = true;
					}
					
					if (!Activity->IsActivityComplete() && Activity->CommandTeam == ETeamType::TT_SERT_BLUE)
					{
						bIsBlueClearing = true;
					}

					return true;
				});

				// does blue have a clearing queue?
				if (bBlueAutoClearing)
				{
					if (!bIsRedClearing)
						AvailableTeams.AddUnique(ETeamType::TT_SERT_RED);
				}

				// does red have a clearing queue?
				if (bRedAutoClearing)
				{
					if (!bIsBlueClearing)
						AvailableTeams.AddUnique(ETeamType::TT_SERT_BLUE);
				}

				if (!bRedAutoClearing && !bBlueAutoClearing)
				{
					if (!bIsRedClearing)
						AvailableTeams.AddUnique(ETeamType::TT_SERT_RED);
					
					if (!bIsBlueClearing)
						AvailableTeams.AddUnique(ETeamType::TT_SERT_BLUE);
				}
			}
			else
			{
				AvailableTeams.Add(CommandTeam);
			}
			
			if (AvailableTeams.Num() == 0)
				return;
			
			TArray<FName> GivenBreachingRooms;
			for (TMap<ADoor*, AThreatAwarenessActor*>::TIterator It = ClearingQueue[CommandTeam].Data.CreateIterator(); It; ++It)
			{
				ADoor* Door = It.Key();
				AThreatAwarenessActor* TAA = It.Value();
				
				if (NumCommandsGiven == 2)
					break;

				if (AvailableTeams.Num() == 0)
					break;
				
				FName BreachingRoom = NAME_None;
				if (Door->GetFrontThreatOwningRoom() == TAA->OwningRoom)
				{
					BreachingRoom = Door->GetBackThreatOwningRoom();
				}
				else if (Door->GetBackThreatOwningRoom() == TAA->OwningRoom)
				{
					BreachingRoom = Door->GetFrontThreatOwningRoom();
				}

				if (GivenBreachingRooms.Contains(BreachingRoom))
				{
					It.RemoveCurrent();
					continue;
				}
				
				if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
				{
					if (const FRoom* OtherRoom = GS->RoomData->Rooms.FindByPredicate([&](const FRoom& Room)
					{
						return Room.Name == BreachingRoom;
					}))
					{
						if (OtherRoom->bClearedBySwat)
						{
							It.RemoveCurrent();
							continue;
						}
						
						// is someone other team already breaching this room?
						bool bSomeoneBreaching = false;
						UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
						{
							if (!Activity->IsActivityComplete() && Activity->GetSharedData<FSharedBreachData>()->Room == OtherRoom)
							{
								if (OtherRoom->AdditionalRootDoors.Contains(Activity->GetSharedData<FSharedBreachData>()->StackUpDoor))
								{
									bSomeoneBreaching = true;
									return false;
								}
							}
							
							return true;
						});

						if (bSomeoneBreaching)
						{
							It.RemoveCurrent();
							continue;
						}
					}
				}
				
				// find the closest team to the door
				ETeamType ClosestTeam = CommandTeam;
				if (ClosestTeam == ETeamType::TT_NONE || CommandTeam == ETeamType::TT_SQUAD)
				{
					if (NumDoorways <= 2 && !bIsRedClearing && !bIsBlueClearing)
					{
						ClosestTeam = ETeamType::TT_SQUAD;
						AvailableTeams.Empty();
					}
					else
					{
						if (AvailableTeams.Num() == 1)
						{
							ClosestTeam = AvailableTeams[0];
						}
						else
						{
							if (const ASWATCharacter* Swat = GetClosestSWATToActor(Door))
							{
								ClosestTeam = Swat->GetTeam();
							}
						}
					}
				}

				if (ClosestTeam != ETeamType::TT_NONE)
				{
					AvailableTeams.Remove(ClosestTeam);

					Door->ActivateDoorBlocker();

					// all swat in team must be in the same room
					bool bAllInRoom = true;
					FName StackingRoom = Door->IsActorInFrontOfDoorway(TAA) ? Door->GetFrontThreatOwningRoom() : Door->GetBackThreatOwningRoom();
					for (ASWATCharacter* Swat : SwatAI)
					{
						if (IsSWATValid(Swat) && (Swat->GetTeam() == ClosestTeam || ClosestTeam == ETeamType::TT_SQUAD))
						{
							if (FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Swat->GetActorLocation()))
							{
								if (Room->Name != StackingRoom)
								{
									bAllInRoom = false;
									break;
								}
							}
							else
							{
								bAllInRoom = false;
								break;
							}
						}
					}

					if (bAllInRoom)
					{
						for (ADoor* OtherDoor : QueueInfo->ClearingRoom->AdditionalRootDoors)
						{
							if (!IsValid(OtherDoor))
								continue;
							
							if (Door != OtherDoor)
								OtherDoor->ActivateDoorBlocker();
						}
					}
					
					if (FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(StackingRoom))
					{
						for (ADoor* OtherDoor : Room->AdditionalRootDoors)
						{
							if (!IsValid(OtherDoor))
								continue;
							
							if (Door != OtherDoor)
								OtherDoor->DeactivateDoorBlocker();
						}
					}
					
					GiveBreachAndClearCommand(Door, EDoorBreachType::Move, ClosestTeam, TAA->GetActorLocation(), nullptr, nullptr, false, false, true, ClearingQueue[CommandTeam].Data.Num() <= 1);

					// deactivate the door blockers in case GiveBreachAndClearCommand failed somehow
					// dont wanna end up in a state where the doors are forever blocked off
					// we could let the door know about a breach activity but idk, need to think about it some more. what we have now seems to work good
					{
						bool bTeamHasBreachAndClear = false;
						for (ASWATCharacter* Swat : SwatAI)
						{
							if (IsSWATValid(Swat) && (Swat->GetTeam() == ClosestTeam || ClosestTeam == ETeamType::TT_SQUAD))
							{
								if (Swat->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
								{
									bTeamHasBreachAndClear = true;
									break;
								}
							}
						}

						if (!bTeamHasBreachAndClear)
						{
							for (ADoor* OtherDoor : QueueInfo->ClearingRoom->AdditionalRootDoors)
							{
								if (!IsValid(OtherDoor))
									continue;
							
								OtherDoor->DeactivateDoorBlocker();
							}
						}
					}
					
					NumCommandsGiven++;
					GivenBreachingRooms.AddUnique(BreachingRoom);
					It.RemoveCurrent();
				}
			}

			if (ClearingQueue[CommandTeam].Data.Num() == 0)
			{
				ClearingQueue.Remove(CommandTeam);

				if (CommandTeam == ETeamType::TT_SQUAD)
					bGoldAutoClearing = false;
			}
		}
	}
}

void USWATManager::AdvanceClearingQueue(ETeamType CommandTeam)
{
}

void USWATManager::OnTrailerSearchComplete(class UBaseActivity* Activity, ACyberneticController* Controller)
{
	UTrailerSearchAndSecureActivity* TrailerActivity = Cast<UTrailerSearchAndSecureActivity>(Activity);
	FRoom* SearchingRoom = TrailerActivity->SearchingRoom;
	
	TArray<AActor*> Securables;
	Securables.Reserve(200);
	UGameplayStatics::GetAllActorsWithInterface(this, USecurable::StaticClass(), Securables);

	TArray<AActor*> SortedSecurables;
	SortedSecurables.Reserve(Securables.Num());
	
	for (AActor* Actor : Securables)
	{
		if (Cast<ACyberneticCharacter>(Actor) && !Cast<ATrailerSWATCharacter>(Actor))
		{
			SortedSecurables.AddUnique(Actor);
		}
	}
	
	for (AActor* Actor : Securables)
	{
		if (ABaseItem* Item = Cast<ABaseItem>(Actor))
		{
			if (Item->IsEvidence())
			{
				SortedSecurables.AddUnique(Actor);
			}
		}
		else if (Cast<AEvidenceActor>(Actor))
		{
			SortedSecurables.AddUnique(Actor);
		}
	}
	
	for (AActor* Actor : Securables)
	{
		if (Cast<ACollectedEvidenceActor>(Actor))
		{
			SortedSecurables.AddUnique(Actor);
		}
	}
	
	TArray<AActor*> FinalSecurables;
	FinalSecurables.Reserve(SortedSecurables.Num());

	for (AActor* Actor : SortedSecurables)
	{
		if (Actor)
		{
			const FRoom* SecurableRoomLocation = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(ISecurable::Execute_GetLocation(Actor));
			
			if (SecurableRoomLocation == SearchingRoom)
			{
				if (ISecurable::Execute_CanBeSecuredByTrailers(Actor) && !Actor->IsHidden())
				{
					FinalSecurables.Add(Actor);
				}
			}
		}
	}
	
	TrailerActivity->SearchingRoom->bClearedByTrailers = FinalSecurables.Num() == 0;
	
	uint8 NumActivated = 0;
	for (ATrailerSWATCharacter* Trailer : SwatTrailers)
	{
		if (!Trailer->bDeactivated)
			NumActivated++;
	}

	if (FinalSecurables.Num() > 0 && (FinalSecurables.Num() >= NumActivated || NumActivated == 0))
	{
		FTimerDelegate Delegate;
		Delegate.BindWeakLambda(this, [=]()
		{
			TrailerActivity->SearchingRoom = SearchingRoom;
			TrailerActivity->AllSecurables = FinalSecurables;
			TrailerActivity->SpawnLocation = OriginalSpawnLocation;
			TrailerActivity->OnFinishActivity.RemoveAll(this);
			TrailerActivity->OnFinishActivity.AddDynamic(this, &USWATManager::OnTrailerSearchComplete);
			
			UActivityManager::GiveActivityTo(Activity, Controller->GetCharacter(), true, true);
		});
		
		GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
	}
	else
	{
		Activity->GetCharacter()->GetCharacterMovement()->SetMovementMode(MOVE_None);
		Activity->GetCharacter()->GetCapsuleComponent()->SetEnableGravity(false);
		Activity->GetCharacter()->SetActorLocation(FVector(OriginalSpawnLocation.X, OriginalSpawnLocation.Y, -10000.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Activity->GetCharacter()->SetActorHiddenInGame(true);
		Activity->GetCharacter()->bDeactivated = true;
		Activity->GetCharacter()->GetMesh()->bNoSkeletonUpdate = true;
		UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(Activity->GetCharacter());
		UReadyOrNotSignificanceManager::Get(this)->ForceActorNotRelevant(Activity->GetCharacter());
		Activity->GetCharacter()->SetActorTickEnabled(false);
	}
}

void USWATManager::GiveTrailerSearchAndSecure(ATrailerSWATCharacter* Character)
{
}

/*
void USWATManager::OnFallInPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	if (bWaitingOnPathTest)
	{
		FallInSwat_PathFound.Add(*FallInSwat_Queries.Find(PathId), NavPath->GetLength());

		if (FallInSwat_PathFound.Num() == SwatAI.Num())
		{
			bWaitingOnPathTest = false;
		}
		
		bool bFoundAnyDoor = false;
		if (const AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			for (FNavPathPoint& NavPathPoint : NavPath->GetPathPoints())
			{
				if (const ADoor* ClosestDoor = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(NavPathPoint.Location, GS->AllDoors))
				{
					if (FVector::Distance(NavPathPoint.Location, ClosestDoor->GetActorLocation()) < 500.0f) // 5m
					{
						//DrawDebugPoint(GetWorld(), NavPathPoint.Location, 5.0f, FColor::Red, false, 0.033f);
						bFoundAnyDoor = true;
						bAnyPathPointToLeaderNearDoor = true;
						break;
					}
				}
			}
		}

		if (!bFoundAnyDoor)
		{
			bAnyPathPointToLeaderNearDoor = false;
		}
	}
}

void USWATManager::OnSwatSortAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	// find the context so we can notify them
	const UObject* Context = nullptr;
	FSwatAsyncPathSortRequest* FoundRequest = nullptr;
	for (auto& k : PathSortRequests)
	{
		if (k.Value.PathQueries.Find(PathId))
		{
			Context = k.Key;
			FoundRequest = &k.Value;
		}
	}

	if (!IsValid(Context) || !FoundRequest)
		return;

	ASWATCharacter* Swat = *FoundRequest->PathQueries.Find(PathId);
	if (!IsSWATValid(Swat))
		return;

	if (FoundRequest->PathFound.Num() == FoundRequest->PathQueries.Num())
	{
		FoundRequest->bWaiting = false;

		FoundRequest->PathFound.ValueSort([](const FNavigationPath& Lhs, const FNavigationPath& Rhs)
		{
			return Lhs.GetLength() < Rhs.GetLength();
		});
		
		TArray<ASWATCharacter*> SortedSwat;
		FoundRequest->PathFound.GenerateKeyArray(SortedSwat);

		// notify the context
		FoundRequest->SuccessCallback.Execute(SortedSwat);

		PathSortRequests.Remove(Context);
	}
	else
	{
		FoundRequest->PathFound.Add(Swat, *NavPath.Get());
	}
}
*/

void USWATManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#ifndef NO_BUENO
	SCOPE_CYCLE_COUNTER(STAT_SWATManagerTick);

	#if !UE_BUILD_SHIPPING
	if (CVarSwatAutomate.GetValueOnAnyThread() > 0)
	{
		SquadLeader = GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Alpha);
	}
	#endif
	
	AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GS)
		return;

	// Find next available character to promote to squad leader
	if (!IsValid(SquadLeader) || SquadLeader->IsDeadOrUnconscious() || SquadLeader->IsIncapacitated())
	{
		if (SquadLeader)
		{
			SquadLeader->OnNightVisionGogglesToggled.RemoveAll(this);
		}
		
		SquadLeader = nullptr;
		
		// Look for player characters first
		for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
		{
			AReadyOrNotPlayerController* pc = *It;
			AReadyOrNotCharacter* Character = pc->GetPawn<AReadyOrNotCharacter>();
			
			if (!Character)
				continue;

			if (!(Character->IsDeadOrUnconscious() || Character->IsIncapacitated()))
			{
				SquadLeader = Character;
				break;
			}
		}

		// Then look for the next swat ai that is not dead or incapacitated
		// Ignore training game mode!
		if (!GetWorld()->GetAuthGameMode<ATrainingGM>())
		{
			if (!SquadLeader)
			{
				SquadLeader = GetSwatCharacterAtSquadPosition(ESquadPosition::SP_Alpha);

				if (ASWATCharacter* Swat = Cast<ASWATCharacter>(SquadLeader))
				{
					Swat->GetCyberneticsController()->FinishActivity(Swat->GetCyberneticsController()->GetCurrentActivity(), true, true);
					Swat->GetCyberneticsController()->ClearActivityList();
					GiveFallInCommand(ETeamType::TT_SQUAD);
				}
			}
		}

		// everyone is dead or incapacitated
		if (!SquadLeader)
		{
			return;
		}

		#if WITH_EDITOR
		if (SquadLeader->GetInventoryComponent()->GetSpawnedGear().Character)
			ULog::Success("New Squad Leader assigned to: " + SquadLeader->GetInventoryComponent()->GetSpawnedGear().Character->CharacterName.ToString());
		#endif
	}
	
	OriginalSpawnLocation = SquadLeader->OriginalSpawnLocation;
	
	if (!SquadLeader->OnNightVisionGogglesToggled.IsAlreadyBound(this, &USWATManager::OnLeaderToggledNightvision))
	{
		SquadLeader->OnNightVisionGogglesToggled.AddDynamic(this, &USWATManager::OnLeaderToggledNightvision);
	}
	
	{
		SCOPE_CYCLE_COUNTER(STAT_RemoveUnavailableSwat);

		//RemoveAllInvalidRequestOrders();

		// Remove all invalid or dead swat characters
		SwatAI.Remove(nullptr);
		SwatAI.RemoveAll([](ASWATCharacter* SWATCharacter)
		{
			return !IsValid(SWATCharacter) ||
					SWATCharacter->bDeactivated ||
					(SWATCharacter && (SWATCharacter->IsDeadOrUnconscious() ||
					SWATCharacter->IsIncapacitated() ||
					!IsValid(SWATCharacter->GetCyberneticsController())));
		});
	}
	
	if (SwatAI.Num() == 0 && SwatTrailers.Num() == 0)
	{
		return;
	}

	// Update squad positions
	{
		uint8 i = 0;
		for (ASWATCharacter* Swat : SwatAI)
		{
			Swat->SetSquadPosition((ESquadPosition)i);
			
			switch ((ESquadPosition)i)
			{
				case ESquadPosition::SP_Alpha:		Swat->GetCapsuleComponent()->AreaClass = UNavQuery_SwatAlpha::StaticClass(); break;
				case ESquadPosition::SP_Beta:		Swat->GetCapsuleComponent()->AreaClass = UNavQuery_SwatBeta::StaticClass(); break;
				case ESquadPosition::SP_Charlie:	Swat->GetCapsuleComponent()->AreaClass = UNavQuery_SwatCharlie::StaticClass(); break;
				case ESquadPosition::SP_Delta:		Swat->GetCapsuleComponent()->AreaClass = UNavQuery_SwatDelta::StaticClass(); break;
				default:							Swat->GetCapsuleComponent()->AreaClass = UNavQuery_Swat::StaticClass(); break;
			}
			
			i++;
		}
	}

	if (!bGivenInitialFallInCommand)
	{
		if (SquadLeader->GetVelocity().Size() > 50.0f || Cast<ASWATCharacter>(SquadLeader))
		{
			const bool bAnySwatHasActivity = UActivityManager::AnyAIHasActivity<UBaseActivity>([](const UBaseActivity* Activity)
			{
				if (Cast<UMoveToActivity>(Activity))
					return false;
				
				if (Activity->GetCharacter()->IsOnSWATTeam())
				{
					return true;
				}
				
				return false;
			});

			if (!bAnySwatHasActivity)
			{
				GiveFallInCommand(ETeamType::TT_SQUAD);
			}
		}
	}
	
	FVector SwatAverageLocation = GetAverageSwatLocation();

	//DrawDebugBox(GetWorld(), SwatAverageLocation, FVector(10.0f), FColor::Red, false, DeltaTime);
	//LOG_NUMBER(OccupiedLookAtPoints.Num());
	
	for (TMap<FIntVector, ASWATCharacter*>::TIterator It = OccupiedLookAtPoints.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Value()) || !It.Value()->IsActive())
			It.RemoveCurrent();
	}

	/*
	#if !UE_BUILD_SHIPPING
	for (const auto& It : OccupiedLookAtPoints)
	{
		DrawDebugLine(GetWorld(), It.Value->GetActorLocation(), FVector(It.Key), FColor::Yellow, false, DeltaTime);
		DrawDebugBox(GetWorld(), FVector(It.Key), FVector(10.0f), FColor::Cyan, false, DeltaTime);
	}
	#endif
	*/
	
	GlobalYellDelay -= DeltaTime;
	
	WaitingReplyDelay -= DeltaTime;

	PrefixCooldown -= DeltaTime;
	
	if (WaitingReplyDelay <= 0.0f)
	{
		WaitingReplyDelay = MaxWaitingReplyDelay;
		
		if (ASWATCharacter* RespondingSWAT = GetClosestSWATToActor(SquadLeader))
		{
			if (!RespondingSWAT->GetCyberneticsController()->GetActivity())
			{
				RespondingSWAT->PlayRawVO(VO_SWAT_GENERAL::CALL_WAITING);
			}
		}
	}

	TimeSincePlayerIssuedCommand += DeltaTime;
	TimeSinceLastTick += DeltaTime;
	if (TimeSinceLastTick > TickInterval)
	{
		TimeSinceLastTick = 0.0f;
		
		{
			SCOPE_CYCLE_COUNTER(STAT_RoomDetection);

			#if !UE_BUILD_SHIPPING
			if (!GIsAutomationTesting)
			#endif
			{
				// mark the current room as cleared
				if (FRoom* CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(SquadLeader->GetActorLocation()))
				{
					CurrentRoom->bClearedBySwat = true; //time based? maybe over 3 sec in room, mark as cleared?
					
					GS->RoomData->ClearedRooms.AddUnique(CurrentRoom);
					
					// automatically clear empty rooms
					for (FRoom& Room : GS->RoomData->Rooms)
					{
						if (!Room.bClearedBySwat && Room.Threats.Num() == 0)
						{
							Room.bClearedBySwat = true;
							Room.bClearedByTrailers = true;
							GS->RoomData->ClearedRooms.AddUnique(&Room);
						}
					}
				}
				
				for (ASWATCharacter* Swat : SwatAI)
				{
					if (Swat->GetCyberneticsController()->GetActivity<UTeamFallinActivity>())
					{
						if (FRoom* CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Swat->GetActorLocation()))
						{
							CurrentRoom->bClearedBySwat = true;
							
							GS->RoomData->ClearedRooms.AddUnique(CurrentRoom);
						}
					}
				}
			}
		}
		
		//bForceSnakeFallIn = bAnyPathPointToLeaderNearDoor;

		FVector Blah;
		bool bProjectSuccessRight = UReadyOrNotAISystem::ProjectPointToNav(SquadLeader->GetActorLocation() + SquadLeader->GetActorRightVector() * 100.0f, Blah);
		bool bProjectSuccessLeft = UReadyOrNotAISystem::ProjectPointToNav(SquadLeader->GetActorLocation() + SquadLeader->GetActorRightVector() * -100.0f, Blah);
		bool bAnySwatProjectFail = false;
		/*
		for (ASWATCharacter* Swat : SwatAI)
		{
			bool bSwatProjectSuccessRight = UReadyOrNotAISystem::ProjectPointToNav(Swat->GetActorLocation() + Swat->GetActorRightVector() * 100.0f, Blah);
			bool bSwatProjectSuccessLeft = UReadyOrNotAISystem::ProjectPointToNav(Swat->GetActorLocation() + Swat->GetActorRightVector() * -100.0f, Blah);
			
			if (!bSwatProjectSuccessLeft || !bSwatProjectSuccessRight)
			{
				bAnySwatProjectFail = true;
				break;
			}
		}
		*/
		
		if (!bProjectSuccessLeft || !bProjectSuccessRight || bAnySwatProjectFail)
		{
			bForceSnakeFallIn = true;
		}
		else
		{
			bForceSnakeFallIn = false;
		}

		FVector AverageDirection = FVector::ZeroVector;
		for (ASWATCharacter* swat : SwatAI)
		{
			AverageDirection += (SwatAverageLocation - swat->GetActorLocation()).GetSafeNormal2D();
		}
		
		AverageDirection.Normalize();
		FVector PerpDirection = FVector::CrossProduct(AverageDirection, FVector::UpVector);
		
		TArray<ASWATCharacter*> FrontSwat, BackSwat;
		for (ASWATCharacter* swat : SwatAI)
		{
			if (FVector::DotProduct((swat->GetActorLocation() - SwatAverageLocation).GetSafeNormal2D(), PerpDirection) < 0.0f)
			{
				BackSwat.Add(swat);
			}
			else
			{
				FrontSwat.Add(swat);
			}
		}
			
		{
			SCOPE_CYCLE_COUNTER(STAT_FallInSort);
			
			//TArray<ASWATCharacter*> TempSwat;
			//TempSwat.Reserve(SwatAI.Num());
				
			FallInSwat_PathFound.Empty(4);

			//if (FallInSwat_Queries.Num() < SwatAI.Num())
			{
				// path distance is more correct and stable than regular straight line distance
				for (ASWATCharacter* swat : SwatAI)
				{
					if (swat->GetCyberneticsController()->GetActivity<UTeamFallinActivity>())
					{
						//TempSwat.Add(swat);
						if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
						{
							if (const ANavigationData* NavData = NavSys->GetNavDataForProps(swat->GetNavAgentPropertiesRef()))
							{
								TSubclassOf<UNavigationQueryFilter> QueryFilterClass = UNavQuery_SwatFallIn::StaticClass();
								const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, QueryFilterClass);
								const FVector StartLocation = swat->GetNavAgentLocation();
								const FVector EndLocation = SquadLeader->GetNavAgentLocation();
								
								FNavLocation StartLocationProjected(StartLocation);
								FNavLocation EndLocationProjected(EndLocation);

								NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 200.0f));
								NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(75.0f, 75.0f, 200.0f));

								//FNavPathQueryDelegate NavDelegate;
								//NavDelegate.BindUObject(this, &USWATManager::OnFallInPathFound);

								//uint32 PathId = NavSys->FindPathAsync(swat->GetNavAgentPropertiesRef(), ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, true, swat->GetCyberneticsController()), NavDelegate, EPathFindingMode::Regular);
								//FallInSwat_Queries.Add(PathId, swat);
								FPathFindingResult Result = NavSys->FindPathSync(swat->GetNavAgentPropertiesRef(), ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, false, swat->GetCyberneticsController()), EPathFindingMode::Regular);
								if (Result.IsSuccessful() && !Result.IsPartial() && Result.Path.IsValid())
								{
									if (FVector::PointsAreNear(Result.Path->GetDestinationLocation(), EndLocationProjected, 100.0f))
									{
										FallInSwat_PathFound.Add(swat, FVector::Distance(StartLocation, EndLocation) + Result.Path->GetLength());
									}

									/*
									for (uint32 i = Result.Path->GetPathPoints().Num()-1; i > 0; i--)
									{
										FVector Start = Result.Path->GetPathPoints()[i].Location + FVector(0.0f, 0.0f, 10.0f);
										FVector End = Result.Path->GetPathPoints()[i-1].Location + FVector(0.0f, 0.0f, 10.0f);
										
										DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, DeltaTime + 0.001f, 0, 1.25f);
									}
									ULog::ObjectName(swat);
									LOG_NUMBER(Result.Path->GetLength());
									*/
								}
							}
						}
					}
				}

				//bWaitingOnPathTest = false;
			}

			//if (!bWaitingOnPathTest)
			{
				FallInSwat_PathFound.ValueSort([](const float& Lhs, const float& Rhs)
				{
					return Lhs < Rhs;
				});
				
				//TArray<ASWATCharacter*> SortedSWAT;
				FallInSwat_PathFound.GenerateKeyArray(FallInSwat);

				//FallInSwat_Queries.Empty();
				//bAnyPathPointToLeaderNearDoor = false;
				
				/*
				if (SortedSWAT.Num() > 0)
				{
					if (ClosestFallInSwat != SortedSWAT[0])
						FallInSwat.Empty();
				}
				
				bool bAnyZHeightIsDifferent = false;
				for (ASWATCharacter* Swat : SortedSWAT)
				{
					const bool bZHeightIsDifferent = (uint16)FMath::Abs(SquadLeader->GetNavAgentLocation().Z - Swat->GetNavAgentLocation().Z) >= 20;
					if (bZHeightIsDifferent)
					{
						bAnyZHeightIsDifferent = true;
						break;
					}
				}

				float Dist = 0.0f;
				if (SortedSWAT.Num() > 0)
					Dist = FVector::Distance(SquadLeader->GetActorLocation(), SortedSWAT[0]->GetActorLocation());
				LOG_NUMBER(Dist);
				*/
				/*
				if (SortedSWAT.Num() > 0)// && !bAnyZHeightIsDifferent && Dist > 250.0f)
				{
					FallInSwat.Empty();
					ClosestFallInSwat = SortedSWAT[0];
					
					//ASWATCharacter* ClosestSwat = GetClosestSWATToActor(SquadLeader);
					//ASWATCharacter* ClosestSwat = SortedSWAT[0];
					ASWATCharacter* ClosestSwat = nullptr;
					TArray<ASWATCharacter*> SwatSide = FrontSwat;
					
					// at back?
					if (FVector::DotProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal2D(), PerpDirection) < 0.0f)
					{
						SwatSide = BackSwat;
					}
					
					FCollisionQueryParams QueryParams;
					QueryParams.AddIgnoredActor(SquadLeader);
					QueryParams.AddIgnoredActors((TArray<AActor*>)SwatAI);
					QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
							
					// needed for proper ordering of swat when falling in without a LOS to the player
					bool bAnySwatNoLosToLeader = false;
					for (ASWATCharacter* swat : SortedSWAT)
					{
						if (GetWorld()->LineTraceTestByObjectType(swat->GetActorLocation(), SquadLeader->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams))
						{
							bAnySwatNoLosToLeader = true;
							break;
						}
					}

					ASWATCharacter* FurthestSwat = UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation<ASWATCharacter>(SwatAverageLocation, SwatSide);

					ClosestSwat = FurthestSwat;

					bool bExceededDistanceThreshold = FVector::Distance(SortedSWAT[0]->GetActorLocation(), SquadLeader->GetActorLocation()) > 300.0f;
					if (!bExceededDistanceThreshold)
					{
						if (ClosestSwat != SortedSWAT[0])
							ClosestSwat = nullptr;
					}
					else
					{
						ClosestSwat = SortedSWAT[0];
					}
					
					if (!bAnySwatNoLosToLeader) // needed for proper ordering of swat when falling in without a LOS to the player
						SortedSWAT.Empty();
					
					if (ClosestSwat)
					{
						SortedSWAT.AddUnique(ClosestSwat);
						TempSwat.Remove(ClosestSwat);

						for (uint8 i = 0; i < SwatAI.Num(); i++)
						{
							// Get closest swat (with extra conditions)
							{
								ASWATCharacter* ClosestSwatTemp = nullptr;
								float ClosestDist = FLT_MAX;
								for (int32 j = 0; j < SwatAI.Num(); j++)
								{
									if (SwatAI[j]->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>())
									{
										if (ClosestSwat != SwatAI[j] && !SortedSWAT.Contains(SwatAI[j]))
										{
											float tempDist = FVector::DistSquared(SwatAI[j]->GetActorLocation(), ClosestSwat->GetActorLocation());
											if (tempDist < ClosestDist)
											{
												ClosestDist = tempDist;
												ClosestSwatTemp = SwatAI[j];
											}
										}
									}
								}
								
								ClosestSwat = ClosestSwatTemp;
							}
							
							if (!ClosestSwat)
							{
								for (ASWATCharacter* Other : TempSwat)
									SortedSWAT.AddUnique(Other);
									
								break;
							}
							
							SortedSWAT.AddUnique(ClosestSwat);
							TempSwat.Remove(ClosestSwat);
						}
					}
				}
					FallInSwat = SortedSWAT;
				*/
			}
		}

		// Half snake swat ordering
		{
			//if (bExceededDistanceThreshold && SwatAI.Num() > 1)
			{
				FallInSwat_FileA = {};
				FallInSwat_FileB = {};
				for (uint8 i = 0; i < SwatAI.Num(); i++)
				{
					ASWATCharacter* swat = SwatAI[i];

					if (!FallInSwat_FileA.Contains(swat) && !FallInSwat_FileB.Contains(swat))
					{
						FVector PerpDirection2 = FVector::CrossProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), FVector::UpVector);
						bool bRightSide = FVector::DotProduct((swat->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), PerpDirection2) > 0.0f;
						if (bRightSide)
						{
							FallInSwat_FileA.AddUnique(swat);
						}
						else
						{
							FallInSwat_FileB.AddUnique(swat);
						}
					}
				}

				FallInSwat_FileA.Sort([&](ASWATCharacter& Lhs, ASWATCharacter& Rhs)
				{
					return FVector::Distance(Lhs.GetActorLocation(), SquadLeader->GetActorLocation()) < FVector::Distance(Rhs.GetActorLocation(), SquadLeader->GetActorLocation());
				});
				
				FallInSwat_FileB.Sort([&](ASWATCharacter& Lhs, ASWATCharacter& Rhs)
				{
					return FVector::Distance(Lhs.GetActorLocation(), SquadLeader->GetActorLocation()) < FVector::Distance(Rhs.GetActorLocation(), SquadLeader->GetActorLocation());
				});
			}
		}

		// Diamond fall in ordering
		{
			FallInSwat_Diamond = {};
			
			if (SwatAI.Num() > 2)
			{
				TArray<ASWATCharacter*> TempSwat = SwatAI;
				TArray<ASWATCharacter*> SortedSwat;
				
				ASWATCharacter* ClosestSwatToLeader = GetClosestSWATToActor(SquadLeader);

				SortedSwat.AddUnique(ClosestSwatToLeader);

				TempSwat.Remove(ClosestSwatToLeader);
				
				for (uint8 i = 0; i < SwatAI.Num(); i++)
				{
					// Get closest swat (with extra conditions)
					{
						ASWATCharacter* ClosestSwatTemp = nullptr;
						float ClosestDist = FLT_MAX;
						for (int32 j = 0; j < TempSwat.Num(); j++)
						{
							if (!SortedSwat.Contains(TempSwat[j]))
							{
								float tempDist = FVector::DistSquared(TempSwat[j]->GetActorLocation(), ClosestSwatToLeader->GetActorLocation());
								if (tempDist < ClosestDist)
								{
									ClosestDist = tempDist;
									ClosestSwatTemp = TempSwat[j];
								}
							}
						}

						TempSwat.Remove(ClosestSwatTemp);
						SortedSwat.AddUnique(ClosestSwatTemp);
						
						if (TempSwat.Num() == 1)
						{
							SortedSwat += TempSwat;
							TempSwat.Empty();
							break;
						}
					}
				}

				FallInSwat_Diamond = SortedSwat;
			}
		}
		
		{
			SCOPE_CYCLE_COUNTER(STAT_SpeechCooldown);
			
			for (TMap<FName, float>::TIterator It = SpeechCooldownMap.CreateIterator(); It; ++It)
			{
				It.Value() -= DeltaTime;
				if (It.Value() <= 0.0f)
				{
					It.RemoveCurrent();
				}
			}
		}
		
		// auto secure any targets in the cleared rooms
		#if !UE_BUILD_SHIPPING
		if (!GIsAutomationTesting)
		#endif
		{
			bool bBeenAWhileSinceActivityGiven = TimeSincePlayerIssuedCommand > 5.0f;
			
			if (!UReadyOrNotAISystem::WasRecentlyInCombat(7.0f) &&
				bBeenAWhileSinceActivityGiven &&
				!GetWorld()->GetAuthGameMode<ATrainingGM>())
			{
				TArray<AActor*> SortedSecurables = GetAvailableSecurables();
				
				if (SortedSecurables.Num() > 0)
				{
					for (FRoom* Room : GS->RoomData->ClearedRooms)
					{
						if (Room->bClearedBySwat)
						{
							TArray<ASWATCharacter*> SwatInRoom;
							SwatInRoom.Reserve(4);
							
							// get all swat within this room
							for (ASWATCharacter* Swat : SwatAI)
							{
								//DrawDebugBox(GetWorld(), Swat->GetNavAgentLocation()+FVector(0.0f, 0.0f, 10.0f), FVector(15.0f), FColor::Cyan, false, 1.0f);
								if (FRoom* SwatRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Swat->GetNavAgentLocation()+FVector(0.0f, 0.0f, 10.0f)))
								{
									if (SwatRoom == Room)
									{
										SwatInRoom.Add(Swat);
									}
								}
							}

							// if none, skip...
							if (SwatInRoom.Num() == 0)
								continue;
							
							TArray<AActor*> FinalSecurables;
							FinalSecurables.Reserve(SortedSecurables.Num());
							
							for (AActor* Actor : SortedSecurables)
							{
								if (Actor)
								{
									if (const FRoom* SecurableRoomLocation = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(ISecurable::Execute_GetLocation(Actor)))
									{
										if (SecurableRoomLocation == Room)
										{
											FinalSecurables.Add(Actor);
										}
									}
								}
							}
							
							if (FinalSecurables.Num() > 0)
							{
								for (AActor* Securable : FinalSecurables)
								{
									if (Cast<ACyberneticCharacter>(Securable))
									{
										// get closest available swat to securable
										if (ASWATCharacter* Swat = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation<ASWATCharacter>(ISecurable::Execute_GetLocation(Securable), SwatInRoom))
										{
											bool bIsCovering = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamCoverAreaActivity>() != nullptr;
											bool bIsHolding = Swat->GetCyberneticsController()->GetCurrentActivity<UHoldActivity>() != nullptr;

											float DistanceToAI = FVector::Distance(Swat->GetActorLocation(), ISecurable::Execute_GetLocation(Securable));

											bool bInRange = Cast<ASWATCharacter>(SquadLeader) || DistanceToAI < 500.0f;
											
											bool bCanSecure = bInRange && !bIsHolding && !bIsCovering &&
																(!Swat->GetCyberneticsController()->GetActivity() ||
																Swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>());
											
											if (bCanSecure)
											{
												if (UActivityManager::AnyAIHasActivity<UArrestTargetActivity>([&](const UArrestTargetActivity* Activity)
												{
													return Activity->ArrestTarget == Cast<ACyberneticCharacter>(Securable);
												}))
												{
													break;
												}
												
												if (UArrestTargetActivity* ArrestTargetActivity = Swat->GetCyberneticsController<ASWATController>()->GetArrestTargetActivity())
												{
													ArrestTargetActivity->ArrestTarget = Cast<ACyberneticCharacter>(Securable);
													
													UActivityManager::GiveActivityTo(ArrestTargetActivity, Swat, true, false);
													break;
												}
											}
										}
									}
									else if (Cast<ABaseItem>(Securable) || Cast<AEvidenceActor>(Securable))
									{
										// get closest available swat to securable
										if (ASWATCharacter* Swat = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation<ASWATCharacter>(ISecurable::Execute_GetLocation(Securable), SwatInRoom))
										{
											bool bIsCovering = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamCoverAreaActivity>() != nullptr;
											bool bIsHolding = Swat->GetCyberneticsController()->GetCurrentActivity<UHoldActivity>() != nullptr;
											float DistanceToEvidence = FVector::Distance(Swat->GetNavAgentLocation(), ISecurable::Execute_GetLocation(Securable));
											bool bIsHoldingNearSecurable = (bIsHolding || bIsCovering) && DistanceToEvidence < 300.0f;

											bool bInRange = Cast<ASWATCharacter>(SquadLeader) || DistanceToEvidence < 500.0f;
											
											bool bCanSecure = bInRange &&
																(!Swat->GetCyberneticsController()->GetActivity() ||
																(Swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() ||
																bIsHoldingNearSecurable));
											
											if (bCanSecure)
											{
												if (UActivityManager::AnyAIHasActivity<UCollectEvidenceActivity>([&](const UCollectEvidenceActivity* Activity)
												{
													return Activity->EvidenceItem == Securable;
												}))
												{
													break;
												}
												
												if (UCollectEvidenceActivity* CollectEvidenceActivity = Swat->GetCyberneticsController<ASWATController>()->GetCollectEvidenceActivity())
												{
													CollectEvidenceActivity->EvidenceItem = Securable;

													UActivityManager::GiveActivityTo(CollectEvidenceActivity, Swat, true, false);
													break;
												}
											}
										}
									}
									else if (Cast<AReportableActor>(Securable))
									{
										// get closest available swat to securable
										if (ASWATCharacter* Swat = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation<ASWATCharacter>(ISecurable::Execute_GetLocation(Securable), SwatInRoom))
										{
											bool bIsCovering = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamCoverAreaActivity>() != nullptr;
											bool bIsHolding = Swat->GetCyberneticsController()->GetCurrentActivity<UHoldActivity>() != nullptr;
											float DistanceToReportable = FVector::Distance(Swat->GetNavAgentLocation(), ISecurable::Execute_GetLocation(Securable));
											bool bIsHoldingNearSecurable = (bIsHolding || bIsCovering) && DistanceToReportable < 300.0f;

											bool bInRange = Cast<ASWATCharacter>(SquadLeader) || DistanceToReportable < 500.0f;
											
											bool bCanSecure = bInRange &&
																(!Swat->GetCyberneticsController()->GetActivity() ||
																(Swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() ||
																bIsHoldingNearSecurable));
											
											if (bCanSecure)
											{
												if (UActivityManager::AnyAIHasActivity<UReportTargetActivity>([&](const UReportTargetActivity* Activity)
												{
													return Activity->ReportTarget == Securable;
												}))
												{
													break;
												}
												
												if (UReportTargetActivity* ReportActivity = Swat->GetCyberneticsController<ASWATController>()->GetReportTargetActivity())
												{
													ReportActivity->ReportTarget = Securable;

													UActivityManager::GiveActivityTo(ReportActivity, Swat, true, false);
													break;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// auto squad leader command
		if (!GetWorld()->GetAuthGameMode<ATrainingGM>() &&
			Cast<ASWATCharacter>(SquadLeader) && Cast<ASWATCharacter>(SquadLeader)->GetCyberneticsController() && GS->RoomData)
		{
			bool bNoneHasActivity = true;
			for (ASWATCharacter* Swat : SwatAI)
			{
				// ignore these as they're just movement based activities and not substantial activities
				if (Swat->GetCyberneticsController()->GetCurrentActivity<UMoveToActivity>() ||
					Swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>())
					continue;
				
				if (Swat->GetCyberneticsController()->GetActivity())
				{
					//ULog::ObjectName(Swat->GetCyberneticsController()->GetActivity());
					bNoneHasActivity = false;
					break;
				}
			}

			bool bAnythingNeedsSecuring = false;
			if (FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(SquadLeader->GetActorLocation()))
			{
				TArray<AActor*> SortedSecurables = GetAvailableSecurables();
				for (AActor* Securable : SortedSecurables)
				{
					if (IsValid(Securable))
					{
						if (UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(ISecurable::Execute_GetLocation(Securable)) == Room)
						{
							ULog::ObjectName(Securable);
							bAnythingNeedsSecuring = true;
							break;
						}
					}
				}
			}

			if (!bAnythingNeedsSecuring)
			{
				TArray<FRoom*> ClearedRooms = GS->RoomData->ClearedRooms;
				
				/*
				FRoom* FirstRoom = nullptr;
				
				if (GS->RoomData->ClearedRooms.Num() > 0)
				{
					FirstRoom = GS->RoomData->ClearedRooms[0];
					if (!AutoClearingRooms.Contains(FirstRoom))
					{
						AutoClearingRooms.Empty();
						AutoClearingRooms.Add(FirstRoom);
					}
				}
				*/

				bool bAllRoomsCleared = GS->RoomData->ClearedRooms.Num() == GS->RoomData->Rooms.Num();
				if (bAllRoomsCleared)
				{
					if (bNoneHasActivity)
					{
						if (AScoringManager::Get()->IsEveryoneKilledOrArrested())
						{
							if (FVector::Distance(OriginalSpawnLocation, SquadLeader->GetActorLocation()) > 300.0f)
							{
								Cast<ASWATCharacter>(SquadLeader)->GetCyberneticsController()->ClearActivityList();
								Cast<ASWATCharacter>(SquadLeader)->GetCyberneticsController()->GiveMoveTo(OriginalSpawnLocation);
								GiveFallInCommand(ETeamType::TT_SQUAD);

							}
							
							if (ACoopGM* GM = GetWorld()->GetAuthGameMode<ACoopGM>())
							{
								if (GM->GetMatchState() != EMatchState::MS_MatchEnded)
									GM->ExfiltrateMission(SwatAI);
							}
						}
						else
						{
							// todo: need to make sure everyone is out of action before giving search and secure command. right now swat will be stuck after clearing all rooms
							GiveSearchAndSecureCommand(ETeamType::TT_SQUAD, SquadLeader->GetActorLocation());
						}
					}
				}
				else
				{
					bool bIsEveryoneFallingIn = true;
					if (SwatAI.Num() > 1)
					{
						for (ASWATCharacter* Swat : SwatAI)
						{
							if (Swat != SquadLeader)
							{
								if (Swat->GetCyberneticsController()->GetCurrentActivity<UMoveToActivity>())
									continue;
								
								if (!Swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>())
								{
									bIsEveryoneFallingIn = false;
									break;
								}
							}
						}
					}
					else
					{
						bIsEveryoneFallingIn = bNoneHasActivity;
					}

					/*
					bool bTrackingNoOne = true;
					
					for (ASWATCharacter* Swat : SwatAI)
					{
						if (Swat->GetCyberneticsController()->GetTrackedTarget())
						{
							bTrackingNoOne = false;
							break;
						}
					}
					*/

					while (ClearedRooms.Num() > 0 && bIsEveryoneFallingIn /*&& bTrackingNoOne*/)
					{
						if (FRoom* CurrentRoom = ClearedRooms.Last())
						{
							/*
							if (CurrentRoom->ConnectingRooms.Num() == 0)
							{
								ClearedRooms.Pop();
								continue;
							}

							CurrentRoom->ConnectingRooms.Sort([&](const FName& Lhs, const FName& Rhs)
							{
								uint8 NumLhsRooms = 0;
								if (FRoom* NextRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(Lhs))
								{
									for (const FName& Name : NextRoom->ConnectingRooms)
									{
										if (CurrentRoom->Name != Name)
										{
											NumLhsRooms++;
										}
									}
								}
								
								uint8 NumRhsRooms = 0;
								if (FRoom* NextRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(Rhs))
								{
									for (const FName& Name : NextRoom->ConnectingRooms)
									{
										if (CurrentRoom->Name != Name)
										{
											NumRhsRooms++;
										}
									}
								}

								return NumLhsRooms < NumRhsRooms;
							});
							*/

							bool bGivenCommand = false;

							//FRoom* ClearingRoom = nullptr;
							for (FName& Name : CurrentRoom->ConnectingRooms)
							{
								if (FRoom* NextRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(Name))
								{
									if (!NextRoom->bClearedBySwat)
									{
										for (ADoor* Door : NextRoom->AdditionalRootDoors)
										{
											if (Door)
												Door->ActivateDoorBlocker();
										}

										// find closest pathed door from current room location
										float ClosestDist = FLT_MAX;
										ADoor* ClosestDoor = NextRoom->RootDoor;
										FVector CommandLocation = FVector(CurrentRoom->Location);
										for (ADoor* Door : NextRoom->AdditionalRootDoors)
										{
											if (!Door)
												continue;
											
											if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
												continue;
											
											float Distance = FLT_MAX;
											FVector To = FVector::ZeroVector;
											if (Door->GetFrontThreatOwningRoom() == NextRoom->Name)
											{
												To = Door->BackThreat->GetActorLocation();
												UReadyOrNotAISystem::FindPath(FVector(CurrentRoom->Location), To, &Distance);
											}
											else if (Door->GetBackThreatOwningRoom() == NextRoom->Name)
											{
												To = Door->FrontThreat->GetActorLocation();
												UReadyOrNotAISystem::FindPath(FVector(CurrentRoom->Location), To, &Distance);
											}

											if (Distance < ClosestDist)
											{
												ClosestDist = Distance;
												ClosestDoor = Door;
												CommandLocation = To;
											}
										}
										
										for (ADoor* Door : NextRoom->AdditionalRootDoors)
										{
											if (Door)
												Door->DeactivateDoorBlocker();
										}
										
										EDoorBreachType BreachType = EDoorBreachType::Open;
										
										TSubclassOf<ABaseItem> DoorBreachItemClass = nullptr, DoorUseItemClass = nullptr;
										
										bool bTeamHasFlashbang = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_Flashbang) != nullptr;
										bool bTeamHasStinger = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_Stingball) != nullptr;
										bool bTeamHasGas = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_CSGas) != nullptr;
										
										bool bTeamHasLauncher = false;
										if (ASWATCharacter* Swat = GetSwatWithItem(ETeamType::TT_SQUAD, AGrenadeLauncher::StaticClass()))
										{
											if (AGrenadeLauncher* Launcher = Swat->GetInventoryComponent()->GetInventoryItemOfClass_Native<AGrenadeLauncher>(AGrenadeLauncher::StaticClass(), false))
											{
												bTeamHasLauncher = Launcher->HasAmmo();
											}
										}
										
										TArray<EItemCategory, TFixedAllocator<5>> BreachItems = {EItemCategory::IC_None, EItemCategory::IC_Flashbang, EItemCategory::IC_Stingball, EItemCategory::IC_CSGas, EItemCategory::IC_Launcher};
										if (!bTeamHasFlashbang)
											BreachItems.Remove(EItemCategory::IC_Flashbang);
										
										if (!bTeamHasStinger)
											BreachItems.Remove(EItemCategory::IC_Stingball);
										
										if (!bTeamHasGas)
											BreachItems.Remove(EItemCategory::IC_CSGas);
										
										if (!bTeamHasLauncher)
											BreachItems.Remove(EItemCategory::IC_Launcher);
										
										EItemCategory ChosenBreachItem = BreachItems[FMath::RandRange(0, BreachItems.Num()-1)];

										// using a breaching item for a small room is a bit overkill :p
										{
											uint8 NumNonExtreme = 0;
											for (AThreatAwarenessActor* TAA : NextRoom->Threats)
											{
												if (TAA->GetThreatLevel() != EThreatLevel::TL_Extreme)
													NumNonExtreme++;
											}
											
											if (NumNonExtreme < 4)
											{
												ChosenBreachItem = EItemCategory::IC_None;
											}
										}

										switch (ChosenBreachItem)
										{
											case EItemCategory::IC_None:			DoorBreachItemClass = nullptr; break;
											case EItemCategory::IC_Flashbang:		DoorBreachItemClass = FlashbangClass; break;
											case EItemCategory::IC_Stingball:		DoorBreachItemClass = StingerClass; break;
											case EItemCategory::IC_CSGas:			DoorBreachItemClass = CSGasClass; break;
											case EItemCategory::IC_Launcher:		DoorBreachItemClass = AGrenadeLauncher::StaticClass(); break;
											
											default: DoorBreachItemClass = nullptr; break;
										}

										if (ClosestDoor->IsDoorwayOnly() || ClosestDoor->IsOpenBeyondCloseThreshold())
										{
											BreachType = EDoorBreachType::Move;
										}
										else
										{
											bool bTeamHasShotgun = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_BreachingShotgun) != nullptr;
											bool bTeamHasRam = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_BatteringRam) != nullptr;
											bool bTeamHasC2 = GetSwatWithItemType(ETeamType::TT_SQUAD, EItemCategory::IC_C2Explosive) != nullptr;
											
											TArray<EDoorBreachType, TFixedAllocator<5>> Breaches = {EDoorBreachType::Open, EDoorBreachType::Kick, EDoorBreachType::Shotgun, EDoorBreachType::Ram, EDoorBreachType::C2};
											if (!bTeamHasShotgun)
												Breaches.Remove(EDoorBreachType::Shotgun);
											
											if (!bTeamHasRam)
												Breaches.Remove(EDoorBreachType::Ram);
											
											if (!bTeamHasC2)
												Breaches.Remove(EDoorBreachType::C2);
											
											if (ClosestDoor->IsLocked() || ClosestDoor->IsJammed() || ClosestDoor->IsDoorBroken())
											{
												Breaches.Remove(EDoorBreachType::Open);
											}

											if (!ClosestDoor->IsDestructible())
											{
												Breaches.Remove(EDoorBreachType::Shotgun);
											}
											
											BreachType = Breaches[FMath::RandRange(0, Breaches.Num()-1)];

											switch (BreachType)
											{
												case EDoorBreachType::None:			DoorUseItemClass = nullptr; break;
												case EDoorBreachType::Open:			DoorUseItemClass = nullptr; break;
												case EDoorBreachType::Kick:			DoorUseItemClass = nullptr; break;
												case EDoorBreachType::Shotgun:		DoorUseItemClass = ABreachingShotgun::StaticClass(); break;
												case EDoorBreachType::Ram:			DoorUseItemClass = ADoorRam::StaticClass(); break;
												case EDoorBreachType::C2:			DoorUseItemClass = AC2Explosive::StaticClass(); break;
												default:							DoorUseItemClass = nullptr; break;
											}
										}

										// automatically detect the safest stacking position
										EStackUpStyle BestStackUpStyle = EStackUpStyle::Auto;
										EDoorRoomPosition FrontRoomPosition = ClosestDoor->FrontRoomPosition;
										EDoorRoomPosition BackRoomPosition = ClosestDoor->BackRoomPosition;
										if (ClosestDoor->IsPointInFrontOfDoorway(CommandLocation))
										{
											if (BackRoomPosition == EDoorRoomPosition::Center)
											{
												if (ClosestDoor->IsOpen_Backward())
													BestStackUpStyle = EStackUpStyle::Left;
												else if (FrontRoomPosition == EDoorRoomPosition::CornerRight)
													BestStackUpStyle = EStackUpStyle::Left;
												else if (FrontRoomPosition == EDoorRoomPosition::CornerLeft)
													BestStackUpStyle = EStackUpStyle::Right;
											}
										}
										else
										{
											if (FrontRoomPosition == EDoorRoomPosition::Center)
											{
												if (ClosestDoor->IsOpen_Forward())
													BestStackUpStyle = EStackUpStyle::Right;
												else if (BackRoomPosition == EDoorRoomPosition::CornerRight)
													BestStackUpStyle = EStackUpStyle::Left;
												else if (BackRoomPosition == EDoorRoomPosition::CornerLeft)
													BestStackUpStyle = EStackUpStyle::Right;
											}
										}
										
										GiveBreachAndClearCommand(ClosestDoor, BreachType, ETeamType::TT_SQUAD, CommandLocation, DoorBreachItemClass, DoorUseItemClass, false, false, false, false, BestStackUpStyle);

										bGivenCommand = true;
										
										//ClearingRoom = NextRoom;

										break;
									}
								}
							}

							/*
							if (ClearingRoom)
							{
								uint8 NumUnclearedRooms = 0;
								for (FName& Name : ClearingRoom->ConnectingRooms)
								{
									if (CurrentRoom->Name != Name)
									{
										if (FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(Name))
										{
											if (!Room->bClearedBySwat)
											{
												NumUnclearedRooms++;
											}
										}
									}
								}
								
								if (NumUnclearedRooms > 0)
								{
									AutoClearingRooms.Add(ClearingRoom);
								}
							}
							*/
							
							if (!bGivenCommand)
							{
								ClearedRooms.Pop();
							}
							else
							{
								break;
							}
						}
					}
					
					if (bNoneHasActivity)
					{
						GiveFallInCommand(ETeamType::TT_SQUAD);
					}
				}
			}
		}

		// report any targets that we encounter
		/*
		{
			if (!UReadyOrNotAISystem::WasRecentlyInCombat(2.0f))
			{
				if (ReportQueue.Num() > 0)
				{
					bool bTocWaitingOnAnyone = false;
					for (ASWATCharacter* Swat : SwatAI)
					{
						if (!Swat->TOCResponseLine.IsEmpty())
						{
							//ULog::Info("Waiting on " + Swat->GetName());
							bTocWaitingOnAnyone = true;
							break;
						}
					}
					
					if (!bTocWaitingOnAnyone && ATOCManager::Get()->GetNumQueuedResponses() == 0 && !ATOCManager::Get()->IsTOCSpeaking())
					{
						AActor* ReportActor = nullptr;
						for (auto& It : ReportQueue)
						{
							ReportActor = It.Key;
							
							if (It.Value->IsActive())
								It.Value->Server_ReportTarget_Implementation(It.Key);
							
							break;
						}

						if (ReportActor)
						{
							ReportQueue.Remove(ReportActor);
						}
					}
				}
			}
		}
		*/

		// trailer update
		#if !UE_BUILD_SHIPPING
		if (!GIsAutomationTesting)
		#endif
		{
			SCOPE_CYCLE_COUNTER(STAT_SwatTrailerUpdate);
			
			bool bMissionSuccess = false;
			if (AScoringManager* ScoringManager = AScoringManager::Get())
			{
				bMissionSuccess = ScoringManager->IsEveryoneKilledOrArrested();//GetWorld()->GetGameState<ACoopGS>()->bMissionSucceded || GetWorld()->GetGameState<ACoopGS>()->bMissionSoftCompleted;
			}

			if (bMissionSuccess && !GetWorld()->GetAuthGameMode<ATrainingGM>())
			{
				FRoom* RoomToClear = nullptr;
				
				if (GS->RoomData && GS->RoomData->ClearedRooms.Num() > 0)
				{
					if (bMissionSuccess) // continously check all the rooms
					{
						if (GS->RoomData->ClearedRooms.IsValidIndex(TrailerClearingIndex))
						{
							RoomToClear = GS->RoomData->ClearedRooms[TrailerClearingIndex];
							TrailerClearingIndex++;
						}
						else
						{
							TrailerClearingIndex = 0;
							RoomToClear = GS->RoomData->ClearedRooms[0];
							TrailerClearingIndex++;
						}
					}
					else
					{
						for (FRoom* Room : GS->RoomData->ClearedRooms)
						{
							if (!Room->bClearedByTrailers)
							{
								RoomToClear = Room;
								break;
							}
							//ULog::Info(Room->Name.ToString() + " cleared");
						}
					}
				}

				if (RoomToClear)
				{
					// is everyone out of this room?
					bool bEveryoneOut = true;
					for (ASWATCharacter* Swat : SwatAI)
					{
						if (FRoom* CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Swat->GetActorLocation()))
						{
							if (CurrentRoom == RoomToClear)
							{
								bEveryoneOut = false;
								break;
							}
						}
					}
					
					if (FRoom* CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(SquadLeader->GetActorLocation()))
					{
						if (CurrentRoom == RoomToClear)
						{
							bEveryoneOut = false;
						}
					}
					
					//ULog::Bool(bEveryoneOut, "Everyone Out: ");
					if (bEveryoneOut)
					{
						//ULog::Info("Room: " + RoomToClear->Name.ToString());

						bool bNoOneLookingAtSpawn = true;
						
						/*
						// any suspects active near spawn location?
						TArray<ACyberneticCharacter*> AllSusCiv = USuspectsAndCivilianManager::Get(this)->GetAllSuspectsAndCivilians();
						bool bAnyAIActive = false;
						for (ACyberneticCharacter* AI : AllSusCiv)
						{
							if (AI->IsActive())
							{
								bAnyAIActive = true;
								
								float ZHeight = FMath::Abs(RoomToClear->Location.Z - AI->GetActorLocation().Z);
								if (ZHeight < 200.0f)
								{
									if (FVector::Distance(AI->GetActorLocation(), FVector(RoomToClear->Location)) < 750.0f)
									{
										bNoOneLookingAtSpawn = false;
									}
								}
							}
						}

						if (bAnyAIActive)
						{
							for (ASWATCharacter* Swat : SwatAI)
							{
								if (FVector::DotProduct(Swat->GetActorForwardVector(), (FVector(RoomToClear->Location) - Swat->GetActorLocation()).GetSafeNormal()) > 0.25f &&
									Swat->HasLineOfSightTo(FVector(RoomToClear->Location)))
								{
									bNoOneLookingAtSpawn = false;
									break;
								}
							}
						}
						*/

						if (FVector::DotProduct(SquadLeader->GetActorForwardVector(), (FVector(RoomToClear->Location) - SquadLeader->GetActorLocation()).GetSafeNormal()) > 0.5f)
						{
							bNoOneLookingAtSpawn = false;
						}

						if (bNoOneLookingAtSpawn)
							RoomToClear->TimeNotLookingAtRoom += TickInterval;
						else
							RoomToClear->TimeNotLookingAtRoom = 0.0f;

						//ULog::Bool(bNoOneLookingAtSpawn, "No One Looking At Spawn: ");
						//LOG_NUMBER(TimeNotLookingAtRoom);
						
						if ((RoomToClear->TimeNotLookingAtRoom > 1.0f && !UReadyOrNotAISystem::WasRecentlyInCombat(5.0f)))
						{
							TArray<AActor*> Securables;
							Securables.Reserve(200);
							UGameplayStatics::GetAllActorsWithInterface(this, USecurable::StaticClass(), Securables);

							TArray<AActor*> SortedSecurables;
							SortedSecurables.Reserve(Securables.Num());
							
							for (AActor* Actor : Securables)
							{
								if (Cast<ACyberneticCharacter>(Actor) && !Cast<ATrailerSWATCharacter>(Actor))
								{
									SortedSecurables.AddUnique(Actor);
								}
							}
							
							for (AActor* Actor : Securables)
							{
								if (ABaseItem* Item = Cast<ABaseItem>(Actor))
								{
									if (Item->IsEvidence())
									{
										SortedSecurables.AddUnique(Actor);
									}
								}
								else if (Cast<AEvidenceActor>(Actor))
								{
									SortedSecurables.AddUnique(Actor);
								}
							}
							
							for (AActor* Actor : Securables)
							{
								if (Cast<ACollectedEvidenceActor>(Actor))
								{
									SortedSecurables.AddUnique(Actor);
								}
							}
							
							TArray<AActor*> FinalSecurables;
							FinalSecurables.Reserve(SortedSecurables.Num());

							for (AActor* Actor : SortedSecurables)
							{
								if (Actor)
								{
									const FRoom* SecurableRoomLocation = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(ISecurable::Execute_GetLocation(Actor));

									if (SecurableRoomLocation == RoomToClear)
									{
										if (ISecurable::Execute_CanBeSecuredByTrailers(Actor) && !Actor->IsHidden())
										{
											FinalSecurables.Add(Actor);
										}
									}
								}
							}
							
							if (FinalSecurables.Num() == 0)
							{
								RoomToClear->bClearedByTrailers = true;
							}
							else
							{
								uint8 NumActivated = 0;
								for (ATrailerSWATCharacter* Trailer : SwatTrailers)
								{
									if (Trailer && !Trailer->bDeactivated)
										NumActivated++;
								}

								if (FinalSecurables.Num() > 0 && (FinalSecurables.Num() > NumActivated || NumActivated == 0))
								{
									ATrailerSWATCharacter* FirstAvailableTrailer = nullptr;
									for (ATrailerSWATCharacter* Trailer : SwatTrailers)
									{
										if (Trailer->bDeactivated)
										{
											Trailer->bDeactivated = false;
											UReadyOrNotSignificanceManager::ForceActorRelevant(Trailer);
											UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(Trailer);
											Trailer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
											Trailer->GetCapsuleComponent()->SetEnableGravity(true);
											Trailer->SetActorLocation(FVector(RoomToClear->Location) + FVector(0.0f, 0.0f, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
											Trailer->SetActorHiddenInGame(false);
											Trailer->SetActorTickEnabled(true);
											
											FirstAvailableTrailer = Trailer;
											break;
										}
									}

									if (FirstAvailableTrailer)
									{
										RoomToClear->TimeNotLookingAtRoom = 0.0f;
										
										if (UTrailerSearchAndSecureActivity* Activity = FirstAvailableTrailer->GetCyberneticsController<ASWATController>()->GetTrailerSearchAndSecureActivity())
										{
											Activity->SearchingRoom = RoomToClear;
											Activity->AllSecurables = FinalSecurables;
											Activity->CommandLocation = FVector(RoomToClear->Location) + FVector(0.0f, 0.0f, 100.0f);
											Activity->SpawnLocation = OriginalSpawnLocation;
											Activity->OnFinishActivity.RemoveAll(this);
											Activity->OnFinishActivity.AddDynamic(this, &USWATManager::OnTrailerSearchComplete);
											
											UActivityManager::GiveActivityTo(Activity, FirstAvailableTrailer, true, true);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif
}

bool USWATManager::IsTickable() const
{
	return true;
}

bool USWATManager::PlaySpeechWithSharedCooldown(FString SpeechRowName, AReadyOrNotCharacter* Speaker, float Cooldown /*= 3.0f*/, FString OverrideSpeakerName /* = ""*/)
{
	if (SpeechCooldownMap.Contains(FName(SpeechRowName)))
		return false;
	
	if (SpeechRowName.IsEmpty())
		return false;
	
	if (!Speaker)
		return false;

	SpeechCooldownMap.Add(FName(SpeechRowName), Cooldown);
	return Speaker->PlayRawVO(SpeechRowName, OverrideSpeakerName);
}

ESwatCommand USWATManager::GetQueuedSwatCommandForSquadPosition(const ESquadPosition SquadPosition) const
{
	for (const auto& CommandQueue : QueuedSwatCommandMap)
	{
		if (CommandQueue.Key == ETeamType::TT_SQUAD && SquadPosition != ESquadPosition::SP_NONE)
		{
			return CommandQueue.Value.Command.CommandType;
		}

		if (CommandQueue.Key == ETeamType::TT_SERT_RED && (SquadPosition == ESquadPosition::SP_Charlie || SquadPosition == ESquadPosition::SP_Delta))
		{
			return CommandQueue.Value.Command.CommandType;
		}
		
		if (CommandQueue.Key == ETeamType::TT_SERT_BLUE && (SquadPosition == ESquadPosition::SP_Alpha || SquadPosition == ESquadPosition::SP_Beta))
		{
			return CommandQueue.Value.Command.CommandType;
		}
	}

	return ESwatCommand::SC_None;
}

bool USWATManager::IsSWATValid(ASWATCharacter* SWATCharacter) const
{
	if (!IsValid(SWATCharacter))
		return false;

	if (!SWATCharacter->GetCyberneticsController())
		return false;

	return true;
}

void USWATManager::RespondToPlayerTeamKill(AReadyOrNotCharacter* InstigatorCharacter, bool bTeleportBehind)
{
	if (SwatAI.Num() == 0)
	{
		return;
	}
	
	if (APlayerCharacter* InstigatorPlayerCharacter = Cast<APlayerCharacter>(InstigatorCharacter))
	{
		#if !UE_BUILD_SHIPPING
		if (CVarSwatKillTeamKiller.GetValueOnAnyThread() == 0)
		{
			return;
		}
		#endif
		
		const bool bInCombatState = UReadyOrNotAISystem::WasRecentlyInCombat(5.0f);

		if (!bInCombatState)
		{
			if (InstigatorPlayerCharacter->IsOnSWATTeam())
			{
				uint8 i = 0;
				for (ASWATCharacter* Swat : SwatAI)
				{
					if (IsSWATValid(Swat) && Swat->IsActive())
					{
						const bool bIsTrackingSuspect = Swat->GetCyberneticsController()->GetTrackedTarget() && Swat->GetCyberneticsController()->GetTrackedTarget()->IsSuspect();
						if (!bIsTrackingSuspect)
						{
							if (bTeleportBehind)
							{
								if (!Swat->HasLineOfSightToCharacter(InstigatorCharacter))
								{
									Swat->SetActorLocation(InstigatorCharacter->GetActorLocation() + InstigatorCharacter->GetActorRightVector().RotateAngleAxis(i * 45.0f, FVector::UpVector) * 200.0f);
									Swat->SetActorRotation((InstigatorCharacter->GetActorLocation() - Swat->GetActorLocation()).GetSafeNormal2D().Rotation());
									i++;
								}
							}

							InstigatorCharacter->GetHealthComponent()->SetResource(1.0f);
							InstigatorCharacter->GetHealthComponent()->SetUnlimitedResource(false);
							
							Swat->GetCyberneticsController()->GetTargetingComp()->AddKnownEnemy(InstigatorCharacter, true);
							Swat->GetCyberneticsController()->GetTargetingComp()->SetLastTrackedTarget(InstigatorPlayerCharacter);
							Swat->GetHealthComponent()->SetCurrentResourceToMax();
							Swat->GetHealthComponent()->SetUnlimitedResource(true);
							if (Swat->GetEquippedWeapon())
							{
								Swat->GetEquippedWeapon()->bInfiniteAmmo = true;
								Swat->GetEquippedWeapon()->SetMagazineCount(4, {});
								Swat->GetEquippedWeapon()->ReplenishAmmo();
							}

							Swat->GetCyberneticsController()->ClearActivityList();
							
							//Swat->GetCapsuleComponent()->SetCanEverAffectNavigation(false);
							// Swat->GetCyberneticsController()->GetRONPathFollowingComp()->SetCrowdSimulationState(ECrowdSimulationState::Disabled);
							//Swat->GetCapsuleComponent()->AreaClass = nullptr;
						}
					}
				}
			}
		}
	}
}

TArray<ASWATCharacter*> USWATManager::GetSWATSortedByDistanceToLocation(FVector Location, ETeamType FilterTeam, ADoor* StackUpDoor, bool bAscendingOrder)
{
	TArray<ASWATCharacter*> TempSWAT = SwatAI;
	TempSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!IsValid(Swat))
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
			
		return false;
	});

	TempSWAT.Sort([Location, bAscendingOrder](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
	{
		if (bAscendingOrder)
			return (Location - Lhs.GetActorLocation()).Size() < (Location - Rhs.GetActorLocation()).Size();
		
		return (Location - Lhs.GetActorLocation()).Size() > (Location - Rhs.GetActorLocation()).Size();
	});

	if (FilterTeam != ETeamType::TT_NONE && FilterTeam != ETeamType::TT_SQUAD)
	{
		TempSWAT.RemoveAll([&](ASWATCharacter* Swat)
		{
			if (!Swat)
				return true;
			
			return Swat->GetTeam() != FilterTeam;
		});
	}

	if (StackUpDoor)
	{
		bool bHasStackUpActivity = false;
		for (int32 i = 0; i < TempSWAT.Num(); i++)
		{
			ASWATCharacter* Swat = TempSWAT[i];
			if (Swat && Swat->GetCyberneticsController())
			{
				const UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>();
				if (StackUpActivity && (StackUpActivity->GetSharedData<FSharedStackUpData>()->StackUpDoor == StackUpDoor || StackUpDoor->GetSubDoor() == StackUpDoor))
				//if (StackUpActivity && (StackUpActivity->StackUpDoor == StackUpDoor || StackUpDoor->GetSubDoor() == StackUpDoor))
				{
					bHasStackUpActivity = true;
					break;
				}
			}
		}
	
		if (bHasStackUpActivity)
		{
			TempSWAT.Sort([&](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
			{
				if (Lhs.GetCyberneticsController() && Rhs.GetCyberneticsController())
				{
					const UTeamStackUpActivity* LhsStackUpActivity = Lhs.GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>();
					const UTeamStackUpActivity* RhsStackUpActivity = Rhs.GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>();
					if (LhsStackUpActivity && RhsStackUpActivity)
					{
						return LhsStackUpActivity->OverrideSquadPosition < RhsStackUpActivity->OverrideSquadPosition;
					}
				}
				
				return false;
			});
		}
	}

    TempSWAT.Remove(nullptr);
	return TempSWAT;
}

TArray<ASWATCharacter*> USWATManager::GetSWATSortedByDistanceToLocationV2(FVector Location, TArray<ASWATCharacter*> ExcludedSwat, ETeamType FilterTeam, bool bAscendingOrder)
{
	if (Location == FVector::ZeroVector)
		return {};
	
	TArray<ASWATCharacter*> SwatByTeam;
	
	ASWATCharacter* ClosestSwatToDoor = nullptr;
	float ClosestDistance = FLT_MAX;
	for (ASWATCharacter* swat : SwatAI)
	{
		if (CanGiveActivityToSWAT(swat, FilterTeam) && !ExcludedSwat.Contains(swat))
		{
			SwatByTeam.Add(swat);

			float Distance = FVector::Distance(swat->GetActorLocation(), Location);
			
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestSwatToDoor = swat;
			}
		}
	}

	TArray<ASWATCharacter*> SortedSWAT;
	{
		TArray<ASWATCharacter*> TempSwat = SwatByTeam;
		
		ASWATCharacter* ClosestSwat = ClosestSwatToDoor;
		SortedSWAT.AddUnique(ClosestSwat);
		TempSwat.Remove(ClosestSwat);

		if (ClosestSwat)
		{
			for (uint8 i = 0; i < SwatByTeam.Num(); i++)
			{
				// Get closest swat (with extra conditions)
				{
					ASWATCharacter* ClosestSwatTemp = nullptr;
					float ClosestDist = FLT_MAX;
					for (int32 j = 0; j < SwatByTeam.Num(); j++)
					{
						if (ClosestSwat != SwatByTeam[j] && !SortedSWAT.Contains(SwatByTeam[j]))
						{
							float tempDist = FVector::DistSquared(SwatByTeam[j]->GetActorLocation(), ClosestSwat->GetActorLocation());
							
							if (tempDist < ClosestDist)
							{
								ClosestDist = tempDist;
								ClosestSwatTemp = SwatByTeam[j];
							}
						}
					}
					
					ClosestSwat = ClosestSwatTemp;
				}
				
				if (!ClosestSwat)
				{
					SortedSWAT += TempSwat;
					break;
				}
				
				SortedSWAT.AddUnique(ClosestSwat);
				TempSwat.Remove(ClosestSwat);
			}
		}
	}

	if (!bAscendingOrder)
		Algo::Reverse(SortedSWAT);

	return SortedSWAT;
}

/*
bool USWATManager::GetSWATSortedByPathDistanceToLocation_Async(UObject* Context, TDelegate<void(const TArray<ASWATCharacter*>&)> Delegate, FVector Location, ETeamType FilterTeam)
{
	FSwatAsyncPathSortRequest Request;
	Request.bWaiting = true;
	Request.SuccessCallback = Delegate;

	for (ASWATCharacter* swat : SwatAI)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			if (const ANavigationData* NavData = NavSys->GetNavDataForProps(swat->GetMovementComponent()->GetNavAgentPropertiesRef()))
			{
				const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, swat->GetCyberneticsController()->GetNavQueryFilter());
				const FVector StartLocation = swat->GetNavAgentLocation();
				const FVector EndLocation = Location;
			
				FNavLocation StartLocationProjected(StartLocation);
				FNavLocation EndLocationProjected(EndLocation);

				NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(35.0f, 35.0f, 150.0f));
				NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(35.0f, 35.0f, 150.0f));

				FNavPathQueryDelegate NavDelegate;
				NavDelegate.BindUObject(this, &USWATManager::OnSwatSortAsyncPathFound);

				uint32 PathId = NavSys->FindPathAsync(swat->GetNavAgentPropertiesRef(), ACyberneticController::CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, true, swat->GetCyberneticsController()), NavDelegate, EPathFindingMode::Hierarchical);

				Request.PathQueries.Add(PathId, swat);
			}
		}
	}

	if (Request.PathQueries.Num() > 0)
	{
		PathSortRequests.Add(Context, Request);
		return true;
	}

	return false;
}
*/

TArray<ASWATCharacter*> USWATManager::GetSWATSortedByPathDistanceToLocation(FVector Location, TArray<ASWATCharacter*> ExcludedSwat, ETeamType FilterTeam, bool bAscendingOrder)
{
	if (Location == FVector::ZeroVector)
		return {};
	
	TArray<ASWATCharacter*> SwatByTeam;
	
	ASWATCharacter* ClosestSwatToDoor = nullptr;
	float ClosestDistance = FLT_MAX;
	for (ASWATCharacter* swat : SwatAI)
	{
		if (CanGiveActivityToSWAT(swat, FilterTeam) && !ExcludedSwat.Contains(swat))
		{
			SwatByTeam.Add(swat);

			float Distance = 0.0f;
			if (Internal_FindPath(Distance, Location, swat->GetNavAgentLocation(), FLT_MAX))
			{
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestSwatToDoor = swat;
				}
			}
		}
	}
	
	if (!ClosestSwatToDoor)
		return {};

	TArray<ASWATCharacter*> SortedSWAT;
	{
		TArray<ASWATCharacter*> TempSwat = SwatByTeam;
		
		ASWATCharacter* ClosestSwat = ClosestSwatToDoor;
		SortedSWAT.AddUnique(ClosestSwat);
		TempSwat.Remove(ClosestSwat);

		if (ClosestSwat)
		{
			for (uint8 i = 0; i < SwatByTeam.Num(); i++)
			{
				// Get closest swat (with extra conditions)
				{
					ASWATCharacter* ClosestSwatTemp = nullptr;
					float ClosestDist = FLT_MAX;
					for (int32 j = 0; j < SwatByTeam.Num(); j++)
					{
						if (ClosestSwat != SwatByTeam[j] && !SortedSWAT.Contains(SwatByTeam[j]))
						{
							float tempDist = 0.0f;
							
							if (Internal_FindPath(tempDist, Location, SwatByTeam[j]->GetNavAgentLocation(), FLT_MAX))
							{
								if (tempDist < ClosestDist)
								{
									ClosestDist = tempDist;
									ClosestSwatTemp = SwatByTeam[j];
								}
							}
						}
					}
					
					ClosestSwat = ClosestSwatTemp;
				}
				
				if (!ClosestSwat)
				{
					SortedSWAT += TempSwat;
					break;
				}
				
				SortedSWAT.AddUnique(ClosestSwat);
				TempSwat.Remove(ClosestSwat);
			}
		}
	}

	if (!bAscendingOrder)
		Algo::Reverse(SortedSWAT);

	return SortedSWAT;
}

bool USWATManager::IsCharacterKnownEnemy(AReadyOrNotCharacter* InCharacter) const
{
	for (ASWATCharacter* swat : SwatAI)
	{
		if (IsSWATValid(swat))
		{
			if (swat->GetCyberneticsController()->IsCharacterKnownEnemy(InCharacter))
				return true;
		}
	}

	return false;
}

bool USWATManager::IsSWATTeamDead(const ETeamType Team) const
{
	if (SwatAI.Num() == 0)
		return true;
	
	if (Team == ETeamType::TT_SQUAD || Team == ETeamType::TT_NONE)
	{
		bool bAnyGoldAlive = false;
		for (ASWATCharacter* swat : SwatAI)
		{
			if (IsSWATValid(swat) && !swat->IsDeadOrUnconscious())
			{
				bAnyGoldAlive = true;
			}
		}
		return !bAnyGoldAlive;
	}

	if (Team == ETeamType::TT_SERT_RED)
	{
		bool bAnyRedAlive = false;
		for (ASWATCharacter* swat : SwatAI)
		{
			if (IsSWATValid(swat) && swat->GetTeam() == ETeamType::TT_SERT_RED && !swat->IsDeadOrUnconscious())
			{
				bAnyRedAlive = true;
			}
		}

		return !bAnyRedAlive;
	}

	if (Team == ETeamType::TT_SERT_BLUE)
	{
		bool bAnyBlueAlive = false;
		for (ASWATCharacter* swat : SwatAI)
		{
			if (IsSWATValid(swat) && swat->GetTeam() == ETeamType::TT_SERT_BLUE && !swat->IsDeadOrUnconscious())
			{
				bAnyBlueAlive = true;
			}
		}

		return !bAnyBlueAlive;
	}
	
	return SwatAI.Num() == 0;
}

bool USWATManager::IsSWATTeamHoldingPosition(ETeamType Team) const
{
	if (SwatAI.Num() == 0)
		return false;
	
	if (Team == ETeamType::TT_SQUAD)
	{
		bool bAllHolding = true;
		for (ASWATCharacter* swat : SwatAI)
		{
			if (IsSWATValid(swat))
			{
				if (!swat->GetCyberneticsController()->GetCurrentActivity<UHoldActivity>())
				{
					bAllHolding = false;
					break;
				}
			}
		}

		return bAllHolding;
	}
	
	bool bAllHolding = true;
	for (ASWATCharacter* swat : SwatAI)
	{
		if (IsSWATValid(swat))
		{
			if (swat->GetTeam() == Team && !swat->GetCyberneticsController()->GetCurrentActivity<UHoldActivity>())
			{
				bAllHolding = false;
				break;
			}
		}
	}

	return bAllHolding;
}

void USWATManager::PlaySwatCommandVoiceLine(FString Voiceline, FString OverrideSpearkerName, const bool bTeamPrefix)
{
	if (!SquadLeader)
	{
		LOCAL_PLAYER;
		LocalPlayer->PlayRawVO(Voiceline, OverrideSpearkerName, false);
		return;
	}
	
	const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;
	if (!bIsSinglePlayer) // temp, as we do not have command VO for all swat ethnicities. Remove once they're supported
	{
		OverrideSpearkerName = "SwatJudge";
	}
	
	if (bTeamPrefix)
	{
		FString VO_Prefix = "";
		/* // idk kinda doesnt flow well
		switch (ActiveCommandTeam)
		{
			case ETeamType::TT_SERT_RED:		VO_Prefix = VO_SWAT_GENERAL::CALL_RED_TEAM; break;
			case ETeamType::TT_SERT_BLUE:		VO_Prefix = VO_SWAT_GENERAL::CALL_BLUE_TEAM; break;
			case ETeamType::TT_SQUAD:			VO_Prefix = VO_SWAT_GENERAL::CALL_GOLD_TEAM; break;
			default: break;
		}
		*/

		const bool bLastVOWasPrefix = SquadLeader->LastVoiceLinePlayed == VO_Prefix;
		
		if (!VO_Prefix.IsEmpty() && !bLastVOWasPrefix)
		{
			SquadLeader->PlayRawVO(VO_Prefix, OverrideSpearkerName, true);

			if (SquadLeader->VoiceSoundSource)
			{
				USWATManager* This = Get(this);
				
				SquadLeader->VoiceSoundSource->OnProgrammerSoundLengthReady.AddWeakLambda(This, [=](float Length)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_Prefix, This, FTimerDelegate::CreateUFunction(This, "PlaySwatCommandVoiceLine", Voiceline, OverrideSpearkerName, false), Length);
				});
				
				return;
			}
		}
	}
	
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_Prefix);
	SquadLeader->PlayRawVO(Voiceline, OverrideSpearkerName, false);
}

void USWATManager::GiveDeployNonLethalItemAtTargetCommand(AReadyOrNotCharacter* Target, ETeamType TeamType, EItemCategory Item)
{
	if (!Target)
		return;

	if (!Target->IsActive())
		return;

	FString ItemName = "";
	bool bValidItem = false;
	switch (Item)
	{
		case EItemCategory::IC_Taser:			bValidItem = true; ItemName = "Taser"; break;
		case EItemCategory::IC_OCSpray:			bValidItem = true; ItemName = "Pepper Spray"; break;
		case EItemCategory::IC_Pepperball:		bValidItem = true; ItemName = "Pepper Ball"; break;
		case EItemCategory::IC_Beanbag:			bValidItem = true; ItemName = "Beanbag"; break;
		case EItemCategory::IC_None:			bValidItem = true; ItemName = "Melee"; break;
		default: break;
	}

	if (!bValidItem)
		return;

	if (ASWATCharacter* Swat = GetClosestSWATToActorForTeamWithItem(Target, TeamType, Item))
	{
		if (const ASWATController* Controller = Swat->GetCyberneticsController<ASWATController>())
		{
			if (UEngageTargetLessLethalActivity* Activity = Controller->GetEngageLessLethalActivity())
			{
				//TODO (Max): ItemName to FText
				Activity->ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "EngageWithName"), FText::FromString(ItemName));
				Activity->TargetCharacter = Target;
				Activity->ItemType = Item;

				BindSWATActivityResponseVoiceLine(Swat, FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND : VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, Activity);
				
				UActivityManager::GiveActivityTo(Activity, Swat, true, false);
			}
		}
	}
}

void USWATManager::GiveBreachAndClearCommand(ADoor* Door, const EDoorBreachType DoorBreachType, const ETeamType TeamType, const FVector CommandLocation, TSubclassOf<ABaseItem> DoorBreachItemClass, TSubclassOf<ABaseItem> DoorUseItemClass, bool bWithLeader, bool bWithLeaderItem, bool bAutoClear, bool bLastAutoClear, EStackUpStyle CustomStackUpStyle)
{
	if (SwatAI.Num() == 0)
		return;
	
	if (!Door || CommandLocation == FVector::ZeroVector)
		return;
	
	ASWATCharacter* RespondingSWAT = GetClosestSWATToActorForTeam(SquadLeader, TeamType);

	if (Door->IsDoorwayOnly() && DoorBreachType != EDoorBreachType::Move)
	{
		RespondingSWAT->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (DoorBreachItemClass)
	{
		if (!GetSwatWithItem(TeamType, DoorBreachItemClass))
		{
			RespondingSWAT->PlayRawVO(UReadyOrNotFunctionLibrary::DoorBreachTypeToVoiceline_Negative(DoorBreachType));
			return;
		}
	}
	
	const FText DoorBreachTypeAsString = DoorBreachTypeToText(DoorBreachType);
	FText ActivityName;
	
	if (bWithLeaderItem)
	{
		ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "BreachTypeLeaderAndClear"), DoorBreachTypeAsString);
		DoorBreachItemClass = nullptr;
	}
	
	if (DoorBreachItemClass)
	{
		if (const ABaseItem* ItemObject = DoorBreachItemClass->GetDefaultObject<ABaseItem>())
		{
			if (DoorBreachItemClass != nullptr)
				ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "BreachTypeItemNameAndClear"), DoorBreachTypeAsString, ItemObject->ItemName);
			else
				ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "BreachTypeAndClear"), DoorBreachTypeAsString);
		}
	}
	// Otherwise, if opening a door or moving through a doorway
	else
	{
		if (!bWithLeaderItem)
			ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "BreachTypeAndClear"), DoorBreachTypeAsString);
	}
	
	if (!DoorUseItemClass)
	{
		switch (DoorBreachType)
		{
			case EDoorBreachType::Shotgun:		DoorUseItemClass = ABreachingShotgun::StaticClass(); break;
			case EDoorBreachType::Ram:			DoorUseItemClass = ADoorRam::StaticClass(); break;
			case EDoorBreachType::C2:			DoorUseItemClass = AC2Explosive::StaticClass(); break;
			case EDoorBreachType::Leader:		DoorUseItemClass = nullptr; break;
			default: break;
		}
	}

	const bool bIsCommandFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
	
	const EDoorRoomPosition DoorPosition_CommandSide_Opposite = Door->IsPointInFrontOfDoorway(CommandLocation) ? Door->BackRoomPosition : Door->FrontRoomPosition;
	const bool bAnyLeftStackPoints = bIsCommandFrontOfDoor ? Door->FrontRightStackUpPoints.Num() > 0 : Door->BackLeftStackUpPoints.Num() > 0;
	const bool bAnyRightStackPoints = bIsCommandFrontOfDoor ? Door->FrontLeftStackUpPoints.Num() > 0 : Door->BackRightStackUpPoints.Num() > 0;

	const bool bCanStackRight = bAnyRightStackPoints;
	const bool bCanStackLeft = bAnyLeftStackPoints;

	const bool bIsCenterFed = DoorPosition_CommandSide_Opposite == EDoorRoomPosition::Center || DoorPosition_CommandSide_Opposite == EDoorRoomPosition::Hallway;

	EStackUpStyle Style = CustomStackUpStyle;
	if (CustomStackUpStyle == EStackUpStyle::Auto)
	{
		if (bIsCenterFed && bCanStackLeft && bCanStackRight)
		{
			const bool bSubDoorOpen = Door->GetSubDoor() ? Door->GetSubDoor()->IsOpenBeyond(0.5f) : false;
			if (Door->IsOpenBeyond(0.5f) || Door->IsDoorwayOnly() || bSubDoorOpen)
			{
				
			}
			else
			{
				Style = EStackUpStyle::Split;
			}
		}
		else
		{
			if (bCanStackRight)
			{
				Style = EStackUpStyle::Right;
			}
			else if (bCanStackLeft)
			{
				Style = EStackUpStyle::Left;
			}
		}
	}

	const FGuid ActivityId = FGuid::NewGuid();

	const FSharedStackUpData* SharedStackUp = &SharedStackUpData[TeamType];
	FSharedBreachData* SharedData = &SharedBreachData[TeamType];
	
	if (SharedStackUp->StackUpDoor == Door)
	{
		SharedData->StackUpDoor = SharedStackUp->StackUpDoor;
		SharedData->StackUpSortedSwat = SharedStackUp->StackUpSortedSwat;
		SharedData->CommandTeam = SharedStackUp->CommandTeam;
		SharedData->StackUpStyle = SharedStackUp->StackUpStyle;
		SharedData->PreviousStackUpStyle = SharedStackUp->PreviousStackUpStyle;
		SharedData->bHasCheckedDoor = SharedStackUp->bHasCheckedDoor;
		SharedData->DoorCheckResult = SharedStackUp->DoorCheckResult;
		SharedData->StackUpStyle = SharedStackUp->StackUpStyle;
		SharedData->DoorToClose = SharedStackUp->DoorToClose;
		
		if (SharedStackUp->StackUpStyle != EStackUpStyle::Auto)
			Style = SharedStackUp->StackUpStyle;

		RespondingSWAT = Cast<ASWATCharacter>(SharedStackUp->DoorChecker);
	}
	
	bool bAnyNotBreaching = false;
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (SWAT->GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>() == nullptr)
			{
				bAnyNotBreaching = true;
				break;
			}
		}
	}
	
	bool bAnyOppositeSideOfDoor = false;
	if (SharedData->CommandLocation != FVector::ZeroVector)
	{
		for (ASWATCharacter* SWAT : SwatAI)
		{
			if (CanGiveActivityToSWAT(SWAT, TeamType))
			{
				const bool bSameSideAsCommand = bIsCommandFrontOfDoor == Door->IsActorInFrontOfDoorway(SWAT);
				if (!bSameSideAsCommand)
				{
					bAnyOppositeSideOfDoor = true;
					break;
				}
			}
		}
	}

	const bool bPreviousTeamCommandWasStackUp = CurrentSharedTeamData[TeamType] == SharedData || CurrentSharedTeamData[TeamType] == SharedStackUp;
	const bool bSameBreachType = DoorBreachType == SharedData->DoorBreachType && DoorUseItemClass == SharedData->DoorUseItemClass && DoorBreachItemClass == SharedData->DoorBreachItemClass;
	bool bStackOpposite = SharedData->StackUpDoor == Door && (SharedData->CommandLocation != FVector::ZeroVector && Door->IsPointInFrontOfDoorway(SharedData->CommandLocation) != bIsCommandFrontOfDoor) || bAnyOppositeSideOfDoor;
	const bool bNewDoor = !bPreviousTeamCommandWasStackUp || SharedData->StackUpDoor != Door || bAnyOppositeSideOfDoor || bStackOpposite || SharedData->CommandTeam != TeamType || SharedData->StackUpSortedSwat.Num() == 0;
	//if (bNewDoor)
		//bStackOpposite = false;
	
	const bool bIsOnFrontSideOfDoor = Door->IsPointInFrontOfDoorway(SharedData->CommandLocation);
	const bool bSameSideAsCommand = SharedData->CommandLocation != FVector::ZeroVector && (bIsCommandFrontOfDoor == bIsOnFrontSideOfDoor);
	
	if (!bNewDoor && bSameSideAsCommand && bSameBreachType && !bAnyNotBreaching)
	{
		return;
	}

	bool bOtherTeamCancelled = false;

	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (UTeamBreachAndClearActivity* BreachAndClearActivity = SWAT->GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
			{
				//if (bNewDoor)
				{
					BreachAndClearActivity->ResetData();
				}
			}
		}
		else
		{
			if (UTeamStackUpActivity* StackUpActivity = SWAT->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
			{
				if (SWAT->GetTeam() != TeamType && StackUpActivity->GetStackUpDoor() == Door)
				{
					SWAT->GetCyberneticsController()->FinishActivity(StackUpActivity, true, true);
					bOtherTeamCancelled = true;
				}
			}
		}
	}

	if (bOtherTeamCancelled)
	{
		if (TeamType == ETeamType::TT_SERT_RED)
		{
			GiveFallInCommand(ETeamType::TT_SERT_BLUE);
		}
		else if (TeamType == ETeamType::TT_SERT_BLUE)
		{
			GiveFallInCommand(ETeamType::TT_SERT_RED);
		}
	}
	
	if (bNewDoor)
	{
		SharedData->Reset();
	}
	
	if (!bAutoClear)
	{
		if (TeamType == ETeamType::TT_SQUAD)
		{
			bGoldAutoClearing = false;
		}
		else if (TeamType == ETeamType::TT_SERT_BLUE)
		{
			bBlueAutoClearing = false;
		}
		else if (TeamType == ETeamType::TT_SERT_RED)
		{
			bRedAutoClearing = false;
		}
	}

	SharedData->bIsAuto = bAutoClear;
	SharedData->bLastForAutoClear = bLastAutoClear;
	SharedData->ActivityId = ActivityId;
	SharedData->CommandLocation = CommandLocation;
	SharedData->CommandTeam = TeamType;
	SharedData->StackUpDoor = Door;
	SharedData->bNewStackUpDoor = bNewDoor;
	SharedData->bStackOppositeSide = false;//bStackOpposite;
	SharedData->StackUpStyle = Style;
	SharedData->DoorBreachType = DoorBreachType;
	SharedData->DoorBreachItemClass = DoorBreachItemClass;
	SharedData->DoorUseItemClass = DoorUseItemClass;
	SharedData->bIsLeaderBreach = bWithLeader;
	SharedData->bIsLeaderThrow = bWithLeaderItem;
	SharedData->ClearingSortedSwat.Empty();
	SharedData->DoorScanActivity = nullptr;
	SharedData->BreachCaller = nullptr;
	SharedData->ClearingTime = 0.0f;
	SharedData->BreachingTime = 0.0f;
	SharedData->DoorUser = nullptr;
	SharedData->DoorBreacher = nullptr;
	SharedData->DoorBreachActivity = nullptr;
	SharedData->DoorUseActivity = nullptr;
	SharedData->DoorScanActivity = nullptr;
	SharedData->DoorScanner = nullptr;
	SharedData->bIsBreaching = false;
	SharedData->bBreacherReady = false;
	SharedData->bHasBreacherBreached = false;
	SharedData->bHasLeaderBreached = false;
	SharedData->bHasUserBreached = false;
	SharedData->bHasChosenScanner = false;
	SharedData->bLeaderUsedItem = false;
	SharedData->bCalledOutLeftOpening = false;
	SharedData->bCalledOutRightOpening = false;
	SharedData->bCalledOutFrontOpening = false;
	SharedData->bCalledOutBorderPatrol = false;
	SharedData->NumInTeam = 0;
	SharedData->Assessment = EThresholdAssessment::None;
	SharedData->FirstEntryMethod = Style == EStackUpStyle::Split ? EEntryMethod::Flow : EEntryMethod::ButtonHook;
	
    const EDoorRoomPosition RoomPosition = bIsCommandFrontOfDoor ? Door->FrontRoomPosition : Door->BackRoomPosition;
	SharedData->StackingRoomPosition = RoomPosition;
	SharedData->BreachingRoomPosition = bIsCommandFrontOfDoor ? Door->BackRoomPosition : Door->FrontRoomPosition;
	
	const FName BreachingRoom = bIsCommandFrontOfDoor ? Door->GetBackThreatOwningRoom() : Door->GetFrontThreatOwningRoom();
	const FName StackingRoom = bIsCommandFrontOfDoor ? Door->GetFrontThreatOwningRoom() : Door->GetBackThreatOwningRoom();
	SharedData->Room = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(BreachingRoom);
	if (!SharedData->Room)
		return;
	SharedData->CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(StackingRoom);

	const bool bTeamHasShield = GetSwatWithItem(TeamType, ABallisticsShield::StaticClass()) != nullptr;

	const bool bWideDoorway = Door->IsDoorwayOnly() && Door->GetDoorSize().Y > 100.0f;
		
	if (Door->bHasFrame)
	{
		if (RoomPosition == EDoorRoomPosition::Center && !bWideDoorway && !DoorUseItemClass && !DoorBreachItemClass && !bWithLeaderItem && !bTeamHasShield)
			SharedData->Assessment = Style == EStackUpStyle::Split ? EThresholdAssessment::CenterCheck : EThresholdAssessment::Pie;
	}
	
	if (bWideDoorway)
	{
		SharedData->FirstEntryMethod = EEntryMethod::ButtonHook;
		SharedData->Assessment = EThresholdAssessment::None;
	}
	
	if (Door->GetSubDoor() || bWithLeader || bWithLeaderItem)
		SharedData->Assessment = EThresholdAssessment::None;
	
	//SharedData->Assessment = EThresholdAssessment::CenterCheck;
	
	// failsafe
	if (!bAutoClear)
	{
		Door->CurrentStackUpActivities.Empty();
		Door->DeactivateDoorBlocker();
		Door->DeactivateBreachBlockers();

		for (ADoor* DoorInRoom : SharedData->Room->AdditionalRootDoors)
		{
			if (DoorInRoom)
			{
				DoorInRoom->CurrentStackUpActivities.Empty();
				DoorInRoom->DeactivateDoorBlocker();
				DoorInRoom->DeactivateBreachBlockers();
			}
		}
		
		for (ADoor* DoorInRoom : SharedData->CurrentRoom->AdditionalRootDoors)
		{
			if (DoorInRoom)
			{
				DoorInRoom->CurrentStackUpActivities.Empty();
				DoorInRoom->DeactivateDoorBlocker();
				DoorInRoom->DeactivateBreachBlockers();
			}
		}
	}
	
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			SharedData->NumInTeam++;
		}
	}

	CurrentSharedTeamData[TeamType] = SharedData;

	/*
	const int32 NumStackupsOnSide = Door->IsPointInFrontOfDoorway(CommandLocation) ?
									Door->GetStackupsForArea(EStackupGenArea::SGA_FrontLeft).Num() + Door->GetStackupsForArea(EStackupGenArea::SGA_FrontRight).Num() :
									Door->GetStackupsForArea(EStackupGenArea::SGA_BackLeft).Num() + Door->GetStackupsForArea(EStackupGenArea::SGA_BackRight).Num();
	*/

	uint8 SquadPosition = 0;
	for (ASWATCharacter* SWAT : SwatAI)
	{
		//if (SquadPosition >= NumStackupsOnSide) // cause of bing chilling.... need to think of something else
			//break;
		
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (UTeamBreachAndClearActivity* BreachAndClearActivity = SWAT->GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
			{
				BreachAndClearActivity->SharedData = SharedData;
				BreachAndClearActivity->OverrideSquadPosition = (ESquadPosition)SquadPosition;
				
				BreachAndClearActivity->ActivityName = ActivityName;

				BreachAndClearActivity->bAbortMoveWhenActivityFinished = SWAT->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>() == nullptr;

				// transfer current stack up activity state over to the breach and clear activity
				if (SWAT->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>() == SWAT->GetCyberneticsController<ASWATController>()->GetStackUpActivity())
				{
					if (UTeamStackUpActivity* StackUpActivity = SWAT->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
					{
						if (StackUpActivity->GetStackUpDoor() == Door)
						{
							StackUpActivity->bAbortMoveWhenActivityFinished = false;
							StackUpActivity->Transfer(BreachAndClearActivity);
						}
					}
				}
				
				SquadPosition++;

				const bool bHasStackUpActivity = SWAT->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>() != nullptr;

				if (UActivityManager::GiveActivityTo(BreachAndClearActivity, SWAT, true, true))
				{
					BreachAndClearActivity->OnCleared.RemoveAll(this);
					BreachAndClearActivity->OnCleared.AddDynamic(this, &USWATManager::OnSwatFinishedClearing);
					
					if (RespondingSWAT == SWAT && !bAutoClear)
					{
						FString VO;
						if (bHasStackUpActivity)
						{
							VO = FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND : VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC;
						}
						else
						{
							VO = FMath::RandBool() ? (FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND : VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC) : VO_SWAT_GENERAL::RESPONSE_STACK_UP;
						}
						
						BindSWATActivityResponseVoiceLine(RespondingSWAT, VO, BreachAndClearActivity);
					}
				}
			}
		}
	}

	// Count number of times breach has been ordered, not bWithLeader
	// Achievements PINE_PAINBRINGER, WALNUT_WARRIOR, MAHOGANY_MASOCHIST
	if (!bWithLeader)
	{
		UAchievementStatics::IncreaseAchievementStat(GetWorld(), EAchievementStats::PROGRESS_BREACH, 1);
	}
}

void USWATManager::GiveMoveCommand(const ETeamType TeamType, const FVector CommandLocation)
{	
	if (SwatAI.Num() == 0)
		return;

	FSharedTeamData* SharedData = &SharedTeamData[TeamType];

	ASWATCharacter* RespondingSWAT = nullptr;

	uint8 SquadPosition = 0;
	FGuid ActivityId = FGuid::NewGuid();
	
	float Angle = 0.0f;
	const float AngleSpacing = 360.0f/(TeamType == ETeamType::TT_SQUAD ? 4 : 2);

	auto GenerateMoveToLocations = [&](TArray<FVector>& OutMoveToLocations)
	{
		for (int32 i = 0; i < (TeamType == ETeamType::TT_SQUAD ? 4 : 2); i++)
		{
			FHitResult Hit;
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Visibility);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
				
			FVector ModifiedCommandLocation = CommandLocation;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, CommandLocation, {CommandLocation.X, CommandLocation.Y, -100000.0f}, CollisionObjectQueryParams))
			{
				ModifiedCommandLocation = Hit.Location + Hit.ImpactNormal * 20.0f;
			}
			
			const FVector MoveToLocation = FVector(UReadyOrNotMathLibrary::CalculatePositionOnCircle({ModifiedCommandLocation.X, ModifiedCommandLocation.Y}, 100.0f, Angle), ModifiedCommandLocation.Z);

			OutMoveToLocations.Add(MoveToLocation);

			Angle += AngleSpacing;
		}
	};

	TArray<ASWATCharacter*> SortedSwat = GetSWATSortedByDistanceToLocation(CommandLocation, TeamType, nullptr, true);

	bool bAnyWithShieldOut = false;
	for (ASWATCharacter* Swat : SortedSwat)
	{
		if (Swat && Swat->GetEquippedItem<ABallisticsShield>())
		{
			bAnyWithShieldOut = true;
		}
	}
	
	if (bAnyWithShieldOut)
	{
		SortedSwat.Sort([](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
		{
			return Lhs.GetEquippedItem<ABallisticsShield>() != nullptr && Rhs.GetEquippedItem<ABallisticsShield>() == nullptr;
		});
	}
	
	TArray<FVector> MoveToLocations;
	GenerateMoveToLocations(MoveToLocations);

	TMap<FVector, ASWATCharacter*> MoveToMap;
	for (int32 z = 0; z < SortedSwat.Num(); z++)
	{
		ASWATCharacter* swat = SortedSwat[z];
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			float FurthestDistance = z == 0 ? 0.0f : BIG_DIST;
			FVector FurthestLocation = swat->GetActorLocation();
			for (int32 i = 0; i < (TeamType == ETeamType::TT_SQUAD ? 4 : 2); i++)
			{
				if (!MoveToMap.Contains(MoveToLocations[i]))
				{
					float Distance = FVector::Distance(swat->GetActorLocation(), MoveToLocations[i]);
					if ( z == 0 ? Distance > FurthestDistance : Distance < FurthestDistance)
					{
						FurthestDistance = Distance;
						FurthestLocation = MoveToLocations[i];
					}
				}
			}

			MoveToMap.Add(FurthestLocation, swat);
		}
	}

	SharedData->Reset();
	SharedData->ActivityId = ActivityId;
	SharedData->CommandTeam = TeamType;
	SharedData->CommandLocation = CommandLocation;
	
	CurrentSharedTeamData[TeamType] = SharedData;
	
	for (auto MoveData : MoveToMap)
	{
		ASWATCharacter* swat = MoveData.Value;
		FVector MoveToLocation = MoveData.Key;
		
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			FHitResult Hit;
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Visibility);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
			
			FVector ModifiedCommandLocation = CommandLocation;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, CommandLocation, {CommandLocation.X, CommandLocation.Y, -100000.0f}, CollisionObjectQueryParams))
			{
				ModifiedCommandLocation = Hit.Location + Hit.ImpactNormal * 20.0f;
			}
			
			bool bHitFloor = false;

			if (GetWorld()->LineTraceTestByObjectType(MoveToLocation, {MoveToLocation.X, MoveToLocation.Y, -100000.0f}, CollisionObjectQueryParams))
			{
				bHitFloor = true;
			}
			
			AThreatAwarenessActor* NearestThreat = UThreatAwarenessSubsystem::Get(this)->GetNearestThreatForLocation(ModifiedCommandLocation, 2000.0f, 200.0f, true, {EThreatLevel::TL_Extreme});
			FVector ModifiedNearestThreatLocation = MoveToLocation;
			if (NearestThreat)
			{
				for (int32 i = 0; i < NearestThreat->PathableThreatAwarenessActors.Num(); i++)
				{
					// Threat awareness actors should always be valid
					if (!IsValid(NearestThreat->PathableThreatAwarenessActors[i]))
						continue;
					
					// make sure no other squad members have the same threat point
					bool bBadLoc = false;
					for (TObjectIterator<UMoveActivity>It; It; ++It)
					{
						UMoveActivity* MoveIt = *It;
						if (!MoveIt->SharedData)
							continue;
						
						if (ActivityId != MoveIt->SharedData->ActivityId)
						{
							continue;
						}
								
						if ((MoveIt->GetLocation() - NearestThreat->PathableThreatAwarenessActors[i]->GetActorLocation()).Size2D() < 150.0f)
						{
							bBadLoc = true;
							break;
						}
					}
					
					if (!bBadLoc)
					{
						NearestThreat = NearestThreat->PathableThreatAwarenessActors[i];
						ModifiedNearestThreatLocation = {NearestThreat->GetActorLocation().X, NearestThreat->GetActorLocation().Y, ModifiedCommandLocation.Z};
						break;
					}
				}
			}

			if (UMoveActivity* MoveActivity = swat->GetCyberneticsController<ASWATController>()->GetMoveActivity())
			{
				MoveActivity->SharedData = SharedData;
				MoveActivity->ActivityStartDelay = (float)SquadPosition * 0.5f;
				MoveActivity->OverrideSquadPosition = (ESquadPosition)SquadPosition;
				SquadPosition++;
				
				if (GetWorld()->LineTraceTestByObjectType(ModifiedCommandLocation, MoveToLocation, CollisionObjectQueryParams))
				{
					#if WITH_EDITOR
					ULog::Info("[Move To Command] Cannot move swat " + RON_ENUM_TO_STRING(ESquadPosition, swat->GetSquadPosition()) + "to target location. Finding nearest threat awareness actor...");
					#endif

					MoveActivity->SetLocation(ModifiedNearestThreatLocation);
					//MoveActivity->CommandLocation = CommandLocation;
				}
				else
				{
					if (!bHitFloor)
					{
						if (NearestThreat)
						{
							MoveActivity->SetLocation(ModifiedNearestThreatLocation);
							//MoveActivity->CommandLocation = CommandLocation;
						}
					}
					else
					{
						MoveActivity->SetLocation(MoveToLocation);
						//MoveActivity->CommandLocation = CommandLocation;
					}
				}

				if (!RespondingSWAT)
				{
					RespondingSWAT = swat;
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_MOVE_TO, MoveActivity);	
				}
				
				UActivityManager::GiveActivityTo(MoveActivity, swat, true, true);
				
				//Delay += 0.15f;
			}
		}
	}
}

void USWATManager::GiveDeployGrenadeAtLocation(ETeamType TeamType, FVector CommandLocation, TSubclassOf<ABaseGrenade> Grenade)
{
	if (!Grenade)
		return;

	if (GetSWATCount() == 0)
		return;
	
	const FString ItemName = Grenade->GetDefaultObject<ABaseItem>()->ItemName.ToString();
	
	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(CommandLocation, TeamType);

	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!Swat)
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UDeployItemAtLocationActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;
	
	ASWATCharacter* Thrower = GetSwatWithItem(TeamType, Grenade, SortedSWAT);
	if (Thrower)
	{
		if (UDeployGrenadeAtLocationActivity* DeployGrenadeAtLocation = Thrower->GetCyberneticsController<ASWATController>()->GetDeployGrenadeAtLocationActivity())
		{
			DeployGrenadeAtLocation->DeployItemClass = Grenade;
			DeployGrenadeAtLocation->DeployLocation = CommandLocation;
			UActivityManager::GiveActivityTo(DeployGrenadeAtLocation, Thrower, true, false);
		}
	}
	else
	{
		const TArray<EItemCategory> ItemCategories = Grenade->GetDefaultObject<ABaseItem>()->ItemCategories;
		
		if (ItemCategories.Contains(EItemCategory::IC_Flashbang))
			GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_FLASHBANG);
		else if (ItemCategories.Contains(EItemCategory::IC_CSGas))
			GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_CSGAS);
		else if (ItemCategories.Contains(EItemCategory::IC_Stingball))
			GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_STINGER);
		else
			GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
	}
}

void USWATManager::GiveDropChemlightAtLocation(ETeamType TeamType, FVector CommandLocation)
{
	if (GetSWATCount() == 0)
		return;
	
	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(CommandLocation, TeamType);

	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!Swat)
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UDeployItemAtLocationActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;
	
	ASWATCharacter* Thrower = GetSwatWithItem(TeamType, AChemlight::StaticClass(), SortedSWAT);
	if (Thrower)
	{
		if (UDeployChemlightActivity* DeployChemlightActivity = Thrower->GetCyberneticsController<ASWATController>()->GetDeployChemlightActivity())
		{
			DeployChemlightActivity->DeployLocation = CommandLocation;
			UActivityManager::GiveActivityTo(DeployChemlightActivity, Thrower, true, false);
		}
	}
	else
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_CHEMLIGHT);
	}
}

void USWATManager::GiveDeployShield(ETeamType TeamType)
{
	// eww.. refactor this
	
	bool bHasEveryGotShieldOut = true;
	for (TActorIterator<ASWATCharacter>It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD)
		{
			if (!It->GetInventoryComponent()->HasAnyInventoryItemsOfClass(ABallisticsShield::StaticClass()))
				continue;
			
			if (!It->GetEquippedItem() || !It->GetEquippedItem()->IsA(ABallisticsShield::StaticClass()))
			{
				bHasEveryGotShieldOut = false;
			}
		}
	}  
	for (TActorIterator<ASWATCharacter>It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD)
		{
			if (bHasEveryGotShieldOut)
			{
				It->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Primary);
			} else
			{
				It->GetInventoryComponent()->EquipItemOfClass(ABallisticsShield::StaticClass());
			}
		}
	}
}

void USWATManager::GiveStackUpCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal, bool bCheckDoor /*= false*/, EStackUpStyle StackUpStyle)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (SwatAI.Num() == 0)
		return;

	const bool bIsCommandFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
	
	const EDoorRoomPosition DoorPosition_CommandSide_Opposite = Door->IsPointInFrontOfDoorway(CommandLocation) ? Door->BackRoomPosition : Door->FrontRoomPosition;
	const bool bAnyLeftStackPoints = bIsCommandFrontOfDoor ? Door->FrontRightStackUpPoints.Num() > 0 : Door->BackLeftStackUpPoints.Num() > 0;
	const bool bAnyRightStackPoints = bIsCommandFrontOfDoor ? Door->FrontLeftStackUpPoints.Num() > 0 : Door->BackRightStackUpPoints.Num() > 0;

	const bool bCanStackRight = bAnyRightStackPoints;
	const bool bCanStackLeft = bAnyLeftStackPoints;

	const bool bIsCenterFed = DoorPosition_CommandSide_Opposite == EDoorRoomPosition::Center;

	if (StackUpStyle == EStackUpStyle::Auto)
	{
		if (bIsCenterFed && bCanStackLeft && bCanStackRight)
		{
			const bool bSubDoorOpen = Door->GetSubDoor() ? Door->GetSubDoor()->IsOpenBeyond(0.5f) : false;
			if (Door->IsOpenBeyond(0.5f) || Door->IsDoorwayOnly() || bSubDoorOpen)
			{
				
			}
			else
			{
				StackUpStyle = EStackUpStyle::Split;
			}
		}
		else
		{
			if (bCanStackRight)
			{
				StackUpStyle = EStackUpStyle::Right;
			}
			else if (bCanStackLeft)
			{
				StackUpStyle = EStackUpStyle::Left;
			}
		}
	}

	const FGuid ActivityId = FGuid::NewGuid();

	FSharedStackUpData* SharedData = &SharedStackUpData[TeamType];

	bool bOtherTeamCancelled = false;

	bool bAnyNotStackedUp = false;
	bool bAnyBreaching = false;
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (SWAT->GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>())
			{
				bAnyBreaching = true;
			}
			
			if (SWAT->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>() == nullptr)
			{
				bAnyNotStackedUp = true;
				break;
			}
		}
	}

	bool bAnyOppositeSideOfDoor = false;
	if (SharedData->CommandLocation != FVector::ZeroVector)
	{
		for (ASWATCharacter* SWAT : SwatAI)
		{
			if (CanGiveActivityToSWAT(SWAT, TeamType))
			{
				const bool bSameSideAsCommand = bIsCommandFrontOfDoor == Door->IsActorInFrontOfDoorway(SWAT);
				if (!bSameSideAsCommand)
				{
					bAnyOppositeSideOfDoor = true;
					break;
				}
			}
		}
	}
	
	const bool bSameStackUpStyle = StackUpStyle == SharedData->StackUpStyle;
	const bool bStackOpposite = SharedData->StackUpDoor == Door && (SharedData->CommandLocation != FVector::ZeroVector && Door->IsPointInFrontOfDoorway(SharedData->CommandLocation) != bIsCommandFrontOfDoor) || bAnyOppositeSideOfDoor;
	const bool bNewDoor = SharedData->StackUpDoor != Door || bAnyNotStackedUp || bStackOpposite || SharedData->CommandTeam != TeamType;
	
	const bool bIsOnFrontSideOfDoor = Door->IsPointInFrontOfDoorway(SharedData->CommandLocation);
	const bool bSameSideAsCommand = SharedData->CommandLocation != FVector::ZeroVector && (bIsCommandFrontOfDoor == bIsOnFrontSideOfDoor);
	if (!bNewDoor && bSameSideAsCommand && bSameStackUpStyle && !bAnyBreaching)
	{
		return;
	}

	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (UTeamStackUpActivity* TeamStackUpActivity = SWAT->GetCyberneticsController<ASWATController>()->GetStackUpActivity())
			{
				TeamStackUpActivity->ResetData();
			}
		}
		else
		{
			if (UTeamStackUpActivity* CurrentStackUpActivity = SWAT->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
			{
				if (SWAT->GetTeam() != TeamType && CurrentStackUpActivity->GetStackUpDoor() == Door)
				{
					SWAT->GetCyberneticsController()->FinishActivity(CurrentStackUpActivity, true, true);
					bOtherTeamCancelled = true;
				}
			}
		}
	}

	if (bOtherTeamCancelled)
	{
		if (TeamType == ETeamType::TT_SERT_RED)
		{
			GiveFallInCommand(ETeamType::TT_SERT_BLUE);
		}
		else if (TeamType == ETeamType::TT_SERT_BLUE)
		{
			GiveFallInCommand(ETeamType::TT_SERT_RED);
		}
	}

	if (bNewDoor)
	{
		SharedData->Reset();
	}
	
	SharedData->ActivityId = ActivityId;
	SharedData->CommandLocation = CommandLocation;
	SharedData->CommandTeam = TeamType;
	SharedData->StackUpDoor = Door;
	SharedData->StackUpStyle = StackUpStyle;
	SharedData->bNewStackUpDoor = bNewDoor;
	SharedData->bStackOppositeSide = bStackOpposite;
	SharedData->StackingRoomPosition = bIsCommandFrontOfDoor ? Door->FrontRoomPosition : Door->BackRoomPosition;
	SharedData->NumInTeam = 0;
	
	const FName BreachingRoom = bIsCommandFrontOfDoor ? Door->GetBackThreatOwningRoom() : Door->GetFrontThreatOwningRoom();
	SharedData->Room = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(BreachingRoom);
	
	const FName StackingRoom = bIsCommandFrontOfDoor ? Door->GetFrontThreatOwningRoom() : Door->GetBackThreatOwningRoom();
	SharedData->CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(StackingRoom);
	
	Door->DeactivateDoorBlocker();
	Door->DeactivateBreachBlockers();

	if (SharedData->Room)
	{
		for (ADoor* DoorInRoom : SharedData->Room->AdditionalRootDoors)
		{
			if (DoorInRoom)
			{
				DoorInRoom->DeactivateDoorBlocker();
				DoorInRoom->DeactivateBreachBlockers();
			}
		}
	}
	
	if (SharedData->CurrentRoom)
	{
		for (ADoor* DoorInRoom : SharedData->CurrentRoom->AdditionalRootDoors)
		{
			if (DoorInRoom)
			{
				DoorInRoom->DeactivateDoorBlocker();
				DoorInRoom->DeactivateBreachBlockers();
			}
		}
	}
	
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			SharedData->NumInTeam++;
		}
	}
	
	CurrentSharedTeamData[TeamType] = SharedData;
	
	const int32 NumStackupsOnSide = Door->IsPointInFrontOfDoorway(CommandLocation) ?
									Door->GetStackupsForArea(EStackupGenArea::SGA_FrontLeft).Num() + Door->GetStackupsForArea(EStackupGenArea::SGA_FrontRight).Num() :
									Door->GetStackupsForArea(EStackupGenArea::SGA_BackLeft).Num() + Door->GetStackupsForArea(EStackupGenArea::SGA_BackRight).Num();

	uint8 SquadPosition = 0;
	ASWATCharacter* RespondingSWAT = GetClosestSWATToActorForTeam(SquadLeader, TeamType);
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (SquadPosition >= NumStackupsOnSide)
			break;
		
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			if (UTeamStackUpActivity* TeamStackUpActivity = SWAT->GetCyberneticsController<ASWATController>()->GetStackUpActivity())
			{
				TeamStackUpActivity->bAbortMoveWhenActivityFinished = bNewDoor;
				TeamStackUpActivity->SharedData = SharedData;
				TeamStackUpActivity->OverrideSquadPosition = (ESquadPosition)SquadPosition;
				SquadPosition++;

				const bool bHasStackUpActivity = SWAT->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>() != nullptr;
				
				if (UActivityManager::GiveActivityTo(TeamStackUpActivity, SWAT, true, true))
				{
					if (RespondingSWAT == SWAT)
					{
						FString VO;
						if (bHasStackUpActivity)
						{
							VO = FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND : VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC;
						}
						else
						{
							VO = FMath::RandBool() ? (FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND : VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC) : VO_SWAT_GENERAL::RESPONSE_STACK_UP;
						}
						
						BindSWATActivityResponseVoiceLine(RespondingSWAT, VO, TeamStackUpActivity);
					}
				}
			}
		}
	}
}

ASWATCharacter* USWATManager::GetSwatCharacterAtSquadPosition(const ESquadPosition InSquadPosition) const
{
	for (ASWATCharacter* Swat : SwatAI)
	{
		if (!Swat || !Swat->GetCyberneticsController())
			continue;

		if (Swat->GetSquadPosition() == InSquadPosition)
			return Swat;
	}
	
	return nullptr;
}

FVector USWATManager::GetAverageSwatLocation() const
{
	if (SwatAI.Num() == 0)
		return FVector::ZeroVector;
	
	FVector SwatLocationSum = FVector::ZeroVector;

	for (ASWATCharacter* swat : SwatAI)
	{
		if (!swat)
			continue;

		SwatLocationSum += swat->GetActorLocation();
	}

	return SwatLocationSum/SwatAI.Num();
}

ASWATCharacter* USWATManager::GetClosestSWATToActor(AActor* TestActor)
{
	if (!TestActor)
		return nullptr;

	float ClosestDist = FLT_MAX;
	ASWATCharacter* ClosestSWAT = nullptr;
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && SwatAI[i]->GetCyberneticsController() && TestActor != SwatAI[i])
		{
			float tempDist = (SwatAI[i]->GetActorLocation() - TestActor->GetActorLocation()).SizeSquared();
			if (tempDist < ClosestDist)
			{
				ClosestDist = tempDist;
				ClosestSWAT = SwatAI[i];

			}
		}
	}
	
	return ClosestSWAT;
}

ASWATCharacter* USWATManager::GetClosestSWATToActorForTeam(AActor* TestActor, ETeamType Team)
{
	if (!TestActor)
		return nullptr;

	float ClosestDist = FLT_MAX;
	ASWATCharacter* ClosestSWAT = nullptr;
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && SwatAI[i]->GetCyberneticsController() && TestActor != SwatAI[i] && (SwatAI[i]->GetTeam() == Team || Team == ETeamType::TT_SQUAD))
		{
			float tempDist = (SwatAI[i]->GetActorLocation() - TestActor->GetActorLocation()).Size();
			if (tempDist < ClosestDist)
			{
				ClosestDist = tempDist;
				ClosestSWAT = SwatAI[i];
			}
		}
	}
	
	return ClosestSWAT;
}

ASWATCharacter* USWATManager::GetClosestSWATToActorForTeamWithItem(AActor* TestActor, ETeamType Team, EItemCategory ItemType)
{
	if (!TestActor)
		return nullptr;
	
	if (ItemType == EItemCategory::IC_None)
	{
		return GetClosestSWATToActorForTeam(TestActor, Team);
	}

	float ClosestDist = FLT_MAX;
	ASWATCharacter* ClosestSWAT = nullptr;
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] &&
			SwatAI[i]->GetCyberneticsController() &&
			TestActor != SwatAI[i] &&
			(SwatAI[i]->GetTeam() == Team || Team == ETeamType::TT_SQUAD) &&
			SwatAI[i]->GetInventoryComponent()->GetInventoryItemOfType(ItemType))
		{
			float tempDist = (SwatAI[i]->GetActorLocation() - TestActor->GetActorLocation()).SizeSquared();
			if (tempDist < ClosestDist)
			{
				ClosestDist = tempDist;
				ClosestSWAT = SwatAI[i];
			}
		}
	}
	
	return ClosestSWAT;
}

class ASWATCharacter* USWATManager::GetClosestSWATToLocation(FVector TestLocation)
{
	if (TestLocation == FVector::ZeroVector)
		return nullptr;

	float ClosestDist = FLT_MAX;
	ASWATCharacter* ClosestSWAT = nullptr;
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && SwatAI[i]->GetCyberneticsController())
		{
			float tempDist = (SwatAI[i]->GetActorLocation() - TestLocation).Size();
			if (tempDist < ClosestDist)
			{
				ClosestDist = tempDist;
				ClosestSWAT = SwatAI[i];
			}
		}
	}
	
	return ClosestSWAT;
}

ASWATCharacter* USWATManager::GetClosestSWATInSightToActor(AActor* TestActor)
{
	if (!TestActor)
		return nullptr;

	float ClosestDist = FLT_MAX;
	ASWATCharacter* ClosestSWAT = nullptr;
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && SwatAI[i]->GetCyberneticsController() && !SwatAI[i]->IsDeadOrUnconscious() && TestActor != SwatAI[i] && SwatAI[i]->GetCyberneticsController()->LineOfSightTo(TestActor))
		{
			float tempDist = (SwatAI[i]->GetActorLocation() - TestActor->GetActorLocation()).Size();
			if (tempDist < ClosestDist)
			{
				ClosestDist = tempDist;
				ClosestSWAT = SwatAI[i];

			}
		}
		
	}
	return ClosestSWAT;
}

void USWATManager::GiveFallInCommand(const ETeamType TeamType, EFallInPattern FallInPattern)
{
	if (GetSwatTeam().Num() == 0)
		return;
	
	const FGuid ActivityId = FGuid::NewGuid();

	FSharedFallInData* SharedData = &SharedFallInData[TeamType];
	SharedData->Reset();
	SharedData->ActivityId = ActivityId;
	SharedData->CommandTeam = TeamType;
	SharedData->NumInTeam = 0;
	SharedData->Pattern = FallInPattern;
	
	CurrentSharedTeamData[TeamType] = SharedData;

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(SquadLeader->GetActorLocation(), TeamType);

	ASWATCharacter* RespondingSWAT = SortedSWAT[0];
	uint8 SquadPosition = 0;
	
	for (ASWATCharacter* SWAT : SwatAI)
	{
		if (SWAT == SquadLeader)
			continue;
		
		if (CanGiveActivityToSWAT(SWAT, TeamType))
		{
			SharedData->NumInTeam++;
		}
	}
	
	for (ASWATCharacter* swat : SortedSWAT)
	{
		if (swat == SquadLeader)
			continue;

		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			if (UTeamFallinActivity* TeamFallinActivity = swat->GetCyberneticsController<ASWATController>()->GetFallinActivity())
			{
				TeamFallinActivity->SharedData = SharedData;
				TeamFallinActivity->OverrideSquadPosition = (ESquadPosition)SquadPosition;
				SquadPosition++;

				if (RespondingSWAT == swat)
				{
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_FALL_IN, TeamFallinActivity);
				}

				UActivityManager::GiveActivityTo(TeamFallinActivity, swat, true, true);
				
				bGivenInitialFallInCommand = true;
			}
		}
	}
}

void USWATManager::GiveCheckForContactsCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;
	
	if (GetSWATCount() == 0)
		return;

	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<UMirrorUnderDoorActivity, AOptiwand>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_MIRRORGUN))
	{
		UMirrorUnderDoorActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetMirrorUnderDoorActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "MirrorUnderDoor");
		DoorActivity->MirrorContactType = EMirrorContactType::Both;
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveCheckForTrapsCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal)
{	
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;
	
	if (Door->IsOpen() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}
	
	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<UMirrorUnderDoorActivity, AOptiwand>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_MIRRORGUN))
	{
		UMirrorUnderDoorActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetMirrorUnderDoorActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "MirrorForTraps");
		DoorActivity->MirrorContactType = EMirrorContactType::Trap;
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveDisarmTrapOnDoorCommand(AActor* Target, const ETeamType TeamType, const FVector CommandLocation)
{
	ADoor* Door;
	if (const ATrapActorAttachedToDoor* Trap = Cast<ATrapActorAttachedToDoor>(Target))
	{
		Door = Trap->AttachedToDoor;
	}
	else
	{
		Door = Cast<ADoor>(Target);
	}

	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (!Door->GetAttachedTrap())
		return;

	if (Door->GetAttachedTrap()->TrapStatus != ETrapState::TS_Live)
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::CALL_TRAP_DISARMED);
		return;
	}

	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<UDisarmDoorTrapActivity, AMultitool>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC))
	{
		UDisarmDoorTrapActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetDisarmDoorTrapActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "DisarmTrap");
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::CALL_DISARM_TRAP, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveWedgeDoorCommand(AActor* Target, const ETeamType TeamType, const FVector CommandLocation, FVector CommandNormal)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (Door->IsOpen() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (Door->IsJammed())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}
	
	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<UDeployWedgeActivity, ADoorJam>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_DOOR_JAM))
	{
		UDeployWedgeActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetDeployWedgeActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "WedgeDoor");
		DoorActivity->bRemoveWedge = false;
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::RESPONSE_DOOR_JAM, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveCloseDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation)
{	
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (Door->IsClosed() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (ASWATCharacter* Swat = FindSwatForDoorInteraction<UToggleDoorActivity>(Door, TeamType))
	{
		UToggleDoorActivity* DoorActivity = Swat->GetCyberneticsController<ASWATController>()->GetToggleDoorActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = Swat->GetNavAgentLocation();
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "CloseDoor");
		DoorActivity->bOpenDoor = false;
		
		BindSWATActivityResponseVoiceLine(Swat, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, Swat, true, false);
	}
}

void USWATManager::GiveOpenDoorCommand(AActor* Target, const ETeamType TeamType, const FVector CommandLocation)
{	
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (!Door->CanOpenDoor() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (ASWATCharacter* Swat = FindSwatForDoorInteraction<UToggleDoorActivity>(Door, TeamType))
	{
		UToggleDoorActivity* DoorActivity = Swat->GetCyberneticsController<ASWATController>()->GetToggleDoorActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = Swat->GetNavAgentLocation();
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "OpenDoor");
		DoorActivity->bOpenDoor = true;
		
		BindSWATActivityResponseVoiceLine(Swat, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, Swat, true, false);
	}
}

void USWATManager::GiveScanDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, EDoorScanMethod ScanMethod)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	// If someone is already using this door, don't give another activity
	if (UActivityManager::AnyAIHasActivity<UScanDoorActivity>([&](const UScanDoorActivity* Activity)
	{
		return Activity->Door == Door;
	}))
	{
		return;
	}

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocationV2(Door->GetDoorMidLocation(), {}, TeamType);
	
	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (const UBaseActivity* CurrentActivity = Swat->GetCyberneticsController()->GetCurrentActivity())
		{
			if (!CurrentActivity->CanOverrideActivity())
				return true;
		}
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UScanDoorActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;

	if (ASWATCharacter* Swat = SortedSWAT[0])
	{
		if (Swat->GetCyberneticsController<ASWATController>())
		{
			if (UScanDoorActivity* DoorActivity = Swat->GetCyberneticsController<ASWATController>()->GetScanDoorActivity())
			{
				DoorActivity->Door = Door;
				DoorActivity->ScanMethod = ScanMethod;
				DoorActivity->CommandLocation = CommandLocation;
				
				BindSWATActivityResponseVoiceLine(Swat, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, DoorActivity);

				UActivityManager::GiveActivityTo(DoorActivity, Swat, true, false);
			}
		}
	}
}

void USWATManager::GiveSearchAndSecureCommand(ETeamType TeamType, FVector CommandLocation, bool bOnlyCurrentRoom)
{
	if (SwatAI.Num() == 0)
		return;

	ASWATCharacter* RespondingSWAT = nullptr;
	
	const FRoom* Room = nullptr;
	if (bOnlyCurrentRoom)
		Room = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(CommandLocation);
	
	for (ASWATCharacter* swat : SwatAI)
	{
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			if (USearchAndSecureActivity* SearchAndSecureActivity = swat->GetCyberneticsController<ASWATController>()->GetSearchAndSecureActivity())
			{
				SearchAndSecureActivity->ResetData();
				SearchAndSecureActivity->SearchingRoom = Room;
				SearchAndSecureActivity->bAuto = false;
				
				if (!RespondingSWAT)
				{
					RespondingSWAT = swat;
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, SearchAndSecureActivity);
				}

				UActivityManager::GiveActivityTo(SearchAndSecureActivity, swat, true, false);
			}
		}
	}
}

void USWATManager::GiveSearchAndSecureCommand_Individual(AActor* Target, FVector CommandLocation, bool bOnlyCurrentRoom)
{
	if (SwatAI.Num() == 0)
		return;

	const FRoom* Room = nullptr;
	if (bOnlyCurrentRoom)
		Room = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(CommandLocation);
	
	for (ASWATCharacter* swat : SwatAI)
	{
		if (swat == Target && swat->GetCyberneticsController() && !swat->IsDeadOrUnconscious())
		{
			if (USearchAndSecureActivity* SearchAndSecureActivity = swat->GetCyberneticsController<ASWATController>()->GetSearchAndSecureActivity())
			{
				SearchAndSecureActivity->ResetData();
				SearchAndSecureActivity->SearchingRoom = Room;
				SearchAndSecureActivity->bAuto = false;
				
				BindSWATActivityResponseVoiceLine(swat, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, SearchAndSecureActivity);

				UActivityManager::GiveActivityTo(SearchAndSecureActivity, swat, true, false);
			}
			
			break;
		}
	}
}

void USWATManager::GiveHoldCommand(const ETeamType TeamType)
{
	ASWATCharacter* RespondingSWAT = nullptr;

	FGuid ActivityId = FGuid::NewGuid();
	
	FSharedTeamData* SharedData = &SharedTeamData[TeamType];
	SharedData->Reset();
	SharedData->ActivityId = ActivityId;
	SharedData->CommandTeam = TeamType;
	
	CurrentSharedTeamData[TeamType] = SharedData;

	for (ASWATCharacter* swat : SwatAI)
	{
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			if (UHoldActivity* HoldActivity = swat->GetCyberneticsController<ASWATController>()->GetHoldActivity())
			{
				HoldActivity->SharedData = SharedData;

				if (!RespondingSWAT)
				{
					RespondingSWAT = swat;
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_HOLD, HoldActivity);
				}

				UActivityManager::GiveActivityTo(HoldActivity, swat, true, false);
			}
		}
	}
}

void USWATManager::RemoveHoldCommand(const ETeamType TeamType)
{
	ASWATCharacter* RespondingSWAT = nullptr;
	
	for (ASWATCharacter* swat : SwatAI)
	{
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			if (UHoldActivity* HoldActivity = swat->GetCyberneticsController()->GetActivity<UHoldActivity>())
			{
				if (!RespondingSWAT)
				{
					RespondingSWAT = swat;
					
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC, HoldActivity);
				}
				
				swat->GetCyberneticsController()->FinishActivity(HoldActivity, true, true);
			}
		}
	}
}

void USWATManager::GivePickLockCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (Door->IsOpen() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (!Door->IsLocked())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCK_PICKED);
		return;
	}
	
	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<ULockPickDoorActivity, AMultitool>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC))
	{
		ULockPickDoorActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetLockPickDoorDoorActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		
		if (const UTeamStackUpActivity* StackUpActivity = SwatWithItem->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
		{
			if (StackUpActivity->OccupiedStackUpActor)
			{
				DoorActivity->OriginalLocation = StackUpActivity->OccupiedStackUpActor->GetActorLocation();
				DoorActivity->bReturnToPositionAfterInteraction = true;
			}
		}
		
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "PickLock");
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::RESPONSE_PICK_LOCK, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveRemoveWedgeCommand(AActor* Target, const ETeamType TeamType, const FVector CommandLocation, FVector CommandNormal)
{
	ADoor* Door = Cast<ADoor>(Target);
	if (!Door)
		return;

	if (GetSWATCount() == 0)
		return;

	if (Door->IsOpen() || Door->IsDoorwayOnly())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (!Door->IsJammed())
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		return;
	}

	if (ASWATCharacter* SwatWithItem = FindSwatWithItemForDoorInteraction<UDeployWedgeActivity, AMultitool>(Door, TeamType, VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC))
	{
		UDeployWedgeActivity* DoorActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetDeployWedgeActivity();
		DoorActivity->Door = Door;
		DoorActivity->CommandLocation = CommandLocation;
		DoorActivity->OriginalLocation = SwatWithItem->GetNavAgentLocation();
		DoorActivity->bReturnToPositionAfterInteraction = SwatWithItem->GetCyberneticsController()->GetActivity() == nullptr;
		DoorActivity->ActivityName = FText::FromStringTable("SwatCommandTable", "RemoveWedge");
		DoorActivity->bRemoveWedge = true;
		
		BindSWATActivityResponseVoiceLine(SwatWithItem, VO_SWAT_GENERAL::RESPONSE_REMOVE_DOOR_JAM, DoorActivity);

		UActivityManager::GiveActivityTo(DoorActivity, SwatWithItem, true, false);
	}
}

void USWATManager::GiveRestrainCommand(AActor* Target, const ETeamType TeamType, FVector CommandLocation)
{
	ACyberneticCharacter* ArrestTarget = Cast<ACyberneticCharacter>(Target);
	if (!ArrestTarget)
		return;

	if (GetSWATCount() == 0)
		return;

	if (!GetSwatWithItem(TeamType, AZipcuffs::StaticClass(), GetSwatTeam()))
	{
		GetSwatTeam()[0]->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);

		return;
	}
	
	// If someone is already arresting this target, don't give another activity
	if (UActivityManager::AnyAIHasActivity<UArrestTargetActivity>([&](const UArrestTargetActivity* Activity)
	{
		return Activity->ArrestTarget == ArrestTarget;
	}))
	{
		return;
	}

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(ArrestTarget->GetActorLocation(), TeamType);

	// Remove all swat that can't override their current activities or if they're currently arresting someone
	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!Swat)
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		if (const UBaseActivity* CurrentActivity = Swat->GetCyberneticsController()->GetCurrentActivity())
		{
			if (!CurrentActivity->CanOverrideActivity())
				return true;
		}
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UArrestTargetActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;

	if (ASWATCharacter* SwatWithItem = GetSwatWithItem(TeamType, AZipcuffs::StaticClass(), SortedSWAT)) // Note(Ali): do we need to care about zipcuffs??
	{
		if (UArrestTargetActivity* ArrestTargetActivity = SwatWithItem->GetCyberneticsController<ASWATController>()->GetArrestTargetActivity())
		{
			ArrestTargetActivity->ArrestTarget = ArrestTarget;
			
			UActivityManager::GiveActivityTo(ArrestTargetActivity, SwatWithItem, true, false);
		}
	}
}

void USWATManager::GiveCollectEvidenceCommand(AActor* Target, const ETeamType TeamType)
{
	if (!IsValid(Target))
		return;

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(Target->GetActorLocation());

	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!Swat)
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UCollectEvidenceActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;

	ASWATCharacter* RespondingSWAT = nullptr;

	if (SortedSWAT.IsValidIndex(0))
	{
		RespondingSWAT = SortedSWAT[0];
	}

	if (CanGiveActivityToSWAT(RespondingSWAT, TeamType))
	{
		if (UCollectEvidenceActivity* CollectEvidenceActivity = RespondingSWAT->GetCyberneticsController<ASWATController>()->GetCollectEvidenceActivity())
		{
			CollectEvidenceActivity->EvidenceItem = Target;

			RespondingSWAT->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC);

			UActivityManager::GiveActivityTo(CollectEvidenceActivity, RespondingSWAT, true, false);
		}
	}
}

void USWATManager::GiveReportTargetCommand(AActor* Target, const ETeamType TeamType)
{
	if (!IsValid(Target))
		return;

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(Target->GetActorLocation());

	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!IsValid(Swat))
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UReportTargetActivity>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return;

	ASWATCharacter* RespondingSWAT = nullptr;

	if (SortedSWAT.IsValidIndex(0))
	{
		RespondingSWAT = SortedSWAT[0];
	}

	if (CanGiveActivityToSWAT(RespondingSWAT, TeamType))
	{
		if (UReportTargetActivity* ReportTargetActivity = RespondingSWAT->GetCyberneticsController<ASWATController>()->GetReportTargetActivity())
		{
			ReportTargetActivity->ReportTarget = Target;

			RespondingSWAT->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC);

			UActivityManager::GiveActivityTo(ReportTargetActivity, RespondingSWAT, true, false);
		}
	}
}

void USWATManager::GiveDisarmStandaloneTrapCommand(AActor* Target, const ETeamType TeamType)
{
	ATrapActor* Trap = Cast<ATrapActor>(Target);
	if (!Trap)
		return;

	if (UActivityManager::AnyAIHasActivity<UDisarmStandaloneTrapActivity>([&](const UDisarmStandaloneTrapActivity* Activity)
	{
		return Activity->TrapToDisarm == Trap;
	}))
	{
		return;
	}
		
	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(Trap->GetActorLocation(), TeamType);

	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (!Swat)
			return true;

		if (!Swat->GetCyberneticsController())
			return true;
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<UDisarmStandaloneTrapActivity>() || Swat->GetCyberneticsController()->GetCurrentActivity<UDisarmDoorTrapActivity>();
	});

	if (SortedSWAT.Num() == 0)
		return;
	
	if (ASWATCharacter* swat = GetSwatWithItem(TeamType, AMultitool::StaticClass(), SortedSWAT))
	{
		if (UDisarmStandaloneTrapActivity* DisarmStandaloneTrapActivity = swat->GetCyberneticsController<ASWATController>()->GetDisarmStandaloneTrapActivity())
		{
			DisarmStandaloneTrapActivity->TrapToDisarm = Trap;
			
			UActivityManager::GiveActivityTo(DisarmStandaloneTrapActivity, swat, true, false);
		}
	}
}

void USWATManager::GiveCoverAreaCommand(ETeamType TeamType, FVector CommandLocation)
{
	if (SwatAI.Num() == 0)
		return;

	const FGuid ActivityId = FGuid::NewGuid();

	FSharedTeamData* SharedData = &SharedTeamData[TeamType];
	SharedData->Reset();
	SharedData->ActivityId = ActivityId;
	SharedData->CommandTeam = TeamType;
	SharedData->CommandLocation = CommandLocation;
	
	CurrentSharedTeamData[TeamType] = SharedData;

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(SquadLeader->GetActorLocation(), TeamType);

	ASWATCharacter* RespondingSWAT = SortedSWAT[0];
	uint8 SquadPosition = 0;
	
	for (ASWATCharacter* swat : SortedSWAT)
	{
		if (CanGiveActivityToSWAT(swat, TeamType))
		{
			if (UTeamCoverAreaActivity* CoverAreaActivity = swat->GetCyberneticsController<ASWATController>()->GetCoverAreaActivity())
			{
				CoverAreaActivity->SharedData = SharedData;
				CoverAreaActivity->OverrideSquadPosition = (ESquadPosition)SquadPosition;
				SquadPosition++;

				if (RespondingSWAT == swat)
				{
					BindSWATActivityResponseVoiceLine(RespondingSWAT, VO_SWAT_GENERAL::RESPONSE_COVER, CoverAreaActivity);
				}

				UActivityManager::GiveActivityTo(CoverAreaActivity, swat, true, true);
			}
		}
	}
}

bool USWATManager::CanGiveActivityToSWAT(ASWATCharacter* Swat, const ETeamType Team) const
{
	if (!Swat)
		return false;
	
	return (Swat->GetTeam() == Team || Team == ETeamType::TT_SQUAD) && Swat->GetCyberneticsController() && !Swat->IsDeadOrUnconscious();
}

ASWATCharacter* USWATManager::GetSwatWithItem(ETeamType TeamType, TSubclassOf<ABaseItem> Item, TArray<ASWATCharacter*> SortedArray)
{
	for (int32 i = 0; i < SortedArray.Num(); i++)
	{
		if (SortedArray[i] && (SortedArray[i]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SortedArray[i]->GetInventoryComponent()->GetInventoryItemOfClass(Item))
		{
			return SortedArray[i];
		}
	}
	
	if (SortedArray.IsValidIndex(0))
	{
		if (SortedArray[0] && (SortedArray[0]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SortedArray[0]->GetInventoryComponent()->GetInventoryItemOfClass(Item))
		{
			return SortedArray[0];
		}
	}
	return nullptr;
}

ASWATCharacter* USWATManager::GetSwatWithItem(ETeamType TeamType, TSubclassOf<ABaseItem> Item)
{
	if (!Item)
		return nullptr;
	
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && (SwatAI[i]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SwatAI[i]->GetInventoryComponent()->GetInventoryItemOfClass(Item, false))
		{
			return SwatAI[i];
		}
	}

	/*
	if (SwatAI.IsValidIndex(0))
	{
		if (SwatAI[0] && (SwatAI[0]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SwatAI[0]->GetInventoryComponent()->GetInventoryItemOfClass(Item))
		{
			return SwatAI[0];
		}
	}
	*/
	
	return nullptr;
}

ASWATCharacter* USWATManager::GetSwatWithItemType(ETeamType TeamType, EItemCategory Item) const
{
	for (int32 i = 0; i < SwatAI.Num(); i++)
	{
		if (SwatAI[i] && (SwatAI[i]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SwatAI[i]->GetInventoryComponent()->GetInventoryItemOfType(Item))
		{
			return SwatAI[i];
		}
	}
	
	if (SwatAI.IsValidIndex(0))
	{
		if (SwatAI[0] && (SwatAI[0]->GetTeam() == TeamType || TeamType == ETeamType::TT_SQUAD) && SwatAI[0]->GetInventoryComponent()->GetInventoryItemOfType(Item))
		{
			return SwatAI[0];
		}
	}
	return nullptr;
}

void USWATManager::BindSWATActivityResponseVoiceLine(class AReadyOrNotCharacter* RespondingAI, const FString& VoiceLine, UBaseActivity* Activity)
{
	if (Cast<ASWATCharacter>(SquadLeader))
		return;
	
	if (!RespondingAI || !Activity)
		return;

	RespondingAI->PlayRawVO(VoiceLine);
}
