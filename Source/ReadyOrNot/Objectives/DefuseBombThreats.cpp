// Void Interactive, 2020

#include "Objectives/DefuseBombThreats.h"

#include "Actors/Gameplay/BombActor.h"
#include "Info/TOCManager.h"

ADefuseBombThreats::ADefuseBombThreats()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.0f;
	
	ObjectiveName = NSLOCTEXT("DefuseTheBombThreat", "Defuse The Bomb Threats", "Defuse The Bomb Threats");
	ObjectiveDescription = NSLOCTEXT("DefuseTheBombThreatNoTimer", "Deactivate any bombs at the scene", "Deactivate any bombs at the scene.");
	
	LockedToMode = ECOOPMode::CM_BombThreat;
}

void ADefuseBombThreats::OnObjectiveCreated_Implementation()
{
	Super::OnObjectiveCreated_Implementation();

	for (TActorIterator<ABombActor> It(GetWorld()); It; ++It)
	{
		ABombActor* BombActor = *It;
		if (!BombActor->bPVPBombOnly)
		{
			BombActor->OnBombDefused.AddDynamic(this, &ADefuseBombThreats::OnBombDefused);
		}
	}
}

void ADefuseBombThreats::OnObjectiveCompleted_Implementation()
{
	Super::OnObjectiveCompleted_Implementation();

	ATOCManager::Get()->StartTOCResponse("bombthreatsuccess", true, ETOCPriority::ETP_Flush);
}

void ADefuseBombThreats::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	for (TActorIterator<ABombActor> It(GetWorld()); It; ++It)
	{
		if (It->GetBombState() != EBombState::BS_Disabled && It->GetBombState() != EBombState::BS_None)
		{
			const float Time = FMath::RoundToFloat((float)FTimespan::FromSeconds(It->GetTimeUntilExplodes()).GetMinutes()) + 1;

			if (Time > 0)
			{
				ObjectiveDescription = FText::Format(NSLOCTEXT("DefuseTheBombThreatWithTimer", "Deactivate any bombs at the scene", "Deactivate any bombs at the scene. Estimated Time Remaining: {0} Minutes"), FText::FromString(FString::FromInt(Time)));
			}

			if (It->GetTimeUntilExplodes() > 0.0f && It->GetTimeUntilExplodes() < 60.0f)
			{
				if (!bHasTOCWarned)
				{
					ATOCManager::Get()->StartTOCResponse("bombthreathurry", true, ETOCPriority::ETP_HighPriority);
					
					bHasTOCWarned = true;

					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ADefuseBombThreats::ResetTOCWarning, TOCWarningCooldown);
				}
			}
			
			return;
		}
	}
}

void ADefuseBombThreats::TickObjective()
{
	for (TActorIterator<ABombActor> It(GetWorld()); It; ++It)
	{
		if (It->bPVPBombOnly)
			continue;
		
		if (It->GetBombState() == EBombState::BS_Exploded)
		{
			ObjectiveFailed();
			return;
		}
		
		if (It->GetBombState() != EBombState::BS_Disabled)
		{
			return;
		}
	}
	
	ObjectiveCompleted();
}

void ADefuseBombThreats::OnBombDefused(ABombActor* DefusedBomb)
{
	ATOCManager::Get()->StartTOCResponse("bombthreatdefused", true);
}

void ADefuseBombThreats::ResetTOCWarning()
{
	bHasTOCWarned = false;
}
