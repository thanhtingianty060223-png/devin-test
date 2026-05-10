// Copyright Void Interactive, 2021

#pragma once

#include "BaseActivity.h"
#include "CoverData.h"
#include "TakeCoverActivity.generated.h"

USTRUCT()
struct FCoverInstigatorStimulus
{
	GENERATED_BODY()
	
	UPROPERTY()
	AReadyOrNotCharacter* InstigatorCharacter = nullptr;

	UPROPERTY()
	FTransform ThreatTransform = FTransform::Identity;

	uint8 bUseThreatTransformAsInstigatorTransform : 1;

	float SearchRadius = 0.0f;
	float ExclusionRadiusFromInstigator = 0.0f;

	FVector GetLocation() const;

	bool IsValid() const;
};

UENUM(BlueprintType)
enum class EAbortCoverReason : uint8
{
	Success,
	Forced,
	EnemySensed,
	SeenEnemyApproaching,
	HeardEnemyApproaching,
	EnemyMovingTowardsUs,
	EnemyBehindUs,
	EnemyFiredNearUs
};

UCLASS()
class READYORNOT_API UTakeCoverActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UTakeCoverActivity();
	
	virtual void PerformActivity(float DeltaTime) override;

	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;

	virtual void GatherDebugString(FString& OutString) override;
	#endif

	void ResetCoverData();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void ResumeActivity() override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool OverrideFireAngleThreshold(float& Threshold) const override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool CanShoot() const override;
	virtual bool CanReload() const override;
	virtual float GetDestinationTolerance() const override;

	virtual bool GetLowReadyOverride(bool& bLowReady) const override;

	virtual bool ShouldDisableMoveRequest() const override;

	bool IsMovingToCover() const;
	bool IsInCover() const;
	bool IsCoverFiring() const;
	bool IsExitingCover() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnteredCover);
	FOnEnteredCover OnEnteredCover;
	
	// How many times does the AI lean and shoot from cover
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover Settings")
	int32 MaxCoverFireCount = 5;

	// How long should we be exposed for when firing from cover?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover Settings")
	float CoverFireTimeCooldown = 2.0f;
	
	// How long should we wait before firing from cover?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover Settings")
	float CoverFireCooldown = 3.0f;

	bool IsPlayingCoverEnterAnims() const;
	bool IsPlayingCoverExitAnims() const;

	FORCEINLINE float GetElapsedTimeInCover() const { return ElapsedTimeInCover; }
	FORCEINLINE int32 GetReloadAtAmmoCount() const { return ReloadAtAmmoCount; }

	UPROPERTY()
	FCoverInstigatorStimulus InstigatorStimulus;

	FCoverData CoverData;

	EAbortCoverReason LastAbortCoverReason = EAbortCoverReason::Success;

	void AbortCoverNow(const EAbortCoverReason& InReason = EAbortCoverReason::Success);

	virtual FName GetMoveStyleOverride_Implementation() const override;

protected:
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;
	
	UFUNCTION()
	void OnEnemyWeaponFire(AReadyOrNotCharacter* Character, ABaseMagazineWeapon* Weapon, FVector FireDirection);
	UFUNCTION()
	void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	UFUNCTION()
	void OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);

	UFUNCTION()
	void EnterMoveToCoverState();
	UFUNCTION()
	void TickMoveToCoverState(float DeltaTime, float Uptime);
	
	UFUNCTION()
	void EnterCoverState();
	UFUNCTION()
	void TickCoverState(float DeltaTime, float Uptime);
	UFUNCTION()
	void ExitCoverState();
	
	UFUNCTION()
	void EnterCoverFireState();
	UFUNCTION()
	void TickCoverFireState(float DeltaTime, float Uptime);
	UFUNCTION()
	void ExitCoverFireState();

	UFUNCTION()
	void EnterCompleteState();
	
	UFUNCTION()
	void EnterCompleteAbruptState();
	
	UFUNCTION()
	bool CanCover() const;
	UFUNCTION()
	bool CanStopCoverFire() const;
	UFUNCTION()
	bool CanFireFromCover() const;
	UFUNCTION()
	bool CanCompleteCover() const;
	
	UFUNCTION()
	bool CanAbruptCompleteCover() const;

	void CompleteCover();

	bool IsSideSwitching() const;
	bool IsWaiting() const;

	void OnInitialEnterAnimFinished();
	
	void UpdateCoverFireType();
	void UpdateCoverDirection();
	
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;

	bool SideSwitch(FVector LastSensedLocation, bool bPlayAnim = false);
	void ToggleCoverDirection(bool bPlayAnim = false);

	void SwitchCoverDirection(const ECoverDirection& NewCoverDirection, bool bPlayAnim = false);

	bool AnySWATHasLOSToCoverPoint() const;
	bool AnySWATHasLOSToUs() const;

	bool HasReachedCoverLocation(float Tolerance = 75.0f) const;
	bool HasReachedEntryLocation(float Tolerance = 75.0f) const;

	bool IsActiveCoverDirectionBlocked() const;
	bool IsCoverDirectionBlocked(const ECoverDirection& InCoverDirection) const;

	void StopEntryAnims();
	void StopExitAnims();

	void RemoveLocationFromHistory();

	void ResetCurrentCoverTypeToOriginal();
	
	void ForceExposeFire();

	bool IsInstigatorMovingTowardsCover() const;

	bool ShouldBlindFire() const;

	FVector InitializeCoverAndCalculateEntry();

	ECoverDirection DetermineDesiredCoverDirection();
	
	UPROPERTY()
	ADoor* Door = nullptr;
	
	UPROPERTY()
	UAnimMontage* LastExitMontagePlayed = nullptr;
	
	UPROPERTY()
	UAnimMontage* EntryMontage = nullptr;
	
	ECrouchCoverType OriginalCrouchCoverType = ECrouchCoverType::Wall;
	ECrouchCoverType CurrentCrouchCoverType = ECrouchCoverType::Wall;
	EStandCoverType OriginalStandCoverType = EStandCoverType::Wall;
	EStandCoverType CurrentStandCoverType = EStandCoverType::Wall;
	
	ECoverFireType CurrentCoverFireType = ECoverFireType::None;
	ECoverFireType PreviousCoverFireType = ECoverFireType::None;
	ECoverDirection CurrentCoverDirection = ECoverDirection::Left;

	float ElapsedTimeInCover = 0.0f;
	float TimeSinceHeardNoiseInCover = 0.0f;
	float TimeSinceLastShotAt = 999.0f;
	float CoverFireTime = 0.0f;
	float EntryDistance = 0.0f;
	float PathDistanceToCover = 0.0f;
	float EntryDotProductDecreaseRate = 0.0f;

	TArray<FVector> InstigatorLocationHistory;
	
	FVector EntryLocation = FVector::ZeroVector;
	FVector CoverFireFocalPoint = FVector::ZeroVector;
	FVector CoverFireFocalPoint_Entry = FVector::ZeroVector;
	FVector LastSensedEnemyPosition = FVector::ZeroVector;

	FString EntryAnimation = "";

	FTimerHandle ChangeCoverFireTypeTimer;
	FTimerHandle ChangeCoverDirectionTypeTimer;
	FTimerHandle ForgetLocationTimer;

	uint16 EnterAnimIndex = 0;
	uint16 EnterAimAnimIndex = 0;
	uint16 EnterCoverCount = 0;
	uint16 ReloadAtAmmoCount = 0;
	
	int16 EntryAngle = 0;
	
	uint8 bShouldCrouch : 1;
	uint8 bHasEverEnteredCover : 1;
	uint8 bHasCoverFireLOS : 1;
	uint8 bShouldExitCoverFireNow : 1;
	uint8 bShouldTryCoverFireNow : 1;
	uint8 bShouldCompleteCoverNow : 1;
	uint8 bEverHadLOSToEnemyInPreviousCoverFire : 1;
	uint8 bChosenCoverAimType : 1;
	uint8 bChosenEntryPoint : 1;
	uint8 bHasEverSideSwitched : 1;
	uint8 bInitialEntryComplete : 1;
	uint8 bPerpetualCoverFire : 1;
};
