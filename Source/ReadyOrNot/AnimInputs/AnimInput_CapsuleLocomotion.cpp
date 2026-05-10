// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimInput_CapsuleLocomotion.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
//#include "KismetAnimationLibrary.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace AnimInputConsoleCommands
{
	static int32 CapsuleLocomotionDebugText = 0;
	static FAutoConsoleVariableRef CVarCapsuleLocomotionDebugText(
		TEXT("a.AnimInput.CapsuleLocomotion.DebugText"), CapsuleLocomotionDebugText,
		TEXT("0: Disable debug, 1: Enable debug"),
		ECVF_Default);

	static int32 CapsuleLocomotionDebugLines = 0;
	static FAutoConsoleVariableRef CVarCapsuleLocomotionDebugLines(
		TEXT("a.AnimInput.CapsuleLocomotion.DebugLines"), CapsuleLocomotionDebugLines,
		TEXT("0: Disable debug, 1: Enable debug, 2: Enable debug with legend"),
		ECVF_Default);
}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

float FAnimInput_CapsuleLocomotion::CalculateDirectionInput(const FVector& Velocity, const FRotator& BaseRotation) const
{
	if (!Velocity.IsNearlyZero())
	{
		FMatrix RotMatrix = FRotationMatrix(BaseRotation);
		FVector ForwardVector = RotMatrix.GetScaledAxis(EAxis::X);
		FVector RightVector = RotMatrix.GetScaledAxis(EAxis::Y);
		FVector NormalizedVel = Velocity.GetSafeNormal2D();

		// get a cos(alpha) of forward vector vs velocity
		float ForwardCosAngle = FVector::DotProduct(ForwardVector, NormalizedVel);
		// now get the alpha and convert to degree
		float ForwardDeltaDegree = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

		// depending on where right vector is, flip it
		float RightCosAngle = FVector::DotProduct(RightVector, NormalizedVel);
		if (RightCosAngle < 0)
		{
			ForwardDeltaDegree *= -1;
		}

		return ForwardDeltaDegree;
	}

	return 0.f;
}

void FAnimInput_CapsuleLocomotion::Update(const APawn* Pawn)
{
	const UCharacterMovementComponent* MovementComponent = (Pawn != nullptr) ? Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent()) : nullptr;
	if (MovementComponent == nullptr)
	{
		// Prevent preview from always falling.
		bIsOnGround = true;

		return;
	}

	const FTransform PawnWorldTransform = Pawn->GetActorTransform();
	const FRotator PawnRotation = Pawn->GetActorRotation();

	// Velocity

	WorldVelocity = Pawn->GetVelocity();
	LocalVelocity = PawnWorldTransform.InverseTransformVectorNoScale(WorldVelocity);
	VelocityYawAngle = CalculateDirectionInput(WorldVelocity, PawnRotation);
	
	Speed2D = WorldVelocity.Size2D();
	bIsMoving2D = (Speed2D > MovingThreshold);

	// Acceleration

	WorldAcceleration = MovementComponent->GetCurrentAcceleration();
	LocalAcceleration = PawnWorldTransform.InverseTransformVectorNoScale(WorldAcceleration);
	AccelerationYawAngle = CalculateDirectionInput(WorldAcceleration, PawnRotation);

	bHasAcceleration2D = !FMath::IsNearlyZero(WorldAcceleration.SizeSquared2D());

	const FVector LocalVelocity2D = FVector(LocalVelocity.X, LocalVelocity.Y, 0.0f);
	const FVector LocalAcceleration2D = FVector(LocalAcceleration.X, LocalAcceleration.Y, 0.0f);
	bAccelerationOpposesVelocity = FVector::DotProduct(LocalAcceleration2D, LocalVelocity2D) < 0.0f;

	// Movement Mode

	bIsOnGround = MovementComponent->IsMovingOnGround();

	// Debug

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (AnimInputConsoleCommands::CapsuleLocomotionDebugText >= 1)
	{
		FString DebugString = FString::Printf(TEXT(
			"WorldVelocity(%s) | LocalVelocity(%s)\n"
			"WorldAcceleration(%s) | LocalAcceleration(%s)\n"
			"VelocityYawAngle(%.3f) | Speed2D(%.3f) | bIsMoving2D(%d)\n"
			"AccelerationYawAngle(%.3f) | bHasAcceleration2D(%d) | bAccelerationOpposesVelocity(%d)\n"
			"bIsOnGround(%d)\n"),
			*WorldVelocity.ToString(), *LocalVelocity.ToString(),
			*WorldAcceleration.ToString(), *LocalAcceleration.ToString(),
			VelocityYawAngle, Speed2D, bIsMoving2D,
			AccelerationYawAngle, bHasAcceleration2D, bAccelerationOpposesVelocity,
			bIsOnGround);
		
		DrawDebugString(Pawn->GetWorld(), PawnWorldTransform.GetLocation() + FVector(0.0f, 0.0f, 100.0f), DebugString, NULL, FColor::White, 0.0f, true, 1.2f);
	}

	const FColor VelocityColor = FColor::Red;
	const FColor AccelerationColor = FColor::Cyan;
	const FColor FacingDirColor = FColor::Black;

	if (AnimInputConsoleCommands::CapsuleLocomotionDebugLines >= 1)
	{
		const FVector DrawLocation = PawnWorldTransform.GetLocation() - FVector(0.0f, 0.0f, Pawn->GetSimpleCollisionHalfHeight());

		const float ArrowLengthMultiplier = 0.1f;
		const float ArrowSize = 40.0f;
		const float ArrowThickness = 5.0f;

		DrawDebugDirectionalArrow(Pawn->GetWorld(), DrawLocation, DrawLocation + WorldVelocity * ArrowLengthMultiplier, ArrowSize, VelocityColor, false, 0.0f, 0, ArrowThickness);
		DrawDebugDirectionalArrow(Pawn->GetWorld(), DrawLocation, DrawLocation + WorldAcceleration * ArrowLengthMultiplier, ArrowSize, AccelerationColor, false, 0.0f, 0, ArrowThickness);
		DrawDebugDirectionalArrow(Pawn->GetWorld(), DrawLocation, DrawLocation + PawnWorldTransform.GetUnitAxis(EAxis::X) * 35.0f, ArrowSize, FacingDirColor, false, 0.0f, 0, ArrowThickness * 2.0f);
	}

	if (AnimInputConsoleCommands::CapsuleLocomotionDebugLines >= 2)
	{
		const float TextScale = 1.2f;
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, VelocityColor, TEXT("Velocity"), false, FVector2D::UnitVector * TextScale);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, AccelerationColor, TEXT("Acceleration"), false, FVector2D::UnitVector * TextScale);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FacingDirColor, TEXT("FacingDir"), false, FVector2D::UnitVector * TextScale);
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UAnimInputCapsuleLocomotionBlueprintLibrary::UpdateCapsuleLocomotionAnimInput(const APawn* Pawn, FAnimInput_CapsuleLocomotion& CapsuleLocomotionAnimInput)
{
	CapsuleLocomotionAnimInput.Update(Pawn);
}