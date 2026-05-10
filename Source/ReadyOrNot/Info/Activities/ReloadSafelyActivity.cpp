// Copyright Void Interactive, 2021

#include "ReloadSafelyActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Components/MoraleComponent.h"

#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/Shotgun.h"

#include "Info/SuspectsAndCivilianManager.h"

#include "NavigationSystem.h"

#include "ReadyOrNotAIConfig.h"

UReloadSafelyActivity::UReloadSafelyActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "Reloading");

	bIsProgressActivity = true;
	bNoMoveActivity = true;
	bAbortMoveWhenActivityFinished = false;
	bAbortMoveWhenActivityOverriden = false;
}

void UReloadSafelyActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	ABaseMagazineWeapon* bmw = GetCharacter()->GetEquippedWeapon();
	
	if (!bmw)
	{
		ACTIVITY_FAILED("No equipped to reload", true);
		return;
	}

	if (bmw->Magazines.Num() == 0)
	{
		ACTIVITY_FAILED("Can't reload. No mags more available", true);
		return;
	}
	
	if (bmw->AmmoMax == 0.0f)
	{
		ACTIVITY_FAILED("Can't reload. No ammo to fill mags", true);
		return;
	}
	
	if (bmw->GetAmmo() >= bmw->AmmoMax)
	{
		ACTIVITY_FAILED("Can't reload. Already full ammo", true);
		return;
	}

	if (bmw->IsPlayingDraw() || bmw->IsPlayingHolster())
	{
		return;
	}

	if (GetCharacter()->IsStunned())
	{
		ACTIVITY_FAILED("Is Stunned", true);
		return;
	}

	// We are reloading.. don't run code below
	if (GetWorld()->GetTimerManager().IsTimerActive(ReloadFinished_Handle))
	{
		ProgressState = FText::FromStringTable("SwatCommandTable", "Reloading"); // For UI
		return;
	}

	if (OwningController->IsSWAT())
	{
		bmw->OnWeaponTacticalReload();
		
		float ReloadTime = 0.01f;
		if (bmw->AnimationData->Tactical_ReloadEmpty.Gun_TP)
		{
			ReloadTime = bmw->AnimationData->Tactical_ReloadEmpty.Gun_TP->GetPlayLength();
		}
		
		GetWorld()->GetTimerManager().SetTimer(ReloadFinished_Handle, this, &UReloadSafelyActivity::ReloadFinished, ReloadTime, false);
		return;
	}

	if (!GetCharacter()->IsAny3PMontageActive())
	{
		// needs to not be 0
		float ReloadTime = 2.0f;

		const float ReloadSlowMorale = AI_CONFIG_GET_FLOAT("SuspectMoraleLowReload");
		const float ReloadMediumMorale = AI_CONFIG_GET_FLOAT("SuspectMoraleMediumReload");
		const float Morale = OwningController->GetMoraleComp()->GetMorale();
		
		FString DesiredReload = "tp_spct_reload";

		bool bUsingTableOverride = false;
		bool bNoMorale = false;

		if (GetCharacter()->IsTakingCover())
		{
			const FString Direction = GetCharacter()->ActiveCoverDirection == ECoverDirection::Left ? "left" : "right";
			DesiredReload += "_cover_" + Direction;
			bUsingTableOverride = false;
			bNoMorale = true; // skip morale check for now, there isnt any cover reload morale anims yet
		}
		else
		{
			if (!bmw->MoraleLowReloadTableOverride.IsEmpty() && Morale < ReloadSlowMorale)
			{
				DesiredReload = bmw->MoraleLowReloadTableOverride;
				bUsingTableOverride = true;
			}
			else if (!bmw->MoraleMediumReloadTableOverride.IsEmpty() && Morale < ReloadMediumMorale)
			{
				DesiredReload = bmw->MoraleMediumReloadTableOverride;
				bUsingTableOverride = true;
			}
			else if (!bmw->MoraleHighReloadTableOverride.IsEmpty())
			{
				DesiredReload = bmw->MoraleHighReloadTableOverride;
				bUsingTableOverride = true;
			}
		}

		if (!bUsingTableOverride)
		{
			if (const AShotgun* Shotgun = Cast<AShotgun>(bmw)/* bmw->ItemClass == EItemClass::IC_Shotgun*/)
			{
				DesiredReload += Shotgun->bIsSawnOff ? "_sawnoff" : "_shotgun";
			}
			else if (bmw->ItemClass == EItemClass::IC_AssaultRifle || bmw->ItemClass == EItemClass::IC_SMG)
			{
				DesiredReload += "_rifle";
			}
			else if (bmw->ItemClass == EItemClass::IC_Pistol)
			{
				DesiredReload += "_pistol";
			}
			else if (bmw->ItemClass == EItemClass::IC_LMG)
			{
				DesiredReload += "_machinegun";
			}

			if (!bNoMorale)
			{
				if (Morale < ReloadSlowMorale)
				{
					DesiredReload += "_moralelow";
				}
				else if (Morale < ReloadMediumMorale)
				{
					DesiredReload += "_moralemedium";
				}
				else
				{
					DesiredReload += "_moralehigh";
				}
			}
		}

		ReloadMontage = GetCharacter()->PlayMontageFromTable(DesiredReload);
		
		#if WITH_EDITOR
		// If none was specified, something went wrong
		ensureAlways(ReloadMontage != nullptr);
		#endif
		
		if (ReloadMontage)
		{
			ReloadTime = ReloadMontage->GetPlayLength() - (ReloadMontage->GetDefaultBlendOutTime() + 0.05f);;
		}

		if (ReloadTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(ReloadFinished_Handle, this, &UReloadSafelyActivity::ReloadFinished, ReloadTime, false);
		}
		else
		{
			ReloadFinished();
		}
	}
	else
	{
		if (GetCharacter()->GetCurrentMontage() != ReloadMontage)
		{
			OwningController->FinishActivity(this, false, true);
		}
	}
}

bool UReloadSafelyActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	FocalPoint = FVector::ZeroVector;
	return true;
}

void UReloadSafelyActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	const ABaseMagazineWeapon* bmw = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();
	if (GetCharacter()->GetEquippedItem<ABallisticsShield>())
	{
		bmw = GetCharacter()->GetEquippedItem<ABallisticsShield>()->PistolEquippedWithShield;
	}

	if (!bmw)
	{
		ACTIVITY_FAILED("No equipped to reload");
		return;
	}

	if (USuspectsAndCivilianManager* SusCivManager = USuspectsAndCivilianManager::Get(this))
		SusCivManager->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::RELOADING, GetCharacter()->GetActorLocation());
}

void UReloadSafelyActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	if (!bSuccess)
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadFinished_Handle);
		
		GetCharacter()->StopTPMontage(ReloadMontage);
	}
}

void UReloadSafelyActivity::ReloadFinished()
{
	if (GetCharacter())
	{
		ABaseMagazineWeapon* bmw = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>();
		if (GetCharacter()->GetEquippedItem<ABallisticsShield>())
		{
			bmw = GetCharacter()->GetEquippedItem<ABallisticsShield>()->PistolEquippedWithShield;
		}
		
		if (bmw)
		{
			bmw->Server_NextMagazine_Implementation();
		}

		OwningController->FinishActivity(this, true, true);
	}
}

bool UReloadSafelyActivity::CanFinishActivity() const
{
	return false; // force finished through event
}

bool UReloadSafelyActivity::CanShoot() const
{
	return false;
}
