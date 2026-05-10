// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "DestructibleVehicle.generated.h"

DECLARE_STATS_GROUP(TEXT("DestructibleVehicle"), STATGROUP_DestructibleVehicle, STATCAT_Advanced);

UCLASS(ClassGroup=DestructibleVehicle, meta=(BlueprintSpawnableComponent))
class READYORNOT_API UDestructibleVehicleBodyPart : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UDestructibleVehicleBodyPart();
	
	UPROPERTY(EditAnywhere)
	bool bCanBeShotOff = false;

	UPROPERTY(EditAnywhere, meta=(EditCondition="bCanBeShotOff"))
	float Health = 30.0f;

	UPROPERTY(Transient)
	bool bBroken = false;
};

UCLASS(ClassGroup=DestructibleVehicle, HideCategories=(ArrowComponent), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UDestructibleVehicleParticleComponent : public UArrowComponent
{
	GENERATED_BODY()

public:
	UDestructibleVehicleParticleComponent();
	
	void PlayEffects();
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* ParticleSystem;
};

UCLASS(ClassGroup=DestructibleVehicle, meta=(BlueprintSpawnableComponent))
class READYORNOT_API UDestructibleVehicleGlassComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UDestructibleVehicleGlassComponent();
};

/*
 *	Destructible vehicle, simulates vehicle related effects such as alarms, tire deflation, airbag, etc.
 */
UCLASS()
class READYORNOT_API ADestructibleVehicle : public AActor
{
	GENERATED_BODY()

public:
	ADestructibleVehicle();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	// Allows vehicle tires to be punctured at higher performance cost
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle")
	bool bComplexVehicle = true;

	// Whether or not the lights should be enabled by default
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle")
	bool bLightsOn = false;

	// FMOD event to play when the car body takes damage
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle")
	UFMODEvent* BodyImpactEvent;

	// FMOD event to play when a breakable body part is hit
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle")
	UFMODEvent* BodyBreakEvent;
	
	// Chance to cause the car alarm to trigger when taking damage
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Alarm")
	float ChanceToCauseAlarmOnDamage = 1.0f;

	// The amount of time to play the car alarm sound
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Alarm")
	float AlarmPlayLength = 30.0f;

	// The interval at which the headlights flash and materials toggle
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Alarm")
	float AlarmHeadLightsFlashInterval = 1.0f;

	// Chance to cause the airbag to deploy when taking damage
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Airbag")
	float ChanceToCauseAirbagToDeployOnDamage = 1.0f;

	// Particle effect to play when the airbag is deployed
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Airbag")
	UParticleSystem* AirbagParticle;

	// FMOD event to play when the airbag is deployed
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Airbag")
	UFMODEvent* AirbagEvent;
	
	// The amount of each glass destructible can take before shattering
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Glass")
	float GlassHealth = 150.0f;

	// The amount of damage to apply to the glass destructible on break, impacts glass shatter force
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Glass")
	float GlassDamageToApply = 1000.0f;

	// List of possible materials that can be applied to the glass when it is shattered
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Glass")
	TArray<UMaterialInstance*> RandomShatteredGlassMaterial;
	
	// FMOD event to play when glass takes damage but is not destroyed
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Glass")
	UFMODEvent* GlassImpactEvent;

	// FMOD event to play when glass is destroyed
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Glass")
	UFMODEvent* GlassBreakEvent;
	
	// The amount of roll to apply to the vehicle when a tire is punctured
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Tires")
	float RollAmountOnTireDamage = 2.0f;

	// The amount of pitch to apply to the vehicle when a tire is punctured
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Tires")
	float PitchAmountOnTireDamage = 2.0f;

	// The amount to sink the vehicle body on the Z axis when a tire is punctured
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Tires")
	float SinkAmountOnTireDamage = -2.0f;

	// The interpolation speed when a tire is deflating
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Tires")
	float TireDeflationInterpSpeed = 0.25f;

	// FMOD event to play when a tire is deflated
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Tires")
	UFMODEvent* TireDeflateEvent;
	
	// Optional flat tire mesh to use
	UPROPERTY(EditDefaultsOnly, Category="Destructible Vehicle|Tires")
	UStaticMesh* FlatTireMesh;

	// Enable this if the vehicle does not have individually destructible lights
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	bool bUseSimplifiedLights = false;
	
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* SimplifiedLightsOnMaterial;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* SimplifiedLightsOffMaterial;
	
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* FrontLightsOnMaterial;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* FrontLightsOffMaterial;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* RearLightsOnMaterial;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	UMaterialInstance* RearLightsOffMaterial;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	int32 SimplifiedLightsMaterialIndex = 0;
	
	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	int32 FrontLeftLightsMaterialIndex = 0;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	int32 FrontRightLightsMaterialIndex = 0;

	UPROPERTY(EditAnywhere, Category="Destructible Vehicle|Lights")
	int32 RearLightsMaterialIndex = 0;
	
private:
	FVector DefaultLocation;
	FRotator DefaultRotation;

	bool bAirbagDeployed = false;
	bool bAlarmDeployed = false;
	bool bForceLightsOn = false;
	bool bHasValidLights = false;

	UPROPERTY()
	bool bLeftLightDestroyed = false;

	UPROPERTY()
	bool bRightLightDestroyed = false;

	UPROPERTY(Replicated)
	bool bFrontLeftTireDestroyed = false;

	UPROPERTY(Replicated)
	bool bFrontRightTireDestroyed = false;

	UPROPERTY(Replicated)
	bool bRearLeftTireDestroyed = false;

	UPROPERTY(Replicated)
	bool bRearRightTireDestroyed = false;
	
	UPROPERTY()
	TMap<UDestructibleVehicleGlassComponent*, float> GlassHealthMap;
	
	FTimerHandle CarAlarmTimer;
	FTimerHandle FlashHeadLightsTimer;
	FTimerHandle DestroyWindowChildrenTimer;

	UFUNCTION()
	void StopCarAlarm();

	UFUNCTION()
	void FlashHeadLights();

	bool HandleGlassDamage(UDestructibleVehicleGlassComponent* Glass, float DamageAmount);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DeployCarFeatures(bool bAirbag, bool bCarAlarm, bool bDisableLeftLight, bool bDisableRightLight);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BreakBodyPart(UDestructibleVehicleBodyPart* BodyPart);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BreakGlass(UDestructibleVehicleGlassComponent* Glass);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShatterGlass(UDestructibleVehicleGlassComponent* Glass);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayTireDestroyedEffects(UStaticMeshComponent* TireMesh);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAudioEvent(UFMODEvent* Event, FVector_NetQuantize Location);

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* CarBody;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* AirBag;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UArrowComponent* AirBagEffects;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* FrontLeftTire;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* FrontRightTire;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* RearLeftTire;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* RearRightTire;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USphereComponent* LeftLightCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USphereComponent* RightLightCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USpotLightComponent* LeftHeadLight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USpotLightComponent* RightHeadLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UFMODAudioComponent* AlarmAudio;
};
