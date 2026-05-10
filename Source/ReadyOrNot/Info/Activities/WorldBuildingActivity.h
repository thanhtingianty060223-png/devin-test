// Copyright Void Interactive, 2021

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "WorldBuildingActivity.generated.h"

/**
 * Base class for all world building activities that AI can perform
 * A world building activity is a start, loop and end anim
 */
UCLASS()
class READYORNOT_API UWorldBuildingActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UWorldBuildingActivity();
	
	UFUNCTION(BlueprintCallable, Category = "World Building Activity")
	void SetRotation(FRotator NewRotator) { Rotation = NewRotator; }

	UFUNCTION(BlueprintPure, Category = "World Building Activity")
	FORCEINLINE FRotator GetRotationOffset() const { return Rotation + FinalRotationOffset; }

	UFUNCTION(BlueprintPure, Category = "World Building Activity")
	bool IsSetupCorrectly() const;
	
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bShouldHolsterWeapon = true;
	
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bShouldSurrenderFromActivity = true;
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bRequireRotationMatch = true;

	float WorldBuildingTime = 5.0f;
	
	bool bAbortDueToPendingSurrender = false;
	bool bAbortNow = false;

	bool bMoveOnly = false;

	virtual void ResetData() override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	
	virtual bool CanFinishActivity() const override;
	virtual bool CanShoot() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual bool CanBePushed() const override;

	virtual bool CanBeCleared() override;
	virtual bool CanOverrideActivity() const override;
	
	virtual float GetDestinationTolerance() const override;
	
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;

	bool TryDrawWeapon();
	bool TryHolsterWeapon();

	UFUNCTION()
	bool ShouldEnd() const;
	UFUNCTION()
	virtual bool ShouldEndAbruptly() const;

	UFUNCTION()
	void EnterMoveToState();
	UFUNCTION()
	void TickMoveToState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool ShouldStart() const;
	
	UFUNCTION()
	void EnterStartState();
	UFUNCTION()
	void EnterEndState();
	UFUNCTION()
	void EnterAbruptEndState();
	UFUNCTION()
	void EnterCompleteState();
	UFUNCTION()
	void EnterLoopState();
	UFUNCTION()
	void TickLoopState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool ShouldLoop() const;
	UFUNCTION()
	bool ShouldComplete() const;

	void LoopNow();
	void CompleteNow();
	
	bool bLocationMatch = false;
	bool bRotationMatch = false;
	
	FRotator Rotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, Category = "Setup")
	FRotator FinalRotationOffset = FRotator::ZeroRotator;

public:
	UPROPERTY(EditAnywhere, Category = "Animation")
	bool bOneShotAnimationDataTable = false;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (EditCondition = "bOneShotAnimationDataTable", EditConditionHides))
	FString TableMontageName = "";

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (EditCondition = "!bOneShotAnimationDataTable", EditConditionHides))
	UAnimSequence* Loop = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (EditCondition = "!bOneShotAnimationDataTable", EditConditionHides))
	UAnimMontage* MontageStart = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (EditCondition = "!bOneShotAnimationDataTable", EditConditionHides))
	UAnimMontage* MontageEnd = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (EditCondition = "!bOneShotAnimationDataTable", EditConditionHides), DisplayName = "Montage Abrupt End (Optional)")
	UAnimMontage* MontageAbruptEnd = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Speech")
	FString StartActivitySpeech = "";

	UPROPERTY(EditAnywhere, Category = "Speech")
	FString FinishActivitySpeech = "";

	UPROPERTY()
	UAnimMontage* TableMontageAnim = nullptr;
	
	float TimeAtWorldBuildingLocation = 0.0f;
	float TotalTimeUntilEnd = 0.0f;

	float TimeDoingWorldBuilding = 0.0f;

	bool bHasHolsteredWeapon = false;
	bool bShouldCompleteNow = false;
	bool bShouldLoopNow = false;
	bool bHasPlayedStartSpeech = false;
};
