// � Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "FirefightGM.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AFirefightGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	AFirefightGM();

	UPROPERTY(BlueprintReadOnly, Category = "Firefight")
	bool bSuddenDeath = false;

	virtual bool ShouldCountDownTimelimitNow() { return !bSuddenDeath; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartMatch() override;
	virtual void RoundEnd() override;
	virtual void MatchEnd() override;
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;
	virtual void ResetClientScores(bool bBetweenRounds) override;

	//sound

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* TeamKilledSound_SERT_RED;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* TeamKilledSound_SERT_BLUE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchLoopMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchStartMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchEndMusic;

	// Random loadouts
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firefight")
	TArray<FSavedLoadout> RandomLoadouts;

	UPROPERTY(BlueprintReadOnly, Category = "Firefight")
		TArray<FSavedLoadout> GeneratedLoadouts;

	UPROPERTY(BlueprintReadOnly, Category = "Firefight")
		int32 NumRedSpawned = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Firefight")
		int32 NumBlueSpawned = 0;

	UFUNCTION(BlueprintCallable)
		void RegenerateRandomLoadouts();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Firefight")
	int32 GetNumberOfActivePlayersOnTeam(ETeamType Team);

	void CheckVictoryConditions();

	// Callbacks
	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
};