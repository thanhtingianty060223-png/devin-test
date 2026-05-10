// Copyright Void Interactive, 2019
#pragma once

#include "MapInfoUtilityWidget.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"

#include "Kismet/GameplayStatics.h"
#include "ReadyOrNot/lib/BpGameplayHelperLib.h"
#include "ReadyOrNot/Actors/Gameplay/AISpawn.h"
#include "ReadyOrNot/ReadyOrNotGameMode.h"
#include "ReadyOrNot/Objectives/Objective.h"
#include "ReadyOrNot/Actors/WorldDataGenerator.h"

#include "Engine.h"

#include "Metagame/Challenge.h"

FString UMapInfoUtilityWidget::GetDebugString()
{
	return "Unknown Data";
}

FString UMapInfoUtilityWidget::GetLevelDetails()
{
	FString d = "";
	FLevelDataLookupTable ld = GetLevelData();
	d += ld.FriendlyLevelName.ToString();
	d += "\n	" + ld.Description.ToString();
	d += "\n	" + ld.LevelNickname.ToString();
	d += "\n		Supported Game Modes";
	for (TSoftClassPtr<AReadyOrNotGameMode> gm : ld.SupportedGameModes)
	{
		if (gm.LoadSynchronous())
		{
			d += "\n			" + gm.Get()->GetPathName();
		}
	}
	d += "\n		Objectives";
	for (TSoftClassPtr<AObjective> obj : ld.Objectives)
	{
		if (obj.LoadSynchronous())
		{
			d += "\n			" + obj.Get()->GetPathName();
		}
	}
	d += "\n		Challenges";
	for (TSoftClassPtr<UChallenge> chl : ld.Challenges)
	{
		if (chl.LoadSynchronous())
		{
			d += "\n			" + chl.Get()->GetPathName();
		}
	}
	
	return d;
}

UTexture2D* UMapInfoUtilityWidget::GetLevelForegroundImage()
{
	return GetLevelData().LoadingScreen.Foreground.LoadSynchronous();
}

UTexture2D* UMapInfoUtilityWidget::GetLevelBackgroundImage()
{
	return GetLevelData().LoadingScreen.Background.LoadSynchronous();
}

FString UMapInfoUtilityWidget::GetAITableNames()
{
	FWorldContext WorldContext = GEditor->GetEditorWorldContext();
	FString tn = "";
	FLevelDataLookupTable ld = GetLevelData();
	for (FAIRoster ai : ld.AISpawnRosters)
	{
		tn += "\nSpawn Group: " + ai.SpawnGroup.ToString();
		for (FAIDataPick pick : ai.SpawnAI)
		{
			tn += "\n	" + pick.AILookupTag + "	" + FString::SanitizeFloat(pick.Chance) + "%";
		}
		tn += "\n\n	Spawners\n";
		for (TActorIterator<AAISpawn> It(WorldContext.World()); It; ++It)
		{
			AAISpawn* spawner = *It;
		}
		tn += "\n\n";
	}
	return tn;
}

FLevelDataLookupTable UMapInfoUtilityWidget::GetLevelData()
{
	UDataTable* dt = UBpGameplayHelperLib::GetLevelLookupDataTable();
	if (dt)
	{
		static const FString ContextString(TEXT("Level Script Level Lookup"));
		FWorldContext WorldContext = GEditor->GetEditorWorldContext();
		if (WorldContext.World())
		{
			FLevelDataLookupTable* ld = dt->FindRow<FLevelDataLookupTable>(*UGameplayStatics::GetCurrentLevelName(WorldContext.World()), ContextString);
			//ensure(ld);
			if (ld)
			{
				return *ld;
			}
		}
	}
	return FLevelDataLookupTable();
}

void UMapInfoUtilityWidget::RegenWorld()
{
	AWorldDataGenerator* WorldGen = nullptr;
	for (TActorIterator<AWorldDataGenerator>It(GetWorld()); It; ++It)
	{
		WorldGen = *It;
		break;
	}

	if (!WorldGen)
	{
		WorldGen = GetWorld()->SpawnActor<AWorldDataGenerator>(AWorldDataGenerator::StaticClass());
		WorldGen->GenerateWorld();
	}
	else
	{
		WorldGen->GenerateWorld();
	}
}
