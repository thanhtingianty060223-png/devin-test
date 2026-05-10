// Copyright Void Interactive, 2024

#pragma once

#include "CoreMinimal.h"
#include "LoadoutManager.generated.h"

class ABaseItem;
class UArmourMaterial;
class UCustomizationDataBase;

/**
 * Loadout manager, handles caching for all loadout / customization assets in the game
 */
UCLASS(Blueprintable)
class READYORNOT_API ULoadoutManager : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<ABaseItem*> AllItems;
	
	UPROPERTY()
	TArray<ABaseItem*> PrimaryWeapons;

	UPROPERTY()
	TArray<ABaseItem*> SecondaryWeapons;

	UPROPERTY()
	TArray<ABaseItem*> LongTacticalItems;

	UPROPERTY()
	TArray<ABaseItem*> TacticalItems;

	UPROPERTY()
	TArray<ABaseItem*> BodyArmors;

	UPROPERTY()
	TArray<ABaseItem*> Helmets;

	UPROPERTY()
	TArray<UArmourMaterial*> ArmorMaterials;

	UPROPERTY()
	TArray<UCustomizationDataBase*> CustomizationAssets;
	
	void Initialize();
	
	TSubclassOf<ABaseItem> GetItemByName(FName ItemName);
	TSubclassOf<ABaseItem> GetItemByLookupIdx(FName LookupIdx);

	UFUNCTION(meta=(WorldContext="WorldContextObject")) // could be bp callable in future
	static TSubclassOf<ABaseItem> GetItemByLookupIdx(const UObject* WorldContextObject, FName LookupIdx);
	
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static TArray<TSubclassOf<ABaseItem>> GetAllItems(UObject* WorldContextObject);

private:
	UPROPERTY()
	TMap<FName, ABaseItem*> ItemLookupMap;

	void LoadLoadoutItems();
	void SortLoadoutItems();
	
	void LoadArmourMaterials();
	void LoadCustomizationAssets();
};
