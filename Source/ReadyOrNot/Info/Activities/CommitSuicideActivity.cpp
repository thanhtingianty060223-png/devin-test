// Void Interactive, 2020

#include "Info/Activities/CommitSuicideActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

UCommitSuicideActivity::UCommitSuicideActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "CommitSuicide");
	bNoMoveActivity = true;
	bIsProgressActivity = false;
	bAbortIfTrackingEnemy = false;
	bAbortIfNotMovingForAWhile = false;
}

#if !UE_BUILD_SHIPPING
void UCommitSuicideActivity::GatherDebugString(FString& OutString)
{
	OutString += AddDebugString("Is Fake Suicide", bFakeOut ? "true" : "false");
	OutString += AddDebugString("Animation", ChosenAnimation);
	OutString += AddDebugString("Suicide Prevention Chance", FString::SanitizeFloat(SuicidePreventionChance));
}
#endif

void UCommitSuicideActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	OwningController->bStopDecisionMaking = true;
	
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UCommitSuicideActivity::OnDamaged);
	
	GetCharacter()->OnMeleeHitTaken.RemoveAll(this);
	GetCharacter()->OnMeleeHitTaken.AddDynamic(this, &UCommitSuicideActivity::OnMeleeHitTaken);
	
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.AddDynamic(this, &UCommitSuicideActivity::OnStunned);
	
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.AddDynamic(this, &UCommitSuicideActivity::OnHeardYell);
	
	GetCharacter()->ReasonsToStandStill.AddUnique("suicide");

	ChosenAnimation = "tp_suicide";

	if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>())
	{
		if (EquippedWeapon->ItemClass == EItemClass::IC_Shotgun)
		{
			ChosenAnimation += "_shotgun";
		}
		else if (EquippedWeapon->ItemClass == EItemClass::IC_Pistol)
		{
			ChosenAnimation += "_pistol";
		}
		else
		{
			ChosenAnimation += "_rifle";
		}
	}

	if (!bFakeOut)
	{
		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			if (!Weapon->HasAmmo())
			{
				if (FMath::RandBool())
				{
					bFakeOut = true;
				}
				else
				{
					Weapon->ReplenishAmmo();
					Weapon->bInfiniteAmmo = true;
				}
			}
		}
	}

	if (bFakeOut)
		ChosenAnimation += "_fake";

	GetCharacter()->Multicast_Stop3PMontage_Implementation(nullptr, 0.25f);
	GetCharacter()->PlayMontageFromTableWithFocalPoint(ChosenAnimation, OwningController->GetTrackedTarget() ? OwningController->GetTrackedTarget()->GetActorLocation() : FVector::ZeroVector);

	GetCharacter()->LastHitBoneName = NAME_None;
	
	GetCharacter()->bCommitingSuicide = true;
}

void UCommitSuicideActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	Location = FVector::ZeroVector;

	if (!GetCharacter()->IsTableMontagePlaying(ChosenAnimation) && ElapsedActivityTime > 1.0f)
	{
		//ULog::Info("not playing anim ------------------");
		OwningController->FinishActivity(this, true, true);
		return;
	}

	if (GetCharacter()->IsDeadOrUnconscious())
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}
}

void UCommitSuicideActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	OwningController->bStopDecisionMaking = false;
	
	if (bFakeOut)
	{
		if (bSuccess)
		{
			OnFakeOutSuccess.Broadcast();
		}
	}
	
	if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
	{
		Weapon->bInfiniteAmmo = false;
	}

	GetCharacter()->OnWeaponForceFire_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
	
	GetCharacter()->ReasonsToStandStill.Remove("suicide");

	GetCharacter()->bCommitingSuicide = false;
}

bool UCommitSuicideActivity::CanOverrideActivity() const
{
	return false;
}

bool UCommitSuicideActivity::CanFinishActivity() const
{
	return false; // only force finished
}

bool UCommitSuicideActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (OwningController->GetTrackedTarget())
	{
		FocalPoint = OwningController->GetTrackedTarget()->GetActorLocation();
		return true;
	}
	
	FocalPoint = FVector::ZeroVector;
	return false;
}

void UCommitSuicideActivity::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::OnKilled(InstigatorCharacter, KilledCharacter);
	
	// Successful suicide
	OwningController->FinishActivity(this, true, true);
}

void UCommitSuicideActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	if (!GetCharacter())
		return;
	
	GetCharacter()->StopTPMontageFromTable(ChosenAnimation);
	GetCharacter()->Surrender();

	ACTIVITY_FAILED("Failed to commit suicide. Stunned by " + GetNameSafe(DamageCauser), true);

	OwningController->ClearActivityList();
}

void UCommitSuicideActivity::OnHeardYell(AReadyOrNotCharacter* Shouter, const bool bLOS)
{
	if (!GetCharacter())
		return;
	
	if (bLOS)
	{
		// one percent chance of listening to stop suiciding
		if (FMath::FRand() <= SuicidePreventionChance)
		{
			GetCharacter()->StopTPMontageFromTable(ChosenAnimation);
			GetCharacter()->Surrender();

			ACTIVITY_FAILED("Failed to commit suicide. Gave up on yell by " + GetNameSafe(Shouter), true);

			OwningController->ClearActivityList();

			return;
		}

		// Increase by one percent everytime we hear a yell
		SuicidePreventionChance += 0.01f;
	}
}

void UCommitSuicideActivity::OnDamaged(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	if (!GetCharacter())
		return;
	
	GetCharacter()->StopTPMontageFromTable(ChosenAnimation);
	GetCharacter()->Surrender();

	ACTIVITY_FAILED("Failed to commit suicide. Damaged by " + GetNameSafe(DamageCauser), true);

	OwningController->ClearActivityList();
}

void UCommitSuicideActivity::OnMeleeHitTaken(AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!GetCharacter())
		return;
	
	GetCharacter()->StopTPMontageFromTable(ChosenAnimation);
	GetCharacter()->Surrender();
	
	ACTIVITY_FAILED("Failed to commit suicide. Melee'd by " + GetNameSafe(InstigatorCharacter), true);

	OwningController->ClearActivityList();
}
