// Void Interactive, 2020

#pragma once

#include "Info/Activities/CombatMove/BaseCombatMoveActivity.h"
#include "FleeingCombatMove.generated.h"

UENUM()
enum EFleeType
{
	FT_None,
	FT_Regular,
	FT_Gas
};

UCLASS()
class READYORNOT_API UFleeingCombatMove final : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	UFleeingCombatMove();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool ShouldDisableMoveRequest() const override;
	
	virtual float GetDestinationTolerance() const override;

	virtual FName GetMoveStyleOverride_Implementation() const override;
	
	float CalculateFleeDesire() const;
	
	bool TryFlee();

	bool FleeGas();

	UFUNCTION(BlueprintCallable)
	TEnumAsByte<EFleeType> GetFleeType();

	bool IsDoingFleeMove() const;

	virtual void ResetData() override;

	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;
	
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
protected:
	virtual void RequestCombatMove(float DeltaTime) override;

	virtual bool ShouldPerformCombatMove() const override;

	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;

	void OnPathFoundSuccess(FNavPathSharedPtr NavPath);

	uint8 FleesRemaining = 0;
	
	UPROPERTY()
	TArray<class AThreatAwarenessActor*> UsedFleePoints;

	UPROPERTY()
	TArray<AThreatAwarenessActor*> LastExitPoints;

	UPROPERTY()
	class AThreatAwarenessActor* LastFleePoint;

	TArray<FVector> PreviouslyValidLocations;

	void OnEQSComplete(TSharedPtr<FEnvQueryResult> Result);
	bool bRunningEQS;

	EFleeType CurrentFleeType = FT_None;
};
