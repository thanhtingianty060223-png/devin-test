// Void Interactive, 2020

#include "SuspectCombatActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "PickupItemActivity.h"
#include "ReadyOrNotAIConfig.h"

TAutoConsoleVariable<int32> CVarSuspectsNoEngage(TEXT("a.RonSuspectsNoEngage"), 0, TEXT("Turn on to disable suspects from firing their weapon"));
TAutoConsoleVariable<int32> CVarDrawSuspectCoverLogic(TEXT("a.RonDrawSuspectCoverLogic"), 0, TEXT("1 = draw swat finding logic"));

USuspectCombatActivity::USuspectCombatActivity()
{
    ActivityName = FText::FromStringTable("SwatCommandTable", "SuspectCombat");
}

void USuspectCombatActivity::StartActivity(AAIController* Owner)
{
    Super::StartActivity(Owner);

    RequiredTimeSpentWithWeaponUp = AI_CONFIG_GET_FLOAT("SuspectTimeWithWeaponUpBeforeFiring");
}

bool USuspectCombatActivity::RunEngagementLogic(const float DeltaTime)
{
    #if !UE_BUILD_SHIPPING
    if (CVarSuspectsNoEngage.GetValueOnAnyThread() == 1)
        return false;
    #endif

    return Super::RunEngagementLogic(DeltaTime);
}

bool USuspectCombatActivity::CanPerformAction() const
{
    #if !UE_BUILD_SHIPPING
    if (CVarSuspectsNoEngage.GetValueOnAnyThread() > 0)
        return false;
    #endif
    
    return Super::CanPerformAction();
}

void USuspectCombatActivity::OnCoverExit()
{
	if (!GetCharacter())
		return;
	
	if (LastTrackedEnemy)
	{
		if (HardCoverCombatMove->GetLastAbortCoverReason() == EAbortCoverReason::HeardEnemyApproaching)
		{
			//const bool bHasAmmo = Cast<ABaseMagazineWeapon>(EquippedItem) ? Cast<ABaseMagazineWeapon>(EquippedItem)->HasAmmo() : false;
			//const bool bShouldMoveToEnemy = EquippedItem->ItemClass == EItemClass::IC_Melee || bHasAmmo;

			// If a bit far from the enemy, don't bee-line towards them, run default engagement behaviour
			const float DistanceToEnemy = FVector::Distance(LastTrackedEnemy->GetActorLocation(), GetCharacter()->GetActorLocation());
			FHitResult HitResult;
			const bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(HitResult, GetCharacter()->GetActorLocation(), LastTrackedEnemy->GetActorLocation(), ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), LastTrackedEnemy));

			if (DistanceToEnemy > 750.0f)
			{
				bAmbushAttacking = false;
				//SetLocation(FVector::ZeroVector);
				return;
			}
				
			if (bHasLOS && DistanceToEnemy > 250.0f)
			{
				bAmbushAttacking = false;
				//SetLocation(FVector::ZeroVector);
				return;
			}
			
			if (ActiveEngagementType == ECombatEngagementType::Melee ||
				ActiveEngagementType == ECombatEngagementType::ExplosiveVest)
			{
				bAmbushAttacking = true;
				//SetLocation(LastTrackedEnemy->GetActorLocation(), true);
			}
		}
	}
}
