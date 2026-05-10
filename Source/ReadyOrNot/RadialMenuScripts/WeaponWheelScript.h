// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "RadialMenuScripts/BaseRadialMenuScript.h"
#include "WeaponWheelScript.generated.h"

/**
 * A base class for executing code on a weapon wheel selection
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UWeaponWheelScript final : public UBaseRadialMenuScript
{
	GENERATED_BODY()

public:
	UWeaponWheelScript(const FObjectInitializer& ObjectInitializer);
	
protected:
	void ExecuteScript_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Wheel Script")
	FText ItemName = FText::FromString("None");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Wheel Script")
	FText ItemCategory = FText::FromString("None");

	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Wheel Script")
	//uint8 bShowInWeaponWheel : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Wheel Script")
	class UWeaponWheelWidget* WeaponWheelOwner;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Wheel Script")
	class APlayerCharacter* PlayerCharacter;
};
