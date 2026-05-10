// Void Interactive, 2020

#pragma once

#include "UObject/Interface.h"
#include "Components/ScoringComponent.h"
#include "ScoringInterface.generated.h"

UINTERFACE(MinimalAPI)
class UScoringInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class READYORNOT_API IScoringInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scoring Interface")
	class UScoringComponent* GetScoringComponent() const;
};
