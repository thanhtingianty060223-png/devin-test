// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "TakeCoverAtLandmarkActivity.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UTakeCoverAtLandmarkActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UTakeCoverAtLandmarkActivity();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void ResumeActivity() override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual void ResetData() override;
	virtual bool CanFinishActivity() const override;
	virtual bool CanShoot() const override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool CanOverrideActivity() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual float GetDestinationTolerance() const override;

	virtual FName GetMoveStyleOverride_Implementation() const override;
	
	// Called from anim notify
	void Notify_OnProxyUse();

	void AbortCoverNow();

	bool IsMovingToLandmark() const;
	bool IsExitingLandmark() const;

	// If null, find nearest cover landmark
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	class ACoverLandmark* CoverLandmark = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	class ACoverLandmarkProxy* ChosenEntryProxy = nullptr;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	class ACoverLandmarkProxy* ChosenExitProxy = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	float SearchRadius = 1000.0f;

	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	//float InitialWaitDuration = 30.0f;
	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	//float SWATSeenLandmarkWaitDuration = 5.0f;
	
	float WaitDuration = 30.0f;

protected:
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;

	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;

	void Broadcast_OnProxyEnd(bool bSuccess);

	void TeleportWeapon();

	// Exit events: exit cover landmark immediately if these are broadcasted
	UFUNCTION()
	void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	UFUNCTION()
	void OnHeardYell(AReadyOrNotCharacter* Shouter, bool bLOS);
	UFUNCTION()
	void OnEnemyWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection);
	UFUNCTION()
	void OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);

	UFUNCTION()
	void EnterMoveToLandmarkState();
	UFUNCTION()
	void TickMoveToLandmarkState(float DeltaTime, float Uptime);
	
	UFUNCTION()
	void Enter_EnterLandmark_State();
	UFUNCTION()
	void Tick_EnterLandmark_State(float DeltaTime, float Uptime);
	UFUNCTION()
	void Enter_ExitLandmark_State();
	UFUNCTION()
	void Tick_ExitLandmark_State(float DeltaTime, float Uptime);
	
	UFUNCTION()
	void Enter_Wait_State();
	UFUNCTION()
	void Tick_Wait_State(float DeltaTime, float Uptime);

	UFUNCTION()
	void Enter_AbruptExit_State();
	UFUNCTION()
	void Tick_AbruptExit_State(float DeltaTime, float Uptime);
	
	UFUNCTION()
	bool CanEnterLandmark() const;
	UFUNCTION()
	bool CanExitLandmark() const;
	UFUNCTION()
	bool ShouldWait() const;
	UFUNCTION()
	bool CanAbruptlyExit() const;

	bool HasReachedEntryLocation(float Tolerance) const;
	
	void PlayExitLandmarkAnim();

	void UpdateHidingState();

	void BindExitEvents();
	void UnbindExitEvents();

	void AddIgnoredActors();
	void RemoveIgnoredActors();

	void EnableProxy();
	void DisableProxy();

	//float TimeSwatNotLookingAtLandmark = 0.0f;
	
	float TimeEnteringLandmark = 0.0f;
	float EntryAnimTime = 0.0f;
	float ExitAnimTime = 0.0f;
	
	float TimeWaiting = 0.0f;
	float TimeSinceLastVisionTrace = 0.0f;

	uint8 FiredAtCount = 0;

	uint8 bIsHiding : 1;
	
	uint8 bShouldExitNow : 1;
	uint8 bPlayedExitAnim : 1;
	uint8 bShouldSurrender : 1;

	uint8 bMoveToExit : 1;
	
	UPROPERTY()
	UAnimMontage* EntryAnim = nullptr;
	UPROPERTY()
	UAnimMontage* ExitAnim = nullptr;
	UPROPERTY()
	UAnimMontage* LoopEntryAnim = nullptr;
	UPROPERTY()
	UAnimMontage* LoopExitAnim = nullptr;

	UPROPERTY()
	TArray<AStaticMeshActor*> IgnoredMeshActors;
	
private:
	class ACoverLandmark* FindClosestCoverLandmark() const;
	bool IsAnySWATLookingAtLandmark() const;
	bool IsAnySWATLookingAtUs() const;
};
