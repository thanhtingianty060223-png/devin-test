// Void Interactive, 2017

#include "LoadingScreen.h"

#include "PreMissionPlanning.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"
#include "ShaderPipelineCache.h"


void ULoadingScreen::NativeConstruct()
{
	
	Super::NativeConstruct();	
	UReadyOrNotFunctionLibrary::StopAllAudio(GetWorld());
	UReadyOrNotFunctionLibrary::PauseFMOD(true);

	for (TObjectIterator<UUserWidget> Itr; Itr; ++Itr)
	{
		UUserWidget* LiveWidget = *Itr;
		if (LiveWidget->IsA(ULoadingScreen::StaticClass()))
			continue;

		if (LiveWidget->IsA(UPreMissionPlanning::StaticClass()))
			continue;

		/* If the Widget has no World, Ignore it (It's probably in the Content Browser!) */
		if (!LiveWidget->GetWorld() || LiveWidget->GetWorld() != GetWorld() || !LiveWidget->IsInViewport())
		{
			continue;
		}
		
		LiveWidget->RemoveFromParent();
	}
	FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Fast);
}

void ULoadingScreen::NativeDestruct()
{
	Super::NativeDestruct();
	UReadyOrNotFunctionLibrary::PauseFMOD(false);
	FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Background);
}

void ULoadingScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UHostMigrationManager* HostMigrationManager = UReadyOrNotStatics::GetReadyOrNotGameInstance()->HostMigrationManager;
	if (HostMigrationManager && HostMigrationManager->IsMigratingHost())
	{
		if (!bIsHostMigrationLoadingScreen)
		{
			bIsHostMigrationLoadingScreen = true;
			ExpectedPlayerCount = HostMigrationManager->ExpectedPlayerCount;
			NextHostText = HostMigrationManager->NextHostName + " has been selected as the new host";
		}
	}
#if WITH_EDITOR
	if (GEditor)
	{
		if (GEditor->IsSimulatingInEditor())
		{
			RemoveFromParent();
		}
	}
#endif

	if (TimeSinceCreated > 0.25f && ((bSeamlessTravel && GetWorld() && (GetWorld()->IsInSeamlessTravel() || GetWorld()->GetMapName().Contains(Map))) || !bSeamlessTravel))
	{
		CalculateLoadPercentage(InDeltaTime);
	}

	TimeSinceCreated += InDeltaTime;

	if (FoundRelatingGameMode.IsEmpty() && !Map.Contains("MainMenu"))
	{
		UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
		if (gi->UrlToModeNameMap.Find(Mode))
		{
			FoundRelatingGameMode = gi->UrlToModeNameMap[Mode];
		} else
		{
			gi->GenerateURLMap();
		}
	}
}

void ULoadingScreen::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	// Do Nothing!
}

void ULoadingScreen::SetLoadingScreen(FString InMap, FString InMode, FString InSessionName, bool bIsSeamlessTravel)
{
	// just find the actual core
	InMap.ReplaceInline(TEXT("_BarricadedSuspects"), TEXT(""));
	InMap.ReplaceInline(TEXT("_Raid"), TEXT(""));
	InMap.ReplaceInline(TEXT("_BombThreat"), TEXT(""));
	InMap.ReplaceInline(TEXT("_ActiveShooter"), TEXT(""));
	InMap.ReplaceInline(TEXT("_HostageRescue"), TEXT(""));
	Map = InMap;
	Mode = InMode;
	SessionName = InSessionName;
	bSeamlessTravel = bIsSeamlessTravel;
}

void ULoadingScreen::GetLoadingScreenDetails(FString& OutMap, FString& OutMode, FString& OutSessionName)
{

#ifdef HOST_MIGRATION_ENABLED
	if (bIsHostMigrationLoadingScreen)
	{
		OutMap = "HOST MIGRATION";
		if (GetWorld()->GetGameState())
		{
			OutMode = NextHostText;
		}
		return;
	}
#endif
	
	// return friendly details
	
	OutMap = UBpGameplayHelperLib::GetMapDetailsFromName(Map).FriendlyLevelName.ToString();
	OutMode = FoundRelatingGameMode;

	if (OutMode.Contains("restart"))
	{
		if (GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			OutMode = GetWorld()->GetGameState<AReadyOrNotGameState>()->ModeName.ToString();
		}
	}

	if (Map.Contains("MainMenu"))
	{
		OutMap = "Main Menu";
	}
	
	OutSessionName = SessionName;
}

void ULoadingScreen::CalculateLoadPercentage(const float DeltaTime)
{
	float LoadPercentage = 0.0f;
	bool bIsAsyncLoading = false;
	
	if (UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true))
	{
		ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game/ReadyOrNot/Level"));
		
		TArray<FAssetData> AssetDatas;
		ObjectLibrary->GetAssetDataList(AssetDatas);
		for (int32 i = 0; i < AssetDatas.Num(); ++i)
		{
			FAssetData& AssetData = AssetDatas[i];
	 
			auto name = AssetData.AssetName.ToString();
			if (AssetData.AssetName == *Map)
			{
				const float NewLoadPercentage = GetAsyncLoadPercentage(AssetData.PackageName) / 100.0f;
				if (NewLoadPercentage > 0.0f)
				{
					if (NewLoadPercentage > InterpLoadPercentage)
					{
						LoadPercentage = NewLoadPercentage;
					}

					bIsAsyncLoading = true;
				}

				break;
			}
		}
	}

	//V_LOGM(LogReadyOrNot, "Async Loading %f %f", InterpLoadPercentage, LoadPercentage);
	if ((bIsAsyncLoading ? LoadPercentage : 1.0f) > InterpLoadPercentage)
	{
		InterpLoadPercentage = bIsAsyncLoading ? LoadPercentage : UKismetMathLibrary::FInterpTo(InterpLoadPercentage, 1.0f, DeltaTime, 3.0f);
	}	
}

float ULoadingScreen::GetLoadingPercentage()
{
	return InterpLoadPercentage;
}

FString ULoadingScreen::GetMapName()
{
	return Map;
}

void ULoadingScreen::UpdateTip(UTextBlock* TipBlock)
{
	FText Tip;
	UBpGameplayHelperLib::GetRandomLoadingScreenTip(Tip);
	TipBlock->SetText(Tip);
}
