// Copyright Void Interactive, 2022

#include "DataSingleton.h"
#include "ReadyOrNot.h"
#include "Animation/MoveStyle/RoNMoveStyleTable.h"

#if WITH_EDITOR

UDataSingleton& UDataSingleton::Get()
{
	UDataSingleton *Singleton = Cast<UDataSingleton>(GEngine->GameSingleton);

	if (Singleton)
	{
		return *Singleton;
	}
	else
	{
		UE_LOG(LogReadyOrNot, Fatal, TEXT("Invalid singleton"));
		return *NewObject<UDataSingleton>(UDataSingleton::StaticClass()); // never calls this
	}
}

void UDataSingleton::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}
	FName PropName = PropertyChangedEvent.Property->GetFName();

	if (PropName == "RonInputKeyTable")
	{
		RefreshInputKeyTable();
	}
	
// 	if (PropName == TEXT("CampaignLevels") || PropName == TEXT("LevelList") || PropName == TEXT("LevelListDevelopment"))
// 	{
// 		MapDataMap.Empty();
// 		LevelListDataMap.Empty();

// 		for (int32 i = 0; i < CampaignLevels.Num(); i++)
// 		{
// 			FCampaignList cl = CampaignLevels[i];
// 			cl.LevelName = "";
// 			cl.DebriefingLevelName = "";
// 			cl.UnlockedLevelName = "";
// 			if (cl.Level.LoadSynchronous())
// 			{
// 				cl.LevelName = cl.Level.LoadSynchronous()->GetMapName();
// 			}
// 			if (cl.DebriefingLevel.LoadSynchronous())
// 			{
// 				cl.DebriefingLevelName = cl.DebriefingLevel.LoadSynchronous()->GetMapName();
// 			}
// 			if (cl.UnlockedLevel.LoadSynchronous())
// 			{
// 				cl.UnlockedLevelName = cl.UnlockedLevel.LoadSynchronous()->GetMapName();
// 			}
// 			CampaignLevels[i] = cl;
// 		}
// 		for (int32 i = 0; i < CampaignLevels.Num(); i++)
// 		{
// 			UWorld* world = CampaignLevels[i].Level.LoadSynchronous();
// 			if (world)
// 			{
// 				MapDataMap.Add(world->GetMapName(), UBpGameplayHelperLib::GetLevelData(world));
// 			}
// 			UWorld* unlockedworld = CampaignLevels[i].UnlockedLevel.LoadSynchronous();
// 			if (unlockedworld)
// 			{
// 				MapDataMap.Add(unlockedworld->GetMapName(), UBpGameplayHelperLib::GetLevelData(unlockedworld));
// 			}
// 			UWorld* debriefingworld = CampaignLevels[i].DebriefingLevel.LoadSynchronous();
// 			if (debriefingworld)
// 			{
// 				MapDataMap.Add(debriefingworld->GetMapName(), UBpGameplayHelperLib::GetLevelData(debriefingworld));
// 			}
// 		}
// 		for (int32 i = 0; i < LevelList.Num(); i++)
// 		{
// 			UWorld* world = LevelList[i].LoadSynchronous();
// 			if (world)
// 			{
// 				MapDataMap.Add(world->GetMapName(), UBpGameplayHelperLib::GetLevelData(world));
// 				LevelListDataMap.Add(world->GetMapName(), UBpGameplayHelperLib::GetLevelData(world));
// 			}
// 		}
// 		for (int32 i = 0; i < LevelListDevelopment.Num(); i++)
// 		{
// 			UWorld* world = LevelListDevelopment[i].LoadSynchronous();
// 			if (world)
// 			{
// 				LevelListDataMapDevelopment.Add(world->GetMapName(), UBpGameplayHelperLib::GetLevelData(world));
// 			}
// 		}
//	}
}
#endif

void UDataSingleton::RefreshInputKeyTable()
{
	if (UDataTable* InputKeyTable = RonInputKeyTable)
	{		
		TArray<FKey> AllKeys;
		EKeys::GetAllKeys(AllKeys);

		TArray<FRonInputKeyTable*> RonInputKeyTableRows;
		InputKeyTable->GetAllRows(TEXT("RefreshInputKeyTable"), RonInputKeyTableRows);

		InputKeyTable->EmptyTable();

		int32 i = 0;
		for (const FKey& Key : AllKeys)
		{
			FRonInputKeyTable InputKeyTableRow;
			InputKeyTableRow.Key.InputName = Key.GetDisplayName().ToString();
			InputKeyTableRow.Key.AlternativeInputName = Key.GetDisplayName().ToString();
			InputKeyTableRow.Key.IconBrush = RonInputKeyTableRows.IsValidIndex(i) ? RonInputKeyTableRows[i]->Key.IconBrush : FSlateBrush();
			
			InputKeyTable->AddRow(Key.GetFName(), InputKeyTableRow);

			i++;
		}

		InputKeyTable->Modify();
	}
}

/* NOT REQUIRED DUE TO MAP DATA MAP CHANGES SEE DATA SINGLETON.H OR ABOVE */
void UDataSingleton::UnloadLevels()
{
// 	LevelListLoaded.Empty();
// 
// 	FStreamableManager& Streamable = AssetLoader;
// 	for (TAssetPtr<UWorld> world : LevelList)
// 	{
// 		Streamable.Unload(world.ToSoftObjectPath());
// 	}
}

void UDataSingleton::LoadLevels()
{
	
// 	TArray<FSoftObjectPath> ItemsToStream;
// 
// 	for (TAssetPtr<UWorld> world : LevelList)
// 	{
// 		ItemsToStream.AddUnique(world.ToSoftObjectPath());
// 	}
// 
// 	FStreamableManager& Streamable = AssetLoader;
// 	Streamable.RequestAsyncLoad(ItemsToStream, FStreamableDelegate::CreateUObject(this, &UDataSingleton::LoadLevelsDeffered), 1, true);
}


