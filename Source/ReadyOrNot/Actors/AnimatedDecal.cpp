// Copyright Void Interactive, 2022

#include "AnimatedDecal.h"

AAnimatedDecal::AAnimatedDecal()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DecalRoot"));
	
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(RootComponent);
}

float AAnimatedDecal::GetRuntimeFloatCurveValue(const FRuntimeFloatCurve& Curve, float Time)
{
	return Curve.GetRichCurveConst()->Eval(Time);
}

