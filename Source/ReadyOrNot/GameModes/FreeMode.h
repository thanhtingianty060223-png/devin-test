// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameMode.h"
#include "FreeMode.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AFreeMode : public AReadyOrNotGameMode
{
	GENERATED_BODY()
	
public:

	AFreeMode();

	UPROPERTY(EditAnywhere, Category = Respawn)
		float RespawnTime = 5.0f;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

	virtual void PlayerArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;

	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;

	// don't do anything here, spawn players individually..
	virtual void RespawnDeadPlayers() override;


	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName /* = TEXT("") */) override;
};
