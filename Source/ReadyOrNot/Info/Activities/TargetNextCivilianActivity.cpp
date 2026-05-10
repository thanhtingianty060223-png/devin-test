// Copyright Void Interactive, 2023

#include "Info/Activities/TargetNextCivilianActivity.h"

#include "BaseCombatActivity.h"
#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticController.h"
#include "ActivityManagerTemplates.h"

UTargetNextCivilianActivity::UTargetNextCivilianActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "TargetNextCivilian");
	
	bAbortIfTrackingEnemy = true;
	bAbortActivityIfCannotReachLocation = true;
	
	MoveAcceptanceRadius = 200.0f;
}

void UTargetNextCivilianActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!GetCharacter()->IsSuspect())
	{
		ACTIVITY_FAILED("Must be a suspect to be able to target civilians");
		return;
	}
	
	FindNextCivilian();

	if (!TargetingCivilian)
	{
		ACTIVITY_FAILED("No civilians available to target", true);
		return;
	}
	
	const bool bAnotherSuspectIsTargeting = UActivityManager::AnyAIHasActivity<UTargetNextCivilianActivity>([&](const UTargetNextCivilianActivity* Activity)
	{
		return Activity != this && Activity->TargetingCivilian && Activity->TargetingCivilian == TargetingCivilian;
	});
	
	if (bAnotherSuspectIsTargeting)
	{
		ACTIVITY_FAILED("Civilian is already being targeted. Aborting...", true);
		return;
	}
}

void UTargetNextCivilianActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	MoveAcceptanceRadius = 200.0f;
	
	TimeUntilNextVO = FMath::Max(TimeUntilNextVO - DeltaTime, 0.0f);

	const bool bCanTargetCivilian = IsValid(TargetingCivilian) && !TargetingCivilian->IsDeadNotUnconscious() && !TargetingCivilian->IsIncapacitated();

	if (bCanTargetCivilian)
	{
		const bool bCloseToCiv = FVector::Distance(GetCharacter()->GetActorLocation(), TargetingCivilian->GetActorLocation()) < 400.0f;
		if (!bCloseToCiv)
		{
			Location = TargetingCivilian->GetNavAgentLocation();
			RequestMoveAsync();
		}
		else
		{
			if (GetCharacter()->HasLineOfSightToCharacter(TargetingCivilian))
			{
				Location = FVector::ZeroVector;
				AbortMove();
			}
			else
			{
				Location = TargetingCivilian->GetNavAgentLocation();
				RequestMoveAsync();
			}
		}
		
		if (Location != FVector::ZeroVector)
		{
		}
		else
		{
			#if !UE_BUILD_SHIPPING
			DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), TargetingCivilian->GetActorLocation(), FColor::MakeRedToGreenColorFromScalar(TimeUntilKill/TimeUntilKillOriginal), false, DeltaTime + 0.025f, 0, 1.5f);
			#endif
			
			TimeUntilKill = FMath::Max(TimeUntilKill - DeltaTime, 0.0f);
			
			if (TimeUntilKill <= 0.0f)
			{
				OwningController->GetCombatActivity()->ScriptedFireAtActor(TargetingCivilian, 1.5f, true, 0.0f, true);
				
				GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::TELL_SHOOTING, 4.0f);
				TargetingCivilian->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING, 3.0f);
			}
			else
			{
				// play VO
				if (TimeUntilNextVO <= 0.0f)
				{
					TimeUntilNextVO = AI_CONFIG_GET_FLOAT("ASAnnounceKillVoiceLineInterval", 5.0f);

					if (GetCharacter()->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_ANNOUNCE_KILL))
					{
						//ULog::Info(GetCharacter()->GetName() + " play VO");
					}
					
					FTimerDelegate d;
					d.BindWeakLambda(this, [=]
					{
						if (TargetingCivilian->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::TELL_FLEEING))
						{
							//ULog::Info(TargetingCivilian->GetName() + " play VO");
						}
					});
					
					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, d, 1.0f);
				}
			}
		}
	}
	else
	{
		OwningController->FinishActivity(this, true, true);
	}
}

void UTargetNextCivilianActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	OwningController->GetCombatActivity()->StopScriptedFire();
}

bool UTargetNextCivilianActivity::CanShoot() const
{
	return true;
}

bool UTargetNextCivilianActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (!TargetingCivilian)
		return false;
	
	if (TimeUntilKill < 2.0f)
	{
		FocalPoint = TargetingCivilian->GetMesh()->GetBoneLocation("spine_3");
		return true;
	}
	
	return false;
}

void UTargetNextCivilianActivity::ResetData()
{
	Super::ResetData();

	TargetingCivilian = nullptr;

	TimeUntilNextVO = 0.0f;
	TimeUntilKill = 0.0f;
	TimeUntilKillOriginal = 0.0f;
}

bool UTargetNextCivilianActivity::CanFinishActivity() const
{
	return false;
}

void UTargetNextCivilianActivity::FindNextCivilian()
{
	if (ACyberneticCharacter* Civ = FindNextClosestAliveCivilian())
	{
		SetLocation(Civ->GetNavAgentLocation());
		const FVector2D Vec = AI_CONFIG_GET_VECTOR2D("ASTimeDelayUntilKill", FVector2D(30.0f, 35.0f));
		TimeUntilKill = FMath::FRandRange(Vec.X, Vec.Y);
		TimeUntilKillOriginal = TimeUntilKill;
		TimeUntilNextVO = 0.0f;
		TargetingCivilian = Civ;
	}
}

ACyberneticCharacter* UTargetNextCivilianActivity::FindNextClosestAliveCivilian() const
{
	float ClosestDistance = FLT_MAX;
	ACyberneticCharacter* ClosestCiv = nullptr;
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI->IsCivilian())
		{
			const float Distance = FVector::Distance(GetCharacter()->GetActorLocation(), AI->GetActorLocation());
			
			if (Distance < ClosestDistance &&
				!(AI->IsDeadOrUnconscious() || AI->IsIncapacitated()) &&
				!UActivityManager::AnyAIHasActivity<UTargetNextCivilianActivity>([&](const UTargetNextCivilianActivity* Activity)
				{
					return Activity != this && Activity->TargetingCivilian && Activity->TargetingCivilian == AI;
				}))
			{
				ClosestDistance = Distance;
				ClosestCiv = AI;
			}
		}
	}

	return ClosestCiv;
}
