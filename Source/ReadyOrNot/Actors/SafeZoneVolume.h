// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/AISpawn.h"
#include "SafeZoneVolume.generated.h"

UCLASS()
class READYORNOT_API ASafeZoneVolume : public AVolume
{
	GENERATED_BODY()

	public:
		ASafeZoneVolume();

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<ASafeZoneVolume*> NextSafeZones;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<AAISpawn*> AIBlacklist;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<AAISpawn*> AIWhitelist;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool bIsFirstSafeZone = false;
};
