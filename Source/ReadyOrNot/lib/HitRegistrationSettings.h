// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "HitRegistrationSettings.generated.h"

/*
 * Hit registration settings for any options we might want to be configurable in a packaged game
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Hit Registration"))
class READYORNOT_API UHitRegistrationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	bool bEnableValidation = true;
	
	UPROPERTY(Config, EditAnywhere)
	float HitscanForgiveness = 50.0f;

	UPROPERTY(Config, EditAnywhere)
	float ShooterForgiveness = 250.0f;

	UPROPERTY(Config, EditAnywhere)
	float TraceDistanceForgiveness = 5.0f;

	UPROPERTY(Config, EditAnywhere)
	float SnapshotVelocityFactor = 1.0f;
	
	UPROPERTY(Config, EditAnywhere)
	FVector MinimumSnapshotExtent = FVector(75.0f, 75.0f, 90.0f);
};
