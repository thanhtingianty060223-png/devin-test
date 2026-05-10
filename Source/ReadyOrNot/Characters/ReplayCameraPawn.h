// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Actors/ReplaySplineActor.h"
#include "Components/ReplaySpringArm.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"
#include "ReplayCameraPawn.generated.h"

UCLASS()
class READYORNOT_API AReplayCameraPawn : public ASpectatorPawn
{
	GENERATED_BODY()

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
	
public:
	AReplayCameraPawn();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
	UCameraComponent* PawnCamera;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
	UReplaySpringArm* SpringArm;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
	UFloatingPawnMovement* FloatingPawnMovement;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Post Process")
	FPostProcessSettings PostProcessSettings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Post Process")
    FPostProcessSettings DefaultPostProcessSettings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gameplay)
	float Sensitivity = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float RollAmount = 0.0f;
	
	bool IsExceedingMaxSpeed(float MaxSpeed) const;
	
	void MoveUp(float Val);
	
	void AddYaw(float Val);
	void AddPitch(float Val);

	void MoveForward(float Val);
	void MoveRight(float Val);

	float SpeedMultiplier = 1.0f;
	void AdjustSpeed(float Val);

	void UpdateLocation(float DeltaTime);

	// Initial max speed of the spectator pawn when we start possession.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug Camera")
	float InitialMaxSpeed = 1200.f;

	// Initial acceleration of the spectator pawn when we start possession.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug Camera")
	float InitialAcceleration = 4000.f;

	// Initial deceleration of the spectator pawn when we start possession.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug Camera")
	float InitialDeceleration = 12000.f;

	// Used as an initial speed multiplier for the character. Plugged into an exponential to get the final speed.
	float SpeedApplicator = 6.7;

	// Current velocity of the pawn.
	FVector Velocity;

	// Current InputAcceleration to be applied for this frame.
	FVector InputAcceleration;

	// Turning boost to allow for changing direction much more responsively.
	float TurningBoost = 8;
};
