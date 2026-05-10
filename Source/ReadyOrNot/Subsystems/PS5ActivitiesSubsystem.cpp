// Copyright Void Interactive, 2023


#include "PS5ActivitiesSubsystem.h"
#include "ReadyOrNotGameInstance.h"
//#include "Commander/CommanderGM.h"
#include "GameModes/LobbyGM.h"

DEFINE_LOG_CATEGORY(LogReadyOrNotPS5Activities);

void UPS5ActivitiesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	#if defined(TARGET_PS5)
		IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
		
		if (ActivityPtr !=  nullptr)
		{
			OnGameActivityActivationRequestedDelegate = FOnGameActivityActivationRequestedDelegate::CreateUObject( this, &ThisClass::ActivityActivationRequested );
			OnGameActivityActivationRequestedDelegateHandle = ActivityPtr->AddOnGameActivityActivationRequestedDelegate_Handle( OnGameActivityActivationRequestedDelegate );
			// OnStartActivityCompleteDelegate = FOnStartActivityComplete::CreateUObject( this, &ThisClass::StartActivityComplete );
			// OnStartActivityCompleteDelegateHandle = Helper.OnlineSub->GetGameActivityInterface()->AddOnStartActivityCompleteDelegate_Handle( OnStartActivityCompleteDelegate );
		}
	#endif	
}

IOnlineGameActivityPtr UPS5ActivitiesSubsystem::GetOnlineGameActivityPtr()
{
#if defined(TARGET_PS5)

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (OnlineSubsystem)
	{
		auto Activities = OnlineSubsystem->GetGameActivityInterface();
		if (Activities.IsValid()) {
			return Activities;
		}
	}
#endif
	return nullptr;	
}

// sent from PS5 OS
void UPS5ActivitiesSubsystem::ActivityActivationRequested(const FUniqueNetId& LocalUserId, const FString& ActivityId, const FOnlineSessionSearchResult* SessionInfo)
{
#if defined(TARGET_PS5)
	UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ActivityActivationRequested %s, %s"), *LocalUserId.ToString(), *ActivityId);

	// Check if already in Commander, then we ignore this request
	ALobbyGM* LobbyGM = Cast<ALobbyGM>(UGameplayStatics::GetGameMode(GetWorld()));
	if (LobbyGM && LobbyGM->CommanderProfile!=nullptr) return; // already in commander mode

	IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
	if (ActivityPtr != nullptr)
	{
		 FOnlineEventParms Parms;
		 FOnStartActivityComplete CompletionDelegate;
		 CompletionDelegate.BindLambda([](const FUniqueNetId& LocalUserId , const FString ActivityId, const FOnlineError& Status)	
		 {
		 	UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ActivityActivationRequested, StartActivity completed activity: %s, status: %s"), *ActivityId, *Status.ToLogString());
		 });		
		ActivityPtr->StartActivity(LocalUserId, ActivityId, Parms, CompletionDelegate);

		UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType = ESessionType::ST_SinglePlayer;

		UCommanderProfile* CommanderProfile = GetLastUsedCommanderProfile();
		FString CommanderSaveSlot;

		if (CommanderProfile != nullptr)
		{
			CommanderSaveSlot = CommanderProfile->GetSlot();
		}
		else
		{
			CommanderSaveSlot = "0";
			CommanderProfile = UCommanderProfile::CreateProfile(CommanderSaveSlot, false);
		}

		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			ULevelStreaming* StreamedLevel = nullptr;

			FString Options = "";
			Options = "?save=" + CommanderSaveSlot;

			UMetaGameProfile* MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
			if (MetaGameProfile)
			{
				MetaGameProfile->LastCampaignSave = CommanderSaveSlot;
				MetaGameProfile->SaveProfile();
			}

			FLevelStreamOptions LevelStreamOptions;
			LevelStreamOptions.bShouldCreateLoadingScreen = false;
			LevelStreamOptions.bShouldRemoveLoadingScreen = false;
			LevelStreamOptions.bStreamInLevelBeforeLoad = true;

			UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());

			FString Level;
			if (gi) {
				Level = gi->LobbyLevel;
			}

			if (!pc->StreamInLevel(Level, Options, StreamedLevel, LevelStreamOptions))
			{
				// Fail silently for now
				//IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
				//if (OnlineSub)
				//{
				//	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

				//	if (Sessions.IsValid())
				//	{
				//		Sessions->DestroySession(NAME_GameSession);
				//	}
				//}
				//const FString Message = NSLOCTEXT("MainMenu", "MainMenuFailedToFindLevel", "Failed to Create World (Level has not been found)").ToString();
				//const FString ButtonText = NSLOCTEXT("MainMenu", "MainMenuOKBtn", "OK").ToString();
				//ShowMessageDisplayBox(Message, ButtonText);
			}
			else
			{
				// localization
				pc->Client_CreateLoadingScreen(Level, "lobby", pc->GetPlayerState<APlayerState>()->GetPlayerName() + "'s Lobby");
			}
		}
	}
#endif	
}

UCommanderProfile* UPS5ActivitiesSubsystem::GetLastUsedCommanderProfile()
{
#if defined(TARGET_PS5)
	UMetaGameProfile* Profile = UMetaGameProfile::GetProfile(GetWorld());
	if (!ensure(Profile))
		return nullptr;

	return UCommanderProfile::LoadProfile(Profile->LastCampaignSave);
#else
	return nullptr;
#endif	
}

void UPS5ActivitiesSubsystem::ActivityStart(const FUniqueNetId& LocalUserId, FString& Activity)
{
#if defined(TARGET_PS5)
	IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
	if (ActivityPtr != nullptr)
	{
		// FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("CompleteOnlineActivity"), GetWorld());
		FOnlineEventParms Parms;
		FOnStartActivityComplete CompletionDelegate;
		CompletionDelegate.BindLambda([](const FUniqueNetId& LocalUserId , const FString ActivityId, const FOnlineError& Status)	
		{
			UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ActivityStart completed activity: %s, outcome: %s, error: %s"), *ActivityId, *Status.ToLogString());
		});
		
		ActivityPtr->StartActivity(LocalUserId,Activity, Parms, CompletionDelegate);
	}
#endif
}

void UPS5ActivitiesSubsystem::ActivityEnd(const FUniqueNetId& LocalUserId,FString& Activity, EOnlineActivityOutcome ActivityOutcome)
{
#if defined(TARGET_PS5)
	IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
	if (ActivityPtr != nullptr)
	{
		FOnlineEventParms Parms;
		FOnEndActivityComplete CompletionDelegate;
		CompletionDelegate.BindLambda([](const FUniqueNetId& LocalUserId , const FString ActivityId, const EOnlineActivityOutcome& Outcome, const FOnlineError& Status)	
		{
			UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ActivityEnd completed activity: %s, outcome: %s, status: %s"), *ActivityId, *UBpGameplayHelperLib::EnumToString("EOnlineActivityOutcome", Outcome), *Status.ToLogString());
		});
		
		ActivityPtr->EndActivity(LocalUserId,Activity, ActivityOutcome, Parms, CompletionDelegate);
	}
#endif	
}

void UPS5ActivitiesSubsystem::ActivitySetAvailable(const FUniqueNetId& LocalUserId,FString& Activity, bool bAvailable)
{
#if defined(TARGET_PS5)
	IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
	if (ActivityPtr != nullptr)
	{
		FOnlineEventParms Parms;
		FOnSetActivityAvailabilityComplete CompletionDelegate;
		CompletionDelegate.BindLambda([Activity, bAvailable](const FUniqueNetId& LocalUserId , const FOnlineError& Status)	
		{
			UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ActivitySetAvailable completed task:%s, available:%s status: %s"), *Activity, bAvailable? "true" : "false", *Status.ToLogString());
		});

		ActivityPtr->SetActivityAvailability(LocalUserId, Activity, bAvailable, CompletionDelegate);
	}
#endif	
}

void UPS5ActivitiesSubsystem::ResetAllActiveActivities(const FUniqueNetId& LocalUserId)
{
#if defined(TARGET_PS5)
	IOnlineGameActivityPtr ActivityPtr = GetOnlineGameActivityPtr();
	if (ActivityPtr != nullptr)
	{
		FOnlineEventParms Parms;
		FOnResetAllActiveActivitiesComplete CompletionDelegate;
		CompletionDelegate.BindLambda([](const FUniqueNetId& LocalUserId , const FOnlineError& Status)	
		{
			UE_LOG(LogReadyOrNotPS5Activities, Warning, TEXT("ResetAllActiveActivities error: %s"), *Status.ToLogString());
		});

		ActivityPtr->ResetAllActiveActivities(LocalUserId, CompletionDelegate);
	}
#endif	
}

void UPS5ActivitiesSubsystem::OnLoadGame(const FUniqueNetId& LocalUserId, UCommanderProfile* CommanderProfile)
{
#if defined(TARGET_PS5)
    // reset activity
    ResetAllActiveActivities(LocalUserId);
    
    for (EActivitiesObjective Objective : TEnumRange<EActivitiesObjective>()) 
    {
        FString ObjectiveString = *UEnum::GetValueAsString(Objective);
        if (ObjectiveString==CommanderProfile->GetNextLevel())
            ActivitySetAvailable(LocalUserId,ObjectiveString,true);
        else
            ActivitySetAvailable(LocalUserId,ObjectiveString,false);
    }   
#endif
}

void UPS5ActivitiesSubsystem::OnSaveGame(const FUniqueNetId& LocalUserId, UCommanderProfile* CommanderProfile)
{
#if defined(TARGET_PS5)
	if (CommanderProfile->CompletedLevels.Num() == 18)
	{
		FString CommanderActivity = TEXT("COMMANDER");
		ActivityEnd(LocalUserId, CommanderActivity, EOnlineActivityOutcome::Completed);
	}
	else
	{
		// update activity
		for (EActivitiesObjective Objective : TEnumRange<EActivitiesObjective>())
		{
			FString ObjectiveString = *UEnum::GetValueAsString(Objective);
			if (ObjectiveString == CommanderProfile->GetNextLevel())
				ActivitySetAvailable(LocalUserId, ObjectiveString, true);
			else
				ActivitySetAvailable(LocalUserId, ObjectiveString, false);
		}
	}
#endif
}

void UPS5ActivitiesSubsystem::Tick( float DeltaTime )
{
}

/* UAchievementStatics */

UPS5ActivitiesSubsystem* UPS5ActivitiesStatics::GetAchievementSubsystem(UObject* WorldContextObject)
{
#if defined(TARGET_PS5)
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;
	if (UBpGameplayHelperLib::IsMultiplayer(World))
		return nullptr;
	
	UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = World->GetGameInstance()->GetSubsystem<UPS5ActivitiesSubsystem>();
	if (!PS5ActivitiesSubsystem)
		return nullptr;

	return PS5ActivitiesSubsystem;
#else
	return nullptr;
#endif
}

FUniqueNetIdPtr UPS5ActivitiesStatics::GetLocalUserId(UObject* WorldContextObject)
{
#if defined(TARGET_PS5)	
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;
	if (UBpGameplayHelperLib::IsMultiplayer(World))
		return nullptr;
	
	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(World);
	if (!PlayerController)
		return nullptr;

	return PlayerController->PlayerState->GetUniqueId().GetUniqueNetId();
#else
	return nullptr;
#endif
}
	

void UPS5ActivitiesStatics::ActivityStart(UObject* WorldContextObject, FString& Activity)
{
#if defined(TARGET_PS5)
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->ActivityStart(*UserID, Activity);
		}
	}
#endif
}

void UPS5ActivitiesStatics::ActivityEnd(UObject* WorldContextObject, FString& Activity, EOnlineActivityOutcome Outcome)
{
#if defined(TARGET_PS5)
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->ActivityEnd(*UserID, Activity, Outcome);
		}
	}
#endif
}

void UPS5ActivitiesStatics::ActivitySetAvailable(UObject* WorldContextObject, FString& Activity, bool bAvailable)
{
#if defined(TARGET_PS5)	
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->ActivitySetAvailable(*UserID,Activity, bAvailable);
		}
	}
#endif	
}

void UPS5ActivitiesStatics::ResetAllActiveActivities(UObject* WorldContextObject)
{
#if defined(TARGET_PS5)	
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->ResetAllActiveActivities(*UserID);
		}
	}
#endif	
}

void UPS5ActivitiesStatics::OnLoadGame(UObject* WorldContextObject, UCommanderProfile* CommanderProfile)
{
#if defined(TARGET_PS5)
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->OnLoadGame(*UserID, CommanderProfile);
		}
	}
#endif	
}

void UPS5ActivitiesStatics::OnSaveGame(UObject* WorldContextObject, UCommanderProfile* CommanderProfile)
{
#if defined(TARGET_PS5)
	if (UPS5ActivitiesSubsystem* PS5ActivitiesSubsystem = UPS5ActivitiesStatics::GetAchievementSubsystem(WorldContextObject))
	{
		FUniqueNetIdPtr UserID = GetLocalUserId(WorldContextObject);
		if (UserID.IsValid())
		{
			PS5ActivitiesSubsystem->OnSaveGame(*UserID, CommanderProfile);
		}
	}
#endif	
}