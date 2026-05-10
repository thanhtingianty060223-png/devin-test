// � Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "KingOfTheHillGM.generated.h"

/*
 *	Tug of War is a mode where each team must continually "use" a button to move a mover to a safe zone.
 *	Each team wins when they are able to get the mover to "their" side. 
 */

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AKingOfTheHillGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartMatch() override;
	virtual void RoundEnd() override;
	virtual void MatchEnd() override;
	virtual void ResetClientScores(bool bBetweenRounds) override;
	virtual void ResetLevel() override;

	bool AreAllPlayersOnTeamArrested(ETeamType Team);
	int32 GetNumberOfArrestedPlayersOnTeam(ETeamType Team);

	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void PlayerFreed(ACharacter* Freed, ACharacter* Freer) override;
	void CheckVictoryConditions();
	virtual void TimeLimitVictoryConditions_Implementation() override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;

	UPROPERTY(BlueprintReadOnly, Category = "Tug of War")
		TArray<class APlayerCharacter*> ArrestedBlueCharacters;

	UPROPERTY(BlueprintReadOnly, Category = "Tug of War")
		TArray<class APlayerCharacter*> ArrestedRedCharacters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* TOWVictorySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchLoopMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchStartMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UFMODEvent* MatchEndMusic;
};