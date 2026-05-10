// Copyright Void Interactive, 2023

#include "Objective.h"
#include "Components/ScoringComponent.h"

AObjective::AObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;

	bFindCameraComponentWhenViewTarget = false;
	
	bReplicates = true;
	bAlwaysRelevant = true;

	ObjectiveStatus = EObjectiveStatus::Objective_InProgress;

	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->ScoreGroupName = "MissionObjectives";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::PrimaryObjective;
	ScoringComponent->bShowPopupOnTakeAllScoresWithNoChange = true;
}

void AObjective::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AObjective, ObjectiveStatus);
}

void AObjective::ObjectiveCompleted()
{
	if (ObjectiveStatus == EObjectiveStatus::Objective_Complete)
		return;

	if (bHiddenObjective && !bAllowCompletionWhileHidden)
		return;
	
	ObjectiveStatus = EObjectiveStatus::Objective_Complete;

	if (bHiddenObjective && UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_Self)
	{
		Multicast_UnlockObjective();
	}

	FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "ObjectiveCompleteWithName"), ObjectiveName);
	ScoringComponent->GiveAllScores(true, !bHiddenObjective, ScoreText);
	
	OnObjectiveCompleted();
	OnObjectiveUpdated.Broadcast(this);
	
	SetActorTickEnabled(false);
}

void AObjective::ObjectiveFailed()
{
	if (ObjectiveStatus == EObjectiveStatus::Objective_Failed)
		return;
	
	ObjectiveStatus = EObjectiveStatus::Objective_Failed;

	FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "ObjectiveFailedWithName"), ObjectiveName);
	ScoringComponent->TakeAllScores(ScoreText, !bHiddenObjective, true);

	OnObjectiveFailed();
	OnObjectiveUpdated.Broadcast(this);
	
	SetActorTickEnabled(false);
}

void AObjective::OnObjectiveCompleted_Implementation()
{
}

void AObjective::OnObjectiveFailed_Implementation()
{
}

void AObjective::OnObjectiveCreated_Implementation()
{
}

void AObjective::TickObjective()
{
	if (!IsObjectiveInProgress())
		return;
		
	TickObjective_BP();
}

bool AObjective::IsObjectiveInProgress() const
{
	return ObjectiveStatus == EObjectiveStatus::Objective_InProgress;
}

bool AObjective::IsObjectiveCompleted() const
{
	return ObjectiveStatus == EObjectiveStatus::Objective_Complete;
}

bool AObjective::IsObjectiveFailed() const
{
	return ObjectiveStatus == EObjectiveStatus::Objective_Failed;
}

void AObjective::OnRep_ObjectiveStatus()
{
	OnObjectiveUpdated.Broadcast(this);
}

void AObjective::OnMissionObjectivesCreated()
{
	if (!bHiddenObjective || UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_None)
		return;

	if (UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_Reportable)
	{		
		for (TActorIterator<AReportableActor> It(GetWorld()); It; ++It)
		{
			if (It->GetClass() != UnlockingReportableClass.Get())
				continue;

			It->OnReported.AddDynamic(this, &AObjective::OnUnlockingReportableReported);
			break;
		}
	}
	else
	{
		for (TActorIterator<AObjective> It(GetWorld()); It; ++It)
		{
			if (It->GetClass() != UnlockingObjectiveClass.Get())
				continue;

			It->OnObjectiveUpdated.AddDynamic(this, &AObjective::OnUnlockingObjectiveUpdated);
			break;
		}
	}
}

void AObjective::OnUnlockingReportableReported(AReportableActor* ReportableActor)
{
	Multicast_UnlockObjective();
}

void AObjective::OnUnlockingObjectiveUpdated(AObjective* Objective)
{
	// If we are still hidden and the objective we need to complete to become unhidden has just failed, we fail this one too
	if (Objective->IsObjectiveFailed() && bHiddenObjective)
	{
		ObjectiveFailed();
	}
	else if (Objective->IsObjectiveCompleted())
	{
		Multicast_UnlockObjective();
	}
}

void AObjective::Multicast_UnlockObjective_Implementation()
{
	bHiddenObjective = false;

	// Tell GameState to broadcast that we've updated an objective
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->OnMissionObjectivesUpdated.Broadcast();
	}

	// Don't show the unlock text if unlock method is self - Objective complete text is enough
	if (UnlockMethod == EHiddenObjectiveUnlockMethod::Unlock_Self)
		return;
	
	//FText UnlockString = ObjectiveName + TEXT(" Objective Unlocked");
	FText UnlockString = FText::Format(FText::FromStringTable("ScoringTable", "ObjectiveUnlocked"), ObjectiveName);
	
	APlayerCharacter* PlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	
	if (PlayerCharacter && PlayerCharacter->HumanCharacterWidget_V2)
	{
		PlayerCharacter->HumanCharacterWidget_V2->AddObjectivePopup(UnlockString);
	}
}