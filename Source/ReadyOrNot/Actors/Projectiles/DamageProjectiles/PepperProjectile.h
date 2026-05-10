// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "BulletProjectile.h"
#include "PepperProjectile.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API APepperProjectile : public ABulletProjectile
{
	GENERATED_BODY()

	APepperProjectile();
	
protected:
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameSessionName, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* GasEffect;
};
