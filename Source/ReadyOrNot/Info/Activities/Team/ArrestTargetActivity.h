// Void Interactive, 2020

#pragma once

#include "Info/Activities/Team/TeamBaseActivity.h"
#include "ArrestTargetActivity.generated.h"

UCLASS()
class READYORNOT_API UArrestTargetActivity final : public UBaseActivity
{
	GENERATED_BODY()
	
public:
	UArrestTargetActivity();
	
	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool CanOverrideActivity() const override;

	virtual void ResetData() override;

	UPROPERTY()
	ACyberneticCharacter* ArrestTarget = nullptr;
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void ResumeActivity() override;
	virtual bool CanBeCleared() override;
	virtual bool CanShoot() const override;

	virtual void RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath) override;

	virtual float GetDestinationTolerance() const override;

	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif

	UFUNCTION()
	void OnArresterKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	
	UFUNCTION()
	void EnterMoveToStage();
	UFUNCTION()
	void EnterArrestStage();
	UFUNCTION()
	void TickArrestStage(float DeltaTime, float Uptime);
	
	UFUNCTION()
	bool CanArrest() const;

	FVector TryFindReachableLocationToArrestFrom() const;
	UInteractionsData* ChooseRandomArrestInteraction() const;
	
	UPROPERTY()
	UInteractionsData* ArrestInteraction = nullptr;

	FVector BestArrestLocation = FVector::ZeroVector;

	bool bCalledOutMove = false;

private:
	bool CheckEdgeCases();
	
	void BindEvents();
	void UnbindEvents();
};
