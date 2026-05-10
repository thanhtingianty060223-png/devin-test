// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "SearchLandmarkActivity.generated.h"

UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API USearchLandmarkActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	USearchLandmarkActivity();

	UFUNCTION(BlueprintNativeEvent)
	void Notify_SearchLandmark();
	void Notify_SearchLandmark_Implementation();

	UPROPERTY()
	ACoverLandmark* CoverLandmark = nullptr;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual float GetDestinationTolerance() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool CanShoot() const override { return true; }
	virtual bool CanFinishActivity() const override { return false; }
	virtual bool CanBePushed() const override;
	virtual void RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath) override;

	virtual void ResetData() override;

	UFUNCTION()
	void EnterMoveToStage();
	UFUNCTION()
	void EnterSearchStage();
	UFUNCTION()
	void TickSearchStage(float DeltaTime, float Uptime);
	UFUNCTION()
	void EnterCompleteStage();
	
	UFUNCTION()
	bool CanSearchNow() const;
	UFUNCTION()
	bool CanAbortSearch() const;
	
	UFUNCTION()
	bool IsSearchFinished() const;

	FString SearchAnimation = "";

	bool bSearchStarted = false;
	bool bSeenAIExiting = false;
	bool bPlayedVO = false;
};
