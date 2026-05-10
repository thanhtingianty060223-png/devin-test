// Copyright Void Interactive, 2021

#pragma once

#include "Interfaces/ListenForDeath.h"
#include "Interfaces/ListenForGameStart.h"
#include "Objectives/Objective.h"
#include "RescueAllOfTheCivilians.generated.h"

UCLASS()
class READYORNOT_API ARescueAllOfTheCivilians : public AObjective, public IListenForDeath, public IListenForGameStart
{
	GENERATED_BODY()

	ARescueAllOfTheCivilians();
	
	virtual void PostInitializeComponents() override;

	virtual void TickObjective() override;

	UFUNCTION()
	void OnAISpawned();
	UFUNCTION()
	void OnCivilianKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
};
