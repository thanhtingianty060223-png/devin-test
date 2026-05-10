// Copyright Void Interactive, 2023

#include "Objectives/ArrestXSuspects.h"

#include "Info/ScoringManager.h"

AArrestXSuspects::AArrestXSuspects()
{
	ObjectiveName = NSLOCTEXT("ArrestXSuspects", "Arrest X Suspects", "Arrest {0} Suspects");
	ObjectiveDescription = NSLOCTEXT("ArrestXSuspects", "Arrest X Suspects at the scene.", "Arrest {0} Suspects at the scene.");
	bFailureEndsMission = false;
}

void AArrestXSuspects::BeginPlay()
{
	Super::BeginPlay();
	ObjectiveName = GetFormattedName();
	ObjectiveDescription = GetFormattedDescription();
}

void AArrestXSuspects::TickObjective()
{
	int32 OutReported = 0, OutArrested = 0, OutKilled = 0, OutTotal = 0;
	AScoringManager::GetSuspectCount(OutReported, OutArrested, OutKilled, OutTotal);

	const bool bArrestedRequiredSuspects = OutArrested >= RequiredArrests;
	const bool bNoRemainingSuspectsToArrest = OutTotal - OutKilled < RequiredArrests;

	if (bArrestedRequiredSuspects)
	{
		ObjectiveCompleted();
	}
	else if (bNoRemainingSuspectsToArrest)
	{
		ObjectiveFailed();
	}
}

FText AArrestXSuspects::GetFormattedName()
{
	return FText::Format(ObjectiveName, FText::AsNumber(RequiredArrests));
}

FText AArrestXSuspects::GetFormattedDescription()
{
	return FText::Format(ObjectiveDescription, FText::AsNumber(RequiredArrests));
}
