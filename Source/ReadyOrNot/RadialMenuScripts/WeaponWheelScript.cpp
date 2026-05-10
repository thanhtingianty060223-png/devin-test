// Copyright Void Interactive, 2021


#include "WeaponWheelScript.h"

#include "Characters/PlayerCharacter.h"

#include "HUD/Widgets/WeaponWheelWidget.h"

UWeaponWheelScript::UWeaponWheelScript(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//bShowInWeaponWheel = true;
}

void UWeaponWheelScript::ExecuteScript_Implementation()
{
	Super::ExecuteScript_Implementation();

	if (!WeaponWheelOwner)
		WeaponWheelOwner = Cast<UWeaponWheelWidget>(RadialMenuOwner);

	PlayerCharacter = Cast<APlayerCharacter>(Actor);
}
