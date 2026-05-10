// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "PlayDeadActivity.generated.h"

UCLASS()
class READYORNOT_API UPlayDeadActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UPlayDeadActivity();

	uint8 bSilentDeath : 1;

	float InitialPlayDeadDuration = 10.0f;
	float SeenSWATPlayDeadDuration = 2.5f;
	
	float PlayDeadDuration = 10.0f;
	
	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool CanOverrideActivity() const override;
	virtual bool CanFinishActivity() const override;

	bool IsAnySWATLookingAtUs() const;
	bool IsAnyPlayerAimingAtUs() const;

	void StopPlayingDeadNow(bool bSurrender = false);

	// Exit events: stop playing dead immediately if these are broadcasted
	UFUNCTION()
	void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	UFUNCTION()
	void OnHeardYell(AReadyOrNotCharacter* Shouter, bool bLOS);
	UFUNCTION()
	void OnDamaged(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);

	float WaitDelay = 0.0f;
	
	float SWATAimTime = 0.0f;
	float SWATNoLookTime = 0.0f;

private:
	uint8 bBindedEvents : 1;
};
