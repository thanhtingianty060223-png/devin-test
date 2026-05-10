#include "SniperCharacter.h"
#include "ReadyOrNot.h"
#include "Actors/BaseWeapon.h"
#include "Components/InventoryComponent.h"

void ASniperCharacter::SetupPlayerInputComponent(UInputComponent* inputComponent)
{
	Super::SetupPlayerInputComponent(inputComponent);
}

void ASniperCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Attach primary stuff to primary and secondary stuff to secondary
	ABaseMagazineWeapon* PrimaryGun = Cast<ABaseMagazineWeapon>(GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Primary));
	ABaseMagazineWeapon* SecondaryGun = Cast<ABaseMagazineWeapon>(GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Secondary));

	if (PrimaryGun)
	{
		PrimaryGun->AddAttachment(PrimaryScopeAttachment);
		PrimaryGun->AddAttachment(PrimaryMuzzleAttachment);
		PrimaryGun->AddAttachment(PrimaryUnderbarrelAttachment);
	}

	if (SecondaryGun)
	{
		SecondaryGun->AddAttachment(SecondaryScopeAttachment);
		SecondaryGun->AddAttachment(SecondaryMuzzleAttachment);
		SecondaryGun->AddAttachment(SecondaryUnderbarrelAttachment);
	}
}

void ASniperCharacter::FireSelect()
{
	FadeToBlackEnable();
	GetWorld()->GetTimerManager().SetTimer(ExitControlHandle, this, &ASniperCharacter::ExitControl, 0.35f, false);
}

void ASniperCharacter::PrimaryUse()
{
	Super::PrimaryUse();
}

void ASniperCharacter::SecondaryUse()
{
	if (bADSLocked)
	{
		return;
	}

	Super::SecondaryUse();
}

void ASniperCharacter::EndSecondaryUse()
{
	if (bADSLocked)
	{
		return;
	}

	Super::EndSecondaryUse();
}

void ASniperCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (bADSLocked)
	{
		ABaseWeapon* Weapon = Cast<ABaseWeapon>(GetEquippedItem());
		if (Weapon)
		{
			//Weapon->StartScopeMask();
		}
	}

	FadeToBlackDisable();
}

void ASniperCharacter::ExitControl()
{
	if (bADSLocked)
	{
		ABaseWeapon* Weapon = Cast<ABaseWeapon>(GetEquippedItem());
		if (Weapon)
		{
			//Weapon->StopScopeMask();
		}
	}

	if (Controller && PreviousPosessedCharacter)
	{
		Controller->Possess(PreviousPosessedCharacter);
	}
}

/*
bool ASniperCharacter::CanControlWithTablet_Implementation(class APlayerCharacter* TabletOwner)
{
	return true;
}

bool ASniperCharacter::CanTabletViewMe_Implementation(APlayerCharacter* TabletOwner, AReadyOrNotGameState* GameState)
{
	return true;
}*/
