// Copyright Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "VIPEscortGM.generated.h"

DECLARE_STATS_GROUP(TEXT("RON VIP GM"), STATGROUP_RONVIPGM, STATCAT_Advanced);

/**
 * S.W.A.T must escort the randomly-selected player VIP to an extraction point within the level to win the round.
 * The enemy team must prevent this by intercepting, capturing, and holding the VIP hostage for a certain amount of time.
 * If the timer runs out and S.W.A.T hasn't successfully freed the VIP, the enemy wins the round.
 */
UCLASS()
class READYORNOT_API AVIPEscortGM final : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	virtual void CheckVictoryConditions() override;
	
	UFUNCTION(BlueprintPure, Category = "Ready Or Not|VIP Escort")
	bool IsVIPAlive();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not|VIP Escort")
	bool IsVIPDead();

	UFUNCTION(BlueprintPure, Category = "Ready Or Not|VIP Escort")
	bool IsVIPArrested();
	
	UFUNCTION(BlueprintPure, Category = "Ready Or Not|VIP Escort")
	FORCEINLINE ETeamType GetCurrentVIPTeam() const { return CurrentVIPTeam; }

	UFUNCTION(BlueprintPure, Category = "Ready Or Not|VIP Escort", DisplayName = "Get VIP Character")
    APlayerCharacter* GetVIPCharacter() const;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVIPKilledSignature, AReadyOrNotCharacter*, InstigatorCharacter, AReadyOrNotCharacter*, KilledCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Ready Or Not|VIP Escort|Events")
	FOnVIPKilledSignature OnVIPKilled;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVIPFreedSignature, ACharacter*, Freed, ACharacter*, Freer);
	UPROPERTY(BlueprintAssignable, Category = "Ready Or Not|VIP Escort|Events")
	FOnVIPFreedSignature OnVIPFreed;

	UPROPERTY(BlueprintReadWrite, Category = "Ready Or Not|VIP Escort|Data")
	APlayerController* VIPPlayer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Ready Or Not|VIP Escort|Data")
	class APlayerStart_VIP_Spawn* ChosenVIPSpawn = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "VIP Escort")
	TSubclassOf<APlayerCharacter> VIPCharacterClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "VIP Escort")
	FName VIPSpawnTag = "VIP_SPAWN";

	// The time allowed (in seconds) to deliver the VIP to the extraction point
	UPROPERTY(EditAnywhere, Category = "VIP Escort")
	float TimeToDeliverVIP = 240.0f;

	UPROPERTY(EditAnywhere, Category = "VIP Escort")
	float HostageHoldTime = 120.0f;

protected:
	void Tick(float DeltaSeconds) override;
	
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	void ResetLevel() override;
	void OnRoundStarted_Implementation() override;
	
	void TimeLimitVictoryConditions_Implementation() override;
	
	bool ShouldCountDownTimelimitNow() override;
	
	void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;

	void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;

	void InitializeVIPRound();

	bool ChooseVIPSpawn();
	
	void ChooseNewVIPPlayer();

	void SpawnVIPPlayer();

	void SwapSides();

	void UpdateVIPAndEnemyTransforms();

	void DestroyVIPCharacter();

	bool AllPlayersWereVIP();
	void ResetVIPFlags();
	
	bool HasVisitedAllVIPSpawns();
	void ResetAllVIPSpawnVisits();
	
	TArray<class APlayerController*> GetAllCompatiblePlayersForVIP();

	TArray<class APlayerStart_VIP_Spawn*> GetAllVIPSpawnsInWorld();

	UFUNCTION()
	void VIPKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

	UFUNCTION()
    void VIPFreed(ACharacter* Freed, ACharacter* Freer);

	ETeamType CurrentVIPTeam = ETeamType::TT_SERT_BLUE;
	
	uint8 bVIPInitialized : 1;
};
