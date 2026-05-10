//Void Interactive, 2017

#pragma once

#include "Actors/BaseDeployableGear.h"
#include "TelescopicLadder.generated.h"

/**
 *	The Telescopic Ladder deployable allows the player to traverse up to new heights, or across chasms.
 *	The ladder can only be placed on designated Ladder Snap Zones.
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class READYORNOT_API ATelescopicLadder : public ABaseDeployableGear
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "Ladder")
	USceneComponent* LadderVerticalIconPoint;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "Ladder")
	USceneComponent* LadderHorizontalIconPoint;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "Ladder")
	USceneComponent* LadderBottomMountPoint;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "Ladder")
	USceneComponent* LadderTopMountPoint;

	ATelescopicLadder();

	void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnItemPrimaryUse() override;
	virtual void Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped) override;

	virtual void Reset() override;

	void ResetLadderState();
	void ResetLadderTransform();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlaceLadder();
	virtual void Server_PlaceLadder_Implementation();
	virtual bool Server_PlaceLadder_Validate() { return true; }

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_PlaceLadder();
	virtual void Multicast_PlaceLadder_Implementation();

	UPROPERTY(EditAnywhere)
		TSubclassOf<ALadderSnapZone> LadderSnapZoneBP;

	UPROPERTY(BlueprintReadOnly)
		bool bFreezeFrame = false;

	UPROPERTY(EditAnywhere)
		float MaxRollDegreesBeforeUnwalkable = 30.0f;
	float RollDegrees = 0.0f;
	FHitResult GroundTrace;
	FRotator PlacementRotation;
	UPROPERTY(Replicated)
	FTransform FreezeTransform;
	AActor* SpawnedCollision = nullptr;
	float TimeSinceNotMoving = 0.0f;
	float MovementDelta = 0.0f;
	bool bFrameOne = false;
	FVector LocationFrameOne = FVector::ZeroVector;
	FVector LocationFrameTwo = FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
		bool bShowGhostLadder = true;

	UPROPERTY(EditAnywhere)
		bool bShowCollapsedLadder = true;

	UPROPERTY(EditAnywhere)
		UAnimSequence* CollapsedLadderAnim;

	UPROPERTY(Replicated)
	FTransform LastTransform;
	FTransform GetPlacementTransform();
	void SpawnGhostLadder();
	void DestroyGhostLadder();
	UPROPERTY()
	ASkeletalMeshActor* GhostLadderActor;
	UPROPERTY(EditAnywhere)
		UMaterial* GhostLadderMaterial;


	// Deploy the ladder at a snap zone.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Ladder")
	void Server_DeployLadderAtZone(class ALadderSnapZone* NewSnapZone);
	virtual void Server_DeployLadderAtZone_Implementation(class ALadderSnapZone* NewSnapZone);
	virtual bool Server_DeployLadderAtZone_Validate(class ALadderSnapZone* NewSnapZone) { return true; }

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_DeployLadderAtZone(class ALadderSnapZone* NewSnapZone);
	virtual void Multicast_DeployLadderAtZone_Implementation(class ALadderSnapZone* NewSnapZone);


	UPROPERTY(EditAnywhere, Category = Physics)
		UPhysicsAsset* DroppedPhysics;

	UPROPERTY(EditAnywhere, Category = Physics)
		UPhysicsAsset* PlacedPhysics;

	UPROPERTY(EditAnywhere)
	UFMODEvent* PlacementSoundEvent;

	UPROPERTY(EditAnywhere)
	UFMODEvent* PickupSoundEvent;

	UPROPERTY(EditAnywhere)
	UFMODEvent* CollideSoundEvent;

	// Un-deploy the ladder.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Ladder")
	void Server_RemoveLadder();
	virtual void Server_RemoveLadder_Implementation();
	virtual bool Server_RemoveLadder_Validate() { return true; }

	// Whether the ladder has been deployed.
	UPROPERTY(ReplicatedUsing=OnRep_Deployed, BlueprintReadOnly, Category = "Ladder")
	bool bDeployed = false;

	// Whether the ladder's current position is horizontal (and we should treat it as a walkable surface)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ladder")
	bool bDeployedHorizontal = false;

	// Whether the ladder is currently mounted by a player.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ladder")
	bool bMounted = false;

	// How many rungs to retract (at maximum)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ladder")
		float RetractedRungCount = 0.0f;

	// The current snap zone that this ladder is attached to (if it is deployed)
	UPROPERTY(ReplicatedUsing=OnRep_CurrentSnapZone, BlueprintReadOnly, Category = "Ladder")
	class ALadderSnapZone* CurrentSnapZone = nullptr;

	UFUNCTION()
		void OnRep_CurrentSnapZone();

	// Boolean from line trace which detects if the wall has been found
	UPROPERTY(BlueprintReadOnly, Category = WallDetection)
		bool bWallFound;

	virtual void OnRep_AttachmentRep() override;

	USceneComponent* GetClosestMountPoint(FVector Location);

	UFUNCTION()
		void OnRep_Deployed();

	FTransform OriginalActorTransform = FTransform::Identity;
};
