// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/CustomizationData.h"
#include "HUD/Widgets/PreMissionPlanning.h"
#include "lib/BpGameplayHelperLib.h"
#include "BaseProfile.generated.h"

/**
 * Base profile, contains common fields between all profiles
 */
UCLASS()
class READYORNOT_API UBaseProfile : public USaveGame
{
	GENERATED_BODY()

public:
	UBaseProfile();
	
	virtual void Serialize(FArchive& Ar) override;
	
	virtual void SaveProfile() {};
	static UBaseProfile* GetCurrentProfile();

private:
	// Version number for tracking if this save needs to be upgraded
	UPROPERTY()
	int32 BaseVersion = 0;

public:
	/* Loadout */
	
	UPROPERTY()
	TArray<FSavedLoadout> Loadouts;

	UPROPERTY()
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponAttachmentData> AttachmentSaveMap;

	UPROPERTY()
	TMap<TSubclassOf<ABaseWeapon>, FStoredWeaponAttachments> LoadoutAttachmentSaveMap;

	UPROPERTY()
	TMap<EItemType, TSubclassOf<ABaseItem>> SavedWeaponClassOfTypeMap;

	UPROPERTY()
	TMap<TSubclassOf<ABaseWeapon>, EFireMode> WeaponClassToDefaultFireModeMap;

	UPROPERTY()
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponPreset> WeaponToWeaponPresetsMap;
	
	UPROPERTY()
	TMap<FString, FSavedLoadout> LoadoutPresetMap;

	/* Customization */

	UPROPERTY()
	TMap<EEquippingSwat, FSavedCustomization> Customizations;
};
