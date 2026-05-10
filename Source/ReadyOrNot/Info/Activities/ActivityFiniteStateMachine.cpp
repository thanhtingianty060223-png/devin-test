// Copyright Void Interactive, 2021

#include "ActivityFiniteStateMachine.h"

void UActivityState::Init()
{
	for (FActivityStateTransition& StateTransition : StateTransitions_Uninitialized)
	{
		#if !UE_BUILD_SHIPPING
		if (StateMachineOwner)
			ensureAlwaysMsgf(!(GetStateByID(StateTransition.TransitionToStateID) == nullptr && GetStateByName(StateTransition.TransitionToStateName) == nullptr), TEXT("Make sure the state you want to transition exists"));
		#endif
	
		CreateTransition(StateTransition);
	}

	StateTransitions_Uninitialized.Empty();
}

void UActivityState::Enter()
{
	Uptime = 0.0f;
	OnEnter.ExecuteIfBound();
}

void UActivityState::Exit()
{
	OnExit.ExecuteIfBound();
	Uptime = 0.0f;
}

void UActivityState::Tick(const float DeltaTime)
{
	Uptime += DeltaTime;

	OnTick.ExecuteIfBound(DeltaTime, Uptime);

	// OnTick could've finished the acitivity and thus shutdown the state machine, check if this was the case and do not transition
	if (StateMachineOwner->IsMachineInitialized())
	{
		for (const FActivityStateTransition& StateTransition : StateTransitions)
		{
			if (StateTransition.ToState)
			{
				if (StateTransition.TransitionDelegate.IsBound() && StateTransition.TransitionDelegate.Execute())
				{
					Exit();
					StateMachineOwner->ActiveState = StateTransition.ToState;
					StateMachineOwner->ActiveState->Enter();
				}
			}
		}
	}
}

UActivityState* UActivityState::BindEventEnter(const FActivityStateEvent& Delegate)
{
	OnEnter = Delegate;
	return this;
}

UActivityState* UActivityState::BindEventExit(const FActivityStateEvent& Delegate)
{
	OnExit = Delegate;
	return this;
}

UActivityState* UActivityState::BindEventTick(const FActivityStateUpdate& Delegate)
{
	OnTick = Delegate;
	return this;
}

UActivityState* UActivityState::CreateTransition(const FActivityStateTransition& InStateTransition)
{
	if (StateMachineOwner && !StateMachineOwner->IsMachineInitialized())
	{
		if (InStateTransition.TransitionToStateID > -1)
		{
			StateTransitions_Uninitialized.Add({InStateTransition.TransitionToStateID, InStateTransition.TransitionDelegate, InStateTransition.Priority});
			return this;
		}

		if (!InStateTransition.TransitionToStateName.IsEmpty())
		{
			StateTransitions_Uninitialized.Add({InStateTransition.TransitionToStateName, InStateTransition.TransitionDelegate, InStateTransition.Priority});
			return this;
		}

		return this;
	}
	
	return CreateTransition_Internal(InStateTransition);
}

UActivityState* UActivityState::RemoveTransitionByName(const FString InName)
{
	if (StateMachineOwner && !StateMachineOwner->IsMachineInitialized())
	{
		StateTransitions_Uninitialized.RemoveAll([&](const FActivityStateTransition& Transition)
		{
			if (Transition.ToState)
				return Transition.ToState->Name == InName;

			return false;
		});
	}
	
	return this;
}

UActivityState* UActivityState::RemoveTransitionByID(const int32 InID)
{
	if (StateMachineOwner && !StateMachineOwner->IsMachineInitialized())
	{
		StateTransitions_Uninitialized.RemoveAll([&](const FActivityStateTransition& Transition)
		{
			if (Transition.ToState)
				return Transition.ToState->ID == InID;

			return false;
		});
	}
	
	return this;
}

UActivityState* UActivityState::CreateTransition_Internal(const FActivityStateTransition& InStateTransition)
{
	if (StateMachineOwner && !StateMachineOwner->IsMachineInitialized())
		return this;
	
	if (CanCreateTransition(InStateTransition))
	{
		FActivityStateTransition NewStateTransition = InStateTransition;
		UActivityState* ActivityState = StateMachineOwner->FindStateFromTransition(InStateTransition);
		
		if (ActivityState)
		{
			NewStateTransition.ToState = ActivityState; 
			NewStateTransition.TransitionToStateName = ActivityState->Name;
			NewStateTransition.TransitionToStateID = ActivityState->ID;
			
			StateTransitions.Add(NewStateTransition);
			StateTransitions.Sort([](const FActivityStateTransition& Lhs, const FActivityStateTransition& Rhs)
			{
				return Lhs.Priority > Rhs.Priority;
			});
		}
	}

	return this;
}

bool UActivityState::CanCreateTransition(const FActivityStateTransition& InTransition) const
{
	if (!StateMachineOwner)
		return false;

	for (const FActivityStateTransition& StateTransition : StateTransitions)
	{
		if (StateTransition == InTransition)
			return false;
	}

	return true;
}

UActivityState* UActivityState::GetStateByID(const int32 InStateID) const
{
	if (StateMachineOwner)
	{
		return StateMachineOwner->GetStateByID(InStateID);
	}

	return nullptr;
}

UActivityState* UActivityState::GetStateByName(const FString& InStateName)
{
	if (StateMachineOwner)
	{
		return StateMachineOwner->GetStateByName(InStateName);
	}

	return nullptr;
}

void UActivityFiniteStateMachine::Init()
{
	if (bMachineInitialized)
		return;

	States.Remove(nullptr);
	
	if (States.Num() > 0)
	{
		bMachineInitialized = true;

		for (UActivityState* State : States)
		{
			State->Init();
		}

		ActiveState = States[0];
		ActiveState->Enter();
	}
}

void UActivityFiniteStateMachine::Reset()
{
	States.Remove(nullptr);

	if (States.Num() > 0)
	{
		bMachineInitialized = true;

		ActiveState = States[0];
		ActiveState->Enter();
	}
}

void UActivityFiniteStateMachine::Shutdown()
{
	for (UActivityState* State : States)
		State->Uptime = 0.0f;
	
	ActiveState = nullptr;
	bMachineInitialized = false;
}

void UActivityFiniteStateMachine::Tick(const float DeltaTime)
{
	if (bMachineInitialized && ActiveState)
	{
		ActiveState->Tick(DeltaTime);
	}
}

UActivityState* UActivityFiniteStateMachine::GetActiveState()
{
	// If no active state present. Make sure the state machine has been initialized
	return ActiveState;
}

UActivityState* UActivityFiniteStateMachine::GetStateByID(const int32 InStateID) const
{
	for (UActivityState* ActivityState : States)
	{
		if (ActivityState->ID == InStateID)
			return ActivityState;
	}

	return nullptr;
}

UActivityState* UActivityFiniteStateMachine::GetStateByName(const FString& InStateName) const
{
	for (UActivityState* ActivityState : States)
	{
		if (ActivityState->Name == InStateName)
			return ActivityState;
	}
	
	return nullptr;
}

UActivityState* UActivityFiniteStateMachine::FindStateFromTransition(const FActivityStateTransition& InStateTransition)
{
	for (UActivityState* ActivityState : States)
	{
		if (ActivityState->ID == InStateTransition.TransitionToStateID || ActivityState->Name == InStateTransition.TransitionToStateName)
			return ActivityState;
	}
	
	return nullptr;
}

UActivityState& UActivityFiniteStateMachine::AddState(const FString& NewStateName)
{
	if (UActivityState* ExistingState = GetStateByName(NewStateName))
		return *ExistingState;
	
	UActivityState* NewState = NewObject<UActivityState>(this, UActivityState::StaticClass());
	NewState->StateMachineOwner = this;
	NewState->Name = NewStateName;
	NewState->ID = States.Num();
	States.Add(NewState);

	return *NewState;
}

UActivityState& UActivityFiniteStateMachine::GetState(const int32 InStateID)
{
	return *GetStateByID(InStateID);
}

UActivityState& UActivityFiniteStateMachine::GetState(const FString& InStateName)
{
	return *GetStateByName(InStateName);
}

UActivityState* UActivityFiniteStateMachine::AddState(const FString& NewStateName, TArray<FActivityStateTransition> Transitions)
{
	UActivityState& NewState = AddState(NewStateName);
	
	for (FActivityStateTransition& StateTransition : Transitions)
	{
		NewState.CreateTransition(StateTransition);
	}
	
	return &NewState;
}
