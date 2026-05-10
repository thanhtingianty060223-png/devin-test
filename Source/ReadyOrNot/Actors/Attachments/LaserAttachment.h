// Copyright Void Interactive, 2022

#pragma once

#include "Actors/Attachments/WeaponAttachment.h"
#include "LaserAttachment.generated.h"

UCLASS()
class READYORNOT_API ULaserAttachment final : public UWeaponAttachment
{
	GENERATED_BODY()

public:
	ULaserAttachment();
	
	UPROPERTY(EditAnywhere, Category = "Laser")
	bool bRequireNVG = true;

	UFUNCTION(BlueprintCallable, Category = "Laser")
	void ToggleLaser(bool bOn);
	
	UFUNCTION(BlueprintPure, Category = "Laser")
	bool IsLaserOn() const;
	
	void AttachLaser();
	void DetachLaser();

	UPROPERTY()
	UParticleSystemComponent* LaserParticleComponent = nullptr;

	UPROPERTY()
	UParticleSystemComponent* LaserBeamEndComponent = nullptr;
	
	// attaches to socket 'laser' on the skeletalmesh specified above..
	UPROPERTY(EditAnywhere, Category = "Particle")
	UParticleSystem* LaserParticle = nullptr;

	// spawns at the end of the laser beam location on the walls normal and hit location
	UPROPERTY(EditAnywhere, Category = "Particle")
	UParticleSystem* LaserBeamEnd = nullptr;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_On)
	bool bRepOn = false;

	UFUNCTION()
	void OnRep_On();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;

	UPROPERTY(EditDefaultsOnly, Category = "Lens Flare")
	TSubclassOf<class ALensFlare> LensFlareClass = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lens Flare")
	class ALensFlare* SpawnedLensFlare = nullptr;
};
