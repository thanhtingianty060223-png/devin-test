// Copyright Void Interactive, 2021

#include "PlayerEffect_ModifyRecoil.h"

#include "Characters/PlayerCharacter.h"

#if !UE_BUILD_SHIPPING
#include "Log.h"
#endif

UPlayerEffect_ModifyRecoil::UPlayerEffect_ModifyRecoil(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeaponFilter.Add(EItemClass::IC_AssaultRifle);
	WeaponFilter.Add(EItemClass::IC_SMG);
	WeaponFilter.Add(EItemClass::IC_LMG);
	WeaponFilter.Add(EItemClass::IC_Pistol);
	WeaponFilter.Add(EItemClass::IC_Shotgun);
}

void UPlayerEffect_ModifyRecoil::Initialize_Implementation(AActor* InActor)
{
	Super::Initialize_Implementation(InActor);

	// Get all magazine weapons in our inventory and filter them out
	TArray<ABaseItem*> InventoryItems = PlayerCharacter->GetInventoryComponent()->GetInventoryItems();
	AffectedWeapons.Reserve(InventoryItems.Num());
	
	for (ABaseItem* Item : InventoryItems)
	{
		if (Item && Item->IsA(ABaseMagazineWeapon::StaticClass()) && ShouldAffectWeapon(Item))
		{
			AffectedWeapons.Add(Cast<ABaseMagazineWeapon>(Item));
		}
	}

	// Store each weapon's original recoil values
	for (ABaseMagazineWeapon* AffectedWeapon : AffectedWeapons)
	{
		if (!AffectedWeapon)
			continue;
		FSpecificWeaponRecoilMod OriginalWeaponRecoilSettings;
		OriginalWeaponRecoilSettings.RecoilFireStrength = AffectedWeapon->RecoilFireStrength;
		OriginalWeaponRecoilSettings.RecoilFireStrengthFirst = AffectedWeapon->RecoilFireStrengthFirst;
		OriginalWeaponRecoilSettings.RecoilDampStrength = AffectedWeapon->RecoilDampStrength;
		OriginalWeaponRecoilSettings.RecoilAngleStrength = AffectedWeapon->RecoilAngleStrength;
		OriginalWeaponRecoilSettings.RecoilRandomness = AffectedWeapon->RecoilRandomness;
		OriginalWeaponRecoilSettings.RecoilFireADSModifier = AffectedWeapon->RecoilFireADSModifier;
		OriginalWeaponRecoilSettings.RecoilBuildupADSModifier = AffectedWeapon->RecoilBuildupADSModifier;
		OriginalWeaponRecoilSettings.RecoilAngleADSModifier = AffectedWeapon->RecoilAngleADSModifier;
		OriginalWeaponRecoilSettings.RecoilBuildupDampStrength = AffectedWeapon->RecoilBuildupDampStrength;
			
		OriginalRecoilValues.Add(AffectedWeapon, OriginalWeaponRecoilSettings);
	}
}

void UPlayerEffect_ModifyRecoil::ApplyEffect_Implementation()
{
	for (ABaseMagazineWeapon* AffectedWeapon : AffectedWeapons)
	{
		if (!AffectedWeapon)
			continue;
	
		#if !UE_BUILD_SHIPPING
		ULog::Success("Applying recoil modification to " + AffectedWeapon->GetName() + ". Owner is " + PlayerCharacter->GetName(), LO_Both);
		#endif

		ApplyRecoil(AffectedWeapon);
	}
}

void UPlayerEffect_ModifyRecoil::ResetEffect_Implementation()
{
	for (ABaseMagazineWeapon* AffectedWeapon : AffectedWeapons)
	{
		if (!AffectedWeapon)
			continue;
		
		#if !UE_BUILD_SHIPPING
		ULog::Success("Resetting recoil modification to " + AffectedWeapon->GetName() + ". Owner is " + PlayerCharacter->GetName(), LO_Both);
		#endif

		const FSpecificWeaponRecoilMod& OriginalRecoilValueSet = OriginalRecoilValues[AffectedWeapon];

		AffectedWeapon->RecoilFireStrength = OriginalRecoilValueSet.RecoilFireStrength;
		AffectedWeapon->RecoilFireStrengthFirst = OriginalRecoilValueSet.RecoilFireStrengthFirst;
		AffectedWeapon->RecoilDampStrength = OriginalRecoilValueSet.RecoilDampStrength;
		AffectedWeapon->RecoilAngleStrength = OriginalRecoilValueSet.RecoilAngleStrength;
		AffectedWeapon->RecoilRandomness = OriginalRecoilValueSet.RecoilRandomness;
		AffectedWeapon->RecoilFireADSModifier = OriginalRecoilValueSet.RecoilFireADSModifier;
		AffectedWeapon->RecoilBuildupADSModifier = OriginalRecoilValueSet.RecoilBuildupADSModifier;
		AffectedWeapon->RecoilAngleADSModifier = OriginalRecoilValueSet.RecoilAngleADSModifier;
		AffectedWeapon->RecoilBuildupDampStrength = OriginalRecoilValueSet.RecoilBuildupDampStrength;
	}
}

void UPlayerEffect_ModifyRecoil::ApplyRecoil(ABaseMagazineWeapon* AffectedWeapon)
{
	const auto ModifyRecoil_Default = [&]()
	{
		if (bApplySpecific)
		{
			for (const FSpecificWeaponRecoilMod& Entry : SpecificWeaponRecoilMods)
			{
				if (AffectedWeapon->ItemClass == Entry.WeaponClass)
				{
					AffectedWeapon->RecoilFireStrength = Entry.RecoilFireStrength;
					AffectedWeapon->RecoilFireStrengthFirst = Entry.RecoilFireStrengthFirst;
					AffectedWeapon->RecoilDampStrength = Entry.RecoilDampStrength;
					AffectedWeapon->RecoilAngleStrength = Entry.RecoilAngleStrength;
					AffectedWeapon->RecoilRandomness = Entry.RecoilRandomness;
					AffectedWeapon->RecoilFireADSModifier = Entry.RecoilFireADSModifier;
					AffectedWeapon->RecoilBuildupADSModifier = Entry.RecoilBuildupADSModifier;
					AffectedWeapon->RecoilAngleADSModifier = Entry.RecoilAngleADSModifier;
					AffectedWeapon->RecoilBuildupDampStrength = Entry.RecoilBuildupDampStrength;
					break;
				}
			}

			return;
		}
			
		AffectedWeapon->RecoilFireStrength = RecoilFireStrength;
		AffectedWeapon->RecoilFireStrengthFirst = RecoilFireStrengthFirst;
		AffectedWeapon->RecoilDampStrength = RecoilDampStrength;
		AffectedWeapon->RecoilAngleStrength = RecoilAngleStrength;
		AffectedWeapon->RecoilRandomness = RecoilRandomness;
		AffectedWeapon->RecoilFireADSModifier = RecoilFireADSModifier;
		AffectedWeapon->RecoilBuildupADSModifier = RecoilBuildupADSModifier;
		AffectedWeapon->RecoilAngleADSModifier = RecoilAngleADSModifier;
		AffectedWeapon->RecoilBuildupDampStrength = RecoilBuildupDampStrength;
	};

	switch (ModificationOption)
	{
		case ERecoilModifierOption::RMO_ModifyRecoil:
			ModifyRecoil_Default();
		break;

		case ERecoilModifierOption::RMO_AddRecoil:
			if (bApplySpecific)
			{
				for (const FSpecificWeaponRecoilMod& Entry : SpecificWeaponRecoilMods)
				{
					if (AffectedWeapon->ItemClass == Entry.WeaponClass)
					{
						AffectedWeapon->RecoilFireStrength += Entry.RecoilFireStrength;
						AffectedWeapon->RecoilFireStrengthFirst += Entry.RecoilFireStrengthFirst;
						AffectedWeapon->RecoilDampStrength += Entry.RecoilDampStrength;
						AffectedWeapon->RecoilAngleStrength += Entry.RecoilAngleStrength;
						AffectedWeapon->RecoilRandomness += Entry.RecoilRandomness;
						AffectedWeapon->RecoilFireADSModifier += Entry.RecoilFireADSModifier;
						AffectedWeapon->RecoilBuildupADSModifier += Entry.RecoilBuildupADSModifier;
						AffectedWeapon->RecoilAngleADSModifier += Entry.RecoilAngleADSModifier;
						AffectedWeapon->RecoilBuildupDampStrength += Entry.RecoilBuildupDampStrength;	
						break;
					}
				}

				return;
			}
			
			AffectedWeapon->RecoilFireStrength += RecoilFireStrength;
			AffectedWeapon->RecoilFireStrengthFirst += RecoilFireStrengthFirst;
			AffectedWeapon->RecoilDampStrength += RecoilDampStrength;
			AffectedWeapon->RecoilAngleStrength += RecoilAngleStrength;
			AffectedWeapon->RecoilRandomness += RecoilRandomness;
			AffectedWeapon->RecoilFireADSModifier += RecoilFireADSModifier;
			AffectedWeapon->RecoilBuildupADSModifier += RecoilBuildupADSModifier;
			AffectedWeapon->RecoilAngleADSModifier += RecoilAngleADSModifier;
			AffectedWeapon->RecoilBuildupDampStrength += RecoilBuildupDampStrength;
		break;

		case ERecoilModifierOption::RMO_SubtractRecoil:
			if (bApplySpecific)
			{
				for (const FSpecificWeaponRecoilMod& Entry : SpecificWeaponRecoilMods)
				{
					if (AffectedWeapon->ItemClass == Entry.WeaponClass)
					{
						AffectedWeapon->RecoilFireStrength -= Entry.RecoilFireStrength;
						AffectedWeapon->RecoilFireStrengthFirst -= Entry.RecoilFireStrengthFirst;
						AffectedWeapon->RecoilDampStrength -= Entry.RecoilDampStrength;
						AffectedWeapon->RecoilAngleStrength -= Entry.RecoilAngleStrength;
						AffectedWeapon->RecoilRandomness -= Entry.RecoilRandomness;
						AffectedWeapon->RecoilFireADSModifier -= Entry.RecoilFireADSModifier;
						AffectedWeapon->RecoilBuildupADSModifier -= Entry.RecoilBuildupADSModifier;
						AffectedWeapon->RecoilAngleADSModifier -= Entry.RecoilAngleADSModifier;
						AffectedWeapon->RecoilBuildupDampStrength -= Entry.RecoilBuildupDampStrength;	
						break;
					}
				}

				return;
			}
			
			AffectedWeapon->RecoilFireStrength -= RecoilFireStrength;
			AffectedWeapon->RecoilFireStrengthFirst -= RecoilFireStrengthFirst;
			AffectedWeapon->RecoilDampStrength -= RecoilDampStrength;
			AffectedWeapon->RecoilAngleStrength -= RecoilAngleStrength;
			AffectedWeapon->RecoilRandomness -= RecoilRandomness;
			AffectedWeapon->RecoilFireADSModifier -= RecoilFireADSModifier;
			AffectedWeapon->RecoilBuildupADSModifier -= RecoilBuildupADSModifier;
			AffectedWeapon->RecoilAngleADSModifier -= RecoilAngleADSModifier;
			AffectedWeapon->RecoilBuildupDampStrength -= RecoilBuildupDampStrength;
		break;
			
		default:
			ModifyRecoil_Default();
		break;
	}
}

bool UPlayerEffect_ModifyRecoil::ShouldAffectWeapon(const ABaseItem* Item)
{
	if (!Item)
		return false;

	if (bApplySpecific)
	{
		for (const FSpecificWeaponRecoilMod& Entry : SpecificWeaponRecoilMods)
		{
			if (Item->ItemClass == Entry.WeaponClass)
				return true;
		}

		return false;
	}

	for (const EItemClass Class : WeaponFilter)
	{
		if (Item->ItemClass == Class)
			return true;
	}

	return false;
}
