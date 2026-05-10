// Void Interactive, 2020

#pragma once

#include "Info/Activities/Team/TeamBaseActivity.h"
#include "TeamFallinActivity.generated.h"

DECLARE_STATS_GROUP(TEXT("FallInActivity"), STATGROUP_FallInActivity, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UTeamFallinActivity final : public UTeamBaseActivity
{
	GENERATED_BODY()
	
public:
	UTeamFallinActivity();
	
	UPROPERTY()
	ASWATCharacter* MyLeader = nullptr;

	virtual void FinishedActivity(bool bSuccess) override;

	virtual float GetDestinationTolerance() const override;
	
	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const override;
	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;
	
	FVector SnakeFallInPosition(AReadyOrNotCharacter* MasterLeader, const TArray<ASWATCharacter*>& SortedSwat, float Spacing = 250.0f);
	FVector SnakeFallInPositionV2(AReadyOrNotCharacter* MasterLeader, const TArray<ASWATCharacter*>& SortedSwat, float Spacing = 250.0f);
	FVector HalfSnakeFallInPosition(AReadyOrNotCharacter* MasterLeader);
	FVector DiamondFallInPosition(AReadyOrNotCharacter* MasterLeader);
	FVector FlockFallInPosition(AReadyOrNotCharacter* MasterLeader);

	virtual void SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated = false) override;

protected:
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual bool CanFinishActivity() const override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void ResumeActivity() override;
	virtual void ResetData() override;
	virtual bool ShouldForceStrafe() const override;

	virtual void GatherDebugString(FString& OutString) override;

	FVector CalculateFallInPosition(EFallInPattern Pattern);

	virtual bool OverrideAvoidanceLocation() const override;
	virtual FVector GetBestAvoidanceLocation(ACyberneticCharacter* OverlappingAI) const override;

	bool bFirstCalculation = true;
};
