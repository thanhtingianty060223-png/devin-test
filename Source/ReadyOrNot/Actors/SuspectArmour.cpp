// Copyright Void Interactive, 2022

#include "SuspectArmour.h"

static TAutoConsoleVariable<int32> CVarDrawSuspectArmourDebug(TEXT("a.RonDrawSuspectArmourDebug"), 0, TEXT("Show suspect armour on-hit debug info"));

FSuspectArmourData::FSuspectArmourData()
{
	BlueprintClass = ASuspectArmour::StaticClass();
}

ASuspectArmour::ASuspectArmour()
{
}

void ASuspectArmour::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASuspectArmour, ArmourData);
}

bool ASuspectArmour::HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser)
{
	ACyberneticCharacter* Character = Cast<ACyberneticCharacter>(GetOwnerCharacter());
	if (!Character)
		return false;

	ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser);
	if (!Weapon)
		return false;

	const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
	if (!AmmoType)
		return false;

	bool bShotBlocked = ShotBlocked(AmmoType);

	if (bShotBlocked)
	{
		Damage *= ArmourData.DamageMultiplier;
	}

	Durability = FMath::Max(Durability - AmmoType->DurabilityDamage, 0.0f);

	bool bAllowHitReactions = true;
	if (Character->Archetype)
		bAllowHitReactions = !Character->Archetype->bIgnoreDamageHitReactions;

	if (bAllowHitReactions)
	{
		// Armoured suspects aren't impervious to the force that some ammo imparts on them
		if (UKismetMathLibrary::RandomBoolWithWeight(AmmoType->ArmouredHitsChance))
		{
			Character->PlayMontageFromTable("tp_hits_armoured");
		}
	}

	// Hitsound
	UFMODEvent* Event = bShotBlocked ? BlockedSoundEvent : PenetratedSoundEvent;

	FTransform EventTransform;
	EventTransform.SetLocation(DamageEvent.HitInfo.Location);

	if (Event)
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), Event, EventTransform, true);

	if (bShotBlocked)
	{
		Character->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_IMMUNE);
	}

#if !UE_BUILD_SHIPPING
	if (CVarDrawSuspectArmourDebug.GetValueOnGameThread() != 0)
	{
		FString BlockedText = bShotBlocked ? TEXT("Blocked") : TEXT("Penetrated");
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Orange, FString::Printf(TEXT("%s! Shot by: %s, Remaining Durability: %d"),
			*BlockedText, *Weapon->GetCurrentAmmoTypeRowName().ToString(), Durability));
	}
#endif

	return bShotBlocked;
}

bool ASuspectArmour::CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance)
{
	if (OutSpallingChance)
		*OutSpallingChance = GetSpallingChance();

	return !ShotBlocked(AmmoType);
}

bool ASuspectArmour::ShotBlocked(const FAmmoTypeData* AmmoType) const
{
	if (!AmmoType)
		return false;

	if (AmmoType->bIgnoresArmour)
		return false;

	if (Durability <= 0.0f)
		return false;

	if (AmmoType->PenetrationLevel >= ArmourData.ArmourLevel)
		return false;

	return true;
}

void ASuspectArmour::SetArmourData(const FSuspectArmourData& Data)
{
	ArmourData = Data;
	OnRep_ArmourData();
}

void ASuspectArmour::OnRep_ArmourData()
{
	Durability = ArmourData.Durability;

	if (ArmourData.Mesh)
	{
		// TODO(killo): we are pretending armour is an attachment? probably rename time
		Rep_CustomItemMeshFromAttachment = ArmourData.Mesh;
		ItemMesh->SetSkeletalMesh(ArmourData.Mesh);
	}

	if (ArmourData.HitParticleEffect)
		ArmourHitParticle = ArmourData.HitParticleEffect;

	if (ArmourData.BlockedSoundEvent)
		BlockedSoundEvent = ArmourData.BlockedSoundEvent;

	if (ArmourData.PenetratedSoundEvent)
		PenetratedSoundEvent = ArmourData.PenetratedSoundEvent;
}
