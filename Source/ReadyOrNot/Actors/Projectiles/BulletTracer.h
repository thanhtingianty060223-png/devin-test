// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletTracer.generated.h"

UCLASS()
class READYORNOT_API ABulletTracer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABulletTracer();

	UPROPERTY()
	USceneComponent* RootComp;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void StartParticle(FVector Start, FVector End);
	void StartSmoke(FVector Start, FVector End);

	FVector StartLoc, EndLoc;
	bool bInterpLoc = false;

	bool bIsSmoke = false;


	UPROPERTY()
	UParticleSystemComponent* ParticleComponent;

	UPROPERTY()
	UParticleSystem* TracerParticle;

	UPROPERTY()
	UParticleSystem* SmokeParticle;

};
