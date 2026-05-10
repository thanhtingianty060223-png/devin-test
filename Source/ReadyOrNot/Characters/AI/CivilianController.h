// Copyright Void Interactive, 2022

#pragma once

#include "Characters/CyberneticController.h"
#include "CivilianController.generated.h"

UCLASS()
class READYORNOT_API ACivilianController final : public ACyberneticController
{
	GENERATED_BODY()

public:
	virtual float GetReactionTime(const EActorSenseType& SenseType) const override;
	
protected:
	virtual void OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;
	virtual void OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;

	virtual void OnSeenGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation) override;
	virtual void OnHeardGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation, const FName& InTag) override;

	virtual bool IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const override;
};
