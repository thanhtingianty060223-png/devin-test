// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "PickUpCharacterActivity.generated.h"

UCLASS()
class READYORNOT_API UPickUpCharacterActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UPickUpCharacterActivity();

	UPROPERTY()
	AReadyOrNotCharacter* PickUpCharacter = nullptr;
	
	FVector FinalDestinationLocation = FVector::ZeroVector;

	virtual float GetDestinationTolerance() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool CanFinishActivity() const override;
	virtual void ResetData() override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;

	UFUNCTION()
	void EnterMoveToStage();
	UFUNCTION()
	bool CanPickUpNow() const;
	
	UFUNCTION()
	void EnterTransitStage();
	UFUNCTION()
	bool CanTransitNow() const;
	
	UFUNCTION()
	void EnterPickUpStage();
	UFUNCTION()
	bool CanPlaceDownNow() const;
	
	UFUNCTION()
	void EnterPlaceDownStage();
	UFUNCTION()
	bool IsPlaceDownComplete() const;
	
	UFUNCTION()
	void EnterCompleteStage();
	
	UFUNCTION()
	void TickPickUpStage(float DeltaTime, float Uptime);
};
