// Void Interactive, 2020

#include "InGameLogSubsystem.h"

void UInGameLogSubsystem::EnqueueLogMessages(const TArray<FInGameLogMessage>& InLogMessages)
{
	for (const FInGameLogMessage& lm : InLogMessages)
	{
		EnqueueLogMessage(lm);
	}
}

void UInGameLogSubsystem::EnqueueLogMessage(const FInGameLogMessage InLogMessage)
{
	LogMessagesQueue.Enqueue(InLogMessage);

	OnLogMessageEnqueued.Broadcast();
}

void UInGameLogSubsystem::EnqueuePVPMessages(const TArray<FInGameLogMessage_PVP>& InLogMessages)
{
	for (const FInGameLogMessage_PVP& lm : InLogMessages)
	{
		EnqueuePVPMessage(lm);
	}
}

void UInGameLogSubsystem::EnqueuePVPMessage(const FInGameLogMessage_PVP InLogMessage)
{
	PVPLogMessagesQueue.Enqueue(InLogMessage);

	OnLogMessageEnqueued.Broadcast();
}

bool UInGameLogSubsystem::GetNextLogMessage(FInGameLogMessage& OutLogMessage)
{
	if (LogMessagesQueue.Dequeue(OutLogMessage))
	{
		OnLogMessageDequeued.Broadcast();
		return true;
	}
	
	return false;
}

bool UInGameLogSubsystem::GetNextLogMessage_PVP(FInGameLogMessage_PVP& OutLogMessage)
{
	if (PVPLogMessagesQueue.Dequeue(OutLogMessage))
	{
		OnLogMessageDequeued.Broadcast();
		return true;
	}

	return false; 
}
