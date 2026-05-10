// Copyright Void Interactive, 2017

#pragma once

#include "UnmannedVehicle.h"
#include "DroneVehicle.generated.h"



UCLASS(Blueprintable, BlueprintType)
class ADroneVehicle : public AUnmannedVehicle
{
	GENERATED_BODY()

public:
	ADroneVehicle();

	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void SetupPlayerInputComponent(class UInputComponent* inputComponent) override;

	void AddControllerYawInput(float Val) override;
	void AddControllerPitchInput(float Val) override;

	void Server_StartPiloting_Implementation(AReadyOrNotPlayerController* NewController) override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_MoveForward(float Val);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_Right(float Val);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_Throttle(float Val);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_Yaw(float Val);

	// Yaw
	void Drone_Turn(float Rate);
	void Drone_TurnAtRate(float Rate);

	// Pitch
	void Drone_LookUp(float Rate);
	void Drone_LookUpAtRate(float Rate);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_Steady();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_QuickTurn();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_ToggleThirdPerson();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Drone_Exit();

	void IncrementSpeed(float Val);

	// Replication
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_UpdateDroneTransform(FTransform newTransform);
	virtual void Server_UpdateDroneTransform_Implementation(FTransform NewTransform);
	virtual bool Server_UpdateDroneTransform_Validate(FTransform newTransform) { return true; }

	UFUNCTION()
	void OnRep_DroneMovement();

	UFUNCTION()
	void UpdatePilotingInfo();

	UFUNCTION()
	void OnDroneHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class UBoxComponent* FlightBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class UAudioComponent* Audio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class USpringArmComponent* ThirdPersonSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components")
	class UFloatingPawnMovement* FloatingMovementComponent;

	// Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxTilt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxRPM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float IdleRPM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RPMForceScale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RPMThrottleMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float ThrottleInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RotationInterpSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float TurnSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RotationInterpSpeedWhenSteady = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float TurnSpeedWhenSteady = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MinSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float SpeedIncrementRate = 20.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	//TMap<EDroneDamageSpeed, float> DroneSpeedToDamageValues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float InvincibilityTimeAfterDamageApplied = 1.0f;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Settings")
	float RPM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	FRotator RotorRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Drone | Info")
	float CurrentAltitude;

	UPROPERTY(BlueprintReadOnly, Category = "Drone | Info")
	float CurrentPilotDistance;

	UPROPERTY(ReplicatedUsing = OnRep_DroneMovement, BlueprintReadOnly, Category = "Drone | Info")
	FTransform DroneTransform;

	UPROPERTY()
	FRotator TargetRotation = FRotator::ZeroRotator;

	FTransform ClientDroneTransform;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	bool bApplyingInput;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	bool bSteadyDrone;

	bool bDroneThirdPerson;

	bool bQuickTurnPositive;

	bool bTurningRight;

	float DistanceMovedThisFrame;

	FVector ActorLocationLastFrame;

	FRotator CompensatedDirection;

	UPROPERTY()
	class APlayerCharacter* DroneOwner;

	UPROPERTY()
	UWorld* World;

private:
	FTimerHandle TH_InvincibilityPeriod;

	float ForwardInput, RightInput;
	
	void TurnRight(float Val);

	//FString DroneDamageSpeedEnumToString(const EDroneDamageSpeed& DroneDamageSpeed);
};
