// Copyright Void Interactive, 2021

#include "CoopGS.h"
#include "CoopGM.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderGM.h"
#include "Commander/MetaGameProfile.h"
#include "Info/ScoringManager.h"
#include "Info/TOCManager.h"

void ACoopGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ACoopGS, bMissionSucceded);
	DOREPLIFETIME(ACoopGS, bCrouchingTigerHiddenDragon);
	DOREPLIFETIME(ACoopGS, ServerTimeUntilNextMap);
	DOREPLIFETIME(ACoopGS, bMissionSoftCompleted);
	DOREPLIFETIME(ACoopGS, TeamKills);
	DOREPLIFETIME(ACoopGS, TotalOfficers);
	DOREPLIFETIME(ACoopGS, TotalAIOfficers);
	DOREPLIFETIME(ACoopGS, bAllPlayerCharactesDead);
	DOREPLIFETIME(ACoopGS, SquadPointsRemaining);
	DOREPLIFETIME(ACoopGS, SelectedRedSpawnPoint);
	DOREPLIFETIME(ACoopGS, SelectedBlueSpawnPoint);
	DOREPLIFETIME(ACoopGS, RedSpawnSquadPoints);
	DOREPLIFETIME(ACoopGS, BlueSpawnSquadPoints);
	DOREPLIFETIME(ACoopGS, CurrentDeployables);
	DOREPLIFETIME(ACoopGS, DeployableDepot);
	DOREPLIFETIME(ACoopGS, CurrentPersonnel);
	DOREPLIFETIME(ACoopGS, CurrentUsedPersonnelPoints);
	DOREPLIFETIME(ACoopGS, NumCompleteExtraObjectives);
	DOREPLIFETIME(ACoopGS, NumTotalExtraObjectives);
	DOREPLIFETIME(ACoopGS, DepotLabel);
	DOREPLIFETIME(ACoopGS, DepotNumber);
	DOREPLIFETIME(ACoopGS, DepotCost);
	DOREPLIFETIME(ACoopGS, YesVotes);
	DOREPLIFETIME(ACoopGS, NoVotes);
	DOREPLIFETIME(ACoopGS, Mode);
}

void ACoopGS::BeginPlay()
{
	Super::BeginPlay();

	AddGameStartListener(this);
	
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	
	// Set the squad points.
	SquadPointsRemaining = LevelData.BaseSquadPts;

	// Set the depot.
	if (LevelData.DeployableDepots.Num() >= 1)
	{
		DepotCost = LevelData.DeployableDepots[0].DepotCost;
		SquadPointsRemaining -= DepotCost;
		DepotLabel = LevelData.DeployableDepots[0].DepotLabel;
	}
	else
	{
		//V_LOGM(LogReadyOrNot, "The level does not have any deployable depots in its level data!!");
	}

	// Subtract cost for the default entries.
	RedSpawnSquadPoints = BlueSpawnSquadPoints = LevelData.Spawn_1.PtsCost;
	SquadPointsRemaining -= (RedSpawnSquadPoints + BlueSpawnSquadPoints);
}

void ACoopGS::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

bool ACoopGS::CanChangeSpawn(bool bBlueTeam, ESelectedSpawn NewSpawn)
{
	int32 AvailableSquadPoints = SquadPointsRemaining + ((bBlueTeam) ? BlueSpawnSquadPoints : RedSpawnSquadPoints);
	int32 SquadPointCost;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();

	switch (NewSpawn)
	{
		case ESelectedSpawn::SS_FirstSpawn:
			if (LevelData.Spawn_1.bSpawnDisabled)
			{
				return false;
			}
			SquadPointCost = LevelData.Spawn_1.PtsCost;
			break;

		case ESelectedSpawn::SS_SecondSpawn:
			if (LevelData.Spawn_2.bSpawnDisabled)
			{
				return false;
			}
			SquadPointCost = LevelData.Spawn_2.PtsCost;
			break;

		case ESelectedSpawn::SS_ThirdSpawn:
			if (LevelData.Spawn_3.bSpawnDisabled)
			{
				return false;
			}
			SquadPointCost = LevelData.Spawn_3.PtsCost;
			break;

		case ESelectedSpawn::SS_FourthSpawn:
			if (LevelData.Spawn_4.bSpawnDisabled)
			{
				return false;
			}
			SquadPointCost = LevelData.Spawn_4.PtsCost;
			break;

		default:
			SquadPointCost = 0;
			break;
	}

	if (SquadPointCost > AvailableSquadPoints)
	{
		return false;
	}

	return true;
}

void ACoopGS::OnRep_COOPMode()
{
	FString Key = GetWorld()->GetMapName();
	Key.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	FLevelDataLookupTable LookupTable = UBpGameplayHelperLib::GetMapDetailsFromName(Key);
	if (LookupTable.COOPModesLevelMap.Find(Mode))
	{
		FString MapName = GetWorld()->GetMapName();
		MapName.RemoveFromEnd("_Core");
		switch(Mode)
		{
		case ECOOPMode::CM_None: break;
		case ECOOPMode::CM_BombThreat:
			MapName += "BombThreat";
			break;
		case ECOOPMode::CM_ActiveShooter:
			MapName += "ActiveShooter";
			break;
		case ECOOPMode::CM_HostageRescue:
			MapName += "HostageRescue";
			break;
		case ECOOPMode::CM_BarricadedSuspects:
			MapName += "BarricadedSuspects";
			break;
		case ECOOPMode::CM_Raid: break;
		default: ;
		}
	}
}

FText ACoopGS::GetModeText() const
{
	// Check net driver if we're in a multiplayer game
	if (GetWorld() && GetWorld()->GetNetDriver())
	{
		return NSLOCTEXT("ReadyOrNotGameState", "Coop", "Cooperative");
	}

	// Check if commander mode, only works in sp
	if (GetWorld() && GetWorld()->GetAuthGameMode<ACommanderGM>())
	{
		return NSLOCTEXT("ReadyOrNotGameState", "Commander", "Commander Mode");
	}
	
	// Otherwise this is practice mode
	return NSLOCTEXT("ReadyOrNotGameState", "Practice", "Practice Mode");
}

bool ACoopGS::IsDeployableEnabled(int32 DeployableNumber)
{
	return !!(CurrentDeployables & (1 << DeployableNumber));
}

TArray<int32> ACoopGS::GetEnabledDeployables()
{
	TArray<int32> ReturnValue;
	const FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
	for (int32 i = 0; i < LevelData.Deployables.Num(); i++)
	{
		if ((CurrentDeployables & (1 << i)) != 0)
		{
			ReturnValue.Add(i);
		}
	}

	return ReturnValue;
}

TArray<int32> ACoopGS::GetUnenabledDeployables()
{
	TArray<int32> ReturnValue;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();

	for (int32 i = 0; i < LevelData.Deployables.Num(); i++)
	{
		if ((CurrentDeployables & (1 << i)) == 0)
		{
			ReturnValue.Add(i);
		}
	}

	return ReturnValue;
}

TArray<FText> ACoopGS::GetEnabledDeployablesShortNames()
{
	TArray<int32> EnabledDeployables = GetEnabledDeployables();
	TArray<FText> ReturnValue;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();

	for (int32 i = 0; i < EnabledDeployables.Num(); i++)
	{
		int32 Deployable = EnabledDeployables[i];
		
		if (Deployable >= LevelData.Deployables.Num())
		{
			continue;
		}

		if (LevelData.Deployables[Deployable].DeployableData == nullptr)
		{
			continue;
		}

		ReturnValue.Add(LevelData.Deployables[Deployable].DeployableData->DeployableShortName);
	}

	return ReturnValue;
}

bool ACoopGS::IsPersonnelEnabled(int32 PersonnelNum)
{
	return (CurrentPersonnel & (1 << PersonnelNum)) != 0;
}

TArray<int32> ACoopGS::GetEnabledPersonnel()
{
	TArray<int32> ReturnValue;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
	for (int32 i = 0; i < LevelData.AllPersonnel.Num(); i++)
	{
		if ((CurrentPersonnel & (1 << i)) != 0)
		{
			ReturnValue.Add(i);
		}
	}

	return ReturnValue;
}

TArray<int32> ACoopGS::GetUnenabledPersonnel()
{
	TArray<int32> ReturnValue;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
	for (int32 i = 0; i < LevelData.AllPersonnel.Num(); i++)
	{
		if ((CurrentPersonnel & (1 << i)) == 0)
		{
			ReturnValue.Add(i);
		}
	}

	return ReturnValue;
}

TArray<int32> ACoopGS::GetUsedPersonnelPoints()
{
	TArray<int32> ReturnValue;
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
	for (int32 i = 0; i < LevelData.AllPersonnelMapPoints.Num(); i++)
	{
		if ((CurrentUsedPersonnelPoints & (1 << i)) != 0)
		{
			ReturnValue.Add(i);
		}
	}

	return ReturnValue;
}

int32 ACoopGS::GetPersonnelForMapNum(int32 MapPointNum)
{
	const int32* Key = PersonnelMapping.FindKey(MapPointNum);

	if (Key != nullptr)
	{
		return *Key;
	}
	return -1;
}

void ACoopGS::Server_SetDeployableDepot_Implementation(AReadyOrNotPlayerController* Controller, int32 NewDepotNum)
{
	if (!Controller->CanSetDepotTo(NewDepotNum))
	{
		return;
	}

	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	if (!LevelData.DeployableDepots.IsValidIndex(NewDepotNum))
	{
		return;
	}

	SquadPointsRemaining += LevelData.DeployableDepots[DepotNumber].DepotCost;
	DepotLabel = LevelData.DeployableDepots[NewDepotNum].DepotLabel;
	DepotCost = LevelData.DeployableDepots[NewDepotNum].DepotCost;
	DepotNumber = NewDepotNum;
	SquadPointsRemaining -= LevelData.DeployableDepots[NewDepotNum].DepotCost;
}

void ACoopGS::OnRep_MapElement()
{
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!pc)
	{
		return;
	}

	pc->Multicast_ForcePlanningRefresh();
}

void ACoopGS::Multicast_BroadcastNewSquadLeader_Implementation(APlayerCharacter* NewSquadLeader)
{
	if (!NewSquadLeader)
	{
		return;
	}

	FString PlayerName = NewSquadLeader->GetPlayerState<AReadyOrNotPlayerState>()->GetPlayerName();
	FRChatMessage ChatMessage;
	ChatMessage.TargetTeam = ETeamType::TT_SQUAD;
	ChatMessage.Message = FText::Format(FTextFormat(PromotedLeaderFormat), FText::AsCultureInvariant(PlayerName)).ToString();

	Multicast_BroadcastChatMessage_Implementation(ChatMessage);
}

void ACoopGS::Multicast_OnMissionEnd_Implementation(const bool bSuccess)
{
	bMissionSucceded = bSuccess;

	// Fail all objectives that are in progress when the mission ends
	for (AObjective* Objective : MissionObjectives)
	{
		if (Objective && Objective->IsObjectiveInProgress())
			Objective->ObjectiveFailed();
	}
}

void ACoopGS::UpdateVotes(int32 Yes, int32 No)
{
	YesVotes = Yes;
	NoVotes	= No;
}

void ACoopGS::OnGameStarted_Implementation()
{	
	if (!HasAuthority())
		return;

	FString TOCLine = "";
	switch (GetDefaultGameMode<ACoopGM>()->GetCOOPMode())
	{
		case ECOOPMode::CM_BombThreat:			TOCLine = "bombthreatstart";		 break;
		case ECOOPMode::CM_ActiveShooter:		TOCLine = "activeshooterstart";		 break;
		case ECOOPMode::CM_HostageRescue:		TOCLine = "hostagerescuestart";		 break;
		case ECOOPMode::CM_BarricadedSuspects:	TOCLine = "barricadedsuspectsstart"; break;
		case ECOOPMode::CM_Raid:				TOCLine = "raidstart";				 break;
		default: break;
	}

	// Mission start audio override
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	if (!LevelData.MissionStartTocVoiceLine.IsNone())
		TOCLine = LevelData.MissionStartTocVoiceLine.ToString();
	
	if (TOCLine.IsEmpty())
		return;

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, GameStartTOCDelay_Handle);
	UReadyOrNotFunctionLibrary::StartTimerForCallback(GameStartTOCDelay_Handle, this, FTimerDelegate::CreateUObject(this, &ACoopGS::StartTOCBriefing, TOCLine), TOCDelay, false);

	if (UFMODBlueprintStatics::EventInstanceIsValid(musicInstance))
	{
		//musicInstance = UFMODBlueprintStatics::PlayEvent2D(this, missionMusic, true);
		UFMODBlueprintStatics::EventInstanceSetParameter(musicInstance, "Music State", 2);
	}
}

FString ACoopGS::GetTOCLineForMap(FString OverrideMapName) const
{
	FString TOCLine = "";
	
	if (OverrideMapName.IsEmpty())
		OverrideMapName = GetWorld()->GetMapName().ToLower();
	else
		OverrideMapName = OverrideMapName.ToLower();
	
	return TOCLine;
}

void ACoopGS::StartTOCBriefing(FString TOCLine)
{
	#if WITH_EDITOR
	FString Key = GetWorld()->GetMapName();
	Key.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	FLevelDataLookupTable LookupTable = UBpGameplayHelperLib::GetMapDetailsFromName(Key);

	// Only play toc manager if the level is part of the official level data table (not a dev test map for example, can get super annoying!!)
	if (LookupTable.COOPModesLevelMap.Find(Mode))
	{
		if (IsValid(TOCManager))
			TOCManager->StartTOCResponse(TOCLine, true, ETOCPriority::ETP_Flush);
	}
	#else
	if (IsValid(TOCManager))
		TOCManager->StartTOCResponse(TOCLine, true, ETOCPriority::ETP_Flush);
	#endif
}

void ACoopGS::OnGameEnded_Implementation()
{
	if(UFMODBlueprintStatics::EventInstanceIsValid(musicInstance))
	{
		int MusicState = 6;
		if(bMissionSucceded)
		{
			MusicState = 5;
		}
		UFMODBlueprintStatics::EventInstanceSetParameter(musicInstance, "Music State", MusicState);
	}
}

bool ACoopGS::PlayerControllerVoted(AReadyOrNotPlayerController* PlayerController, bool bVoteYes)
{
	if (!HasAuthority() || !IsValid(PlayerController))
		return false;
	
	if (MissionEndVoteState != EMissionEndVoteState::VS_InProgress)
		return false;
	
	if (IsNetMode(NM_Standalone))
	{
		// Don't allow no votes in singleplayer
		if (!bVoteYes)
			return false;
	}

	// Update the controllers vote state
	if (bVoteYes)
	{
		// Toggle vote
		PlayerController->MyVoteData.CurrentVoteState = PlayerController->MyVoteData.CurrentVoteState == EVoteState::Yes ? EVoteState::Undecided : EVoteState::Yes;
	}
	else
	{
		// Toggle vote
		PlayerController->MyVoteData.CurrentVoteState = PlayerController->MyVoteData.CurrentVoteState == EVoteState::No ? EVoteState::Undecided : EVoteState::No;
	}
	
	TArray<AReadyOrNotPlayerController*> ControllersAvailableForVote = GetControllersAvailableForVote();
	YesVotes = 0;
	NoVotes = 0;
	for (const AReadyOrNotPlayerController* PC : ControllersAvailableForVote)
	{
		switch (PC->MyVoteData.CurrentVoteState)
		{
			case EVoteState::Undecided:						break;
			case EVoteState::Yes:		YesVotes++;			break;
			case EVoteState::No:		NoVotes++;			break;
			default:										break;
		}
	}
	
	const bool bAllVoted = (YesVotes + NoVotes) >= ControllersAvailableForVote.Num();

	// End vote if majority says so
	// Let int division truncate then add one to get majority
	int32 CountNeededForMajority = (ControllersAvailableForVote.Num() / 2) + 1; 
	bool bMajorityYesVote = YesVotes >= CountNeededForMajority;
	bool bMajorityNoVote = NoVotes >= CountNeededForMajority;

	// If everyone has voted and it's tied, go with finishing the mission
	if (bAllVoted && YesVotes == NoVotes)
		bMajorityYesVote = true;

	if (bMajorityYesVote || bMajorityNoVote)
	{
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			if (AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(*It))
			{
				Controller->EndVote();
				Controller->EndVote_Implementation();
			}
		}

		YesVotes = 0;
		NoVotes = 0;
		MissionEndVoteState = bMajorityYesVote ? EMissionEndVoteState::VS_MajorityYes : EMissionEndVoteState::VS_MajorityNo;
	}

	return true;
}

TSet<FName> ACoopGS::GetLevelProgressionTags(float ScorePercentage)
{
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData(GetWorld());
	if (LevelData.ProgressionTagPrefix.IsNone())
		return {};

	TSet<FName> LevelProgressionTags;
	
	TSet<FString> LetterGrades = AScoringManager::Get()->GetCompletedGradesUpToScore(ScorePercentage);
	for (const FString& LetterGrade : LetterGrades)
	{
		FString CompletionTag = FString::Printf(TEXT("%s_grade_%s"), *LevelData.ProgressionTagPrefix.ToString(), *LetterGrade);
		LevelProgressionTags.Add(FName(CompletionTag));
	}
	return LevelProgressionTags;	
}

void ACoopGS::CheckAllLevelsCompleted(TSet<FName>& InProgressionTags)
{
	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	if (!CampaignData)
		return;

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (!MetaGameProfile)
		return;
	
	bool bAllLevelsCompleted = true;
	for (const FString& Level : CampaignData->Levels)
	{
		// If level has not been completed in either solo or multiplayer, do not unlock
		if (!MetaGameProfile->GetCompletedLevels().Contains(Level) &&
			!MetaGameProfile->GetCompletedMultiplayerLevels().Contains(Level))
		{
			bAllLevelsCompleted = false;
			break;
		}
	}

	if (bAllLevelsCompleted)
		InProgressionTags.Add("completed_all");
}

void ACoopGS::Multicast_GrantProgressionTags_Implementation(float ScorePercentage)
{
	if (HasAuthority())
		return;
	
	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (!ensure(MetaGameProfile))
		return;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	MapName = MapName.ToLower();
	
	MapName.ReplaceInline(TEXT("_BarricadedSuspects"), TEXT(""));
	MapName.ReplaceInline(TEXT("_ActiveShooter"), TEXT(""));
	MapName.ReplaceInline(TEXT("_BombThreat"), TEXT(""));
	MapName.ReplaceInline(TEXT("_HostageRescue"), TEXT(""));
	MapName.ReplaceInline(TEXT("_Raid"), TEXT(""));
	
	MetaGameProfile->AddCompletedMultiplayerLevel(MapName);
	
	TSet<FName> ProgressionTags = GetLevelProgressionTags(ScorePercentage);
	CheckAllLevelsCompleted(ProgressionTags);
	
	MetaGameProfile->AddProgressionTags(ProgressionTags);
	MetaGameProfile->SaveProfile();
}

