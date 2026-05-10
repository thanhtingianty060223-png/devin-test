// Void Interactive, 2020

#include "SoftCoverCombatMove.h"

#include "Characters/CyberneticController.h"
#include "Characters/AI/SuspectCharacter.h"

#include "NavigationSystem.h"

void USoftCoverCombatMove::RequestCombatMove(float DeltaTime)
{
    if (OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() == FVector::ZeroVector)
        return;

    if (TimeUntilToggleVis < 0.0f)
    {
        bWantsToBeVisible = !bWantsToBeVisible;
        !bWantsToBeVisible ? TimeUntilToggleVis = 10.0f : TimeUntilToggleVis = 1.0f;
        Location = FVector::ZeroVector;
    }
    
    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FHitResult CharacterHit;
        FRotator ViewRotation;
        FVector ViewPoint;
        GetCharacter()->GetActorEyesViewPoint(ViewPoint, ViewRotation);
        GetWorld()->LineTraceSingleByObjectType(CharacterHit, OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() + FVector(0.0f, 0.0f, 70.0f), ViewPoint, FCollisionObjectQueryParams(ECC_WorldStatic));
        bCanSeeLastKnownPosition = !CharacterHit.bBlockingHit;

        if ((!bWantsToBeVisible && CharacterHit.bBlockingHit) || (bWantsToBeVisible && !CharacterHit.bBlockingHit))
        {
            TimeUntilToggleVis -= DeltaTime;
        }     
        
        if (!bWantsToBeVisible && CharacterHit.bBlockingHit)
            return;
        
        if (bWantsToBeVisible && !CharacterHit.bBlockingHit)
        {
            OwningController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::ForcedScript);
            return;
        }
        
        if (!bWantsToBeVisible)
        {
            if (!OwningController->IsMoving())
            {
                FNavLocation OutLocation;
                float DistToLastKnownPos = (OwningController->GetTargetingComp()->GetLastKnownEnemyPosition() - GetCharacter()->GetActorLocation()).Size();
                DistToLastKnownPos = FMath::Max(DistToLastKnownPos, 700.0f);
                NavSys->GetRandomReachablePointInRadius(GetCharacter()->GetActorLocation(), DistToLastKnownPos, OutLocation);
        
                FHitResult Hit;
                GetWorld()->LineTraceSingleByObjectType(Hit, OwningController->GetTargetingComp()->GetLastKnownEnemyPosition(), OutLocation.Location + FVector(0.0f, 0.0f, 70.0f), FCollisionObjectQueryParams(ECC_WorldStatic));
                if (Hit.bBlockingHit)
                {
                    Location = OutLocation.Location;
                }
            }
        }
        else
        {
            Location = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
        }

        if (!OwningController->IsMoving() && Location != FVector::ZeroVector)
        {
            PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::BARK_COVER_TRANSITION, true, 5.0f);   
        }
    }
}
