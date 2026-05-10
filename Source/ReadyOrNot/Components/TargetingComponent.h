// Void Interactive, 2020

#pragma once

#include "Actors/ThreatAwarenessActor.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("TargetingComponent"), STATGROUP_TargetingComponent, STATCAT_Advanced);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent), HideCategories = ("Activation", "Cooking", "AssetUserData", "Collision"))
class READYORNOT_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ACyberneticController;

public:	
	UTargetingComponent();

	UFUNCTION(BlueprintPure)
	FORCEINLINE AReadyOrNotCharacter* GetLastKnownEnemy() const { return LastKnownEnemy; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE AReadyOrNotCharacter* GetTrackedTarget() const { return TrackedTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE AReadyOrNotCharacter* GetLastTrackedTarget() const { return LastTrackedTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE AThreatAwarenessActor* GetNearestThreat() const { return NearestThreat; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE AThreatAwarenessActor* GetNearestExtremeThreat() const { return NearestExtremeThreat; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetLastHeardAggressiveNoiseLocation() const { return LastHeardAggressiveNoise.StimulusLocation; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetLastHeardNoiseLocation() const { return LastHeardNoiseStimulus.StimulusLocation; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetLastKnownEnemyPosition() const { return LastKnownTargetPosition; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetLastSeenKnownEnemyFrom() const { return LastSeenKnownTargetFrom; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE ETargetingCompTracking GetTrackingType() const { return TrackingType; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetLastKnownTrackingTimeConfig() const { return LastKnownTrackingTime; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastSeenEnemy() const { return TimeSinceLastSeenEnemy; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastSeenNeutral() const { return TimeSinceLastSeenNeutral; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeSinceLastSeenFriendly() const { return TimeSinceLastSeenFriendly; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeTrackingTarget() const { return TimeTrackingTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetRequiredTrackingTime() const { return RequiredTimeTrackingTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetTimeTrackingHead() const { return TimeTrackingHead; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE int32 GetKnownFriendlyCount() const { return KnownFriendlies.Num(); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE FName GetTargetedBone() const { return TargetedBone; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasLineOfSightToTrackedTarget() const { return bHasLOSToTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool CanSeeTrackedTarget() const { return bCanSeeTarget; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasLineOfSightToLastTrackedTarget() const { return bHasLOSToLastTarget; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool CanSeeLastTrackedTarget() const { return bCanSeeLastTarget; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasLineOfSightToLastKnownTargetPosition() const { return bHasLOSToLastKnownTargetPosition; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool AnyAITrackingMe() const { return AITrackingMe.Num() > 0; }
	
	void SetIgnoredTrackingDirection(const FVector& NewLocation) { ThreatTrackingIgnoredDirection = NewLocation; }
	void SetAllowedTrackingDoors(const TArray<ADoor*>& NewDoors) { AllowedTrackingDoors = NewDoors; }
	
	void ClearCustomFocusPoints();

	UFUNCTION(BlueprintCallable)
	void SetLastTrackedTarget(AReadyOrNotCharacter* Target) { LastTrackedTarget = Target; }
	
	UFUNCTION(BlueprintPure)
	bool CanCharacterBeSeen(AReadyOrNotCharacter* InCharacter) const;
	UFUNCTION(BlueprintPure)
	bool CanActorBeSeen(AActor* InActor) const;
	UFUNCTION(BlueprintPure)
	bool IsTrackingMontagePosition() const;
	
	UFUNCTION(BlueprintPure)
	bool HasSeenCharacterFor(AReadyOrNotCharacter* InCharacter, float Seconds) const;

	void SetMontageFocalPoint(UAnimMontage* Montage, FVector FocalPoint);
	void ClearMontageFocalPoint() { MontageFocalAnim = nullptr; MontageFocalPoint = FVector::ZeroVector; }

	UFUNCTION(BlueprintCallable)
	void AddCharacterToSeenMap(AReadyOrNotCharacter* InCharacter);
	
	UFUNCTION(BlueprintCallable)
	void AddKnownEnemy(AReadyOrNotCharacter* Enemy, bool bForce = false);
	UFUNCTION(BlueprintCallable)
	void AddKnownFriendly(AReadyOrNotCharacter* Friendly);
	UFUNCTION(BlueprintCallable)
	void AddKnownNeutral(AReadyOrNotCharacter* Neutral);
	
	UFUNCTION(BlueprintPure)
	bool IsTrackedByKnownFriendly(AReadyOrNotCharacter* Target);
	
	UFUNCTION(BlueprintPure)
	int32 GetVisibleKnownFriendlies();
	
	UFUNCTION(BlueprintPure)
	bool IsTrackedInKnownEnemies(AReadyOrNotCharacter* PlayerCharacter) const;
	UFUNCTION(BlueprintPure)
	bool IsTrackedInKnownFriendlies(AReadyOrNotCharacter* PlayerCharacter) const;
	UFUNCTION(BlueprintPure)
	bool IsTrackedInKnownNeutrals(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure)
	bool IsLookingAtFocalPoint(float Tolerance);

	void GatherDebugText(FString& OutText);

	void SetLastHeardNoiseLocation(FExposedToNoise Noise);

	UFUNCTION(BlueprintCallable)
	FExposedToNoise GetLastNoiseByTag(FName Tag);
	
	UFUNCTION(BlueprintCallable)
	TArray<FName> GetLastNoisesTags();

	// updated when we use a door or something so the swat don't look behind them at this location
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector ThreatTrackingIgnoredDirection = FVector::ZeroVector;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<ADoor*> AllowedTrackingDoors;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Focal Point")
	UAnimMontage* MontageFocalAnim = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Focal Point")
	FVector MontageFocalPoint = FVector::ZeroVector;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Focal Point")
	FVector CustomFocusLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Focal Point")
	AActor* CustomFocusActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Focal Point")
	class AInterestOverrideZone* CurrentInterestZone = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector HeadTrackingLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat")
	float Threat = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat")
	float Tension = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SWAT|Threat")
	AThreatAwarenessActor* ThreatLookPoint = nullptr;
	
	float CalculatedThreat = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FExposedToNoise LastHeardAggressiveNoise;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<FName, FExposedToNoise> HeardNoises;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FExposedToNoise LastHeardNoiseStimulus;
	
	TArray<FVector> TargetLocationHistory;
	float TimeUntilNextLocationHistoryUpdate = 0.0f;

	UFUNCTION()
	void OnTrackedTargetKilled(AReadyOrNotCharacter* Instigator, AReadyOrNotCharacter* KilledCharacter);
	
	UFUNCTION()
	void OnTrackedTargetIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	
	UFUNCTION()
	void OnTrackedTargetExitedSurrender(ACyberneticCharacter* Character, ESurrenderExitType ExitType);

	UFUNCTION()
	void OnTrackedTargetStartedReloading(APlayerCharacter* Character);

	// Called from ACyberneticController
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	#if !UE_BUILD_SHIPPING
	virtual void TickComponent_Debug(float DeltaTime);
	#endif

protected:
	virtual void BeginPlay() override;

	float CalculateExposure(AReadyOrNotCharacter* Observer, AReadyOrNotCharacter* Target) const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<AReadyOrNotCharacter*, float> CharactersSeen;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	ETargetingCompTracking TrackingType = ETargetingCompTracking::TCT_None;

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<AReadyOrNotCharacter*> KnownFriendlies;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<AReadyOrNotCharacter*> KnownNeutrals;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (TitleProperty = "Target"))
	TArray<AReadyOrNotCharacter*> KnownEnemies;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector LastKnownTargetPosition = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector LastKnownTargetPositionInLOS = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FVector LastSeenKnownTargetFrom = FVector::ZeroVector;
	
	const FLookAtPoint* CurrentThreatLookAtPoint = nullptr;
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastSeenTarget = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastSeenEnemy = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastSeenNeutral = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastSeenFriendly = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float PreviousTimeNotSeenTarget = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float PreviousTimeNotSeenEnemy = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float PreviousTimeNotSeenFriendly = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float PreviousTimeNotSeenNeutral = FLT_MAX;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeTrackingTarget = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeTrackingEnemy = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeTrackingNeutral = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeTrackingFriendly = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastFriendlyDeath = 86400.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastFriendlyTookDamage = 86400.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastFriendlyStunned = 86400.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastEnemyDeath = 86400.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastEnemyTookDamage = 86400.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastEnemyStunned = 86400.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float EngagementTimeUntilReachedLastBoneZone = 4.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float BoneRetargetingRate = 1.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeSinceLastBoneRetarget = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float LastKnownTrackingTime = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<AReadyOrNotCharacter*> AITrackingMe;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bHasLOSToTarget : 1;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bCanSeeTarget : 1;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bHasLOSToLastTarget : 1;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bCanSeeLastTarget : 1;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bHasLOSToLastKnownTargetPosition : 1;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bone Targeting")
	uint8 CurrentBoneTargetZoneIndex = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bone Targeting")
	uint8 PreviousBoneTargetZoneIndex = 0;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bone Targeting")
	FName TargetedBone = NAME_None;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bone Targeting")
	TArray<FName> BonesToTarget;
	
	TArray<TArray<FName>> BoneTargetZones;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exposure")
	class ABaseGrenade* SmokeGrenadeBetweenTarget = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exposure")
	float ExposureFromEnemy = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exposure")
	float EnemyExposureFromUs = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float RequiredTimeTrackingTarget = 1.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float TimeTrackingHead = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	AReadyOrNotCharacter* TrackedTarget = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	AReadyOrNotCharacter* LastTrackedTarget = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	AReadyOrNotCharacter* LastKnownEnemy = nullptr;

	ACyberneticCharacter* GetOwningCharacter() const;
	ACyberneticController* GetOwningController() const;
	
	bool TrackPointsOfInterest();
	bool TrackSwatPointsOfInterest();
	bool TrackSuspectPointsOfInterest();
	bool TrackCivilianPointsOfInterest();
	bool TrackMontagePosition();
	bool TrackScriptedFireAtActor();
	bool TrackScriptedLookAtActor();
	bool TrackCombatMoveLocation();
	bool TrackVisibleNeutrals();
	bool TrackMoveVector();
	bool TrackActivityRelatedPoints();
	bool TrackStairThreatAwarenessActors();
	bool TrackThreatAwarenessActors();
	bool TrackThreatAwarenessActors_V2();
	bool TrackNearestDoor();
	bool TrackOverrideInterests();
	
	bool ShouldTrackTarget();
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stimulus")
	FVector LastMoveVectorFocalPoint = FVector::ZeroVector;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stimulus")
	ADoor* LastTrackedDoor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stimulus")
	float TimeSinceLastHeardNoiseStimulus = 0.0f;

	// The latest noise as retrieved by the async path finding 
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stimulus")
	FVector LatestNoiseLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	AThreatAwarenessActor* LastThreatAwarenessActor = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	bool bSearchingPathAwareness = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	float TimeSinceGotLastThreatAwarenessActor = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	float TimeUntilFinishedCheckingThreat = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	FIntVector LastLookAtPoint = FIntVector::ZeroValue;

	// The last noise that we searched async on (so we don't constantly keep looking up the noise)
	bool TrackNoiseRelatedEvents();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	TMap<EPathedAwareness, FPathAwarenessInfo> LatestPathedAwareness;
	
	// the last location we tested (not the final noise path)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	TMap<EPathedAwareness, FVector> LastSearchedPathedAwareness;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
    TMap<EPathedAwareness, float> PathAwarenessSearchTimeout;
	UPROPERTY(VisibleInstanceOnly, Category = "Threat Awareness")
	TMap<uint32, EPathedAwareness> PathedAwarenessQueryType;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	TArray<AReadyOrNotCharacter*> AllKnownCharacters;
	
	void FindAwarenessPath(FVector StimulusLocation, EPathedAwareness AwarenessType);
	void OnAwarenessPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	
public:
	void CalculatePathedAwareness(EPathedAwareness AwarenessType, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	
protected:
	void SetFocusActor(AActor* Actor);

	float TimeSinceLastThreatSearch = 1.0f;
	
	// Track and update this in a singular location so we can access it and make it async
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	AThreatAwarenessActor* NearestThreat = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Threat Awareness")
	AThreatAwarenessActor* NearestExtremeThreat = nullptr; 

	FTimerHandle TH_FindNearestThreatAwareness;
	FTimerHandle TH_RemoveMontageFocalPoint;
	FTimerHandle TH_NoisePathAwarenessExpiry;

	FTraceHandle TrackedTargetLOSTraceHandle;
	FTraceHandle LastTrackedTargetLOSTraceHandle;
	FTraceHandle LastKnownTargetLOSTraceHandle;
	
	FTraceHandle ExposureTraceHandles_1[6];
	FTraceHandle ExposureTraceHandles_2[6];
};
