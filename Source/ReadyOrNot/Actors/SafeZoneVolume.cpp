// Copyright Void Interactive, 2023

#include "SafeZoneVolume.h"

#include "Actors/Door.h"
#include "Info/SoundManager.h"

ASafeZoneVolume::ASafeZoneVolume()
{
	bGenerateOverlapEventsDuringLevelStreaming = true;
	
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}
