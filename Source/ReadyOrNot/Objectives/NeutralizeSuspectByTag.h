// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Objectives/Objective.h"
#include "NeutralizeSuspectByTag.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ANeutralizeSuspectByTag : public AObjective/*, public IListenForSpawn, public IListenForArrest, public IListenForDeath, public IListenForReport, public IListenForGameStart, public IListenForIncapacitation*/
{
	GENERATED_BODY()

	virtual void TickObjective() override;
	virtual void PostInitializeComponents() override;

	UPROPERTY()
	ACyberneticCharacter* Suspect = nullptr;

public:
	UPROPERTY(EditAnywhere, Category = "Neutralization Properties")
	FName SuspectTag;

	UPROPERTY(EditAnywhere, Category = "Neutralization Properties")
	bool bRequireArrest = false;

	UPROPERTY(EditAnywhere, Category = "Neutralization Properties")
	bool bAllowIncapacitation = true;

	bool HasNeutralizedAIByTag(bool& bArrested); 

	FORCEINLINE FName GetSuspectTag() const { return SuspectTag; }

	UFUNCTION()
	void OnSuspectKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	UFUNCTION()
	void OnAISpawned();

};
