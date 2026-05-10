// Copyright Void Interactive, 2023

#pragma once

//#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"
//#include "Navigation/NavMeshSplinePath.h"
#include "ReadyOrNotPathFollowingComp.generated.h"

class UNavLinkCustomComponent;

DECLARE_STATS_GROUP(TEXT("Ready Or Not Path Following"), STATGROUP_ReadyOrNotPathFollowing, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UReadyOrNotPathFollowingComp final : public UPathFollowingComponent
{
	GENERATED_BODY()
	
public:
	bool IsUsingCustomLink() const;
	bool IsUsingThisDoorLink(ADoor* Door) const;
	
	FVector GetNavLinkDestination() const;

	bool DoesCurrentPathGoThroughDoor(ADoor* Door) const;
	bool DoesPathGoThroughDoor(FNavPathSharedPtr NavPath, ADoor* Door) const;

	//FORCEINLINE bool IsUsingSplinePathing() const { return bUsingSplinePath; }
	//FORCEINLINE const FSplineCurves* GetSplineCurvePath() const { return &SplineCurvePath; }

	//FORCEINLINE const FNavMeshSplinePath::FMovementData* GetCurrentPathMovementData() const { return &SplinePathMovementData; }

	//FORCEINLINE float GetAcceleration() const { return Acceleration; }

	float GetCurrentPathProgress() const;

	uint32 GetClosestPathPointFromLocation(const FVector& Location) const;
	
	//bool GetCurrentSplinePathMovementData(FNavMeshSplinePath::FMovementData& OutMovementData, float DistanceOffset = 0.0f) const;
	
protected:
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void SetMoveSegment(int32 SegmentStartIndex) override;

	virtual bool HasReachedCurrentTarget(const FVector& CurrentLocation) const override;

	virtual void OnPathUpdated() override;
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;
	
	virtual void StartUsingCustomLink(INavLinkCustomInterface* CustomNavLink, const FVector& DestPoint) override;
	virtual void FinishUsingCustomLink(INavLinkCustomInterface* CustomNavLink) override;

	virtual FVector GetMoveFocus(bool bAllowStrafe) const override;
	
	//void OnDoorLinkStart(UNavLinkCustomComponent* Link, ADoor* Door);
	void OnDoorLinkFinished(UNavLinkCustomComponent* Link, ADoor* Door);
	
	void OnWallHoleLinkStart(UNavLinkCustomComponent* Link, class AWallHoleTraversal* WallHole);

	virtual void PauseMove(FAIRequestID RequestID, EPathFollowingVelocityMode VelocityMode) override;
	virtual void ResumeMove(FAIRequestID RequestID) override;
	
	UFUNCTION()
	void OnHoleTraversalFinished(class UBaseActivity* Activity, ACyberneticController* Controller);
	
	UPROPERTY(VisibleInstanceOnly, Category = "Hole Traversal")
	float HoleTraversalCooldown = 0.0f;

	float TimeBlocked = 0.0f;

	float TimeSinceLastValidNavLinkDestination = FLT_MAX;
	
	FVector NavLinkDestination = FVector::ZeroVector;
	
	UPROPERTY()
	ADoor* LastUsedDoorLink = nullptr;
	
	UPROPERTY()
	AWallHoleTraversal* LastUsedWallHole = nullptr;
	
	UPROPERTY()
	UNavLinkCustomComponent* LastUsedDoorLinkComp = nullptr;
	
	//UPROPERTY()
	//TMap<ADoor*, float> TimeSinceLastOpenedDoor;

	//UPROPERTY()
	//TMap<ADoor*, float> TimeSinceLastClosedDoor;
	
	uint8 DoorTestIndex = 0;

	uint8 bUsingSplinePath : 1;

	//UPROPERTY(VisibleInstanceOnly)
	//float Acceleration = 0.0f;

	//FSplineCurves SplineCurvePath;
	//FNavMeshSplinePath::FMovementData SplinePathMovementData;
	
	//TArray<FNavPathPoint> RawPath;

	//FVector Seek(const FVector& Target);
};

