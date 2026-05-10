// Copyright Void Interactive, 2021

#pragma once

#include "CombatMove/HardCoverCombatMove.h"
#include "Info/Activities/BaseActivity.h"
#include "BaseCombatActivity.generated.h"

DECLARE_STATS_GROUP(TEXT("BaseCombatActivity"), STATGROUP_CombatActivity, STATCAT_Advanced);

UENUM()
enum class ECombatEngagementType : uint8
{
	FireWeapon,
	Melee,
	ExplosiveVest,
	Custom
};

/**
 * Base class for AI combat logic
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UBaseCombatActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UBaseCombatActivity();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackNewEnemy, AReadyOrNotCharacter*, NewTrackedEnemy);
	UPROPERTY(BlueprintAssignable)
	FOnTrackNewEnemy OnTrackNewEnemy;
	
	void StartRunningCombatMove(UClass* Class);
	
	UFUNCTION(BlueprintCallable)
	void StartRunningCombatMove(UBaseCombatMoveActivity* CombatMove);

	UFUNCTION(BlueprintCallable)
	void FinishCombatMove(bool bSuccess = true);

	UFUNCTION(BlueprintCallable)
	bool IsRunningCombatMoveActivity(UClass* Class) const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE UBaseCombatMoveActivity* GetCombatMoveActivity() const { return CombatMoveActivity; }
	
	template<class T = UBaseCombatMoveActivity>
	T* GetCombatMoveActivity() const;
	
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool CanShoot() const override;
	virtual bool CanReload() const override;

	virtual float GetDestinationTolerance() const override;

	virtual void GatherDebugText(FString& OutText) {}

	virtual void GatherDebugString(FString& OutString) override;

	virtual bool GetStrafeDebugString(FString& OutString) const;
	
	virtual bool CanEngageNow() const;

	bool IsReloading() const;
	bool ShouldReloadNow() const;

	virtual bool ShouldTrackTarget() const;
	
	virtual bool ShouldStrafe() const;
	bool IsStrafing() const;

	virtual bool TryMoveIntoCoverLandmark(const FVector& ThreatLocation, const FVector& ThreatDirection, float MinDistanceFromInstigator = 0.0f, AReadyOrNotCharacter* InstigatorCharacter = nullptr);
	virtual bool TryMoveIntoCoverLandmark();

	bool TryMoveIntoCover(const FCoverInstigatorStimulus& InstigatorStimulus, bool bRequireLOS = true);
	
	virtual bool TryMoveIntoCover(AReadyOrNotCharacter* InstigatorCharacter, float MinDistanceFromInstigator = 0.0f, float ExclusionRadiusAroundInstigator = 0.0f, bool bRequireLOS = true);
	bool TryMoveIntoCoverLandmark(const FVector& ThreatLocation, float MinDistanceFromInstigator = 0.0f, AReadyOrNotCharacter* InstigatorCharacter = nullptr);

	class AWallHoleTraversal* FindClosestWallHoleTraversal() const;

	bool TryTraverseNearestHole();
	
	bool TryPlayDead(bool bSilentDeath = false, bool bForce = false);
	bool TryCommitSuicide(bool bFakeOut = false);
	bool TryFindPickupItem();

	void ReloadEquippedWeapon();

	void ResetSuicideConsideration();

	virtual bool RunEngagementLogic(AReadyOrNotCharacter* Enemy, float DeltaTime);

	// if EnemyCharacter is null, use tracked enemy if possible
	bool FireWeapon(AReadyOrNotCharacter* EnemyCharacter = nullptr, bool bEnableInfiniteAmmo = false);

	FORCEINLINE ECombatEngagementType GetCombatEngagementType() const { return ActiveEngagementType; }

	void BindEvents();
	void UnbindEvents();

	UFUNCTION(BlueprintCallable)
	void ScriptedFireAtActor(AActor* InActor, float InTime, bool bOverrideTarget, float AccuracyPenaltyMultiplier, bool bInfiniteAmmo = false);
	UFUNCTION(BlueprintCallable)
	void ScriptedFireAtLocation(FVector InLocation, float InTime, bool bOverrideTarget, float AccuracyPenaltyMultiplier, bool bInfiniteAmmo = false);
	UFUNCTION(BlueprintCallable)
	void StopScriptedFire();
	
	UFUNCTION(BlueprintCallable)
	void ScriptedLookAtActor(AActor* InActor, float InTime);
	UFUNCTION(BlueprintCallable)
	void ScriptedLookAtLocation(FVector InLocation, float InTime);
	UFUNCTION(BlueprintCallable)
	void StopScriptedLook();

	UFUNCTION(BlueprintPure)
	bool IsTryingToFireAtScriptedActor() const;
	bool IsTryingToFireAtScriptedActor(FScriptedFireAt& OutScriptedFireAt) const;

	FVector GetFiringPointOnActor(AActor* Actor);
	
	UPROPERTY(BlueprintReadOnly)
	FScriptedFireAt CurrentScriptedFireAt;
	
	UPROPERTY(BlueprintReadOnly)
	FScriptedLookAt CurrentScriptedLookAt;

	float TimeSpentWithWeaponUp = 0.0f;
	float TimeSpentEngagingOnTarget = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float FleeDesire = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float TimeSincePerformingAnyCombatMove = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	float TimePerformingAnyCombatMove = 0.0f;

	void BeginAction(FAIActionData* InAction);
	void EndAction(FAIActionData* InAction);
	
	void OnSuccessfullyConsideredAction(FAIActionData* InAction);
	void OnFailedToConsiderAction(FAIActionData* InAction);
	
	virtual bool PerformAction(FAIActionData* InAction, float DeltaTime);
	virtual bool CanPerformAction() const;

	#if !UE_BUILD_SHIPPING
	FString DebugActionData(FAIActionData* InAction);
	FString UnableToFireReason = "";
	FString UnableToMeleeReason = "";
	#endif

	float GetConsiderationsCount(FAIActionData* InAction, bool bSuccessfulConsiderations = true);
	
	UPROPERTY()
	class UTraverseHoleActivity* TraverseHoleActivity = nullptr;
	
	UPROPERTY()
	UBaseCombatMoveActivity* CombatMoveActivity = nullptr;
	
	UPROPERTY()
	UBaseCombatMoveActivity* PreviousCombatMoveActivity = nullptr;

	UPROPERTY()
	class UHardCoverCombatMove* HardCoverCombatMove = nullptr;
	
	UPROPERTY()
	class UDuelingCombatMove* DuelingCombatMove = nullptr;
	
	UPROPERTY()
	class UFlankingCombatMove* FlankingCombatMove = nullptr;
	
	UPROPERTY()
	class USuppressionCombatMove* SuppressionCombatMove = nullptr;
	
	UPROPERTY()
	class UPushCombatMove* PushCombatMove = nullptr;
	
	UPROPERTY()
	class UChargeCombatMove* ChargeCombatMove = nullptr;

	UPROPERTY()
	class UFleeingCombatMove* FleeingCombatMove = nullptr;
	
	UPROPERTY()
	class URepositionCombatMove* RepositionCombatMove = nullptr;
	
protected:
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	
	virtual bool EngageEnemy(AReadyOrNotCharacter* EnemyCharacter, float DeltaTime);
	
	// Returns true if a new AI character is being tracked, otherwise false if already tracking same AI
	bool TryTrackEnemy(AReadyOrNotCharacter* NewTrackedEnemy);

	virtual bool RunEngagementLogic(float DeltaTime);
	
	virtual void RunNonEngagementLogic(float DeltaTime);

	virtual bool CanEngageEnemy(AReadyOrNotCharacter* Enemy) const;

	virtual bool TryFlee();

	UFUNCTION(BlueprintPure)
	int32 GetFailureCountForCombatMove(TSubclassOf<UBaseCombatMoveActivity> CombatMoveClass) const;
	
	UFUNCTION()
	virtual void OnCoverFound();
	UFUNCTION()
	virtual void OnNoCoverFound();
	UFUNCTION()
	virtual void OnCoverExit();
	UFUNCTION()
	virtual void OnRequestCover();
	UFUNCTION()
	virtual void OnRequestCoverLandmark();
	UFUNCTION()
	virtual void OnCoverLandmarkExit();
	
	virtual void OnTrackEnemy(AReadyOrNotCharacter* Enemy);

	ECombatEngagementType ActiveEngagementType = ECombatEngagementType::FireWeapon;
	
	float RequiredTimeSpentWithWeaponUp = 0.5f;
	
	float TimeStrafing = 0.0f;
	float TimeNotStrafing = 0.0f;

	float OneFrameAccuracyMultiplier = 5.0f;

	// Accuracy multiplier set when charging the player when using explosive vests
	float ExplosiveVestAccuracyMultiplier = 1.0f;

	bool IsTryingToFireOnFriendly() const;

	int32 GetWeaponAmmoRemaining() const;

	bool ShouldTriggerReloadNow() const;

	bool ShouldFinishCombatMoveNow() const;

	bool DoEquippedWeaponHitScan(AReadyOrNotCharacter* EnemyCharacter);
	
	UFUNCTION(BlueprintPure)
	bool IsFocusingOnActor(const AActor* InActor) const;
	
	UFUNCTION()
	virtual void EnterStrafeState();
	UFUNCTION()
	virtual void EnterNoStrafeState();
	
	UFUNCTION()
	virtual void PerformNoStrafeLogic(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void PerformStrafeLogic(float DeltaTime, float Uptime);
	
	UFUNCTION()
	virtual void TrackEnemyFire(AReadyOrNotCharacter* FromCharacter, ABaseMagazineWeapon* Weapon, FVector FireDirection);
	UFUNCTION()
	virtual void TrackEnemyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION()
	virtual void OnTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION()
	virtual void OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover Settings", meta = (ClampMin = 0.0f, ClampMax = 120.0f))
	float CoverLandmarkEvaluationCooldown = 3.0f;
	
	UPROPERTY()
	AReadyOrNotCharacter* LastTrackedEnemy = nullptr;
	
	FVector LastTrackedEnemyFireDirection = FVector::ZeroVector;
	
	bool IsExplodingVest() const;
	
	virtual bool CanPlayDead() const;
	
	UFUNCTION()
	virtual void PlayDeadStarted(UBaseActivity* Activity, ACyberneticController* Controller);
	UFUNCTION()
	virtual void PlayDeadFinished(UBaseActivity* Activity, ACyberneticController* Controller);

	UFUNCTION()
	virtual void OnSuicideFakeOutSuccess();

	UPROPERTY()
	class UPickupItemActivity* PickupItemActivity = nullptr;
	UPROPERTY()
	class UReloadSafelyActivity* ReloadSafelyActivity = nullptr;
	UPROPERTY()
	class UPlayDeadActivity* PlayDeadActivity = nullptr;
	UPROPERTY()
	class UCommitSuicideActivity* CommitSuicideActivity = nullptr;

	float CurrentCoverEvaluationCooldown = 0.0f;
	float CurrentCoverLandmarkEvaluationCooldown = 0.0f;

	bool bConsideredSuicide = false;
	bool bTryPlayDead = false;

	bool bAmbushAttacking = false;

	bool HasRecentlySeenTarget(AReadyOrNotCharacter* TargetCharacter) const;
	
	float GetFireRate(const ABaseMagazineWeapon* MagazineWeapon) const;
	float GetFireRateDeviationPercentage(const ABaseMagazineWeapon* MagazineWeapon) const;

	
	TMap<FName, int32> ActionSuccessfulConsiderations;
	TMap<FName, int32> ActionFailedConsiderations;

public:
	TMap<FName, float> CombatMoveActionsStartTime;
	TMap<FName, float> CombatMoveActionsEndTime;	

	
	FAIActionData* LastBegunCombatMoveAction;
	FAIActionData* ActiveCombatMoveAction;
	static bool IsActionCombatMove(FAIActionData* InAction);
};

template <class T>
T* UBaseCombatActivity::GetCombatMoveActivity() const
{
	static_assert(TIsDerivedFrom<T, UBaseCombatMoveActivity>::Value, "T must be derived from UBaseCombatMoveActivity");

	return Cast<T>(CombatMoveActivity);
}