// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "CollectEvidenceActivity.generated.h"

UCLASS()
class READYORNOT_API UCollectEvidenceActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UCollectEvidenceActivity();
	
	UPROPERTY(BlueprintReadOnly)
	AActor* EvidenceItem = nullptr;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void ResumeActivity() override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool CanFinishActivity() const override;
	virtual void ResetData() override;

	virtual void RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath) override;
	
	UFUNCTION()
	void EnterMoveToStage();
	UFUNCTION()
	void EnterCollectStage();
	UFUNCTION()
	void TickCollectStage(float DeltaTime, float Uptime);
	
	UFUNCTION()
	bool CanCollectEvidence() const;

	UFUNCTION()
	void OnCollectEvidenceBegin();
	UFUNCTION()
	void OnCollectEvidenceEnd();
	
	void OnCollectEvidenceAnimFinished();
	
	UFUNCTION()
	void OnEvidenceCollected();

private:
	bool CheckEdgeCases();
	
	void BindEvents();
	void UnbindEvents();
	
	void StopCollectEvidenceAnim(float BlendOutTime = 0.25f);

	FTimerHandle TH_CollectEvidenceAnim;

	bool bCalledOutMove = false;
};
