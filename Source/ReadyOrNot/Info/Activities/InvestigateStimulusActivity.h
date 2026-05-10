// Copyright Void Interactive, 2022

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "InvestigateStimulusActivity.generated.h"

UCLASS()
class READYORNOT_API UInvestigateStimulusActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UInvestigateStimulusActivity();
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;

	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
	virtual bool CanFinishActivity() const override;

	virtual bool ShouldForceStrafe() const override;

	virtual float GetDestinationTolerance() const override;
	
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual void ResetData() override;

	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus InStimulus, AActor*& OutOverrideSensedActor) override;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FAIStimulus Stimulus;
	
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	AReadyOrNotCharacter* Instigator = nullptr;

	FVector CurrentInvestigationLocation = FVector::ZeroVector;

	bool bEverHadLOSToStimulus = false;
	bool bEverHadLOSToStimulusSource = false;
	bool bInvestigatingSource = false;
	bool bHasEverSpoken = false;
	float StimulusLOSTime = 0.0f;
	float StimulusSourceLOSTime = 0.0f;
	float TimeInvestigatingWhenArrived = 0.0f;
};
