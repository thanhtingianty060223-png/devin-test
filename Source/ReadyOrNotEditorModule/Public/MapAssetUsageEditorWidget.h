// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MapAssetUsageEditorWidget.generated.h"


USTRUCT(BlueprintType)
struct FMapAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName AssetName;

	UPROPERTY(BlueprintReadOnly)
	FName PackageName;
};

USTRUCT(BlueprintType)
struct FMapAssetSummary
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName AssetName;

	UPROPERTY(BlueprintReadOnly)
	TArray<FName> ReferencedMaps;

	UPROPERTY(BlueprintReadOnly)
	FName AssetClass;
};


UCLASS()
class READYORNOTEDITORMODULE_API UMapAssetUsageEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FMapAsset> Levels;

	UPROPERTY(BlueprintReadOnly)
	TArray<FMapAssetSummary> AssetSummaries;
	
	UFUNCTION(BlueprintCallable)
	void RefreshLevels() ;

	UFUNCTION(BlueprintCallable)
	void GetAssetSummaryForMaps(TArray<FMapAsset> SelectedMaps);

	UFUNCTION(BlueprintCallable)
	FString GenerateSummaryText();
};
