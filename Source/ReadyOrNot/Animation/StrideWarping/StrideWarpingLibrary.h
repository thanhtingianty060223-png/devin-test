// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StrideWarpingLibrary.generated.h"

class UAnimSequence;

UCLASS()
class UStrideWarpingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Take care of calculating out distributed amount between Playrate and Stride scaling based on animation speed and actual character speed.
	* DeltaTime						-	Current DT
	* Velocity						-	Velocity from Char Move Comp	
	* CurrentAnimationSpeed			-	Current Animation Speed from Sequence Curve
	* PlayrateMaxAdjustment			-	The max possible adjustment of Sequence Playrate
	* StrideScaling					-	Updated reference of Stride Scaling
	* Playrate						-	Updated reference of Playrate
	* 
	* 
	* Example Settings for Directional Strafe Stride Warping Node:
	*	->	Manual Speed Warping Dir		:	Use VelocitySmoothed Result
	*	->	Speed Scaling					:	Use StrideScaling Result
	*	->	Speed Warping Axis Mode			:	World Space Vector Input
	*	->	Speed Scaling Scale Bias Clamp	:	Interp Result and set Interp Speed
	* 
	*/
	UFUNCTION(BlueprintCallable, Category = "Animation Stride Warping", meta = (BlueprintThreadSafe))
	static void UpdateStrideWarping(float DeltaTime, const FVector& Velocity, const float& VelocityInterpTime, const float& CurrentAnimationSpeed, const float& PlayrateMaxAdjustment, UPARAM(ref) float& StrideScaling, UPARAM(ref) float& Playrate, UPARAM(ref) FVector& VelocitySmoothed);
};