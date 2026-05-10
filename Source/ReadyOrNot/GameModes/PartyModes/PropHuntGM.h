// Copyright Void Interactive, 2023

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "PropHuntGM.generated.h"

UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API APropHuntGM final : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prop Hunt")
	TSubclassOf<ACharacter> PropHuntCharacterClass = nullptr;

	virtual void StartMatch() override;
	virtual void OnRoundStarted_Implementation() override;

	virtual void NextRound() override;
	virtual void RespawnAllPlayers() override;
	
	void RespawnPlayerProp(APlayerController* Controller);
	
	virtual void TimeLimitVictoryConditions_Implementation() override;

	virtual void CheckVictoryConditions() override;

	UPROPERTY()
	TArray<AReadyOrNotPlayerController*> Hunters;
	
	UPROPERTY()
	TArray<AReadyOrNotPlayerController*> Props;
};
