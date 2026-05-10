// Copyright Void Interactive, 2023

#pragma once

#include "ThrownItem.h"
#include "ThrownChemlight.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class AThrownChemlight : public AThrownItem
{
	GENERATED_BODY()

public:
	AThrownChemlight();

	virtual void Tick(float DeltaTime) override;
	
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override final;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UPointLightComponent* LightSource;

	// The color of the chemlight's emissive and point light
	UPROPERTY(EditAnywhere)
	FLinearColor ChemlightColor = FLinearColor::Green;

	// The emissive strength at full brightness
	UPROPERTY(EditAnywhere)
	float EmissiveBrightness = 7.0f;

	// The intensity of the point light at full brightness
	UPROPERTY(EditAnywhere)
	float LightIntensity = 45.0f;

	// Rate that the chemlight strength increases
	UPROPERTY(EditAnywhere)
	float InitialGlowSpeed = 2.0f;

	// Rate at which the chemlight should dim
	UPROPERTY(EditAnywhere)
	float LightDimSpeed = 30.0f;

	// Rate at which the chemlight should dim when destroyed
	UPROPERTY(EditAnywhere)
	float DestroyedDimSpeed = 30.0f;
	
	// The time at which the chemlight should begin to dim (in seconds)
	UPROPERTY(EditAnywhere)
	float StartDimTime = 1800.0f;

	// How long it takes for the chemlight to despawn all together (in seconds)
	UPROPERTY(EditAnywhere)
	float TotalLifeTime = 2000.0f;

	// Decal that should be spawned on the ground when this chemlight is destroyed
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DestroyedDecal;

	// Offset to center the light onto the chemlight
	UPROPERTY(EditAnywhere)
	float LightZOffset = 5.0f;

	// How much the light should hover above the actual chemlight mesh
	UPROPERTY(EditAnywhere)
	float LightAdditionalHeight = 5.0f;
	
	// The size of the destroyed chemlight decal
	UPROPERTY(EditDefaultsOnly)
	float DestroyedDecalSize = 50.0f;
	
	float CurrentLifeTime = 0.0f;
	float CurrentStrength = 0.0f;

	bool bChemlightDestroyed = false;
	
	UPROPERTY()
	UMaterialInstanceDynamic* ChemlightMaterialInstance;
};
