// � Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "TugOfWarMover.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ATugOfWarMover : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATugOfWarMover();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mover, meta = (AllowPrivateAccess = "true"))
		USplineComponent* MoverPath;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mover, meta = (AllowPrivateAccess = "true"))
		USkeletalMeshComponent* MoverMesh;

	float AnnouncerDelay = 0.0f;
	FTimerHandle AnnounceCapturing_Handle;

	// This is relative to the distance of the spline, so 1.0 = cover the entire distance of the spline in one second!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover)
		float MoverSpeed = 0.003f;

	// If true, the 0th spline point causes a victory for Blue; else it causes a victory for red
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover)
		bool bInvertVictoryPositions = false;

	// Whether the mover is currently moving forwards (increasing spline point)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Mover)
		bool bMoverForward = true;

	// Whether the mover is moving
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Mover)
		bool bMoverMoving = false;

	// The starting point of the mover (0.5 = halfway)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Mover)
		float MoverStartingPosition = 0.5f;

	// The current position of the mover
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Mover)
		float MoverCurrentPosition = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Mover)
	bool bContested = false;

	// Update the movement of the mover based on how many influencers there are (server only call!)
	void UpdateMovement();

	ETeamType LastAnnouncedTeamType = ETeamType::TT_NONE;

	// The people who are currently using this mover
	UPROPERTY(BlueprintReadOnly, Category = Mover)
		TArray<class APlayerCharacter*> Influencers;

	// In the editor we move the mesh along with the spline
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	void RepositionMesh();
#endif	

	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return MoverMesh; }
	FORCEINLINE class USplineComponent* GetPath() const { return MoverPath; }
};