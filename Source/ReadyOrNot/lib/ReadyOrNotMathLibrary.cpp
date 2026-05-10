// Copyright Void Interactive, 2021

#include "ReadyOrNotMathLibrary.h"
#include "ReadyOrNot.h"

FVector2D UReadyOrNotMathLibrary::CalculatePositionOnCircle(const FVector2D Origin, const float Radius, const float Angle)
{
	return FVector2D(Origin.X + Radius * FMath::Cos(FMath::DegreesToRadians(Angle)), Origin.Y + Radius * FMath::Sin(FMath::DegreesToRadians(Angle)));
}

FVector2D UReadyOrNotMathLibrary::CalculatePositionOnEllipse(const FVector2D Origin, const float RadiusX, const float RadiusY, const float Angle)
{
	return FVector2D(Origin.X + RadiusX * FMath::Cos(FMath::DegreesToRadians(Angle)), Origin.Y + RadiusY * FMath::Sin(FMath::DegreesToRadians(Angle)));
}

FVector UReadyOrNotMathLibrary::CalculateLocationOnSphere(const FVector Origin, const float Radius, const float Angle, const float Phi)
{
	const float X = Origin.X + Radius * FMath::Sin(Angle) * FMath::Cos(Phi);
	const float Y = Origin.Y + Radius * FMath::Sin(Angle) * FMath::Sin(Phi);
	const float Z = Origin.Z + Radius * FMath::Cos(Angle);

	return FVector(X, Y, Z);
}

FVector UReadyOrNotMathLibrary::GenerateRandomLocationOnSphere(const FVector Origin, const float Radius)
{
	const float Angle = FMath::FRandRange(0.0f, PI);
	const float Phi = FMath::FRandRange(0.0f, 2 * PI);

	const float X = Origin.X + Radius * FMath::Sin(Angle) * FMath::Cos(Phi);
	const float Y = Origin.Y + Radius * FMath::Sin(Angle) * FMath::Sin(Phi);
	const float Z = Origin.Z + Radius * FMath::Cos(Angle);

	return FVector(X, Y, Z);
}

float UReadyOrNotMathLibrary::WrapAngleIfOutOfBounds(const float Angle, const float MinBounds, const float MaxBounds)
{
	if (Angle > MaxBounds)
		return Angle - MaxBounds;

	if (Angle < MinBounds)
		return Angle + MaxBounds;

	if (MinBounds == MaxBounds)
		return MinBounds;

	if (MinBounds > MaxBounds)
		return Angle;

	if (MaxBounds < MinBounds)
		return Angle;

	return Angle;
}

float UReadyOrNotMathLibrary::KeepAngleBelow360(const float Angle)
{
	return Angle > 360.0f ? Angle - 360.0f : Angle;
}

float UReadyOrNotMathLibrary::KeepAngleAbove0(const float Angle)
{
	return Angle < 0.0f ? Angle + 360.0f : Angle;
}

float UReadyOrNotMathLibrary::GetLastKeyTime_FloatCurve(const FRuntimeFloatCurve& InCurve)
{
	float LastKeyTime = 0.0f;
	
	if (InCurve.ExternalCurve)
	{
		LastKeyTime = InCurve.ExternalCurve->FloatCurve.Keys.Last().Time;
	}
	else if (InCurve.EditorCurveData.GetNumKeys() > 1)
	{
		LastKeyTime = InCurve.EditorCurveData.Keys.Last().Time;
	}

	return LastKeyTime;
}

float UReadyOrNotMathLibrary::GetLastKeyTime_VectorCurve(const FRuntimeCurveLinearColor& InCurve)
{
	float LastKeyTime = 0.0f;

	if (InCurve.ExternalCurve)
	{
		for (int32 j = 0; j < UE_ARRAY_COUNT(InCurve.ExternalCurve->FloatCurves); j++)
		{
			const float Local_LastKeyTime = InCurve.ExternalCurve->FloatCurves[j].Keys.Last().Time;

			if (Local_LastKeyTime > LastKeyTime)
				LastKeyTime = Local_LastKeyTime;
		}
	}
	else
	{
		for (int32 j = 0; j < UE_ARRAY_COUNT(InCurve.ColorCurves); j++)
		{
			const float Local_LastKeyTime = InCurve.ColorCurves[j].Keys.Last().Time;
			
			if (Local_LastKeyTime > LastKeyTime)
				LastKeyTime = Local_LastKeyTime;
		}
	}

	return LastKeyTime;
}

float UReadyOrNotMathLibrary::EaseAlpha(float InAlpha, uint8 EasingFunc, float BlendExp, int32 Steps)
{
	switch (EasingFunc)
	{
		case EEasingFunc::Linear:				return FMath::Pow(FMath::Lerp<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::Step:					return FMath::Pow(FMath::InterpStep<float>(0.f, 1.f, InAlpha, Steps), BlendExp);
		case EEasingFunc::SinusoidalIn:			return FMath::Pow(FMath::InterpSinIn<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::SinusoidalOut:		return FMath::Pow(FMath::InterpSinOut<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::SinusoidalInOut:		return FMath::Pow(FMath::InterpSinInOut<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::EaseIn:				return FMath::Pow(FMath::InterpEaseIn<float>(0.f, 1.f, InAlpha, 1.0f), BlendExp);
		case EEasingFunc::EaseOut:				return FMath::Pow(FMath::InterpEaseOut<float>(0.f, 1.f, InAlpha, 1.0f), BlendExp);
		case EEasingFunc::EaseInOut:			return FMath::Pow(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 1.0f), BlendExp);
		case EEasingFunc::ExpoIn:				return FMath::Pow(FMath::InterpExpoIn<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::ExpoOut:				return FMath::Pow(FMath::InterpExpoOut<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::ExpoInOut:			return FMath::Pow(FMath::InterpExpoInOut<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::CircularIn:			return FMath::Pow(FMath::InterpCircularIn<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::CircularOut:			return FMath::Pow(FMath::InterpCircularOut<float>(0.f, 1.f, InAlpha), BlendExp);
		case EEasingFunc::CircularInOut:		return FMath::Pow(FMath::InterpCircularInOut<float>(0.f, 1.f, InAlpha), BlendExp);
		default:								return InAlpha;
	}
}
