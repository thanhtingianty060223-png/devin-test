// Copyright Void Interactive, 2021

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Structs.h"
#include "Activities/ScanDoorActivity.h"
#include "Activities/Team/DoorInteractionActivity.h"
#include "Actors/Door.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "SWATManager.generated.h"

DECLARE_STATS_GROUP(TEXT("SWATManager"), STATGROUP_SWATManager, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EPriorityOfLife : uint8
{
	Hostages,
	Civilians,
	EmergencyPersonnel,
	Suspects,
	Evidence
};

USTRUCT()
struct FClearQueueInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ADoor*, AThreatAwarenessActor*> Data;

	const FRoom* ClearingRoom = nullptr;

	uint8 NumDoorways = 0;
	uint8 TotalDoors = 0;
};

UCLASS()
class READYORNOT_API USWATManager final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	USWATManager();

public:
	static USWATManager* Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	
	virtual TStatId GetStatId() const override;

	void SetupSwatManager();

	void OnWorldBeingPlay();

	TArray<AActor*> GetAvailableSecurables(bool bNoSecureCheck = false) const;

	FVector OriginalSpawnLocation = FVector::ZeroVector;

	float CreationTime = 0.0f;
	float TickInterval = 0.0f;
	float TimeSinceLastTick = 0.0f;

	//bool bAnyPathPointToLeaderNearDoor = false;
	bool bGivenInitialFallInCommand = false;
	//bool bWaitingOnPathTest = false;
	bool bForceSnakeFallIn = false;

	float GlobalYellDelay = 0.0f;

	float PrefixCooldown = 0.0f;

	TMap<FName, float> SpeechCooldownMap;
	
	float MaxWaitingReplyDelay = 45.0f;
	float WaitingReplyDelay = 45.0f;

	float TimeSincePlayerIssuedCommand = 0.0f;

	UPROPERTY()
	TArray<ASWATCharacter*> SwatAI;

	UPROPERTY()
	TArray<class ATrailerSWATCharacter*> SwatTrailers;

	UPROPERTY()
	ASWATCharacter* ClosestFallInSwat = nullptr;
	
	//UPROPERTY()
	//TMap<uint32, ASWATCharacter*> FallInSwat_Queries;
	
	UPROPERTY()
	TMap<ASWATCharacter*, float> FallInSwat_PathFound;

	UPROPERTY()
	TArray<ASWATCharacter*> FallInSwat;
	
	UPROPERTY()
	TArray<ASWATCharacter*> FallInSwat_FileA;
	
	UPROPERTY()
	TArray<ASWATCharacter*> FallInSwat_FileB;
	
	UPROPERTY()
	TArray<ASWATCharacter*> FallInSwat_Diamond;

	UPROPERTY()
	TMap<ETeamType, FClearQueueInfo> ClearingQueue;
	
	UPROPERTY()
	TMap<FIntVector, ASWATCharacter*> OccupiedLookAtPoints;

	UPROPERTY()
	TMap<AActor*, FString> CallOutQueue;
	
	UPROPERTY()
	TArray<AActor*> CallOutHistory;
	
	UPROPERTY()
	TMap<AActor*, ASWATCharacter*> ReportQueue;

	FORCEINLINE ASWATCharacter* IsLookAtPointOccupied(FIntVector Point) { if (ASWATCharacter** FoundSwat = OccupiedLookAtPoints.Find(Point)) return *FoundSwat; return nullptr; }

	// one for each team, Gold, Red, Blue
	TMap<ETeamType, FSharedBreachData, TFixedSetAllocator<3>> SharedBreachData = {};
	TMap<ETeamType, FSharedStackUpData, TFixedSetAllocator<3>> SharedStackUpData = {};
	TMap<ETeamType, FSharedFallInData, TFixedSetAllocator<3>> SharedFallInData = {};
	TMap<ETeamType, FSharedTeamData, TFixedSetAllocator<3>> SharedTeamData = {};

	TMap<ETeamType, FSharedTeamData*, TFixedSetAllocator<3>> CurrentSharedTeamData = {};
	
	TSubclassOf<ABaseGrenade> FlashbangClass = nullptr;
	TSubclassOf<ABaseGrenade> StingerClass = nullptr;
	TSubclassOf<ABaseGrenade> CSGasClass = nullptr;

	bool bGoldAutoClearing = false;
	bool bRedAutoClearing = false;
	bool bBlueAutoClearing = false;

	TArray<FRoom*> AutoClearingRooms;

	uint16 TrailerClearingIndex = 0;

	UFUNCTION()
	void OnTrailerSearchComplete(class UBaseActivity* Activity, ACyberneticController* Controller);

	void GiveTrailerSearchAndSecure(ATrailerSWATCharacter* Character);
	
	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* SquadLeader = nullptr;

	//void OnFallInPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);
	//void OnSwatSortAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);

	class ASWATCharacter* GetClosestSWATToActor(AActor* TestActor);
	class ASWATCharacter* GetClosestSWATToActorForTeam(AActor* TestActor, ETeamType Team);
	class ASWATCharacter* GetClosestSWATToActorForTeamWithItem(AActor* TestActor, ETeamType Team, EItemCategory ItemType);
	class ASWATCharacter* GetClosestSWATToLocation(FVector TestLocation);
	
	class ASWATCharacter* GetClosestSWATInSightToActor(AActor* TestActor);
	
	bool PlaySpeechWithSharedCooldown(FString SpeechRowName, class AReadyOrNotCharacter* Speaker, float Cooldown = 3.0f, FString OverrideSpeakerName = "");

	UPROPERTY(BlueprintReadOnly)
	TMap<ETeamType, FQueuedSwatCommand> QueuedSwatCommandMap;
	
	UPROPERTY(BlueprintReadOnly, Category = "SWAT Manager")
	ESwatCommand CurrentDefaultCommand = ESwatCommand::SC_FallIn;
	
	UPROPERTY(BlueprintReadOnly, Category = "SWAT Manager")
	ETeamType ActiveCommandTeam = ETeamType::TT_SQUAD;

	UFUNCTION(BlueprintPure)
	FORCEINLINE TArray<ASWATCharacter*> GetSwatTeam() const { return SwatAI; }
	
	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	ASWATCharacter* GetSwatCharacterAtSquadPosition(ESquadPosition InSquadPosition) const;

	UFUNCTION(BlueprintCallable)
	ESwatCommand GetQueuedSwatCommandForSquadPosition(ESquadPosition SquadPosition) const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE uint8 GetSWATCount() const { return SwatAI.Num(); }

	UFUNCTION()
	void OnActivityStarted(UBaseActivity* Activity, ACyberneticController* OwningController);

	UFUNCTION()
	void OnLeaderToggledNightvision(AReadyOrNotCharacter* Character, bool bOn);

	bool IsSWATValid(ASWATCharacter* SWATCharacter) const;

	void RespondToPlayerTeamKill(AReadyOrNotCharacter* InstigatorCharacter, bool bTeleportBehind = false);
	
	bool Internal_FindPath(float& OutDistance, FVector From, FVector To, float MaxPathLength);

	UFUNCTION()
	void OnSwatFinishedClearing(class UTeamBreachAndClearActivity* BreachAndClearActivity, bool bAuto);
	UFUNCTION()
	void OnSwatFinishedRoomSearch(class USearchAndSecureActivity* SearchAndSecureActivity, ADoor* BreachedDoor);

	void AdvanceClearingQueue(ETeamType CommandTeam);

	UFUNCTION(BlueprintPure)
	FVector GetAverageSwatLocation() const;

	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	FORCEINLINE AReadyOrNotCharacter* GetSquadLeader() const { return SquadLeader; }

	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	TArray<ASWATCharacter*> GetSWATSortedByDistanceToLocation(FVector Location, ETeamType FilterTeam = ETeamType::TT_NONE, ADoor* StackUpDoor = nullptr, bool bAscendingOrder = true);
	
	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	TArray<ASWATCharacter*> GetSWATSortedByDistanceToLocationV2(FVector Location, TArray<ASWATCharacter*> ExcludedSwat, ETeamType FilterTeam = ETeamType::TT_NONE, bool bAscendingOrder = true);

	//UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	//bool GetSWATSortedByPathDistanceToLocation_Async(UObject* Context, TDelegate<void(const TArray<ASWATCharacter*>&)> Delegate, FVector Location, ETeamType FilterTeam = ETeamType::TT_NONE);
	
	TArray<ASWATCharacter*> GetSWATSortedByPathDistanceToLocation(FVector Location, TArray<ASWATCharacter*> ExcludedSwat, ETeamType FilterTeam = ETeamType::TT_NONE, bool bAscendingOrder = true);

	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	bool IsCharacterKnownEnemy(AReadyOrNotCharacter* InCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	bool IsSWATTeamDead(ETeamType Team = ETeamType::TT_SQUAD) const;
	
	UFUNCTION(BlueprintPure, Category = "SWAT Manager")
	bool IsSWATTeamHoldingPosition(ETeamType Team = ETeamType::TT_SQUAD) const;
	
	UFUNCTION(BlueprintCallable)
	void PlaySwatCommandVoiceLine(FString Voiceline, FString OverrideSpearkerName = "", bool bTeamPrefix = true);
	FTimerHandle TH_Prefix;
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveDeployNonLethalItemAtTargetCommand(AReadyOrNotCharacter* Target, ETeamType TeamType, EItemCategory Item);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveBreachAndClearCommand(ADoor* Door, EDoorBreachType DoorBreachType, ETeamType TeamType, FVector CommandLocation, TSubclassOf<ABaseItem> DoorBreachItemClass = nullptr, TSubclassOf<ABaseItem> DoorUseItemClass = nullptr, bool bWithLeader = false, bool bWithLeaderItem = false, bool bAutoClear = false, bool bLastAutoClear = false, EStackUpStyle CustomStackUpStyle = EStackUpStyle::Auto);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveMoveCommand(ETeamType TeamType, FVector CommandLocation);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveDeployGrenadeAtLocation(ETeamType TeamType, FVector CommandLocation, TSubclassOf<ABaseGrenade> Grenade);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveDropChemlightAtLocation(ETeamType TeamType, FVector CommandLocation);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveDeployShield(ETeamType TeamType);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveStackUpCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal, bool bCheckDoor = false, EStackUpStyle StackUpStyle = EStackUpStyle::Auto);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveFallInCommand(ETeamType TeamType, EFallInPattern FallInPattern = EFallInPattern::Snake);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveCheckForContactsCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveCheckForTrapsCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveDisarmTrapOnDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveWedgeDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveCloseDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveOpenDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveScanDoorCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, EDoorScanMethod ScanMethod);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveSearchAndSecureCommand(ETeamType TeamType, FVector CommandLocation, bool bOnlyCurrentRoom = false);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveSearchAndSecureCommand_Individual(AActor* Target, FVector CommandLocation, bool bOnlyCurrentRoom = false);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveHoldCommand(ETeamType TeamType);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void RemoveHoldCommand(ETeamType TeamType);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GivePickLockCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveRemoveWedgeCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation, FVector CommandNormal);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveRestrainCommand(AActor* Target, ETeamType TeamType, FVector CommandLocation);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveCollectEvidenceCommand(AActor* Target, ETeamType TeamType);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveReportTargetCommand(AActor* Target, ETeamType TeamType);

	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
    void GiveDisarmStandaloneTrapCommand(AActor* Target, ETeamType TeamType);
	
	UFUNCTION(BlueprintCallable, Category = "SWAT Manager Commands")
	void GiveCoverAreaCommand(ETeamType TeamType, FVector CommandLocation);

	UFUNCTION(BlueprintPure, Category = "SWAT Manager Commands")
	bool CanGiveActivityToSWAT(ASWATCharacter* Swat, ETeamType Team) const;

	template<typename T, class ItemClass = ABaseItem>
	ASWATCharacter* FindSwatWithItemForDoorInteraction(ADoor* Door, ETeamType TeamType, const FString& NoItemResponse = "");
	
	template<typename T>
	ASWATCharacter* FindSwatForDoorInteraction(ADoor* Door, ETeamType TeamType);

	ASWATCharacter* GetSwatWithItem(ETeamType TeamType, TSubclassOf<ABaseItem> Item, TArray<ASWATCharacter*> SortedArray);
	ASWATCharacter* GetSwatWithItem(ETeamType TeamType, TSubclassOf<ABaseItem> Item);
	ASWATCharacter* GetSwatWithItemType(ETeamType TeamType, EItemCategory Item) const;
	
protected:
	void BindSWATActivityResponseVoiceLine(class AReadyOrNotCharacter* RespondingAI, const FString& VoiceLine, class UBaseActivity* Activity);
};

template <typename T, class ItemClass>
ASWATCharacter* USWATManager::FindSwatWithItemForDoorInteraction(ADoor* Door, ETeamType TeamType, const FString& NoItemResponse)
{
	static_assert(TIsDerivedFrom<T, UDoorInteractionActivity>::Value, "T must be derived from UDoorInteractionActivity");
	static_assert(TIsDerivedFrom<ItemClass, ABaseItem>::Value, "ItemClass must be derived from ABaseItem");

	if (!Door)
		return nullptr;

	if (GetSWATCount() == 0)
		return nullptr;

	if (!GetSwatWithItem(TeamType, ItemClass::StaticClass(), GetSwatTeam()))
	{
		GetSwatTeam()[0]->PlayRawVO(NoItemResponse);
		return nullptr;
	}

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(Door->GetDoorMidLocation(), TeamType, Door);
	
	// If someone is already using this door, don't give another activity
	if (UActivityManager::AnyAIHasActivity<UDoorInteractionActivity>([&](const UDoorInteractionActivity* Activity)
	{
		return Activity->Door == Door;
	}))
	{
		return nullptr;
	}
	
	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (const UBaseActivity* CurrentActivity = Swat->GetCyberneticsController()->GetCurrentActivity())
		{
			if (!CurrentActivity->CanOverrideActivity())
				return true;
		}
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<T>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return nullptr;

	return GetSwatWithItem(TeamType, ItemClass::StaticClass(), SortedSWAT);
}

template <typename T>
ASWATCharacter* USWATManager::FindSwatForDoorInteraction(ADoor* Door, ETeamType TeamType)
{
	static_assert(TIsDerivedFrom<T, UDoorInteractionActivity>::Value, "T must be derived from UDoorInteractionActivity");

	if (!Door)
		return nullptr;

	if (GetSWATCount() == 0)
		return nullptr;

	// If someone is already using this door, don't give another activity
	if (UActivityManager::AnyAIHasActivity<UDoorInteractionActivity>([&](const UDoorInteractionActivity* Activity)
	{
		return Activity->Door == Door;
	}))
	{
		return nullptr;
	}

	TArray<ASWATCharacter*> SortedSWAT = GetSWATSortedByDistanceToLocation(Door->GetDoorMidLocation() + Door->GetActorRightVector() * 60.0f, TeamType);
	
	SortedSWAT.RemoveAll([](const ASWATCharacter* Swat)
	{
		if (const UBaseActivity* CurrentActivity = Swat->GetCyberneticsController()->GetCurrentActivity())
		{
			if (!CurrentActivity->CanOverrideActivity())
				return true;
		}
		
		return Swat->GetCyberneticsController()->GetCurrentActivity<T>() != nullptr;
	});

	if (SortedSWAT.Num() == 0)
		return nullptr;

	return SortedSWAT[0];
}
