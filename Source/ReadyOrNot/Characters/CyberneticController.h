// Copyright Void Interactive, 2021

#pragma once

#include "AIController.h"
#include "AI/AIDataTypes.h"
#include "Actors/Gameplay/AISpawn.h"
#include "Components/ReadyOrNotPathFollowingComp.h"
#include "Components/TargetingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Info/Activities/CombatMove/HardCoverCombatMove.h"
#include "CyberneticController.generated.h"

class UMoraleComponent;
class UTargetingComponent;
class UBaseCombatActivity;

class AThreatAwarenessActor;

class APlayerCharacter;

class UAISenseConfig_Sight;
class UAISenseConfig_Touch;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;

static FName RON_SENSE_SIGHT = "Default__ReadyOrNotAISense_Sight";
static FName RON_SENSE_HEARING = "Default__AISense_Hearing";
static FName RON_SENSE_DAMAGE = "Default__AISense_Damage";

extern TAutoConsoleVariable<int32> CVarRonToggleAIDebugLines;
extern TAutoConsoleVariable<int32> CVarRonDrawAIDebug;
extern TAutoConsoleVariable<int32> CVarRonDrawFocalPoint;

namespace EAIFocusPriority
{
	typedef uint8 Type;

	constexpr Type Montage = 3;
}

UENUM()
enum class EActorSenseType : uint8
{
	Sight,
	Sound,
	Damage
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAITargetType : uint8
{
	None	 = 0,
	Enemy	 = 1 << 0,
	Neutral  = 1 << 1,
	Friendly = 1 << 2
};
ENUM_CLASS_FLAGS(EAITargetType);

USTRUCT()
struct FActorSense
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Actor = nullptr;

	FName Tag = NAME_None;
	
	FAIStimulus Stimulus;
	
	float SenseTime = 0.0f;
	float SenseReactionTime = 0.0f;
	float SenseForgetTime = 0.0f;

	friend bool operator==(const FActorSense& Lhs, const FActorSense& Rhs)
	{
		return Lhs.Actor == Rhs.Actor &&
				Lhs.Tag == Rhs.Tag;
	}
};

USTRUCT()
struct FActorMemory
{
	GENERATED_BODY()
	
	TArray<FVector> Locations;
};

DECLARE_STATS_GROUP(TEXT("CyberneticController"), STATGROUP_CyberneticController, STATCAT_Advanced);

UCLASS()
class READYORNOT_API ACyberneticController : public AAIController
{
	GENERATED_BODY()

public:
	explicit ACyberneticController(const FObjectInitializer& ObjectInitializer);

	FORCEINLINE UMoraleComponent* GetMoraleComp() const { return MoraleComponent; }
	FORCEINLINE UTargetingComponent* GetTargetingComp() const { return TargetingComponent; }
	FORCEINLINE UAIPerceptionComponent* GetRONPerceptionComp() const { return AIPerceptionComponent; }
	FORCEINLINE UReadyOrNotPathFollowingComp* GetRONPathFollowingComp() const { return Cast<UReadyOrNotPathFollowingComp>(GetPathFollowingComponent()); }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMoveToActivity* GetPushMoveToActivity() const { return PushMoveToActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMoveToActivity* GetAvoidanceMoveToActivity() const { return AvoidanceMoveToActivity; }
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIMoveCompleted, AAIController*, Controller, int32, RequestID);

	float UtilityAIThinkRate = 0.1f;
	float TimeSinceThink = 0.1f;
	float TimeSinceTick = 0.1f;
	bool TickUtilityAI(float DeltaTime);
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	TArray<UAIAction*> CustomActions;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAIPerceptionComponent* AIPerceptionComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UReadyOrNotAISenseConfig_Sight* SightConfig = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAISenseConfig_Touch* TouchConfig = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAISenseConfig_Damage* DamageConfig = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAISenseConfig_Hearing* HearingConfig = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UMoraleComponent* MoraleComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTargetingComponent* TargetingComponent = nullptr;
	
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& TestActors);

	void ProcessActorSightStimulus(AActor* SensedActor, FAIStimulus Stimulus);
	void ProcessActorHearingStimulus(AActor* SensedActor, FAIStimulus Stimulus);
	void ProcessActorDamageStimulus(AActor* SensedActor, FAIStimulus Stimulus);

	virtual void ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor);

public:
	AReadyOrNotCharacter* SensedActorToCharacter(AActor* SensedActor) const;

	UPROPERTY(BlueprintReadOnly)
	TMap<AActor*, float> LastHeardActorTime;

	UPROPERTY(BlueprintReadOnly)
	APlayerCharacter* SensingCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	AActor* LastSensedActor = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* HeardActorInstigator = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* LastSensedCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	FVector LastSensedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	uint8 bCanOpenDoorThroughNavLink : 1;

	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastMove = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float TimeUntilRecentlySeenCharactersClear = 1.0f;

	UPROPERTY()
	TArray<AReadyOrNotCharacter*> RecentlySeenSwat;
	UPROPERTY()
	TArray<AReadyOrNotCharacter*> RecentlySeenSuspects;
	UPROPERTY()
	TArray<AReadyOrNotCharacter*> RecentlySeenCivilians;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncPathFoundDelegate, int32, PathId, ERonNavigationQueryResult, Result);
	UPROPERTY(BlueprintAssignable)
	FAsyncPathFoundDelegate OnAsyncPathFound;
	
	TArray<uint32> AsyncPathRequestIDs;

	TMap<int32, int32> MoveRequestIdToPathIdMap;

	UFUNCTION(BlueprintCallable)
	int32 RequestMoveAsync(FVector Location, bool bProjectToNavigation = true, float AcceptanceRadius = 10.0f);

	UPROPERTY(VisibleInstanceOnly, Category = "Path Finding")
	float LastAcceptanceRadius = 10.0f;

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsFindingPath(int32 PathId) const { return AsyncPathRequestIDs.Contains((uint32)PathId); }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsFindingAnyPath() const { return AsyncPathRequestIDs.Num() > 0; }

	void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	
	//UFUNCTION(BlueprintCallable, meta = (DisplayName = "Request Move (Async)"))
	//void K2_RequestMoveAsync(FAsyncMoveRequestDelegate Event, FVector Location, bool bProjectToNavigation = false);
	
	static FPathFindingQuery CreatePathFindingQuery(const FSharedConstNavQueryFilter& NavQueryFilter, const ANavigationData* NavData, const FNavLocation& InStartLocation, const FNavLocation& InEndLocation, bool bAllowPartialPaths = false, TWeakObjectPtr<const UObject> Owner = nullptr);
	static FAIMoveRequest CreateMoveRequest(const FVector& MoveLocation, float AcceptanceRadius, const TSubclassOf<UNavigationQueryFilter> NavQueryFilter, bool bUsePathfinding, bool bCanStrafe, bool bAllowPartialPath, bool bProjectGoalLocation, bool bReachTestIncludesAgentRadius);
	
	#if !UE_BUILD_SHIPPING
	FString LastActivityFailedReason = "";
	#endif
	
	UPROPERTY()
	TMap<AReadyOrNotCharacter*, float> DamagedBy;
	UPROPERTY()
	TMap<AReadyOrNotCharacter*, FVector> DamagedByLocation;

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastMove() const { return TimeSinceLastMove; }

	UFUNCTION(BlueprintPure)
	bool HasRecentlySeenSwat(FVector& OutLocation) const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasRecentlySeenSuspect() const { return RecentlySeenSuspects.Num() > 0; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasRecentlySeenCivilian() const { return RecentlySeenCivilians.Num() > 0; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE EAIAwarenessState GetAwarenessState() const { return AwarenessState; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE AReadyOrNotCharacter* GetHeardActorInstigator() const { return HeardActorInstigator; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMoveToActivity* GetMoveToActivity() const { return MoveToActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMoveActivity* GetMoveActivity() const { return TeamMoveActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMoveToExitActivity* GetMoveToExitActivity() const { return MoveToExitActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UTargetNextCivilianActivity* GetTargetNextCivilianActivity() const { return TargetNextCivilianActivity; }
		
	// how many bullets we have fired towards accruacy penalty
	// different from bullets fired because this can be reset with stuns/supression etc
	uint32 BulletsFiredTowardsAccuracyPenalty = 0;
	uint32 TotalBulletsFired = 0;
	
	bool DoesPathGoPastKnownEnemy(FNavPathSharedPtr NavPath);
	void InvestigateStimulus(const FAIStimulus& Stimulus);

	void MoveToCover(const FCoverInstigatorStimulus& InstigatorStimulus, bool bRequireLOS = false);
	
	FSpawnData GetSpawnData();

	UPROPERTY()
	class UInvestigateStimulusActivity* InvestigateStimulusActivity = nullptr;
	
	UFUNCTION(BlueprintPure, BlueprintNativeEvent)
	bool IsCharacterNeutral(AReadyOrNotCharacter* InCharacter) const;
	UFUNCTION(BlueprintPure, BlueprintNativeEvent)
	bool IsCharacterEnemy(AReadyOrNotCharacter* InCharacter) const;
	UFUNCTION(BlueprintPure, BlueprintNativeEvent)
	bool IsCharacterFriendly(AReadyOrNotCharacter* InCharacter) const;
	
	UFUNCTION(BlueprintPure)
	bool DoesCharacterMatchTargetType(AReadyOrNotCharacter* InCharacter, int32 TargetTypeMask) const;

	UFUNCTION()
	void OnStunDamageTaken(EStunType StunType);
	
protected:
	UFUNCTION()
	void OnDoorExploded(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter);

	virtual void OnHeardSWAT(AReadyOrNotCharacter* HeardCharacter);
	virtual void OnHeardGunShot(AReadyOrNotCharacter* InInstigator, const FVector& WeaponLocation, const FName& InTag);
	virtual void OnHeardDoor(AReadyOrNotCharacter* InInstigator, ADoor* Door, const FName& InTag);
	virtual void OnHeardGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation, const FName& InTag);
	virtual void OnHeardExplosion(AReadyOrNotCharacter* InInstigator, const FVector& ExplosionLocation);

	virtual bool CanSpotCharacter(AReadyOrNotCharacter* SensedCharacter) const { return true; }

	virtual bool IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const;
	virtual bool IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const;
	virtual bool IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const;
	
	virtual void OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus);
	virtual void OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus);
	virtual void OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus);
	
	virtual void OnSeenGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation);
	
	virtual void OnSeenFlashlight(AReadyOrNotCharacter* InInstigator, const FVector& FlashlightLocation);
	
	virtual void OnSeenIncapHuman(class AIncapacitatedHuman* IncapHuman);

	void OnRespondToSpottedSuspectAndCivilian();
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	UFUNCTION()
	virtual void OnAIFinishSpawning();
	
	uint8 NewEnemies = 0;
	uint8 NewNeutrals = 0;

	void TryStopCurrentConversation();

	virtual void OnAwarenessStateChanged();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAwarenessChangeEvent, ACyberneticController*, Controller, EAIAwarenessState, Previous, EAIAwarenessState, Current);
	UPROPERTY(BlueprintAssignable)
	FAwarenessChangeEvent OnAwarenessStateChangedDelegate;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	EAIAwarenessState PreviousAwarenessState = EAIAwarenessState::Unalerted;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	EAIAwarenessState AwarenessState = EAIAwarenessState::Unalerted;

public:
	#if !UE_BUILD_SHIPPING
	virtual void DisplayAIDebugInfo(float DeltaTime);
	#endif

	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;
	virtual void InstigatedAnyDamage(float Damage, const class UDamageType* DamageType, class AActor* DamagedActor, class AActor* DamageCauser) override;
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;
	
	virtual void OnTrackedTargetKilled(AReadyOrNotCharacter* Insitgator, AReadyOrNotCharacter* KilledCharacter);
	virtual void OnTrackedTargetIncapacitated(AReadyOrNotCharacter* KilledCharacter);
	virtual void OnTrackedTargetExitedSurrender(ACyberneticCharacter* InCharacter, ESurrenderExitType ExitType);
	virtual void OnTrackedTargetStartedReloading(APlayerCharacter* InCharacter);
	
	void SpottedEnemy(AReadyOrNotCharacter* Enemy);
	void SpottedFriendly(AReadyOrNotCharacter* Friendly);
	void SpottedNeutral(AReadyOrNotCharacter* Neutral);
	
	void PushCharacter(FVector ToLocation, bool bNoMoveBack);
	void LookAt(float DeltaTime);
	
	EAIFocusPriority::Type PreviousFocusType = EAIFocusPriority::Default;
	FVector TargetFocalPoint = FVector::ZeroVector;
	FVector CurrentFocalVector = FVector::ZeroVector;
	FVector StartingFocalVector = FVector::ZeroVector;
	float DistanceToCurrentFocalPoint = 0.0f;
	float FocalVectorAlpha = 0.0f;
	uint8 bStartFocalInterp : 1;
	uint8 bFocalPointIsRight : 1;
	uint8 bShouldTurn : 1;

	uint8 bCanTrackMoveVectorWhileStrafe : 1;
	uint8 bHasEverSpottedEnemyBefore : 1;
	uint8 bHasEverSpottedSWAT : 1;
	uint8 bTriggeredFullTurn : 1;
	uint8 bCanCallForHelp : 1;
	uint8 bInAlertedStimulusState : 1;
	uint8 bInSuspiciousStimulusState : 1;
	uint8 bForceStrafe : 1;
	uint8 bHasCalledForBackup : 1;
	
	UPROPERTY(BlueprintReadOnly)
	FAIStimulus LatestStimulus;

	UPROPERTY(BlueprintReadOnly)
	FAIStimulus LatestSightStimulus;
	UPROPERTY(BlueprintReadOnly)
	FAIStimulus LatestHearingStimulus;
	UPROPERTY(BlueprintReadOnly)
	FAIStimulus LatestDamageStimulus;
	
	float TimeSinceFirstSpottedEnemy = 0.0f;
	float CallDelay = 0.0f;

protected:
	float TimeSpentADSAt = 0.0f;
	float TimeSpendingADSBeforeAbuse = 5.0f;
	
	UPROPERTY()
	TArray<AActor*> TrackedDistractions;
	FVector LastFocalPoint;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	DECLARE_DELEGATE_ThreeParams(FOnMoveCompleted, AAIController*, FAIRequestID, const FPathFollowingResult&);

public:
	UFUNCTION(BlueprintPure, Category = "Cybernetics")
	AReadyOrNotCharacter* GetTrackedTarget();
	AReadyOrNotCharacter* GetLastTrackedEnemy();

	UFUNCTION(BlueprintPure)
	FORCEINLINE FAIStimulus GetLatestStimulus() const { return LatestStimulus; }

	FOnMoveCompleted OnMoveCompletedDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnAIMoveCompleted OnMoveComplete;

	virtual float GetReactionTime(const EActorSenseType& SenseType) const;

	//void AddActorSense(EActorSenseType SenseType, const FActorSense& NewActorSense);
	//void RemoveActorSense(EActorSenseType SenseType, AActor* InActor, const FName& InTag);
	//bool IsSensingActor(EActorSenseType SenseType, AActor* InActor) const;
	//bool IsSensingActorWithTag(EActorSenseType SenseType, AActor* InActor, const FName& InTag) const;
	//float GetSenseTimeForActor(EActorSenseType SenseType, AActor* InActor, const FName& InTag) const;

	void AddActorSightSense(FActorSense& NewActorSense);
	void AddActorSoundSense(FActorSense& NewActorSense);
	void AddActorDamageSense(FActorSense& NewActorSense);
	void RemoveActorSightSense(AActor* InActor, const FName& InTag);
	void RemoveActorSoundSense(AActor* InActor, const FName& InTag);
	void RemoveActorDamageSense(AActor* InActor, const FName& InTag);
	bool IsSightReactingToActor(AActor* InActor) const;
	bool IsSoundReactingToActor(AActor* InActor) const;
	bool IsDamageReactingActor(AActor* InActor) const;
	bool IsSightReactingToActorWithTag(AActor* InActor, const FName& InTag) const;
	bool IsSoundReactingToActorWithTag(AActor* InActor, const FName& InTag) const;
	bool IsDamageReactingToActorWithTag(AActor* InActor, const FName& InTag) const;
	float GetSightSenseTimeForActor(AActor* InActor, const FName& InTag) const;
	float GetSoundSenseTimeForActor(AActor* InActor, const FName& InTag) const;
	float GetDamageSenseTimeForActor(AActor* InActor, const FName& InTag) const;

	FORCEINLINE const TArray<FActorSense>& GetActorSightSenseMap() const { return ActorSightSenseMap; }
	FORCEINLINE const TArray<FActorSense>& GetActorSoundSenseMap() const { return ActorSoundSenseMap; }
	FORCEINLINE const TArray<FActorSense>& GetActorDamageSenseMap() const { return ActorDamageSenseMap; }
	
	//const TArray<FActorSense>& GetActorSenseMap() const;

	UPROPERTY()
	TArray<FActorSense> ActorSightSenseMap;
	UPROPERTY()
	TArray<FActorSense> ActorSoundSenseMap;
	UPROPERTY()
	TArray<FActorSense> ActorDamageSenseMap;

	virtual void OnSeenActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus);
	virtual void OnHeardActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus, float ExpiryTime = 10.0f);
	virtual void OnDamagedByActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus);

	// Let us take control over their behaviour and stop thinking for themselves
	UPROPERTY(BlueprintReadWrite)
	uint8 bStopDecisionMaking : 1;
	uint8 bStopUtilityTick : 1;
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Activity")
	class UBaseActivity* CurrentActivity = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Activity")
	TArray<class UBaseActivity*> ActivityQueue;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActivityComplete, class UBaseActivity*, CompletedActivity, class UBaseActivity*, NextActivity, bool, bSuccessfullyCompleted);
	FOnActivityComplete OnActivityComplete;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Call for Help")
	float CallForHelpCoolDownDuration = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Call for Help")
	float MaxHearingForHelpDistance = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flash Light Seen")
	float FlashLightSeenCoolDownDuration = 7.5f;

	void PerformActivity(UBaseActivity* InActivity, float DeltaTime);
	
public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE ACyberneticCharacter* GetCharacter() const { return GetPawn<ACyberneticCharacter>(); }
	
	template<class T = ACyberneticCharacter>
	FORCEINLINE T* GetCharacter() const { return GetPawn<T>(); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE UBaseCombatActivity* GetCombatActivity() const { return CombatActivity; }
	
	template<class T = UBaseCombatActivity>
	T* GetCombatActivity() const;

	UFUNCTION(BlueprintCallable, Category = "AddActivity")
	bool AddActivity(class UBaseActivity* Activity, bool bOverrideCurrentActivity = false);

	void ClearActivityList(bool bKeepCurrent = false);

	UFUNCTION(BlueprintPure, Category = "Targeting")
	bool IsCharacterKnownEnemy(AReadyOrNotCharacter* InCharacter) const;
	
	UFUNCTION(BlueprintPure)
	int32 GetSuccessConsiderCountForAction(FName Action) const;
	UFUNCTION(BlueprintPure)
	int32 GetFailedConsiderCountForAction(FName Action) const;
	
	UFUNCTION(BlueprintCallable, Category = "FinishActivity")
	void FinishActivity(class UBaseActivity* Activity, bool bSuccess, bool bForce = false);
	
	UFUNCTION(BlueprintPure)
	UBaseActivity* GetActivity(TSubclassOf<class UBaseActivity> ActivityType) const;
	
	template<class T = UBaseActivity>
	T* GetActivity() const;
	
	UFUNCTION(BlueprintPure, Category = "HasActivity")
	bool HasActivityType(TSubclassOf<class UBaseActivity> ActivityType);

	UFUNCTION(BlueprintCallable, Category = "RemoveActivity")
	void RemoveActivitiesOfType(TSubclassOf<class UBaseActivity> ActivityType, bool bClearCurrent = true);
	
	UFUNCTION(BlueprintCallable, Category = "RemoveActivity")
	void RemoveAllActivitiesExcept(TSubclassOf<class UBaseActivity> ActivityType);

	template<class T = UBaseActivity>
	T* GetCurrentActivity() const;

	UFUNCTION(BlueprintCallable, Category = "Queue")
	FString GetActivityQueueAsString();
	
	UFUNCTION(BlueprintCallable, Category = "Queue")
	FORCEINLINE int32 GetActivityQueueCount() const { return ActivityQueue.Num(); }

	UFUNCTION()
	void OnKnownEnemyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION()
	void OnKnownEnemyIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	UFUNCTION()
	void OnKnownEnemyTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION()
	void OnKnownEnemyStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	
	UFUNCTION()
	void OnKnownFriendlyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION()
	void OnKnownFriendlyIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	UFUNCTION()
	void OnKnownFriendlyTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION()
	void OnKnownFriendlyStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	
	UFUNCTION()
	void OnKnownNeutralKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION()
	void OnKnownNeutralIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	UFUNCTION()
	void OnKnownNeutralTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION()
	void OnKnownNeutralStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);

	void GiveMoveTo(FVector Location, bool bClearAllActivities = false);

protected:
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Enemy Killed")
	void OnKnownEnemyKilled_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Enemy Incapacitated")
	void OnKnownEnemyIncapacitated_Blueprint(AReadyOrNotCharacter* IncapacitatedCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Enemy Take Damage")
	void OnKnownEnemyTakeDamage_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Enemy Stunned")
	void OnKnownEnemyStunned_Blueprint(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Friendly Killed")
	void OnKnownFriendlyKilled_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Friendly Incapacitated")
	void OnKnownFriendlyIncapacitated_Blueprint(AReadyOrNotCharacter* IncapacitatedCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Friendly Take Damage")
	void OnKnownFriendlyTakeDamage_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Friendly Stunned")
	void OnKnownFriendlyStunned_Blueprint(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Neutral Killed")
	void OnKnownNeutralKilled_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Neutral Incapacitated")
	void OnKnownNeutralIncapacitated_Blueprint(AReadyOrNotCharacter* IncapacitatedCharacter);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Neutal Take Damage")
	void OnKnownNeutralTakeDamage_Blueprint(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Known Neutral Stunned")
	void OnKnownNeutralStunned_Blueprint(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	uint8 bDisableSensePerception : 1;
	
	//The tags of the stimulus exposed too so we can track if an AI heard a gunshot, footstep etc
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<FName, FExposedToNoise> ExposedToStimulusTags;

	void NextActivityOnRoute();
	void RestartActivityRouteCollection();
	FTimerHandle TH_WorldBuildingRouteCooldown;
	uint8 bPendingNextActivityRoute : 1;

	FAIActionData* BestAction = nullptr;
	FAIActionData* BestUnalertAction = nullptr;
	FAIActionData* BestSuspiciousAction = nullptr;
	FAIActionData* BestCombatMoveAction = nullptr;
	FAIActionData* BestContinuousAction = nullptr;

	void DetermineBestInterruptAction(TArray<FAIActionDataContainer>& InActions, FAIActionData*& OutBestAction);

	bool IsPerformingAction(FAIActionData* Action) const;
	bool IsPerformingCustomAction(FName BlueprintTag) const;
	
	void ForceEndAction(FAIActionData*& InAction);
	void ForceEndAllActions();

	UFUNCTION(BlueprintPure)
	bool IsLastAlive() const;
	
	TArray<FName> AggressiveTags;
	TArray<FName> InvestigativeTags;
	
	UFUNCTION(BlueprintCallable)
	void AddExposedToStimulusTag(const FName& Tag, FVector StimulusLocation, bool bFriendly, AReadyOrNotCharacter* StimulusInstigator = nullptr, float ExpiryTime = 10.0f);
	
	UFUNCTION(BlueprintPure)
	bool HasBeenExposedToAggressiveNoise(float SinceSeconds = 0.0f, float MaxDistance = 0.0f, UPARAM(meta = (Bitmask, BitmaskEnum = EAITargetType)) int32 TargetTypeMask = 0) const;
	
	UFUNCTION(BlueprintPure)
	bool HasBeenExposedToAggressiveNoise_Tag(FName& OutTag, float SinceSeconds = 0.0f, float MaxDistance = 0.0f, UPARAM(meta = (Bitmask, BitmaskEnum = EAITargetType)) int32 TargetTypeMask = 0) const;
	
	UFUNCTION(BlueprintPure)
	bool HasBeenExposedToAnyNoise(float SinceSeconds = 0.0f, float MaxDistance = 0.0f, UPARAM(meta = (Bitmask, BitmaskEnum = EAITargetType)) int32 TargetTypeMask = 0) const;
	
	UFUNCTION(BlueprintPure)
	bool HasBeenExposedToAnyNoise_Tag(FName& OutTag, float SinceSeconds = 0.0f, float MaxDistance = 0.0f, UPARAM(meta = (Bitmask, BitmaskEnum = EAITargetType)) int32 TargetTypeMask = 0) const;
	
	UFUNCTION(BlueprintPure)
	bool IsTagAggressiveNoise(const FName& Tag) const;
	UFUNCTION(BlueprintPure)
	bool IsTagInvestigativeNoise(const FName& Tag) const;

	UFUNCTION(BlueprintCallable)
	void AbortMove(bool bKeepVelocity = false);

	UFUNCTION(BlueprintPure, Category = "Team")
	ETeamType GetTeam() const;

	UFUNCTION(BlueprintCallable)
	void AbortCover();
	UFUNCTION(BlueprintCallable)
	void AbortCoverLandmark();

	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastExposedToAggressiveStimulus = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastExposedToAnyStimulus = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastExposedToSightStimulus = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastExposedToSoundStimulus = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 bEverHeardAggressiveStimulus : 1;
	
	UPROPERTY(BlueprintReadOnly)
	float UnalertTime = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float AlertTime = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float SuspiciousTime = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bHasEverHeardSwat = false;

	// if this is greater than 0.0 then the ai will be in the alerted state and should already have their gun up.
	//float AlertTime = 0.0f;
	
	FTimerHandle BeginInvestigating_Handle;
	FTimerHandle CallForBackup_Handle;

	UFUNCTION(BlueprintPure)
	bool IsMovingForRequests(TArray<FAIRequestID> Requests) const;
	
	UFUNCTION(BlueprintPure)
	bool IsMovingForRequest(int32 RequestID) const;
	
	void PickupItem(ABaseItem* Item, bool bEquipItem);

	UPROPERTY()
	UBaseCombatActivity* CombatActivity = nullptr;

	UFUNCTION(BlueprintPure)
	bool IsSWAT() const;
	UFUNCTION(BlueprintPure)
	bool IsCivilian() const;
	UFUNCTION(BlueprintPure)
	bool IsSuspect() const;

	UFUNCTION(BlueprintPure)
	bool IsMoving() const;
	UFUNCTION(BlueprintPure)
	bool IsActivelyMovingOnPath() const;
	
	UFUNCTION(BlueprintPure)
	bool DoesPathGoThroughDoor(ADoor*& Door) const;
	bool DoesPathGoThroughDoor(FNavPathSharedPtr NavPath, ADoor*& Door) const;
	
	// Allows SWAT, Suspects and Civilians to use the  world differently
	// ie. swat will not enter doors, suspects will
	UFUNCTION(BlueprintPure)
	TSubclassOf<UNavigationQueryFilter> GetNavQueryFilter() const;
	TSubclassOf<UNavigationQueryFilter> GetNavQueryFilter(ETeamType SpecificTeam) const;

	float TimeMoving = 0.0f;

	uint32 LastPathID;
	FAIRequestID LastRequestID;
	FAIMoveRequest LastMoveRequest;
	
	UPROPERTY()
	class UMoveToActivity* MoveToActivity = nullptr;
	
	UPROPERTY()
	class UMoveActivity* TeamMoveActivity = nullptr;

	UPROPERTY()
	class UMoveToActivity* PushMoveToActivity = nullptr;
	
	UPROPERTY()
	class UMoveToActivity* AvoidanceMoveToActivity = nullptr;
	
	UPROPERTY()
	class UMoveToExitActivity* MoveToExitActivity = nullptr;
	
	UPROPERTY()
	class UTargetNextCivilianActivity* TargetNextCivilianActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	ADoor* LastHeardDoorKick = nullptr;

	void ClearLastHeardDoorKick();
	
	FTimerHandle TH_LastHeardDoorKick;
};

template <class T>
T* ACyberneticController::GetActivity() const
{
	static_assert(TIsDerivedFrom<T, UBaseActivity>::Value, "T must be derived from UBaseActivity");
	
	if (Cast<T>(CurrentActivity))
	{
		return Cast<T>(CurrentActivity);
	}
	
	for (UBaseActivity* Activity : ActivityQueue)
	{
		if (Cast<T>(Activity))
		{
			return Cast<T>(Activity);
		}
	}
	
	return nullptr;
}

template <class T>
T* ACyberneticController::GetCurrentActivity() const
{
	static_assert(TIsDerivedFrom<T, UBaseActivity>::Value, "T must be derived from UBaseActivity");
	
	return Cast<T>(CurrentActivity);
}

template <class T>
T* ACyberneticController::GetCombatActivity() const
{
	static_assert(TIsDerivedFrom<T, UBaseCombatActivity>::Value, "T must be derived from UBaseCombatActivity");
	
	return Cast<T>(CombatActivity);
}
