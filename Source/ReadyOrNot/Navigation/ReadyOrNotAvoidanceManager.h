// Void Interactive, 2020

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ReadyOrNotAvoidanceManager.generated.h"

DECLARE_STATS_GROUP(TEXT("ReadyOrNotAvoidanceManager"), STATGROUP_ReadyOrNotAvoidanceManager, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UReadyOrNotAvoidanceManager final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	UReadyOrNotAvoidanceManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	bool IsOverlapping(ACyberneticCharacter* AI, ACyberneticCharacter*& OverlappingAI) const;
	bool CanOverrideLocation(ACyberneticCharacter* AI) const;
	void OverrideActivityLocation(ACyberneticCharacter* AI, FVector Location);
	
	FVector GetBestFreeLocation(ACyberneticCharacter* AI, ACyberneticCharacter* OverlappingAI) const;

	int32 AvoidanceIdx = 0;

	float TickInterval = 0.0f;
	float TimeSinceLastTick = 0.0f;
};
