// Copyright Void Interactive, 2021

#include "ReportReportableByTag.h"

#include "Actors/Gameplay/ReportableActor.h"
#include "AnimNodes/AnimNode_RandomPlayer.h"

void AReportReportableByTag::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AReportableActor> It(GetWorld()); It; ++It)
	{
		AReportableActor* ReportableActor = *It;

		if (ReportableActor->Tags.Contains(ReportTag))
		{
			ReportableActor->OnReported.AddDynamic(this, &AReportReportableByTag::OnReportableReported);
		}
	}

	ObjectiveName = GetFormattedName();
	ObjectiveDescription = GetFormattedDescription();
}

void AReportReportableByTag::OnReportableReported(AReportableActor* Reportable)
{
	if (!HasAuthority())
		return;
	
	// Confirm that the reportable still has the desired tag
	if (Reportable->Tags.Contains(ReportTag))
	{
		CurrentReportCount++;
	}
}

void AReportReportableByTag::TickObjective()
{
	if (!HasAuthority())
		return;

	if (HasReportedReportableByTag(ReportTag))
	{
		ObjectiveCompleted();
		DisableAllRelatedReportables();
	}
}

bool AReportReportableByTag::HasReportedReportableByTag(const FName& Tag)
{
	if (CurrentReportCount >= NumReportsToComplete)
		return true;
	
	return false;
}

FText AReportReportableByTag::GetFormattedName()
{
	return FText::Format(ObjectiveName, FText::AsNumber(NumReportsToComplete));
}

FText AReportReportableByTag::GetFormattedDescription()
{
	return FText::Format(ObjectiveDescription, FText::AsNumber(NumReportsToComplete));
}

void AReportReportableByTag::DisableAllRelatedReportables()
{
	// Could keep an array of these rather than looping through an iterator. Runs infrequently so no big deal tho
	for (TActorIterator<AReportableActor> It(GetWorld()); It; ++It)
	{
		AReportableActor* ReportableActor = *It;

		if (ReportableActor->Tags.Contains(ReportTag))
		{
			ReportableActor->SetReportableEnabled(false);
		}
	}
}