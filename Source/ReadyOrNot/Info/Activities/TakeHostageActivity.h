// Copyright Void Interactive, 2021

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "TakeHostageActivity.generated.h"

UCLASS()
class READYORNOT_API UTakeHostageActivity : public UBaseActivity
{
	GENERATED_BODY()
	
public:
	UTakeHostageActivity();
	
	virtual void PerformActivity(float DeltaTime) override;
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool CanFinishActivity() const override;
	virtual bool CanShoot() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual float GetDestinationTolerance() const override;
	virtual bool ShouldForceStrafe() const override;

	virtual bool CanOverrideActivity() const override;
	virtual bool CanBeOverridenBy(UBaseActivity* InOverridingActivity) override;

	virtual void ResetData() override;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	class ACyberneticCharacter* Hostage = nullptr;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	float TimeToSurrenderHostage = 30.0f;
	
protected:
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	
	bool HasReachedEntryLocation(float Tolerance) const;

	void OverrideMoveStyle();
	void ClearMoveStyleOverride();

	UFUNCTION()
	void OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	
	UFUNCTION()
	void OnHostageKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

	UFUNCTION()
	void OnHostageTakeStartComplete_Driver(AActor* Actor);
	UFUNCTION()
	void OnHostageTakeStartComplete_Slave(AActor* Actor);
	
	UFUNCTION()
	void OnHostageTakeEndComplete_Driver(AActor* Actor);
	UFUNCTION()
	void OnHostageTakeEndComplete_Slave(AActor* Actor);

	UFUNCTION()
	void OnHostageTakeTurnComplete_Driver(AActor* Actor);
	UFUNCTION()
	void OnHostageTakeTurnComplete_Slave(AActor* Actor);
	
	UFUNCTION()
	void OnHostageTakeKillComplete_Driver(AActor* Actor);
	UFUNCTION()
	void OnHostageTakeKillComplete_Slave(AActor* Actor);
	
	UFUNCTION()
	void EnterMoveToState();
	UFUNCTION()
	void TickMoveToState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanStartHostageTake() const;

	UFUNCTION()
	void EnterBeginHostageTakeState();
	UFUNCTION()
	void TickBeginHostageTakeState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanIdle() const;
	
	UFUNCTION()
	void EnterTakingState();
	UFUNCTION()
	void EndTakingState();
	UFUNCTION()
	void TickTakingState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanEndHostageTake() const;

	UFUNCTION()
	void EnterTurnState();
	UFUNCTION()
	void TickTurnState(float DeltaTime, float Uptime);
	UFUNCTION()
	bool ShouldTurn() const;
	
	UFUNCTION()
	void EnterEndHostageTakeState();
	UFUNCTION()
	void TickEndHostageTakeState(float DeltaTime, float Uptime);

	UFUNCTION()
	void OnSensedCharacter(AReadyOrNotCharacter* SensedCharacter);

	UFUNCTION()
	void OnHeardYell(AReadyOrNotCharacter* Shouter, bool bLOS);

	UFUNCTION()
	void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);

	void ForceStop();
	void ForceFinish();

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* LastEnemySensed = nullptr;

	UPROPERTY()
	TMap<FName, UInteractionsData*> HostageInteractions;

	FName DriverMoveStyleOverride = NAME_None;
	FName SlaveMoveStyleOverride = NAME_None;
	
	uint8 bKillHostageNow : 1;
	uint8 bSurrenderHostageNow : 1;
	uint8 bIsLooping : 1;
	uint8 bShouldTurnNow : 1;
	uint8 bRightTurn : 1;
	uint8 bIsTurning : 1;
	uint8 bEverHadLOSOnTrackedTargetUponEntering : 1;
	
	float TimeEnteringHostageTake = 0.0f;
	float EntryAnimTime = 0.0f;
	float ExitAnimTime = 0.0f;

	float VOCooldown = 0.0f;

	FVector EntryPoint = FVector::ZeroVector;
};
