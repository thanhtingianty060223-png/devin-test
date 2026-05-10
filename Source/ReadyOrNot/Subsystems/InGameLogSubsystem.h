// Void Interactive, 2020

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Debug/Public/Debug.h"
#include "InGameLogSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FInGameLogMessage
{
	GENERATED_BODY()

	FInGameLogMessage() = default;

	
	FInGameLogMessage(const EDebugLogType& InLogSeverity, const FText& InLogMessage, const float InDelayBetweenWords = 0.08f, const float InDelayBetweenLetters = 0.0f, const float InTimeOnScreen = 5.0f)
	{
		LogSeverity = InLogSeverity;
		LogMessage = InLogMessage;
		DelayBetweenWords = InDelayBetweenWords;
		DelayBetweenLetters = InDelayBetweenLetters;
		TimeOnScreen = InTimeOnScreen;
	
		bAutoDetermineIcon = true;
		Icon = nullptr;
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	TEnumAsByte<EDebugLogType> LogSeverity = DL_Info;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	uint8 bAutoDetermineIcon : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data", meta = (EditCondition = "!bAutoDetermineIcon"))
	UTexture2D* Icon = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data", meta = (MultiLine = "true"))
	FText LogMessage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	float DelayBetweenWords = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	float DelayBetweenLetters = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	float TimeOnScreen = 5.0f;
};

USTRUCT(BlueprintType)
struct FInGameLogMessage_PVP
{
	GENERATED_BODY()

	FInGameLogMessage_PVP() = default;
	
	FInGameLogMessage_PVP(AReadyOrNotCharacter* InCauser, AReadyOrNotCharacter* InVictim, const EPVPEvent& InPVPEvent, const FText& InCustomMessage = FText::FromString(""), ECharacterDeathReason InCauseOfDeath = ECharacterDeathReason::None, const float InTimeOnScreen = 6.0f)
	{
		Causer = InCauser;
		Victim = InVictim;
		PVPEvent = InPVPEvent;
		CustomMessage = InCustomMessage;
		CauseOfDeath = InCauseOfDeath;
		TimeOnScreen = InTimeOnScreen;
	}

	FInGameLogMessage_PVP(AReadyOrNotCharacter* InCauser, const EPVPEvent& InPVPEvent, const FText& InCustomMessage = FText::FromString(""), ECharacterDeathReason InCauseOfDeath = ECharacterDeathReason::None, const float InTimeOnScreen = 6.0f)
	{
		Causer = InCauser;
		PVPEvent = InPVPEvent;
		CustomMessage = InCustomMessage;
		CauseOfDeath = InCauseOfDeath;
		TimeOnScreen = InTimeOnScreen;
	}
	
	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	AReadyOrNotCharacter* Causer = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	AReadyOrNotCharacter* Victim = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	EPVPEvent PVPEvent = EPVPEvent::None;

	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	FText CustomMessage = FText::FromString("");

	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	ECharacterDeathReason CauseOfDeath = ECharacterDeathReason::None;

	UPROPERTY(BlueprintReadWrite, Category = "Log Data")
	float TimeOnScreen = 6.0f;
};

/**
 * Provides access to the in-game log system for logging mission status messages or pvp events
 */
UCLASS()
class READYORNOT_API UInGameLogSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLogMessageEnqueued);
	UPROPERTY(BlueprintAssignable, Category = "In-Game Log Subsystem")
	FOnLogMessageEnqueued OnLogMessageEnqueued;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLogMessageDequeued);
	UPROPERTY(BlueprintAssignable, Category = "In-Game Log Subsystem")
	FOnLogMessageDequeued OnLogMessageDequeued;
	
	UFUNCTION(BlueprintCallable, Category = "In-Game Log Subsystem")
	void EnqueueLogMessages(const TArray<FInGameLogMessage>& InLogMessages);
	
	UFUNCTION(BlueprintCallable, Category = "In-Game Log Subsystem")
	void EnqueueLogMessage(FInGameLogMessage InLogMessage);

	UFUNCTION(BlueprintCallable, Category = "In-Game Log Subsystem")
	void EnqueuePVPMessages(const TArray<FInGameLogMessage_PVP>& InLogMessages);

	UFUNCTION(BlueprintCallable, Category = "In-Game Log Subsystem")
	void EnqueuePVPMessage(FInGameLogMessage_PVP InLogMessage);
	
	UFUNCTION(BlueprintPure, Category = "In-Game Log Subsystem")
	bool GetNextLogMessage(FInGameLogMessage& OutLogMessage);
	
	UFUNCTION(BlueprintPure, Category = "In-Game Log Subsystem", DisplayName = "Get Next Log Message (PVP)")
	bool GetNextLogMessage_PVP(FInGameLogMessage_PVP& OutLogMessage);

protected:
	TQueue<FInGameLogMessage> LogMessagesQueue;
	TQueue<FInGameLogMessage_PVP> PVPLogMessagesQueue;
};
