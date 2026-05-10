// ÂCopyright Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "VIPEscortGS.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AVIPEscortGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	AVIPEscortGS();
	
	virtual void Tick(float DeltaSeconds) override;

	void OnResetLevel();

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	float HoldVIP_TimeRemaining;

	UPROPERTY(ReplicatedUsing = OnRep_VIPArrested, BlueprintReadWrite, Category = VIP)
	bool bVIPArrested;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	bool bCanKillVIP;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	APlayerCharacter* VIPCharacter;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	APlayerController* VIPPlayer;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	class AReadyOrNotPlayerState* VIPPlayerState;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	ETeamType LastWinningTeam;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	bool bVIPSelected = false;

	UPROPERTY(ReplicatedUsing = OnRep_VIPKilled, BlueprintReadWrite, Category = VIP)
	bool bVIPKilled = false;
	
	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	ETeamType CurrentVIPTeam = ETeamType::TT_NONE;
	
	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	AReadyOrNotCharacter* RecentArrester = nullptr;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	AReadyOrNotCharacter* RecentFreer = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadWrite, Category = VIP)
	AReadyOrNotCharacter* RecentVIPKiller = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VIP)
	FText VIPRescueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VIP)
	FText VIPExecuteText;

private:
	UFUNCTION()
	void OnRep_VIPArrested();

	UFUNCTION()
	void OnRep_VIPKilled();
};
