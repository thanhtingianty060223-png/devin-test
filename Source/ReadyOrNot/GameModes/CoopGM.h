// Copyright Void Interactive, 2023

#pragma once

#include "ReadyOrNotGameMode.h"
#include "CoopGM.generated.h"

DECLARE_STATS_GROUP(TEXT("CO-OP"), STATGROUP_COOPGameMode, STATCAT_Advanced);

UCLASS()
class READYORNOT_API ACoopGM : public AReadyOrNotGameMode
{
	GENERATED_BODY()

public:
	ACoopGM();
	
	virtual void StartMatch() override;
	virtual void ResetLevel() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostSeamlessTravel() override;
	virtual void AutoAssignTeam(AController* Player) override;

	void OnNavigationInitDone();
	void TryGenerateWorld();
	
	virtual bool DoesLevelRequireGeneration() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual bool AreAllPlayersDead() override;
	
	UFUNCTION(BlueprintCallable)
	virtual void ReturnToStation();
	
	virtual bool ShouldAlertSuspectWhenLastAlive() const override;
	
	// stagger the creation of things
	bool bInitedWorld = false;
	void InitWorld();
	bool bInitedAI = false;
	void InitAI();

	// hidden pvp mode
	bool bHiddenTigerCrouchingDragon = false;

	void EnableHiddenTigerCrouchingDragon();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;

	bool IsROEDisabled() const;

	//UFUNCTION(CallInEditor, Category = "Scoring")
	//void UpdateScorePoolAllocations();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAllAISpawnedDelegate);
	UPROPERTY(BlueprintAssignable)
	FAllAISpawnedDelegate OnAllAISpawned;

	UPROPERTY()
	class AWorldDataGenerator* WorldDataGenerator = nullptr;

	virtual void PreInitializeComponents() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECOOPMode Mode;

	bool bMissionCompleted = false;
	virtual void OnMissionCompleted();
	virtual void CheckProgression(TSet<FName>& InProgressionTags);

public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE ECOOPMode GetCOOPMode() const { return Mode; }

	void RemoveAllBombsExceptDesignated();

	bool bAssignedSquadLeader = false;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual bool CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const override;
	
	FTimerHandle CheckWinConditions_Handle;
	void CheckWinConditions();
	
	bool bForceWin = false;

	FTimerHandle FindSpotUpgrade_Handle;

	FTimerHandle MissionEndTimer_Handle;

	//UPROPERTY(BlueprintReadOnly, Category = Vote)
//	int32 YesVotes;

	//UPROPERTY(BlueprintReadOnly, Category = Vote)
//	int32 NoVotes;

	UFUNCTION(Server, Reliable, Category = Vote)
	void Server_SoftClearVoteCheck();

	UFUNCTION()
	void StartMissionEndTimer(bool bWon);

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Debug End Mission")
	void DebugEndMission();
#endif
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Debug End Mission")
	bool bDebugWonMission = true;
#endif

	UPROPERTY(EditAnywhere, Category = "Gameplay")
	TSubclassOf<class AActor> KilledSuspectAvoidanceShape;

	// Finds the player start associated with a team
	UFUNCTION(BlueprintCallable, Category = Spawn)
	APlayerStart* FindPlayerStartForTeam(ETeamType Team);

	void FindNewSquadLeader();
	void ResetSquadLeader();

	UFUNCTION(BlueprintCallable, Category = AI)
	void SpawnSuspectsAndHostages();

	// Spawns players and AI controlled officers
	UFUNCTION(BlueprintCallable, Category = AI)
	void SpawnPolice();

	// Spawns a single AI controlled officer
	void SpawnAIOfficer(ESquadPosition SquadPosition, ETeamType CommandTeam, FString LoadoutName, TSubclassOf<ACyberneticCharacter> Class);
	void SpawnAITrailerOfficer(ESquadPosition SquadPosition, ETeamType CommandTeam, FString LoadoutName);

	virtual void SetupOfficerCustomization(class ASWATCharacter* Character, FSavedCustomization& OutCustomization);
	
	// Sets up doors
	void ResetDoors();

	// Spawns door keycards
	void SetupKeycards();

	UFUNCTION(BlueprintCallable, Category = AI)
	void RemoveAllSpawnedAI();

	// Spawn in all of the personnel
	void SpawnPersonnel();

	// Spawn one specific personnel
	void SpawnSinglePersonnel(int32 PersonnelNum);

	// True if the Negotiator perk is enabled
	UPROPERTY(EditAnywhere, Category = COOP)
	bool bNegotiatorActive = false;

	// Spawn either a Spotter, Marksman, or Sniper.
	UFUNCTION(Category = COOP)
	void Personnel_SpawnHighground(int32 PersonnelNum, int32 MapPointNum, bool bSpotter, bool bMarksman, bool bSniper);

	// Spawn either a noisemaker or floodlight operator.
	UFUNCTION(Category = COOP)
	void Personnel_SpawnOperator(int32 PersonnelNum, int32 MapPointNum, bool bNoisemaker);

	// Spawn a "ventilation expert."
	UFUNCTION(Category = COOP)
	void Personnel_SpawnVentilation(int32 PersonnelNum, int32 MapPointNum);

	// Spawn a truck driver.
	UFUNCTION(Category = COOP)
	void Personnel_SpawnTruckDriver(int32 PersonnelNum, int32 MapPointNum);

	// Spawn a Negotiator
	UFUNCTION(Category = COOP)
	void Personnel_SpawnNegotiator();

	// Spawn a Power Crew
	UFUNCTION(Category = COOP)
	void Personnel_SpawnPowerCrew();

	UPROPERTY(EditAnywhere, Category = COOP)
	bool bAIEquipSameLoadoutAsPlayer = false;

	UPROPERTY(EditAnywhere, Category = COOP)
	int32 MaxHostagesKilledBeforeMissionFailed = 1;

	UPROPERTY(EditAnywhere, Category = COOP)
	int32 MaxTeamKillsBeforeAIRetaliates = 2;

	UPROPERTY(EditAnywhere, Category = Swat)
	TSubclassOf<class ACyberneticCharacter> SwatAlphaClass;

	UPROPERTY(EditAnywhere, Category = Swat)
	TSubclassOf<class ACyberneticCharacter> SwatBetaClass;

	UPROPERTY(EditAnywhere, Category = Swat)
	TSubclassOf<class ACyberneticCharacter> SwatCharlieClass;

	UPROPERTY(EditAnywhere, Category = Swat)
	TSubclassOf<class ACyberneticCharacter> SwatDeltaClass;
	
	UPROPERTY(EditAnywhere, Category = Swat)
	TSubclassOf<class ACyberneticCharacter> SwatTrailerClass;

	UPROPERTY(EditAnywhere, Category = COOP)
	TSubclassOf<class AAIController> FriendlyAIController;

	UPROPERTY(EditAnywhere, Category = COOP)
	FSavedLoadout FriendlyAILoadout;

	UPROPERTY(EditAnywhere, Category = COOP)
	TSubclassOf<class AEvidenceActor> EvidenceClass;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEnded, bool, bMissionSucceded);
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = Events)
	FOnMissionEnded OnMissionEnded;

	UPROPERTY(EditAnywhere, Category = COOP)
	TSubclassOf<UUserWidget> GameStartedWidget;

	UPROPERTY(EditAnywhere, Category = COOP)
	TSubclassOf<AActor> KeycardClass;

	UFUNCTION(BlueprintCallable, Category = COOP)
	void MissionEnd(bool bSuccess);

	virtual void CreateMatchEndWidgets(class AReadyOrNotPlayerController* PlayerController);
	
	UPROPERTY(VisibleAnywhere, Category = COOP)
	FName AI_SpawnTag = "AI_SPAWN";

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	
	UFUNCTION()
	virtual void AIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	
	UFUNCTION()
	virtual void AIIncapacitated(AReadyOrNotCharacter* Incapacitated, AReadyOrNotCharacter* InstigatorCharacter);

	UFUNCTION()
	virtual void AIArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	
	UFUNCTION()
	virtual void AISurrendered(AReadyOrNotCharacter* Character);

	UFUNCTION()
	void IncapHumanKilled(AReadyOrNotCharacter* InstigatorCharacter, AIncapacitatedHuman* KilledHuman);

	UFUNCTION()
	virtual void FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

	void DeactivateAllAI();
	
	UPROPERTY()
	TArray<class AEvidenceActor*> EvidenceInWorld;

	UPROPERTY()
	int32 NextHighgroundDesignation = 0;

	UPROPERTY(BlueprintReadOnly, Category = Personnel)
	TArray<class AHighgroundVolume*> Highground;

	// Whether to use different sound sets for each officer (should only be true once we have all the sounds in!!)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AI)
	bool bUniqueOfficerSoundsets;

	// If Unique Soundsets is true, this is how many officer soundsets have been picked.
	UPROPERTY(BlueprintReadOnly, Category = AI)
	int32 NumPickedOfficerSoundsets;

	// If Unique Soundsets is true, this is the flag of picked soundsets
	UPROPERTY(BlueprintReadOnly, Category = AI)
	int32 PickedOfficerSoundsets;

	UPROPERTY(BlueprintReadOnly, Category = AI)
	TArray<class ASWATCharacter*> SpawnedSWATAI;
	
	UPROPERTY(BlueprintReadOnly, Category = AI)
	TArray<class ATrailerSWATCharacter*> SpawnedTrailerSWATAI;
	
	UPROPERTY()
	TArray<class AAISpawn*> SpawnLaterSpawns;

	UPROPERTY(EditAnywhere, Category = "Optimization")
	float AISpawnDistance = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class USoundCue* NegFeedback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class USoundCue* PosFeedBack;

	UPROPERTY()
	TArray<class ADoor*> KickedDoorsTriggeredMoraleChange;

	virtual void ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters) override;
};
