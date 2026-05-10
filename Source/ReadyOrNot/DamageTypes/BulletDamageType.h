// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/DamageType.h"
#include "BulletDamageType.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UBulletDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Gameplay)
	TSubclassOf<class UUserWidget> HitVisuals;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	TSubclassOf<ULegacyCameraShake> HitShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bArmorPiercing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bNonLethal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float AggressionChangeInAI = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float HeadDamageMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float UpperBodyDamageMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float LowerBodyDamageMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float ArmDamageMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float LegDamageMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limb Damage")
	float FootDamageMultiplier = 0.1f;
};
