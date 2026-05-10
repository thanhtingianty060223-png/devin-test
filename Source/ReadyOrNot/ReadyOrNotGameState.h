// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/GameState.h"
#include "HUD/Widgets/DebugDisplayWidget.h"
#include "Structs.h"
#include "Data/AIData.h"
#include "Data/LevelData.h"
#include "Data/TocSpeechData.h"
#include "Interfaces/ListenForDeath.h"
#include "Interfaces/ListenForGameStart.h"
#include "Interfaces/ListenForGameEnd.h"
#include "Interfaces/IHttpRequest.h"

#include "Objectives/Objective.h"
#include "HUD/Widgets/PlanningMapWidget.h"
#include "Info/HostMigrationManager.h"

#include "ReadyOrNotGameState.generated.h"

UENUM(BlueprintType)
enum class ESelectedSpawn : uint8
{
	SS_None,
	SS_FirstSpawn,
	SS_SecondSpawn,
	SS_ThirdSpawn,
	SS_FourthSpawn
};

UENUM(BlueprintType)
enum class EPlayerStatus : uint8
{
	PS_None,
	PS_NotReady,
	PS_Ready,
	PS_Deployed
};

USTRUCT(BlueprintType)
struct FDeploymentStatus
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
		FText Position;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
		EPlayerStatus Status;

	UPROPERTY(BlueprintReadOnly, Category = "Deployment")
		class AReadyOrNotPlayerState* PlayerState;
};


USTRUCT(BlueprintType)
struct FKillFeedData
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly)
		FString Name;

	UPROPERTY(BlueprintReadOnly)
		EKillfeedType Type;
};

USTRUCT(BlueprintType)
struct FRChatMessage
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		FString SenderName;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		class AReadyOrNotPlayerState* SenderPlayerState = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		FString Message;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		FLinearColor Color;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		FDateTime TimeStamp;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		FString Args;


	// Used for PvP modes to state which team the killer is on (TargetTeam states who is affected by the message, and this didn't fit)
	UPROPERTY(BlueprintReadWrite, Category = Chat)
		ETeamType AssociatedTeam;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		bool bKillfeed = false;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		bool bCommand = false;

	// IF should send to single PC, if valid does not send to target team.
	UPROPERTY(BlueprintReadWrite, Category = Chat)
		APlayerController* TargetPlayerController = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Chat)
		ETeamType TargetTeam;

	FRChatMessage()
		: SenderName(""), Message(""), Color(FLinearColor::White), TargetPlayerController(nullptr), TargetTeam(ETeamType::TT_NONE)
	{}

	FRChatMessage(const FString& InSenderName, const FString& InMessage, const FLinearColor& InColor, APlayerController* InTarget, ETeamType InTeamType = ETeamType::TT_NONE)
		: SenderName(InSenderName), Message(InMessage), Color(InColor), TargetPlayerController(InTarget), TargetTeam(InTeamType)
	{}


};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMissionObjectivesDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerStateDelegate, APlayerState*);

DECLARE_STATS_GROUP(TEXT("ReadyOrNotGameState"), STATGROUP_ReadyOrNotGameState, STATCAT_Advanced);

UCLASS()
class READYORNOT_API AReadyOrNotGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AReadyOrNotGameState();

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	FPlayerStateDelegate OnPlayerStateAdded;
	FPlayerStateDelegate OnPlayerStateRemoved;
	
	bool bHasEverPlayedGameIntroRules = false;

	void LoadDataTables();

	// Used for GetRandomInRangeFromStream
	UPROPERTY(ReplicatedUsing=OnRep_StreamSeed)
	int32 RandomStreamSeed = 0;

	FRandomStream RandomStream;

	FRoomGenData* RoomData = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 NumSuspectsActive = 0;
	UPROPERTY(BlueprintReadOnly)
	uint8 NumCiviliansActive = 0;
	UPROPERTY(BlueprintReadOnly)
	uint8 NumSwatActive = 0;

	bool bCharactersDirty = false;
	void UpdateActiveControllers();

	UPROPERTY()
	class UReadyOrNotVoiceConfig* VoiceConfig = nullptr;

	bool bGlobalSuspendVoiceOver = false;

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	void SetGlobalSuspendVoiceOver(bool bEnable);
	
	UPROPERTY(BlueprintReadOnly)
	class ULoadout_V2* Loadout_V2;
	
	UPROPERTY(BlueprintReadOnly)
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;
	
	UFUNCTION()
	void OnRep_StreamSeed();

	int32 GetPlayerCharactersNum();

	UPROPERTY(ReplicatedUsing=OnRep_NextHost)
	APlayerState* NextHost;
	
	UPROPERTY(ReplicatedUsing=OnRep_NextHost)
	FString MigrationGUID;

	UFUNCTION()
	void OnRep_NextHost();

	float TimeUntilRefreshNextHost = 0.0f;
	void RefreshNextHost();
	
	UPROPERTY(Replicated)
	bool bHasHostFinishedLoading = false;

	UPROPERTY(BlueprintReadOnly)
	TArray<ABaseItem*> AllItems;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class AEvidenceActor*> AllEvidenceActors;

	UPROPERTY(BlueprintReadOnly)
	TArray<class AReportableActor*> AllReportableActors;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class ATrapActorAttachedToDoor*> AllDoorTrapActors;

	UPROPERTY(BlueprintReadOnly)
	TArray<class ACoverLandmark*> AllCoverLandmarks;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class AWallHoleTraversal*> AllWallHoles;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<class APairedInteractionDriver*> AllPairedInteractionActors;

	/*
	UFUNCTION()
	void OnAnimNotifyPoolParticleFinished(UParticleSystemComponent* PSystem);

	UPROPERTY()
	TArray<class UParticleSystemComponent*> ParticleComponentPool_AnimNotify_Inactive;
	UPROPERTY()
	TArray<class UParticleSystemComponent*> ParticleComponentPool_AnimNotify_Active;

	UParticleSystemComponent* GetAvailableParticleComponent_AnimNotify();
	*/

	// Don't allow these to be GC'd as they are slow to reload
	UPROPERTY()
	TArray<UDataTable*> LoadedDataTables;
	
	int32 GetNumPlayers();

	UFUNCTION(BlueprintPure)
	TArray<AReadyOrNotPlayerState*> GetPlayersAvailableForVote() const;
	
	UFUNCTION(BlueprintPure)
	TArray<AReadyOrNotPlayerController*> GetControllersAvailableForVote() const;
	
	FORCEINLINE class AScoringManager* GetScoringManager() const { return ScoringManager; }
	
	UPROPERTY(Replicated)
	class AScoringManager* ScoringManager = nullptr;

	UPROPERTY()
	TMap<FString, class AAIFactionManager*> AIFactionManagers;
	
	UFUNCTION()
	void OnAlphaAccessChecked(bool bBanned, FString BanReason);

	void CreateLevelObjectives();

	UPROPERTY(Replicated)
	FDataTableRowHandle Rep_GameModeSettings;

	UFUNCTION(BlueprintCallable)
	FGameModeSettings GetGameModeSettings();

	UFUNCTION(BlueprintCallable, Category = Scoring)
		void GetPlayerStatesOnTeamOrderedByScore(ETeamType Team, TArray<AReadyOrNotPlayerState*>& PlayerStates);

	// countdown to game time start once everyone is ready
	UPROPERTY(BlueprintReadOnly, Replicated)
	float TimeTillGameStartCountdown = 10.0f;
	bool bNearReadyFadenIn = false;

	float InterpToTimeDialtion = 1.0f;

	#if (WITH_EDITOR || UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
	void TickBadAIAction();
	#endif
	
	UPROPERTY()
	TArray<class ABadAIAction*> BadAIActionActors;

	UFUNCTION(BlueprintCallable, Category = "Bad AI Action")
	class ABadAIAction* GetMostRecentBadAIActionReport() const;

	virtual void PreGamePlayingStateLogic();

	UFUNCTION(BlueprintCallable, Category = "Loadout")
	virtual void OnLoadoutFinished();

	UPROPERTY(BlueprintReadOnly)
	bool bHasLeftLoadOut = false;
	
	// Returns true if loaded
	bool bCompletedInitialLoad = false;
	virtual bool TryLoadSubPreMissionPlanning();
	void LoadSubPreMissionPlanningFromTimer() { TryLoadSubPreMissionPlanning(); }
	
	UPROPERTY(BlueprintReadOnly)
	ULevelStreaming* PreMissionStreamedLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> SubPreMissionPlanningLevel;


	FTimerHandle SerialCheck_Handle;
	void SerialCheck();

	UFUNCTION(BlueprintCallable, Category = "URL")
		FString GetMapURL();

	// Mode name that shows up in PvP pregame menu
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Info", meta = (multiline = true))
	FText ModeName = FText::FromString(FString("Game Mode"));

	// Description for the mode. Should include info on how to play the mode.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Info", meta = (multiline = true))
	FText ModeRules = FText::FromString(FString("This game mode doesn't have any rules written for it. Try writing them in the Game State."));

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetModeText() const { return ModeName; }
	
	// The mission objectives that have spawned. (FIXME: Put this in CO-OP so it's co-op only?)
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_MissionObjectives, BlueprintReadWrite, Category="Info")
	TArray<AObjective*> MissionObjectives;

	UPROPERTY(BlueprintAssignable)
	FMissionObjectivesDelegate OnMissionObjectivesUpdated;
	
	UFUNCTION()
	void OnRep_MissionObjectives() { OnMissionObjectivesUpdated.Broadcast(); }
	
	// Obituary data
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Obituary")
	class UObituaryData* ObituaryData;

	// If true, we announce reinforcements when we are in this mode
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reinforcements")
	bool bAnnounceReinforcements = true;
		
	// If true, this mode has radio glare enabled
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
		bool bRadioGlareEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Replenish Ammo")
	UFMODEvent* ReplenishAllAmmoSound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<class ASpectatePawn*> SpectatePawns;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "General")
	TArray<AReadyOrNotCharacter*> RedTeamPlayers;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "General")
	TArray<AReadyOrNotCharacter*> BlueTeamPlayers;
	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<class AThrownItem*> AllThrownItems;
	
	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<class ADoor*> AllDoors;
	
	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<AReadyOrNotCharacter*> AllReadyOrNotCharacters;
	
	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<ACyberneticCharacter*> AllAICharacters;
	
	UPROPERTY(BlueprintReadOnly, Category = "General")
	TArray<UInteractableComponent*> AllInteractableComponents;

	void PlayAnnouncerForTeam(FString SpeechRowName, ETeamType TeamType);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAnnouncerForTeam(const FString& SpeechRowName, ETeamType TeamType);

	UPROPERTY()
	UFMODAudioComponent* AnnouncerAudioComponent; 

	UPROPERTY(Replicated)
	int32 TotalMissionAbuseCount = 0;

	UFUNCTION(BlueprintPure)
	FORCEINLINE int32 GetTotalMissionAbuseCount() const { return TotalMissionAbuseCount; }

	UPROPERTY(BlueprintReadOnly, Category = "Info")
	TArray<TScriptInterface<IListenForDeath>> DeathListeners;

	// listeners for game start
	UPROPERTY(BlueprintReadOnly, Category = "Info")
	TArray<TScriptInterface<IListenForGameStart>> GameStartListeners;
	
	UPROPERTY(BlueprintReadOnly, Category = "Info")
	TArray<TScriptInterface<IListenForGameEnd>> GameEndListeners;

	// Add/Remove a Death Listener
	UFUNCTION(BlueprintCallable, Category = "Info")
	void AddDeathListener(TScriptInterface<IListenForDeath> DeathListener);

	UFUNCTION(BlueprintCallable, Category = "Info")
	void RemoveDeathListener(TScriptInterface<IListenForDeath> DeathListener);

	// Add/Remove a Game Start listener
	UFUNCTION(BlueprintCallable, Category = "Info")
		void AddGameStartListener(TScriptInterface<IListenForGameStart> GameStartListener);

	UFUNCTION(BlueprintCallable, Category = "Info")
		void RemoveGameStartListener(TScriptInterface<IListenForGameStart> GameStartListener);

	// Add/Remove a Game End listener
	UFUNCTION(BlueprintCallable, Category = "Info")
        void AddGameEndListener(TScriptInterface<IListenForGameEnd> GameEndListener);

	UFUNCTION(BlueprintCallable, Category = "Info")
        void RemoveGameEndListener(TScriptInterface<IListenForGameEnd> GameEndListener);

	//hack to change display name when starting single player game 
	UFUNCTION(BlueprintCallable, Category = "Info")
	void OverWriteModeNameText(FText newModeName);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnCharacterDied(class AReadyOrNotCharacter* Victim, class AReadyOrNotCharacter* Killer, class AActor* Inflictor);
	void Multicast_OnCharacterDied_Implementation(class AReadyOrNotCharacter* Victim, class AReadyOrNotCharacter* Killer, class AActor* Inflictor) { OnCharacterDied(Victim, Killer, Inflictor); }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnGameStarted();
	void Multicast_OnGameStarted_Implementation() { OnGameStarted(); }

	UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnGameEnded();
	void Multicast_OnGameEnded_Implementation() { OnGameEnded(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
		TArray<FDeploymentStatus> GetDeploymentStatusOfPlayers();

	// Whether the equipment menu is disabled in this mode.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Info")
	bool bDisableEquipment = false;

	// Whether commands should send chat in this mode (should be disabled for singleplayer modes!)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Info")
		bool bEnableCommandChat = true;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = VIP)
		bool bUseReinforcements = false;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
		float Reinforcements_TimeRemaining;

	UFUNCTION()
		void OnAuthenticationResponse(bool bSuccess, bool bSerialFound, bool bSerialValid, FString failedReason);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Warmup")
		bool bRunWarmup = true;

	UPROPERTY(EditAnywhere, Category = "Required!")
	TSubclassOf<AActor> SceneCapturePlayerCameraClass;

	UPROPERTY(EditAnywhere, Category = "Gameplay")
		bool bShowEnemiesAsSuspects = false;

	UFUNCTION(Client, Reliable)
			void Client_BindCharacterEvents(APlayerCharacter* Character);
	virtual void Client_BindCharacterEvents_Implementation(APlayerCharacter* Character);

	UFUNCTION()
	void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

	UFUNCTION()
	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter);

	UPROPERTY(BlueprintReadWrite)
	TArray<FKillFeedData> KillfeedData;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUpdateKillFeed, AActor*, Causer, ACharacter*, InstigatorCharacter, ACharacter*, KilledCharacter);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnUpdateKillFeed OnUpdateKillFeed;

	UFUNCTION(BlueprintPure, Category = Scoring)
		virtual float GetWinningScore(bool& bUsesScoring);

	UFUNCTION(BlueprintPure, Category = Scoring)
		float GetTeamScore(ETeamType Team);

	UFUNCTION(BlueprintPure, Category = Scoring)
		TArray<class AReadyOrNotPlayerState*> GetPlayerStatesOfTeam(ETeamType Team);

	UFUNCTION(BlueprintCallable)
	virtual void LoadStartupWidgetsAfterLoadingScreen();

	
	// how long until next map, displayed on the map end screen
	UPROPERTY(Replicated, BlueprintReadOnly)
	float ServerTimeUntilNextMap = 0.0f;


	bool bSpawnedMatchStartWidget = false;
	UPROPERTY(EditAnywhere)
	FString GameRulesIntroAnnouncerRowName;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = UI)
		bool bWaitingForPlayers = false;

	UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = UI)
	float PlanningTimeLeft = 480;

	UPROPERTY(ReplicatedUsing=OnRep_WinsUpdated, BlueprintReadOnly, EditAnywhere, Category = UI)
		int32 RedTeamWins = 0;

	UPROPERTY(ReplicatedUsing=OnRep_WinsUpdated, BlueprintReadOnly, EditAnywhere, Category = UI)
		int32 BlueTeamWins = 0;

	UFUNCTION()
	void OnRep_WinsUpdated();

	bool bPendingWinsUpdate = false;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWinsUpdated);
	UPROPERTY(BlueprintAssignable)
	FOnWinsUpdated OnWinsUpdated;

	UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = UI)
		float EndPlayTimer = 30.0f;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = UI)
		float RoundTimeRemaining = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = UI)
		bool bUseTimelimit = false;

	UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = UI)
		int32 Scorelimit = 150;

	UPROPERTY(BlueprintReadOnly, Category = Time)
		float TimeSinceMatchStarted = 0.0f;

	// What gets shown on the score card
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Info")
		virtual int32 GetCurrentSwatScore() { return BlueTeamWins; }

	// What gets shown on the score card
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Info")
		virtual int32 GetCurrentSuspectScore() { return RedTeamWins; }

	// What gets shown on the score card
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Info")
		virtual int32 GetMaxSwatScore() { return RoundsToPlay; }

	// What gets shown on the score card
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Info")
		virtual int32 GetMaxSuspectScore() { return RoundsToPlay; }

	// The current referendum that is being held. If this is nullptr, there is no referendum currently being held.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Referendum)
		class AReferendum* CurrentReferendum = nullptr;

	// The kinds of referendums that we can hold in this game mode
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
		TArray<TSubclassOf<class AReferendum>> AllowedReferendumTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
		bool bCanShowScoreboard = true;

	UPROPERTY(EditAnywhere)
		bool bUsePlanningUICamera = false;

	UFUNCTION(BlueprintCallable, Category = Rounds)
		void ResetReplicatedTimers();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = Rounds)
		void Multicast_OnRoundReset();
	virtual void Multicast_OnRoundReset_Implementation();

	UPROPERTY(BlueprintReadOnly, Category = Rounds)
		float RoundTimeElapsed = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Rounds)
		int32 RoundsPlayed = 0;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Rounds)

	int32 RoundsToPlay = 0;

	UFUNCTION(BlueprintPure, Category = Rounds)
		int32 GetRemainingRounds();

	UPROPERTY(ReplicatedUsing=OnRep_WinsUpdated, BlueprintReadOnly, Category = Gameplay)
		ETeamType RoundWinningTeam;

	UPROPERTY(ReplicatedUsing=OnRep_WinsUpdated, BlueprintReadOnly, Category = Gameplay)
		ETeamType MatchWinningTeam;

	// used in non-team games
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		TArray<class AReadyOrNotPlayerState*> RoundWinners;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessage, FRChatMessage, Message);

	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnChatMessage OnChatMessageReceived;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = Objectives)
		FString MissionName; 

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = Objectives)
		FText MissionDescription;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRep_MatchState, Category = State)
		EMatchState MatchState = EMatchState::MS_None;

	UFUNCTION()
	void OnRep_MatchState();

protected:
	void OnCharacterDied(AReadyOrNotCharacter* Victim, AReadyOrNotCharacter* Killer, AActor* Inflictor);
	void OnGameStarted();
	void OnGameEnded();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterArrested, AReadyOrNotCharacter*, Character, AReadyOrNotCharacter*, ArrestedBy);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCharacterArrested OnCharacterArrested;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterKilled, AReadyOrNotCharacter*, Character, AReadyOrNotCharacter*, KilledBy);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnCharacterKilled OnCharacterKilled;

	UFUNCTION(BlueprintCallable)
	bool IsEveryoneReady();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySequence(class ULevelSequence* Sequence);
	virtual void Multicast_PlaySequence_Implementation(class ULevelSequence* Sequence);

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_StopSequence(class ULevelSequence* Sequence);
	virtual void Multicast_StopSequence_Implementation(class ULevelSequence* Sequence);

	UFUNCTION(BlueprintCallable)
		void SkipMVPScreen();

	UPROPERTY(Replicated)
	FString NextURLReplicated;
	UFUNCTION(BlueprintCallable, Category = NextMapMode)
		void GetNextMapMode(FString& Map, FString& Mode);

	UFUNCTION()
		void OnSequenceStartedFunc(ULevelSequence* LevelSequence) { OnSequenceStarted.Broadcast(LevelSequence); }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSequenceStarted, class ULevelSequence*, LevelSequence);
	UPROPERTY(BlueprintAssignable)
		FOnSequenceStarted OnSequenceStarted;

	// Anything with this label is NOT allowed to spawn due to the way that the dynamic level generation has happened.
	UPROPERTY(Replicated)
	TArray<FName> WhitelistedLabels;

	// The widget to use for DebugDisplayActor widgets.
	UPROPERTY(EditAnywhere, Category = Debug)
	TSubclassOf<UDebugDisplayWidget> DebugDisplayWidget;

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_BroadcastChatMessage(FRChatMessage ChatMessage);
	virtual void Multicast_BroadcastChatMessage_Implementation(FRChatMessage ChatMessage);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Url)
		FString ModeURL_Replicated;

	UPROPERTY(ReplicatedUsing = OnRep_DrawPointDataChanged)
		TArray<FFloorMapPointData> DrawingPointData;

	UFUNCTION()
		void OnRep_DrawPointDataChanged();

	// Whether this is a PvP mode or not
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Gameplay)
		bool bPvPMode = false;

	// Whether we can report things in this mode or not
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Gameplay)
		bool bCanReportToTOC = false;

	// The damage modifiers for this game type
	//////////////////////////////////////////

	// Overall gameplay damage modifier.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Damage)
	float GametypeDamageModifier = 1.0f;

	// Amount to modify damage by, when hitting the head.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Damage)
	float HeadDamageModifier = 2.0f;

	// Amount to modify damage by, when hitting the foot.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Damage)
	float FootDamageModifier = 0.2f;

	// Amount to modify damage by, when hitting the legs.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Damage)
	float LegDamageModifier = 0.3f;

	// Amount to modify damage by, when hitting the arms.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Damage)
	float ArmDamageModifier = 0.2f;

	// Amount to modify damage by, when hitting some other body part.
	UPROPERTY(BlueprintReadWrite, Category = Damage)
	float OtherLimbDamageModifier = 1.0f;

	// Whether this game state (and its associated mode) is considered a "free for all" mode as opposed to teamplay.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gameplay)
	bool bFreeForAll;

	// Whether this game state (and its associated mode) disables picking up dropped items.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gameplay)
	bool bDisablePickups;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay) 
	uint8 bRevivesAllowed : 1;

	// The chat messages that have been sent
	UPROPERTY(BlueprintReadOnly)
	TArray<FRChatMessage> SavedChatMessages;

	// Get the current profile - this will return NULL in a dedicated server, so be careful!!
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = Gameplay)
	virtual class UReadyOrNotProfile* GetCurrentProfile();

	//////////////////////////////////////////
	// Debug stuff
	UPROPERTY(BlueprintReadWrite, Category = Debug)
	bool bBallisticsDebug = false;

	UPROPERTY(BlueprintReadWrite, Category = Debug)
	bool bDamageDebug = false;

	UPROPERTY(BlueprintReadWrite, Category = Debug)
	bool bSpeechRecognitionDebug = false;

	UPROPERTY(BlueprintReadWrite, Category = Debug)
	bool bGOAPDebug = false;


	//
	//////////////////////////////////////////

	//////////////////////////////////////////
	//	Announcements

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Broadcasting)
	bool bAnnounceTeamReinforcements = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Broadcasting)
	FText Msg_BothTeamsReinforced;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Broadcasting)
	FText Msg_RedTeamReinforced;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Broadcasting)
	FText Msg_BlueTeamReinforced;

	//
	//////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenges)
		TArray<TSubclassOf<class UChallenge>> GameModeChallenges;

	//UPROPERTY(BlueprintReadOnly, Category = Challenges)
		//class UChallengeManager* ChallengeManager;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenges)
		//TSubclassOf<class UChallengeManager> ChallengeManagerClass;

	UPROPERTY(BlueprintReadWrite, Category = Planning) //UnReplicated,
	uint8 bInPlanningMenu : 1;

	// time dilation stuff

	UPROPERTY(ReplicatedUsing=OnRep_CustomTimeDilation)
		float CustomTimeDilationApplied = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void SetTimeDilationSynced(float TimeDilation);

	UFUNCTION()
	void OnRep_CustomTimeDilation();

	UPROPERTY(Replicated)
	TArray<APlayerController*> AdminPlayerControllers;

	UFUNCTION(BlueprintCallable, Category = "Server Administration")
		bool IsAdminPlayerController(APlayerController* PlayerController);

	void OnPostBugReportResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	UPROPERTY(Replicated)
	class ATOCManager* TOCManager;

private:
	UPROPERTY(Config)
	bool bHideLevelEffectsInPreMission = true;

	void HideLevelEffects();
	void RestoreLevelEffects();
	
	UPROPERTY(Transient)
	TArray<APostProcessVolume*> WorldPostProcessVolumes;

	UPROPERTY(Transient)
	TArray<AExponentialHeightFog*> WorldExponentialHeightFogs;

	UFUNCTION()
	void UpdateDoorTickIntervals();

	FTimerHandle TH_UpdateDoorTickIntervals;

	
};
