// Copyright Void Interactive, 2024

#include "LoadoutManager.h"

#include "Actors/BaseItem.h"
#include "Actors/SWATArmour.h"
#include "Data/CustomizationData.h"
#include "Engine/AssetManager.h"
#include "lib/GameFeatureLibrary.h"

double GetDuration(double StartTime, double EndTime) { return (EndTime - StartTime) * 1000.0; }

void ULoadoutManager::Initialize()
{
	double StartTime = FPlatformTime::Seconds();
	
#if defined(WITH_MODIO)
#endif

	LoadLoadoutItems();
	SortLoadoutItems();
	
	LoadArmourMaterials();
	LoadCustomizationAssets();
	
	double EndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Loadout Manager initialized in %.0f ms"), GetDuration(StartTime, EndTime));
}

TSubclassOf<ABaseItem> ULoadoutManager::GetItemByName(FName ItemName)
{
	for (ABaseItem* Item : AllItems)
	{
		if (IsValid(Item) && Item->GetPrimaryAssetId().PrimaryAssetName == ItemName)
			return Item->GetClass();
	}
	return nullptr;
}

TSubclassOf<ABaseItem> ULoadoutManager::GetItemByLookupIdx(FName LookupIdx)
{
	ABaseItem** Item = ItemLookupMap.Find(LookupIdx);
	if (Item && IsValid((*Item)))
	{
		return (*Item)->GetClass();
	}
	return nullptr;
}

TSubclassOf<ABaseItem> ULoadoutManager::GetItemByLookupIdx(const UObject* WorldContextObject, FName LookupIdx)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance || !GameInstance->LoadoutManager)
		return nullptr;

	return GameInstance->LoadoutManager->GetItemByLookupIdx(LookupIdx);
}

TArray<TSubclassOf<ABaseItem>> ULoadoutManager::GetAllItems(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return {};

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance)
		return {};

	ULoadoutManager* LoadoutManager = GameInstance->LoadoutManager;
	if (!LoadoutManager)
		return {};
	
	TArray<TSubclassOf<ABaseItem>> AllItemClasses;
	AllItemClasses.Reserve(LoadoutManager->AllItems.Num());
	
	for (ABaseItem* Item : LoadoutManager->AllItems)
	{
		AllItemClasses.Add(Item->GetClass());
	}
	return AllItemClasses;
}

void ULoadoutManager::LoadLoadoutItems()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	
	double GetAssetsStartTime = FPlatformTime::Seconds();
	
	TArray<FAssetData> AssetDatas;
	{
		FAssetManagerSearchRules SearchRules;
		SearchRules.AssetBaseClass = ABaseItem::StaticClass();
		SearchRules.AssetScanPaths = { "/Game" };
		SearchRules.bHasBlueprintClasses = true;
		SearchRules.bForceSynchronousScan = true;	

		AssetManager.SearchAssetRegistryPaths(AssetDatas, SearchRules);
	}
	
	double GetAssetsEndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Retrieved LoadoutItem asset list in %.0f ms"), GetDuration(GetAssetsStartTime, GetAssetsEndTime));

	double IteratingAssetsStartTime = FPlatformTime::Seconds();
	for (FAssetData& AssetData : AssetDatas)
	{
		double ItemStartTime = FPlatformTime::Seconds();

		// Don't remove, required to load the blueprint class which we reference below
		UObject* BlueprintAsset = AssetData.GetAsset();
		
		TSubclassOf<ABaseItem> Class = AssetManager.GetPrimaryAssetObjectClass<ABaseItem>(AssetData.GetPrimaryAssetId());
		if (!Class)
		{
			UE_LOG(LogReadyOrNotLoadout, Warning, TEXT("Failed to get class for loadout item %s"), *AssetData.AssetName.ToString());
			continue;
		}
		
		if (ABaseItem* Item = Class->GetDefaultObject<ABaseItem>())
		{
			if (!ensure(Item))
			{
				UE_LOG(LogReadyOrNotLoadout, Warning, TEXT("Default object did not exist for %s"), *Class->GetName());
				continue;
			}

#if !UE_BUILD_SHIPPING
			if (!Item->LookupTableIdx.IsNone())
			{
				ItemLookupMap.Add(Item->LookupTableIdx, Item);
			}
#endif
			
			if (!Item->bShowInLoadout)
				continue;

			bool bHasDlc = true;
			// TODO: Should use the ItemDataTable to check for DLC instead of the item itself
			for (const EGameVersionRestriction Dlc : Item->LockedToDLC)
			{
				if (!UGameFeatureLibrary::IsGameVersionEnabled(Dlc))
				{
					UE_LOG(LogReadyOrNotLoadout, Warning, TEXT("User does not have the DLC for %s"), *Class->GetName());
					bHasDlc = false;
				}
			}
			if (!bHasDlc)
				continue;
			
			AllItems.AddUnique(Item);
			
			double ItemEndTime = FPlatformTime::Seconds();
			UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Loaded LoadoutItem %s in %.0f ms"), *FPackageName::GetShortName(Item->GetPackage()->GetName()), GetDuration(ItemStartTime, ItemEndTime));
		}
	}
	double IteratingAssetsEndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Loaded all LoadoutItems in %.0f ms"), GetDuration(IteratingAssetsStartTime, IteratingAssetsEndTime));
}

void ULoadoutManager::SortLoadoutItems()
{
	double SortingAssetsStartTime = FPlatformTime::Seconds();

	Algo::Sort(AllItems, [](const ABaseItem* Left, const ABaseItem* Right)
	{
		if (Left->LoadoutPriority != Right->LoadoutPriority)
			return Left->LoadoutPriority < Right->LoadoutPriority;
		
		if (Left->ItemClass != Right->ItemClass)
			return Left->ItemClass < Right->ItemClass;
		
		return Left->ItemName.CompareToCaseIgnored(Right->ItemName) < 0;
	});

	for (ABaseItem* Item : AllItems)
	{
		if (Item->CategoryFlags & static_cast<uint32>(ELoadoutCategory::Primary))
			PrimaryWeapons.AddUnique(Item);
				
		if (Item->CategoryFlags & static_cast<uint32>(ELoadoutCategory::Secondary))
			SecondaryWeapons.AddUnique(Item);

		if (Item->CategoryFlags & static_cast<uint32>(ELoadoutCategory::LongTactical))
			LongTacticalItems.AddUnique(Item);

		if (Item->CategoryFlags & static_cast<uint32>(ELoadoutCategory::TacticalDevice))
			TacticalItems.AddUnique(Item);

		if (Item->ItemCategories.Contains(EItemCategory::IC_Armor) && !Item->ItemCategories.Contains(EItemCategory::IC_Helmet))
			BodyArmors.AddUnique(Item);

		if (Item->ItemCategories.Contains(EItemCategory::IC_Helmet))
			Helmets.AddUnique(Item);
	}
	
	double SortingAssetsEndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Sorted all LoadoutItems in %.0f ms"), GetDuration(SortingAssetsStartTime, SortingAssetsEndTime));
}

template<typename T>
void LoadAllAssetsGeneric(FPrimaryAssetType AssetType, TArray<T*>& OutAssets)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	double GetAssetsStartTime = FPlatformTime::Seconds();
	
	TArray<FAssetData> AssetDatas;
	//AssetManager.GetPrimaryAssetDataList(AssetType, AssetDatas);
	AssetManager.GetAssetRegistry().GetAssetsByClass(T::StaticClass()->GetFName(), AssetDatas, true);
	
	double GetAssetsEndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Retrieved %s asset list in %.0f ms"), *AssetType.ToString(), GetDuration(GetAssetsStartTime, GetAssetsEndTime));

	double IteratingAssetsStartTime = FPlatformTime::Seconds();
	for (const FAssetData& AssetData : AssetDatas)
	{
		double ItemStartTime = FPlatformTime::Seconds();
		
		T* Asset = Cast<T>(AssetData.GetAsset());
		if (!Asset)
			continue;

		bool bHasDlc = true;
		// TODO: Should use the ItemDataTable to check for DLC instead of the asset itself
		for (const EGameVersionRestriction Dlc : Asset->LockedToDLC)
		{
			if (!UGameFeatureLibrary::IsGameVersionEnabled(Dlc))
			{
				UE_LOG(LogReadyOrNotLoadout, Warning, TEXT("User does not have the DLC for %s"), *Asset->GetName());
				bHasDlc = false;
			}
		}
		if (!bHasDlc)
			continue;
		
		OutAssets.Add(Asset);

		double ItemEndTime = FPlatformTime::Seconds();
		UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Loaded %s %s in %.0f ms"), *AssetType.ToString(), *FPackageName::GetShortName(Asset->GetPackage()->GetName()), GetDuration(ItemStartTime, ItemEndTime));
	}
	double IteratingAssetsEndTime = FPlatformTime::Seconds();
	UE_LOG(LogReadyOrNotLoadout, Log, TEXT("Loaded and stored all %ss in %.0f ms"), *AssetType.ToString(), GetDuration(IteratingAssetsStartTime, IteratingAssetsEndTime));
}

void ULoadoutManager::LoadArmourMaterials()
{
	ArmorMaterials.Empty();
	LoadAllAssetsGeneric<UArmourMaterial>("ArmourMaterial", ArmorMaterials);

	Algo::Sort(ArmorMaterials, [](const UArmourMaterial* Left, const UArmourMaterial* Right)
	{
		return Left->Priority < Right->Priority;
	});
}

void ULoadoutManager::LoadCustomizationAssets()
{
	CustomizationAssets.Empty();
	LoadAllAssetsGeneric<UCustomizationDataBase>("CustomizationAsset", CustomizationAssets);

	Algo::Sort(CustomizationAssets, [](const UCustomizationDataBase* Left, const UCustomizationDataBase* Right)
	{
		// "Family" parents should show up first
		if (Right->Parent == Left)
			return true;

		// Then order by display priority
		if (Left->DisplayPriority != Right->DisplayPriority)
			return Left->DisplayPriority < Right->DisplayPriority;

		// Finally order alphabetically
		return Left->Name.CompareToCaseIgnored(Right->Name) < 0;
	});
}
