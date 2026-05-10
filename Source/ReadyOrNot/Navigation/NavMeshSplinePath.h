// Void Interactive, 2022

#pragma once

#include "Components/SplineComponent.h"
#include "NavMesh/NavMeshPath.h"

struct READYORNOT_API FNavMeshSplinePath final : FNavMeshPath
{
	typedef FNavMeshPath Super;

	struct FMovementData
	{
		float K;
		float MaxVelocity;

		FVector Kv;
		FVector W;
	};
	
	FNavMeshSplinePath();

	static float GetInputKeyClosestToWorldLocation(const FSplineCurves* Spline, const FTransform& InTransform, const FVector& WorldLocation);
	static FVector GetLocationAtInputKey(const FSplineCurves* Spline, float	InputKey);
	static FVector GetLocationAtDistanceAlongSpline(const FSplineCurves* Spline, float Distance);
	static float GetDistanceAlongSplineAtInputKey(const FSplineCurves* Spline, float InputKey);

	static FMovementData CalculateMovementDataForPathIndex(const FSplineCurves* Spline, int32 Index, float CurrentVelocity);
};

