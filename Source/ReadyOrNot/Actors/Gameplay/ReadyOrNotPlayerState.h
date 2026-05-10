// Copyright Void Interactive, 2023

#pragma once

#include "Data/CustomizationData.h"
#include "GameFramework/PlayerState.h"
#include "lib/DataSingleton.h"
#include "Info/MissionPlanManager.h"
#include "ReadyOrNotPlayerState.generated.h"

UENUM(BlueprintType)
enum class EVoiceType : uint8
{
	VT_Local,
	VT_Team
};

/**
 * 
 */
UCLASS()
class READYORNOT_API AReadyOrNotPlayerState : public APlayerState
{
	GENERATED_BODY()

	AReadyOrNotPlayerState();
public:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ClientInitialize(AController* C) override;

	float LastScoreTeamUpdate = 0.0f;
	
	// for the server to check when equipping
	TArray<EGameVersionRestriction> UnlockedDLC;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendUnlockedDLC(EGameVersionRestriction Dlc);
	
	UPROPERTY(Replicated)
	bool bHasFinishedLoading = false;

	UFUNCTION(BlueprintPure)
	static bool HasEveryoneFinishedLoading(int32& OutTotal, int32& OutLoading, int32& OutLoaded);

	virtual void SetPlayerName(const FString& S) override;

	void TrySetPreferredTeam();

	void IncreaseScore(float Amount);
	void DecreaseScore(float Amount);

	//UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	//	EPlayerHealthStatus HealthStatus = EPlayerHealthStatus::HS_NotAvailable;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	int32 Kills;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 KillsThisLife;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 TeamKills;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	int32 Arrests;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 TimesArrested;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 ArrestsThisLife;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	int32 Objectives;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 Reports = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 Evidence = 0;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	TArray<class AEvidenceActor*> EvidenceActorsInPossession;

	EFireMode LastPlayerFireMode = EFireMode::FM_Single;

	UPROPERTY(Replicated, Transient)
	APlayerCharacter* LastCharacter;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		TSubclassOf<UDamageType> DeathDamageType; // last death damage type

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		class ABaseMagazineWeapon* DeathWeapon; // last death weapon

	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
		EFireMode LastFireMode = EFireMode::FM_Single;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		bool bDeadToPointDamage = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	FHitResult DeathTraceHit;	// last death trace type

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		class APlayerCharacter* DeathKiller;	// last person who killed us

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	int32 Deaths;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		int32 Incapacitations = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	float DamageDealt;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	float DamageReceived;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	FString PlayerSpawnTag ;  //= "SERT_BLUE"


	bool bSquadTeamAssigned = false;
	bool bJoinedOnSquadLeader = false;

	// ran after auto balancing team
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetJoinedOnSquadLeader();
	UFUNCTION(BlueprintPure)
	bool IsVipPlayerState();

	UFUNCTION()
	void IncrementBulletsFired(class ABaseWeapon* Weapon);

	UFUNCTION()
	void ResetBulletsFired();

	UFUNCTION(BlueprintPure)
	void GetNetworkStatus(float& AvgLag);

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Stats)
	int32 BulletsFired = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Stats)
	int32 BulletsFiredThisLife = 0;
public:
	

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	ETeamType Team = ETeamType::TT_NONE;
	ETeamType PendingTeam = ETeamType::TT_NONE;

	UFUNCTION(Client, Reliable)
		void Notify_PendingChangeTeam(ETeamType NewTeamType);
	virtual void Notify_PendingChangeTeam_Implementation(ETeamType NewTeamType);

	// Saved loadout on the server. 
	UPROPERTY(ReplicatedUsing = OnRep_UpdateServerSavedLoadout, BlueprintReadOnly, Category = Gameplay)
	FSavedLoadout ServerSavedLoadout;
	
	virtual bool Server_UpdateStats_Validate(float NewMapBestTime, int32 NewMapBestRating) { return true; }

	UFUNCTION()
		void OnRep_UpdateServerSavedLoadout();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
		void Server_UpdatePlayerSpawnTag(const FString& NewTag);
	virtual void Server_UpdatePlayerSpawnTag_Implementation(const FString& NewTag);
	virtual bool Server_UpdatePlayerSpawnTag_Validate(const FString& NewTag){return true;}
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLoadoutChanged, FSavedLoadout, NewLoadout);
	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnPlayerLoadoutChanged OnPlayerLoadoutChanged;

	bool bSpawnLoadout = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	bool bReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		bool bSquadLeader = false;

	UFUNCTION(BlueprintPure, Category = Gameplay)
		bool IsSquadLeader();

	UPROPERTY(ReplicatedUsing = OnRep_UpdateServerSavedLoadout, BlueprintReadOnly, Category = Gameplay)
	FSavedLoadout LastLoadout = FSavedLoadout();
	
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromKills;
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromDamage;
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromArrests;
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromObjective;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromReportingKills = 0.0f;
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Gameplay)
	float PointsFromReportingArrests = 0.0f;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
	int32 GetKillCount();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Gameplay)
	int32 GetDeathCount();

	UFUNCTION(BlueprintCallable)
	void SetTeam(ETeamType NewTeam);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_SetTeam(ETeamType NewTeam);
	virtual void Server_SetTeam_Implementation(ETeamType NewTeam);
	virtual bool Server_SetTeam_Validate(ETeamType NewTeam) { return true; }

	void TrySetPendingTeamAsTeam();

	UFUNCTION(BlueprintPure, Category = Gameplay)
	ETeamType GetTeam();

	UFUNCTION(BlueprintPure, Category = Gameplay)
	ETeamType GetPendingTeam();

	FString GetTeamAsString();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
	void Server_SetLoadout(FSavedLoadout newLoadout);
	virtual void Server_SetLoadout_Implementation(FSavedLoadout newLoadout);
	virtual bool Server_SetLoadout_Validate(FSavedLoadout newLoadout) { return true; }

	UFUNCTION(BlueprintCallable)
	void SetReady(bool bIsReady, FSavedLoadout NewLoadout);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
		void Server_SetReady(bool bIsReady, FSavedLoadout NewLoadout);
	virtual void Server_SetReady_Implementation(bool bIsReady, FSavedLoadout NewLoadout);
	virtual bool Server_SetReady_Validate(bool bIsReady, FSavedLoadout NewLoadout) { return true; }

	FSavedLoadout GetLoadout();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
		void Server_SetPlayerName(FName NewPlayerName);
	virtual void Server_SetPlayerName_Implementation(FName NewPlayerName);
	virtual bool Server_SetPlayerName_Validate(FName NewPlayerName) { return true; }

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Player)
		bool bIsInGame;

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
		void Server_SetIsInGame(bool bNewIsInGame);
	virtual void Server_SetIsInGame_Implementation(bool bNewIsInGame);
	virtual bool Server_SetIsInGame_Validate(bool bNewIsInGame) { return true; }

	UPROPERTY(BlueprintReadOnly)
	bool bJoinInProgress;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = VIP)
		bool bIsVIP;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = VIP)
	bool bWasVIP;



	UPROPERTY(BlueprintReadOnly, Replicated, Category = Stats)
		int32 GrenadesThrown = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Stats)
		int32 TotalYells = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Stats)
		int32 NumberOrdersGiven = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Stats)
		int32 BulletsHit = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Stats)
		int32 BulletsHitThisLife = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Stats)
		int32 Headshots = 0;

	// Not replicated
	UPROPERTY(BlueprintReadOnly, Category = Stats)
		float TimeAlive = 0.0f;

	int32 GunGameIdx = 0;
	int32 KillsSinceUpgrade = 0;
	
private:
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_VoiceType)
	EVoiceType VoiceType = EVoiceType::VT_Local;

	UFUNCTION()
	void OnRep_VoiceType() { OnVoiceChannelChanged.Broadcast(); }
	
	UPROPERTY(Replicated)
	bool bIsTalking;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVoiceChannelChanged);

	UPROPERTY(BlueprintAssignable)
	FOnVoiceChannelChanged OnVoiceChannelChanged;
	
public:
	UFUNCTION(BlueprintPure, Category = "Voice Type")
	EVoiceType GetVoiceType();
	UFUNCTION(Server, Reliable, WithValidation, Category = "Set Talking Test BP")
	void SetIsTalking(bool bNewTalking);
	UFUNCTION(BlueprintPure, Category = IsTalking)
	bool IsTalking() const;
	FDelegateHandle TalkingStateDelegate_Handle;

	UFUNCTION(BlueprintPure, Category = "Owner")
		bool IsOwnerOfPlayerState();

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Voice)
		void Server_SetVoiceType(EVoiceType NewVoiceType);
	virtual void Server_SetVoiceType_Implementation(EVoiceType NewVoiceType);
	virtual bool Server_SetVoiceType_Validate(EVoiceType NewVoiceType) { return true; }

	/////////////////////////////////
	//
	//	Not replicated, clientside only stuff goes here

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bTorsoInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bLeftArmInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bRightArmInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bLeftLegInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bRightLegInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		bool bHeadInjured = false;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		int32 BulletsBlocked = 0;

	UPROPERTY(BlueprintReadOnly, Category = Health)
		int32 HitsReceived = 0;

	float LastSentScore = 0.0f;
	FTimerHandle UpdateScore_Handle;
	UFUNCTION()
	void UpdateScore();

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Replay")
	bool bIsReplaySpectator = false;

	// The planning player number assigned by server
	UPROPERTY(Replicated)
	int32 PlanningPlayerNumber = 0;
	
	// Return the displayable player number used in planning UI. Returns zero if invalid
	UFUNCTION(BlueprintCallable)
	int32 GetPlanningPlayerNumber() const { return PlanningPlayerNumber; }
	
	static constexpr int32 MaxDrawings = 10;
	static constexpr int32 MaxDrawingPoints = 512;
	
	UPROPERTY(Replicated)
	FPlanningDrawingArray DrawingArray;

	UPROPERTY(Replicated)
	FPlanningDrawing CurrentDrawing;
	
	UFUNCTION(Server, Reliable)
	void Server_StartDrawing(int32 Floor, FVector2D StartingPoint);
	
	UFUNCTION(Server, Reliable)
	void Server_UpdateDrawing(FVector2D NewPoint);
	
	UFUNCTION(Server, Reliable)
	void Server_FinishDrawing();

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_Customization)
	FSavedCustomization Customization;
	
	bool bResetPremissionCustomization = false; // TODO(killo): temp fix for 1.0 remove later
	
	UFUNCTION(Server, Reliable)
	void Server_SetCustomization(FSavedCustomization InCustomization);

private:
	UFUNCTION()
	void OnRep_Customization() { bResetPremissionCustomization = true; }
};
