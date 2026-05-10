// Void Interactive, 2020

#pragma once

#include "TeamBaseActivity.h"
#include "DisarmStandaloneTrapActivity.generated.h"

UCLASS()
class READYORNOT_API UDisarmStandaloneTrapActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UDisarmStandaloneTrapActivity();
	
	UPROPERTY()
	class ATrapActor* TrapToDisarm = nullptr;

	virtual bool CanOverrideActivity() const override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void ResumeActivity() override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool CanFinishActivity() const override;
	
	bool IsTrapLive() const;

	UFUNCTION()
	void EnterGetInPositionStage();
	UFUNCTION()
	bool CanPerformDisarm() const;

	UFUNCTION()
	void EnterDisarmStage();

	UFUNCTION()
	void OnTrapDisarmed(); // called from anim notify

	UFUNCTION()
	void OnTrapTriggered(ATrapActor* Trap, AReadyOrNotCharacter* TriggeredBy);

	FVector DisarmLocation = FVector::ZeroVector;
	
private:
	void BindEvents();
	void UnbindEvents();
	void PlayDisarmTrapAnim(const FVector& InFocalPoint = FVector::ZeroVector);
	void StopDisarmTrapAnim(float FadeOutTime = 0.25f);
};
