// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapStatisticsSystem.generated.h"

enum class EAIAwarenessState : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogInternalAnalytics, Verbose, All);

UENUM(BlueprintType)
enum class EActorAnalyticsState : uint8
{
	AAS_None = 0,
	AAS_Wounded = 1,
	AAS_Dead = 2,
	AAS_Arrested = 3
};

UENUM(BlueprintType)
enum class ESuspectStateData : uint8
{
	SSD_NONE = 0,
	SSD_IS_TRACKING = 1 << 0,
	SSD_HAS_BEST_ACTION = 1 << 1,
	SSD_HAS_BEST_CONTINUOUS_ACTION = 1 << 2,
	SSD_HAS_BEST_COMBAT_MOVE_ACTION = 1 << 3
};
ENUM_CLASS_FLAGS(ESuspectStateData)

USTRUCT(BlueprintType)
struct FAnalyticsSuspectState
{
	
	GENERATED_USTRUCT_BODY()
	ESuspectStateData State;
	EAIAwarenessState AwarenessState;
	
	uint8 TrackingActorId;
	float Morale;

	FString BestAction;
	float BestActionScore;

	FString BestContinuousAction;
	float BestContinuousActionScore;

	FString BestCombatMoveAction;
	float BestCombatMoveActionScore;

};

USTRUCT(BlueprintType)
struct FAnalyticsStatus
{
	GENERATED_USTRUCT_BODY()
	uint8 ActorId;
	EActorAnalyticsState State;
	uint32 TickOffset;
	FVector Position;
	float Heading;
	ETeamType Team;

	bool bHasSuspectState;
	FAnalyticsSuspectState SuspectState;
};



UCLASS()
class READYORNOT_API AMapStatisticsSystem final : public AInfo
{
	static constexpr uint8 MAP_STATISTICS_PROTOCOL_VERSION = 2;
	const float TICK_RATE = 0.2f;
	const int PACKET_STATUS_SIZE = 1024;
	
	int32 BytesPerMinutePerActor = static_cast<int32>(sizeof(FAnalyticsStatus) * 1.0f / TICK_RATE);;
	
	GENERATED_BODY()

	AMapStatisticsSystem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;;
	
private:
	TArray<AReadyOrNotCharacter*> GetActors() const;
	
	static void NotifyNewActor(const FGuid InGameId, int8 InActorId, AActor* InActor);
	static void NotifyGameStart(const FGuid InGameId, FString InLevelName, FString InMode);
	static void NotifyGameData(const FGuid InGameId, uint32 PacketIndex, TArray<FAnalyticsStatus>& InStatuses, bool HasGameEnded = false);

public:
	UFUNCTION(BlueprintCallable)
	bool IsRecording() const { return bIsRecording; }

	UFUNCTION(BlueprintCallable)
	FGuid GetGameId() const { return GameId; }

	UFUNCTION(BlueprintCallable)
	FString GetRecordingStatus() const;
	
	UFUNCTION(BlueprintCallable)
	void StartRecording(const FString& InLevelName, const FString& InGameMode);
	
	UFUNCTION(BlueprintCallable)
	void EndLevel();
private:
	UPROPERTY()
	FGuid GameId;
	
	bool bIsRecording = false;
	bool bIsDisabled = false;
	int64 StartTick = 0;
	int8 CurrentActorId = 0;
	int32 PacketIndex = 0;

	UPROPERTY()
	TMap<FString, uint8> ActorIdMap;
		
	UPROPERTY()
	TArray<FAnalyticsStatus> Statuses;
};
