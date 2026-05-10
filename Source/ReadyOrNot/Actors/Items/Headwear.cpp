// Copyright Void Interactive, 2024

#include "Headwear.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Commander/RosterManager.h"

AHeadwear::AHeadwear()
{
	GetItemMesh()->SetOwnerNoSee(true);

	if (VoiceLineEventOverrideSpatalized == nullptr)
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> TPEvent(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_Mask_3P.Vl_Mask_3P'"));

		if (TPEvent.Object != nullptr)	
			VoiceLineEventOverrideSpatalized = TPEvent.Object;
	}

	if (VoiceLineEventOverrideLocal == nullptr)
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FPEvent(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_Mask_1P.Vl_Mask_1P'"));

		if (FPEvent.Object != nullptr)
			VoiceLineEventOverrideLocal = FPEvent.Object;
	}
}

void AHeadwear::BeginPlay()
{
	Super::BeginPlay();

	// When the armorer trait is enabled, flat percentage increase in durability
	float ArmorerTrait = URosterManager::GetSquadTraitValue("Armorer", GetWorld());
	if (ArmorerTrait > 0.5f)
	{
		Durability += Durability * 0.5f;
	}
}

void AHeadwear::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeadwear, Durability);
}

void AHeadwear::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	DestroyMaskOverlay();
}

bool AHeadwear::HandleDamage(float& Damage, FPointDamageEvent const& DamageEvent, AActor* DamageCauser)
{
	ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser);
	if (!Weapon)
		return false;

	const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
	if (AmmoType && AmmoType->bIgnoresArmour)
		return false;

	bool bRicochet = FMath::FRand() <= RicochetChance;
	bool bReducedDamage = false;

	float DurabilityDamage = AmmoType ? AmmoType->DurabilityDamage : 1.0f;
	DurabilityDamage *= CalculateDurabilityDamageFactor(Damage, AmmoType);
	
	if (bRicochet)
	{
		Damage *= 0.0f;
		bReducedDamage = true;
	}
	else if (Durability > 0.0f)
	{
		Damage *= FMath::Clamp(1.0f - DamageReduction, 0.0f, 1.0f);
		Durability = FMath::Max(Durability - DurabilityDamage, 0.0f);

		bReducedDamage = true;
	}

	if (bRicochet && RicochetEvent)
	{
		FTransform Transform = FTransform(DamageEvent.HitInfo.Location);
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), RicochetEvent, Transform, true);
	}

	return bReducedDamage;
}

bool AHeadwear::CheckPenetration(const FHitResult& HitResult, const FAmmoTypeData* AmmoType, float* OutSpallingChance)
{
	if (OutSpallingChance)
		*OutSpallingChance = 0.0f;

	if (!AmmoType)
		return true;

	if (AmmoType->bIgnoresArmour)
		return true;

	return Durability <= 0.0f;
}

void AHeadwear::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetOwner())
	{
		DestroyMaskOverlay();
	}

	if (GetOwnerPlayerCharacter())
	{
		// You might be asking, why do we do this in Tick and not BeginPlay?
		// Well, UE4 hasn't resolved who the local client is in BeginPlay until AFTER the next tick.
		// So in order to get around that, we resolve all of that here...
		if (!bSpawnedOverlay)
		{
			if (GetOwnerPlayerCharacter()->IsLocalPlayer())
			{
				SpawnMaskOverlay(Cast<APlayerController>(GetOwnerPlayerCharacter()->GetController()));
			}
		}

		if (bEnablePostProcess)
		{
			GetOwnerPlayerCharacter()->GetFirstPersonCameraComponent()->PostProcessSettings = MaskPostProcess;
		}
	}
}

void AHeadwear::IsOverridingBreathingSounds(bool& bIsOverridingBreathingSounds, TArray<USoundCue*>& BreathingSounds, float BreathGap)
{
	BreathingSounds = OverriddenBreathingSounds;
	bIsOverridingBreathingSounds = bOverrideBreathingSound;
	
	GapBetweenBreaths = BreathGap;
}

bool AHeadwear::IsUsingPostProcess()
{
	return bEnablePostProcess;
}

FPostProcessSettings AHeadwear::GetMaskSupressionPostProcess()
{
	return MaskSupressionPostProcess;
}

void AHeadwear::SpawnMaskOverlay(APlayerController* OwningController)
{
	DestroyMaskOverlay();

	if (MaskOverlay)
	{
		const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("CharacterHUD_V2");

		SpawnedMaskOverlay = CreateWidget<UUserWidget>(OwningController, MaskOverlay);
		if (SpawnedMaskOverlay)
		{
			SpawnedMaskOverlay->AddToViewport(WidgetData.ZOrder - 1); // Always below HUD
			bSpawnedOverlay = true;
		}
	}
}

void AHeadwear::DestroyMaskOverlay()
{
	if (!SpawnedMaskOverlay)
		return;

	SpawnedMaskOverlay->RemoveFromParent();
	SpawnedMaskOverlay = nullptr;
}

float AHeadwear::GetWeight()
{
	return Super::Super::GetWeight();
}
