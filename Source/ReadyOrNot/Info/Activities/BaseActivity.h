// Copyright Void Interactive, 2021

#pragma once

#include "Enums.h"
#include "AI/Navigation/NavigationTypes.h"
#include "AIModule/Classes/AITypes.h"
#include "AIModule/Classes/Navigation/PathFollowingComponent.h"
#include "ActivityFiniteStateMachine.h"
#include "Info/ActivityManager.h"
#include "Interfaces/ReceiveAISenseUpdates.h"
#include "BaseActivity.generated.h"

#if !UE_BUILD_SHIPPING
#define ACTIVITY_FAILED(...) ActivityFailed(__VA_ARGS__)
#else
#define ACTIVITY_FAILED(...) ActivityFailed()
#endif

extern TAutoConsoleVariable<int32> CVarRonToggleActivityDebugLines;
extern TAutoConsoleVariable<int32> CVarUseRawPath;

UENUM(BlueprintType)
enum class EActivityStatus : uint8
{
	Uninitialized,
	Initialized,
	Started,
	Paused,
	Complete
};

DECLARE_STATS_GROUP(TEXT("BaseActivity"), STATGROUP_BaseActivity, STATCAT_Advanced);

/**
 * Base class for all activities that can be performed by AI characters
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, Transient)
class READYORNOT_API UBaseActivity : public UObject, public IReceiveAISenseUpdates
{
	GENERATED_BODY()
	
public:
	UBaseActivity();

	friend class ACyberneticController;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStartActivity, UBaseActivity*, Activity, ACyberneticController*, Controller);
	UPROPERTY(BlueprintAssignable)
	FOnStartActivity OnStartActivity;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPerformActivity, UBaseActivity*, Activity,  ACyberneticController*, Controller, float, DeltaTime);
	UPROPERTY(BlueprintAssignable)
	FOnPerformActivity OnPerformActivity;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFinishActivity, UBaseActivity*, Activity, ACyberneticController*, Controller);
	UPROPERTY(BlueprintAssignable)
	FOnFinishActivity OnFinishActivity;
	UPROPERTY(BlueprintAssignable)
	FOnFinishActivity OnFinishActivity_NoOwner;

	DECLARE_MULTICAST_DELEGATE(FOnRespondToActivity);
	FOnRespondToActivity OnRespondToActivityStart;
	
	// Start Activity
	bool InitActivity(AAIController* Owner);
	virtual void StartActivity(AAIController* Owner);
	virtual void ResumeActivity();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Start Activity")
	void StartActivity_Blueprint(AAIController* Owner);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Finished Activity")
	void FinishedActivity_Blueprint(bool bSuccess);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Perform Activity")
	void PerformActivity_Blueprint(float DeltaTime);

	// Tick Activity
	virtual void PerformActivity(float DeltaTime);
	
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime);
	#endif
	
	// End Activity
	virtual void FinishedActivity(bool bSuccess);
	
	// If no owning pawn, this will be called instead of FinishedActivity. It is not safe to call GetCharacter() in this function
	virtual void FinishedActivity_NoOwner(bool bSuccess);
	
	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const { return false; }

	virtual void ActivityOverriden(UBaseActivity* OverridingActivity);

	virtual void OnClearedFromQueue();

	virtual void OnPathMoveCompleted(AAIController* Controller, FAIRequestID RequestID, const FPathFollowingResult& Result);
	virtual void OnPathMovePaused(AAIController* Controller, FAIRequestID RequestID);
	virtual void OnPathMoveResumed(AAIController* Controller);
	
	UFUNCTION(BlueprintNativeEvent)
			FName GetMoveStyleOverride() const;
	virtual FName GetMoveStyleOverride_Implementation() const;

	UFUNCTION()
	virtual void OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	
	UFUNCTION()
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	
	virtual bool CanTick() const;
	virtual bool CanFinishActivity() const { return true; }
	virtual bool CanShoot() const { return false; }
	virtual bool CanReload() const { return true; }
	virtual bool OverrideFireAngleThreshold(float& Threshold) const { return false; }

	virtual bool TryFinishActivityOnMoveComplete() { return true; }

	UFUNCTION(BlueprintCallable)
	virtual void ResetData();
	
	virtual float GetDestinationTolerance() const;

	// Overrides the focal point in the targeting component
	virtual bool OverrideFocalPoint(FVector& FocalPoint);

	UFUNCTION(BlueprintImplementableEvent)
	bool OverrideFocalPoint_Blueprint(FVector& FocalPoint);
	
	UFUNCTION(BlueprintPure)
	FString GetActiveStateName() const;
	UFUNCTION(BlueprintPure)
	float GetActiveStateUptime() const;
	UFUNCTION(BlueprintPure)
	int32 GetActiveStateID() const;
	
	virtual void GatherDebugString(FString& OutString);

	FORCEINLINE float GetElapsedActivityTime() const { return ElapsedActivityTime; }
	
	UFUNCTION(BlueprintCallable, Category = "Location")
	void SetLocation(const FVector& NewLocation, bool bRequestMove = false, FVector CustomExtent = FVector::ZeroVector);

	UFUNCTION(BlueprintPure)
	FORCEINLINE ACyberneticController* GetOwningController() const { return OwningController; }
	
	template<typename T>
	FORCEINLINE T* GetOwningController() const { return Cast<T>(OwningController); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE ACyberneticCharacter* GetCharacter() const { return OwningCharacter; }

	template<typename T>
	FORCEINLINE T* GetCharacter() const { return Cast<T>(OwningCharacter); }

	UFUNCTION(BlueprintPure)
	virtual bool ShouldForceStrafe() const { return false; }
	UFUNCTION(BlueprintPure)
	virtual bool ShouldForceNoStrafe() const { return false; }
	
	UFUNCTION(BlueprintPure)
	virtual bool CanBePushed() const { return true; }
	UFUNCTION(BlueprintPure)
	virtual bool CanOverrideActivity() const { return true; }

	virtual bool GetLeanOverride(float& LeanOverride) const { return false; }
	virtual bool GetLowReadyOverride(bool& bLowReady) const { return false; }

	virtual bool ShouldDisableMoveRequest() const { return false; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasStartedActivity() const { return ActivityStatus == EActivityStatus::Started; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsActivityPaused() const { return ActivityStatus == EActivityStatus::Paused; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsActivityComplete() const { return ActivityStatus == EActivityStatus::Complete; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsActivityInitialized() const { return ActivityStatus == EActivityStatus::Initialized; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE EActivityStatus GetActivityStatus() const { return ActivityStatus; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsProgressActivity() const { return bIsProgressActivity; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsNoMoveActivity() const { return bNoMoveActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE FText GetProgressState() const { return ProgressState; }
	
	UFUNCTION(BlueprintPure)
	bool HasReachedLocation(float Tolerance = 10.0f) const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetLocation() const { return Location; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity Settings|General")
	FText ActivityName = FText::FromStringTable("SwatCommandTable", "Activity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	float MaxActivityTime = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	float ActivityStartDelay = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bNoResetDataOnFinish : 1;

	virtual bool CanBeCleared() { return true; }

	virtual UWorld* GetWorld() const override final;
	
	// small buffer to make sure the path fails X times, navmesh could be updating
	int32 PathFailedCount = 0;
	
	UFUNCTION(BlueprintCallable)
	void EquipWeapon();

	void RequestMoveAsync(bool bAllowPartialPath = false, bool bProjectDestination = true);

protected:
	void BroadcastActivityStart();
	
	virtual void PlayAISpeech(FString VoiceLine, bool bHasSharedCooldown = true, float cooldown = 3.0f);

	virtual void AbortMove(bool bAbortAll = false, const FPathFollowingResultFlags::Type& Result = FPathFollowingResultFlags::ForcedScript);
	virtual void AbortLastMove(const FPathFollowingResultFlags::Type& Result = FPathFollowingResultFlags::ForcedScript);
	
	virtual void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	virtual void OnPathFound_Internal(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);

	virtual void AbortActivityOnPathNotFound();

	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() { return nullptr; }

	virtual bool CanBeOverridenBy(UBaseActivity* InOverridingActivity) { return true; }

	#if !UE_BUILD_SHIPPING
	void ActivityFailed(const FString& InActivityFailureReason, bool bWarn = false);
	#else
	void ActivityFailed();
	#endif

	UFUNCTION(BlueprintCallable)
	void EquipItem(EItemCategory InItemCategory);
	UFUNCTION(BlueprintCallable)
	void EquipItemOfClass(TSubclassOf<ABaseItem> InItemClass);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	float MoveAcceptanceRadius = 10.0f;
	float OriginalMoveAcceptanceRadius = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General", meta = (EditCondition = "!bGlobalCooldownRandomRange"))
	float GlobalCooldown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bGlobalCooldownRandomRange : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General", meta = (EditCondition = "!bGlobalCooldownRandomRange"))
	FVector2D GlobalCooldownRange = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bIsProgressActivity : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bFinishActivityWhenOverriden : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortMoveWhenActivityFinished : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortMoveWhenActivityOverriden : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortActivityIfCannotReachLocation : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortActivityIfProjectedLocationFailed : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortIfTrackingEnemy : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bPauseIfTrackingEnemy : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bPauseIfRecentlyInCombat : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	float RecentCombatTimeThreshold = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAllowWhileArrested : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bNoMoveActivity : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bResetStateMachineWhenActivityResumed : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAllowPartialMove : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAllowRePathOnInvalidation : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortIfNotMovingForAWhile : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAlwaysRequestMove : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity Settings|General")
	uint8 bAbortIfSurrendered : 1;

protected:
	UPROPERTY(BlueprintReadOnly)
	float ElapsedActivityTime = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float TimeNotMoving = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FText ProgressState = FText::FromStringTable("SwatCommandTable", "InProgress");
	
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastAsyncMove = 999.0f;
	
	UPROPERTY(BlueprintReadOnly)
	FVector LastRequestedLocation = FVector::ZeroVector;
	
	FAIRequestID LastRequestedMoveId;

	TMap<FIntVector, FAIRequestID> MoveRequestID;
	
	uint32 LastAsyncQueryId = 0;
	
	UPROPERTY(BlueprintReadOnly)
	EActivityStatus ActivityStatus = EActivityStatus::Uninitialized;

	int16 SpeechLimit = 1;
	int16 CurrentSpeechIncrement = 0;
	uint8 bHasLimitSpeechLimit : 1;

	UPROPERTY(BlueprintReadOnly)
	uint8 bSearchingPath : 1;
	UPROPERTY(BlueprintReadOnly)
	uint8 bLastRequestedLocationWaitingForRepath : 1;

	FAIRequestID LastPauseRequestID;

	FNavPathSharedPtr LastNavPath;
	UPROPERTY(BlueprintReadOnly)
	float LastPathLength = 0.0f;
	
	UPROPERTY()
	class ACyberneticController* OwningController = nullptr;
	UPROPERTY()
	class ACyberneticCharacter* OwningCharacter = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient)
	UActivityFiniteStateMachine* ActivityStateMachine = nullptr;

	FPathFindingResult FindPath(const FVector& InLocation);
	uint32 FindPathAsync(const FVector& InLocation);
	
	virtual void OnAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	void OnAsyncPathFound_Internal(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	
	virtual void RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath);

	virtual float GetMoveAcceptanceRadiusOverride() const { return MoveAcceptanceRadius; }

	#if !UE_BUILD_SHIPPING
	FString AddDebugString(FString Category, FString Text);
	#endif
};

