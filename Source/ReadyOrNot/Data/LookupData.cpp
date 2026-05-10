// Copyright Void Interactive, 2021

#include "LookupData.h"
#include "ReadyOrNot.h"
#include "Actors/BaseGrenade.h"

#include "Actors/Projectile.h"

#if WITH_EDITOR
void ULookupData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!PropertyChangedEvent.Property)
		return;

	FName PropName = PropertyChangedEvent.Property->GetFName();
	if (PropName.IsEqual("bToggleToDoAssetUpdate"))
	{
		ForceUpdate();
	}
}

#endif

void ULookupData::EmptyData()
{

	Items.Empty();
	Weapons.Empty();
	Armour.Empty();
	Grenades.Empty();
	Shells.Empty();
	Projectiles.Empty();
	Characters.Empty();
	AI.Empty();
}

void ULookupData::ForceUpdate()
{
	Items.Empty();
	Weapons.Empty();
	Armour.Empty();
	Grenades.Empty();
	Shells.Empty();
	Projectiles.Empty();
	Characters.Empty();
	AI.Empty();

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* ItrClass = *It;
		if (!ItrClass)
			continue;

		if (It->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && !It->HasAnyClassFlags(CLASS_Transient))
		{
			if (It->GetPathName().Contains("skel"))
			{
				continue;
			}
			if (It->IsChildOf(ABaseItem::StaticClass()))
			{
				Items.AddUnique(*It);
			}
			if (It->IsChildOf(ABaseArmour::StaticClass()))
			{
				Armour.AddUnique(*It);
			}
			if (It->IsChildOf(ABaseWeapon::StaticClass()))
			{
				Weapons.AddUnique(*It);
			}
			if (It->IsChildOf(ABaseGrenade::StaticClass()))
			{
				Grenades.AddUnique(*It);
			}
			if (It->IsChildOf(ABaseShell::StaticClass()))
			{
				Shells.AddUnique(*It);
			}
			if (It->IsChildOf(AProjectile::StaticClass()))
			{
				Projectiles.AddUnique(*It);
			}
			if (It->IsChildOf(APlayerCharacter::StaticClass()))
			{
				Characters.AddUnique(*It);
			}
			if (It->IsChildOf(ACyberneticCharacter::StaticClass()))
			{
				AI.AddUnique(*It);
			}
		}

	}
}
