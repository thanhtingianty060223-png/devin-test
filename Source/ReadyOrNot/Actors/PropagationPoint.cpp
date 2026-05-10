// Copyright Void Interactive, 2022

#include "PropagationPoint.h"

APropagationPoint::APropagationPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
}
