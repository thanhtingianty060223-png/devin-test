// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LadderSnapZone.generated.h"

/*
 *	Ladder Snap Zones are designated places where ladders can be placed.
 *	The Ghost Zone of the ladder will be where the ladder is deployed. 
 *	The Highlight Zone is where the player will need to be aiming at in order to place the ladder.
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ALadderSnapZone : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
	UBoxComponent* SelectionZone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
	USkeletalMeshComponent* GhostLadder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
	UBoxComponent* Collision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
	USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
		UMaterial* ValidPlacementMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
		UMaterial* InvalidPlacementMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder Zone")
		float MaxRetractedRungCount = 0.0f;

	ALadderSnapZone();
	virtual void BeginPlay() override;

	// Whether this ladder zone is horizontal. If it is, the behavior will be to walk on the ladder,
	// instead of climbing on it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder Zone")
	bool bHorizontal;

	// Start showing the ghost mesh.
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, WithValidation, Category = "Ladder Zone")
	void Multicast_StartShowingGhostMesh(bool bAbleToPlace);
	virtual void Multicast_StartShowingGhostMesh_Implementation(bool bAbleToPlace);
	virtual bool Multicast_StartShowingGhostMesh_Validate(bool bAbleToPlace) { return true; }

	// Stop showing the ghost mesh.
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, WithValidation, Category = "Ladder Zone")
	void Multicast_StopShowingGhostMesh();
	virtual void Multicast_StopShowingGhostMesh_Implementation();
	virtual bool Multicast_StopShowingGhostMesh_Validate() { return true; }

	// The current ladder that is propped up on this zone.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ladder")
	class ATelescopicLadder* AttachedLadder;

	// Whether there is something in this zone that is preventing us from placing the ladder
	UPROPERTY(BlueprintReadOnly, Category = "Ladder")
	bool bUnableToPlace = false;

	// The number of alive actors that are overlapping with our collision
	UPROPERTY(BlueprintReadOnly, Category = "Ladder")
	int32 NumberOverlappers = 0;

	// The collision area started overlapping with something.
	UFUNCTION()
	void OnCollisionOverlapBegin(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult & SweepResult);

	// The collision area stopped overlapping with something.
	UFUNCTION()
	void OnCollisionOverlapEnd(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Enable collision (after a ladder has been placed in this area)
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	void EnableCollision();

	// Disable collision (after a ladder has been removed in this area)
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	void DisableCollision();
};
