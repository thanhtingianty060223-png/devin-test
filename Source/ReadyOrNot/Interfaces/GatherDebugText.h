// Copyright Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "GatherDebugText.generated.h"

USTRUCT(BlueprintType)
struct FDebugData
{
	GENERATED_BODY()

	FDebugData()
	{
		Label = FText::FromString("None");
		Value = FText::FromString("None");
	}
	
	FDebugData(const FString& InLabel)
	{
		Label = FText::FromString(InLabel);
		Value = FText::FromString("None");
	}
	
	FDebugData(const FString& InLabel, const FText& InValue)
	{
		Label = FText::FromString(InLabel);
		Value = InValue;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Data")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Data")
	FText Value;
};

/**
 * 
 */
UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UGatherDebugInterface : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IGatherDebugInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|GatherDebugInterface")
	void GatherDebugData(TArray<FDebugData>& OutDebugData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|GatherDebugInterface")
	void GatherDebugText(FString& OutText);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|GatherDebugInterface")
	void DrawVisualDebug();
};

