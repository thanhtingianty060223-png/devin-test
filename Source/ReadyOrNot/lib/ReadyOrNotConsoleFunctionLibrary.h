// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/OnlineGameActivityInterface.h"
#include "ReadyOrNotConsoleFunctionLibrary.generated.h"


UENUM(BlueprintType)
enum class ERuntimeDevice : uint8
{
	PS4,
	PS4_Pro,
	PS4_Pro_4K,
	PS5,
	XBoxOne,
	XBoxOneS,
	XBoxOneX,
	XBoxSeriesX,
	XBoxSeriesS,
	PC
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotConsoleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static ERuntimeDevice GetRuntimeDeviceProfile();
	UFUNCTION()
	static void ConsoleApplyLevelSpecificSettings(FString MapName, bool QualityOverFrameRate);
};
