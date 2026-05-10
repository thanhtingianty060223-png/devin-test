// Copyright Void Interactive, 2023

#pragma once

#include "Characters/AI/SWATCharacter.h"
#include "TrailerSWATCharacter.generated.h"

UCLASS()
class READYORNOT_API ATrailerSWATCharacter final : public ASWATCharacter
{
	GENERATED_BODY()

public:
	ATrailerSWATCharacter();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void Tick_Authority(float DeltaSeconds) override;
	
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const override;
	virtual void FellOutOfWorld(const UDamageType& dmgType) override;
};
