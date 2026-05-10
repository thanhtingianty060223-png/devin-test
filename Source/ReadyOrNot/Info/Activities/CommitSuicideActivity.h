// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "CommitSuicideActivity.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCommitSuicideActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UCommitSuicideActivity();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSuicideActivityDelegate);
	FSuicideActivityDelegate OnFakeOutSuccess;
	
	uint8 bFakeOut : 1;

	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool CanOverrideActivity() const override;
	virtual bool CanFinishActivity() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	// Exit events: stop suiciding immediately if these are broadcasted
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	
	UFUNCTION()
	void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	UFUNCTION()
	void OnHeardYell(AReadyOrNotCharacter* Shouter, bool bLOS);
	UFUNCTION()
	void OnDamaged(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION()
	void OnMeleeHitTaken(AReadyOrNotCharacter* InstigatorCharacter);

private:
	FString ChosenAnimation = "";

	float SuicidePreventionChance = 0.01f;
};
