// Copyright Void Interactive, 2022

#pragma once

#include "Characters/CyberneticController.h"
#include "SuspectController.generated.h"

UCLASS()
class READYORNOT_API ASuspectController final : public ACyberneticController
{
	GENERATED_BODY()

public:
	virtual float GetReactionTime(const EActorSenseType& SenseType) const override;
	
protected:
	virtual void ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor) override;

	virtual void OnHeardActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus, float ExpiryTime = 10.0f) override;
	
	virtual void OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;
	virtual void OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;
	virtual void OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;

	virtual void OnHeardGunShot(AReadyOrNotCharacter* InInstigator, const FVector& WeaponLocation, const FName& InTag) override;

	virtual void OnTrackedTargetStartedReloading(APlayerCharacter* InCharacter) override;
	
	virtual bool IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const override;
};
