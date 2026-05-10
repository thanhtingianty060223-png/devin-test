// Copyright Void Interactive, 2023

#pragma once

#include "AISystem.h"
#include "ReadyOrNotAISystem.generated.h"

UCLASS()
class READYORNOT_API UReadyOrNotAISystem final : public UAISystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static bool ProjectPointToNav(FVector Point, FVector& OutLocation, FVector Extent = FVector(25.0f, 25.0f, 150.0f));

	static bool FindPath(FVector From, FVector To, float* OutLength = nullptr, FNavigationPath* OutPath = nullptr);

	UFUNCTION(BlueprintCallable)
	static bool WasRecentlyInCombat(float SinceSeconds, bool bCivilianCheck = false);
	
protected:
	virtual void PostInitProperties() override;
};
