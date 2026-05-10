// Void Interactive, 2020

#include "ReadyOrNotNavigationSystem.h"

void UReadyOrNotNavigationSystem::RebuildDirtyAreas(float DeltaSeconds)
{
	DefaultDirtyAreasController.DirtyAreasUpdateFreq = 999.0f;
	
	Super::RebuildDirtyAreas(DeltaSeconds);
}
