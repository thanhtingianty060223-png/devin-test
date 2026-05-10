// Copyright Void Interactive, 2023

#include "Actors/Environment/MissionPortal.h"

#include "MissionSelect.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameSession.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Components/InteractableComponent.h"
#include "GameModes/LobbyGM.h"
#include "Metagame/Profile.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"

TAutoConsoleVariable<int32> CVarRonUnlockAllMissions(TEXT("a.RonUnlockAllMissions"), 0, TEXT("Unlock All the missions"));

AMissionPortal::AMissionPortal()
{	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	OverlappingClasses = {APlayerCharacter::StaticClass()};
	
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "SelectMission"));
	InteractableComponent->ActionSlot1.bCondition = true;
	InteractableComponent->SetupAttachment(GetRootComponent());
	MissionURL = "";
	ModeURL = "";
	SelectedEntryPoint = "Spawn_1";

	bReplicates = true;
	
}

void AMissionPortal::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMissionPortal, MissionURL);
	DOREPLIFETIME(AMissionPortal, ModeURL);
	DOREPLIFETIME(AMissionPortal, SelectedEntryPoint);
	DOREPLIFETIME(AMissionPortal, NumReadyPlayers);
	
}

void AMissionPortal::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AMissionSelect> It(GetWorld()); It; ++It)
	{
		MissionSelect = *It;
		break;
	}
	
	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		SelectRandomMission();
	}
	
	TArray<UTextRenderComponent*> TextRenderComponents;
	GetComponents(TextRenderComponents);
	if (TextRenderComponents.Num() > 0)
	{
		WhiteboardText = TextRenderComponents[0];
	}
	
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "MissionPortalStencil", OutActors);
	for (AActor* a : OutActors)
	{
		TArray<UStaticMeshComponent*> OutStatics;
		a->GetComponents(OutStatics);
		CompsToOutline.Append(OutStatics);
		TArray<ULightComponent*> OutLights;
		a->GetComponents(OutLights);
		LightsToEnable.Append(OutLights);
	}

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "MissionPortalStencilSelectedMission", OutActors);
	for (AActor* a : OutActors)
	{
		TArray<UStaticMeshComponent*> OutStatics;
		a->GetComponents(OutStatics);
		CompsToOutlineMissionSelected.Append(OutStatics);
		TArray<ULightComponent*> OutLights;
		a->GetComponents(OutLights);
		LightsToEnableMissionSelected.Append(OutLights);
	}

	Profile = UBpGameplayHelperLib::GetCurrentProfile(GetWorld());

	ALobbyGM* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGM>() : nullptr;
	if (LobbyGM)
	{
		CommanderProfile = LobbyGM->CommanderProfile;
	}
}

void AMissionPortal::PreInitializeComponents()
{
	ALobbyGM* LobbyGM = Cast<ALobbyGM>(GetWorld()->GetAuthGameMode());
	if (!LobbyGM)
		return;

	LobbyGM->OnGenericPlayerInitialization.AddDynamic(this, &AMissionPortal::OnPlayerJoinedLobby);
	FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &AMissionPortal::OnPlayerLeftLobby);
}


void AMissionPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		if (GetFormattedMissionURL().IsEmpty())
		{
			// Set a random mission
			SelectRandomMission();
		}
	}

	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;

	bool bMissionSelectOpen = IsValid(MissionSelect) ? MissionSelect->IsOpen() : false;
	bool bCanOpenMenu = (GetFormattedMissionURL().IsEmpty() || !AreAllPlayersInPortal()) && !bMissionSelectOpen;
	InteractableComponent->ActionSlot1.bCondition = HasAuthority() && !UBpGameplayHelperLib::HasWidgetInViewport("WorldMap") && bCanOpenMenu;
	InteractableComponent->ActionSlot2.bCondition = false;
	
	if (!GetFormattedMissionURL().IsEmpty() && bTimerRunning)
	{
		CurrentTimer -= DeltaSeconds;
		if (!UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
		{
			if (IsValid(MissionSelect))
				MissionSelect->CloseMissionSelect();
			
			AReadyOrNotPlayerController* PlayerController = UReadyOrNotStatics::GetReadyOrNotPlayerController();
			if (PlayerController && PlayerController->WidgetStack.Num() > 0 && PlayerController->WidgetStack.Top() == "WorldMap")
				PlayerController->EscapeMenu();
			
			WhiteboardText->SetText(FText::FromString(FString::Printf(TEXT("%s"), *UBpGameplayHelperLib::ConvertFloatToStringMinutes(FMath::Max(CurrentTimer, 0.0f)))));
		}
		
		if (CurrentTimer <= 0.f)
		{
			if (HasAuthority())
			{
				FString InternalMap, TranslatedMap, Mode;
				UBpGameplayHelperLib::GetFriendlyMapAndModeFromName(GetFormattedMissionURL(), InternalMap, TranslatedMap, Mode);
				const int32 NumPlayers = GetNumPlayers();
				if (NumPlayers > 1)
				{
					FString Message = "Starting Multiplayer Mission: " + Mode + " on " + TranslatedMap + "(" + InternalMap  + ") Players " + FString::FromInt(GetNumPlayers());
					for (TActorIterator<APlayerState>It(GetWorld()); It; ++It)
					{
						if (*It == UReadyOrNotStatics::GetReadyOrNotPlayerController()->GetPlayerState<APlayerState>())
							continue;
						
						Message += "\n" + It->GetPlayerName() + " (" + It->GetUniqueId().ToString() + ")";
					}
					UReadyOrNotBackend::LogMessage(Message);
				}
				
				StartMission();
			}
			Destroy();
		}
	}
	else
	{
		if (!UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
		{
			if (WhiteboardText)
				WhiteboardText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d [%s]"), GetNumberOverlappingPlayers(), GetNumPlayers(), *UBpGameplayHelperLib::GetMapDetailsFromName(GetFormattedMissionURL()).FriendlyLevelName.ToString())));
		}
	}

	if (InteractableComponent->ActionSlot1.bCondition)
	{
		InteractableComponent->IsBeingLookedAt(UReadyOrNotStatics::GetReadyOrNotPlayerController(), InteractableComponent->ShowPromptAtDistance, InteractableComponent->RequiredLookAtPercentage) ? DrawMissionPortalOutline() : DisableMissionPortalOutline();
	}

	MissionURL.IsEmpty() || ModeURL.IsEmpty() ? DisableMissionSelectedPortalOutline() : DrawMissionSelectedPortalOutLine();
}

FString AMissionPortal::GetFormattedMissionURL()
{
	FString InternalMap, TranslatedMap, Mode, TempURL;
	TempURL = MissionURL;
	if (!TempURL.Contains("?game="))
	{
		TempURL = MissionURL + "?game=" + ModeURL;
	}
	UBpGameplayHelperLib::GetFriendlyMapAndModeFromName(TempURL, InternalMap, TranslatedMap, Mode);
	// if we dont have a valid map/mode then set it to empty
	if (TranslatedMap.IsEmpty() || Mode.IsEmpty() || InternalMap.IsEmpty())
	{
		return "";
	}
	
	Mode.ReplaceInline(TEXT(" "), TEXT(""));
	FString MapName = InternalMap;
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	MapName.RemoveFromEnd("_Core");
	MapName += "_";
	MapName += Mode;
	MapName += "_Core";

	if (UReadyOrNotStatics::DoesMapExist(MapName))
	{
		TempURL = MapName + "?game=" + ModeURL;
	}
	
	return TempURL;
}

void AMissionPortal::OnMissionSelected()
{
	// Delegate call for server, client runs this when mission url is replicated below
	OnMissionSelected_Delegate.Broadcast();
}

void AMissionPortal::OnRep_MissionURL()
{
	FString RecvMissionUrl = MissionURL.IsEmpty() ? "None" : MissionURL;
	FString RecvMissionMode = ModeURL.IsEmpty() ? "None" : ModeURL;
	V_LOGM(LogReadyOrNot, "Received Mission URL %s?game=%s from server!", *RecvMissionMode, *RecvMissionMode);

	// New mission selected, get client to update widget
	OnMissionSelected_Delegate.Broadcast();
}

void AMissionPortal::SetSelectedMode(FName InMode)
{
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		It->ModeURL = InMode.ToString();
		FString OutMap, OutMode;
		if (It->MissionURL.Split("?game=", &OutMap, &OutMode))
		{
			It->MissionURL = OutMap + "?game=" + InMode.ToString();
		} 
		else
		{
			It->MissionURL += "?game=" + InMode.ToString();
		}
	}
}

bool AMissionPortal::IsGameModeSelectable(ECOOPMode InMode)
{
	if (InMode == ECOOPMode::CM_BarricadedSuspects)
		return true;
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		FString URL, OutL, OutR;
		if (It->MissionURL.Split("?game=", &OutL, &OutR))
		{
			 URL = OutL;
		}
		else
		{
			URL = It->MissionURL;
		}

		FString MapName = URL;
		MapName.RemoveFromEnd("_Core");
		MapName += "_";
		switch(InMode)
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
		case ECOOPMode::CM_Raid:
			MapName += "Raid";
			break;
		default: ;
		}
		MapName += "_Core";
		
		return UReadyOrNotStatics::DoesMapExist(MapName);
	}
	
	return false;
}

bool AMissionPortal::GetSelectedModeName(FString& OutName)
{
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		for (TSubclassOf<AReadyOrNotGameMode> gm : It->SelectableGameModes)
		{
			if (gm->GetDefaultObject<AReadyOrNotGameMode>()->urlShortName == It->ModeURL)
			{
				OutName = gm->GetDefaultObject<AReadyOrNotGameMode>()->GameStateClass->GetDefaultObject<AReadyOrNotGameState>()->ModeName.ToString();
				return true;
			}
		}
	}

	return false;
}

bool AMissionPortal::GetSelectedMode(FString& OutMode)
{
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		FString OutL, OutR;
		if (It->GetFormattedMissionURL().Split("?game=", &OutL, &OutR))
		{
			OutMode = OutR;
		}
	}
	return false;
}

void AMissionPortal::SetSelectedMission(FString InMissionURL)
{	
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		It->MissionURL = InMissionURL;
	}
}

void AMissionPortal::GetSelectedMission(FString& OutMissionURL)
{
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		FString OutL, OutR;
		if (It->MissionURL.Split("?game=", &OutL, &OutR))
		{
			OutMissionURL = OutL;
		}
	}
}

bool AMissionPortal::IsMissionStarting(bool& bStarting, float& Countdown)
{
	bStarting = false;
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		if (It->bTimerRunning)
		{
			bStarting = true;
			Countdown = It->CurrentTimer;
		}
	}
	return false;
}

bool AMissionPortal::GetPlayersReady(int32& Ready, int32& Total)
{
	for (TActorIterator<AMissionPortal>It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		Ready = It->NumReadyPlayers;
		Total = It->GetNumPlayers();
		return true;
	}
	return false;
}

bool AMissionPortal::IsLevelUnlocked(FString InURL, bool& OutIsUnlocked, float& OutScoreRequired, FString& OutLockedUrl)
{
#if !UE_BUILD_SHIPPING
	if (CVarRonUnlockAllMissions.GetValueOnAnyThread() == 1)
	{
		OutIsUnlocked = true;
		return true;
	}
#endif
	
	OutIsUnlocked = true;
	OutScoreRequired = 0.0f;
	OutLockedUrl = "";
	
	for (TActorIterator<AMissionPortal> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		if (!It->GetWorld())
			continue;
		
		ALobbyGM* LobbyGM = It->GetWorld()->GetAuthGameMode<ALobbyGM>();
		if (!LobbyGM)
			continue;

		FString OutL, OutR;
		if (InURL.Split("?game=", &OutL, &OutR))
		{
			InURL = OutL;
		}
		
		UMetaGameProfile* MetaSaveData = It->MetaGameProfile;
		UCommanderProfile* CommanderProfile = It->CommanderProfile;
		UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();

		const TArray<FString>* CompletedLevels = nullptr;

		// Ignore progression in multiplayer, except for secret commander mode coop
		if (It->GetWorld()->GetNetMode() != NM_Standalone && !CommanderProfile)
		{
			OutIsUnlocked = true;
			return OutIsUnlocked;
		}
		
		// Use the player's completed levels for the current mode
		if (CommanderProfile)
		{
			CompletedLevels = &CommanderProfile->CompletedLevels;
		}
		else if (MetaSaveData)
		{
			CompletedLevels = &MetaSaveData->GetCompletedLevels();
		}

		if (CompletedLevels && CampaignData)
		{
			int32 LevelIndex = CampaignData->Levels.IndexOfByKey(InURL);
			if (LevelIndex > 0)
			{
				FString PreviousURL = CampaignData->Levels[LevelIndex - 1];
				OutIsUnlocked = CompletedLevels->Contains(PreviousURL);
				OutLockedUrl = PreviousURL;
			}
			return OutIsUnlocked;
		}
		
		FLevelDataLookupTable LookupTable = UBpGameplayHelperLib::GetMapDetailsFromName(InURL);
		if (It->Profile)
		{
			if (LookupTable.UnlockRequirements.Score <= 0.0f)
			{
				OutIsUnlocked = true;
				return true;
			}
			
			if (DoesLevelExistInBuild(LookupTable.UnlockRequirements.MapURL))
			{
				if (It->Profile->LevelStats.Contains(LookupTable.UnlockRequirements.MapURL))
				{
					if (It->Profile->LevelStats[LookupTable.UnlockRequirements.MapURL].BestRating < OutScoreRequired)
					{
						OutIsUnlocked = false;
						OutLockedUrl = LookupTable.UnlockRequirements.MapURL;
						OutScoreRequired = LookupTable.UnlockRequirements.Score;
						return true;
					}
				}
				else
				{
					OutIsUnlocked = false;;
					OutLockedUrl = LookupTable.UnlockRequirements.MapURL;
					OutScoreRequired = LookupTable.UnlockRequirements.Score;
					return true;
				}
				OutIsUnlocked = true;
				return true;
			}
		}
		return true;
	}
	return false;
}

bool AMissionPortal::DoesLevelExistInBuild(FString InUrl)
{
	return UReadyOrNotStatics::DoesMapExist(InUrl)
	|| UReadyOrNotStatics::DoesMapExist(InUrl.Replace(TEXT("_Core"), TEXT("_BarricadedSuspects_Core")))
	|| UReadyOrNotStatics::DoesMapExist(InUrl.Replace(TEXT("_Core"), TEXT("_ActiveShooter_Core")))
	|| UReadyOrNotStatics::DoesMapExist(InUrl.Replace(TEXT("_Core"), TEXT("_Raid_Core")))
	|| UReadyOrNotStatics::DoesMapExist(InUrl.Replace(TEXT("_Core"), TEXT("_HostageRescue_Core")))
	|| UReadyOrNotStatics::DoesMapExist(InUrl.Replace(TEXT("_Core"), TEXT("_BombThreat_Core")));
}

void AMissionPortal::SetSelectedEntryPoint(FName EntryPoint)
{
	for (TActorIterator<AMissionPortal> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		if (!It->HasAuthority())
			return;
	
		It->SelectedEntryPoint = EntryPoint;
	}
}

void AMissionPortal::NextMission()
{
	UReadyOrNotGameInstance* gi = UBpGameplayHelperLib::GetRONGameInstance();
	TArray<FString> maplist = gi->GetBuiltMapList();
	TArray<FString> COOPMaps;
	for (FString f : maplist)
	{
		if (f.Contains("?game=coop"))
		{
			COOPMaps.AddUnique(f);
		}
	}

	int32 idx = -1;
	// find isn't working try this
	for (int32 i = 0; i < COOPMaps.Num(); i++)
	{
		if (COOPMaps[i] == MissionURL)
		{
			idx = i;
			break;
		}
	}

	if (COOPMaps.Num() > 0)
	{
		if (COOPMaps.IsValidIndex(idx + 1))
		{
			MissionURL = COOPMaps[idx +1];
		} else
		{
			MissionURL = COOPMaps[0];
		}
	}
	
}

void AMissionPortal::PreviousMission()
{
	UReadyOrNotGameInstance* gi = UBpGameplayHelperLib::GetRONGameInstance();
	TArray<FString> maplist = gi->GetBuiltMapList();
	TArray<FString> COOPMaps;
	for (FString f : maplist)
	{
		if (f.Contains("?game=coop"))
		{
			COOPMaps.AddUnique(f);
		}
	}

	int32 idx = -1;
	// find isn't working try this
	for (int32 i = 0; i < COOPMaps.Num(); i++)
	{
		if (COOPMaps[i] == MissionURL)
		{
			idx = i;
			break;
		}
	}

	if (COOPMaps.Num() > 0)
	{
		if (COOPMaps.IsValidIndex(idx - 1))
		{
			MissionURL = COOPMaps[idx - 1];
		} else
		{
			MissionURL = COOPMaps[COOPMaps.Num() - 1];
		}
	}
	
}

void AMissionPortal::SelectRandomMission(bool bUseServerMapList)
{
	UReadyOrNotGameInstance* gi = UBpGameplayHelperLib::GetRONGameInstance();
	TArray<FString> maplist = gi->GetBuiltMapList();
	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		AReadyOrNotGameSession* gs = Cast<AReadyOrNotGameSession>(UReadyOrNotStatics::GetReadyOrNotGameMode()->GameSession);
		if (gs)
		{
 			if (gs->MapList.Num() > 0)
			{
				if (!gs->MapList.IsValidIndex(UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerMapIdx))
					UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerMapIdx = 0;
				
				MissionURL = gs->MapList[UReadyOrNotStatics::GetReadyOrNotGameInstance()->DedicatedServerMapIdx];
				FString OutL, OutR;
				if (MissionURL.Split("?game=", &OutL, &OutR))
				{
					ModeURL = OutR;
				}
 				//V_LOGM(LogReadyOrNot, "Selecting Random Mission %s [%s]", *MissionURL, *ModeURL);
				// We're using this map for real!
				if (bUseServerMapList)
					return;
					
			}
		}
	}

	
	TArray<FString> COOPMaps;
	for (FString f : maplist)
	{
		if (f.Contains("coop", ESearchCase::IgnoreCase))
		{
			COOPMaps.AddUnique(f);
		}
	}

	if (COOPMaps.Num() > 0)
	{
		FString RandomMissionURL = COOPMaps[FMath::RandRange(0, COOPMaps.Num() - 1)];
		FString OutL, OutR;
		if (RandomMissionURL.Split("?", &OutL, &OutR))
		{
			RandomMissionURL = OutL;
		}
		while (true)
		{
			ECOOPMode RndMode = (ECOOPMode)FMath::RandRange(1, 5);
			if (IsGameModeSelectable(RndMode))
			{
				switch (RndMode)
				{
				case ECOOPMode::CM_None: break;
				case ECOOPMode::CM_BombThreat:
					ModeURL = "BT_COOP";
					break;
				case ECOOPMode::CM_ActiveShooter:
					ModeURL = "AS_COOP";
					break;
				case ECOOPMode::CM_HostageRescue:
					ModeURL = "HR_COOP";
					break;
				case ECOOPMode::CM_BarricadedSuspects:
					ModeURL = "BS_COOP";
					break;
				case ECOOPMode::CM_Raid:
					ModeURL = "RD_COOP";
					break;
				default: ;
				}

			
				RandomMissionURL.RemoveFromEnd("?game=coop", ESearchCase::IgnoreCase);
				
				MissionURL = RandomMissionURL + "?game=" + ModeURL;
				break;
			}
		}
		
	}

	V_LOGM(LogReadyOrNot, "Selecting Random Mission %s [%s]", *MissionURL, *ModeURL);
}

void AMissionPortal::StartMission()
{
	FString FullURL = GetFormattedMissionURL();
				
	if (CommanderProfile)
	{
		FString OutMap, OutMode;
		FullURL.Split("?game=", &OutMap, &OutMode);
		FullURL = OutMap + "?game=commander?save=" + CommanderProfile->GetSlot();
	}
				
	if (!SelectedEntryPoint.IsNone())
		FullURL += "?entrypoint=" + SelectedEntryPoint.ToString();
				
	GetWorld()->GetAuthGameMode()->ProcessServerTravel(FullURL, true);
}

void AMissionPortal::DrawMissionPortalOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(true);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnable)
	{
		if (s)
		{
			s->SetVisibility(true);
		}
	}
}

void AMissionPortal::DisableMissionPortalOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(false);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnable)
	{
		if (s)
		{
			s->SetVisibility(false);
		}
	}
}

void AMissionPortal::DrawMissionSelectedPortalOutLine()
{
	for (UStaticMeshComponent* s : CompsToOutlineMissionSelected)
	{
		if (s)
		{
			s->SetRenderCustomDepth(true);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnableMissionSelected)
	{
		if (s)
		{
			s->SetVisibility(true);
		}
	}
}

void AMissionPortal::DisableMissionSelectedPortalOutline()
{
	
	for (UStaticMeshComponent* s : CompsToOutlineMissionSelected)
	{
		if (s)
		{
			s->SetRenderCustomDepth(false);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnableMissionSelected)
	{
		if (s)
		{
			s->SetVisibility(false);
		}
	}
}

FName AMissionPortal::DetermineAnimatedIcon_Implementation() const
{
	return "ChangeMission";
}

FText AMissionPortal::DetermineActionText_Implementation() const
{
	return NSLOCTEXT("MissionPortal", "MissionPortalNextMission", "Next Mission");
}

void AMissionPortal::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator,
                                             UInteractableComponent* InInteractableComponent)
{
	if (AreAllPlayersInPortal() && !GetFormattedMissionURL().IsEmpty())
		return;

	// Reload save data each time in case we've run commands to unlock levels
	ALobbyGM* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGM>() : nullptr;
	if (LobbyGM)
	{
		MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
		CommanderProfile = LobbyGM->CommanderProfile;
	}
	
	// Break out early with new mission select
	if (IsValid(MissionSelect))
	{
		MissionSelect->OpenMissionSelect();
		return;
	}
	
	if (!UBpGameplayHelperLib::HasWidgetInViewport("WorldMap"))
	{
		UReadyOrNotStatics::GetReadyOrNotPlayerController()->Client_CreateWidget("WorldMap");
	}
}

bool AMissionPortal::CanInteract_Implementation() const
{
	return HasAuthority();
}

bool AMissionPortal::AreAllPlayersInPortal() const
{
	return (GetNumberOverlappingPlayers() == GetNumPlayers());
}

int32 AMissionPortal::GetNumPlayers() const
{
	int32 i = 0;

	for (TActorIterator<AReadyOrNotPlayerState>It(GetWorld()); It; ++It)
	{
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(It->GetPawn());
		if (It->GetUniqueId().IsValid() && !It->bIsReplaySpectator && !ReplayCameraPawn)
		{
			i++;
		}
	}

#if defined(TARGET_CONSOLE) 
	if (i == 0) i = 1;
#endif

	return i;
}

int32 AMissionPortal::GetNumberOverlappingPlayers() const
{
	int32 NumPlayers = 0;
	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		if (It && GetCollisionComponent()->IsOverlappingActor(*It))
		{
			NumPlayers++;
		}
	}
	if(UReadyOrNotGameInstance* gi = UBpGameplayHelperLib::GetRONGameInstance())
	{
		gi->ReplayNumPlayers = NumPlayers;
	}
	return NumPlayers;
}

void AMissionPortal::OnPlayerJoinedLobby(AReadyOrNotPlayerController* PlayerController)
{
	if (!PlayerController)
		return;
	
	RequiredReadyPlayers = FMath::CeilToInt(GetWorld()->GetNumPlayerControllers() / 2.f);

	PlayerController->OnPlayerReadyChange.AddDynamic(this, &AMissionPortal::OnPlayerReadyChange);

	OnPlayerReadyChange(PlayerController, false);
}

void AMissionPortal::OnPlayerLeftLobby(AGameModeBase* GM, AController* Controller)
{
	// Make sure the world still exists, this function could have been called by shutting down
	if (!GetWorld())
		return;
	
	RequiredReadyPlayers = FMath::CeilToInt(GetWorld()->GetNumPlayerControllers() / 2.f);
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(Controller);
	if (!PlayerController)
		return;

	ReadiedPlayers.Remove(PlayerController);
	
	OnPlayerReadyChange(PlayerController, false);
}

void AMissionPortal::OnPlayerReadyChange(AReadyOrNotPlayerController* ReadyOrNotPlayerController, bool bReady)
{
	// Recalc the required ready players here, as having it done only in OnPlayerJoined/Left can cause errors after a seamless travel
	// as the playercontroller from previous map carries over briefly
	RequiredReadyPlayers = FMath::CeilToInt(GetWorld()->GetNumPlayerControllers() / 2.f);
	
	if (bReady)
	{
		ReadiedPlayers.AddUnique(ReadyOrNotPlayerController);
	}
	else
	{
		ReadiedPlayers.Remove(ReadyOrNotPlayerController);
	}
	
	if (ReadiedPlayers.Num() >= GetWorld()->GetNumPlayerControllers())
	{
		if (LastReadyState != RS_All)
		{
			Multicast_SetTimer(true, AllReadyCountdown);
		}

		LastReadyState = RS_All;
	}
	else if (ReadiedPlayers.Num() >= RequiredReadyPlayers)
	{
		if (LastReadyState != RS_Majority)
		{
			Multicast_SetTimer(true, MajorityReadyCountdown);
		}

		LastReadyState = RS_Majority;
	}
	else
	{
		if (LastReadyState != RS_Minority && LastReadyState != RS_None)
		{
			Multicast_SetTimer(false, MajorityReadyCountdown);
		}

		LastReadyState = ReadiedPlayers.Num() ? RS_Minority : RS_None;
	}

	NumReadyPlayers = ReadiedPlayers.Num();
	OnRep_ReadiedPlayersChange();
}

void AMissionPortal::Multicast_SetTimer_Implementation(bool bEnabled, float SetTime)
{
	bTimerRunning = bEnabled;
	CurrentTimer = SetTime;

	if (bEnabled)
	{
		OnCountdownStarted.Broadcast(CurrentTimer);
	}
	else
	{
		OnCountdownCancelled.Broadcast();
	}
}

void AMissionPortal::OnRep_ReadiedPlayersChange()
{
	APlayerCharacter* PlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (!PlayerCharacter || !PlayerCharacter->HumanCharacterWidget_V2)
		return;
	
	PlayerCharacter->HumanCharacterWidget_V2->ReadiedPlayersChanged();
}