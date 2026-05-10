// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "LightningGenerator.generated.h"

// Lightning Generators....generate lightning. 
// Or more specifically, they are a place where lightning and thunder can play.
// The only native part of this is the timer controls. If you aren't using a Blueprint for this, you're doing it wrong!
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API ALightningGenerator : public AActor
{
	GENERATED_BODY()

protected:
	FTimerHandle LightningHandle;
	FTimerHandle ThunderHandle;
	
public:
	ALightningGenerator();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Root Scene Component
	UPROPERTY(BlueprintReadOnly)
	USceneComponent* SceneRoot;

	// The component responsible for playing sounds
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	UAudioComponent* Thunder;

	// The component responsible for playing particle effects
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	UParticleSystemComponent* ParticleComponent;

	// The random chance (0..1) of a particle playing
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	float ParticleSpawnChance;

	// The different particle effects that can play
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	TArray<UParticleSystem*> ParticleTemplates;

	// The sounds that can play from the thunder component
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	TArray<USoundBase*> ThunderSounds;

	// The light that plays
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Lightning)
	UDirectionalLightComponent* Lightning;

	// The function that is called when we want to play lightning
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Lightning)
	void PlayLightning();

	// The function that is called when we want to play thunder
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Lightning)
	void PlayThunder();

	// Amount of time between playing lightning and playing thunder (minimum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float ThunderDelayMin;

	// Amount of time between playing lightning and playing thunder (maximum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float ThunderDelayMax;

	// Color (min) to randomize
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	FLinearColor LightningColorMin;

	// Color (max) to randomize
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	FLinearColor LightningColorMax;

	// Amount of time between lightning strikes (minimum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningDelayMin;

	// Amount of time between lightning strikes (maximum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningDelayMax;

	// Amount of initial lightning intensity (min)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityMin;

	// Amount of initial lightning intensity (max)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityMax;

	// How fast the lightning intensity decays
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityDecay;

	// How much to jitter the lightning intensity by (minimum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityJitterMin;

	// How much to jitter the lightning intensity by (maximum)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityJitterMax;

	// How long between lightning jitters (min)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityJitterTimeMin;

	// How long between lightning jitters (max)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Lightning)
	float LightningIntensityJitterTimeMax;

	// How long before we can jitter again
	UPROPERTY(BlueprintReadOnly, Category = Lightning)
	float LightningJitterTimeRemaining;
};
