// Copyright Void Interactive, 2023


#include "HUD/Widgets/MissionEnd.h"
#include "Subsystems/AchievementSubsystem.h"

#if defined(TARGET_PS5)
#include "Subsystems/PS5ActivitiesSubsystem.h"
#endif



void UMissionEnd::NativeConstruct()
{
	Super::NativeConstruct();
	UAchievementStatics::EndMission(GetWorld());
}
