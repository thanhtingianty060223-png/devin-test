// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "ReadyOrNotNavigationSystem.generated.h"

UCLASS()
class READYORNOT_API UReadyOrNotNavigationSystem : public UNavigationSystemV1
{
	GENERATED_BODY()

public:
	FNavigationDirtyAreasController& GetDefaultDirtyAreasController() { return DefaultDirtyAreasController; }

protected:
	virtual void RebuildDirtyAreas(float DeltaSeconds) override;
};
