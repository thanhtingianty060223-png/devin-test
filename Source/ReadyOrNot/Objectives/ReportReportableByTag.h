// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Actors/Gameplay/ReportableActor.h"
#include "Objectives/Objective.h"
#include "ReportReportableByTag.generated.h"

UCLASS()
class READYORNOT_API AReportReportableByTag : public AObjective
{
	GENERATED_BODY()
	
	// How many reportables with this tag do we need to find and report for this objective to be completed?
	UPROPERTY(EditDefaultsOnly, Category = "Objective Properties")
	uint32 NumReportsToComplete = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Objective Properties")
	uint32 CurrentReportCount = 0;

	UFUNCTION(BlueprintPure)
	bool HasReportedReportableByTag(const FName& Tag);

	virtual void TickObjective() override;

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnReportableReported(AReportableActor* Reportable);

	virtual FText GetFormattedName() override;
	virtual FText GetFormattedDescription() override;

	void DisableAllRelatedReportables();

public:
	UPROPERTY(EditAnywhere, Category = "Objective Properties")
	FName ReportTag;
};
