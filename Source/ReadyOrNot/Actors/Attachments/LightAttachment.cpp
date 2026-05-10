// Copyright Void Interactive, 2022

#include "LightAttachment.h"

#include "Actors/Gameplay/LensFlare.h"
#include "Actors/Gameplay/FlashLightTrackingPoint.h"

#include "Actors/Items/BallisticsShield.h"

void ULightAttachment::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULightAttachment::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWeapon())
	{
		DetachLight();
		return;
	}

	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetWeapon()->GetOwner());
	if (!pc || (pc && (pc->IsDeadOrUnconscious() || pc->IsIncapacitated())))
	{
		if (IsLightOn())
		{
			DetachLight();
		}
		
		return;
	}

	FHitResult Hit;

	if (GetWeapon() && SpotLightComponent)
	{
		bool bBounceLightEnabled;
		UBpGameplayHelperLib::GetBounceLightEnabled(bBounceLightEnabled);
		if (!bBounceLightEnabled && PointLightComponent)
		{
			PointLightComponent->DestroyComponent();
			PointLightComponent = nullptr;
		}
		
		SpotLightComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, "light");
		SpotLightComponent->SetWorldRotation(GetWeapon()->GetBulletSpawn()->GetComponentRotation());
		
		FVector StartTrace = GetSocketLocation("light") + GetWeapon()->GetBulletSpawn()->GetComponentRotation().Vector() * -40.0f;
		FVector EndTrace = StartTrace + GetWeapon()->GetBulletSpawn()->GetComponentRotation().Vector() * SpotLightComponent->AttenuationRadius;
		
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
		//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::White, false, 1.0f, 0 , 1);
		
		FCollisionQueryParams CollisionQueryParams = pc->GetCollisionQueryParameters();
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)FlashLightTrackingPoints);
		
		GetWorld()->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, CollisionObjectQueryParams, CollisionQueryParams);
		
		// hide the light from the tp shadow but if moving up to a wall don't allow it to go int othe shadow
		SpotLightComponent->SetVisibility(IsLightOn());

		if (Hit.Location != FVector::ZeroVector)
		{
			//DrawDebugSphere(GetWorld(), Hit.Location, 10.0f, 4, FColor::Red, false, DeltaTime + 0.005f, 0, 2.0f);
			//DrawDebugString(GetWorld(), Hit.Location, GetNameSafe(Hit.GetActor()), nullptr, FColor::White, DeltaTime + 0.005f, true);
		}

		if (PointLightComponent)
		{
			// NOTE(killo): makes no sense to not calculate this every tick while light is on anyway, player is going to be moving/looking around
			// tried checking for blocking animation on gun, but then kicks also made the bounced light look bad

			FHitResult SphereTest;
			UKismetSystemLibrary::SphereTraceSingleForObjects(GetWorld(), StartTrace, EndTrace, 12.0f, { UEngineTypes::ConvertToObjectType(ECC_WorldStatic),
				UEngineTypes::ConvertToObjectType(ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECC_DOOR), UEngineTypes::ConvertToObjectType(ECC_Pawn) }, true, { pc, GetWeapon() }, EDrawDebugTrace::None, SphereTest, true);

			float PreviousDistance = AvgDistance;
			AvgDistance = SphereTest.Distance;
			if (SphereTest.bBlockingHit)
			{
				FVector GIBounceLoc = SpotLightComponent->GetComponentLocation() + GetWeapon()->GetBulletSpawn()->GetComponentRotation().Vector() * UKismetMathLibrary::WeightedMovingAverage_Float(AvgDistance * 0.5f, PreviousDistance, 0.5f);
				PointLightComponent->SetWorldLocation(GIBounceLoc);
			}
		}

		float FinalIntensity = Intensity;
		float FinalBouncedIntensity = BouncedIntensity;

		// Individual maps can slightly adjust flashlight parameters
		AReadyOrNotLevelScript* LS = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
		if (LS)
		{
			FinalIntensity += LS->FlashlightIntensityBoost;
			FinalBouncedIntensity += LS->FlashlightBouncedIntensityBoost;
		}

		SpotLightComponent->SetIntensity(FinalIntensity);
		//SpotLightComponent->SetRelativeLocation(FVector::ZeroVector);
		if (SpawnedLensFlare)
		{
			FRotator lensFlareRotation = GetWeapon()->GetBulletSpawn()->GetComponentRotation();
			SpawnedLensFlare->SetActorRotation(lensFlareRotation);
		}

		if (PointLightComponent)
		{
			float CloseFactor = FMath::Clamp(AvgDistance / 100.0f, 0.0f, 1.0f); // Not sure if needed, helps speed up intensity falloff at close range
			float FadeInFactor = FMath::Clamp(AvgDistance / 400.0f, 0.0f, 1.0f);
			float FadeOutFactor = 1.0f - FMath::Clamp((AvgDistance - 1000.0f) / (1600.0f - 1000.0f), 0.0f, 1.0f);

			float TargetIntensity = AvgDistance == 0.0f ? 0.0f : FinalBouncedIntensity * 0.00086f * FadeInFactor * FadeOutFactor * CloseFactor * 0.05f;
			float TargetAttenuation = AvgDistance == 0.0f ? 0.0f : 600.0f;

			float CurrentIntensity = FMath::FInterpTo(PointLightComponent->Intensity, TargetIntensity, DeltaTime, 30.0f); 
			float CurrentAttenuation = FMath::FInterpTo(PointLightComponent->AttenuationRadius, TargetAttenuation, DeltaTime, 30.0f);

			PointLightComponent->SetIntensity(CurrentIntensity);
			PointLightComponent->SetAttenuationRadius(CurrentAttenuation);
		}
	}


	for (AFlashLightTrackingPoint* TrackingPoint : FlashLightTrackingPoints)
	{
		if (!TrackingPoint)
			continue;
		
		if (Hit.Location != FVector::ZeroVector)
		{
			if (TrackingPoint->IsActive())
			{
				TrackingPoint->SetActorLocation(Hit.Location + Hit.Normal * 20.0f, false, nullptr, ETeleportType::TeleportPhysics);
				//DrawDebugSphere(GetWorld(), TrackingPoint->GetActorLocation(), 5.0f, 4, FColor::Blue, false, DeltaTime + 0.005f, 0, 1.5f);
			}
			else
			{
				TrackingPoint->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
				TrackingPoint->SetActorRelativeLocation(FVector::ZeroVector);
			}
		}
		else
		{
			TrackingPoint->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
			TrackingPoint->SetActorRelativeLocation(FVector::ZeroVector);
		}
	}
}

void ULightAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ULightAttachment, bRepOn);
}

void ULightAttachment::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	DestroyFlashLightTrackingPoints();
}

void ULightAttachment::OnRep_On()
{
	ToggleLight(bRepOn);
}

void ULightAttachment::ToggleLight(bool bOn)
{
	if (bOn != bRepOn) // new value
	{
		bRepOn = bOn;
		bOn ? AttachLight() : DetachLight();
		PlayToggleSound();
	}
}

bool ULightAttachment::IsLightOn()
{
	return SpotLightComponent ? SpotLightComponent->IsVisible() : false;
}

void ULightAttachment::AttachLight()
{
	DetachLight();

	if (GetWeapon()->GetOwnerCharacter() && GetWeapon()->GetOwnerCharacter()->GetEquippedItem() != GetWeapon())
	{
		bool bEquippedWithShield = false;
		ABallisticsShield* bShield = Cast<ABallisticsShield>(GetWeapon()->GetOwnerCharacter()->GetEquippedItem());
		if (bShield)
		{
			bEquippedWithShield = bShield->PistolEquippedWithShield == GetWeapon();
		}
		if (!bEquippedWithShield)
		{
			return;
		}
	}

	// initialize, if not already
	{
		if (!SpotLightComponent)
		{
			SpotLightComponent = NewObject<USpotLightComponent>(GetWeapon(), USpotLightComponent::StaticClass());
			SpotLightComponent->RegisterComponent();
			SpotLightComponent->SetIntensity(Intensity);
			SpotLightComponent->SetInnerConeAngle(InnerConeAngle);
			SpotLightComponent->SetOuterConeAngle(OuterConeAngle);
			SpotLightComponent->SetLightColor(LightColor);
			SpotLightComponent->SetLightFunctionMaterial(LightFunctionMaterial);
			SpotLightComponent->SetLightFunctionScale(LightFunctionScale);
			SpotLightComponent->SetVolumetricScatteringIntensity(0.0f);
			SpotLightComponent->SetAttenuationRadius(Attenuation);
			SpotLightComponent->SetSpecularScale(0.25f);
			SpotLightComponent->SetAffectTranslucentLighting(false);
			SpotLightComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, "light");
			SpotLightComponent->SetVisibility(false);
			SpotLightComponent->SetHiddenInGame(true);
			SpotLightComponent->SetActive(false);
			SpotLightComponent->SetAutoActivate(false);

			if (!SpawnedLensFlare && LensFlareClass)
			{
				SpawnedLensFlare = GetWorld()->SpawnActor<ALensFlare>(LensFlareClass);
				SpawnedLensFlare->AttachToComponent(SpotLightComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, NAME_None);
				SpawnedLensFlare->SetOwningCharacter(GetWeapon()->GetOwnerCharacter());
				SpawnedLensFlare->SetActorHiddenInGame(true);
			}

			if (FlashLightTrackingPoints.Num() < 1)
				InitialiseFlashLightTrackingPoints();
			else
				AttachTrackingPoint();
		}

		if (GetWeapon() && GetWeapon()->GetOwnerCharacter() && GetWeapon()->GetOwnerCharacter()->IsLocalPlayer())
		{
			if (!PointLightComponent)
			{
				PointLightComponent = NewObject<UPointLightComponent>(GetWeapon(), UPointLightComponent::StaticClass());
				PointLightComponent->RegisterComponent();
				PointLightComponent->SetIntensity(0.0f);
				PointLightComponent->SetLightColor(LightColor);
				PointLightComponent->SetAttenuationRadius(600.0f);
				PointLightComponent->SetCastShadows(false);
				PointLightComponent->SetSourceRadius(600.0f);
				PointLightComponent->SetSoftSourceRadius(600.0f);
				PointLightComponent->SetSpecularScale(0.0f); // Doesn't make sense for bounced light, causes bloom/glare
				PointLightComponent->SetAffectTranslucentLighting(false);
				PointLightComponent->bUseInverseSquaredFalloff = false;
				PointLightComponent->SetLightFalloffExponent(4.0f);
				PointLightComponent->SetHiddenInGame(true);
				PointLightComponent->SetVisibility(false);
				PointLightComponent->SetActive(false);
				PointLightComponent->SetAutoActivate(false);
			}
		}
	}

	// Prestream light function material to avoid ugly streaming when first using flashlight
	if (LightFunctionMaterial)
	{
		LightFunctionMaterial->SetForceMipLevelsToBeResident(true, true, -1.0f);
	}

	if (SpotLightComponent)
	{
		SpotLightComponent->SetActive(true, true);
		SpotLightComponent->SetHiddenInGame(false);
		SpotLightComponent->SetVisibility(true);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetActive(true, true);
		PointLightComponent->SetHiddenInGame(false);
		PointLightComponent->SetVisibility(true);
	}

	if (SpawnedLensFlare)
	{
		SpawnedLensFlare->SetActorHiddenInGame(false);
	}
}

void ULightAttachment::DetachLight()
{
	if (SpotLightComponent)
	{
		SpotLightComponent->SetActive(false);
		SpotLightComponent->SetHiddenInGame(true);
		SpotLightComponent->SetVisibility(false);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetActive(false);
		PointLightComponent->SetHiddenInGame(true);
		PointLightComponent->SetVisibility(false);
	}

	if (SpawnedLensFlare)
	{
		SpawnedLensFlare->SetActorHiddenInGame(true);
	}

	DetachTrackingPoint();
}

void ULightAttachment::InitialiseFlashLightTrackingPoints()
{
	FlashLightTrackingPoints.Init(nullptr, NumOfFlashLightTrackingPoints);

	// Ignore the weapon when flashlight trace
	FlashLightQueryParams.AddIgnoredActor(GetWeapon());

	// Forces the pawn to spawn even if colliding
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.bNoFail = true;
	SpawnInfo.Owner = GetWeapon();
	SpawnInfo.Instigator = GetWeapon()->GetOwnerCharacter();

	if (PrimaryTrackingPoint)
	{
		PrimaryTrackingPoint->Destroy();
		PrimaryTrackingPoint = nullptr;
	}

	PrimaryTrackingPoint = GetWorld()->SpawnActor<AFlashLightTrackingPoint>(AFlashLightTrackingPoint::StaticClass(), GetWeapon()->GetActorLocation(), GetWeapon()->GetActorRotation(), SpawnInfo);
	
	if (PrimaryTrackingPoint)
	{
		PrimaryTrackingPoint->bIsPrimary = true;
		
		PrimaryTrackingPoint->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
		PrimaryTrackingPoint->SetupTrackingPoint();
		PrimaryTrackingPoint->ToggleTrackingPoint(true);

		// Actor for flashlight trace to ignore
		FlashLightQueryParams.AddIgnoredActor(PrimaryTrackingPoint);

		FlashLightTrackingPoints.Add(PrimaryTrackingPoint);
	}
	
	for (int32 i = 0; i < NumOfFlashLightTrackingPoints; i++)
	{
		//FString PointName = ItemName.ToString() + "_Point_" + FString::FromInt(i);
		
		if (AFlashLightTrackingPoint* TrackingPoint = GetWorld()->SpawnActor<AFlashLightTrackingPoint>(
			AFlashLightTrackingPoint::StaticClass(), GetWeapon()->GetActorLocation(), GetWeapon()->GetActorRotation(),
			SpawnInfo))
		{
			FlashLightTrackingPoints[i] = TrackingPoint;
			
			FlashLightTrackingPoints[i]->bIsPrimary = false;
			FlashLightTrackingPoints[i]->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
			FlashLightTrackingPoints[i]->SetupTrackingPoint();
			FlashLightTrackingPoints[i]->ToggleTrackingPoint(true);

			// Actor for flashlight trace to ignore
			FlashLightQueryParams.AddIgnoredActor(FlashLightTrackingPoints[i]);	
		}
	}

	bIsFlashLightTrackable = true;
}

void ULightAttachment::DestroyFlashLightTrackingPoints()
{
	for (auto TrackingPoint : FlashLightTrackingPoints)
	{
		if (TrackingPoint != nullptr)
			TrackingPoint->Destroy();
	}
}

void ULightAttachment::AttachTrackingPoint()
{
	bIsFlashLightTrackable = true;
	for (auto TrackingPoint : FlashLightTrackingPoints)
	{
		if (TrackingPoint != nullptr)
			TrackingPoint->ToggleTrackingPoint(true);
	}
}

void ULightAttachment::DetachTrackingPoint()
{
	bIsFlashLightTrackable = false;
	for (auto TrackingPoint : FlashLightTrackingPoints)
	{
		if (TrackingPoint != nullptr)
			TrackingPoint->ToggleTrackingPoint(false);
	}
}

void ULightAttachment::DestroyComponent(bool bPromoteChildren)
{
	DetachLight();
	
	DestroyFlashLightTrackingPoints();

	DESTROY_COMPONENT(SpotLightComponent);
	DESTROY_COMPONENT(PointLightComponent);

	if (SpawnedLensFlare)
	{
		SpawnedLensFlare->Destroy();
		SpawnedLensFlare = nullptr;
	}
	
	Super::DestroyComponent(bPromoteChildren);
}

FVector ULightAttachment::CalculatePointInLine(FVector Start, FVector End, float Position)
{
	Position = FMath::Clamp(Position, 0.0f, 1.0f);
	
	float DifferenceFromOne = 1 - Position;
	
	FVector OutputVector;
	OutputVector.X = (Start.X * DifferenceFromOne) + (End.X * Position);
	OutputVector.Y = (Start.Y * DifferenceFromOne) + (End.Y * Position);
	OutputVector.Z = (Start.Z * DifferenceFromOne) + (End.Z * Position);

	return OutputVector;
}
