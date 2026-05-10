// Copyright Void Interactive, 2023

#include "ReadyOrNotPlayerState.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"
#include "Characters/ReplayController.h"
#include "Commander/BaseProfile.h"

#include "GameModes/CoopGS.h"
#include "GameModes/LobbyGM.h"
#include "GameModes/VIPEscortGS.h"

#include "Net/UnrealNetwork.h"

#include "lib/CompetitionHelperLib.h"

void AReadyOrNotPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, DeathDamageType, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, DeathTraceHit, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, DeathKiller, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, DeathWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, bDeadToPointDamage, COND_OwnerOnly);

	DOREPLIFETIME(AReadyOrNotPlayerState, LastCharacter);
	//DOREPLIFETIME(AReadyOrNotPlayerState, HealthStatus);
	
	DOREPLIFETIME(AReadyOrNotPlayerState, DamageDealt);
	DOREPLIFETIME(AReadyOrNotPlayerState, DamageReceived);
	DOREPLIFETIME(AReadyOrNotPlayerState, Arrests);
	DOREPLIFETIME(AReadyOrNotPlayerState, ArrestsThisLife);
	DOREPLIFETIME(AReadyOrNotPlayerState, TimesArrested);
	DOREPLIFETIME(AReadyOrNotPlayerState, Objectives);
	DOREPLIFETIME(AReadyOrNotPlayerState, Reports);
	DOREPLIFETIME(AReadyOrNotPlayerState, Evidence);
	DOREPLIFETIME(AReadyOrNotPlayerState, Incapacitations);
	DOREPLIFETIME(AReadyOrNotPlayerState, EvidenceActorsInPossession);

	DOREPLIFETIME(AReadyOrNotPlayerState, Kills);
	DOREPLIFETIME(AReadyOrNotPlayerState, KillsThisLife);
	DOREPLIFETIME(AReadyOrNotPlayerState, TeamKills);
	DOREPLIFETIME(AReadyOrNotPlayerState, Team);
	DOREPLIFETIME(AReadyOrNotPlayerState, LastLoadout);
	DOREPLIFETIME(AReadyOrNotPlayerState, Deaths);

	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromKills);
	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromDamage);
	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromArrests);
	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromObjective);

	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromReportingKills);
	DOREPLIFETIME(AReadyOrNotPlayerState, PointsFromReportingArrests);
	
	DOREPLIFETIME(AReadyOrNotPlayerState, bReady);
	DOREPLIFETIME(AReadyOrNotPlayerState, bIsInGame);
	DOREPLIFETIME(AReadyOrNotPlayerState, bIsReplaySpectator);

	DOREPLIFETIME(AReadyOrNotPlayerState, bIsVIP);
	DOREPLIFETIME(AReadyOrNotPlayerState, bWasVIP);

	DOREPLIFETIME(AReadyOrNotPlayerState, TotalYells);
	DOREPLIFETIME(AReadyOrNotPlayerState, NumberOrdersGiven);
	DOREPLIFETIME(AReadyOrNotPlayerState, BulletsFired);
	DOREPLIFETIME(AReadyOrNotPlayerState, BulletsFiredThisLife);
	DOREPLIFETIME(AReadyOrNotPlayerState, BulletsHit);
	DOREPLIFETIME(AReadyOrNotPlayerState, BulletsHitThisLife);
	DOREPLIFETIME(AReadyOrNotPlayerState, GrenadesThrown);
	DOREPLIFETIME(AReadyOrNotPlayerState, Headshots);
	DOREPLIFETIME(AReadyOrNotPlayerState, bSquadLeader);

	DOREPLIFETIME(AReadyOrNotPlayerState, VoiceType);
	DOREPLIFETIME(AReadyOrNotPlayerState, ServerSavedLoadout);
	DOREPLIFETIME(AReadyOrNotPlayerState, PlayerSpawnTag);

	DOREPLIFETIME(AReadyOrNotPlayerState, bHasFinishedLoading);
	DOREPLIFETIME(AReadyOrNotPlayerState, bIsTalking);

	DOREPLIFETIME(AReadyOrNotPlayerState, PlanningPlayerNumber);
	
	DOREPLIFETIME(AReadyOrNotPlayerState, DrawingArray);
	DOREPLIFETIME_CONDITION(AReadyOrNotPlayerState, CurrentDrawing, COND_SkipOwner);

	DOREPLIFETIME(AReadyOrNotPlayerState, Customization);
}

AReadyOrNotPlayerState::AReadyOrNotPlayerState()
{
	NetUpdateFrequency = 1.0f;
	PrimaryActorTick.bCanEverTick = true;
}

void AReadyOrNotPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	// should be done on client only!
	if (IsOwnerOfPlayerState())
	{
		TrySetPreferredTeam();

		if (USteamworksIntegration::IsSteamEnabled())
		{
			if (USteamworksIntegration::IsDLCInstalled(STEAM_DLC_PREORDER_BONUS))
			{
				Server_SendUnlockedDLC(USteamworksIntegration::GetDLCENum(STEAM_DLC_PREORDER_BONUS));
			}
		
			if (USteamworksIntegration::IsDLCInstalled(STEAM_DLC_SUPPORTER))
			{
				Server_SendUnlockedDLC(USteamworksIntegration::GetDLCENum(STEAM_DLC_SUPPORTER));
			}
		}
		
		FSavedLoadout Loadout;
		UBpGameplayHelperLib::LoadLoadout(Loadout, "default");
		Server_SetLoadout(Loadout);
		
		LastLoadout = Loadout;

		UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
		if (Profile)
		{
			FSavedCustomization* PlayerCustomization = Profile->Customizations.Find(EEquippingSwat::ES_None);
			if (PlayerCustomization)
			{
				Server_SetCustomization(*PlayerCustomization);
			}
		}
		
		if (UBpGameplayHelperLib::GetDMOTeamType() != ETeamType::TT_NONE)
		{
			Server_SetTeam(UBpGameplayHelperLib::GetDMOTeamType());
		}

		// Setup the default voice type and send to server
		EVoiceType DefaultVoiceType = EVoiceType::VT_Local;
		UBpGameplayHelperLib::GetVoiceType(DefaultVoiceType);
		
		Server_SetVoiceType(DefaultVoiceType);
		Server_SetVoiceType_Implementation(DefaultVoiceType);
	}
	
// 	// le pirates be wary
// #if UE_BUILD_SHIPPING
// 	if (GetUniqueId().ToString().Contains("-"))
// 	{
// 		FPlatformMisc::RequestExit(true);
// 	}
// #endif
}

void AReadyOrNotPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AReadyOrNotPlayerState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LastScoreTeamUpdate += DeltaSeconds;
	if (LastScoreTeamUpdate > 1.0f)
	{
		UpdateScore();
		TrySetPreferredTeam();
	}

	for (FPlanningDrawing& Drawing : DrawingArray.Items)
	{
		Drawing.Time += DeltaSeconds;
	}
}

void AReadyOrNotPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
	
	if (Cast<AReplayController>(C))
	{
		bIsReplaySpectator = true;
	}
}

void AReadyOrNotPlayerState::Server_SendUnlockedDLC_Implementation(EGameVersionRestriction Dlc)
{
	UnlockedDLC.AddUnique(Dlc);
}

bool AReadyOrNotPlayerState::Server_SendUnlockedDLC_Validate(EGameVersionRestriction Dlc)
{
	return true;
}

bool AReadyOrNotPlayerState::HasEveryoneFinishedLoading(int32& OutTotal, int32& OutLoading, int32& OutLoaded)
{
	for (TActorIterator<AReadyOrNotPlayerState>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		if(!It->bIsReplaySpectator && !It->IsOnlyASpectator() && !It->IsSpectator())
		{
			OutTotal++;
			It->bHasFinishedLoading ? OutLoaded++ : OutLoading++;
		}
	}
	
	return OutTotal == OutLoaded;
}

void AReadyOrNotPlayerState::SetPlayerName(const FString& S)
{
	FString Name = S.Left(30);
	Super::SetPlayerName(Name);	
}

void AReadyOrNotPlayerState::TrySetPreferredTeam()
{
	{
		UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
		if (!gi->TeamUniqueNetId.IsEmpty() || true)
		{
			if (!bSquadTeamAssigned)
			{
				if (UBpGameplayHelperLib::GetLocalRoNPlayerController() && UBpGameplayHelperLib::GetLocalRoNPlayerController()->PlayerState == this)
				{
					for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
					{
						AReadyOrNotPlayerState* t = *It;
						if (t != this)
						{
							if (t->GetUniqueId().IsValid() && !t->GetUniqueId()->ToString().IsEmpty())
							{
								if (t->GetUniqueId().ToString() == gi->TeamUniqueNetId)
								{
									ServerSetJoinedOnSquadLeader();
									V_LOGM(LogReadyOrNot, "Joining player preferred team: %s", *gi->TeamUniqueNetId);
									bSquadTeamAssigned = true;
									SetTeam(t->GetTeam());
								}
							}
						}
					}
				}
			}
		}
	}
}

void AReadyOrNotPlayerState::IncreaseScore(const float Amount)
{
	SetScore(GetScore() + Amount);
}

void AReadyOrNotPlayerState::DecreaseScore(const float Amount)
{
	SetScore(GetScore() - Amount);
}

void AReadyOrNotPlayerState::ServerSetJoinedOnSquadLeader_Implementation()
{
	bJoinedOnSquadLeader = true;
}

bool AReadyOrNotPlayerState::ServerSetJoinedOnSquadLeader_Validate()
{
	return true;
}

bool AReadyOrNotPlayerState::IsVipPlayerState()
{
	AVIPEscortGS* gs = Cast<AVIPEscortGS>(GetWorld()->GetGameState());
	if (gs)
	{
		if (gs->VIPPlayerState == this)
		{
			return true;
		}
	}
	return false;
}

void AReadyOrNotPlayerState::IncrementBulletsFired(class ABaseWeapon* Weapon)
{
	BulletsFired++;
	BulletsFiredThisLife++;
}

void AReadyOrNotPlayerState::ResetBulletsFired()
{
	BulletsFired = 0;
	BulletsFiredThisLife = 0;
}

void AReadyOrNotPlayerState::GetNetworkStatus(float& AvgLag)
{
	// ##UE5UPGRADE## Deprecated
	AvgLag = ExactPing;
}

void AReadyOrNotPlayerState::Notify_PendingChangeTeam_Implementation(ETeamType NewTeamType)
{
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (gs)
	{
		// don't need to nodify of team changes in co-op right now
		if (!gs->bPvPMode)
			return;

		FRChatMessage ChatMessage;
		ChatMessage.TargetPlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		ChatMessage.SenderName = "[SYSTEM]";
		FString TeamNameString = "";
		switch(NewTeamType)
		{
		case ETeamType::TT_NONE:
			break;
		case ETeamType::TT_SERT_RED:
			TeamNameString = "^1Red";
			break;
		case ETeamType::TT_SERT_BLUE:
			TeamNameString = "^4Blue";
			break;
		case ETeamType::TT_SUSPECT:
			TeamNameString = "^2Suspects";
			break;
		case ETeamType::TT_CIVILIAN:
			break;
		case ETeamType::TT_SQUAD:
			break;
		default:
			break;

		}
		ChatMessage.Message = "You will change to " + TeamNameString + "^7 team on your next death.";
		gs->OnChatMessageReceived.Broadcast(ChatMessage);
	}
}

void AReadyOrNotPlayerState::OnRep_UpdateServerSavedLoadout()
{
	OnPlayerLoadoutChanged.Broadcast(LastLoadout);
}

void AReadyOrNotPlayerState::Server_UpdatePlayerSpawnTag_Implementation(const FString& NewTag)
{
	PlayerSpawnTag = NewTag;
}

bool AReadyOrNotPlayerState::IsSquadLeader()
{
	if (!GetWorld())
		return false;

	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (gs)
	{
		if (gs->bPvPMode)
			return false;
	}
	return bSquadLeader;
}

void AReadyOrNotPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);
	
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(NewPlayerState);
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	
	if (ps && gs)
	{
		ps->Server_SetLoadout(LastLoadout);
		ps->Server_SetTeam(Team);
		ps->bSquadLeader = IsSquadLeader();
	}
	if (ps)
	{
		ps->Server_SetCustomization(Customization); // can we just copy customization, and not set?
		ps->PlanningPlayerNumber = PlanningPlayerNumber;
	}
}

int32 AReadyOrNotPlayerState::GetKillCount()
{
	return Kills;
}

int32 AReadyOrNotPlayerState::GetDeathCount()
{
	return Deaths;
}


void AReadyOrNotPlayerState::SetTeam(ETeamType NewTeam)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		Server_SetTeam_Implementation(NewTeam);
	}
	Server_SetTeam(NewTeam);
}

void AReadyOrNotPlayerState::Server_SetTeam_Implementation(ETeamType NewTeam)
{
	if (!bIsInGame)
	{
		Team = NewTeam;
	}
	else
	{
		// code to keep compatibility with existing bluepritn code, 
		//update pending team if already selected as most tihngs probably are reading TeamType not PendingTeamType
		if (PendingTeam == NewTeam && NewTeam == ETeamType::TT_SERT_BLUE)
		{
			NewTeam = ETeamType::TT_SERT_RED;
		}
		else if (PendingTeam == NewTeam && NewTeam == ETeamType::TT_SERT_RED)
		{
			NewTeam = ETeamType::TT_SERT_BLUE;
		}
		PendingTeam = NewTeam;
		Notify_PendingChangeTeam(NewTeam);
	}
}

void AReadyOrNotPlayerState::TrySetPendingTeamAsTeam()
{
	if (PendingTeam != ETeamType::TT_NONE)
	{
		Team = PendingTeam;
		PendingTeam = ETeamType::TT_NONE;
	}
}

ETeamType AReadyOrNotPlayerState::GetTeam()
{
	return Team;
}

ETeamType AReadyOrNotPlayerState::GetPendingTeam()
{
	return PendingTeam;
}

FString AReadyOrNotPlayerState::GetTeamAsString()
{
	const UEnum* EnumPtrTeam = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETeamType"), true);
	return EnumPtrTeam ? EnumPtrTeam->GetDisplayNameTextByIndex((uint8)GetTeam()).ToString() : "Invalid";
}

void AReadyOrNotPlayerState::Server_SetLoadout_Implementation(FSavedLoadout newLoadout)
{	
	LastLoadout = newLoadout;
	ServerSavedLoadout = newLoadout;
	OnRep_UpdateServerSavedLoadout();
}

void AReadyOrNotPlayerState::SetReady(bool bIsReady, FSavedLoadout NewLoadout)
{
	bReady = bIsReady;
	LastLoadout = NewLoadout;
	Server_SetReady(bIsReady, NewLoadout);
#if WITH_EDITOR
	for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
    {
        AReadyOrNotPlayerState* PlayerState = *It;
		if (PlayerState != this)
		{
			PlayerState->ServerSavedLoadout = NewLoadout;
			PlayerState->bReady = bIsReady;
		}
    }
#endif
}

void AReadyOrNotPlayerState::Server_SetReady_Implementation(bool bIsReady, FSavedLoadout NewLoadout)
{
	// TODO: hardcoded for now
	NewLoadout.CharacterType = "SWATJudge";
	
	bReady = bIsReady;

	ServerSavedLoadout = NewLoadout;
	LastLoadout = NewLoadout;
	OnRep_UpdateServerSavedLoadout();
}

FSavedLoadout AReadyOrNotPlayerState::GetLoadout()
{
	return LastLoadout;
}

void AReadyOrNotPlayerState::Server_SetPlayerName_Implementation(FName NewPlayerName)
{
	SetPlayerName(NewPlayerName.ToString());
}

void AReadyOrNotPlayerState::Server_SetIsInGame_Implementation(bool bNewIsInGame)
{
	bIsInGame = bNewIsInGame;
}

EVoiceType AReadyOrNotPlayerState::GetVoiceType()
{
	return VoiceType;
}

void AReadyOrNotPlayerState::SetIsTalking_Implementation(bool bNewTalking)
{
	bIsTalking = bNewTalking;
}

bool AReadyOrNotPlayerState::SetIsTalking_Validate(bool bNewTalking)
{
	return true;
}

bool AReadyOrNotPlayerState::IsTalking() const
{
	return bIsTalking;
}

bool AReadyOrNotPlayerState::IsOwnerOfPlayerState()
{
	if (!GetWorld())
		return false;

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
		return false;

	APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController(GetWorld());
	return IsOwnedBy(PlayerController);
}

void AReadyOrNotPlayerState::Server_SetVoiceType_Implementation(EVoiceType NewVoiceType)
{
	VoiceType = NewVoiceType;
	OnRep_VoiceType();
}

void AReadyOrNotPlayerState::UpdateScore()
{
	AReadyOrNotGameMode* gm = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>();
	if (gm)
	{
		if (GetScore() - LastSentScore > 1.0f)
		{
			FString SteamId = GetUniqueId().ToString();
			FString SteamName = GetPlayerName();
			UCompetitionHelperLib::AddScore(gm->EventID, SteamId, SteamName, GetScore() - LastSentScore);
			LastSentScore = GetScore();
		}
	}
}

void AReadyOrNotPlayerState::Server_StartDrawing_Implementation(int32 Floor, FVector2D StartPoint)
{
	ensure(CurrentDrawing.Points.Num() <= 0);
	if (CurrentDrawing.Points.Num() > 0)
		return;
	
	CurrentDrawing.Floor = Floor;
	CurrentDrawing.Points.Init(StartPoint, 1);
}

void AReadyOrNotPlayerState::Server_UpdateDrawing_Implementation(FVector2D NewPoint)
{
	ensure(CurrentDrawing.Points.Num() + 1 <= MaxDrawingPoints);
	if (CurrentDrawing.Points.Num() + 1 > MaxDrawingPoints)
		return;
	
	CurrentDrawing.Points.Add(NewPoint);
}

void AReadyOrNotPlayerState::Server_FinishDrawing_Implementation()
{
	if (DrawingArray.Items.Num() > MaxDrawings)
		DrawingArray.Items.RemoveAt(0);

	DrawingArray.Items.Add(CurrentDrawing);
	DrawingArray.MarkArrayDirty();
	
	CurrentDrawing = FPlanningDrawing();
}

void AReadyOrNotPlayerState::Server_SetCustomization_Implementation(FSavedCustomization InCustomization)
{
	InCustomization.Sanitize();
	Customization = InCustomization;
	bResetPremissionCustomization = true;
	
	// If we're in the lobby, allow live customization changes
	ALobbyGM* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGM>() : nullptr;
	if (!LobbyGM || !LobbyGM->HasActorBegunPlay())
		return;
	
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(GetPawn());
	if (Character)
	{
		Character->Customization = Customization;
		Character->OnRep_Customization();

		Customization.ApplyCustomizationSkins(Character);
	}
}
