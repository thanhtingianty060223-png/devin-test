// Void Interactive, 2020

#pragma once

#include "UObject/Interface.h"
#include "CanIssueCommandOn.generated.h"

UINTERFACE(MinimalAPI)
class UCanIssueCommandOn : public UInterface
{
	GENERATED_BODY()
};

/**
 * Add to any actor that can have commands issued on them by the SWAT team
 */
class READYORNOT_API ICanIssueCommandOn
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Can Issue Command On")
	bool CanIssueCommand() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Can Issue Command On")
	AActor* GetCommandActor() const;
};
