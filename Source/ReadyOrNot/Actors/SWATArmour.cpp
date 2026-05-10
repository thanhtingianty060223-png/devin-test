// Copyright Void Interactive, 2024

#include "SWATArmour.h"

#include "Commander/RosterManager.h"

static TAutoConsoleVariable<int32> CVarDrawSWATArmourDebug(TEXT("a.RonDrawSWATArmourDebug"), 0, TEXT("Show SWAT armour on-hit debug info"));

ASWATArmour::ASWATArmour()
{
	Durabilities.SetNumZeroed(NumPlates);
}

void ASWATArmour::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWATArmour, ArmourCoverage);
	DOREPLIFETIME(ASWATArmour, ArmourMaterial);
	DOREPLIFETIME(ASWATArmour, Durabilities)
}

void ASWATArmour::GetDurabilityAndMaterial(const FHitResult& HitResult, float** OutDurability,
	const UArmourMaterial** OutMaterial)
{
	if (Durabilities.Num() != NumPlates)
	{
		ensureMsgf(false, TEXT("Plate durability array should always match the set number of plates"));
		return;
	}

	AReadyOrNotCharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->GetController())
		return;
	
	/*
	 *	Determine hit direction
	 */
	FVector PlayerDirection = GetOwnerCharacter()->GetControlRotation().Vector();
	FVector PlayerDirectionXY = FVector::VectorPlaneProject(PlayerDirection, FVector::UpVector);
	PlayerDirectionXY.Normalize();
	
	FVector ShotDirection = HitResult.TraceStart - HitResult.Location;
	FVector ShotDirectionXY = FVector::VectorPlaneProject(ShotDirection, FVector::UpVector);
	ShotDirectionXY.Normalize();
	
	float ShotDot = FMath::Clamp(FVector::DotProduct(PlayerDirectionXY, ShotDirectionXY), -1.0f, 1.0f);
	float ShotAngle = FMath::RadiansToDegrees(FMath::Acos(ShotDot));

	float HitAzimuth = FMath::FindDeltaAngleDegrees(ShotAngle, 0.0f);
	HitAzimuth = FMath::Abs(HitAzimuth);
	
	/*
	 *	Determine armor plate and material
	 */
	constexpr float SidesAngle = 30.0f;
	constexpr float FrontThreshold = 90 - SidesAngle;
	constexpr float BackThreshold = 90 + SidesAngle;
	
	// If we don't have coverage in an area, substitute the carrier's material, but only if the carrier also covers that area
	int32 PlateIndex = 0;
	const UArmourMaterial* Material = nullptr;
	if (HitAzimuth <= FrontThreshold && MaxArmourCoverage >= EArmourCoverage::AC_FrontOnly)
	{
		PlateIndex = 0; // Front
		Material = ArmourCoverage >= EArmourCoverage::AC_FrontOnly ? ArmourMaterial : CarrierMaterial;
	}
	else if (HitAzimuth >= BackThreshold && MaxArmourCoverage >= EArmourCoverage::AC_FrontBack)
	{
		PlateIndex = 1; // Back
		Material = ArmourCoverage >= EArmourCoverage::AC_FrontBack ? ArmourMaterial : CarrierMaterial;
	}
	else if (MaxArmourCoverage >= EArmourCoverage::AC_FrontBackSides)
	{
		PlateIndex = 2; // Sides
		Material = ArmourCoverage >= EArmourCoverage::AC_FrontBackSides ? ArmourMaterial : CarrierMaterial;
	}
	
	if (OutDurability)
		*OutDurability = &Durabilities[PlateIndex];
	if (OutMaterial)
		*OutMaterial = Material;
}

bool ASWATArmour::HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser)
{
	if (Durabilities.Num() != NumPlates)
	{
		ensureMsgf(false, TEXT("Plate durability array should always match the set number of plates"));
		return false;
	}

	ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser);
	if (!Weapon)
		return false;

	const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
	if (AmmoType && AmmoType->bIgnoresArmour)
		return false;

	float* Durability = nullptr;
	const UArmourMaterial* Material = nullptr;
	GetDurabilityAndMaterial(DamageEvent.HitInfo, &Durability, &Material);

	if (!Durability || !Material)
		return false;
	
	float DurabilityReduction = AmmoType ? AmmoType->DurabilityDamage : 1.0f;
	DurabilityReduction *= CalculateDurabilityDamageFactor(Damage, AmmoType);
	
	*Durability = FMath::Max(*Durability - DurabilityReduction, 0.0f);

	float DamageReduction = 0.0f;
	bool bHasRemainingDurability = *Durability > 0.0f || !Material->bDurabilityEnabled;
	bool bAmmoPenetrated = AmmoType && AmmoType->PenetrationLevel >= Material->ArmourLevel;
	if (bHasRemainingDurability && !bAmmoPenetrated)
	{
		DamageReduction = Material->DamageReduction;
	}
	else
	{
		DamageReduction = Material->MinDamageReduction;
	}

	// When the armorer trait is enabled, damage reduction is increased by a set amount
	float ArmorerTrait = URosterManager::GetSquadTraitValue("Armorer", GetWorld());
	if (ArmorerTrait > 0.5f)
	{
		DamageReduction += 0.05f;
	}
	
	float DamageMultiplier = FMath::Clamp(1.0f - DamageReduction, 0.0f, 1.0f);
	Damage *= DamageMultiplier;

	// HACK(killo): We set the armour hit particle based on the last hit material
	// Externally this should be called before spawning the impact effects
	ArmourHitParticle = Material->HitParticle;

#if !UE_BUILD_SHIPPING
	if (CVarDrawSWATArmourDebug.GetValueOnGameThread() != 0)
	{
		FString BlockedText = DamageReduction > 0.0f ? TEXT("Blocked") : TEXT("Penetrated");
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("%s! Shot by: %s, Damage Multiplier: %f, Remaining Durability: %f"), 
			*BlockedText, *Weapon->GetCurrentAmmoTypeRowName().ToString(), DamageMultiplier, *Durability));
	}
#endif

	return true;
}

bool ASWATArmour::CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance)
{
	if (OutSpallingChance)
		*OutSpallingChance = 0.0f;
	
	if (!AmmoType)
		return true;
	
	float* Durability = nullptr;
	const UArmourMaterial* Material = nullptr;

	GetDurabilityAndMaterial(HitResult, &Durability, &Material);

	if (Material)
		*OutSpallingChance = Material->SpallingChance;
	
	if (!Durability || !Material)
		return true;
	
	return *Durability <= 0.0f && Material->ArmourLevel <= AmmoType->PenetrationLevel && !AmmoType->bIgnoresArmour;
}

void ASWATArmour::SetArmourCoverage(EArmourCoverage Coverage)
{
	// When we change the armour coverage, we must also reset the plate durabilities by resetting the material
	ArmourCoverage = FMath::Min(Coverage, MaxArmourCoverage);
	SetArmourMaterial(ArmourMaterial);
}

void ASWATArmour::SetArmourMaterial(const UArmourMaterial* Material)
{
	// If no material is provided, just use the carrier's material
	ArmourMaterial = Material ? Material : CarrierMaterial;

	float CarrierDurability = CarrierMaterial ? CarrierMaterial->Durability : 0.0f;
	float PlateDurability = ArmourMaterial ? ArmourMaterial->Durability : 0.0f;

	// When the armorer trait is enabled, flat percentage increase in durability
	float ArmorerTrait = URosterManager::GetSquadTraitValue("Armorer", GetWorld());
	if (ArmorerTrait > 0.5f)
	{
		CarrierDurability += CarrierDurability * 0.5f;
		PlateDurability += PlateDurability * 0.5f;
	}
	
	// We use the carrier's material only if we hit outside of the plate coverage area
	Durabilities[0] = ArmourCoverage >= EArmourCoverage::AC_FrontOnly ? PlateDurability : CarrierDurability;
	Durabilities[1] = ArmourCoverage >= EArmourCoverage::AC_FrontBack ? PlateDurability : CarrierDurability;
	Durabilities[2] = ArmourCoverage >= EArmourCoverage::AC_FrontBackSides ? PlateDurability : CarrierDurability;
	
	if (!ArmourMaterial)
		return;
}

float ASWATArmour::GetArmourSpeedMultiplier() const
{
	if (!ArmourMaterial)
		return 1.0f;

	float Multiplier = static_cast<uint8>(ArmourCoverage) * ArmourMaterial->MovementSpeedModifier;
	return FMath::Clamp(1.0f + Multiplier, 0.0f, 1.0f);
}

float ASWATArmour::GetArmourAccelerationMultiplier() const
{
	if (!ArmourMaterial)
		return 1.0f;

	float Multiplier = static_cast<uint8>(ArmourCoverage) * ArmourMaterial->MovementAccelerationModifier;
	return FMath::Clamp(1.0f + Multiplier, 0.0f, 1.0f);
}

bool ASWATArmour::HasRemainingProtection() const
{
	if (ArmourMaterial && !ArmourMaterial->bDurabilityEnabled)
		return true;

	for (int32 i = 0; i < Durabilities.Num(); i++)
	{
		if (Durabilities[i] > 0.0f)
			return true;
	}
	return false;
}

float ASWATArmour::GetDurabilityPercentage() const
{
	const UArmourMaterial* Material = ArmourCoverage >= EArmourCoverage::AC_FrontOnly ? ArmourMaterial : CarrierMaterial;
	Material = Material ? Material : CarrierMaterial;

	// TODO(killo): Probably change this to a durability sum instead
	float Percentage = 0.0f;
	if (Material)
		Percentage = Durabilities[0] / ArmourMaterial->Durability;

	return FMath::Clamp(Percentage, 0.0f, 1.0f);
}