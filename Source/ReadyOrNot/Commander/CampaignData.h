// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CampaignData.generated.h"

// TODO(killo): modded campaigns eventually

/**
 * 
 */
UCLASS()
class READYORNOT_API UCampaignData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FText CampaignTitle;

	UPROPERTY(EditAnywhere)
	FText CampaignAuthor;

	UPROPERTY(EditAnywhere)
	FText CampaignDescription;

	// The list of levels that belong to this campaign, in order of completion
	UPROPERTY(EditAnywhere)
	TArray<FString> Levels;
};
