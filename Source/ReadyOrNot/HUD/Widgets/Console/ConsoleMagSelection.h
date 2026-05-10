// Copyright Void Interactive, 2022

#pragma once

#include "ConsoleMagSelection.generated.h"

UCLASS()
class READYORNOT_API UConsoleMagSelection : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnMagCheck(ABaseMagazineWeapon* MagazineWeapon);
	UFUNCTION()
	void OnWeaponFired(ABaseWeapon* Weapon);

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UHorizontalBox* Container;

	void UpdateMagazines(ABaseMagazineWeapon* MagazineWeapon);

protected:
	UPROPERTY()
	APlayerCharacter* PlayerCharacter;
	virtual bool Initialize() override;

};
