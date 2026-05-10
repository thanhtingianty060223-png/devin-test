// Copyright Void Interactive, 2021

#pragma once

#include "ReadyOrNotGameState.h"
#include "CoopGS.generated.h"

UENUM()
enum EMissionEndVoteState
{
	VS_NotStarted,
	VS_InProgress,
	VS_MajorityYes,
	VS_MajorityNo
};

UCLASS()
class READYORNOT_API ACoopGS : public AReadyOrNotGameState, public IListenForGameStart, public IListenForGameEnd
{
	GENERATED_BODY()
protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

public:

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
	bool bMissionSucceded = false;

	// mission completed but not every objective collected yet..
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
	bool bMissionSoftCompleted = false;

	UPROPERTY(ReplicatedUsing=OnRep_COOPMode, EditAnywhere, BlueprintReadOnly)
	ECOOPMode Mode;

	UFUNCTION()
	void OnRep_COOPMode();
	
	virtual FText GetModeText() const override;
	
	// hidden pvp mode
	UPROPERTY(Replicated)
	bool bCrouchingTigerHiddenDragon = false;
	
	//////////////////////////////////////////////////////////////
	//
	//	Deployables

	// The current set of deployables (32 bit flags) that are enabled.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = COOP)
	int32 CurrentDeployables;

	// Sees if the listed deployable is enabled.
	UFUNCTION(BlueprintCallable, Category = COOP)
	bool IsDeployableEnabled(int32 DeployableNumber);

	// Gets a list of enabled deployables as an array.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<int32> GetEnabledDeployables();

	// Gets a list of unenabled deployables as an array.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<int32> GetUnenabledDeployables();

	// Gets a list of enabled deployables as their short names.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<FText> GetEnabledDeployablesShortNames();

	// The depot where all of the deployables are stored. Only valid after the game has started!
	UPROPERTY(Replicated, BlueprintReadOnly, Category = COOP)
	AActor* DeployableDepot;

	// The label of the currently selected depot.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = COOP)
	FName DepotLabel;

	// The number of the currently selected depot.
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_MapElement, BlueprintReadOnly, Category = COOP)
	int32 DepotNumber;

	// The cost of the currently selected depot.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = COOP)
	int32 DepotCost;

	// Set the current depot.
	UFUNCTION(Server, Reliable, WithValidation, Category = COOP)
	void Server_SetDeployableDepot(AReadyOrNotPlayerController* Controller, int32 NewDepotNum);
	virtual void Server_SetDeployableDepot_Implementation(AReadyOrNotPlayerController* Controller, int32 NewDepotNum);
	bool Server_SetDeployableDepot_Validate(AReadyOrNotPlayerController* Controller, int32 NewDepotNum) { return true; }

	//////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////
	//
	//	Personnel

	// The current set of personnel that are enabled.
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
	int32 CurrentPersonnel;

	// The current set of personnel map points that are available (not occupied by anything)
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
	int32 CurrentUsedPersonnelPoints;

	// A map that binds a specific personnel to a personnel map point. 
	UPROPERTY(BlueprintReadWrite, Category = COOP)
	TMap<int32, int32> PersonnelMapping;

	// Checks to see if a personnel is enabled.
	UFUNCTION(BlueprintCallable, Category = COOP)
	bool IsPersonnelEnabled(int32 PersonnelNum);

	// Gets a list of currently enabled personnel, as an array.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<int32> GetEnabledPersonnel();

	// Gets a list of currently available personnel, as an array.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<int32> GetUnenabledPersonnel();

	// Gets a list of currently available personnel points, as an array.
	UFUNCTION(BlueprintCallable, Category = COOP)
	TArray<int32> GetUsedPersonnelPoints();

	// For a given map zone, finds the personnel assigned to it. Returns -1 if there is no personnel assigned to it.
	// This is fairly expensive and we should avoid doing it every frame.
	UFUNCTION(BlueprintCallable, Category = COOP)
	int32 GetPersonnelForMapNum(int32 MapPointNum);

	//////////////////////////////////////////////////////////////
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 TotalAIOfficers = 0;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 TotalOfficers = 0;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 NumCompleteExtraObjectives = 0;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 NumTotalExtraObjectives = 0;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 TeamKills = 0;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		bool bAllPlayerCharactesDead = false;

	// The number of squad points remaining.
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 SquadPointsRemaining;

	// The currently selected spawn point for red team.
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_MapElement, BlueprintReadOnly, Category = COOP)
		ESelectedSpawn SelectedRedSpawnPoint = ESelectedSpawn::SS_FirstSpawn;

	// The number of squad points that the red spawn point uses.
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 RedSpawnSquadPoints;

	// The currently selected spawn point for blue team.
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_MapElement, BlueprintReadOnly, Category = COOP)
		ESelectedSpawn SelectedBlueSpawnPoint = ESelectedSpawn::SS_FirstSpawn;

	// The number of squad points that the blue spawn point uses.
	UPROPERTY(Replicated, BlueprintReadWrite, Category = COOP)
		int32 BlueSpawnSquadPoints;

	// Sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	UFMODEvent* missionMusic;

	UPROPERTY(BlueprintReadWrite, Category = Sound)
	FFMODEventInstance musicInstance;

	// Checks to see if the selected spawn point can be changed.
	UFUNCTION(BlueprintCallable, Category = Planning)
		bool CanChangeSpawn(bool bBlueTeam, ESelectedSpawn NewSpawn);

	UFUNCTION(BlueprintCallable, Category = COOP)
		void OnRep_MapElement();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Localization)
		FText PromotedLeaderFormat;

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = COOP)
		void Multicast_BroadcastNewSquadLeader(APlayerCharacter* NewLeader);
	virtual void Multicast_BroadcastNewSquadLeader_Implementation(APlayerCharacter* NewLeader);

	UFUNCTION(NetMulticast, Reliable, Category = COOP)
			void Multicast_OnMissionEnd(bool bSuccess);
	virtual void Multicast_OnMissionEnd_Implementation(bool bSuccess);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Vote)
		int32 YesVotes = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Vote)
		int32 NoVotes = 0;

	EMissionEndVoteState MissionEndVoteState = EMissionEndVoteState::VS_NotStarted;	

	UFUNCTION(Category = Vote)
		void UpdateVotes(int32 Yes, int32 No);

	/** Returns whether the vote is valid */
	UFUNCTION()
	bool PlayerControllerVoted(AReadyOrNotPlayerController* PlayerController, bool bVoteYes);
	
	virtual void OnGameStarted_Implementation() override;

	FString GetTOCLineForMap(FString OverrideMapName = "") const;

	UFUNCTION()
	void StartTOCBriefing(FString TOCLine);

	virtual void OnGameEnded_Implementation() override;
	
	FTimerHandle GameStartTOCDelay_Handle;

	UPROPERTY(EditAnywhere)
	float TOCDelay = 5.0f;

	TSet<FName> GetLevelProgressionTags(float ScorePercentage);
	void CheckAllLevelsCompleted(TSet<FName>& InProgressionTags);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_GrantProgressionTags(float ScorePercentage);
};
