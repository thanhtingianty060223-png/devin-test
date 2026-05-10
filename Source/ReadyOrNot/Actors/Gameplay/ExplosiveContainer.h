// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplosiveContainer.generated.h"

UCLASS()
class READYORNOT_API AExplosiveContainer : public AActor
{
	GENERATED_BODY()
	
public:	
	AExplosiveContainer();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* FireEffectParticle;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ExplosionEffectParticle;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UFMODAudioComponent* FMODFireAudioComponent;

	bool bTriggered = false;
	bool bExploded = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHideMeshAfterDetonation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scorch")
	UMaterialInterface* ScorchDecal = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Explosion)
	UFMODEvent* FMODExplosionAudio;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Explosion)
	TSubclassOf<UStunDamage> StunDamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Explosion)
	TSubclassOf<ULegacyCameraShake> ExplosionScreenShake;

	UPROPERTY()
	AController* ExplosionInstigator;

	void Explode();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_TriggerExplosive();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosionEffects();

	UPROPERTY(EditAnywhere)
	float TimerUntilExplosionOnceTriggered = 3.0f;

	UPROPERTY(EditAnywhere)
	float MinDamageToTrigger = 1.0f;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
};
