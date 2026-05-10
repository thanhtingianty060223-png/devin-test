// Copyright Void Interactive, 2023

#include "ThrownChemlight.h"

AThrownChemlight::AThrownChemlight()
{
	LightSource = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightSource"));
	LightSource->SetupAttachment(StaticMesh);
	LightSource->SetIntensity(0.0f);
	LightSource->SetLightColor(ChemlightColor);
	
	StaticMesh->SetCollisionProfileName("PhysicsItem");

	bReplicates = false;
}

void AThrownChemlight::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Setup the dynamic material instance if we haven't already
	if (!ChemlightMaterialInstance)
	{
		ChemlightMaterialInstance = StaticMesh->CreateAndSetMaterialInstanceDynamic(0);
		ChemlightMaterialInstance->SetVectorParameterValue("EmissiveColor", ChemlightColor);
	}

	// Position the light above the chemlight mesh with correct offsets
	FVector LightPosition = StaticMesh->GetComponentLocation() + StaticMesh->GetUpVector() * LightZOffset + FVector(0, 0, LightAdditionalHeight);
	LightSource->SetWorldLocation(LightPosition);
	
	CurrentLifeTime += DeltaTime;
	
	if (CurrentLifeTime > TotalLifeTime)
	{
		// We've started to fade
		Destroy();
		return;
	}
	
	if (bChemlightDestroyed)
	{
		// Fade out pointlight
		LightSource->SetIntensity(FMath::FInterpTo(LightSource->Intensity, 0.0f, DeltaTime, DestroyedDimSpeed));
		
		if (LightSource->Intensity <= 0.0f)
			Destroy();
		
		return;
	}

	bool bBrightnessChanged = false;
	if (CurrentLifeTime > StartDimTime)
	{
		// Light is dying
		CurrentStrength = FMath::FInterpTo(CurrentStrength, 0.0f, DeltaTime, LightDimSpeed);
		bBrightnessChanged = true;
	}
	else if (CurrentStrength < 1.0f)
	{
		// Initial brightness ramp
		CurrentStrength = FMath::FInterpTo(CurrentStrength, 1.0f, DeltaTime, InitialGlowSpeed);
		bBrightnessChanged = true;
	}
	
	// Only actually alter the material instance if the brightness changed this frame, otherwise it can potentially get expensive.
	if (bBrightnessChanged)
	{
		ChemlightMaterialInstance->SetScalarParameterValue("EmissiveBrightness", CurrentStrength * EmissiveBrightness);
		LightSource->SetIntensity(CurrentStrength * LightIntensity);
	}
}

float AThrownChemlight::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (Damage > 5.0f)
	{
		FVector TraceStart = LightSource->GetComponentLocation();
		FVector TraceEnd = TraceStart + FVector::DownVector * 10.0f;
		
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

		FHitResult OutHit;
		GetWorld()->LineTraceSingleByObjectType(OutHit, TraceStart, TraceEnd, ObjectQueryParams);

		FRotator Rotation = (-OutHit.ImpactNormal).Rotation();
		Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

		FVector DecalSize = FVector(12.0f, DestroyedDecalSize, DestroyedDecalSize);
	
		UGameplayStatics::SpawnDecalAttached(DestroyedDecal, DecalSize, OutHit.GetComponent(), OutHit.BoneName,
			OutHit.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition);

		bChemlightDestroyed = true;
		StaticMesh->SetStaticMesh(nullptr);
	}

	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}