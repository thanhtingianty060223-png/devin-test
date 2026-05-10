// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReadyOrNotMathLibrary.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static FVector2D CalculatePositionOnCircle(FVector2D Origin, float Radius, float Angle);
	
	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static FVector2D CalculatePositionOnEllipse(FVector2D Origin, float RadiusX, float RadiusY, float Angle);

	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static FVector CalculateLocationOnSphere(FVector Origin, float Radius, float Angle, float Phi);

	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static FVector GenerateRandomLocationOnSphere(FVector Origin, float Radius);

	// Wraps the given angle between two bounds. If Angle is -20 degrees, MinBounds is 0 and MaxBounds is 360, then the function would return (-20 + 360) = 340 degrees
	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static float WrapAngleIfOutOfBounds(float Angle, float MinBounds = 0.0f, float MaxBounds = 360.0f);

	// Wraps the given angle if above 360 degrees.
	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static float KeepAngleBelow360(float Angle);

	// Wraps the given angle if below 0 degrees.
	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library")
	static float KeepAngleAbove0(float Angle);

	UFUNCTION(BlueprintPure, Category = "Ready or Not | Math Library | Curve", DisplayName = "Get Last Key Time (Float Curve)")
	static float GetLastKeyTime_FloatCurve(const FRuntimeFloatCurve& InCurve);
	
    static float GetLastKeyTime_VectorCurve(const FRuntimeCurveLinearColor& InCurve);

	static float EaseAlpha(float InAlpha, uint8 EasingFunc, float BlendExp, int32 Steps);
};
