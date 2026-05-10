// Copyright Void Interactive, 2023

#include "StationSubLevelController.h"

#include "Commander/CampaignData.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "GameModes/LobbyGM.h"
#include "Misc/UObjectToken.h"
#include "Subsystems/AchievementSubsystem.h"

TAutoConsoleVariable<FString> CVarOverridePreviousLevel(TEXT("a.RonOverridePreviousLevel"), TEXT(""), TEXT("Override previous level for station sub level controllers"));

AStationSubLevelController::AStationSubLevelController()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_SubLevel.T_SubLevel'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AStationSubLevelController::BeginPlay()
{
	Super::BeginPlay();
	
	// If we're in multiplayer, check if this a multiplayer only sub level and load if so
	if (GetWorld()->GetNetDriver())
	{
		if (bIsMultiplayerOnly)
		{
			LoadSubLevel();
		}
		return;
	}

	// Ensure multiplayer only levels do not get loaded from here on
	if (bIsMultiplayerOnly)
		return;

	// Effectively a HasAuthority check
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM)
		return;
	
	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	
	int32 CurrentCampaignIndex = GetCurrentCampaignIndex(CampaignData);
	int32 EnableLevelIndex = CampaignData->Levels.IndexOfByKey(EnableAfterCompleting.ToString());
	int32 DisableLevelIndex = CampaignData->Levels.IndexOfByKey(DisableAfterCompleting.ToString());

	// If we haven't completed the required level, don't load
	if (CurrentCampaignIndex <= EnableLevelIndex)
		return;

	// If we have passed the required level to disable, don't load
	if (CurrentCampaignIndex > DisableLevelIndex && DisableLevelIndex != INDEX_NONE)
		return;

	LoadSubLevel();
}

void AStationSubLevelController::LoadSubLevel()
{
	// Needs to be called once everytime the station is loaded to recover from possible Steam failures
	UAchievementStatics::RetrySteamStats(GetWorld());

	// Find the existing level streaming for the desired level
	ULevelStreaming* LevelStreaming = UGameplayStatics::GetStreamingLevel(GetWorld(), LevelToLoad);
	if (!LevelStreaming)
	{
#if WITH_EDITOR
		FMessageLog MessageLog(FName("PIE"));
		TSharedRef<FTokenizedMessage> Message = MessageLog.Error();
		FString ErrorString = FString::Printf(TEXT("Could not find streaming level '%s' for sublevel controller"), *LevelToLoad.ToString());
		Message->AddToken(FTextToken::Create(FText::FromString(ErrorString)));
		Message->AddToken(FUObjectToken::Create(this));
#endif
		
		UE_LOG(LogReadyOrNot, Warning, TEXT("Could not find streaming level '%s' for sublevel controller '%s'"), *LevelToLoad.ToString(), *GetName());
		return;
	}

	// Load level if found
	UE_LOG(LogReadyOrNot, Log, TEXT("Loading streaming level '%s' for sub level controller '%s'"), *LevelToLoad.ToString(), *GetName());
	
	LevelStreaming->bShouldBlockOnLoad = true;
	LevelStreaming->bShouldBlockOnUnload = true;
	LevelStreaming->SetShouldBeLoaded(true);
	LevelStreaming->SetShouldBeVisible(true);
}

int32 AStationSubLevelController::GetCurrentCampaignIndex(const UCampaignData* CampaignData) const
{
	if (!GetWorld() || !CampaignData)
		return 0;

	// Multiplayer lobbies should be act as if campaign was fully completed
	if (GetWorld()->GetNetDriver())
		return CampaignData->Levels.Num();
	
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM)
		return 0;
	
	TArray<FString> CompletedLevels;
	if (LobbyGM->CommanderProfile)
	{
		CompletedLevels = LobbyGM->CommanderProfile->CompletedLevels;
	}
	else
	{
		UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
		if (MetaGameProfile)
			CompletedLevels = MetaGameProfile->GetCompletedLevels();
	}

	FString CompletedOverride = CVarOverridePreviousLevel.GetValueOnAnyThread();
	if (!CompletedOverride.IsEmpty())
		CompletedLevels.Add(CompletedOverride);
	
	for (int32 i = CompletedLevels.Num() - 1; i >= 0; i--)
	{
		const FString& CompletedLevel = CompletedLevels[i];
		int32 CampaignIndex = CampaignData->Levels.IndexOfByKey(CompletedLevel);

		if (CampaignIndex == INDEX_NONE)
			continue;

		return CampaignIndex + 1;
	}
	return 0;
}

TArray<FString> AStationSubLevelController::GetStreamingLevelOptions() const
{
	if (!GetWorld())
		return {};

	TArray<FString> AvailableLevels;
	const TArray<ULevelStreaming*>& StreamingLevels = GetWorld()->GetStreamingLevels();
	for (ULevelStreaming* LevelStreaming : StreamingLevels)
	{
		if (!IsValid(LevelStreaming))
			continue;

		if (LevelStreaming->ShouldBeAlwaysLoaded())
			continue;
		
		AvailableLevels.Add(FPackageName::GetShortName(LevelStreaming->GetWorldAssetPackageName()));
	}
	return AvailableLevels;
}

TArray<FString> AStationSubLevelController::GetCampaignLevelOptions() const
{
	TArray<FString> LevelOptions = { "none" };
	
	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	if (CampaignData)
	{
		LevelOptions.Append(CampaignData->Levels);
	}

	return LevelOptions;
}
