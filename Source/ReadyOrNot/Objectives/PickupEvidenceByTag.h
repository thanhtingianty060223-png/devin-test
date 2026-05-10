// Copyright Void Interactive, 2021

#pragma once

#include "Objectives/Objective.h"
#include "PickupEvidenceByTag.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API APickupEvidenceByTag : public AObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Evidence Properties")
	FName EvidenceTag;

	UFUNCTION(BlueprintPure)
	static bool HasCollectedEvidenceByTag(const FName& Tag);
	
	virtual void TickObjective() override;
};
