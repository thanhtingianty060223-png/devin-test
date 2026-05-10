// Copyright Void Interactive, 2023

#pragma once

#include "ReadyOrNotGameMode.h"
#if defined(WITH_MODIO)
#include "Types/ModioCommonTypes.h"
#endif
#if defined(TARGET_PS5)
#include "Interfaces/OnlineGameActivityInterface.h"
#endif

#include "MainMenuGM.generated.h"

class UCreateSessionCallbackProxyAdvanced;
class UFindSessionsCallbackProxyAdvanced;
class UDestroySessionCallbackProxyAdvanced;

/**
 * 
 */
UCLASS()
class READYORNOT_API AMainMenuGM : public AGameModeBase
{
	GENERATED_BODY()

	// keep this here to prevent callbacks being garbage collected when using OnlineSubsystemSony
	UPROPERTY()
	UCreateSessionCallbackProxyAdvanced* CreateSessionCallbackProxyAdvanced;
	
	UPROPERTY()
	UFindSessionsCallbackProxyAdvanced* FindSessionsCallbackProxyAdvanced;

	UPROPERTY()
	UDestroySessionCallbackProxyAdvanced* DestroySessionCallbackProxyAdvanced;

public:
	AMainMenuGM();
	
	UPROPERTY(EditAnywhere)
	UFMODEvent* MainMenuMusic;
	
	UPROPERTY(EditAnywhere)
	UFMODEvent* MainMenuAmbience;

	FFMODEventInstance MainMenuMusicInst;
	FFMODEventInstance MainMenuAmbienceInst;

	FRotator OriginalCameraRotation;

	void PlayMainMenuMusic();
	
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void OnBanStatusChecked(FString SteamId, bool bIsBanned, FString BanReason, bool bIsMySteamId);

	void ApplyRONGameUserSettings();

	FTimerHandle TH_ShowMainMenuMsg;
	UFUNCTION()
	void ShowMainMenuMsg();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;
	
	float DesiredSessionPing = 50.0f;
	float TimeUntilFindNextSessionList = 5.0f;
	bool bShouldFindSession = false;
	bool bPVPSessionSearch = false;
	bool bDisplayedRestart = false;

	UFUNCTION(BlueprintCallable)
	void FindOnlineSession(bool bNewSearch = true, bool bPVPSession = false);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUpdateSessionSearch, bool, bComplete, FText, OutMessage, bool, bPVPSearch);
	UPROPERTY(BlueprintAssignable)
	FOnUpdateSessionSearch OnUpdateSessionSearch;

	UFUNCTION(BlueprintCallable)
	void CancelSessionSearch();

	UFUNCTION(BlueprintCallable)
	bool IsSearchingForSession();

	bool bPendingCancelSessionSearch = false;

	UFUNCTION()
	void OnFindSessionFailed(const TArray<FBlueprintSessionResult>& Results);

	FTimerHandle TH_FindNextSessionList;
	UFUNCTION()
	void FindNextSessionList();

	UFUNCTION()
	void OnFindSessionSuccess(const TArray<FBlueprintSessionResult>& Results);
	

	UFUNCTION()
	void OnDestroySessionBeforeStartingLobby();

	bool bCommanderMode = false;
	FString CommanderSaveSlot;
	
	bool bPendingLobbyOnlineMode = false;
	bool bFriendsOnlyLobby = false;

	// Travels to a commander mode lobby for the specified slot index, loading or creating a profile if necessary
	UFUNCTION(BlueprintCallable)
	void GoToCommanderMode(int32 ProfileSlot, bool bIronmanMode, bool bTutorialMode);

	// Travels to a commander lobby for the last played save, if available
	UFUNCTION(BlueprintCallable)
	void ContinueCommanderMode();

	// Returns true if there is commander profile data to continue a game from
	UFUNCTION(BlueprintCallable)
	bool CanContinueCommanderMode();

	// Returns true if training has been completed
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool HasCompletedTraining();

	UFUNCTION(BlueprintCallable)
	void GoToTraining();

	UFUNCTION()
	void OnTrainingSuccess();
	
	UFUNCTION(BlueprintCallable)
	void GoToLobby(bool bOnlineMode, bool bFriendsOnly);
	
	UFUNCTION()
	void OnLobbySuccess();

	UFUNCTION()
	void OnLobbyFailed();

	void CreateAuthenticationMenu();

	void DestroyAuthenticationMenu();

	UPROPERTY()
	UUserWidget* AuthenticationMenu;

	bool bCreatedMainMenu = false;
	bool bShouldCreateMainMenu = true;
	bool bCreatedLoginFailedMsg = false;

	void CreateMainMenu();

	void DestroyMainMenu();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;

#if defined(WITH_MODIO)
	void OnModStateChange(FModioModID Mod);
#endif

	UFUNCTION(BlueprintImplementableEvent)
	void CreateRestartWidget();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowMessageDisplayBox(const FString& MessageText, const FString& ButtonText, const bool QuitOnPress = false);
};
