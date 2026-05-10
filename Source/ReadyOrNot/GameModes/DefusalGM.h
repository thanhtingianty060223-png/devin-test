// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameMode_PVP.h"
#include "GameModes/DefusalGS.h"
#include "DefusalGM.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ADefusalGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

	UPROPERTY()
	ABombActor* SelectedBombActor;

	UPROPERTY()
	TArray<AAISpawn*> SpawnPoints;

	UPROPERTY()
	APlayerStart* SwatSpawn;

	UPROPERTY()
	APlayerStart* SuspectSpawn;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void NextRound() override;
	
	void ResetBomb();
	void ResetAI();

	UPROPERTY()
	TMap<APlayerCharacter*, FCharacterLookOverride> CharacterLookMap;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* BlankFaceMesh;

	virtual void RoundWon(TArray<AReadyOrNotPlayerState*> WinningPlayers) override;
	virtual void StartMatch() override;
	virtual void RoundWonTeam(ETeamType WinningTeam) override;
	
	void SwapSides();
	
	void SetDefusalMatchState(EDefusalMatchSate NewMatchState);
	EDefusalMatchSate GetDefusalMatchState();
	ADefusalGS* GetDefusalGameState();

	UPROPERTY()
	TArray<APlayerController*> PendingPlayerSpawn;

	UPROPERTY(EditAnywhere)
	TArray<FSpawnData> SuspectSpawnData;
	
	UPROPERTY(EditAnywhere)
	USkeletalMesh* SuspectFPArmsOverride;
};
