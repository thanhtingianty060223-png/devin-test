// Copyright Void Interactive, 2023


#include "lib/BpAutomationLib.h"
#include <InputCoreTypes.h>
#include <GameFramework/InputSettings.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerInput.h>
#include <Kismet/GameplayStatics.h>
#include "FMODBlueprintStatics.h"
#include "Actors/BaseItem.h"
#include "Actors/BaseWeapon.h"


void UTestInputBlueprintFunctionLibrary::RonApplyInputAction(
    UObject* Context, const FName& ActionName,
    EInputEvent InputEventType)
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Context, 0);

    const UInputSettings* InputSettings = GetDefault<UInputSettings>();

    for (const FInputActionKeyMapping& Mapping : InputSettings->GetActionMappings())
    {
        if (Mapping.ActionName == ActionName)
        {
			// ##UE5UPGRADE##
			FInputKeyParams Params(Mapping.Key, InputEventType, double(0.f), false);
            PlayerController->InputKey(Params);
            return;
        }
    }
}

void UTestInputBlueprintFunctionLibrary::RonApplyInputAxis(UObject* Context, const FName& AxisName,
    float AxisValue)
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Context, 0);

    if (!IsValid(PlayerController))
    {
        return;
    }

    const UInputSettings* InputSettings = GetDefault<UInputSettings>();

    for (const FInputAxisKeyMapping& Mapping : InputSettings->GetAxisMappings())
    {
        if (Mapping.AxisName == AxisName)
        {
			// ##UE5UPGRADE##
			FInputKeyParams Params(Mapping.Key, AxisValue, double(0.f), 1, false);
            PlayerController->InputKey(Params);
            return;
        }
    }
}

//Wasn't accurate for finding current ammo type
/*void UTestInputBlueprintFunctionLibrary::GetCurrentAmmo(ABaseWeapon* BaseWeapon, FName& OutputOne)
{
    if (BaseWeapon) {
        const FAmmoTypeData* ammoType = BaseWeapon->GetCurrentAmmoType();
        FName ammoTypeRowName = BaseWeapon->GetCurrentAmmoTypeRowName();
        OutputOne = ammoTypeRowName;
    }
    return;
}*/