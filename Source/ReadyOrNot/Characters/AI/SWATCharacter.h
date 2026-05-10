// Copyright Void Interactive, 2022

#pragma once

#include "Characters/CyberneticCharacter.h"
#include "SWATCharacter.generated.h"

DECLARE_STATS_GROUP(TEXT("Swat Character"), STATGROUP_SwatCharacter, STATCAT_Advanced);

UCLASS()
class READYORNOT_API ASWATCharacter : public ACyberneticCharacter
{
	GENERATED_BODY()
	
protected:
	explicit ASWATCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual bool IsAffectedByDamageType(UDamageType* DamageType) const override;

	virtual void Multicast_OnKilled_Implementation(FName LastBone, AActor* DamageCauser) override;

	virtual void StartStun(EStunType StunType, AActor* StunCauser) override;
	
	virtual void UpdateDefaultMoveStyle() override;

	virtual void Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal) override;
	virtual void Multicast_InflictSuppression_NoLineOfSight_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal) override;

	void ReturnSuppressiveFire(const FSuppressionData& SuppressionData, bool bIsUsingLessLethal = false);

	virtual bool IsSecured_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
	virtual void Secure_Implementation(AReadyOrNotCharacter* InInstigator) override;
	
    virtual void Surrender() override;
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Knockout(float Duration, bool bPlayVO = true) override;
	virtual bool IsSurrendered() const override { return false; }

	virtual FString MutateVoiceline(const FString& VO) override;

	virtual bool CanPushDoor(ADoor* Door) const override;
	
	virtual void Server_ReportTarget_Implementation(AActor* Character) override;

	float TimeUntilNextFlashlightCheck = 0.0f;

	float OnShotResponseDelay = 0.75f;

	FTimerDelegate OnShotResponse_Delegate;
	FTimerHandle OnShotResponse_Handle;

	FTraceHandle ViewBlockTraceHandle;
	FTraceHandle LeanTraceHandle;

	UFUNCTION()
	void PlayOnShotDialogue(bool bIsFriendly);	
	
	virtual bool OnTakeDamage(float& Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void OnGameEnded_Implementation() override;

	void PlayGestureAnimation();
	void StopGestureAnimation();

	float CurrentGestureInterval = 0.0f;
	
	float TimeSinceFriendlyShotAtMe = FLT_MAX;
	float TimeStuck = 0.0f;

protected:
	UPROPERTY(BlueprintReadOnly)
	class URosterCharacter* RosterCharacter;
	
public:
	void SetRosterCharacter(URosterCharacter* InRosterCharacter);
	FORCEINLINE URosterCharacter* GetRosterCharacter() const { return RosterCharacter; }

	FText GetSwatCharacterName() const;
	
	bool bViewBlockedByOtherSwat;
};
