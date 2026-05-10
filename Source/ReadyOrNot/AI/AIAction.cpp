// Copyright Void Interactive, 2022

#include "AIAction.h"

#include "Characters/CyberneticController.h"

FName UAIAction::GetMoveStyleOverride_Implementation() const
{
	return NAME_None;
}

UWorld* UAIAction::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	#if WITH_EDITOR
	ensureAlways(OwningController != nullptr);
	#endif
	
	if (OwningController)
	{
		return OwningController->GetWorld();
	}
	
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UAIAction::OnCreate(ACyberneticController* InController)
{
	LastMoveRequestMoveID = -1;
	LastMoveRequestPathID = -1;
	bSearchingPath = false;
	
	OwningController = InController;

	OnCreate_Blueprint(InController);
}

void UAIAction::OnSucceededToConsider()
{
	OnSucceededToConsider_Blueprint();
}

void UAIAction::OnFailedToConsider()
{
	OnFailedToConsider_Blueprint();
}

void UAIAction::InitAction(ACyberneticController* InController, FAIActionData* InActionData)
{
	bAbortAction = false;
	bSearchingPath = false;
	
	OwningController = InController;
	ActionData = InActionData;

	#if WITH_EDITOR
	ensureAlways(OwningController != nullptr);
	ensureAlways(ActionData != nullptr);
	#endif

	InitAction_Blueprint(InController);
}

void UAIAction::AbortAction()
{
	bAbortAction = true;
	bSearchingPath = false;
	
	OwningController->OnAsyncPathFound.RemoveAll(this);
	OwningController->OnMoveComplete.RemoveAll(this);
}

void UAIAction::BeginAction()
{
	bAbortAction = false;
	bSearchingPath = false;
	
	#if WITH_EDITOR
	ensureAlways(OwningController != nullptr);
	ensureAlways(ActionData != nullptr);
	#endif
	
	BeginAction_Blueprint();
}

void UAIAction::EndAction()
{
	// cancel all latent actions, like Delay nodes in blueprint graph
	FLatentActionManager& LatentActionManager = GetWorld()->GetLatentActionManager();
	LatentActionManager.RemoveActionsForObject(this);

	bSearchingPath = false;
	OwningController->OnAsyncPathFound.RemoveAll(this);
	OwningController->OnMoveComplete.RemoveAll(this);
	
	EndAction_Blueprint();
}

void UAIAction::Tick(const float DeltaTime)
{
	Tick_Blueprint(DeltaTime);
}

void UAIAction::OnTakeDamage(float Damage, AReadyOrNotCharacter* Instigator)
{
	OnTakenDamage_Blueprint(Damage, Instigator);
}

bool UAIAction::ShouldPerformAction_Implementation() const
{
	return true;
}

void UAIAction::RequestMove(FVector Location, float AcceptanceRadius)
{
	if (bSearchingPath)
		return;
	
	if (OwningController->IsMovingForRequest(LastMoveRequestMoveID))
		return;

	bSearchingPath = true;
	
	LastMoveRequestPathID = OwningController->RequestMoveAsync(Location, true, AcceptanceRadius);

	OwningController->OnAsyncPathFound.RemoveAll(this);
	OwningController->OnAsyncPathFound.AddDynamic(this, &UAIAction::OnPathFound);

	OwningController->OnMoveComplete.RemoveAll(this);
	OwningController->OnMoveComplete.AddDynamic(this, &UAIAction::OnMoveComplete);
}

void UAIAction::OnPathFound(int32 PathId, ERonNavigationQueryResult Result)
{
	bSearchingPath = false;
	
	if (PathId == LastMoveRequestPathID)
	{
		OwningController->OnAsyncPathFound.RemoveAll(this);
		
		if (Result != ERonNavigationQueryResult::Success)
		{
			AbortAction();
			return;
		}

		for (const auto& It : OwningController->MoveRequestIdToPathIdMap)
		{
			if (It.Value == PathId)
			{
				LastMoveRequestMoveID = It.Key;
				break;
			}
		}

		if (!bAbortAction)
		{
			OnPathFound_Blueprint(PathId, Result);
		}
	}
}

void UAIAction::OnMoveComplete(AAIController* Controller, int32 RequestID)
{
	if (RequestID == LastMoveRequestMoveID)
	{
		OwningController->OnMoveComplete.RemoveAll(this);

		if (!bAbortAction)
		{
			OnMoveComplete_Blueprint(Controller, RequestID);
		}
	}
}

ACyberneticCharacter* UAIAction::GetCharacter() const
{
	return OwningController->GetCharacter();
}

int32 UAIAction::GetActionRunCount() const
{
	if (const uint32* RunCountPtr = ActionData->RunCount.Find(OwningController))
	{
		return *RunCountPtr;
	}

	return -1;
}

FString UAIAction::GatherDebugInfo_Implementation() const
{
	return "";
}

