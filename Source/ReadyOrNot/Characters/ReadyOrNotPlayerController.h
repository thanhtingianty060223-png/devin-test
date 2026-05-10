// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/PlayerController.h"
#include "ReadyOrNotGameState.h"

#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
#pragma push_macro("ARRAY_COUNT")
#undef ARRAY_COUNT

#if USING_CODE_ANALYSIS
MSVC_PRAGMA(warning(push))
MSVC_PRAGMA(warning(disable : ALL_CODE_ANALYSIS_WARNINGS))
#endif	// USING_CODE_ANALYSIS

#if USING_CODE_ANALYSIS
MSVC_PRAGMA(warning(pop))
#endif	// USING_CODE_ANALYSIS

#pragma warning(disable:4265)
#pragma pop_macro("ARRAY_COUNT")
#endif

#include "Components/ProgressionComponent.h"
// ##UE5UPGRADE## Zeus
//#include "ApiMatchmaking.h"
#include "Structs.h"
#include "UASAimAssistConfigDataAsset.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"
#include "HUD/Widgets/MissionPlanWidget.h"
#include "ReadyOrNotPlayerController.generated.h"

//A struct holds necessary infos sent from client to complete authentication
USTRUCT()
struct FSteamAuthenticationToken
{
	GENERATED_BODY()

public:
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		//Ar.ByteOrderSerialize(rgchToken, 1024);
		Ar << rgchTokenString;
		Ar << unTokenLen;
		Ar << steamid;

		bOutSuccess = true;
		return true;
	}
public:

	UPROPERTY()
	FString rgchTokenString;
	UPROPERTY()
	uint32 unTokenLen = 0;
	UPROPERTY()
	uint64 steamid;
};

template<>
struct TStructOpsTypeTraits<FSteamAuthenticationToken> : public TStructOpsTypeTraitsBase2<FSteamAuthenticationToken>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT(BlueprintType)
struct FVoteData
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, Category = Vote)
	EVoteState CurrentVoteState = EVoteState::Undecided;

	UPROPERTY(BlueprintReadOnly, Category = Vote)
	FString VoteReason = "Vote Reason";
	
	UPROPERTY(BlueprintReadOnly, Category = Vote)
	FString VoteQuestion = "";

	UPROPERTY(BlueprintReadOnly, Category = Vote)
	uint8 bVoteEnabled : 1;

	UPROPERTY(BlueprintReadOnly, Category = Vote)
	uint8 bCanVoteNo : 1;
};

USTRUCT()
struct FProjectileData
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<ABulletProjectile*> Projectiles;
};

class UJoinSessionCallbackProxyAdvanced;

UCLASS()
class READYORNOT_API AReadyOrNotPlayerController : public APlayerController
{
	GENERATED_BODY() 

	ETeamType Team;

	UPROPERTY()
	class UProgressionComponent* ProgressionComp;

	void UpdateRichPresence(FString MapName);

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnPossess() override;
	static const FString MENU_PRE_MISSION_PLANNING;
	AReadyOrNotPlayerController();

	bool bWantsHudClearOnPossess = true;

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SetHasFinishedLoading();

    FKey GetInputKey(FName InputName);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(Replicated)
	int32 ServerSideChecksum = 0;
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetChecksum(int32 Checksum);
	bool bStartedChecksumKick = false;
	
	bool bExiting = false;
	
	class AReadyOrNotPlayerState* GetReadyOrNotPlayerState();

	float LastSetMicInputGain = 1.0f;
	float LastSetVOIPVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bShouldShowMouseCursor = true;

	UFUNCTION(BlueprintCallable)
	void SetShouldShowMouseCursor(bool bShow);

	UFUNCTION(BlueprintCallable)
	bool IsConsoleTarget();

	UFUNCTION(Client, Reliable)
	void ClientSetNetSpeed(int32 NewNetSpeed);
	virtual void ClientSetNetSpeed_Implementation(int32 NewNetSpeed);

	virtual void SetPawn(APawn* InPawn) override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPawnPossessed, APawn*, PossessedPawn);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnPawnPossessed OnPawnPossessed;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitSettingsMenu);
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnExitSettingsMenu OnExitSettingsMenu;

	virtual void PawnLeavingGame() override;
	
	virtual void SetupInputComponent() override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutOfBoundsChanged, bool, bIsOutOfBounds);
	UPROPERTY(BlueprintAssignable)
	FOnOutOfBoundsChanged OnOutOfBoundsChanged;

	/** sets spectator location and rotation */
	UFUNCTION(reliable, client)
	void ClientSetSpectatorCamera(FVector CameraLocation, FRotator CameraRotation);

	virtual void PawnPendingDestroy(APawn* inPawn) override;

	bool FindDeathCameraSpot(FVector& CameraLocation, FRotator& CameraRotation);

	virtual void ClientTravelInternal_Implementation(const FString& URL, ETravelType TravelType, bool bSeamless, FGuid MapPackageGuid) override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPreClientTravel);

	UPROPERTY(BlueprintAssignable)
	FOnPreClientTravel OnPreClientTravel;

	UFUNCTION(Exec, BlueprintCallable, Category = "ReturnToMenu")
	void BP_ReturnToMenu(const FText& ReturnReason);

	virtual void ClientReturnToMainMenuWithTextReason(const FText& ReturnReason) override;

	UFUNCTION(BlueprintPure)
	void GetNetworkConnectionStatus(float& AvgLag, int32& OutLostPackets, int32& InLostPackets, int32& OutLostPacketPrcnt, int32& InLostPacketPrcnt);

	// todo: client
	void ClientStartCameraShake2(class UCameraShakeBase* Shake, float Scale = 1.f, ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	
	bool bDisableCameraShakes = false;
	virtual void ClientStartCameraShake_Implementation(TSubclassOf<class UCameraShakeBase> Shake, float Scale = 1.f, ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator) override;

	/** stores pawn location at last player death, used where player scores a kill after they died **/
	FVector LastDeathLocation;

	/** Starts the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientStartOnlineGame();

	/** Ends the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientEndOnlineGame();

	// Our player has just spawned
	UFUNCTION(Reliable, Client)
	void ClientSpawned();
	virtual void ClientSpawned_Implementation();

	UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable)
	void Server_StartSpectating();

	UPROPERTY()
	float RespawnTimeLeft = 0.0f;
	UFUNCTION(Reliable, Client)
	void NotifyRespawnTime(float RespawnTime);

	UFUNCTION(BlueprintPure)
	static float GetRespawnTimeRemaining();

	UFUNCTION()
	void ChangeInputMode(bool bGameMode, bool bMouseCursorEnabled = true, UWidget* Widget = nullptr);

	UFUNCTION()
    void EnablePlayerInput();
	
	UFUNCTION(BlueprintCallable)
	virtual void EscapeMenu();

	bool CanOpenPauseMenu();
	void OpenPauseMenu();

	void ShowTabMenu();
	void HideTabMenu();

	UFUNCTION(BlueprintCallable)
	bool IsOverlayHudVisible() const;
	
	UPROPERTY()
	UUserWidget* TabMenuWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> TabMenuWidgetClass;

	void SetupSubtitles();
	void DestroySubtitles();
	
	UPROPERTY()
	UUserWidget* SubtitlesWidget;
	
	UFUNCTION(BlueprintCallable)
	void ToggleDeployMenu();

	UFUNCTION(Exec)
	void DownloadBlacklistHashes();

	UFUNCTION(Exec)
	void DestroyAllExceptClosestCharacter();

	UFUNCTION(Exec)
	void DestroyAllExceptClosestDoor();

	UFUNCTION(Exec)
	void OnlyCastLocalPlayerDynamicShadow();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Equip(const FString& ItemName);
	void Server_Equip_Implementation(const FString& ItemName);
	bool Server_Equip_Validate(const FString& ItemName) { return true; }

	UFUNCTION(Exec)
	void EnableWeaponFovShader();
	UFUNCTION(Exec)
	void DisableWeaponFovShader();
	
	UFUNCTION(Exec)
	void AIEquipPrimary();

	UFUNCTION(Exec)
	void DestroyAllActorsOfName(FString Name);

	UFUNCTION(Exec)
	void AIEquipSecondary();

	UFUNCTION(Exec)
	void OptimizeWorld();

	UFUNCTION(Exec)
	void DestroyAllDecals();

	UFUNCTION(Exec)
	void DestroyEverything();

	UFUNCTION(Exec)
	void DestroyAllWorldDynamicItems();

	UFUNCTION(Exec)
	void DestroyAllLights();

	UFUNCTION(Exec)
	void ReportAllInstancedStaticMeshes();

	UFUNCTION(Exec)
	void StartBleeding();
	
	UFUNCTION(Exec)
	void ResetDoorLockStateKnowledge();
	
	UFUNCTION(Exec)
	void AbortCover();

	UFUNCTION(Exec)
	void AIHide();

	UFUNCTION(Exec)
	void AIStopHide();

	UFUNCTION(Exec)
	void SkipTutorial();

	bool IsAFK(float AFKTime);
	float SecondsSinceLastMovement = 0.0f;

#if WITH_EDITOR
	UFUNCTION(Exec)
	void EnableDLCChecks(int32 i = 0);

	int32 DLCChecksEnabled = 0;
#endif

	// used for various debug commands
	UPROPERTY()
	ACameraActor* SpectateCamera;
	
	UFUNCTION(BlueprintCallable, Category = Mouse)
	void SetMousePosition(float LocationX, float LocationY);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Server Administration")
	void Server_LoginAsAdmin(const FString& password);
	virtual void Server_LoginAsAdmin_Implementation(const FString& password);
	virtual bool Server_LoginAsAdmin_Validate(const FString& password) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Server Administration")
	void Server_AdminRestartServer();
	virtual void Server_AdminRestartServer_Implementation();
	virtual bool Server_AdminRestartServer_Validate();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Server Administration")
	void Server_AdminNextMap();
	virtual void Server_AdminNextMap_Implementation();
	virtual bool Server_AdminNextMap_Validate();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Server Administration")
	void Server_AdminKickPlayer(APlayerState* KickingPlayerState, const FText& ReasonOveride);
	virtual void Server_AdminKickPlayer_Implementation(APlayerState* KickingPlayerState, const FText& ReasonOveride);
	virtual bool Server_AdminKickPlayer_Validate(APlayerState* KickingPlayerState, const FText& ReasonOveride);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Server Administration")
	void Server_AdminBanPlayer(APlayerState* BanningPlayerState);
	virtual void Server_AdminBanPlayer_Implementation(APlayerState* BanningPlayerState);
	virtual bool Server_AdminBanPlayer_Validate(APlayerState* BanningPlayerState);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_AdminAddMapToRotation(const FString& MapURL);
	virtual void Server_AdminAddMapToRotation_Implementation(const FString& MapURL);
	virtual bool Server_AdminAddMapToRotation_Validate(const FString& MapURL);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_AdminAddMapToRotationAtIndex(const FString& MapURL, int32 Idx);
	virtual void Server_AdminAddMapToRotationAtIndex_Implementation(const FString& MapURL, int32 Idx);
	virtual bool Server_AdminAddMapToRotationAtIndex_Validate(const FString& MapURL, int32 Idx);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_AdminRemoveMapFromRotation(const FString& MapURL);
	virtual void Server_AdminRemoveMapFromRotation_Implementation(const FString& MapURL);
	virtual bool Server_AdminRemoveMapFromRotation_Validate(const FString& MapURL);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_AdminRemoveMapFromRotationByIndex(int32 Idx);
	virtual void Server_AdminRemoveMapFromRotationByIndex_Implementation(int32 Idx);
	virtual bool Server_AdminRemoveMapFromRotationByIndex_Validate(int32 Idx);

	bool IsAdmin();
	
	// stream in level
	UFUNCTION(BlueprintCallable, Category = "Loading")
	bool StreamInLevel(const FString& NewLevel, const FString& Options, ULevelStreaming*& OutStreamedLevel, FLevelStreamOptions LevelStreamOptions);

private:
	// store these here so we can GC them
	UPROPERTY()
	TArray<ULevelStreaming*> StreamingLevels;

	UPROPERTY()
	UJoinSessionCallbackProxyAdvanced* JoinSession;
	
	UPROPERTY()
	FString StreamingLevel;
	UPROPERTY()
	FString StreamingOptions;

	UFUNCTION()
	void OnStreamedLevelLoadedComplete();

	FTimerHandle TH_OpenLevelSwtichDelay;
	UFUNCTION()
	void OnLevelOpen();
	UFUNCTION()
	void OnStreamedLevelLoadedExecuteOpen();

	FTimerHandle TH_OpenLevelRemoveLoadingScreenDelay;
	UFUNCTION()
	void OnStreamedLevelLoadedRemovingLoadingScreen();

	UFUNCTION()
	void UpdateMouseCursorVisibility();

public:
	UFUNCTION(BlueprintCallable, Category = "Loading")
	bool StreamInSession(FBlueprintSessionResult SessionResult, ULevelStreaming*& OutStreamedLevel, bool bShouldCreateLoadingScreen = true);

private:
	UFUNCTION()
	void OnSessionJoinSuccess();

	UFUNCTION()
	void OnSessionJoinFailed();

public:
	// if anything but empty the build is pirated
	FString bIsBuildPirated = "";

	void SetPiratedTrue();

	UFUNCTION(BlueprintCallable, Category = Maplist)
	void ReplicateMapListIfAdmin();
	UPROPERTY(ReplicatedUsing=OnRep_Maplist)
	TArray<FString> ReplicatedMapList;

	UFUNCTION()
	void OnRep_Maplist();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapListReplicated, const TArray<FString>&, MapList);
	UPROPERTY(BlueprintAssignable, Category = Maplist)
	FOnMapListReplicated OnMapListReplicated;

	UFUNCTION(BlueprintPure, Category = Maplist)
	TArray<FString> GetReplicatedMapRotation();	

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	ETeamType GetTeamType();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_SetTeamType(ETeamType NewTeam);
	virtual void Server_SetTeamType_Implementation(ETeamType NewTeam);
	virtual bool Server_SetTeamType_Validate(ETeamType NewTeam) { return true; }

	// Hold a referendum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Referendum)
	void Server_StartReferendum(TSubclassOf<class AReferendum> ReferendumClass);
	virtual void Server_StartReferendum_Implementation(TSubclassOf<class AReferendum> ReferendumClass);
	virtual bool Server_StartReferendum_Validate(TSubclassOf<class AReferendum> ReferendumClass) { return true; }

	// Hold a map referendum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Referendum)
	void Server_StartMapReferendum(TSubclassOf<class AMapReferendum> ReferendumClass, const FString& MapURL);
	virtual void Server_StartMapReferendum_Implementation(TSubclassOf<class AMapReferendum> ReferendumClass, const FString& MapURL);
	virtual bool Server_StartMapReferendum_Validate(TSubclassOf<class AMapReferendum> ReferendumClass, const FString& MapURL) { return true; }

	// Hold a player referendum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Referendum)
	void Server_StartPlayerReferendum(TSubclassOf<class APlayerReferendum> ReferendumClass, class AReadyOrNotPlayerState* TargetPlayer);
	virtual void Server_StartPlayerReferendum_Implementation(TSubclassOf<class APlayerReferendum> ReferendumClass, class AReadyOrNotPlayerState* TargetPlayer);
	virtual bool Server_StartPlayerReferendum_Validate(TSubclassOf<class APlayerReferendum> ReferendumClass, class AReadyOrNotPlayerState* TargetPlayer) { return true; }

	//UFUNCTION()
	//	void VoteYes() { Server_ReferendumVoteYes(); }

	//UFUNCTION()
	//	void VoteNo() { Server_ReferendumVoteNo(); }

	// Vote yes in the current referendum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Referendum)
	void Server_ReferendumVoteYes();
	virtual void Server_ReferendumVoteYes_Implementation();
	virtual bool Server_ReferendumVoteYes_Validate() { return true; }

	// Vote no in the current referendum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Referendum)
	void Server_ReferendumVoteNo();
	virtual void Server_ReferendumVoteNo_Implementation();
	virtual bool Server_ReferendumVoteNo_Validate() { return true; }

	// anti-spam guard for voting
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
	float VoteAntiSpamDebouncer = 5.0f;

	float VoteAntiSpamDebouncerCurrent = 0.0f;

	//Contains "CurrentVoteState" | "VoteEnabled" | "VoteReason"
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Vote)
	FVoteData MyVoteData;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Vote)
	void Vote(bool VoteYes);

	UFUNCTION(Client, Reliable, Category = Vote)
	void BeginVote(const FString& Reason, const FString& Question, bool bCanVoteNo);
	
	UFUNCTION(Client, Reliable, Category = Vote)
	void EndVote();

	UFUNCTION(Category = Vote)
	EVoteState GetVote();

	UFUNCTION(BlueprintImplementableEvent, Category = Vote)
	void AcknowledgeVote(FVoteData CurrentVoteData);

	UFUNCTION(BlueprintImplementableEvent, Category = Vote)
	void RequestVoteInput(FVoteData CurrentVoteData);
	
	UFUNCTION(BlueprintImplementableEvent, Category = Vote)
	void StopVoteInput();
	
	UFUNCTION(BlueprintImplementableEvent, Category = Events)
	void OnSaveLoadout();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = HUD)
	void Client_ClearHUDWidgets();
	virtual void Client_ClearHUDWidgets_Implementation();

	void RemoveLoadingScreen();
	bool HasLoadingScreenInViewport();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = HUD)
	void Client_HideHUDWidgets();
	virtual void Client_HideHUDWidgets_Implementation();
	
	UFUNCTION( BlueprintCallable, Client, Reliable, Category = Events)
	void Client_RemoveWidget(TSubclassOf<UUserWidget> Widget);
	virtual void Client_RemoveWidget_Implementation(TSubclassOf<UUserWidget> Widget);

	UFUNCTION(BlueprintCallable, Category = "MouseControl")
	void PassMouseControlToValidWidget();

	UFUNCTION(BlueprintCallable)
	void DisableForceShowMouseCursor();

	UFUNCTION(Exec)
	void DropAllSuspectWeapons();

	virtual void DisableInput(class APlayerController* PlayerController) override;

	UPROPERTY(BlueprintReadWrite)
	bool bStatsProfiledQueued = false;

	// Widget classes that will never be cleared..
	UPROPERTY(EditAnywhere, Category = Widgets)
	TArray<TSubclassOf<UUserWidget>> ProtectedWidgetClasses;

	UFUNCTION(BlueprintImplementableEvent, Category = Events)
	void BP_HandleMessage(FRChatMessage ChatMessage);

	UFUNCTION(Client, Reliable)
	void Client_PostLogin();
	virtual void Client_PostLogin_Implementation();

	bool bPreEscapeMenuMouseEnabled = false;

	TArray<FString> WidgetStack;

	UPROPERTY(BlueprintReadWrite)
	bool bCanOpenOptionsMenu = true;
	
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = HUD)
	void Client_CreateWidget(const FString& WidgetName, bool bForceAddToWidgetStack = false, bool bIsEscapeKey = false);
	virtual void Client_CreateWidget_Implementation(const FString& WidgetName, bool bForceAddToWidgetStack = false, bool bIsEscapeKey = false);

	UFUNCTION(BlueprintCallable)
	virtual UUserWidget* CreateWidgetForPlayer(const FString& WidgetName, bool bForceAddToWidgetStack = false, bool bIsEscapeKey = false);

	UFUNCTION(BlueprintCallable)
	bool RemoveWidgetFromStack(const FString& WidgetName);
	
	UPROPERTY()
	TMap<FString, UUserWidget*> CreatedWidgetMap;

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = HUD)
	void Client_CreateLoadingScreen(const FString& Map, const FString& Mode, const FString& SessionName = "", bool bIsSeamlessTravel = false);
	virtual void Client_CreateLoadingScreen_Implementation(const FString& Map, const FString& Mode, const FString& SessionName = "", bool bIsSeamlessTravel = false);

	void CreateSessionLoadingScreen(FBlueprintSessionResult SessionSearchResult);

	void RecreateOverlays();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_RespawnAsLobby(TSubclassOf<ASpectatorPawn> Class, FTransform SpawnTransform);
	virtual void Server_RespawnAsLobby_Implementation(TSubclassOf<ASpectatorPawn> Class, FTransform SpawnTransform);
	virtual bool Server_RespawnAsLobby_Validate(TSubclassOf<ASpectatorPawn> Class, FTransform SpawnTransform) { return true; }

    UFUNCTION(BlueprintImplementableEvent)
    void ShowCoopScoreChangeWidget(float ScoreChangeValue);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Gameplay)
	void Client_DisableUIMouse();
	virtual void Client_DisableUIMouse_Implementation();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Gameplay)
	void Client_SetControlRotation(FRotator NewControlRotation);
	virtual void Client_SetControlRotation_Implementation(FRotator NewControlRotation);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Gameplay)
	void Client_SetViewTargetWithBlend(APawn* NewViewTarget);
	virtual void Client_SetViewTargetWithBlend_Implementation(APawn* NewViewTarget);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = VIP)
	void Server_BecomeVIP();
	virtual void Server_BecomeVIP_Implementation();
	virtual bool Server_BecomeVIP_Validate() { return true; }

	UFUNCTION(BlueprintCallable, Category = VIP)
	void ReleaseVIP();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ReleaseVIP();
	virtual void Server_ReleaseVIP_Implementation();
	virtual bool Server_ReleaseVIP_Validate() { return true; }

	UFUNCTION(Client, Reliable)
	void Client_GetStats();
	virtual void Client_GetStats_Implementation();

	// temporarily store the server password for use
	FString ServerPassword;

	UFUNCTION(BlueprintCallable, Category = Password)
	void SetServerPasswordOnConnection(FString password);

	UFUNCTION(BlueprintCallable, Category = Password)
	FString GetPasswordOnConnection();
	
	UFUNCTION(BlueprintCallable, Category = Team)
	void SetPreferredTeamUniqueNetIdOnConnection(FString UniqueId);

	UFUNCTION(BlueprintCallable)
	FString GetPrefferredTeamUniqueNetIdOnConnection();

	// ##UE5UPGRADE## Zeus
	//UPROPERTY(Replicated)
	//FZeuzMatchMakingServerInfo Rep_MatchmakingServerInfo;
	//UPROPERTY(Replicated)
	//FZeuzMatchMakingPartyInit Rep_MatchmakingPartyInit;

	UFUNCTION(Exec)
	void ResetKeybinds();

	UFUNCTION(Exec)
	void DestroyAllDynamicLights();
	
	UFUNCTION(Exec)
	void TestMatchmakingServerRefresh();
	
	UFUNCTION(Exec)
	void ResetAI(float Range = 1500.0f);

	UFUNCTION(Exec)
	void DisplayAllPlayingFMODEvents();

	UFUNCTION(Exec)
	void TestMatchmakingServerCreateParty();

	UFUNCTION(Exec)
	void TeleportToNextRemainingAI();

	UFUNCTION(Exec)
	void StartSwatAutomation();

	UPROPERTY()
	class ASwatAutomationManager* SwatAutomationManager;

	UFUNCTION(Exec)
	void StopSwatAutomation();
	
	void DebugGatherDebugActors();
	void DebugDrawDebugActors(float DeltaTime);
	UPROPERTY()
	TMap<AActor*, float> DebugActorList;

	void OnArrest();

	/** Achievements write object */
	FOnlineAchievementsWritePtr WriteObject;

	UFUNCTION(Client, Reliable)
	void UpdateAchievementProgress(const FString& Id, float Percent);

	/**
 * Reads achievements to precache them before first use
 */
	void QueryAchievements();

	/**
 * Called when the read achievements request from the server is complete
 *
 * @param PlayerId The player id who is responsible for this delegate being fired
 * @param bWasSuccessful true if the server responded successfully to the request
 */
	void OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful);

	/** notify player about finished match */
	virtual void ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner);

	// Centerprint
	//UFUNCTION(BlueprintCallable)
	//void CenterPrint(FName Type, float Duration, class APlayerCharacter* other);

	UFUNCTION(BlueprintCallable)
	void StartSpeaking();
	UFUNCTION(BlueprintCallable)
	void StopSpeaking();
	void CycleVoiceChannel();

	UFUNCTION(reliable, client)
	void ClientJoinVoice(const FString &OnlineSessionId, const int32 &TeamNum = -1);

	bool bPendingJoinVoice = false;
	FString PendingVoiceSessionId;
	int32 PendingVoiceTeamNum;

	UFUNCTION(BlueprintCallable, Category = Chat)
	void SendChatMessage(FRChatMessage ChatMessage);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHudWidgetsCleared);
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Events")
	FOnHudWidgetsCleared OnHudWidgetsClearedComplete;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingScreenCleared);
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Events")
	FOnLoadingScreenCleared OnLoadingScreenCleared;

	AReadyOrNotPlayerState* GetRoNPlayerState();

	/** Notify client they were kicked from the server */
	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Gameplay)
	void BP_ClientWasKicked(const FText& KickReason);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Planning)
	bool CanSetSpawn(ETeamType Team, ESelectedSpawn NewSpawnPoint, bool bSameSpawn = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Planning)
	bool CanEnableDeployable(int32 DeployableNum);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Planning)
	bool CanDisableDeployable(int32 DeployableNum);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Planning)
	bool CanEnablePersonnel(int32 PersonnelNum, int32 MapPointNum);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Planning)
	bool CanDisablePersonnel(int32 PersonnelNum);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Planning)
	void Server_SetSpawn(ETeamType SpawnTeam, ESelectedSpawn NewSpawnPoint);
	virtual void Server_SetSpawn_Implementation(ETeamType SpawnTeam, ESelectedSpawn NewSpawnPoint);
	virtual bool Server_SetSpawn_Validate(ETeamType SpawnTeam, ESelectedSpawn NewSpawnPoint) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Planning)
	void Server_EnableDeployable(int32 DeployableNum);
	virtual void Server_EnableDeployable_Implementation(int32 DeployableNum);
	virtual bool Server_EnableDeployable_Validate(int32 DeployableNum) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Planning)
	void Server_DisableDeployable(int32 DeployableNum);
	virtual void Server_DisableDeployable_Implementation(int32 DeployableNum);
	virtual bool Server_DisableDeployable_Validate(int32 DeployableNum) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Planning)
	void Server_EnablePersonnel(int32 PersonnelNum, int32 MapPointNum);
	virtual void Server_EnablePersonnel_Implementation(int32 PersonnelNum, int32 MapPointNum);
	virtual bool Server_EnablePersonnel_Validate(int32 PersonnelNum, int32 MapPointNum) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Planning)
	void Server_DisablePersonnel(int32 PersonnelNum);
	virtual void Server_DisablePersonnel_Implementation(int32 PersonnelNum);
	virtual bool Server_DisablePersonnel_Validate(int32 PersonnelNum) { return true; }

	UFUNCTION(NetMulticast, Reliable, Category = Planning)
	void Multicast_SetPersonnelAtPoint(int32 PersonnelNum, int32 MapPointNum);
	virtual void Multicast_SetPersonnelAtPoint_Implementation(int32 PersonnelNum, int32 MapPointNum);

	UFUNCTION(NetMulticast, Reliable, Category = Planning)
	void Multicast_RemovePersonnelAtPoint(int32 PersonnelNum);
	virtual void Multicast_RemovePersonnelAtPoint_Implementation(int32 PersonnelNum);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "SQL Commands")
	void Server_SwapPlayersTeam(APlayerState* ps);
	virtual void Server_SwapPlayersTeam_Implementation(APlayerState* ps);
	virtual bool Server_SwapPlayersTeam_Validate(APlayerState* ps) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Deployable")
	void Server_SetDeployableDepot(int32 NewDepot);
	virtual void Server_SetDeployableDepot_Implementation(int32 NewDepot);
	bool Server_SetDeployableDepot_Validate(int32 NewDepot) { return true; };

	UFUNCTION(BlueprintCallable, Category = "Deployable")
	bool CanSetDepotTo(int32 NewDepot, bool bSameDepot = false);

	UFUNCTION(BlueprintCallable, Reliable, NetMulticast, Category = "Planning")
	void Multicast_ForcePlanningRefresh();
	virtual void Multicast_ForcePlanningRefresh_Implementation();

	UFUNCTION(Exec)
	void PrintAllSceneComponentWhoseOriginIsZero();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestLoadoutChange(FSavedLoadout Loadout);

	UFUNCTION(BlueprintCallable, Exec, Category = "Console Command")
	void SpawnAI(FString TableName = "", int32 Count = 1);

	UFUNCTION(Exec, Category = "Console Command")
	void StartMatchmaking();

	UFUNCTION(BlueprintCallable, Exec, Category = "Console Command")
	void SpawnRandomAI(int32 Count = 1);

	UFUNCTION(BlueprintCallable)
	void SpawnAIAtLocation(FString TableName, FVector Location, FVector MoveLocation);

	UFUNCTION(Exec, Category = "Console Command")
	void FlushDeadBodies();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroyAllItems();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroyAllItemMeshes();

	UFUNCTION(Exec, Category = "Console Command")
	void HideAllItemMeshes();

	UFUNCTION(Exec, Category = "Console Command")
	void UnhideAllItemMeshes();

	UFUNCTION(Exec, Category = "Console Command")
	void HideAllSmallItemMeshes();

	UFUNCTION(Exec, Category = "Console Command")
	void UnhideAllSmallItemMeshes();

	UFUNCTION(Exec, Category = "Console Command")
	void HideAllWeaponAttachments();

	UFUNCTION(Exec, Category = "Console Command")
	void UnhideAllWeaponAttachments();

	UFUNCTION(Exec, Category = "Console Command")
	void SwapAllItemSkeletalMeshesToCubes();

	UFUNCTION(Exec, Category = "Console Command")
	void DisableAllItemMaterials();

	UFUNCTION(Exec, Category = "Console Command")
	void DisableAllMaterials();

	UFUNCTION(Exec, Category = "Console Command")
	void DisableSkeletalMeshShadowCasting();
		
	UFUNCTION(Exec, Category = "Console Command")
	void PlayAnimationFromLookupTable(FString AnimationRowName);

	UFUNCTION(Exec, Category = "Console Command")
	void Kill(float time);

	UFUNCTION(Exec, Category="ConsoleCommand")
	void TeleportUp();
	
	UFUNCTION(Exec, Category="ConsoleCommand")
	void SetClubMusicMasterVolume(float Volume);

	UPROPERTY(EditAnywhere)
	USkeletalMesh* TestCube;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* TestMaterial;

	UFUNCTION(Exec, Category = "Console Command")
	void TakeDamageExec(float DamageAmount);

	UFUNCTION(Exec, Category = "Console Command")
	void KillSWATTeam();

	UFUNCTION(Exec, Category = "Console Command")
	void SavePerformanceStats();
	
	UFUNCTION()
	void InternalSaveStats();

	UFUNCTION(Exec, Category = "Console Command")
	void ExecuteLineTracePerfTest();

	UFUNCTION(Exec, Category = "Console Command")
	void ExecuteSweepTraceSinglePerfTest();

	UFUNCTION(Exec, Category = "Console Command")
	void ExecuteSweepTraceMultiPerfTest();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroyAllAI();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroyAllSuspects();


	UFUNCTION(Exec, Category = "Console Command")
	void DestroySwatTeam();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroySwatInventoryItems();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroySwatControllers();

	UFUNCTION(Exec, Category = "Console Command")
	void DestroySwatAnimation();

	UFUNCTION(Exec, Category = "Console Command")
	void DisableAllItemTicks();

	UFUNCTION(Exec, Category = "Console Command")
	void ServerStatFileStart();

	UFUNCTION(Exec, Category = "Console Command")
	void ServerStatFileStop();
	
	UFUNCTION(Exec, Category = "Console Command")
	void SetTimelimit(float Time);

	UFUNCTION(Exec, Category = "Console Command")
	void SetScorelimit(int32 Score);

	UFUNCTION(Exec, Category = "Console Command")
	void DeleteAnyNonMeshComponents();

	UFUNCTION(Exec, Category = "Console Command")
	void SetAllComponentsUseParentsBounds();

	UFUNCTION(Exec, Category = "Console Command")
	void RemoveHUD();
	
	UFUNCTION(BlueprintCallable, Category = HUD)
	void ShowHUD();
	UFUNCTION(BlueprintCallable, Category = HUD)
	void HideHUD();

	UFUNCTION(Exec, Category = "Console Command")
	void FreeVIP();

	UFUNCTION(Exec, Category = "Console Command")
	void GibAllComponents();


	FTimerHandle TriggerInvalidateWidgets_Handle;
	void TriggerInvalidateWidgets();

	UFUNCTION(Reliable, Server, WithValidation)
	void Server_StatFile(bool bStartStatFile);
	virtual void Server_StatFile_Implementation(bool bStartStatFile);
	virtual bool Server_StatFile_Validate(bool bStartStatFile) { return true; }

	// UFUNCTION(Client, Reliable)
	// 	void ClientRequestSteamAuth();
	//
	// UFUNCTION(Server, Reliable, WithValidation)
	// 	void ServerBeginSteamAuth(FSteamAuthenticationToken AuthToken);
	//
	// void CancelSteamAuthForPlayerController();

	// //A AuthTickect client should keep track of, used when client log off 
	// HAuthTicket m_hAuthTicket;
	// FSteamAuthenticationToken ClientsAuthToken;

	// //Get called on server when authentication complete with result.
	// STEAM_GAMESERVER_CALLBACK_MANUAL(AReadyOrNotPlayerController, OnAuthTicketResponse, ValidateAuthTicketResponse_t, OnAuthTicketResponseCallback);

	UPROPERTY(BlueprintReadOnly)
	bool bStartedCoopAsSpectator = false;

	void SetLastKilledCharacter(ACharacter* KilledCharacter);

	void DestroyLastKilledCharacter();
protected:

	UPROPERTY()
	ACharacter* LastKilledCharacter = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChatMessage(FRChatMessage ChatMessage);
	virtual void Server_SendChatMessage_Implementation(FRChatMessage ChatMessage);
	virtual bool Server_SendChatMessage_Validate(FRChatMessage ChatMessage) { return true; }

	virtual void UpdateRotation(float DeltaTime) override;
	virtual FRotator GetDesiredRotation() const override;


	/** for saving Anti-Aliasing and Motion-Blur settings during Pause State */
	int32 PreviousAASetting;
	int32 PreviousMBSetting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfig_High;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfigADS_High;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfig_Medium;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfigADS_Medium;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfig_Low;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfigADS_Low;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfig_Off;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim Assist")
	UUASAimAssistConfigDataAsset* AimAssistConfigADS_Off;

	UFUNCTION(BlueprintCallable)
	void SetAimAssistIntensity(FString NewAimAssistIntensity);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Replay")
	bool bIsReplaySpectator = false;

	//DEMO CONTROLLER CONTROLS

	bool bMoveUp = false;
	void MoveUp();
	void StopMovingUp();

	bool bMoveDown = false;
	void MoveDown();
	void StopMovingDown();

	FRotator DemoRecorderRotation = FRotator::ZeroRotator;
	void AddYaw(float Val);
	void AddPitch(float Val);

	FVector DemoRecorderLocation = FVector::ZeroVector;
	void MoveForward(float Val);
	void MoveRight(float Val);

	float SpeedMultiplier = 1.0f;
	void AdjustSpeed(float Val);

	virtual void SetPlayer(UPlayer* InPlayer);

	FORCEINLINE class UProgressionComponent* GetProgressionComponent() const { return ProgressionComp; }

	UFUNCTION(Exec, Category = "Console Command")
	void Equip(FString ItemName);

	bool bDrawScopeAlignmentTrace = false;
	UFUNCTION(Exec, Category = "Console Command")
	void EnableScopeAlignmentTool();

	UFUNCTION(Exec, Category = "Console Command")
	void ApplyWeaponSkin(FString SkinName);

	UFUNCTION(Exec, Category = "Console Command")
	void RemoveWeaponSkin();

	UFUNCTION(Exec, Category = "Console Command")
	void ApplyCharacterSkin(FString SkinName);

	UFUNCTION(Exec, Category = "Console Command")
	void RemoveCharacterSkin();

	UFUNCTION(Exec, Category = "Console Commands")
	void GiveSWATRam();

	UFUNCTION(Exec, Category = "Console Commands")
	void WinCoop();

	UFUNCTION(Client, Reliable)
	void SetHostMigrationComplete();
	
	UFUNCTION(Exec, Category = "Console Commands")
	void DebugMigrateHost();

	UFUNCTION(Client, Reliable)
	void DebugClientStartHostMigration();
				
	UFUNCTION(Exec, Category = "Console Commands")
	void EquipAndDropEvidence();

	UFUNCTION(Exec, Category = "Console Commands")
	void SoftWinCoop();

	UFUNCTION(Exec, Category = "Console Commands")
	void KnockoutAllEnemies();

	UFUNCTION()
	void ArrestAll(ETeamType TargetTeam);
	
	UFUNCTION(Exec, Category = "Console Commands")
	void ArrestAllSuspects() { ArrestAll(ETeamType::TT_SUSPECT); }

	UFUNCTION(Exec, Category = "Console Commands")
	void ArrestAllCivilians() { ArrestAll(ETeamType::TT_CIVILIAN); }

	UFUNCTION(Exec, Category = "Console Commands")
	void SecureAllEvidence() const;
	
	UFUNCTION()
	void KillAll(ETeamType TargetTeam);
	
	UFUNCTION(Exec, Category = "Console Commands")
	void KillAllSuspects() { KillAll(ETeamType::TT_SUSPECT); }

	UFUNCTION(Exec, Category = "Console Commands")
	void KillAllCivilians() { KillAll(ETeamType::TT_CIVILIAN); }

	UFUNCTION()
	void ReportAll(ETeamType Team);

	UFUNCTION(Exec, Category = "Console Commands")
	void ReportAllSuspects() { ReportAll(ETeamType::TT_SUSPECT); }
	
	UFUNCTION(Exec, Category = "Console Commands")
	void ReportAllCivilians() { ReportAll(ETeamType::TT_CIVILIAN); }
	
	UFUNCTION(Exec, Category = "Console Commands")
	void PlayDeadAllEnemies();

	UFUNCTION(Exec, Category = "Console Commands")
	void DestroyAllWidgets();

	UFUNCTION(Exec, Category = "Console Commands")
	void ToggleRTXDMO();
	
	UFUNCTION(Exec, Category = "Console Commands")
	void ArrestOne();
	
	UFUNCTION(Exec, Category = "Console Commands")
	void ToggleGodModeOnEveryone();

	UFUNCTION(Exec, Category = "Console Commands")
	void SpawnBotsForGame(int32 BotCount = 2);
	
	UFUNCTION(Exec, Category = "Console Commands")
	void TestSendCrash();

	UFUNCTION(Exec, Category = "Console Commands")
	void PS5ActivityStart(FString& Activity);

	UFUNCTION(Exec, Category = "Console Commands")
	void PS5ActivityEnd(FString& Activity, int Outcome);
	
	UFUNCTION(Exec, Category = "Console Commands")
	void PS5ActivitySetAvailable(FString& Activity, bool bAvailable = true);

	UFUNCTION(Exec, Category = "Console Commands")
	void PS5ResetAllActiveActivities();

	UFUNCTION(Exec, Category = "Console Commands")
	void ConsoleSetMaxPlayers(int MaxPlayers);
	
	bool bRTXDMOOn;
			
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRTXDMOChanged, bool, bRTXOn);
	UPROPERTY(BlueprintAssignable)
	FOnRTXDMOChanged OnRTXDMOChanged;

	UFUNCTION(Exec)
	void PrintGPUBrand();

	UFUNCTION(BlueprintCallable, Category = Steamworks)
	bool SaveFileToUserCloud(FString FullPath);

	void OnFileSavedTouserCloud(bool bWasSuccessful, const FUniqueNetId& UniqueNetId, const FString& FilePath);

	UFUNCTION(Exec)
	void MakeCrash();

	UFUNCTION(BlueprintPure)
	virtual bool IsCameraFading();

	void PossessOrbitalCamera();

	void SpectateNextPlayer();
	void SpectatePreviousPlayer();

	UPROPERTY()
	TArray<FRChatMessage> ChatMessages;
	UFUNCTION(BlueprintPure)
	void RetrieveChatLog(TArray<FRChatMessage>& OutMessages);
	UFUNCTION(BlueprintCallable)
	void SaveChatMessage(FRChatMessage Message);

	UPROPERTY(BlueprintReadWrite, Category = "Aim Assist")
	float AimAssistPitch = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Aim Assist")
	float AimAssistYaw = 1.0f;

private:
	/** Handle for efficient management of ClientStartOnlineGame timer */
	FTimerHandle TimerHandle_ClientStartOnlineGame;

	// dualsense gyro
	bool bWasAimingLastFrame = false;
	FVector PreviousTiltOffset;

	UFUNCTION(Exec)
	void SetRosterStressValue(float Stress);

	UFUNCTION(Exec)
	void RosterInstantFinishStatus();
	
public:
	UFUNCTION(Server, Reliable)
	void Server_AddMarker(const FPlanningMarker& Marker);

	UFUNCTION(Server, Reliable)
	void Server_RemoveMarker(int32 ID);
	
	UFUNCTION(Server, Reliable)
	void Server_AddLine(const FPlanningLine& Line);

	UFUNCTION(Server, Reliable)
	void Server_RemoveLine(int32 ID);
	
	UPROPERTY()
	TMap<int32, FProjectileData> ActiveProjectiles;

	UFUNCTION()
	void OnLocallyFiredProjectile(ABulletProjectile* Projectile, int32 ProjectileIdentifier);

	UFUNCTION(Client, Reliable, Category = Networking)
	void Client_OnProjectileValidation(int32 ProjectileSeed, EServerValidationState Validation);

	// Lobby stuff
	bool bIsReady = false;

	UFUNCTION()
	void ReadyUp();
	UFUNCTION()
	void ReadyUp_Failsafe();
	
	UFUNCTION(Server, Reliable, Category = "MissionSelect")
	void Server_SetReadyState(bool bReady);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerReadyChange, AReadyOrNotPlayerController*, PlayerController, bool, bReady);
	UPROPERTY(BlueprintAssignable)
	FOnPlayerReadyChange OnPlayerReadyChange;

	UFUNCTION(Client, Reliable, Category="MissionSelect")
	void Client_LocalReadyStateChanged(bool bReady);

private:
	TArray<float> RoundTripTimeBuffer;
	int32 RoundTripTimeBufferIndex = 0;

	float ServerTimeDelta;

	UFUNCTION(Server, Unreliable)
	void Server_RequestWorldTime(float ClientTime);

	UFUNCTION(Client, Unreliable)
	void Client_UpdateWorldTime(float ServerTime, float SentClientTime);

	void SetActiveAimAssistConfig(UUASAimAssistConfigDataAsset* AimAssistConfig);

	void LoadAimAssistIntensity();

	UUASAimAssistConfigDataAsset* GetAimAssistConfigByIntensity(const FString& Intensity, bool bAds);

	UPROPERTY();
	FString AimAssistIntensity = "off";

public:
	virtual void PostNetInit() override;

	float GetSynchronizedWorldTime() const;

	void SetAimDownSightsAimAssist(bool Enable);
	
	UPROPERTY(BlueprintReadOnly, Category="PauseMenu")
	class UPauseMenu_Wrapper* PauseMenu = nullptr;
};
