// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseItem.h"
#include "Quadrotor.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API AQuadrotor : public ABaseItem
{
	GENERATED_BODY()
	
		UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
		USkeletalMeshComponent* ViewfinderMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Capture, meta = (AllowPrivateAccess = "true"))
	class USceneCaptureComponent2D* SceneCapture2D;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Capture, meta = (AllowPrivateAccess = "true"))
	class UTextureRenderTarget2D* RenderTarget;

public:

	AQuadrotor();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void OnItemPrimaryUse() override;

	virtual void OnItemSecondaryUsed() override;

	virtual void OnItemUseComplete() override;

	virtual bool ConsumeLeanInput(float leanAmount) override;

	virtual bool ConsumeMovementForward(float Val) override;

	virtual bool ConsumeMovementRight(float Val) override;

	virtual bool ConsumeMouseMovement(FRotator RotateVector) override;

	virtual bool ConsumeCrouchInput() override;

	virtual bool ConsumeSprintInput() override;

	virtual bool ConsumeJumpInput() override;

	virtual bool PlayDraw(bool bDrawFirst) override;

	UPROPERTY(BlueprintReadOnly, Category = Drone)
	bool bToggleDroneControl = false;

	UPROPERTY(EditDefaultsOnly, Category = Drone)
		UMaterial* DefaultViewfinderMaterial;

	UPROPERTY()
		UMaterialInstanceDynamic* ViewfinderScreenMaterial;

	UPROPERTY(EditAnywhere, Category = Drone)
		FVector2D LocalPlayerCaptureResolution = FVector2D(640, 480);

	UPROPERTY(EditAnywhere, Category = Drone)
		FVector2D SimulatedPlayerCaptureResolution = FVector2D(64, 48);

	FName AttachSceneCaptureToBone;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SpawnDrone();
	virtual void Server_SpawnDrone_Implementation();
	virtual bool Server_SpawnDrone_Validate() { return true; }

	void ToggleDroneCapture(bool bCapture);

	UPROPERTY(EditDefaultsOnly, Category = Drone)
		FName ViewfinderSocket_Hands;

	UPROPERTY(EditDefaultsOnly, Category = Drone)
		FName ViewfinderSocket_Body;

	UPROPERTY(EditAnywhere, Category = Drone)
		UAnimMontage* ThrowDrone_1P;

	UPROPERTY(EditAnywhere, Category = Drone)
		UAnimMontage* ThrowDrone_3P;

	UPROPERTY(EditAnywhere, Category = Drone)
	TSubclassOf<class AQuadrotorPawn> DronePawnClass;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Drone)
	class AQuadrotorPawn* SpawnedDrone;

	void HideActorsForDrone();

	virtual void OnRep_AttachmentRep() override;

	FORCEINLINE class USkeletalMeshComponent* GetViewfinderMesh() const { return ViewfinderMesh; }
	
	
private:
	bool bScreenOn;
};

/**
*
*/
UCLASS()
class READYORNOT_API AQuadrotorPawn : public APawn
{
	GENERATED_BODY()

		UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* FlightBox;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* DroneMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;


public:

	AQuadrotorPawn();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	void OnPickedUp(APlayerCharacter* Interactor);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Drone)
	float RPM;

	UPROPERTY(BlueprintReadOnly, EditAnywhere,  Category = Drone)
		float RPMThrottleMultiplier = 1.0f;

	UPROPERTY(Replicated)
		bool bApplyingInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Drone)
	float MaxRPM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Drone)
	float IdleRPM;

	// How much force to apply per RPM
	UPROPERTY(EditAnywhere, Category = Drone)
	float RPMForceScale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Drone)
	float MaximumTilt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Drone)
	float MaxVelocity;

	UPROPERTY(BlueprintReadOnly, Category = Drone)
	bool bEngineOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Drone)
		FRotator RotorRotation;

	UPROPERTY(ReplicatedUsing=OnRep_DroneMovement, BlueprintReadOnly, Category = Drone)
		FTransform DroneTransform;

	UPROPERTY()
		FRotator TargetRotation = FRotator::ZeroRotator;

	bool bSteadyDrone = false;

	UFUNCTION(Server, Unreliable, WithValidation)
		void Server_UpdateDroneTransform(FTransform newTransform);
	virtual void Server_UpdateDroneTransform_Implementation(FTransform newTransform);
	virtual bool Server_UpdateDroneTransform_Validate(FTransform newTransform) { return true; }

	FTransform ClientDroneTransform;


	UFUNCTION()
	void OnRep_DroneMovement();

	void DroneThrottle(float Val);

	void DroneForward(float Val);

	void DroneRight(float Val);

	void DroneYaw(float Val);


	FORCEINLINE class UBoxComponent* GetFlightBox() const { return FlightBox; }
	FORCEINLINE class USkeletalMeshComponent* GetDroneMesh() const { return DroneMesh; }
	FORCEINLINE class UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

};
