// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "FMODAudioComponent.h"
#include "Elevator.generated.h"

UCLASS()
class READYORNOT_API AElevator : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AElevator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual bool ShouldTickIfViewportsOnly() const override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Root, meta = (AllowPrivateAccess = "true"))
		USplineComponent* ElevatorPath;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Elevator)
		bool bMoveElevator = false;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Elevator)
		bool bCloseDoors = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ElevatorMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
		UFMODAudioComponent* DestinationReachedSound;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
		UFMODAudioComponent* FloorReachedSound;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
		UFMODAudioComponent* DoorOpenSoundFMOD;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
		UFMODAudioComponent* DoorCloseSoundFMOD;

	UPROPERTY(BlueprintReadOnly, Category = Elevator)
	FVector DefaultElevatorLoc;

	UPROPERTY(BlueprintReadOnly, Category = Elevator)
	int32 Floors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Elevator)
		float ElevatorSpeed = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = Elevator)
	int32 currentFloor;

	// The delay after selecting the floor of the elevator before moving
	UPROPERTY(EditAnywhere, Category = Elevator)
		float MoveElevatorDelay = 1.5f;

	UPROPERTY()
		FTimerHandle MoveElevatorDelay_Handle;

	UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = Elevator)
		int32 SelectedFloor;

	UFUNCTION(BlueprintCallable, Category = Elevator)
		void StartMovingElevator();

	UFUNCTION(BlueprintCallable, Category = Elevator)
		void SetSelectedFloor(int32 Floor);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SetSelectedFloor(int32 Floor);
	virtual void Server_SetSelectedFloor_Implementation(int32 Floor);
	virtual bool Server_SetSelectedFloor_Validate(int32 Floor) { return true; }

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayDoorOpenSound();
	virtual void Multicast_PlayDoorOpenSound_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayDoorCloseSound();
	virtual void Multicast_PlayDoorCloseSound_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayFloorReachedSound();
	virtual void Multicast_PlayFloorReachedSound_Implementation();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlayDestinationReachedSound();
	virtual void Multicast_PlayDestinationReachedSound_Implementation();

	FORCEINLINE class USkeletalMeshComponent* GetElevatorMesh() const { return ElevatorMesh; }
	FORCEINLINE class USplineComponent* GetPath() const { return ElevatorPath; }

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_OpenCloseDoors(bool bShouldCloseDoors);
	virtual void Server_OpenCloseDoors_Implementation(bool bShouldCloseDoors);
	virtual bool Server_OpenCloseDoors_Validate(bool bShouldCloseDoors) { return true; }
};
