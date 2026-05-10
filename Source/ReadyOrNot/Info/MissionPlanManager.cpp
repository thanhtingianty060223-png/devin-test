// Copyright Void Interactive, 2023

#include "MissionPlanManager.h"

#include "Actors/Environment/MissionPortal.h"

void FPlanningMarker::PreReplicatedRemove(const FPlanningMarkerArray& InArraySerializer)
{
	if (ReplicationID == INDEX_NONE)
		return;
	
	InArraySerializer.OnPlanningMarkerRemoved.Broadcast(ReplicationID);
}

void FPlanningMarker::PostReplicatedAdd(const FPlanningMarkerArray& InArraySerializer)
{
	if (ReplicationID == INDEX_NONE)
		return;

	InArraySerializer.OnPlanningMarkerAdded.Broadcast(*this);
}

void FPlanningLine::PreReplicatedRemove(const FPlanningLineArray& InArraySerializer)
{
	if (ReplicationID == INDEX_NONE)
		return;

	InArraySerializer.OnPlanningLineRemoved.Broadcast(ReplicationID);
}

void FPlanningLine::PostReplicatedAdd(const FPlanningLineArray& InArraySerializer)
{
	if (ReplicationID == INDEX_NONE)
		return;

	InArraySerializer.OnPlanningLineAdded.Broadcast(*this);
}

AMissionPlanManager::AMissionPlanManager()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	PrimaryActorTick.bCanEverTick = false;
}

void AMissionPlanManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && UReadyOrNotFunctionLibrary::IsInLobby())
	{
		for (const TActorIterator<AMissionPortal> It(GetWorld()); It;)
		{
			It->OnMissionSelected_Delegate.RemoveAll(this);
			It->OnMissionSelected_Delegate.AddDynamic(this, &AMissionPlanManager::OnMissionChanged);
			break;
		}
	}
}

void AMissionPlanManager::ClearPlan()
{
	if (!HasAuthority())
		return;

	MarkerArray.Items.Empty();
	MarkerArray.MarkArrayDirty();

	LineArray.Items.Empty();
	LineArray.MarkArrayDirty();
}

void AMissionPlanManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMissionPlanManager, MarkerArray);
	DOREPLIFETIME(AMissionPlanManager, LineArray);
}

void AMissionPlanManager::AddPlayer(AReadyOrNotGameState* GameState, AReadyOrNotPlayerState* PlayerState)
{
	if (!GameState || !PlayerState)
		return;

	// Ignore if a player already has a number assigned
	if (PlayerState->PlanningPlayerNumber > 0)
		return;
	
	// Construct array of all currently used player numbers
	TArray<int32> PlanningPlayerNumbers;
	for (APlayerState* ExistingPlayerState : GameState->PlayerArray)
	{
		AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(ExistingPlayerState);
		if (ReadyOrNotPlayerState && ReadyOrNotPlayerState->PlanningPlayerNumber > 0)
			PlanningPlayerNumbers.AddUnique(ReadyOrNotPlayerState->PlanningPlayerNumber);
	}
	
	// Find the first player number that isn't taken
	int32 i = 1;
	while (PlanningPlayerNumbers.Contains(i))
	{
		i++;
	}

	// Assign player their number
	PlayerState->PlanningPlayerNumber = i;
}

AReadyOrNotPlayerState* AMissionPlanManager::GetPlayerStateFromPlanningNumber(const UObject* WorldContextObject, int32 Number)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
		return nullptr;

	AReadyOrNotGameState* GameState = World->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return nullptr;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(PlayerState);
		if (ReadyOrNotPlayerState && ReadyOrNotPlayerState->PlanningPlayerNumber == Number)
			return ReadyOrNotPlayerState;
	}

	return nullptr;
}

