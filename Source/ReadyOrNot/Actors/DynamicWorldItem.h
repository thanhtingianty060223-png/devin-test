// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FMODAudioComponent.h"
#include "DynamicWorldItem.generated.h"

UCLASS()
class READYORNOT_API ADynamicWorldItem : public AActor
{
	GENERATED_BODY()
	
public:	
	ADynamicWorldItem();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ImpactParticle;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UFMODAudioComponent* ImpactAudioFMOD;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UStaticMesh* PostImpactMesh;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UMaterialInterface* PostImpactMaterial;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	UMaterialInterface* PhysicsImpactDecal;

	UPROPERTY(EditAnywhere, Category = "Tweaks")
	float PhysicsImpactDecalScale = 1.0f;

public:	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	// We use a destroyed locally variable just in case the unreliable RPC failed
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_ItemDestroyed)
	bool bItemDestroyed = false;
	bool bItemDestroyedLocally = false;

	UFUNCTION()
	void OnRep_ItemDestroyed();

	// NOTE(killo): reliable for now until we can optimize rpc / replication so this doesnt look glitchy
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyItem();

	// Called on both client and server by the server only when this item is destroyed
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void OnItemDestroyed();
};


