// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "FlankingCombatMove.generated.h"

UCLASS()
class READYORNOT_API UFlankingCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UFlankingCombatMove();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual float GetDestinationTolerance() const override;

	virtual void ResetData() override;
	
	virtual void RequestCombatMove(float DeltaTime) override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;

	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
protected:
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;

	void UpdateFlankingVolume(const FVector& InFlankLocation);

	void OnFlankPathFail(FNavPathSharedPtr NavPath);

	UPROPERTY()
	AReadyOrNotCharacter* FlankingAgainstCharacter = nullptr;
	
	UPROPERTY()
	class AFlankingAvoidanceVolume* FlankingAvoidanceVolume = nullptr;

	FVector FlankAgainstLocation = FVector::ZeroVector;

	float TimeSinceFinishedFlanking = 0.0f;
	float RegainedLOSTime = 0.0f;
	float RequiredRegainedLOSTime = 0.25f;
	float FlankPathExposure = 1.0f;
	float FlankVolumeWidth = 500.0f;
	float DistanceRemainingOnFlankPath = 0.0f;

	FVector FlankLocation = FVector::ZeroVector;

	#if !UE_BUILD_SHIPPING
	FString LastFlankPathFailReason = "";
	#endif
	
	uint32 FlankPathFailCount = 0;
	
	uint8 bHasStartedFlanking : 1;
	uint8 bHasCompletedFlank : 1;
	uint8 bHasBrokenLOS : 1;
};
