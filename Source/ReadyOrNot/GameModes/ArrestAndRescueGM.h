// Copyright Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "ArrestAndRescueGM.generated.h"

/**
 *
 */
UCLASS()
class READYORNOT_API AArrestAndRescueGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Arrest and Rescue")
		bool bSuddenDeath = false;

	virtual bool ShouldCountDownTimelimitNow() override { return !bSuddenDeath; }

	class AArrestAndRescueGS*  GetAARGS();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartMatch() override;
	virtual void RoundEnd() override;
	virtual void MatchEnd() override;
	virtual void ResetLevel() override;

	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void PlayerFreed(ACharacter* Freed, ACharacter* Freer) override;
	virtual void CheckVictoryConditions() override;
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;
	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator /* = false */) override;
	virtual void RespawnAllPlayersOnTeam(ETeamType Team) override;
	virtual void RespawnAllPlayers() override;
	virtual void RespawnDeadPlayers() override;

	UPROPERTY(BlueprintReadOnly, Category = "Arrest and Rescue")
		TArray<class APlayerCharacter*> ArrestedBlueCharacters;

	UPROPERTY(BlueprintReadOnly, Category = "Arrest and Rescue")
		TArray<class APlayerCharacter*> ArrestedRedCharacters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* VIPArrestedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* VIPKilledSound;
};
