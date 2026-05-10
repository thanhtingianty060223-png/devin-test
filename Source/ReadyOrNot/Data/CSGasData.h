// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/Environment/PepperGasCloud.h"
#include "UObject/Object.h"
#include "CSGasData.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCSGasData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	//EQS to run for generation gas and safe points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	UEnvQuery* GasGenerationQuery;

	//Damage type to apply when in range of the gas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	TSubclassOf<UDamageType> DamageType;

	//Class for the gas source caused by several pepper balls in location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	TSubclassOf<APepperGasCloud> PepperGasCloud;

	//Extra buffer distance added to gas source radii for calculating safe points for AI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	float SafePointsBufferDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gas)
	FVector NavModifierExtents;
};
