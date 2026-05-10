// Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "PingInterface.generated.h"

USTRUCT(BlueprintType)
struct FPingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush IconBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 10.0f;
};

UINTERFACE(MinimalAPI)
class UPingInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class READYORNOT_API IPingInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Ping Interface")
	FSlateBrush GetPingIcon();

	UFUNCTION(BlueprintNativeEvent, Category = "Ping Interface")
	FText GetPingText();

	UFUNCTION(BlueprintNativeEvent, Category = "Ping Interface")
	FVector GetPingLocation();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Ping Interface")
    float GetPingDuration();

	UFUNCTION(BlueprintNativeEvent, Category = "Ping Interface")
	bool CanPing();

	uint8 bPinged : 1;
};
