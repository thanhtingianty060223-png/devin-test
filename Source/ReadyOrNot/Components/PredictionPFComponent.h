// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "PredictionPFComponent.generated.h"

UCLASS()
class READYORNOT_API UPredictionPFComponent : public UCrowdFollowingComponent
{
	GENERATED_BODY()

public:
	UPredictionPFComponent(const FObjectInitializer& ObjectInitializer);

	/* main tick function of PF component! */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* update during follow of current path segment */
	virtual void FollowPathSegment(float DeltaTime) override;

	/* if getting stuck this is run by default */
	virtual void OnUnableToMove(const UObject& Instigator) override;

	/** notify about changing current path: new pointer or update from path event */
	virtual void OnPathUpdated() override;

	/* when reaching current segment in path */
	bool HasReachedCurrentTarget(const FVector& CurrentLocation) const override;

	/* when pathing is finished */
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Distance Matching|Stop Matching")
	FVector PathStopLocation;

	/** sets variables related to current move segment */
	virtual void SetMoveSegment(int32 SegmentStartIndex) override;

	/* this fixes the path to make sure the character will always brake even with large acceptance radius */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PathFollowing|Fixups")
	bool bFixPathToAlwaysHaveBraking;

	/* this fixes the path to make sure there are no close points to improve the navigation stability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PathFollowing|Fixups")
	bool bFixPathRemoveClosePoints;

	/* the distance threshold at which we should consider cutting close points  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PathFollowing|Fixups")
	float PathPointRemoveDistanceThreshold;

	/* override the existing acceptance radius using this custom defined radius*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PathFollowing|AcceptanceRadius")
	float CustomAcceptanceRadius;

	virtual void DebugDrawPath(float DeltaTime);
	bool bShowDebug;
	float OriginalAcceptanceRadius;

	TArray<FNavPathPoint> OptimizedPath;

	/* will return the last path point with the radius calculated in */
	FVector GetLastPathPointWithRadius(float PointRadius);

	bool bAppliedAcceptanceRadiusFix;

	virtual void ModifyLastPathPointToIncludeRadius();
};
