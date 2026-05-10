// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PenetrationData.generated.h"

/**
 *	Material penetration data, properties determine ballistic response
 */
USTRUCT(BlueprintType)
struct FMaterialPenetration
{
	GENERATED_BODY()

	// Whether or not this material is penetrable
	UPROPERTY(EditAnywhere, Category = "Penetration")
	bool bIsPenetrable = false;
	
	// Penetration density, multiplies penetration distance to represent easier and more difficult to penetrate materials
	UPROPERTY(EditAnywhere, Category = "Penetration", meta=(EditCondition="bIsPenetrable"))
	float PenetrationDensity = 1.0f;

	// Bullet projectile only penetration multiplier
	UPROPERTY(VisibleAnywhere, Category = "Penetration")
	float PenetrationMultiplier = 1.0f;
	
	// The armour level of this material, higher levels block more powerful rounds
	UPROPERTY(EditAnywhere, Category = "Penetration", meta=(EditCondition="bIsPenetrable"))
	uint8 ArmourLevel = 0;

	// Whether or not this surface can be ricocheted off of
	UPROPERTY(EditAnywhere, Category = "Penetration")
	bool bCanRicochet = true;

	// Ricochet chance multiplier, multiplied by ammunition type ricochet chance
	UPROPERTY(EditAnywhere, Category="Penetration", meta=(EditCondition="bCanRicochet"))
	float RicochetChanceMultiplier = 1.0f;

	// Ricochet chance multiplier, multiplied by ammunition type ricochet chance
	UPROPERTY(EditAnywhere, Category="Penetration")
	float SpallingChance = 0.0f;
};

/**
 *	Penetration data, maps all in-game materials to their respective penetration data
 */
UCLASS()
class READYORNOT_API UPenetrationData : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Penetration")
	FMaterialPenetration GetPenetrationData(EPhysicalSurface Surface);

private:
	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration DefaultPenetrationData;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Aluminium;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Asphalt;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Brick;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_CarbonFibre;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Cardboard;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Ceramic;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_ConcreteSoft;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_ConcreteStrong;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Dirt;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Drywall;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Electrical;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_EnergyShield;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Fabric_Carpet;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Fabric_Stuffing;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Fabric_Thin;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Flesh;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Galvanized;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Glass_Plate;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Glass_Windshield;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Grass;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Gravel;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Ice;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Lava;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Lead;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Leaves;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Limestone;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Mahogany;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Marble_Coated;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Marble_Thick;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Mud;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Oil;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Paper;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Pine;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Plaster;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Plastic;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Plywood;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Polystyrene;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Powder;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Rock;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Rubber;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Sand;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Snow;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Soil;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Steel;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Tin;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Treewood;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Wallpaper;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Water;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Vehicle;

	UPROPERTY(EditAnywhere, Category = "Impact")
	FMaterialPenetration RON_Bulletproof_Glass;
};
