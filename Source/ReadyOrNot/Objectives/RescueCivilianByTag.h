// Copyright Void Interactive, 2021

#pragma once

#include "Objectives/Objective.h"
#include "RescueCivilianByTag.generated.h"

UCLASS()
class READYORNOT_API ARescueCivilianByTag : public AObjective, public IListenForDeath, public IListenForGameStart
{
	GENERATED_BODY()

	virtual void PostInitializeComponents() override;

public:

	UPROPERTY(EditAnywhere, Category = "Rescue Properties")
	FName CivilianTag;

	UPROPERTY()
	AReadyOrNotCharacter* Civilian = nullptr;

	UPROPERTY(EditAnywhere, Category = "Rescue Properties")
	bool bAllowIncapacitation = true;

	UFUNCTION(BlueprintPure)
	bool HasNeutralizedCivilianByTag(bool& bArrested);

	virtual void TickObjective() override;
	
	UFUNCTION()
	void OnAISpawned();
	UFUNCTION()
	virtual void OnCivilianKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
};
