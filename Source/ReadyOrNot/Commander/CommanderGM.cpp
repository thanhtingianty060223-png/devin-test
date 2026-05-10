// Copyright Void Interactive, 2023

#include "CommanderGM.h"

#include "CampaignData.h"
#include "CommanderProfile.h"
#include "MetaGameProfile.h"
#include "RosterManager.h"
#include "Actors/Environment/ExfilPortal.h"
#include "Characters/AI/SWATCharacter.h"
#include "GameModes/CoopGS.h"
#include "Info/ScoringManager.h"
#include "Info/SWATManager.h"
#include "Info/Activities/SuspectCombatActivity.h"
#include "Objectives/DefuseBombThreats.h"

TAutoConsoleVariable<int32> CVarForceCommanderMode(TEXT("a.ForceCommanderMode"), 0, TEXT("Force commander mode on with a debug save"));

ACommanderGM::ACommanderGM()
{
	ExfilPortalClass = AExfilPortal::StaticClass();
}

void ACommanderGM::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	
	if (UGameplayStatics::HasOption(Options, "save"))
	{
		FString CommanderSaveSlot = UGameplayStatics::ParseOption(Options, "save");
		CommanderProfile = UCommanderProfile::LoadProfile(CommanderSaveSlot);
	}

#if WITH_EDITOR
	if (CVarForceCommanderMode.GetValueOnAnyThread() != 0)
	{
		CommanderProfile = UCommanderProfile::GetDebugProfile();
	}
#endif

	if (ensureAlways(CommanderProfile))
	{
		RosterManager = NewObject<URosterManager>();
		RosterManager->LoadFromProfile(CommanderProfile);
	}
}

bool ACommanderGM::AreAllPlayersDead()
{
	return Super::Super::AreAllPlayersDead(); // ignore swat ai
}

void ACommanderGM::ReturnToStation()
{
	if (!GetWorld())
		return;
	
	if (!ensure(CommanderProfile))
		return;

	FString LobbyLevel = GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
	FString GameMode = "?game=lobby";
	FString CommanderSave = "?save=" + CommanderProfile->GetSlot();
	FString Grade = "?grade=" + AScoringManager::Get()->CalculateGradeLetterFromPlayerScore();
	
	FString URL = LobbyLevel + GameMode + CommanderSave + Grade;
	ProcessServerTravel(URL, true);
}

void ACommanderGM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ACommanderGM::BeginPlay()
{
	Super::BeginPlay();
	
	if (ensureAlways(RosterManager))
	{
		RosterManager->StartMission();
	}

	AudioStartTime = GetWorld() ? GetWorld()->GetAudioTimeSeconds() : 0.0f;

	AController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (!ensureAlways(IsValid(Controller)))
		return;
	
	AActor* StartPoint = FindPlayerStart(Controller);
	if (!ensureAlways(IsValid(StartPoint)))
	{
		return;
	}
	
	AExfilPortal* ExfilPortal = GetWorld()->SpawnActor<AExfilPortal>(ExfilPortalClass, StartPoint->GetTransform());
}

void ACommanderGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Save the roster once we exit, potentially for the second time if we won the mission
	if (RosterManager && CommanderProfile)
	{
		RosterManager->SaveToProfile(CommanderProfile);
	}
	
	if (CommanderProfile)
	{
		float PlayTime = FMath::Max(0.0f, GetWorld()->GetAudioTimeSeconds() - AudioStartTime);
		
		CommanderProfile->bReturningFromMission = true;
		CommanderProfile->TotalPlaytime += FTimespan::FromSeconds(PlayTime);
		CommanderProfile->SaveProfile();
	}
}

void ACommanderGM::StartMatch()
{
	Super::StartMatch();
}

void ACommanderGM::OnMissionCompleted()
{
	if (!GetWorld())
		return;
	
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	MapName = MapName.ToLower();
	
	MapName.ReplaceInline(TEXT("_BarricadedSuspects"), TEXT(""));
	MapName.ReplaceInline(TEXT("_ActiveShooter"), TEXT(""));
	MapName.ReplaceInline(TEXT("_BombThreat"), TEXT(""));
	MapName.ReplaceInline(TEXT("_HostageRescue"), TEXT(""));
	MapName.ReplaceInline(TEXT("_Raid"), TEXT(""));
	
	if (ensure(CommanderProfile))
	{
		// Ensure roster state is up to date just in case user quits out/crashes immediately after mission win
		if (ensure(RosterManager))
		{
			RosterManager->CompleteMission(MapName);
			RosterManager->SaveToProfile(CommanderProfile);
		}
		
		// Only save to completed levels if they exist in the campaign
		if (CommanderProfile->Campaign && CommanderProfile->Campaign->Levels.Contains(MapName))
			CommanderProfile->CompletedLevels.AddUnique(MapName);

		ACoopGS* CoopGS = GetGameState<ACoopGS>();
		if (CoopGS)
		{
			TSet<FName> LevelProgressionTags = CoopGS->GetLevelProgressionTags(AScoringManager::Get()->GetFinalGradePercentage());
			CommanderProfile->ProgressionTags.Append(LevelProgressionTags);
		}
		
		CommanderProfile->bReturningFromMission = true;
		CommanderProfile->SaveProfile();
	}

	// Do this after so when we check progression the mission completion has been saved
	Super::OnMissionCompleted();
}

void ACommanderGM::CheckProgression(TSet<FName>& InProgressionTags)
{
	Super::CheckProgression(InProgressionTags);

	CheckFlawlessIronman(InProgressionTags);
}

void ACommanderGM::CheckFlawlessIronman(TSet<FName>& InProgressionTags)
{
	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	if (!CampaignData || !CommanderProfile || !RosterManager)
		return;

	if (!CommanderProfile->bIronmanMode)
		return;
	
	for (const FString& Level : CampaignData->Levels)
	{
		if (!CommanderProfile->CompletedLevels.Contains(Level))
			return;
	}

	bool bLostAnyOfficer = RosterManager->GetPreviousCharacters().Num() > 0;
	if (!bLostAnyOfficer)
		InProgressionTags.Add("ironman_flawless");
}

void ACommanderGM::FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::FriendlyAIKilled(InstigatorCharacter, KilledCharacter);
	
	if (!RosterManager || !InstigatorCharacter)
		return;
	
	ASWATCharacter* SWATCharacter = Cast<ASWATCharacter>(KilledCharacter);
	if (!SWATCharacter)
		return;

	URosterCharacter* RosterCharacter = SWATCharacter->GetRosterCharacter();
	if (!RosterCharacter)
		return;

	bool bForceIncapacitation = InstigatorCharacter->IsLocalPlayer();
	RosterManager->OnCharacterKilled(RosterCharacter, bForceIncapacitation);
}

void ACommanderGM::CreateMatchEndWidgets(AReadyOrNotPlayerController* PlayerController)
{
	if (RosterManager)
	{
		RosterManager->PreReview();
	}
	
	if (!CommanderProfile || !CommanderProfile->bIronmanMode)
	{
		Super::CreateMatchEndWidgets(PlayerController);
		return;
	}
	
	if (AreAllPlayersDead() && PlayerController->IsLocalController())
	{
		CommanderProfile->DeleteProfile();
		CommanderProfile = nullptr;
		
		PlayerController->Client_ClearHUDWidgets();
		PlayerController->Client_CreateWidget("IronManGameOver");

		return;
	}
	
	Super::CreateMatchEndWidgets(PlayerController);
}

void ACommanderGM::PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::PlayerKilled(InstigatorCharacter, KilledCharacter);

	if (!RosterManager || !InstigatorCharacter)
		return;

	RosterManager->OnPlayerKilled(InstigatorCharacter);
}

void ACommanderGM::AIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::AIKilled(InstigatorCharacter, KilledCharacter);

	SuspectKilledOrIncapacitated(KilledCharacter, InstigatorCharacter);
	CheckPlayerGenocide(KilledCharacter, InstigatorCharacter);
}

void ACommanderGM::AIIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::AIIncapacitated(IncapacitatedCharacter, InstigatorCharacter);
	
	SuspectKilledOrIncapacitated(IncapacitatedCharacter, InstigatorCharacter);
	CheckPlayerGenocide(IncapacitatedCharacter, InstigatorCharacter);
}

void ACommanderGM::SuspectKilledOrIncapacitated(AReadyOrNotCharacter* KilledCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!RosterManager || !InstigatorCharacter)
		return;

	if (!InstigatorCharacter->IsOnSWATTeam())
		return;
	
	ASWATCharacter* SWATCharacter = Cast<ASWATCharacter>(InstigatorCharacter);
	URosterCharacter* RosterCharacter = SWATCharacter ? SWATCharacter->GetRosterCharacter() : nullptr;
	
	RosterManager->OnSuspectKilled(RosterCharacter, KilledCharacter->IsCivilian());
}

void ACommanderGM::CheckPlayerGenocide(AReadyOrNotCharacter* KilledCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!RosterManager || !KilledCharacter || !InstigatorCharacter)
		return;

	if (!InstigatorCharacter->IsLocalPlayer())
		return;

	if (KilledCharacter->IsCivilian() || KilledCharacter->IsArrested() || KilledCharacter->IsSurrenderedFor(5.0f))
	{
		RosterManager->OnFriendlyOrSurrenderedKilled();
	}
}

void ACommanderGM::AIArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::AIArrested(ArrestedCharacter, InstigatorCharacter);

	if (!RosterManager || !ArrestedCharacter)
		return;

	if (ArrestedCharacter->IsDeadOrUnconscious() || ArrestedCharacter->IsIncapacitated())
		return;
	
	ASWATCharacter* SWATCharacter = Cast<ASWATCharacter>(InstigatorCharacter);
	URosterCharacter* RosterCharacter = SWATCharacter ? SWATCharacter->GetRosterCharacter() : nullptr;

	RosterManager->OnSuspectArrested(RosterCharacter, ArrestedCharacter->IsCivilian());
}

void ACommanderGM::SetupOfficerCustomization(ASWATCharacter* Character, FSavedCustomization& OutCustomization)
{
	Super::SetupOfficerCustomization(Character, OutCustomization);
	
	if (!Character || !RosterManager)
		return;
	
	ERosterSquadPosition RosterSquadPosition = ERosterSquadPosition::Unassigned;
	switch (Character->GetSquadPosition())
	{
	case ESquadPosition::SP_Alpha: RosterSquadPosition = ERosterSquadPosition::BlueOne; break;
	case ESquadPosition::SP_Beta: RosterSquadPosition = ERosterSquadPosition::BlueTwo; break;
	case ESquadPosition::SP_Charlie: RosterSquadPosition = ERosterSquadPosition::RedOne; break;
	case ESquadPosition::SP_Delta: RosterSquadPosition = ERosterSquadPosition::RedTwo; break;
	default: return;
	}

	URosterCharacter* RosterCharacter = RosterManager->GetSquadCharacter(RosterSquadPosition, true);
	Character->SetRosterCharacter(RosterCharacter);

	if (RosterCharacter)
	{
		OutCustomization.Character = RosterCharacter->Character;
		OutCustomization.Voice = RosterCharacter->Voice;
	}
}

void ACommanderGM::ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters)
{
	Super::ExfiltrateMission(ExfilCharacters);

	if (!RosterManager)
		return;

	USWATManager* SWATManager = USWATManager::Get(GetWorld());
	if (!IsValid(SWATManager) || !SWATManager->SwatAI.Num())
		return;

	const bool bHasActiveBombThreat = HasActiveBombThreat();
	bool bIsActiveShooter = HasActiveShooter();
	
	RosterManager->OnExfiltratedMission(GetWorld(), ExfilCharacters, bHasActiveBombThreat, bIsActiveShooter);

	UMetaGameProfile* MetaGameProfile = UMetaGameProfile::LoadProfile();
	if (ensureAlways(MetaGameProfile))
	{
		MetaGameProfile->AddExfilData(FExfiltrationData(true, bHasActiveBombThreat, bIsActiveShooter));
	}
}

bool ACommanderGM::HasActiveBombThreat() const
{
	for (TActorIterator<ADefuseBombThreats> It(GetWorld()); It; ++It)
	{
		ADefuseBombThreats* Threat = *It;
		if (!IsValid(Threat))
			continue;

		if (!Threat->IsObjectiveCompleted())
		{
			return true;
		}
	}

	return false;
}

bool ACommanderGM::HasActiveShooter() const
{
	AReadyOrNotGameState* ReadyOrNotGameState = GetGameState<AReadyOrNotGameState>();
	if (!IsValid(ReadyOrNotGameState))
		return false;
	
	for (ACyberneticCharacter* CyberneticCharacter : ReadyOrNotGameState->AllAICharacters)
	{
		if (!IsValid(CyberneticCharacter) || !CyberneticCharacter->IsSuspect() || CyberneticCharacter->IsDeadOrUnconscious() || CyberneticCharacter->IsIncapacitated())
			continue;

		// This character is an alive suspect, check if they have an activeshooter archetype
		if (!IsValid(CyberneticCharacter->Archetype))
			continue;

		FString ArchetypeName = CyberneticCharacter->Archetype->Name;
		ArchetypeName.RemoveSpacesInline();
		if (CyberneticCharacter->Archetype->Name.Contains("ActiveShooter"))
			return true;
	}
	
	return false;
}

