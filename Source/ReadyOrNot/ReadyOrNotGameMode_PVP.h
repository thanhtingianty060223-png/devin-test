// Copyright Void Interactive, 2021

#pragma once

#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameMode_PVP.generated.h"

DECLARE_STATS_GROUP(TEXT("RON GM PVP"), STATGROUP_RONGMPVP, STATCAT_Advanced);

/**
 * Base class for all Ready Or Not PVP game modes
 */
UCLASS()
class READYORNOT_API AReadyOrNotGameMode_PVP : public AReadyOrNotGameMode
{
	GENERATED_BODY()

public:
	// Some team won this round
	UFUNCTION(BlueprintCallable, Category = Gameplay)
    virtual void RoundWonTeam(ETeamType WinningTeam);
	
	// Some players won this round
	UFUNCTION(BlueprintCallable, Category = Gameplay)
    virtual void RoundWon(TArray<AReadyOrNotPlayerState*> WinningPlayers);
    
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual void RoundEnd();
	
	UFUNCTION(BlueprintCallable, Category = Gameplay)
    virtual void TimelimitReached();

	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool AnyDeathsOnWinningTeam();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchStart);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnMatchStart OnMatchStart;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundWon, ETeamType, WinningTeam);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnRoundWon OnRoundWon;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundStart);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnRoundStart OnRoundStart;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundEnd);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnRoundEnd OnRoundEnd;

protected:
	void Tick(float DeltaSeconds) override;
	void ResetLevel() override;

	void StartMatch() override;

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void PlayerDowned(AReadyOrNotCharacter* DownedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;

	void IncrementRoundsPlayed();

	virtual void CheckVictoryConditions();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	virtual void MatchEnd();

	// Move on to the next round
	UFUNCTION(BlueprintCallable, Category = "Round", Exec)
    virtual void NextRound();

    // Overwrite this depending on the mode to determine who is the winner on a timelimit
    UFUNCTION(BlueprintNativeEvent, Category = Gameplay)
			void TimeLimitVictoryConditions();
    virtual void TimeLimitVictoryConditions_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = Gameplay)
			void OnRoundStarted();
	virtual void OnRoundStarted_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = Gameplay)
			void OnRoundEnded();
	virtual void OnRoundEnded_Implementation();

	// Check victory conditions per tick
	UFUNCTION(BlueprintNativeEvent, Category = Gameplay)
			void CheckRoundEnd(float DeltaSeconds);
    virtual void CheckRoundEnd_Implementation(float DeltaSeconds);

	UFUNCTION(NetMulticast, Reliable, Category = Gameplay)
	void Multicast_SetWinningTeam(ETeamType WinningTeam);
	void Multicast_SetWinningTeam_Implementation(ETeamType WinningTeam);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	float RoundEndResetDelay = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	float MatchEndResetDelay = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PVP Settings")
	TSubclassOf<class URoundEndWidget_PVP> RoundEndWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Round")
	uint8 bIncrementedRoundCounterThisRound : 1;

private:
	void UpdatePlayerLeaderboardStats();

	// check the victory conditions every so often in case of situations like players leaving on round end and nobody existing on next round...
	FTimerHandle CheckVictoryConditions_Handle;
};
