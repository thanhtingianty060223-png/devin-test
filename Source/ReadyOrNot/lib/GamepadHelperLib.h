#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GamepadHelperLib.generated.h"

UCLASS()
class READYORNOT_API UGamepadHelperLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gamepad Helper")
	static int GetActiveButton(int currentIndex, int navigationDirection, TArray<bool> buttonAvailability);

	UFUNCTION(BlueprintCallable, Category = "Gamepad Helper")
	static bool EnableCommonInputPreprocessing(UWorld* World, bool Enable);
};
