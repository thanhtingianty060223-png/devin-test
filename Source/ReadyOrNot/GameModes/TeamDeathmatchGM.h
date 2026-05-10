// Copyright Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "TeamDeathmatchGM.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ATeamDeathmatchGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Team Deathmatch")
	bool bSuddenDeath = false;

	virtual bool ShouldCountDownTimelimitNow() override { return !bSuddenDeath; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartMatch() override;
	virtual void RoundEnd() override;
	virtual void MatchEnd() override;
	
	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	void CheckWinConditions();
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchLoopMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchStartMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchEndMusic;
};
