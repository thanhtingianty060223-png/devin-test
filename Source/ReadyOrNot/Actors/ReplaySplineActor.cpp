// Copyright Void Interactive, 2022

#include "Actors/ReplaySplineActor.h"

#include "Components/SplineComponent.h"

AReplaySplineActor::AReplaySplineActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline Component"));
	SplineComponent->SetClosedLoop(false);
	SplineComponent->ClearSplinePoints();

	bReplayRewindable = true;
}
