// © Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ObituaryData.generated.h"

USTRUCT(BlueprintType)
struct FObituaryForBone
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FText> DeathMessages;
};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UObituaryData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite , Category = Obituary)
		TMap<FName, FObituaryForBone> PointDeathMessages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Obituary)
		TArray<FText> DefaultBulletDeathText;
};
