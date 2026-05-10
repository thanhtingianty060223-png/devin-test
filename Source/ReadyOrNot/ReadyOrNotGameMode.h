// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/GameModeBase.h"
#include "Enums.h"
#include "Characters/ReplayCameraPawn.h"
#include "lib/DataSingleton.h"
#include "ReadyOrNotGameMode.generated.h"

class APlayerCharacter;
class APlayerStart;
class ASpectatePawn;
class AReadyOrNotPlayerState;

DECLARE_STATS_GROUP(TEXT("RON GM"), STATGROUP_RONGM, STATCAT_Advanced);

UENUM(BlueprintType)
enum class ERespawnMode : uint8
{
	NoRespawn,
	ImmediateRespawn,
	DelayedRespawn
};

UCLASS()
class READYORNOT_API AReadyOrNotGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AReadyOrNotGameMode();
	// Used by Competition Server
	int32 EventID = 0;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreInitializeComponents() override;
	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;
	virtual void InitGameState() override;

	UFUNCTION()
	virtual void OnBanStatusChecked(FString SteamId, bool bIsBanned, FString BanReason, bool bIsMySteamId);

	TMap<FString, FString> BannedSteamIds;

	virtual bool ShouldAlertSuspectWhenLastAlive() const;
	virtual bool ShouldAlertCivilianWhenLastAlive() const;
	
	bool HasEveryoneFinishedLoading();

	UFUNCTION()
	void RefreshSession();

	UFUNCTION(BlueprintCallable, Category = GameMode)
	virtual bool ShouldCountDownTimelimitNow() { return true; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = GameMode)
	class AReadyOrNotGameSession* GetReadyOrNotGameSession();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = GameMode)
	class AReadyOrNotGameState* GetReadyOrNotGameState();

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle GameModeSettings;

	FGameModeSettings GetGameModeSettings();

	bool bCalledMatchStart = false;
	float TimeSinceGameStart = 0.0f;
	bool bPlayedSWATCountdownAnnouncement = false;
	bool bPlayedMLOCountdownAnnouncement = false;

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal /* = TEXT("") */) override;
	// Name Options Map Stored from PreLogin
	TMap<FString, FString> NameOptionsMap;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(CallInEditor)
	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute = false) override;
	virtual void PostSeamlessTravel() override;

	UFUNCTION(BlueprintCallable)
	virtual bool DoesLevelRequireGeneration();

	virtual void ResetClientScores(bool bBetweenRounds);
	virtual void RespawnPlayerInMatch(APlayerController* Player);
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
	virtual void RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	virtual void ResetLevel() override;
	virtual bool CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const;
	virtual void AutoAssignTeam(AController* Player);
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;

	virtual void RequestNewLoadout(AReadyOrNotCharacter* Character, FSavedLoadout Loadout);
	
	UFUNCTION(BlueprintCallable, Category = "Game")
	virtual void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "Game", Exec)
	virtual void NextGame();

	UFUNCTION(BlueprintNativeEvent, Category = "Out of Bounds")
			void OnOutOfBoundsTimeLimitEnded();
	virtual void OnOutOfBoundsTimeLimitEnded_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Game")
	static void AddAbuse(AReadyOrNotCharacter* Abuser, class ACyberneticCharacter* Abused);

	UPROPERTY(BlueprintReadOnly, Category = "Ready Or Not Game Mode")
	TMap<AReadyOrNotCharacter*, int> AbuseCountMap;

	FString GetNextURL();

	UPROPERTY(EditAnywhere, Category = "Campaign")
		bool bIsCampaignTransitioning = false;

	UPROPERTY()
		FTimerHandle Reinforcement_Handle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
		bool bRunWarmup = true;

	// Whether being arrested forces us into spectator.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
		bool bArrestSpectator = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
		bool bAllowLoadouts = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
		int32 MaxPlayersAllowed = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Security)
		bool bEnabledForPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Security)
		FString urlShortName;

	UPROPERTY(BlueprintReadOnly, Category = Security)
		FString Password = "";

	UFUNCTION(BlueprintCallable, Category = Security)
		void SetPassword(FString newPassword);

	UPROPERTY(EditAnywhere, Category = Spawning)
	TSubclassOf<AActor> PlayerStartClass;

	UPROPERTY(EditAnywhere, Category = Spawning)
		TSubclassOf<ASpectatePawn> DeadSpectatorClass;

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName LobbyStartTag = "LobbyView";

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName RedCustomizationStartTag = "RedCustomization";

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName BlueCustomizationStartTag = "BlueCustomization";

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName SWATBlueStartTag = "SERT_BLUE";

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName SWATRedStartTag = "SERT_RED";

	UPROPERTY(EditAnywhere, Category = Spawning)
		FName SuspectStartTag = "SUSPECT";
	
	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bSpectateKillerOnDeath = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bInitialPlayerRespawn = true;
	
	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	uint8 bDrawPlayerCameraSphere : 1;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool KickPlayer(APlayerController* KickedPlayer, const FText& KickReason);

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual void StartMatch();

	virtual void LoadHostMigrationData();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchStarted);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnMatchStarted OnMatchStarted;

	bool AreAllPlayersOnTeamArrested(ETeamType Team);
	bool AreAllPlayersOnTeamDead(ETeamType Team);
	bool AreAllPlayersOnTeamDowned(ETeamType Team);
	bool AreAllPlayersOnTeamDead(APlayerCharacter* PlayerCharacter);
	bool AreAllPlayersOnTeamDowned(APlayerCharacter* PlayerCharacter);
	int32 GetNumberOfArrestedPlayersOnTeam(ETeamType Team);
	int32 GetNumberOfFreePlayersOnTeam(ETeamType Team);
	ETeamType GetOtherTeam(ETeamType TeamType) { return TeamType == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE; }

	UPROPERTY(EditAnywhere, Category = Gameplay)
	ERespawnMode RespawnMode = ERespawnMode::ImmediateRespawn;

	uint8 CurrentAIIndex = 0;
	
	//UPROPERTY(EditAnywhere, Category = Gameplay)
	//	bool bReinforcementsUsedInMode = true;
	//	bool bReinforcementsUsedInMode = true;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	bool bTimelimitUsedInMode = true;

	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
	EMatchState CurrentMatchState = EMatchState::MS_Warmup;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, EMatchState, NewMatchState);
	UPROPERTY(BlueprintAssignable, Category = MATCH)
	FOnMatchStateChanged OnMatchStateChanged;

	UFUNCTION(BlueprintCallable, Category = MATCH)
	void SetMatchState(EMatchState newMatchState);
	UFUNCTION(BlueprintPure, Category = MATCH)
	virtual EMatchState GetMatchState();

	// true if we've ever had a controller in this game
	// false if no controller (aka dedicated server loaded up with no one in it)
	bool bHasEverHadControllerInGame = false;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual bool AreAllPlayersDead();
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool IsTeamDead(ETeamType Team, bool bIncludeArrestedAsDead = false);

	virtual APlayerController* ProcessClientTravel(FString& FURL, bool bSeamless, bool bAbsolute) override;

	UPROPERTY(EditAnywhere, Category = Loadout)
		FSavedLoadout DefaultLoadoutIfNothingLoaded;

	// if set then countdown to the next map on the map end screen
	bool bShouldUseCountdown = false;
	float TimeRemainingToNextMap = 30.0f;

	// Mode name, for the UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FText ModeName;

	// Mode description, for the UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FText ModeDescription;

	UPROPERTY(EditDefaultsOnly, Category = Gameplay) 
	bool bCanRespawn = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	int32 MinimumPlayersToStart = 1;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	int32 MinimumPlayersForTimer = 2;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void SwapPlayerTeams();

	UPROPERTY(EditAnywhere, Category = Gameplay)
	TSoftClassPtr<APlayerCharacter> BlueCharacterClass;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	TSoftClassPtr<APlayerCharacter> RedCharacterClass;

	UFUNCTION()
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	
	UFUNCTION()
	virtual void PlayerDowned(AReadyOrNotCharacter* DownedCharacter, AReadyOrNotCharacter* InstigatorCharacter);

	void AddPlayerToReinformcementTimer(APlayerController* DeadPlayer);

	bool bFirstArrestAnnouncementPlayed = false;
	UFUNCTION()
	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter);

	UFUNCTION()
	virtual void PlayerFreed(ACharacter* Freed, ACharacter* Freer);

	UFUNCTION()
		virtual void PlayerTakenDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		APlayerCharacter* SpawnPlayerCharacter(APlayerController* Controller, TSubclassOf<APlayerCharacter> Class, FTransform SpawnTransform);

	FTransform LastPlayerSpawnPoint;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		class ASpectatorPawn* SpawnSpectator(APlayerController* Controller, TSubclassOf<class ASpectatorPawn> Class, FTransform SpawnTransform);
	
	UPROPERTY()
	TArray<APlayerController*> DeadPlayers;

	UPROPERTY()
	TArray<APlayerController*> RespawnableDeadPlayers;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool RemoveDeadPlayer(APlayerController* InPlayerController);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool RemoveDeadPlayerAt(int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual void RespawnDeadPlayers();

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual void RespawnAllPlayers();

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual void RespawnAllPlayersOnTeam(ETeamType Team);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual void RespawnDeadPlayersOnTeam(ETeamType Team);

	UFUNCTION(BlueprintCallable, Category = Gameplay)
		virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnPlayerRespawned")
	FString PlayerSpawnTag ;

	UFUNCTION()
	AActor* GetThisPlayersStartPointByTag(APlayerController* Player,const FString& IncomingName);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerRespawned, APawn*, Pawn, APlayerController*, Controller);
	UPROPERTY(BlueprintAssignable, Category = "OnPlayerRespawned")
	FOnPlayerRespawned OnPlayerRespawned;

	UPROPERTY(EditAnywhere, Category = HUD)
		TSubclassOf<UUserWidget> CharacterHUD;

	AActor* GetRandomSafeStart();

	virtual APlayerStart* FindPlayerStartWithTag(const FName& Tag) const;

	UFUNCTION(BlueprintPure)
	TArray<APlayerCharacter*> GetAllPlayerCharactersInWorld() const;

	// Check to announce teamkill
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Gameplay)
		void CheckToAnnounceTeamkill(ACharacter* InstigatorCharacter, ACharacter* KilledCharacter);
	virtual void CheckToAnnounceTeamkill_Implementation(ACharacter* InstigatorCharacter, ACharacter* KilledCharacter);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Classes")
	TSubclassOf<ASpectatorPawn> NormalSpectatorPawn;
	
	UPROPERTY()
	class AMissionPlanManager* MissionPlanManager;
	
	// Exfil Related
	UPROPERTY(BlueprintReadOnly, Category = "Exfil")
	bool bMissionExfiltrated = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Exfil")
	bool bIsExfilEnabled = false;
	
	UFUNCTION(BlueprintCallable)
	virtual bool GetIsExfilEnabled();
	UFUNCTION(BlueprintCallable)
	virtual void SetExfilEnabled(bool bEnabled);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExfiltrateMission);
	FOnExfiltrateMission OnExfiltrateMission;

	UFUNCTION(BlueprintCallable)
	virtual void ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExfilEnabledChange, bool, bEnabled);
	FOnExfilEnabledChange OnExfilEnabledChange;
};
