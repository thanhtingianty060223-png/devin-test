// Void Interactive, 2020

#pragma once

#include "TOCData.generated.h"

UENUM(BlueprintType)
enum class ETOCPriority : uint8
{
	ETP_Flush, // Add to front of the queue and flush all line remaining Lines 
	ETP_HighPriority, // Add to front of the queue
    ETP_MediumPriority, // Add to back of the queue
    ETP_LowPriority, // Add to back of Queue and overwrite last line if it's low priority
};

USTRUCT(BlueprintType)
struct FTOCData
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString TOCLine = "";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETOCPriority QueuePriority = ETOCPriority::ETP_MediumPriority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsNetworked : 1;

	friend bool operator==(const FTOCData& Lhs, const FTOCData& Rhs)
	{
		return Lhs.TOCLine == Rhs.TOCLine &&
				Lhs.QueuePriority == Rhs.QueuePriority &&
				Lhs.bIsNetworked == Rhs.bIsNetworked;
	}
};
