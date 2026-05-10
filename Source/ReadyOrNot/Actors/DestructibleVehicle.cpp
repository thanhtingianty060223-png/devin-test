// Copyright Void Interactive, 2023

#include "DestructibleVehicle.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Destructible Vehicle Tick"), STAT_DestructibleVehicleTick, STATGROUP_DestructibleVehicle);

UDestructibleVehicleBodyPart::UDestructibleVehicleBodyPart()
{
	SetGenerateOverlapEvents(false);
}

UDestructibleVehicleParticleComponent::UDestructibleVehicleParticleComponent()
{
	ArrowColor = FColor::Cyan;
	ArrowSize = 0.75f;
	ArrowLength = 40.0f;
}

void UDestructibleVehicleParticleComponent::PlayEffects()
{
	if (UParticleSystemComponent* Component = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, this))
	{
		Component->AddRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		Component->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}

UDestructibleVehicleGlassComponent::UDestructibleVehicleGlassComponent()
{
	UPrimitiveComponent::SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	SetGenerateOverlapEvents(false);
}

ADestructibleVehicle::ADestructibleVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CarBody = CreateDefaultSubobject<UStaticMeshComponent>("CarBody");
	RootComponent = CarBody;
	CarBody->SetGenerateOverlapEvents(false);

	AirBag = CreateDefaultSubobject<UStaticMeshComponent>("AirBag");
	AirBag->SetupAttachment(CarBody, "AirBag");
	AirBag->SetGenerateOverlapEvents(false);

	AirBagEffects = CreateDefaultSubobject<UArrowComponent>("AirBagEffects");
	AirBagEffects->SetupAttachment(CarBody);

	FrontLeftTire = CreateDefaultSubobject<UStaticMeshComponent>("FrontLeftTyre");
	FrontLeftTire->SetupAttachment(CarBody, "FrontLeftTyre");
	FrontLeftTire->SetGenerateOverlapEvents(false);

	FrontRightTire = CreateDefaultSubobject<UStaticMeshComponent>("FrontRightTyre");
	FrontRightTire->SetupAttachment(CarBody, "FrontRightTyre");
	FrontRightTire->SetGenerateOverlapEvents(false);

	RearLeftTire = CreateDefaultSubobject<UStaticMeshComponent>("RearLeftTyre");
	RearLeftTire->SetupAttachment(CarBody, "RearLeftTyre");
	RearLeftTire->SetGenerateOverlapEvents(false);

	RearRightTire = CreateDefaultSubobject<UStaticMeshComponent>("RearRightTyre");
	RearRightTire->SetupAttachment(CarBody, "RearRightTyre");
	RearRightTire->SetGenerateOverlapEvents(false);

	LeftLightCollision = CreateDefaultSubobject<USphereComponent>("LeftLightCollision");
	LeftLightCollision->SetupAttachment(CarBody, "LeftHeadLightCollision");
	LeftLightCollision->SetGenerateOverlapEvents(false);

	RightLightCollision = CreateDefaultSubobject<USphereComponent>("RightLightCollision");
	RightLightCollision->SetupAttachment(CarBody, "RightHeadLightCollision");
	RightLightCollision->SetGenerateOverlapEvents(false);

	LeftHeadLight = CreateDefaultSubobject<USpotLightComponent>("LeftHeadLight");
	LeftHeadLight->SetupAttachment(LeftLightCollision);

	RightHeadLight = CreateDefaultSubobject<USpotLightComponent>("RightHeadLight");
	RightHeadLight->SetupAttachment(RightLightCollision);

	AlarmAudio = CreateDefaultSubobject<UFMODAudioComponent>("AlarmAudio");
	AlarmAudio->SetupAttachment(CarBody);
}

void ADestructibleVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADestructibleVehicle, bFrontLeftTireDestroyed);
	DOREPLIFETIME(ADestructibleVehicle, bFrontRightTireDestroyed);
	DOREPLIFETIME(ADestructibleVehicle, bRearLeftTireDestroyed);
	DOREPLIFETIME(ADestructibleVehicle, bRearRightTireDestroyed);
}

void ADestructibleVehicle::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();
	DefaultRotation = GetActorRotation();

	TArray<UDestructibleVehicleGlassComponent*> GlassComponents;
	GetComponents<UDestructibleVehicleGlassComponent>(GlassComponents);
	for (UDestructibleVehicleGlassComponent* GlassComponent : GlassComponents)
	{
		GlassHealthMap.Add(GlassComponent, GlassHealth);
	}
	
	if (!bAirbagDeployed)
		AirBag->SetVisibility(false);

	if (bUseSimplifiedLights)
	{
		bHasValidLights = SimplifiedLightsMaterialIndex >= 0
		&& SimplifiedLightsOnMaterial != nullptr
		&& SimplifiedLightsOffMaterial != nullptr;

#if WITH_EDITOR
		if (!bHasValidLights)
			UE_LOG(LogReadyOrNot, Warning, TEXT("Vehicle %s has invalid simple light material setup, lights disabled"), *GetNameSafe(this));
#endif
	}
	else
	{
		bHasValidLights = FrontLeftLightsMaterialIndex >= 0
		&& FrontRightLightsMaterialIndex >= 0
		&& RearLightsMaterialIndex >= 0
		&& FrontLightsOnMaterial != nullptr
		&& FrontLightsOffMaterial != nullptr
		&& RearLightsOnMaterial != nullptr
		&& RearLightsOffMaterial != nullptr;

#if WITH_EDITOR
		if (!bHasValidLights)
			UE_LOG(LogReadyOrNot, Warning, TEXT("Vehicle %s has invalid light material setup, lights disabled"), *GetNameSafe(this));
#endif
	}
	
	CarBody->SetMobility(bComplexVehicle ? EComponentMobility::Movable : EComponentMobility::Static);
}

void ADestructibleVehicle::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_DestructibleVehicleTick);
	Super::Tick(DeltaTime);

	if (bLightsOn || bForceLightsOn)
	{
		if (bUseSimplifiedLights)
		{
			if (bHasValidLights)
				CarBody->SetMaterial(SimplifiedLightsMaterialIndex, SimplifiedLightsOnMaterial);
			
			LeftHeadLight->SetVisibility(true);
			RightHeadLight->SetVisibility(true);
		}
		else
		{
			if (!bLeftLightDestroyed)
			{
				if (bHasValidLights)
					CarBody->SetMaterial(FrontLeftLightsMaterialIndex, FrontLightsOnMaterial);
				
				LeftHeadLight->SetVisibility(true);
			}

			if (!bRightLightDestroyed)
			{
				if (bHasValidLights)
					CarBody->SetMaterial(FrontRightLightsMaterialIndex, FrontLightsOnMaterial);
				
				RightHeadLight->SetVisibility(true);
			}

			if (bHasValidLights)
			{
				CarBody->SetMaterial(RearLightsMaterialIndex, RearLightsOnMaterial);
				CarBody->SetMaterial(RearLightsMaterialIndex, RearLightsOnMaterial);
			}
		}
	}
	else
	{
		if (bUseSimplifiedLights)
		{
			if (bHasValidLights)
				CarBody->SetMaterial(SimplifiedLightsMaterialIndex, SimplifiedLightsOffMaterial);
			
			LeftHeadLight->SetVisibility(false);
			RightHeadLight->SetVisibility(false);
		}
		else
		{
			LeftHeadLight->SetVisibility(false);
			RightHeadLight->SetVisibility(false);

			if (bHasValidLights)
			{
				CarBody->SetMaterial(FrontLeftLightsMaterialIndex, FrontLightsOffMaterial);
				CarBody->SetMaterial(FrontRightLightsMaterialIndex, FrontLightsOffMaterial);

				CarBody->SetMaterial(RearLightsMaterialIndex, RearLightsOffMaterial);
				CarBody->SetMaterial(RearLightsMaterialIndex, RearLightsOffMaterial);
			}
		}
	}
	
	if (!bComplexVehicle)
		return;

	float SinkAmount = 0.0f;
	if (bFrontLeftTireDestroyed)
		SinkAmount += SinkAmountOnTireDamage;

	if (bFrontRightTireDestroyed)
		SinkAmount += SinkAmountOnTireDamage;

	if (bRearLeftTireDestroyed)
		SinkAmount += SinkAmountOnTireDamage;

	if (bRearRightTireDestroyed)
		SinkAmount += SinkAmountOnTireDamage;

	FVector DesiredLocation = DefaultLocation + FVector(0.0f, 0.0f, SinkAmount);

	FVector FinalLocation = FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaTime, TireDeflationInterpSpeed);
	SetActorLocation(FinalLocation);

	FRotator AppliedRotation = DefaultRotation;
	if (!(bFrontLeftTireDestroyed && bFrontRightTireDestroyed && bRearLeftTireDestroyed && bRearRightTireDestroyed))
	{
		if (bFrontLeftTireDestroyed || bRearLeftTireDestroyed)
			AppliedRotation.Pitch -= RollAmountOnTireDamage;

		if (bFrontRightTireDestroyed || bRearRightTireDestroyed)
			AppliedRotation.Pitch += RollAmountOnTireDamage;

		if (bFrontLeftTireDestroyed || bFrontRightTireDestroyed)
			AppliedRotation.Roll += PitchAmountOnTireDamage;

		if (bRearLeftTireDestroyed || bRearRightTireDestroyed)
			AppliedRotation.Roll -= PitchAmountOnTireDamage;
	}

	FRotator FinalRotation = FMath::RInterpTo(GetActorRotation(), AppliedRotation, DeltaTime, TireDeflationInterpSpeed);
	SetActorRotation(FinalRotation);
}

float ADestructibleVehicle::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority())
		return 0.0f;
	
	// TODO(killo): allow certain non-damaging grenades to trigger car alarm (i.e. flashbang, but not CS gas)
	
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		
		bool bDeployAirbag = false;
		bool bDeployCarAlarm = false;
		bool bDisableLeftLight = false;
		bool bDisableRightLight = false;
		
		if (FMath::FRandRange(0.0f, 1.0f) < ChanceToCauseAlarmOnDamage)
		{
			bDeployCarAlarm = true;
		}

		// Don't break mirrors or lights or deploy airbags for things like physics collisions, pepperballs...
		if (DamageAmount > 2.0f)
		{
			if (FMath::FRandRange(0.0f, 1.0f) < ChanceToCauseAirbagToDeployOnDamage)
			{
				bDeployAirbag = true;
			}
			if (!bUseSimplifiedLights && PointDamageEvent->HitInfo.GetComponent() == LeftLightCollision)
			{
				bDisableLeftLight = true;
			}
			if (!bUseSimplifiedLights && PointDamageEvent->HitInfo.GetComponent() == RightLightCollision)
			{
				bDisableRightLight = true;
			}
		}

		Multicast_DeployCarFeatures(bDeployAirbag, bDeployCarAlarm, bDisableLeftLight, bDisableRightLight);

		// Don't destroy tires or break glass because of physics collisions, pepperballs...
		if (DamageAmount > 2.0f)
		{
			UPrimitiveComponent* HitComponent = PointDamageEvent->HitInfo.GetComponent();
			
			if (UDestructibleVehicleGlassComponent* Glass = Cast<UDestructibleVehicleGlassComponent>(HitComponent))
			{
				const FHitResult& HitResult = PointDamageEvent->HitInfo;
				if (HandleGlassDamage(Glass, DamageAmount))
				{
					Multicast_BreakGlass(Glass);
				}
				else if (GlassImpactEvent)
				{
					Multicast_PlayAudioEvent(GlassImpactEvent, HitResult.Location);
				}
			}
			else if (UDestructibleVehicleBodyPart* BodyPart = Cast<UDestructibleVehicleBodyPart>(HitComponent))
			{
				if (BodyPart->bCanBeShotOff && !BodyPart->bBroken)
				{
					BodyPart->Health -= DamageAmount;
					BodyPart->bBroken = BodyPart->Health <= 0.0f;
					if (BodyPart->bBroken)
					{
						Multicast_BreakBodyPart(BodyPart);
					}
				}
			}
			else
			{
				// Bit hacky but we want to play body impact events if we don't hit glass
				Multicast_PlayAudioEvent(BodyImpactEvent, PointDamageEvent->HitInfo.Location);
			}

			if (HitComponent == FrontLeftTire)
			{
				if (!bFrontLeftTireDestroyed)
				{
					bFrontLeftTireDestroyed = true;
					Multicast_PlayTireDestroyedEffects(FrontLeftTire);
				}
			}
			if (HitComponent == FrontRightTire)
			{
				if (!bFrontRightTireDestroyed)
				{
					bFrontRightTireDestroyed = true;
					Multicast_PlayTireDestroyedEffects(FrontRightTire);
				}
			}
			if (HitComponent == RearLeftTire)
			{
				if (!bRearLeftTireDestroyed)
				{
					bRearLeftTireDestroyed = true;
					Multicast_PlayTireDestroyedEffects(RearLeftTire);
				}
			}
			if (HitComponent == RearRightTire)
			{
				if (!bRearRightTireDestroyed)
				{
					bRearRightTireDestroyed = true;
					Multicast_PlayTireDestroyedEffects(RearRightTire);
				}
			}
		}
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		const FRadialDamageEvent* RadialDamageEvent = static_cast<const FRadialDamageEvent*>(&DamageEvent);
		if (DamageAmount >= 2.0f)
		{
			for (const FHitResult& HitResult : RadialDamageEvent->ComponentHits)
			{
				UDestructibleVehicleGlassComponent* Glass = Cast<UDestructibleVehicleGlassComponent>(HitResult.GetComponent());
				if (!Glass)
					continue;

				if (HandleGlassDamage(Glass, DamageAmount))
				{
					Multicast_BreakGlass(Glass);
				}
				else if (GlassImpactEvent)
				{
					Multicast_PlayAudioEvent(GlassImpactEvent, HitResult.Location);
				}
			}
		}
	}

	return 0.0f;
}

#if WITH_EDITOR
void ADestructibleVehicle::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	LeftHeadLight->SetVisibility(bLightsOn);
	RightHeadLight->SetVisibility(bLightsOn);
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ADestructibleVehicle::StopCarAlarm()
{
	AlarmAudio->Stop();

	GetWorld()->GetTimerManager().ClearTimer(FlashHeadLightsTimer);
	bForceLightsOn = bLightsOn;
}

void ADestructibleVehicle::FlashHeadLights()
{
	bForceLightsOn = !bForceLightsOn;
}

bool ADestructibleVehicle::HandleGlassDamage(UDestructibleVehicleGlassComponent* Glass, float DamageAmount)
{
	if (!IsValid(Glass))
		return false;

	if (GlassHealthMap.Contains(Glass))
	{
		// Only change material once
		if (GlassHealthMap[Glass] >= GlassHealth)
		{
			Multicast_ShatterGlass(Glass);
		}

		GlassHealthMap[Glass] -= DamageAmount;

		if (GlassHealthMap[Glass] > 0.0f)
			return false;
	}

	// We still apply damage to destructible even if it's not in our health map
	return true;
}

void ADestructibleVehicle::Multicast_DeployCarFeatures_Implementation(bool bAirbag, bool bCarAlarm, bool bDisableLeftLight, bool bDisableRightLight)
{
	if (bAirbag && !bAirbagDeployed)
	{
		bAirbagDeployed = true;
		AirBag->SetVisibility(true);
		
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AirbagParticle, AirBagEffects->GetComponentLocation(), AirBagEffects->GetComponentRotation() + FRotator(-90, 0, 0));
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), AirbagEvent, AirBagEffects->GetComponentTransform(), true);
	}
	if (bCarAlarm && !bAlarmDeployed)
	{
		bAlarmDeployed = true;
		AlarmAudio->Play();
		GetWorld()->GetTimerManager().SetTimer(CarAlarmTimer, this, &ADestructibleVehicle::StopCarAlarm, AlarmPlayLength, false);
		GetWorld()->GetTimerManager().SetTimer(FlashHeadLightsTimer, this, &ADestructibleVehicle::FlashHeadLights, AlarmHeadLightsFlashInterval, true);
	}

	if (bDisableLeftLight)
	{
		bLeftLightDestroyed = true;
		LeftHeadLight->SetVisibility(false);
	}

	if (bDisableRightLight)
	{
		bRightLightDestroyed = true;
		RightHeadLight->SetVisibility(false);
	}
}

void ADestructibleVehicle::Multicast_BreakBodyPart_Implementation(UDestructibleVehicleBodyPart* BodyPart)
{
	if (!IsValid(BodyPart))
		return;

	ensure(BodyPart->bCanBeShotOff);

	TArray<USceneComponent*> SceneComponents;
	BodyPart->GetChildrenComponents(true, SceneComponents);
	
	UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), BodyBreakEvent, BodyPart->GetComponentTransform(), true);
	//BodyPart->SetCollisionObjectType(ECC_Destructible);
	BodyPart->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BodyPart->SetSimulatePhysics(true);
	
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);
		if (!IsValid(PrimitiveComponent))
			continue;
		
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void ADestructibleVehicle::Multicast_BreakGlass_Implementation(UDestructibleVehicleGlassComponent* Glass)
{
	if (!IsValid(Glass))
		return;

	UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), GlassBreakEvent, Glass->GetComponentTransform(), true);
	
	TArray<USceneComponent*> GlassChildren;
	Glass->GetChildrenComponents(true, GlassChildren);
	
	for (USceneComponent* SceneComponent : GlassChildren)
	{
		if (!IsValid(SceneComponent))
			continue;

		UDestructibleVehicleParticleComponent* ParticleComponent = Cast<UDestructibleVehicleParticleComponent>(SceneComponent);
		if (ParticleComponent)
		{
			ParticleComponent->PlayEffects();
		}

		UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(SceneComponent);
		if (ChildActorComponent)
		{
			ChildActorComponent->DestroyChildActor();
		}
		
		SceneComponent->DestroyComponent();
	}

	Glass->DestroyComponent();
}

void ADestructibleVehicle::Multicast_ShatterGlass_Implementation(UDestructibleVehicleGlassComponent* Glass)
{
	if (!IsValid(Glass))
		return;

	UMaterialInterface* ShatteredMaterial = nullptr;
	if (RandomShatteredGlassMaterial.Num() > 0)
	{
		ShatteredMaterial = RandomShatteredGlassMaterial[FMath::RandRange(0, RandomShatteredGlassMaterial.Num() - 1)];
	}

	if (!ShatteredMaterial)
		return;
	
	for (int32 i = 0; i < Glass->GetMaterials().Num(); i++)
	{
		Glass->SetMaterial(i, ShatteredMaterial);
	}
}

void ADestructibleVehicle::Multicast_PlayTireDestroyedEffects_Implementation(UStaticMeshComponent* TireMesh)
{
	UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), TireDeflateEvent, FTransform(TireMesh->GetComponentLocation()), true);

	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (FlatTireMesh && GameInstance)
	{
		FStreamableManager& StreamableManager = GameInstance->StreamableManager;
		StreamableManager.RequestAsyncLoad(FlatTireMesh, [&, TireMesh]()
			{
				// TODO(killo): Does UE handle not calling this lambda if this actor is gone in a couple frames?
				if (IsValid(this) && TireMesh)
				{
					TireMesh->SetStaticMesh(FlatTireMesh);
				}
			});
	}
}

void ADestructibleVehicle::Multicast_PlayAudioEvent_Implementation(UFMODEvent* Event, FVector_NetQuantize Location)
{
	UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), Event, FTransform(Location), true);
}
