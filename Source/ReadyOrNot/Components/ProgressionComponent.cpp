// Copyright Void Interactive, 2021

#include "ProgressionComponent.h"

UProgressionComponent::UProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProgressionComponent::AddLevel(float OverflowXP)
{
	// overflow XP cannot be negative
	OverflowXP = FMath::Max(0.0f, OverflowXP);

	StatLevel++;
	StatExperience = 0 + OverflowXP;
	
	V_LOGM(LogReadyOrNotProgression, "Player gained level %f overflow experience %f", StatLevel, StatExperience);
}

void UProgressionComponent::AddExperience(const float XP)
{
	V_LOGM(LogReadyOrNotProgression, "Player gained experience %f", StatExperience);
	
	StatExperience += XP;
	
	if (StatExperience >= XPRequiredToLevel)
	{
		AddLevel(XPRequiredToLevel - StatExperience);
	}
}
