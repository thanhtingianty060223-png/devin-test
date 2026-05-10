// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimTurnInPlaceLibrary.h"

void UAnimTurnInPlaceLibrary::UpdateTurnInPlace(float DeltaTime, bool bAllowTurnInPlace, bool bHoldYawOffset, bool bIsTurnTransitionStateRelevant, bool bClampYawOffset, float YawOffsetLimit,
	const FRotator& MeshWorldRotation, const FAnimTurnInPlaceAnimSet& AnimSet, UPARAM(ref) FAnimTurnInPlaceState& TurnInPlaceState, float TurnInPlaceSpeedMultiplier)
{
	TurnInPlaceState.Update(DeltaTime, bAllowTurnInPlace, bHoldYawOffset, bIsTurnTransitionStateRelevant, bClampYawOffset, YawOffsetLimit, MeshWorldRotation, AnimSet, TurnInPlaceState, TurnInPlaceSpeedMultiplier);
}

/* post process the turn in place yaw offset to prevent 180 degree flips when reaching boundaries */
void UAnimTurnInPlaceLibrary::PostProcessYawOffset(float DeltaSeconds, UPARAM(ref) float& YawOffset, UPARAM(ref) float& LastYawOffset, UPARAM(ref) float& LastPostProcessedYawOffset, UPARAM(ref) float& TurnAroundTimeToGo, float TurnAroundBlendTime)
{
	// Save original AimOffset passed in.
	const float OriginalYawOffset = YawOffset;

	// If wrapping and switching abruptly from once side to the other, trigger smooth interpolation
	// only apply corrections on large aim range
	if (FMath::Abs(OriginalYawOffset) > 140.0f)
	{
		if ((YawOffset * LastYawOffset) < 0.f && FMath::Abs(YawOffset - LastYawOffset) > 0.5f)
		{
			TurnAroundTimeToGo = TurnAroundBlendTime;
		}
	}

	// Perform interpolation
	if (TurnAroundTimeToGo > 0.f)
	{
		if (TurnAroundTimeToGo > DeltaSeconds)
		{
			TurnAroundTimeToGo -= DeltaSeconds;
			const float Delta = YawOffset - LastPostProcessedYawOffset;
			const float BlendDelta = Delta * FMath::Clamp((DeltaSeconds / TurnAroundTimeToGo), 0.f, 1.f);
			YawOffset = LastPostProcessedYawOffset + BlendDelta;
		}
		else
		{
			TurnAroundTimeToGo = 0.f;
		}
	}

	// Save our aimoffset values to use for next frame.
	LastPostProcessedYawOffset = YawOffset;
	LastYawOffset = OriginalYawOffset;
}