// Copyright Void Interactive, 2023

#include "Info/FootstepImpactEffectsPool.h"

#include "Actors/Gameplay/ImpactEffect.h"

UFootstepImpactEffectsPool::UFootstepImpactEffectsPool()
{
	PoolName = "Footstep Effects Pool";
	PoolSize = 1024;
	ReuseSetting = EObjectPoolReuseSetting::Reuse;
	ObjectClassToPool = AImpactEffect::StaticClass();
	bFillPoolOnBeginPlay = true;
}
