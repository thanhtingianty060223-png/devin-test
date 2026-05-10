// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "WeaponCacheActor.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AWeaponCacheActor : public AActor
{
	GENERATED_BODY()
public:	
	AWeaponCacheActor();

	UPROPERTY(EditAnywhere, Category = "Weapon Cache")
	TArray<TSubclassOf<ABaseMagazineWeapon>> AvailableWeapons;

	UFUNCTION(BlueprintPure, Category = "Weapon Cache")
	TSubclassOf<ABaseMagazineWeapon> GetRandomAvailableWeapon() const;

	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	#endif

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UArrowComponent* ArrowComponent = nullptr;
};
