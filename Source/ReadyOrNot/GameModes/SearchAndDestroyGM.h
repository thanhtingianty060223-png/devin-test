// Copyright Void Interactive, 2021

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "SearchAndDestroyGM.generated.h"


UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ASearchAndDestroyGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "S&D")
		bool bBombPlanted = false;
		bool bBombDetonate = false;
		bool bBombDefused = false;

	virtual bool BombCountActive() { return !bBombPlanted; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartMatch() override;
	virtual void RoundEnd() override;
	virtual void MatchEnd() override;
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
	/*virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;*/

	UPROPERTY(BlueprintReadOnly, Category = "S&D")
		int32 NumRedSpawned = 0;

	UPROPERTY(BlueprintReadOnly, Category = "S&D")
		int32 NumBlueSpawned = 0;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "S&D")
		int32 GetNumberOfActivePlayersOnTeam(ETeamType Team);

	void CheckVictoryConditions();
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

};
