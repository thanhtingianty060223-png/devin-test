// Copyright Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "GunGameGM.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AGunGameGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	AGunGameGM();

	virtual void BeginPlay() override;
	virtual void RoundEnd() override;
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual bool ShouldCountDownTimelimitNow() override { return !bSuddenDeath; }

	virtual void Tick(float DeltaSeconds) override;

	void PlayerWon(AController* Player);
	
	UPROPERTY(EditAnywhere, Category = Respawn)
	float RespawnTime = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Gun Game")
	bool bSuddenDeath = false;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Gun Game")
		TArray<class AReadyOrNotPlayerState*> FindTopKillers();

	UPROPERTY(EditAnywhere, Category = "Gun Game")
	TArray<TSoftClassPtr<ABaseItem>> Itemlist;

	UPROPERTY(EditAnywhere, Category = "Gun Game")
		FSavedLoadout DefaultItems;
	
	UPROPERTY(EditAnywhere, Category = "Gun Game")
		int32 KillsToProgress = 5;
	
	UFUNCTION(BlueprintCallable, Category = "Gun Game")
	ABaseItem* EquipNextGun(APlayerCharacter* Player, bool bAdvanceGunIdx = false);

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;

	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	virtual void RespawnAllPlayers() override;

};
