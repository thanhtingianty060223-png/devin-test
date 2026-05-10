// Copyright Void Interactive, 2023


#include "MapAssetUsageEditorWidget.h"

#include "ReadyOrNot.h"
#include "Engine/AssetManager.h"
#include "Engine/ObjectLibrary.h"

void UMapAssetUsageEditorWidget::RefreshLevels()
{
	UObjectLibrary* const ObjectLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game/ReadyOrNot/Level"));
	TArray<FAssetData> AssetDatas;
	ObjectLibrary->GetAssetDataList(AssetDatas);

	Levels.Empty();
	for(FAssetData Asset : AssetDatas)
	{
		// Maps are classed as World assets
		if(Asset.AssetClass == FName(TEXT("World")))
		{
			UE_LOG(ReadyOrNotEditorModule, Verbose, TEXT("Found map: %s"), *Asset.AssetName.ToString());
			FMapAsset a;
			a.AssetName = Asset.AssetName;
			a.PackageName = Asset.PackageName;
			Levels.Add(a);
		}
	}
	Levels.Sort([](const FMapAsset& ip1, const FMapAsset& ip2) {
		return  ip1.AssetName.FastLess(ip2.AssetName);
	});
}

void UMapAssetUsageEditorWidget::GetAssetSummaryForMaps(TArray<FMapAsset> SelectedMaps)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetSummaries.Empty();
	for (const auto m : SelectedMaps)
	{
		TArray<FName> OutDependencies;
		AssetRegistry.GetDependencies(m.PackageName,OutDependencies);

		for(const FName Dependency : OutDependencies)
		{
			FMapAssetSummary* FoundSummary = AssetSummaries.FindByPredicate([&Dependency](const FMapAssetSummary& S)
			{
				return S.AssetName == Dependency;
			});
			
			if(FoundSummary == nullptr)
			{
				// TODO Find more information about the asset, like the type, size etc.
				FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(Dependency, false);
				FMapAssetSummary Summary;
				Summary.AssetName = Dependency;
				Summary.AssetClass = AssetData.AssetClass;
				Summary.ReferencedMaps.Add(m.AssetName);
				AssetSummaries.Add(Summary);
			}
			else
			{
				FoundSummary->ReferencedMaps.Add(m.AssetName);
			}
		}
	}

	AssetSummaries.Sort([](const FMapAssetSummary &ip1, const FMapAssetSummary &ip2) {
		return ip1.ReferencedMaps.Num() > ip2.ReferencedMaps.Num();
	});

	for(auto a : AssetSummaries)
	{
		UE_LOG(ReadyOrNotEditorModule, Verbose, TEXT("%i: %s "), a.ReferencedMaps.Num(), *a.AssetName.ToString());
	}
}

FString UMapAssetUsageEditorWidget::GenerateSummaryText()
{
	// TODO Return this as a row in a list as opposed to a CSV text string
	FString Result;
	for(auto a : AssetSummaries)
	{
		Result += FString::Printf(TEXT("%s, %i \n"), *a.AssetName.ToString(), a.ReferencedMaps.Num());
	}
	return Result;
}
 