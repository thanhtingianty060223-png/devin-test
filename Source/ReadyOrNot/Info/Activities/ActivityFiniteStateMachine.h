// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "ActivityFiniteStateMachine.generated.h"

#define MAKE_DELEGATE_BINDING(UserObject, FuncName) UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)) 

DECLARE_DELEGATE_RetVal(bool, FActivityStateTransitionDelegate);

USTRUCT(BlueprintType)
struct FActivityStateTransition
{
	GENERATED_BODY()

	FActivityStateTransition() {}
	
	FActivityStateTransition(const int32 InTransitionToStateID, const FActivityStateTransitionDelegate& InTransitionDelegate, const int32 InPriority = -1)
	{
		TransitionToStateID = InTransitionToStateID;
		TransitionDelegate = InTransitionDelegate;
		Priority = InPriority;
	}
	
	FActivityStateTransition(const FString& InTransitionToStateName, const FActivityStateTransitionDelegate& InTransitionDelegate, const int32 InPriority = -1)
	{
		TransitionToStateName = InTransitionToStateName;
		TransitionDelegate = InTransitionDelegate;
		Priority = InPriority;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TransitionToStateID = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TransitionToStateName = "None";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = -1;
	
	FActivityStateTransitionDelegate TransitionDelegate;

	UPROPERTY()
	class UActivityState* ToState = nullptr;

	friend bool operator==(const FActivityStateTransition& Lhs, const FActivityStateTransition& Rhs)
	{
		return Lhs.TransitionToStateID == Rhs.TransitionToStateID &&
				Lhs.TransitionToStateName == Rhs.TransitionToStateName &&
				Lhs.ToState == Rhs.ToState;
	}
};

UCLASS(BlueprintType, NotBlueprintable, Transient)
class READYORNOT_API UActivityState final : public UObject
{
	GENERATED_BODY()

public:
	UActivityState() {}
	UActivityState(class UActivityFiniteStateMachine* NewStateMachineOwner, const FString& InStateName, int32 InStateID)
	{
		StateMachineOwner = NewStateMachineOwner;
		Name = InStateName;
		ID = InStateID;
	}

	DECLARE_DELEGATE(FActivityStateEvent);
	FActivityStateEvent OnEnter;
	FActivityStateEvent OnExit;

	DECLARE_DELEGATE_TwoParams(FActivityStateUpdate, float, float);
	FActivityStateUpdate OnTick;
	
	void Init();
	void Enter();
	void Exit();
	void Tick(float DeltaTime);
		
	UActivityState* GetStateByID(int32 InStateID) const;
	UActivityState* GetStateByName(const FString& InStateName);

	UFUNCTION(BlueprintPure)
	FORCEINLINE TArray<FActivityStateTransition> GetTransitions() const { return StateTransitions; }

	template<class UserClass>
	UActivityState& BindEventEnter(UserClass* InUserObject, void(UserClass::* InMethodPtr)(), FName InFunctionName);
	template<class UserClass>
	UActivityState& BindEventExit(UserClass* InUserObject, void(UserClass::* InMethodPtr)(), FName InFunctionName);
	template<class UserClass>
	UActivityState& BindEventTick(UserClass* InUserObject, void(UserClass::* InMethodPtr)(float, float), FName InFunctionName);
	
	template<class UserClass>
	UActivityState& CreateTransition(int32 InStateID, UserClass* InUserObject, bool(UserClass::* InMethodPtr)(), FName InFunctionName, int32 Priority = -1);
	template<class UserClass>
	UActivityState& CreateTransition(const FString& InStateName, UserClass* InUserObject, bool(UserClass::* InMethodPtr)(), FName InFunctionName, int32 Priority = -1);
	
	// Const versions
	template<class UserClass>
	UActivityState& CreateTransition(int32 InStateID, UserClass* InUserObject, bool(UserClass::* InMethodPtr)() const, FName InFunctionName, int32 Priority = -1);
	template<class UserClass>
	UActivityState& CreateTransition(const FString& InStateName, UserClass* InUserObject, bool(UserClass::* InMethodPtr)() const, FName InFunctionName, int32 Priority = -1);

	UFUNCTION(BlueprintCallable)
	UActivityState* CreateTransition(const FActivityStateTransition& InStateTransition);
	
	UFUNCTION(BlueprintCallable)
	UActivityState* RemoveTransitionByName(const FString InName);
	
	UFUNCTION(BlueprintCallable)
	UActivityState* RemoveTransitionByID(const int32 InID);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Name = "Invalid";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ID = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Uptime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UActivityFiniteStateMachine* StateMachineOwner = nullptr;

protected:
	UActivityState* BindEventEnter(const FActivityStateEvent& Delegate);
	UActivityState* BindEventExit(const FActivityStateEvent& Delegate);
	UActivityState* BindEventTick(const FActivityStateUpdate& Delegate);
	
private:
	UActivityState* CreateTransition_Internal(const FActivityStateTransition& InStateTransition);
	bool CanCreateTransition(const FActivityStateTransition& InTransition) const;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FActivityStateTransition> StateTransitions;
	TArray<FActivityStateTransition> StateTransitions_Uninitialized;
};

template<class UserClass>
UActivityState& UActivityState::BindEventEnter(UserClass* InUserObject, void(UserClass::* InMethodPtr)(), FName InFunctionName)
{
	OnEnter.BindUObject(InUserObject, InMethodPtr);
	//OnEnter.__Internal_BindDynamic(InUserObject, InMethodPtr, InFunctionName);
	return *this;
}

template<class UserClass>
UActivityState& UActivityState::BindEventExit(UserClass* InUserObject, void(UserClass::* InMethodPtr)(), FName InFunctionName)
{
	OnExit.BindUObject(InUserObject, InMethodPtr);
	//OnExit.__Internal_BindDynamic(InUserObject, InMethodPtr, InFunctionName);
	return *this;
}

template<class UserClass>
UActivityState& UActivityState::BindEventTick(UserClass* InUserObject, void(UserClass::* InMethodPtr)(float, float), FName InFunctionName)
{
	OnTick.BindUObject(InUserObject, InMethodPtr);
	//OnTick.__Internal_BindDynamic(InUserObject, InMethodPtr, InFunctionName);
	return *this;
}

template <class UserClass>
UActivityState& UActivityState::CreateTransition(int32 InStateID, UserClass* InUserObject, bool(UserClass::* InMethodPtr)(), FName InFunctionName, int32 Priority)
{
	FActivityStateTransitionDelegate Delegate;
	Delegate.BindUObject(InUserObject, InMethodPtr);
	//Delegate.__Internal_BindDynamic(InUserObject, reinterpret_cast<typename FActivityStateTransitionDelegate::TMethodPtrResolver<UserClass>::FMethodPtr>(InMethodPtr), InFunctionName);

	return *CreateTransition({InStateID, Delegate, Priority});
}

template <class UserClass>
UActivityState& UActivityState::CreateTransition(const FString& InStateName, UserClass* InUserObject, bool(UserClass::* InMethodPtr)(), FName InFunctionName, int32 Priority)
{
	FActivityStateTransitionDelegate Delegate;
	Delegate.BindUObject(InUserObject, InMethodPtr);
	//Delegate.__Internal_BindDynamic(InUserObject, reinterpret_cast<typename FActivityStateTransitionDelegate::TMethodPtrResolver<UserClass>::FMethodPtr>(InMethodPtr), InFunctionName);

	return *CreateTransition({InStateName, Delegate, Priority});
}

template<class UserClass>
UActivityState& UActivityState::CreateTransition(int32 InStateID, UserClass* InUserObject, bool(UserClass::* InMethodPtr)() const, FName InFunctionName, int32 Priority)
{
	FActivityStateTransitionDelegate Delegate;
	Delegate.BindUObject(InUserObject, InMethodPtr);
	//Delegate.__Internal_BindDynamic(InUserObject, reinterpret_cast<typename FActivityStateTransitionDelegate::TMethodPtrResolver<UserClass>::FMethodPtr>(InMethodPtr), InFunctionName);

	return *CreateTransition({InStateID, Delegate, Priority});
}

template<class UserClass>
UActivityState& UActivityState::CreateTransition(const FString& InStateName, UserClass* InUserObject, bool(UserClass::* InMethodPtr)() const, FName InFunctionName, int32 Priority)
{
	FActivityStateTransitionDelegate Delegate;
	Delegate.BindUObject(InUserObject, InMethodPtr);
	//Delegate.__Internal_BindDynamic(InUserObject, reinterpret_cast<typename FActivityStateTransitionDelegate::TMethodPtrResolver<UserClass>::FMethodPtr>(InMethodPtr), InFunctionName);
	
	return *CreateTransition({InStateName, Delegate, Priority});
}

UCLASS(BlueprintType, NotBlueprintable, Transient)
class READYORNOT_API UActivityFiniteStateMachine final : public UObject
{
	GENERATED_BODY()

	friend UActivityState;

public:
	void Init();
	void Reset();
	void Shutdown();
	void Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	UActivityState* AddState(const FString& NewStateName, TArray<FActivityStateTransition> Transitions);
	UActivityState& AddState(const FString& NewStateName);
	
	UActivityState& GetState(int32 InStateID);
	UActivityState& GetState(const FString& InStateName);

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsMachineInitialized() const { return bMachineInitialized; }
	
	UFUNCTION(BlueprintPure)
	UActivityState* GetActiveState();
	
	UFUNCTION(BlueprintPure)
	UActivityState* GetStateByID(int32 InStateID) const;
	UFUNCTION(BlueprintPure)
	UActivityState* GetStateByName(const FString& InStateName) const;
	
	UActivityState* FindStateFromTransition(const FActivityStateTransition& InStateTransition);

private:
	UPROPERTY(VisibleInstanceOnly)
	TArray<UActivityState*> States;

	UPROPERTY(VisibleInstanceOnly)
	UActivityState* ActiveState = nullptr;
	
	UPROPERTY(VisibleInstanceOnly)
	uint8 bMachineInitialized : 1;
};