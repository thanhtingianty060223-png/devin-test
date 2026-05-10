// Void Interactive, 2022

#include "NavMeshSplinePath.h"

#include "Components/SplineComponent.h"

FNavMeshSplinePath::FNavMeshSplinePath()
{
	bUseOnPathUpdatedNotify = true;
}

float FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(const FSplineCurves* Spline, const FTransform& InTransform, const FVector& WorldLocation)
{
	FVector LocalLocation = InTransform.InverseTransformPosition(WorldLocation);
	float Dummy;
	return Spline->Position.InaccurateFindNearest(LocalLocation, Dummy);
}

FVector FNavMeshSplinePath::GetLocationAtInputKey(const FSplineCurves* Spline, const float InputKey)
{
	return Spline->Position.Eval(InputKey, FVector::ZeroVector);
}

FVector FNavMeshSplinePath::GetLocationAtDistanceAlongSpline(const FSplineCurves* Spline, const float Distance)
{
	const float Param = Spline->ReparamTable.Eval(Distance, 0.0f);
	return Spline->Position.Eval(Param, FVector::ZeroVector);
}

float FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(const FSplineCurves* Spline, float InputKey)
{
	const int32 NumPoints = Spline->Position.Points.Num();
	const int32 NumSegments = NumPoints - 1;

	if ((InputKey >= 0) && (InputKey < NumSegments))
	{
		const int32 PointIndex = FMath::FloorToInt(InputKey);
		const float Fraction = InputKey - PointIndex;
		const int32 ReparamPointIndex = PointIndex * 10;
		const float Distance = Spline->ReparamTable.Points[ReparamPointIndex].InVal;
		return Distance + Spline->GetSegmentLength(PointIndex, Fraction, false);
	}
	
	if (InputKey >= NumSegments)
	{
		return Spline->GetSplineLength();
	}

	return 0.0f;
}

FNavMeshSplinePath::FMovementData FNavMeshSplinePath::CalculateMovementDataForPathIndex(const FSplineCurves* Spline, int32 Index, float CurrentVelocity)
{
	//Index = FMath::Clamp(Index, 0, Spline->Position.Points.Num());
	Index = Index == 0 ? Index+1 : Index == Spline->Position.Points.Num() ? Index-1 : Index;

	// Calculate max velocity of a path segement
	const FVector Y1 = Spline->Position.EvalDerivative(Index, FVector(1.0f, 0.0f, 0.0f));
	const FVector Y2 = Spline->Position.EvalSecondDerivative(Index, FVector(0.0f, 1.0f, 0.0f));
	
	constexpr float MaxPerpAcceleration = 500.0f;
	constexpr float MaxAngularVelocity = 200.0f;
	const float K = (Y1 * Y2).Size() / FMath::Pow(Y1.Size(), 3.0f);

	// Calculate turn rate limit of a path segement
	const FVector Kv = (Y1 * Y2) / FMath::Pow(Y1.Size(), 3.0f);
	const FVector W = Kv*FMath::Clamp(FMath::Abs(CurrentVelocity), 10.0f, 100000.0f); // Current velocity

	//const float KYaw = FVector::DotProduct(Kv, FVector::UpVector) * 600.0f;

	const float MaxVelocity = FMath::Min(FMath::Sqrt(MaxPerpAcceleration/K), MaxAngularVelocity/K);
	
	FMovementData Data;
	Data.K = K;
	Data.Kv = Kv;
	Data.W = W;
	Data.MaxVelocity = MaxVelocity;

	return Data;
}

