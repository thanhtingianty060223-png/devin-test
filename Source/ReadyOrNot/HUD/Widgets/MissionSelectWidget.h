// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Metagame/Profile.h"
#include "MissionSelectWidget.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API ULevelData : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TSoftObjectPtr<UWorld> LevelPreview;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsLocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FBasicLevelStats LevelStats;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LevelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LevelFriendlyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LevelNickname;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> LevelImage;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LevelDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LevelLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LevelTimeOfDay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FEntryPoint> EntryPoints;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UMissionSelectWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere)
	TArray<FName> DefaultMaps;
	
	UFUNCTION(BlueprintCallable)
	const TArray<ULevelData*>& GetLevelDataList();

	UFUNCTION(BlueprintCallable)
	void CloseMissionSelect();

	UFUNCTION(BlueprintCallable)
	void PreviewMission(ULevelData* LevelData);
	
	UFUNCTION(BlueprintCallable)
	void SelectMission(ULevelData* LevelData);
	
private:
	UPROPERTY(Transient)
	TArray<ULevelData*> CachedLevelDatas;

	void SetupLevelData(ULevelData* LevelData, FLevelDataLookupTable* LookupTable);
};
