// Copyright Void Interactive, 2022

#pragma once

#include "ReadyOrNotCharacter.h"
#include "CoverData.h"
#include "Actors/SafeZoneVolume.h"
#include "Actors/Gameplay/AISpawn.h"
#include "Info/AIFactionManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "CyberneticCharacter.generated.h"

UENUM(BlueprintType)
enum class EPseudoSpeedType : uint8
{
	Null,
	Slow,
	Walk,
	Run,
	Sprint,
	LimpWalk,
	LimpRun,
	Last
};

class AAISpawn;

UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API ACyberneticCharacter : public AReadyOrNotCharacter, public ISecurable, public ICanIssueCommandOn
{
	GENERATED_BODY()

public:
	explicit ACyberneticCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual bool OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	virtual UAnimMontage* PlayMontageFromTable(const FString& Animation) override;
		
	virtual void StartBeingTasered(float PingStunDuration, class ATaser* WeaponUsed) override;

	virtual void FinishAISpawning(AAISpawn* Spawner, const FAIDataLookupTable* AIData);

	virtual void OnMelee_Implementation(AReadyOrNotCharacter* Attacker, FHitResult Hit) override;

	virtual void PlayWeaponFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, bool bOnlyTP) override;
	
	virtual void Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal) override;
	
	virtual void OnActorSpawned(AActor* Actor) override;
	
	virtual UFMODEvent* GetAppropriateVoiceLineEvent() override;
	
	virtual FVector GetNavAgentLocation() const override;

	virtual bool CanArrest() const override;
	virtual bool CanArrestRagdoll() const override;
	
	virtual void SetIsStrafing(bool bNewStrafing, bool bPlayBlendAnimation = true);
	virtual void GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const override;

	virtual void DoLowReadyTrace() override;

	virtual bool ToggleDoor(ADoor* Door, bool bOpen, bool bBypassLock = false);
	virtual void Server_KickQueuedDoor_Implementation() override;
	virtual void Server_KickBreakQueuedDoor_Implementation() override;
	
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void StartStun(EStunType StunType, AActor* StunCauser) override;
	
	virtual void StartPepperSprayed(APepperspray* PeppersprayUsed) override;
	
	virtual void Server_DoMelee_Implementation() override;
	
	virtual void ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true) override;
	
	virtual void Multicast_OnKilled_Implementation(FName LastBoneHit, AActor* DamageCauser) override;

	virtual void ArrestComplete(AReadyOrNotCharacter* PlayerMakingArrest, class AZipcuffs* Zipcuffs) override;
	
	virtual void Fire();
	virtual bool Fire(ABaseMagazineWeapon* Weapon);
	
	virtual bool IsOpeningDoor(ADoor* Door) const override;
	virtual bool IsClosingDoor(ADoor* Door) const override;

	virtual bool CanPushDoor(ADoor* Door) const override;
	
	virtual FRotator GetFireAtRotation(FVector BulletSpawnLocation, FRotator BulletSpawnRotation);
	virtual FRotator GetAimOffsets() const;

	virtual bool GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface) override;

	UFUNCTION(BlueprintCallable)
	virtual UAnimMontage* PlayMontageFromTableWithFocalPoint(const FString& Animation, const FVector& FocalPoint);
	UFUNCTION(BlueprintCallable)
	virtual UAnimMontage* PlayMontageFromTableWithIndexWithFocalPoint(const FString& Animation, int32 Index, const FVector& FocalPoint);
	
	UFUNCTION(BlueprintCallable)
	virtual bool PlayMontageWithFocalPoint(UAnimMontage* Montage, const FVector& FocalPoint);
	
	virtual void StopKnockout();

	virtual void OnRagdollDurationComplete() override;
	virtual bool GetBackupAfterRagdoll() override;

	virtual bool IsGettingUp() const override;

	bool IsPlayingHitReaction() const;
	bool IsPlayingFullBodyHitReaction() const;
	
	virtual void DoMelee(bool bLocal) override;
	virtual void OnMeleeTrace(FHitResult HitResult, bool bLocal) override;
	virtual bool CanMelee() const override;
	virtual bool CanPlayVO(const FString& VoiceLine) const override;
	
	virtual void OnRep_MeshReplicated() override;
	
	virtual void ReactToCarryThrow() override;
	virtual void ResetThrownByCharacter() override;
	
	virtual void OnRep_Surrendered() override;
	
	virtual bool IsActive() const override;
	virtual bool IsActiveForMovement() const override;
	
	bool IsActiveForThinking() const;
	
	virtual bool IsAnimationBlocking() const override;

	virtual bool IsAffectedByDamageType(UDamageType* DamageType) const override;
	
	virtual void GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData) override;
	virtual void GatherDebugText_Implementation(FString& OutText) override;
	virtual void DrawVisualDebug_Implementation() override;
	
	virtual void PickupEvidence(AActor* InEvidence) override;

	virtual void EndStun(EStunType StunType) override;
	
	bool IsPickingUpWeapon() const;
	
	void ReactToHeadBeanbagHit(float Damage, FPointDamageEvent* HitEvent);
	void GetUp(bool bFlip = false);
	
	void EquipLoadoutOnAI(bool bForce = false);
	void EquipArmourOnAI();
	
	void ResetRagdollStates();
	void ResetPlayDeadState();

	void AddMoveDataOverrides(FAIMoveDataBlock& OutMoveData);
	
    void Event_SurrenderCompleted();

	bool IsDrawingWeapon() const;
	
	bool IsRecoiling() const;

	UFUNCTION(BlueprintCallable)
	bool HasLineOfSightToCharacter(AReadyOrNotCharacter* InCharacter) const;

	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const override;

	void DestroyPlayerMarkerComponent();

	FTraceHandle FocalPointLOSTraceHandle;
	
	UPROPERTY()
	APawn* ClosestPawn = nullptr;
	float DistanceToClosestPawn = 0.0f;

	UFUNCTION(BlueprintPure)
	bool CanEverSuicide() const;

	UFUNCTION(BlueprintPure)
	bool CanExitSurrender() const;
	
	UFUNCTION(BlueprintCallable)
	ESurrenderExitType DetermineSurrenderExitType() const;
	
	void OnSurrenderFinished(const FString& CustomMontage = {});

	UFUNCTION(BlueprintCallable)
	bool SurrenderExit(ESurrenderExitType ExitType, FVector FocalPoint = FVector::ZeroVector);

	UFUNCTION(BlueprintPure)
	bool IsExitingSurrender() const;
	
	UFUNCTION(BlueprintPure)
	bool IsHiding() const;

	bool IsProtectedAgainstInstantKnockout() const;
	bool IsBreaching() const;
	
	void UpdateAimOffset();
	
	bool IsStrafing() const;
	
	void AddScoreGroups();

	void SetRecentMeleeVictim(AReadyOrNotCharacter* InRecentMeleeVictim) { RecentMeleeVictim = InRecentMeleeVictim; }
	void ClearRecentMeleeVictim() { RecentMeleeVictim = nullptr; }

	void MeleeVictim(AReadyOrNotCharacter* Victim);
	
	int32 GetMeleeCountFor(const AReadyOrNotCharacter* Character);
	
	UFUNCTION(BlueprintPure)
	TArray<FDebugData> GetDebugInfoOnROE() const;
	
	UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void Surrender();

	bool bNegotiatorTried = false;
	
	// Called when a player or AI controlled officer has yelled and we are in range
	UFUNCTION(BlueprintNativeEvent, Category = Behaviour)
			void OnOfficerShouted(AReadyOrNotCharacter* Shouter, bool bLOS);
    virtual void OnOfficerShouted_Implementation(AReadyOrNotCharacter* Shouter, bool bLOS);
	
	void BarkNonCompliant();

	UFUNCTION(BlueprintPure, Category = "Armor")
	bool IsWearingHeadArmor() const;
	
	UFUNCTION(BlueprintPure, Category = "Armor")
	bool IsWearingExplosiveVest() const;
	UFUNCTION(BlueprintCallable, Category = "AI")
    void FakeSurrender();
	
	UFUNCTION(BlueprintCallable, Category = "AI")
    void DrawWeapon();

	UFUNCTION(BlueprintCallable, Category = AI)
    void ForceFireGun(float Chance = 1.0f);
	
	UFUNCTION(BlueprintPure, Category = Focus)
	FVector GetFocalPoint() const;
	
	UFUNCTION(BlueprintPure, Category = "Damage")
	bool IsDamagedByLethal() const;
	
	UFUNCTION(BlueprintPure, Category = "Damage")
	bool IsDamagedByLessLethal() const;

	bool IsTraversingHole() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsPlayingDead() const { return bIsPlayingDead; }
	
	UFUNCTION(BlueprintPure)
	bool IsRaisingWeapon() const;
	
	UFUNCTION(BlueprintPure)
	bool IsLoweringWeapon() const;

	bool ShouldReload() const;

	bool CanLowReady() const;
	bool ShouldLowReadyNow();

	UFUNCTION(BlueprintPure)
	FORCEINLINE UAIArchetypeData* GetAIArchetype() const { return Archetype; }

	UFUNCTION(BlueprintPure, Category = "Cybernetics | General")
	FORCEINLINE class ACyberneticController* GetCyberneticsController() const { return (ACyberneticController*)GetController(); }

	template <class T>
	FORCEINLINE T* GetCyberneticsController() const { return Cast<T>(GetController()); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool WasRecentlyYelledAt(const float Seconds) const { return TimeSinceHeardOfficerYell <= Seconds; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsHesitatingFor(const float Seconds) const { return HesitationTime >= Seconds; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetHesitationTime() const { return HesitationTime; }
	
	UFUNCTION(BlueprintPure)
	bool IsHesitating() const;
	
	UFUNCTION(BlueprintPure, Category = "Cybernetics")
	ABaseArmour* GetArmour() const;

	UFUNCTION(BlueprintPure)
	virtual bool IsArrestCapable(APlayerCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure)
	bool IsActiveForCombat() const;
	
	UFUNCTION(BlueprintPure)
	bool IsTakingCover() const;
	UFUNCTION(BlueprintPure)
	bool IsTakingCoverAtLandmark() const;
	
	UFUNCTION(BlueprintPure)
	bool IsMovingToCover() const;
	UFUNCTION(BlueprintPure)
	bool IsMovingToLandmarkCover() const;
	
	UFUNCTION(BlueprintPure)
	bool IsFiringFromCover() const;
	
	UFUNCTION(BlueprintPure)
	bool IsTakingHostage() const;
	
	UFUNCTION(BlueprintPure)
	bool IsBeingTakenHostage() const;
	
	UFUNCTION(BlueprintPure)
	bool IsBeginningHostageTake() const;
	
	UFUNCTION(BlueprintPure)
	bool IsEndingHostageTake() const;
	
	UFUNCTION(BlueprintCallable, Category = "AI | Play Dead")
	virtual void StopPlayingDead();

	UFUNCTION(BlueprintCallable, Category = "AI | Knockout")
	virtual void Knockout(float Duration, bool bPlayVO = true);
	UFUNCTION(BlueprintCallable, Category = "AI | Play Dead")
	virtual void PlayDead(float Duration, const bool bPlayVO);

	UFUNCTION()
	virtual void OnGetupAfterRagdollComplete();

	FORCEINLINE AReadyOrNotCharacter* GetRecentMeleeVictim() const { return RecentMeleeVictim; }

	UFUNCTION(BlueprintCallable)
	void PlayBarkOrStartConversation(FString SpeechRow, bool bHasSharedCooldown = false, float Cooldown = 3.0f);
	
	UFUNCTION()
	void PlayShootingWeaponConversation();
	
	void PlayStunnedVoiceLine(EStunType StunType, bool bIsImmune = false);
	
	UFUNCTION(BlueprintPure, Category = "Stun")
	bool IsPlayingStunAnimation() const;
	
	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsUnjustifiedUseOfForce(AReadyOrNotCharacter* Aggressor, ABaseItem* ForceWeapon, UDamageType* ForceUsed = nullptr) const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasDamagedSWAT() const { return bHasDamagedSWATTeam; }

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayArmourHitEffects(ABaseArmour* Armour, FHitResult Hit, AController* HitInstigator);
	
	float TimeUntilNextLookCheck = 0.0f;
	float TimeNotBeingLookedAt = 0.0f;
	float RequiredTimeNotBeingLookedAt = 15.0f;

	UFUNCTION(BlueprintPure)
	float GetVisibleSWATPercentage() const;

	TArray<FTraceHandle> VisibleSwatTraceHandles;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI")
	float VisibleSwatPercentage = 0.0f;

	float CachedIntimidatorValue = -1.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = MoveStyle)
	class URoNMoveStyleComponent* MoveStyle;
	
	// Goes from 100 < 0 then applys a stun then 'resets'
	float StunHealthRemaining = 100;

	// Current accuracy penalty percentage applied by stuns (like the nine-banger)
	float StunAccuracyPenalty = 0.0;

	float PepperSprayAccuracyPenalty = 0.0;

	// Stun accuracy penalty percentage recovery per second
	float StunAccuracyPenaltyRecovery = 0.025f;
	float PepperSprayAccuracyPenaltyRecovery = 3.5;

	FVector ArrestedPushLocation = FVector::ZeroVector;

	bool bHasEverExitedSurrender = false;
	bool bShouldKnifeUpCloseWhenBeingLookedAt = false;

	UPROPERTY(BlueprintReadOnly)
	float ForceComplianceStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	ECoverDirection ActiveCoverDirection = ECoverDirection::Left;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	ECoverAimType ActiveCoverAimType = ECoverAimType::LeftOrRight;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	ECoverFireType ActiveCoverFireType = ECoverFireType::None;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	UAnimSequence* ActiveCoverFirePose = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	UAnimSequence* ActiveCoverIdlePose = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly)
	class ACoverLandmark* CurrentCoverLandmarkInUse = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	class ACoverLandmark* LastCoverLandmarkUsed = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	class AWallHoleTraversal* CurrentWallHoleTraversalInUse = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	class AWallHoleTraversal* LastWallHoleTraversalUsed = nullptr;
	
	float StunDamageUntilNextStun = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "Voice Lines")
	UFMODEvent* VoiceLineEventMask;

	float AccuracyNerfPercentage = 1.0f;

	UPROPERTY(ReplicatedUsing=OnRep_SimulatingAttachedStaticMeshes)
	TArray<UStaticMeshComponent*> SimulatingAttachedStaticMeshes;

	UFUNCTION()
	void OnRep_SimulatingAttachedStaticMeshes();

	UPROPERTY(ReplicatedUsing=OnRep_AttachedMeshData)
	TArray<FAttachedMeshData> AttachedMeshData;

	UFUNCTION()
	void OnRep_AttachedMeshData();

	UPROPERTY(ReplicatedUsing=OnRep_AttachedSkeletalMeshData)
	TArray<FAttachedSkeletalMeshData> AttachedSkeletalMeshData;
		
	UFUNCTION()
	void OnRep_AttachedSkeletalMeshData();

	void HandleLimbDismembered(FName Bone);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bIsMoving;

	FVector AvoidanceLocation = FVector::ZeroVector;
	float AvoidanceTime = 0.0f;
	
	FVector CachedSpine3Offset;
	float TimeSinceLastMadeLowReady = 999.0f;
	float LastAimOffSetWhenLowReady = 0.0f;

	UPROPERTY()
	ABaseItem* LastEquippedBreachItem = nullptr;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurrendered, AReadyOrNotCharacter*, Character);
	UPROPERTY(BlueprintAssignable)
	FOnSurrendered OnSurrendered;
	UPROPERTY(BlueprintAssignable)
	FOnSurrendered OnFakeSurrendered;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpottedCharacter, ACyberneticCharacter*, Spotter, AReadyOrNotCharacter*, Character);
	UPROPERTY(BlueprintAssignable)
	FOnSpottedCharacter OnSpottedEnemy;
	UPROPERTY(BlueprintAssignable)
	FOnSpottedCharacter OnSpottedFriendly;
	UPROPERTY(BlueprintAssignable)
	FOnSpottedCharacter OnSpottedNeutral;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensedActor, AActor*, Actor);
	UPROPERTY(BlueprintAssignable)
	FOnSensedActor OnSensedActor;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensedCharacter, AReadyOrNotCharacter*, Character);
	UPROPERTY(BlueprintAssignable)
	FOnSensedCharacter OnSensedCharacter;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIFinishedSpawningDelegate);
	UPROPERTY(BlueprintAssignable)
	FAIFinishedSpawningDelegate OnAIFinishSpawning;
	
	DECLARE_DELEGATE_OneParam(FDoorBreachFinishedDelegate, ABaseItem*);
	FDoorBreachFinishedDelegate OnDoorBreachFinished;
	
	// just a meme text render for lewd
	UPROPERTY()
	UTextRenderComponent* NoBuenoTextRender = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | Abuse")
	int32 AbuseCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics")
	TArray<FString> ReasonsToSprint = {};
	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics")
	TArray<FString> ReasonsToStandStill = {};
	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics")
	TArray<FString> ReasonsToWalk = {};

	UPROPERTY(Replicated, BlueprintReadWrite)
	FCoverAnimStateMachineData Rep_CoverAnimState;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	FHidingAnimStateMachineData Rep_HidingAnimState;

	UPROPERTY(Replicated, BlueprintReadWrite)
	FHoleTraversalAnimStateMachineData Rep_HoleTraversalAnimState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | Aim Offset")
	FVector2D AimOffset;
	FVector2D PreviousAimOffset;

	float AimOffsetAlpha = 0.0f;

	float TimeSinceExitStunned = 0.0f;
	uint8 bWasStunned : 1;

	UPROPERTY()
	TMap<AReadyOrNotCharacter*, int32> MeleeCountMap;

	/* the activities to use when civilians should cower */
	UPROPERTY(EditAnywhere, Category = "Civilian")
	TArray<TSubclassOf<class UWorldBuildingActivity>> CivilianCowerActivities;

	UPROPERTY(EditAnywhere, Category = "Civilian")
	float CivilianCowerActivityDuration = 10.0f;

	int32 RandGenUnarmedCalmPick = 0;
	int32 RandGenUnarmedPanicPick = 0;
	
	// cannot be lower than anim instance blend time or it will pop between move styles!
	float TimeSinceLastMoveStyleUpdate = 0.5f;
	float MoveStyleUpdateTime = 0.5f;
	virtual void UpdateDefaultMoveStyle();

	UPROPERTY(BlueprintReadOnly, Category = "Factions")
	class AAIFactionManager* FactionManager = nullptr;

	UFUNCTION(BlueprintPure)
	bool IsSameFaction(ACyberneticCharacter* OtherAI) const;

	FAIFactionTable FactionData;

	FSpawnData* GetSpawnData();
	const FSpawnData* GetSpawnData() const;

	// The data this AI was spawned with (usually from the AI spawner)
	FSpawnData* SpawnData = nullptr;
	
	// Use this if no data from the spawner so we don't get a null reference anywhere
	FSpawnData DefaultSpawnData;

	UPROPERTY(BlueprintReadOnly)
	AAISpawn* SpawnedFromSpawner = nullptr;

	UPROPERTY(Replicated)
	ECombatState CombatState = ECombatState::CS_Unaware;
	
	uint8 bDeactivated : 1;
	uint8 bHasAddedScoreGroups : 1;

	UPROPERTY()
	AReadyOrNotCharacter* RecentMeleeVictim = nullptr;
	UPROPERTY()
	AReadyOrNotCharacter* PendingMeleeTarget = nullptr;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bHasEverShot = false;

	bool bBroadcastedStressIncrease = false;
	
	// IScoringInterface
	///////////////////////////////////
	virtual class UScoringComponent* GetScoringComponent_Implementation() const override;
	///////////////////////////////////

	// IReportable
	///////////////////////////////////
	virtual bool CanReportNow_Implementation() override;
	virtual FString GetSpeechTypeForReport_Implementation() override;
	///////////////////////////////////
	
	// IUseablilityInterface
	///////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual FText DetermineActionText_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;
	virtual float DetermineInteractionDistance_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	///////////////////////////////////

	virtual void Secure_Implementation(AReadyOrNotCharacter* InInstigator) override;
	virtual bool IsSecured_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
	
	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;
	
#if WITH_EDITOR	
	UFUNCTION(CallInEditor, Category="Debug Voice Line")
	virtual void PlayDebugVoiceLineFromTable();

	UFUNCTION(CallInEditor, Category="Debug Voice Line")
	virtual void PlayDebugConversation();
#endif
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Voice Line")
	FString DebugVoiceLine;
#endif

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponForceFireNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnWeaponForceFireNotifyEvent OnWeaponForceFire_FromAnimNotify;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorBreachNotifyEvent);
	UPROPERTY(BlueprintAssignable)
	FOnDoorBreachNotifyEvent OnDoorShotgunBreach_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDoorBreachNotifyEvent OnDoorKickBreach_FromAnimNotify;
	UPROPERTY(BlueprintAssignable)
	FOnDoorBreachNotifyEvent OnDoorRamBreach_FromAnimNotify;
	
	UPROPERTY(BlueprintAssignable)
	FOnItemThrownNotifyEvent OnPendingItemThrown_FromAnimNotify;
	
	UPROPERTY(BlueprintReadWrite)
	ABaseItem* PendingThrownItem = nullptr;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIFire, ACyberneticCharacter*, AICharacter, ABaseMagazineWeapon*, MagazineWeapon, FVector, FireDirection);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnAIFire OnAIFire;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCollectPendingEvidenceNotify);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCollectPendingEvidenceNotify OnCollectPendingEvidenceBegin_FromAnimNotify;
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCollectPendingEvidenceNotify OnCollectPendingEvidenceEnd_FromAnimNotify;
	
	FTimerHandle TH_MontageFocalPointRemoval;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeardOfficerYellSignature, AReadyOrNotCharacter*, Shouter, bool, bLOS);
	UPROPERTY(BlueprintAssignable)
	FOnHeardOfficerYellSignature OnHeardOfficerYell;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExitedSurrender, ACyberneticCharacter*, Character, ESurrenderExitType, ExitType);
	UPROPERTY(BlueprintAssignable)
	FOnExitedSurrender OnExitedSurrender;
	
	float FakeSurrenderTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float SuppressionAmount = 0.0f;
	
	/////////////////////////////
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	ACyberneticCharacter* BeingRestrainedBy = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Scoring")
	class UScoringComponent* ScoringComponent = nullptr;

	//UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI")
	const FAIDataLookupTable* AssignedAIData;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Personality")
	UAIArchetypeData* Archetype = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	FVector SpawnLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly)
	class UAIArchetypeData* DefaultSuspectArchetype = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	class UAIArchetypeData* DefaultCivilianArchetype = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "AI")
	FActivityRouteCollection ActivityRouteCollection;

	UPROPERTY(Replicated)
	FVector Rep_AimOffsetFocalPoint = FVector::ZeroVector;
	
	UPROPERTY(Replicated)
	FVector Rep_FocalPoint = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector Rep_HeadFocalPoint = FVector::ZeroVector;

	UPROPERTY(Replicated)
	AActor* Rep_FocalActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bHasLOSToFocalPoint = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Yell)
	uint8 bHeardYellFromOfficer : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = Yell)
	float TimeSinceHeardOfficerYell = FLT_MAX;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	float TimeSinceLastAggressiveForce = 999999.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = Arrest)
	float ArrestedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = Visuals)
	bool bFemale = false;

	UPROPERTY(BlueprintReadOnly, Category = Visuals)
	bool bChild = false;

	UPROPERTY()
	class UAnimMontage* LastGetUpMontage = nullptr;
	
	UPROPERTY(Replicated)
	uint8 bRecoveringFromRagdoll : 1;
	
	UPROPERTY(Replicated)
	uint8 bIsKnockedOut : 1;

	UPROPERTY(Replicated)
	uint8 bIsPlayingDead : 1;

	uint8 bWantsReload : 1;

	UPROPERTY(BlueprintReadOnly)
	float TimeHiding = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float TimePlayingDead = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastPlayDead = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceAtLastCoverLandmark = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastSeenCharacterWhilstHiding = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceSeenCharacterNotLookingWhilstHiding = FLT_MAX;
	UPROPERTY(BlueprintReadOnly)
	bool bHasEverSeenCharacterWhilstHiding = false;

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* CharacterSeenWhilstHiding = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Unarmed_Calm_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Unarmed_Sr_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Unarmed_Ar_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Unarmed_Ar_Crouch_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Unarmed_Alert_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Rifle_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Pistol_AD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	UReadyOrNotCharacterAnimData* Pistol_OneHanded_AD;

	UPROPERTY()
	ACyberneticCharacter* AvoidingCharacter = nullptr;
	
	// if specified with override the move data
	UPROPERTY()
	FAIMoveDataBlock MoveDataOverride;
	// used for fast access
	UPROPERTY(EditAnywhere)
	FAIMoveDataBlock CurMoveDataBlock;
	
	// Contains all the movement style names for this AI, probably replacing above
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAIMovementStyleData MovementStyleData;

	bool bOverridingMoveStyle = false;

	void RespondToFriendlyFire(AReadyOrNotCharacter* InstigatorCharacter);

	// How often is an AI to flee or surrender (will be reduced over the mission time as they hear more gunshots and things)
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float Stress = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float StartingStress = 0.0f;

	UFUNCTION(BlueprintCallable)
	void IncreaseStress(float Amount);
	UFUNCTION(BlueprintCallable)
	void DecreaseStress(float Amount);
	
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float TimeSinceLastShot = FLT_MAX;
	
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float TimeSinceArrest = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bHasPlayedSurrenderAnim = false;
	UPROPERTY(BlueprintReadOnly, Replicated,Category = "AI")
	bool bIsFakeSurrender = false;
	UPROPERTY(BlueprintReadOnly, Replicated,Category = "AI")
	bool bHasEverFakeSurrendered = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bFinishedEquippingLoadout = false;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bFinishedEquippingArmour = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bForceFiringEnabled = false; // If Force firing is enabled then the AI will be able to shoot even while playing montages

	UPROPERTY(ReplicatedUsing=OnRep_CharacterMeshData)
	FCharacterMesh CharacterMeshData;
	UFUNCTION()
	void OnRep_CharacterMeshData();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendCharacterMeshData(FCharacterMesh RPC_CharacterMeshData);
	void Multicast_SendCharacterMeshData_Implementation(FCharacterMesh RPC_CharacterMeshData);

	// this is replicated but sending it may also help
	bool bHasSentCharacterMeshData = false;

	// this is called whenever this character aimed at somebody
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAimedAt, ACyberneticCharacter*, Character, AReadyOrNotCharacter*, Target);
	UPROPERTY(BlueprintAssignable)
	FOnAimedAt OnAimedAt;

	float TimeNotStrafing = 0.0f;

	float NoPathCooldown = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	uint8 bAimingAtTarget : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bHasEverAimedAtTarget : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bIsFleeing : 1;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bIsExitingLandmark : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bDrawingWeapon : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bPickingUpWeapon : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bHasDamagedSWATTeam : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bHitScannedFriendly : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bDiedWhilstTraversingHole : 1;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	uint8 bDiedWhilstHiding : 1;
	UPROPERTY(Replicated)
	uint8 bIsRaisingWeapon : 1;
	UPROPERTY(Replicated)
	uint8 bIsLoweringWeapon : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float DrawingWeaponTime = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float PickingUpWeaponTime = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float RaisingWeaponTime = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float LoweringWeaponTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AI")
	FHitResult CachedHitScanResult;

	UPROPERTY(Replicated, BlueprintReadWrite)
	FWorldBuildingAnimState Rep_WorldBuildingAnimState;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	FTakeHostageAnimState Rep_TakeHostageAnimState;
	
	bool bCommitingSuicide = false;

	UPROPERTY(EditDefaultsOnly, Category = "Hostage Take")
	UAnimSequence* HostageMasterIdleLoop = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hostage Take")
	UAnimSequence* HostageSlaveIdleLoop = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Hostage Take")
	ACyberneticCharacter* TakenHostageBy = nullptr;

	bool bHasBreathedWhileIncapacitated = false;
	
	float FireAngleThreshold = 0.95f;
	float TimeInsideFireAngleThreshold = 0.0f;
	
	FName SeenBone = NAME_None;

	FTraceHandle GroundCheckHandle;
	FTraceHandle LowReadyCheckHandle;
	bool bPreviousLowReadyTraceHit = false;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Cybernetics | General")
	float TimeSurrendered = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	#if !UE_BUILD_SHIPPING
	virtual void Tick_Debug(float DeltaSeconds) override;
	#endif
	
	virtual void Tick_Authority(float DeltaSeconds) override;
	
	virtual void ApplyHeadDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftArmDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightArmDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftLegDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightLegDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftFootDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightFootDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyBodyDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void OnIncapacitated(AReadyOrNotCharacter* InstigatorCharacter) override;

	//bool IsBeingLookedAt_Old(const bool bPlayerOnly, const float Threshold, float& OutClosestDistance, APawn*& OutClosestPawn);
	bool IsBeingLookedAt(const float Threshold, float& OutClosestDistance, APawn*& OutClosestPawn);
	
	void StopAllMovement();
	
	void ResetSurrenderStates();
	void ResetFakeSurrenderStates();

	void StopAnimationMontage(UAnimMontage* Montage);
	
	virtual bool CanPlayDeathAnimation() const override;

	virtual void OnEquippedWeaponFire(ABaseMagazineWeapon* Weapon, bool bServer) override;

	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | General")
	uint8 bIsComplying : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | General")
	uint8 bIsWaiting : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | Abuse")
	float PepperSprayAbuseLevel = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | Taser")
	float TimeSinceLastTasered = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Cybernetics | Taser")
	int32 TimesTasered = 0;

	UPROPERTY(Replicated)
	FVector RagdollMeshLocation = FVector::ZeroVector;
	UPROPERTY(Replicated)
	FRotator RagdollMeshRotation = FRotator::ZeroRotator;
	
	bool bRagdollSetCapsuleOnce = false;

	UPROPERTY(BlueprintReadOnly)
	float HesitationTime = 0.0f;
	float ForceFireGunDelay = 0.0f;
	
	FTimerHandle StopMontageSection_Handle;
	FTimerHandle EquipLoadout_DelayHandle;
	FTimerHandle ReloadDelay_Handle;

public:
	float CurrentPitchInterpolated;
	float CurrentYawInterpolated;

	/* used in look at controller function */
	UPROPERTY(EditAnywhere, Category = "Aiming")
	float FocalPointInterpSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	EAlphaBlendOption FocalPointInterpCurve;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	EAlphaBlendOption AimOffsetInterpCurve;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	float FocusTurnSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	float TurnDegreesPerSecond;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	float ActorRotationInterpStandingSpeed;

	UPROPERTY(EditAnywhere, Category = "Aiming")
	float ActorRotationInterpMovingSpeed;

	UPROPERTY(EditAnywhere, Category = "Aiming")
	float AimOffsetInterpSpeed;

	UFUNCTION(BlueprintCallable, Category = "Head Look")
	FRotator GetLookAtRotation(float YawLimit, float PitchLimit) const;
	
	UPROPERTY(EditAnywhere, Category = "EQS")
	UEnvQuery* EscapeGasQuery;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ASafeZoneVolume* LastUsedSafeZone;
	
	UPROPERTY()
	UWidgetComponent* DebugAISelectionWidget = nullptr;
};
