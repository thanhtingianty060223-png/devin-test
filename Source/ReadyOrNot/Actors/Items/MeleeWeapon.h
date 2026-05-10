// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseItem.h"
#include "MeleeWeapon.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AMeleeWeapon : public ABaseWeapon
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, Category = "Melee")
	FString MeleeMontage = "tp_spct_knife";
	UPROPERTY(EditAnywhere, Category = "Melee")
	bool bApplyBleed = true;
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeDamage = 50.0f;
};
