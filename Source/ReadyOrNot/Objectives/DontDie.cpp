// Copyright Void Interactive, 2021

#include "DontDie.h"

#include "GameModes/CoopGS.h"
#include "Info/ScoringManager.h"

ADontDie::ADontDie()
{
	ObjectiveName = NSLOCTEXT("DontDie", "Survive The Mission", "Survive The Mission");
	ObjectiveDescription = NSLOCTEXT("DontDie", "All officers must survive the operation.", "All officers must survive the operation.");
	
	bFailureEndsMission = true;
	bHiddenObjective = true;

	ScoringComponent->ScoringData.bHiddenScore = true;
	ScoringComponent->bAutoAddToScorePool = false;
	ScoringComponent->ScoreGroupName = "MissionObjectives";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::PrimaryObjective;
}

void ADontDie::TickObjective()
{
	if (!HasAuthority())
		return;

	ACoopGS* CoopGS = GetWorld()->GetGameState<ACoopGS>();
	if (!CoopGS)
		return;

	if (CoopGS->MatchState != EMatchState::MS_Playing)
		return;

	// this needs to either be completed or failed at any given point it cannot be inprogress as then soft complete won't work
	
	bool bAnyDead = false;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* PlayerController = *It;
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
		if (PlayerCharacter)
		{
			if (PlayerCharacter->IsDeadOrUnconscious())
			{
				bAnyDead = true;
				break;
			}
		}
	}
	
	if (bAnyDead)
	{
		ObjectiveFailed();
	} else
	{
		ObjectiveCompleted();
	}
}
