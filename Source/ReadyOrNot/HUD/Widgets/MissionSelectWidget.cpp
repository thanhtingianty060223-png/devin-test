// Copyright Void Interactive, 2023

#include "MissionSelectWidget.h"

#include "Actors/Environment/MissionPortal.h"
#include "Actors/Environment/MissionSelect.h"
#include "Commander/CampaignData.h"

void UMissionSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UDataTable* LevelDataTable = UBpGameplayHelperLib::GetLevelLookupDataTable();
	if (!LevelDataTable)
		return;

	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	if (!CampaignData)
		return;
	
	CachedLevelDatas.Empty();
	for (FString Map : CampaignData->Levels)
	{
		FName MapName = FName(Map);
		
		FLevelDataLookupTable* LookupTable = LevelDataTable->FindRow<FLevelDataLookupTable>(MapName, "MissionSelect");
		if (!LookupTable)
			continue;

		bool bIsUnlocked = false;
		float ScoreRequired;
		FString LockedURL;
		AMissionPortal::IsLevelUnlocked(Map, bIsUnlocked, ScoreRequired, LockedURL);
		
		ULevelData* LevelData = NewObject<ULevelData>();
		LevelData->bIsLocked = !bIsUnlocked;
		LevelData->LevelName = MapName;
		SetupLevelData(LevelData, LookupTable);

		CachedLevelDatas.Add(LevelData);
	}

	UReadyOrNotGameInstance* GameInstance = GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance)
		return; // !?

	// Sort our level data first
	TArray<UModLevelData*> UnsortedModLevels = GameInstance->ModdedLevelDataAssets;
	UnsortedModLevels.Sort([](const UModLevelData& Left, const UModLevelData& Right)
	{
		return Right.LevelName.LexicalLess(Left.LevelName);
	});
	
	for (UModLevelData* ModLevelData : UnsortedModLevels)
	{
		if (!ModLevelData->bShowInMissionSelect)
			continue;
		
		ULevelData* LevelData = NewObject<ULevelData>();
		LevelData->bIsLocked = false;
		LevelData->LevelName = ModLevelData->LevelName;
		SetupLevelData(LevelData, &ModLevelData->Data);

		CachedLevelDatas.Add(LevelData);
	}
}

const TArray<ULevelData*>& UMissionSelectWidget::GetLevelDataList()
{
	return CachedLevelDatas;
}

void UMissionSelectWidget::CloseMissionSelect()
{
	for (TActorIterator<AMissionSelect> It(GetWorld()); It; ++It)
	{
		AMissionSelect* MissionSelect = *It;
		if (IsValid(MissionSelect))
		{
			MissionSelect->CloseMissionSelect();
			break;
		}
	}
}

void UMissionSelectWidget::PreviewMission(ULevelData* LevelData)
{
	for (TActorIterator<AMissionSelect> It(GetWorld()); It; ++It)
	{
		AMissionSelect* MissionSelect = *It;
		if (IsValid(MissionSelect))
		{
			MissionSelect->PreviewMission(LevelData);
			break;
		}
	}
}


void UMissionSelectWidget::SelectMission(ULevelData* LevelData)
{
	for (TActorIterator<AMissionSelect> It(GetWorld()); It; ++It)
	{
		AMissionSelect* MissionSelect = *It;
		if (IsValid(MissionSelect))
		{
			MissionSelect->SelectMission(LevelData);
			break;
		}
	}
}

void UMissionSelectWidget::SetupLevelData(ULevelData* LevelData, FLevelDataLookupTable* LookupTable)
{
	if (!LevelData)
		return;
	
	LevelData->LevelPreview = LookupTable->MissionSelectMap;
	LevelData->LevelDescription = LookupTable->Description;
	LevelData->LevelImage = LookupTable->LevelPicture;
	LevelData->LevelFriendlyName = LookupTable->FriendlyLevelName;
	LevelData->LevelNickname = LookupTable->LevelNickname;
	LevelData->LevelLocation = LookupTable->LevelDesignation;
	LevelData->LevelTimeOfDay = LookupTable->TimeOfDayText;

	TArray<FEntryPoint> EntryPoints = LookupTable->EntryPoints;
	for (const FLevelFloor& LevelFloor : LookupTable->Floors)
	{
		EntryPoints.Append(LevelFloor.EntryPoints);
	}
	LevelData->EntryPoints = EntryPoints;

	const FString* MapString = LookupTable->COOPModesLevelMap.Find(ECOOPMode::CM_BarricadedSuspects);
	if (MapString)
	{
		UReadyOrNotProfile::LoadLevelStats(LevelData->LevelStats, ECOOPMode::CM_BarricadedSuspects, *MapString);
	}
}
