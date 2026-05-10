// Void Interactive, 2020

#include "SwatCombatActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Team/ArrestTargetActivity.h"

#include "CombatMove/DuelingCombatMove.h"
#include "CombatMove/PushCombatMove.h"
#include "CombatMove/FlankingCombatMove.h"

#include "Team/TeamBreachAndClearActivity.h"

#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotAISystem.h"
#include "Actors/ExplosiveVest.h"
#include "Actors/Items/BallisticsShield.h"
#include "GameModes/CoopGM.h"
#include "Info/SWATManager.h"
#include "Objectives/NeutralizeSuspectByTag.h"
#include "Team/DoorBreachActivity.h"

TAutoConsoleVariable<int32> CVarSwatNoEngage(TEXT("SWAT.NoEnage"), 0, TEXT("Turn on to disable swat from firing their weapon"));

USwatCombatActivity::USwatCombatActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "SwatCombat");
}

void USwatCombatActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	RequiredTimeSpentWithWeaponUp = AI_CONFIG_GET_FLOAT("SwatTimeWithWeaponUpBeforeFiring");
}

void USwatCombatActivity::PerformActivity(const float DeltaTime)
{
    Super::PerformActivity(DeltaTime);

	// Some basic swat AI logic against the local player if they so happen to survive the onslaught
	if (LOCAL_PLAYER)
	{
		if (USWATManager::Get(this)->IsCharacterKnownEnemy(LocalPlayer))
		{
			GetCharacter()->SetLowReady(false, false);
			
			OwningController->GetTargetingComp()->SetLastTrackedTarget(LocalPlayer);
			if (OwningController->GetTargetingComp()->CanCharacterBeSeen(LocalPlayer))
			{
				StartRunningCombatMove(DuelingCombatMove);
			}
			else
			{
				TimeUntilNextLocalPlayerCombatMove -= DeltaTime;
				if (TimeUntilNextLocalPlayerCombatMove < 0.0f)
				{
					TimeUntilNextLocalPlayerCombatMove = 5.0f;
					if (FMath::RandBool())
						StartRunningCombatMove(PushCombatMove);
					else
						StartRunningCombatMove(FlankingCombatMove);
				}
			}
		}
	}
	else
	{
		if (OwningController->GetTrackedTarget() && !IsRunningCombatMoveActivity(UDuelingCombatMove::StaticClass()))
			StartRunningCombatMove(DuelingCombatMove);
	}

	YellDelay = FMath::Clamp(YellDelay - DeltaTime, 0.0f, YellDelay);

	// try yell at target
	if (OwningController->GetTrackedTarget())
	{
		if (!OwningCharacter->bIsPairedInteractionPlaying)
		{
			if (YellDelay <= 0.0f && USWATManager::Get(this)->GlobalYellDelay <= 0.0f) // to prevent multiple swat guys from yelling at once
			{
				GetCharacter()->Server_Yell();
				
				YellDelay = 3.0f;
				USWATManager::Get(this)->GlobalYellDelay = 0.35f;
			}
		}
	}

	// swapping between primary and secondary weapons
	TickWeaponSwitch();
}

void USwatCombatActivity::TickWeaponSwitch()
{
	const bool bIsInCombat = UReadyOrNotAISystem::WasRecentlyInCombat(4.0f);
	
	if (const ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
	{
		if (IsReloading())
			return;
		
		if (GetCharacter()->GetInventoryComponent()->IsEquippingItem())
			return;

		if (Weapon->IsPlayingDraw() || Weapon->IsPlayingHolster())
			return;
		
		if (!bIsInCombat)
		{
			if (Weapon->ContainsItemCategory(EItemCategory::IC_Secondary))
			{
				EquipItem(EItemCategory::IC_Primary);
				return;
			}
		}

		bool bBelowThreshold = Weapon->GetCurrentAmmoPercentage() < 0.3f;
		if (Weapon->ContainsItemCategory(EItemCategory::IC_Secondary))
		{
			bBelowThreshold = Weapon->GetCurrentAmmoPercentage() <= 0.0f;
		}
		
		if (bBelowThreshold)
		{
			const bool bShouldEquipSecondary = bIsInCombat && Weapon->ContainsItemCategory(EItemCategory::IC_Primary) && (!Weapon->IsLessLethalWeapon() || !Weapon->HasAnyAmmo());
			if (bShouldEquipSecondary)
			{
				EquipItem(EItemCategory::IC_Secondary);
				return;
			}

			ReloadEquippedWeapon();
		}
	}
}

bool USwatCombatActivity::CanShoot() const
{
	if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon())
	{
		if (!EquippedWeapon->HasAmmo())
			return false;

		if (IsReloading())
			return false;

		return true;
	}
	
	return false;
}

bool USwatCombatActivity::ShouldStrafe() const
{
	// Is local player an enemy to swat?
	if (LOCAL_PLAYER)
	{
		if (OwningController->IsCharacterKnownEnemy(LocalPlayer))
		{
			//ULog::Info(GetCharacter()->GetName() + " | ShouldStrafe | Local player is a known enemy");
			return true;
		}
	}
	
	if ((CurrentScriptedFireAt.FireAtActor || CurrentScriptedFireAt.FireAtLocation != FVector::ZeroVector) && CurrentScriptedFireAt.TimeRemaining > 0.0f)
	{
		//ULog::Info("ShouldStrafe | Scripted fire");
		return true;
	}
	
	if ((CurrentScriptedLookAt.LookAtActor || CurrentScriptedLookAt.LookAtLocation != FVector::ZeroVector) && CurrentScriptedLookAt.TimeRemaining > 0.0f)
	{
		//ULog::Info("ShouldStrafe | Scripted fire");
		return true;
	}

	const bool bIgnoreViewBlockCheck = OwningController->GetCurrentActivity<UThrowItemThroughDoorActivity>() != nullptr ||
										OwningController->GetCurrentActivity<ULaunchGrenadeThroughDoorActivity>() != nullptr;

	if (GetCharacter<ASWATCharacter>()->bViewBlockedByOtherSwat && !bIgnoreViewBlockCheck)
	{
		//ULog::Info(GetCharacter()->GetName() + " | ShouldStrafe | Focal point view blocked by another swat");
		return false;
	}
	
	return Super::ShouldStrafe();
}

bool USwatCombatActivity::EngageEnemy(AReadyOrNotCharacter* EnemyCharacter, float DeltaTime)
{
	#if !UE_BUILD_SHIPPING
	if (CVarSwatNoEngage.GetValueOnAnyThread() > 0)
		return false;
	#endif
	
	if (CanEngageEnemy(EnemyCharacter))
	{
		if (HasRecentlySeenTarget(EnemyCharacter))
		{
			return FireWeapon(EnemyCharacter);
		}
		
		return true;
	}

	return false;
}

void USwatCombatActivity::EnterStrafeState()
{
	Super::EnterStrafeState();

	GetCharacter<ASWATCharacter>()->StopGestureAnimation();
}

bool USwatCombatActivity::GetStrafeDebugString(FString& OutString) const
{
	// Is local player an enemy to swat?
	if (LOCAL_PLAYER)
	{
		if (OwningController->IsCharacterKnownEnemy(LocalPlayer))
		{
			OutString = "Local player is a known enemy";
			return true;
		}
	}
	
	if (bObstacleInFront)
	{
		OutString = "Lean-able obstacle in front";
		return true;
	}
	
	if (GetCharacter<ASWATCharacter>()->bViewBlockedByOtherSwat)
	{
		OutString = "Focal point view blocked by another swat";
		return false;
	}
	
	return Super::GetStrafeDebugString(OutString);
}

void USwatCombatActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
	Super::GatherDebugString(OutString);
	
	#endif
}

bool USwatCombatActivity::ShouldTrackTarget() const
{
	if (const UScanDoorActivity* Activity = OwningController->GetCurrentActivity<UScanDoorActivity>())
	{
		return Activity->GetActiveStateID() == 0;
	}
	
	return true;
}

bool USwatCombatActivity::TryMoveIntoCover(AReadyOrNotCharacter* InstigatorCharacter, float MinDistanceFromInstigator, float ExclusionRadiusAroundInstigator, bool bRequireLOS)
{
	// Swat does not dynamically cover
	return false;
}

bool USwatCombatActivity::CanEngageEnemy(AReadyOrNotCharacter* Enemy) const
{
	if (!Enemy->IsActive())
		return false;

	if (Enemy->IsOnSWATTeam())
	{
		if (USWATManager::Get(this)->IsCharacterKnownEnemy(Enemy))
			return true;
	}
	
	if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(Enemy))
	{
		if (AICharacter->TimeSinceLastAggressiveForce < 1.0f) // any recent aggressive force used? always allowed to engage
			return true;
		
		// Priority 0: Arrested or Surrendered. Don't engage when this AI has given up
		if (AICharacter->bSurrendered || AICharacter->bArrestComplete ||
			AICharacter->bIsBeingArrested || AICharacter->CanArrest() ||
			AICharacter->IsInRagdoll() || AICharacter->IsStunned() ||
			AICharacter->IsRagdollBlending() ||
			AICharacter->IsPlayingDead() || AICharacter->IsGettingUp() ||
			AICharacter->IsIncapacitated())
			return false;

		if (AICharacter->IsTableMontagePlaying("tp_melee") ||
			AICharacter->IsTableMontagePlaying("tp_spct_detonatevest"))
			return true;
		
		if (const AExplosiveVest* ExplosiveVest = Cast<AExplosiveVest>(AICharacter->GetArmour()))
		{
			if (AICharacter->IsTableMontagePlaying(ExplosiveVest->DetonationMontage))
				return true;
		}

		if (!AICharacter->GetEquippedItem() && !AICharacter->IsWearingExplosiveVest())
			return false;

		// Priority 1: Don't spam fire if an AI is taking cover, only fire when they're exposing themselves
		if (AICharacter->Rep_CoverAnimState.bIsFiring)
			return true;
		
		if (AICharacter->Rep_HidingAnimState.bIsHiding || AICharacter->bIsExitingLandmark)
		{
			return false;
		}

		// Priority 3: Swat team damage. Can always engage if the swat team has been damaged by this AI
		if (AICharacter->HasDamagedSWAT() || GetCharacter()->DamagedByCharacters.Find(AICharacter) != INDEX_NONE)
			return true;
		
		// Priority 4: Less lethal weapons. Can always engage if we have less lethals equipped
		if (const ABaseMagazineWeapon* EquippedWeapon = GetCharacter()->GetEquippedWeapon())
		{
			if (EquippedWeapon->IsLessLethalWeapon())
			{
				return true;
			}
		}

		// Priority 5: AI states. Can engage under certain conditions
		if (AICharacter->bHasEverShot)
			return true;

		if (AICharacter->IsHesitating() || AICharacter->IsStartling())
			return false;

		if ((AICharacter->bIsFakeSurrender && AICharacter->FakeSurrenderTime > 1.0f) || (AICharacter->IsExitingSurrender() && AICharacter->FakeSurrenderTime > 2.0f))
			return true;

		if (AICharacter->bDrawingWeapon && AICharacter->DrawingWeaponTime > 0.1f)
			return true;
		
		if (AICharacter->bPickingUpWeapon && AICharacter->PickingUpWeaponTime > 0.1f)
			return true;

		if (AICharacter->GetCyberneticsController()->GetCombatActivity()->TimeSpentEngagingOnTarget > 1.0f)
			return true;
		
		return AICharacter->bAimingAtTarget && AICharacter->GetCyberneticsController()->GetCombatActivity()->TimeSpentEngagingOnTarget > 0.5f;
	}

	return false;
}
