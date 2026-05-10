// Copyright Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "CanUseMultitoolOn.generated.h"

UENUM(BlueprintType)
enum class EMultitoolFunctions : uint8
{
	MF_None,
	MF_Lockpick,
	MF_Knife,
	MF_Wirecutter
};

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UCanUseMultitoolOn : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API ICanUseMultitoolOn
{
	GENERATED_BODY()

public:
	// Returns true if we can use the multitool on this thing now
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	bool CanUseMultitoolNow(class AReadyOrNotCharacter* ToolOwner, class AMultitool* Tool, FHitResult TraceHit);

	// Returns true if we can cancel the multitool action while it's in progress
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	bool CanCancelMultitoolAction();

	// Get the kind of function that we need to open the multitool to
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	EMultitoolFunctions GetMultitoolUseType();
	
	// Get the amount of time that we need to use the multitool for
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	float GetMultitoolUseTime();

	// The action that is performed when we finish our multitool action. Executed on the server.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	void Server_FinishedUsingMultitool(class AReadyOrNotCharacter* ToolOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	void Client_FinishedUsingMultitool(class AReadyOrNotCharacter* ToolOwner);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUseMultitoolOn")
	bool ShouldOperate() const;
};
