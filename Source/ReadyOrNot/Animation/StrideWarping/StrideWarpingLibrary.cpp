// Copyright Epic Games, Inc. All Rights Reserved.

#include "StrideWarpingLibrary.h"
#include "Kismet/KismetMathLibrary.h"


void UStrideWarpingLibrary::UpdateStrideWarping(float DeltaTime, const FVector& Velocity, const float& VelocityInterpTime, const float& CurrentAnimationSpeed, const float& PlayrateMaxAdjustment, UPARAM(ref) float& StrideScaling, UPARAM(ref) float& Playrate, UPARAM(ref) FVector& VelocitySmoothed)
{
	VelocitySmoothed = FMath::VInterpTo(VelocitySmoothed, Velocity, DeltaTime, VelocityInterpTime);
	float Speed2D = VelocitySmoothed.Size2D();
	float CurrentSpeedScaling = FMath::Clamp(UKismetMathLibrary::SafeDivide(Speed2D, CurrentAnimationSpeed), 0.0f, 3.0f);
	Playrate = FMath::Clamp(CurrentSpeedScaling, 1.0f - PlayrateMaxAdjustment, 1.0f + PlayrateMaxAdjustment);
	StrideScaling = FMath::Clamp(UKismetMathLibrary::SafeDivide(Speed2D, CurrentAnimationSpeed * Playrate), 0.0f, 3.0f);
}