// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Actors/Vehicles/UnmannedVehicle.h"
#include "Components/TimelineComponent.h"
#include "Drone.generated.h"

UENUM(BlueprintType)
enum class EDroneDamageSpeed : uint8
{
	DDS_10PercentSpeed	UMETA(DisplayName="Over 10% Speed"),
	DDS_20PercentSpeed	UMETA(DisplayName="Over 20% Speed"),
	DDS_30PercentSpeed	UMETA(DisplayName="Over 30% Speed"),
	DDS_40PercentSpeed	UMETA(DisplayName="Over 40% Speed"),
	DDS_50PercentSpeed	UMETA(DisplayName="Over 50% Speed"),
	DDS_60PercentSpeed	UMETA(DisplayName="Over 60% Speed"),
	DDS_70PercentSpeed	UMETA(DisplayName="Over 70% Speed"),
	DDS_80PercentSpeed	UMETA(DisplayName="Over 80% Speed"),
	DDS_90PercentSpeed	UMETA(DisplayName="Over 90% Speed"),
};

/**
 * An unmanned aerial vehicle that can be controlled without a pilot on board.
 */
UCLASS()
class READYORNOT_API ADrone : public AUnmannedVehicle
{
	GENERATED_BODY()

public:
	ADrone();

	// Returns TPCameraArm subobject
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return TPCameraArm; }
	// Returns TPCamera subobject
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return TPCamera; }
	// Returns Mesh subobject
	FORCEINLINE class USkeletalMeshComponent* GetSkeletalMesh() const { return Mesh; }
	// Returns FloatingMovementComponent subobject
	FORCEINLINE class UFloatingPawnMovement* GetCustomMovementComponent()  { return FloatingMovementComponent; }
	// Returns FlightBox subobject
	FORCEINLINE class UBoxComponent* GetFlightBox()  { return FlightBox; }
	// Returns DetectionSphere subobject
	FORCEINLINE class USphereComponent* GetDetectionSphere()  { return DetectionSphere; }

	// Returns the current distance to the pilot
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetDistanceToPilot() const { return CurrentPilotDistance; }

	// Returns the drone controller object
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE AReadyOrNotPlayerController* GetDroneController() const { return DroneController; }

	// Returns true if the drone is moving forward
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsMovingForward() const { return ForwardInput > 0.0f; }

	// Returns true if the drone is moving backward
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsMovingBackward() const { return ForwardInput < 0.0f; }

	// Returns true if the drone is moving right
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsMovingRight() const { return RightInput > 0.0f; }

	// Returns true if the drone is moving left
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsMovingLeft() const { return RightInput < 0.0f; }

	// Returns true if we are applying input in any direction
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsApplyingInput() const { return ForwardInput != 0.0f || RightInput != 0.0f; }

	// Returns true if we are stabilized/steady
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsStabilized() const { return bSteadyDrone; }
	
	// Returns true if we are in third person mode
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool IsInThirdPersonMode() const { return bDroneThirdPerson; }

	// Returns true if we have a pilot controlling this drone
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE bool HasPilot() const { return Pilot != nullptr; }
	
	// Returns true if we are moving in any direction
	UFUNCTION(BlueprintPure, Category = "Drone")
	bool IsMoving() const;

	// Returns the last damage information
	UFUNCTION(BlueprintCallable, Category = "Drone")
	void RetrieveLastHitDamageInfo(EDroneDamageSpeed& InDroneDamageSpeed, float& InDamageAmount) const;

	// Returns the current altitude from the ground
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetAltitude() const { return CurrentAltitude; }

	// Returns the current RPM value
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetCurrentRPM() const { return RPM; }

	// Returns the maximum RPM value
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetMaxRPM() const { return MaxRPM; }
	
	// Returns the idle RPM value
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetIdleRPM() const { return IdleRPM; }

	// Returns the maximum drone speed value
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetMaxSpeed() const { return MaxSpeed; }
	
	// Returns the minimum drone speed value
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE float GetMinSpeed() const { return MinSpeed; }

	// Returns true if we are in invincible mode (Is the invincible timer active?)
	UFUNCTION(BlueprintPure, Category = "Drone")
	bool IsInvincible() const;

	// Returns the current drone's speed as a percentage of the maximum speed
	UFUNCTION(BlueprintPure, Category = "Drone")
	float GetCurrentSpeedAsPercentage() const;

	// Returns the current movement direction
	UFUNCTION(BlueprintPure, Category = "Drone")
	FORCEINLINE FVector GetCurrentMovementDirection() const { return MovementDirection; }
	
protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// Called for forwards/backward input
	void MoveForward(float Value);

	// Called for side to side input
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	void AddControllerYawInput(float Value) override;
	void AddControllerPitchInput(float Value) override;

	void Throttle(float Rate);

	void Rotate(float Value);

	void TurnRight(float Value);

	static FString DroneDamageSpeedEnumToString(const EDroneDamageSpeed& DroneDamageSpeed);

	UFUNCTION(BlueprintCallable, Category = "Drone")
	void SteadyDrone();

	UFUNCTION(BlueprintCallable, Category = "Drone")
	void ToggleThirdPerson();

	UFUNCTION(BlueprintCallable, Category = "Drone")
	void QuickTurn();

	UFUNCTION(BlueprintCallable, Category = "Drone")
	void ExitDrone();

	UFUNCTION(BlueprintCallable, Category = "Drone")
	void IncrementSpeed(float Value);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void Server_UpdateDrone(FTransform NewTransform, float InRPM);
	virtual void Server_UpdateDrone_Implementation(FTransform NewTransform, float InRPM);
	virtual bool Server_UpdateDrone_Validate(FTransform NewTransform, float InRPM) { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
	void Client_UpdateDrone(FTransform NewTransform, float InRPM);
	virtual void Client_UpdateDrone_Implementation(FTransform NewTransform, float InRPM);
	virtual bool Client_UpdateDrone_Validate(FTransform NewTransform, float InRPM) { return true; }

	UFUNCTION()
	void UpdatePilotingInfo();

	UFUNCTION()
	void OnDroneHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION()
	void OnDetectionSphereOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void Tick_CameraReset();

	UFUNCTION()
	void Tick_CameraDamage();
	
	UFUNCTION()
	void Finished_CameraDamage();

	UFUNCTION(BlueprintPure, Category = "Drone")
	bool IsSpeedThresholdMet(float InSpeedAsPercentage);

	// The controller class to use when piloting the drone
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	//TSubclassOf<AReadyOrNotPlayerController> DroneControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	TSubclassOf<class UUserWidget> DroneWidgetClass;

	// The curve asset to use when exiting steady mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	UCurveFloat* FPCameraRotationCurve;

	// The speed at which the camera resets when exiting steady mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float FPCameraRotationResetSpeed = 1.0f;

	// The curve asset to use when the drone has been damaged by going to too fast
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	UCurveVector* FPDamageCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float FPDamageSpeed = 1.0f;

	// The maximum amount of pitch rotation to apply when moving forwards or backwards (Purely cosmetic)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxPitchTilt = 30.0f;

	// The maximum amount of roll rotation to apply when moving left or right (Purely cosmetic)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxRollTilt = 30.0f;

	// The maximum rounds per minute the drone's propellers are allowed to spin at (Purely cosmetic, higher or lower values do not affect the drone's movement speeds)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxRPM = 2000.0f;

	// The default rounds per minute the drone's propellers will spin at (Purely cosmetic, higher or lower values do not affect the drone's movement speeds)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float IdleRPM = 800.0f;

	// Adds to the RPM value when throttling input is applied in either direction (Higher values = Faster drone movements, Lower values = Slower drone movements)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RPMThrottleMultiplier = 100.0f;

	// The speed of which we interpolate at when throttling input is applied in either direction (Higher values = Faster interp speed, Lower values = Slower interp speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float ThrottleInterpSpeed = 20.0f;

	// The speed of which we interpolate at when turning the drone left or right in non-steady mode (Higher values = Faster interp speed, Lower values = Slower interp speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RotationInterpSpeed = 5.0f;

	// The speed of which we turn the drone at when in non-steady mode (Higher values = Faster rotation speed, Lower values = Slower rotation speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float TurnSpeed = 5.0f;

	// The speed of which we interpolate at when turning the drone left or right in steady mode (Higher values = Faster interp speed, Lower values = Slower interp speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float RotationInterpSpeedWhenSteady = 5.0f;

	// The speed of which we turn the drone at when in steady mode (Higher values = Faster rotation speed, Lower values = Slower rotation speed)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float TurnSpeedWhenSteady = 3.0f;

	// The minimum speed of the drone (Higher values = Fast drone speeds, Lower values = Slow drone speeds)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MinSpeed = 300.0f;

	// The maximum speed of the drone (Higher values = Fast drone speeds, Lower values = Slow drone speeds)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float MaxSpeed = 1200.0f;

	// Adds/Subtracts to the drone's current speed when using the incremental system input (Higher values = Fast increment rates, Lower values = Slow increment rates)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float SpeedIncrementRate = 20.0f;

	// Maps drone's speed to the damage amount that should be applied
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	TMap<EDroneDamageSpeed, float> DroneSpeedToDamageValues;

	// When the drone has been hit, wait the specified amount of time (in seconds) before applying damage again (Prevents multiple TakeDamage calls)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings")
	float InvincibilityTimeAfterDamageApplied = 1.0f;

	// The current rotation of all 4 of the drone's propellers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone | Settings", Replicated)
	FRotator RotorRotation;
	
	// The current rounds per minute the drone's propellers are operating at
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	float RPM = 0.0f;

	// The current altitude from the ground
	UPROPERTY(BlueprintReadOnly, Category = "Drone | Info")
	float CurrentAltitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Drone | Info")
	float CurrentPilotDistance = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	FTransform DroneTransform;
	
	UPROPERTY(Replicated)
	FRotator TargetRotation = FRotator::ZeroRotator;

	UPROPERTY(Replicated)
	FRotator TargetSteadyCameraRotation = FRotator::ZeroRotator;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	bool bApplyingInput;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone | Info")
	bool bSteadyDrone;

	UPROPERTY(BlueprintReadOnly, Category = "Drone | Info")
	bool bDroneThirdPerson;

	UPROPERTY(BlueprintReadOnly, Category = "Drone")
	UWorld* World;
	
	UPROPERTY(BlueprintReadOnly, Category = "Drone")
	AReadyOrNotPlayerController* DroneController;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Drone")
	AReadyOrNotPlayerController* OriginalController;

	FVector MovementDirection;
	FVector LastHitLocation;

	bool bNegativeRollValue;

private:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* FlightBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* DetectionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class UFloatingPawnMovement* FloatingMovementComponent;
	
	// Camera boom positioning the camera behind the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* TPCameraArm;

	// Third person follow camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TPCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* Audio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone | Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPCamera;

	UPROPERTY(BlueprintReadOnly, Category = "Drone", meta = (AllowPrivateAccess = "true"))
	class UUserWidget* DroneWidgetHUD;

	FTimeline TL_ResetFPCameraRotation;
	FTimeline TL_Damage;

	FTimerHandle TH_InvincibilityPeriod;

	float ForwardInput = 0.0f;
	float RightInput = 0.0f;

	// Drone damage info
	EDroneDamageSpeed LastDroneDamageSpeed;
	float LastDroneDamageAmount = 0.0f;

	float DotProductOnDroneHit = 0.0f;
};
