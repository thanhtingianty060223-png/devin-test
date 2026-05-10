// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LookupData.generated.h"

class AProjectile;
class ABaseItem;
class ABaseWeapon;
class ABaseArmour;
class ABaseGrenade;
class ABaseShell;
class ACyberneticCharacter;
class APlayerCharacter;

/**
 * 
 */
UCLASS()
class READYORNOT_API ULookupData : public UDataAsset
{
	GENERATED_BODY()

#if WITH_EDITOR
		void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// toggle this to force update this data asset
	UPROPERTY(EditAnywhere, Category = "Update")
	bool bToggleToDoAssetUpdate = false;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ABaseItem>> Items;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ABaseWeapon>> Weapons;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ABaseArmour>> Armour;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ABaseGrenade>> Grenades;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ABaseShell>> Shells;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<AProjectile>> Projectiles;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<APlayerCharacter>> Characters;

	UPROPERTY(VisibleAnywhere, Category = "Item Lookup")
		TArray<TSoftClassPtr<ACyberneticCharacter>> AI;


	// toggle this to force update the lookup data table 
	UPROPERTY(EditAnywhere, Category = "Item")
		bool bAddSelectedBlueprintToItemData = false;

	UPROPERTY(EditAnywhere, Category = "Item")
		TArray<TSubclassOf<ABaseItem>> biClassArray;

	UPROPERTY(EditAnywhere, Category = "Item")
	class UDataTable* ItemDataLookupTable;

	UPROPERTY(EditAnywhere, Category = "Level")
		bool bToggleForceLevelDataTable = false;
	UPROPERTY(EditAnywhere, Category = "Level")
	class UDataTable* LevelDataLookupTable;

	UPROPERTY(EditAnywhere, Category = "AI")
		bool bToggleForceAIData = false;
	UPROPERTY(EditAnywhere, Category = "AI")
		class UDataTable* AIDataLookupTable;

	UFUNCTION(CallInEditor)
	void EmptyData();

public:

	/* WARNING HAS EXTREMELY HIGH COST */
	void ForceUpdate();

	TArray<TSoftClassPtr<ABaseItem>> GetItems() { return Items; }
	TArray<TSoftClassPtr<ABaseWeapon>> GetWeapons() { return Weapons; }
	TArray<TSoftClassPtr<ABaseArmour>> GetArmour() { return Armour; }
	TArray<TSoftClassPtr<ABaseGrenade>> GetGrenades() { return Grenades; }
	TArray<TSoftClassPtr<ABaseShell>> GetShells() { return Shells; }
	TArray<TSoftClassPtr<AProjectile>> GetProjectiles() { return Projectiles; }
	TArray<TSoftClassPtr<APlayerCharacter>> GetCharacters() { return Characters; }
	TArray<TSoftClassPtr<ACyberneticCharacter>> GetAICharacters() { return AI; }
};
