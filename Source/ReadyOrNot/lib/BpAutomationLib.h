// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include <Engine/EngineBaseTypes.h>
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineBaseTypes.h"
#include "Actors/BaseWeapon.h"
#include "FMODBlueprintStatics.h"
#include "Actors/BaseItem.h"
#include "BpAutomationLib.generated.h"



UCLASS()
class UTestInputBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Applies the input action with the specified name once. */
    UFUNCTION(BlueprintCallable, Category = "TestAutomation|Input",
        meta = (HidePin = "Context", DefaultToSelf = "Context"))
        static void RonApplyInputAction(UObject* Context, const FName& ActionName,
            EInputEvent InputEventType = EInputEvent::IE_Pressed);

    /** Applies the input axis with the specified name. Pass AxisValue 0.0f to reset the input axis. */
    UFUNCTION(BlueprintCallable, Category = "TestAutomation|Input",
        meta = (HidePin = "Context", DefaultToSelf = "Context"))
        static void RonApplyInputAxis(UObject* Context, const FName& AxisName, float AxisValue = 1.0f);


    /*UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Current Ammo Type"), Category = "TestAutomation|Input")
        static void GetCurrentAmmo(ABaseWeapon* BaseWeapon, FName& OutputOne);*/
};
