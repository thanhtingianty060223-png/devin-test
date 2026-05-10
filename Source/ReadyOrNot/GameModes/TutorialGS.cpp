// Void Interactive, 2020


#include "TutorialGS.h"


#include "ReadyOrNotGameMode.h"
#include "HUD/Widgets/SwatCommandWidget.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

const FString SPAWN_AI_POLICE = "a.RonNoSpawnSWAT 0";
const FString SPAWN_NO_AI_POLICE = "a.RonNoSpawnSWAT 1";

// Sets default values
ATutorialGS::ATutorialGS()
{
	PrimaryActorTick.bCanEverTick = true;
	TutorialMenuLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Menu.RoN_EATut_Menu'");

	// Author: Sujay
	// I know this is a bad fix, but setting the references on blueprint made it nullable, until loaded.
	// This sets the reference right away   
	ShootingRangeLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Range.RoN_EATut_Range'");
	KillHouseLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Killhouse.RoN_EATut_Killhouse'");
	BasicControlsLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_BasicControls.RoN_EATut_BasicControls'");
	MirrorgunLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Mirrorgun.RoN_EATut_Mirrorgun'");
	StackUpLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_StackUp.RoN_EATut_StackUp'");
	ArrestLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Arrest.RoN_EATut_Arrest'");
	GrenadesLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Grenades.RoN_EATut_Grenades'");
	MovementLevel = FSoftObjectPath("World'/Game/ReadyOrNot/Level/RoN_EATut_Core/Levels/RoN_EATut_Movement.RoN_EATut_Movement'");
}

void ATutorialGS::BeginPlay()
{
	Super::BeginPlay();
}

void ATutorialGS::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if(CurrentTutorialStreamedLevel)
	{
		GetWorld()->RemoveStreamingLevel(CurrentTutorialStreamedLevel);
	}

	for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
	{
		ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
		if (Level->GetWorldAssetPackageName().Contains("RoN_EATut_Core") == false)
		{
			Level->SetShouldBeVisible(false);
			GetWorld()->RemoveStreamingLevel(Level);
		}
	}

	FTutorialMissionData TutorialData;
	TutorialData.MissionType = ETutorialMissionType::ETM_None;
	TutorialData.bSpawnSWATTeam = true;
	TutorialData.SpawnTag = "";
	SetCurrentTutorialData(TutorialData);
}

void ATutorialGS::OnPostUpdateSwatCommands(USwatCommandWidget* Widget, TArray<FSwatCommand>& SwatCommands)
{
	for (int32 i = 0; i < SwatCommands.Num(); i++)
	{
		SwatCommands[i].bEnabled = true;
	}
}

void ATutorialGS::SetCurrentTutorialData(FTutorialMissionData TutorialData)
{
	CurrentTutorialData = TutorialData;

	SetSpawnPoliceState(TutorialData.bSpawnSWATTeam);

	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);

	if(!GameMode)
		return;
	
	if(AReadyOrNotGameMode* RONGameMode = Cast<AReadyOrNotGameMode>(GameMode))
	{
		RONGameMode->PlayerSpawnTag = TutorialData.SpawnTag;
	}
}

TSoftObjectPtr<UWorld> ATutorialGS::GetCurrentTutorialStreamedLevel()
{
	switch(CurrentTutorialData.MissionType)
	{
		case ETutorialMissionType::ETM_ShootingRange:
			return ShootingRangeLevel;
		case ETutorialMissionType::ETM_KillHouse:
			return KillHouseLevel;
		case ETutorialMissionType::ETM_BasicControls:
			return BasicControlsLevel;
		case ETutorialMissionType::ETM_Mirrorgun:
			return MirrorgunLevel;
		case ETutorialMissionType::ETM_StackUp:
			return StackUpLevel;
		case ETutorialMissionType::ETM_Arrest:
			return ArrestLevel;
		case ETutorialMissionType::ETM_Grenades:
			return GrenadesLevel;
		case ETutorialMissionType::ETM_Movement:
			return MovementLevel; 

	}
	
	return nullptr;
}

bool ATutorialGS::TryLoadMenuTutorialLevel()
{
	if(!TutorialMenuStreamedLevel)
	{
		// Remove any existing sub premission planning levels as we now load it directly instead of via the levels window
		for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
		{
			ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
			if (Level->GetWorldAssetPackageName().Contains("SubPreMissionPlanning"))
			{
				Level->SetShouldBeVisible(false);
				GetWorld()->RemoveStreamingLevel(Level);
			}
		}
		TutorialMenuStreamedLevel = NewObject<ULevelStreaming>(GetWorld(), ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags);
		TutorialMenuStreamedLevel->SetWorldAsset(TutorialMenuLevel);
		GetWorld()->AddStreamingLevel(TutorialMenuStreamedLevel);
	}
	else
	{
#if WITH_EDITOR
		// Hack so we don't try load the level on the client in PIE (which doesn't work) should still allow the use of the menu etc
		// Also tutorial should not be networked, but this exists to deal with accidents
		if (GetWorld()->WorldType == EWorldType::PIE)
		{
			if (!HasAuthority())
			{
				return true;
			}
		}
#endif

		if(TutorialMenuStreamedLevel->GetCurrentState() != ULevelStreaming::ECurrentState::LoadedVisible)
		{
			TutorialMenuStreamedLevel->SetShouldBeVisible(true);
			TutorialMenuStreamedLevel->SetShouldBeLoaded(true);
		}
		else if (TutorialMenuStreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::LoadedVisible)
		{
			if (TutorialMenuStreamedLevel->GetLoadedLevel())
			{
				TutorialMenuStreamedLevel->GetLoadedLevel()->bClientOnlyVisible = true;
				return true;
			}
		}
	}

	return false;
}

void ATutorialGS::UnloadStreamedLevel(ULevelStreaming* StreamedLevel)
{
	if(StreamedLevel->IsLevelVisible() == false || StreamedLevel->HasLoadedLevel() == false)
		return;

	// TODO: Sujay clean up
	// Unload Menu Tutorial
	for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
	{
		ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
		if (Level->GetWorldAssetPackageName().Contains("RoN_EATut_Menu"))
		{
			Level->SetShouldBeVisible(false);
			GetWorld()->RemoveStreamingLevel(Level);
		}
	}

	StreamedLevel->SetShouldBeVisible(false);
	StreamedLevel->SetShouldBeLoaded(false);
}

bool ATutorialGS::TryLoadTutorialMission()
{
	if(CurrentTutorialData.MissionType == ETutorialMissionType::ETM_None)
	{
		UE_LOG(LogReadyOrNot, Warning, TEXT("CurrentTutorialData is None and no level will be loaded"))
		return false;	
	}

	if(!CurrentTutorialStreamedLevel)
	{
		// Remove any existing sub premission planning levels as we now load it directly instead of via the levels window
		for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
		{
			ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
			if (Level->GetWorldAssetPackageName().Contains("SubPreMissionPlanning"))
			{
				Level->SetShouldBeVisible(false);
				GetWorld()->RemoveStreamingLevel(Level);
			}
		}
		CurrentTutorialStreamedLevel = NewObject<ULevelStreaming>(GetWorld(), ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags);
		CurrentTutorialStreamedLevel->SetWorldAsset(GetCurrentTutorialStreamedLevel());
		GetWorld()->AddStreamingLevel(CurrentTutorialStreamedLevel);
	}
	else
	{
#if WITH_EDITOR
		// Hack so we don't try load the level on the client in PIE (which doesn't work) should still allow the use of the menu etc
		// Also tutorial should not be networked, but this exists to deal with accidents
		if (GetWorld()->WorldType == EWorldType::PIE)
		{
			if (!HasAuthority())
			{
				return true;
			}
		}
#endif

		if(CurrentTutorialStreamedLevel->GetCurrentState() != ULevelStreaming::ECurrentState::LoadedVisible)
		{
			CurrentTutorialStreamedLevel->SetShouldBeVisible(true);
			CurrentTutorialStreamedLevel->SetShouldBeLoaded(true);
		}
		else if (CurrentTutorialStreamedLevel->GetCurrentState() == ULevelStreaming::ECurrentState::LoadedVisible)
		{
			if (CurrentTutorialStreamedLevel->GetLoadedLevel())
			{
				CurrentTutorialStreamedLevel->GetLoadedLevel()->bClientOnlyVisible = true;
				return true;
			}
		}
	}

	return false;
}

void ATutorialGS::PreGamePlayingStateLogic()
{
	if(!bHasLoadedTutorialMenuLevel && bFinishedUsingTutorialMenu == false)
	{
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc && pc->IsLocalController() && pc->PlayerCameraManager)
		{
			if (!bHasLoadedTutorialMenuLevel)
			{
				// load all data
				if (!GetWorld()->GetName().Contains("MainMenu"))
				{
					UBpGameplayHelperLib::GetWidgetData();
					UBpGameplayHelperLib::GetPenetrationData();
				}

				if (TryLoadMenuTutorialLevel())
				{
					pc->PlayerCameraManager->StartCameraFade(5.0f, 0.0f, 5.0f, FLinearColor::Black, false, false);
					bHasLoadedTutorialMenuLevel = true;
					pc->RemoveLoadingScreen();
				} else
				{
					if(TutorialMenuStreamedLevel)
					{
						UnloadStreamedLevel(TutorialMenuStreamedLevel);
					}
					pc->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 5.0f, FLinearColor::Black, false, true);
				}	
			}
		}
	}
	else if(bFinishedUsingTutorialMenu && !bHasLeftLoadOut)
	{
		if(!bHasLoadedTutorialMission)
		{
			if (TryLoadTutorialMission())
			{
				bHasLoadedTutorialMission = true;
			}	
		}
		else
		{
			Super::PreGamePlayingStateLogic();	
		}
	}
}

void ATutorialGS::LoadStartupWidgetsAfterLoadingScreen()
{
	if(bFinishedUsingTutorialMenu)
	{
		Super::LoadStartupWidgetsAfterLoadingScreen();
		return;
	}
	
	FString	StartupWidget = "EATutorial";

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* c = *It;
		if (c->Tags.Contains("PreviewCharacter"))
		{
			c->SetReplicates(false);
			c->TearOff();
		}
	}

	if (GetWorld()->GetMapName().Contains("MainMenu"))
	{
		StartupWidget = "Menu";
	}

	if (GetName().Contains("FreeMode"))
	{
		StartupWidget = "";
	}
	
	AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (pc && !pc->bIsReplaySpectator)
	{
		FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData(StartupWidget);
		if (WidgetData.WidgetClass)
		{
			if (MatchState != EMatchState::MS_Playing)
			{
				pc->Client_ClearHUDWidgets_Implementation();
				pc->Client_CreateWidget_Implementation(StartupWidget);
			}
		}
	}
}

void ATutorialGS::SetSpawnPoliceState(bool bSpawnPolice)
{
	APlayerController* PController= UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if(PController)
	{
		PController->ConsoleCommand(bSpawnPolice ? *SPAWN_AI_POLICE : *SPAWN_NO_AI_POLICE, true);
	}
}

