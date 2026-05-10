// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "PickupActor.h"
#include "BaseWeapon.h"
#include "Attachments/WeaponAttachment.h"
#include "PickupWeaponActor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API APickupWeaponActor : public APickupActor
{
	GENERATED_BODY()
public:	
	APickupWeaponActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponPickup, Meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ABaseWeapon> Weapon;

	// Whether to delete the item from the world when we've picked it up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponPickup, Meta = (AllowPrivateAccess = "true"))
	bool bKillOnPickup;

	// Whether this is a primary or a secondary weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponPickup, Meta = (AllowPrivateAccess = "true"))
	bool bSecondaryWeapon;

	// Whether to modify the player's loadout when picking this item up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponPickup, Meta = (AllowPrivateAccess = "true"))
	bool bModifyLoadout;

	// The scope attachment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponPickup|Attachments")
	TSubclassOf<UWeaponAttachment> ScopeAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponPickup|Attachments")
	TSubclassOf<UWeaponAttachment> MuzzleAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponPickup|Attachments")
	TSubclassOf<UWeaponAttachment> UnderbarrelAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponPickup|Attachments")
	TSubclassOf<UWeaponAttachment> OverbarrelAttachment;

	virtual void BeginPlay() override;
	virtual void ActorPickedUp(AActor* InPickupInstigator) override;
};
