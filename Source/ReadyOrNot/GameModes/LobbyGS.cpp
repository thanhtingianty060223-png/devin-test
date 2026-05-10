// Copyright Void Interactive, 2023

#include "GameModes/LobbyGS.h"

#include "LobbyGM.h"
#include "Actors/Environment/MissionPortal.h"
#include "Actors/Triggers/LoadoutPortal.h"
#include "Actors/Triggers/LobbyFiringRangeArea.h"
#include "Commander/CommanderProfile.h"
#include "Commander/RosterSelectionWidget.h"

void ALobbyGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ALobbyGS, MissionGradeMusic);
}

void ALobbyGS::BeginPlay()
{
	Super::BeginPlay();

	ALoadoutPortal::OnLoadoutOpened.AddUObject(this, &ALobbyGS::SetLoadoutMusic, 1.0f);
	ALoadoutPortal::OnLoadoutClosed.AddUObject(this, &ALobbyGS::SetLoadoutMusic, 0.0f);
	URosterSelectionWidget::OnRosterSelectionOpened.AddUObject(this, &ALobbyGS::SetCommanderMusic, 1.0f);
	URosterSelectionWidget::OnRosterSelectionClosed.AddUObject(this, &ALobbyGS::SetCommanderMusic, 0.0f);

	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (LobbyGM && LobbyGM->RosterManager)
	{
		const TArray<URosterCharacter*>& RosterCharacters = LobbyGM->RosterManager->GetAllCharacters();
		
		float TotalStress = 0.0f;
		for (URosterCharacter* Character : RosterCharacters)
		{
			TotalStress += Character->StressLevel;
			if (Character->State == ERosterCharacterState::Deceased)
			{
				LobbyMusicMorale = 0.0f;
				break;
			}
		}

		int32 TotalCharacters = RosterCharacters.Num();
		float AverageStress = FMath::Clamp(TotalStress / TotalCharacters, 0.0f, 1.0f);

		LobbyMusicMorale = FMath::RoundToFloat(3.0f - AverageStress * 2.0f);
		ensure(LobbyMusicMorale >= 1.0f && LobbyMusicMorale <= 3.0f);
	}
	else
	{
		int32 RandomLobbyMusicMorale = FMath::RandRange(1, 3);
		LobbyMusicMorale = RandomLobbyMusicMorale;
	}
}

void ALobbyGS::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	StopLobbyMusic();

	ALoadoutPortal::OnLoadoutOpened.RemoveAll(this);
	ALoadoutPortal::OnLoadoutClosed.RemoveAll(this);
	URosterSelectionWidget::OnRosterSelectionOpened.RemoveAll(this);
	URosterSelectionWidget::OnRosterSelectionClosed.RemoveAll(this);
}

void ALobbyGS::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	if (!PlayerController)
		return;

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
	
	if (PlayerCharacter && !LobbyEventInst.Instance)
	{
		PlayLobbyMusic();
	}

	AReadyOrNotPlayerState* PlayerState = PlayerController->GetReadyOrNotPlayerState();
	if (PlayerState && !PlayerState->GetLoadout().IsValid())
	{
		FSavedLoadout Loadout;
		UBpGameplayHelperLib::LoadLoadout(Loadout, "default");
		PlayerState->Server_SetLoadout(Loadout);
		PlayerState->Server_SetLoadout_Implementation(Loadout);
	}
	else if (!bFinishedLoading && PlayerCharacter)
	{
		PlayerController->RemoveLoadingScreen();
		bFinishedLoading = !PlayerController->HasLoadingScreenInViewport();
	}
	
	bool bOurCharacterInShootingRange = ALobbyFiringRangeArea::IsInFiringRange(PlayerController->GetPawn());

	if (PlayerCharacter)
	{
		if (PlayerCharacter->bForceLowReady && bOurCharacterInShootingRange)
		{
			PlayerCharacter->OnLowReadyButtonUp();
		}
		PlayerCharacter->bForceLowReady = !bOurCharacterInShootingRange;
	}  

	if (LobbyEventInst.Instance)
	{
		for (TActorIterator<AMissionPortal> It(GetWorld()); It; ++It)
		{
			bool bMissionSelected = !It->GetFormattedMissionURL().IsEmpty();
			LobbyEventInst.Instance->setParameterByName("LobbyMusicReadyRoom", It->AreAllPlayersInPortal() ? 1.0f : 0.0f);
			LobbyEventInst.Instance->setParameterByName("LobbyMusicMissionSelect", bMissionSelected ? 1.0f : 0.0f);
			break;
		}
	}
}

void ALobbyGS::SetPreviousMissionGrade(const FString& Grade)
{
	MissionGradeMusic = 0.0f;

	if (Grade.StartsWith("S") || Grade.StartsWith("A"))
	{
		MissionGradeMusic = 2.0f;
	}
	else if (Grade.StartsWith("D") || Grade.StartsWith("E") || Grade.StartsWith("F"))
	{
		MissionGradeMusic = 1.0f;
	}
}

void ALobbyGS::PlayLobbyMusic()
{
	LobbyEventInst = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), LobbyMusicEvent, true);
	
	if (LobbyEventInst.Instance)
	{
		float ReturnStingFinal = GetServerWorldTimeSeconds() <= 15.0f ? MissionGradeMusic : 0.0f;
		
		LobbyEventInst.Instance->setParameterByName("LobbyMusicReturnSting", ReturnStingFinal);
		LobbyEventInst.Instance->setParameterByName("LobbyMusicMorale", LobbyMusicMorale);
	}
}

void ALobbyGS::StopLobbyMusic()
{
	UFMODBlueprintStatics::EventInstanceStop(LobbyEventInst);
}

void ALobbyGS::SetLoadoutMusic(float Value)
{
	if (LobbyEventInst.Instance)
		LobbyEventInst.Instance->setParameterByName("LobbyMusicLoadout", Value);
}

void ALobbyGS::SetCommanderMusic(float Value)
{
	if (LobbyEventInst.Instance)
		LobbyEventInst.Instance->setParameterByName("LobbyMusicCommander", Value);
}
