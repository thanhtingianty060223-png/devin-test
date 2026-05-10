// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "DeployableData.generated.h"

UCLASS()
class READYORNOT_API UDeployableData : public UDataAsset
{
	GENERATED_BODY()

public:
	// The name of this deployable as it appears in the menu.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Deployable)
	FText DeployableName;

	// The short name of this deployable as it appears in the menu.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Deployable)
	FText DeployableShortName;

	// The description of this deployable as it appears in the menu.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Deployable)
	FText DeployableDescription;

	// The texture associated with this deployable as it appears in the menu.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Deployable)
		TSoftObjectPtr<UTexture2D> DeployableTexture;

	// The label of this deployable. Whenever the game starts, it will spawn all deployables matching this label on the depot.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Deployable)
		FName DeployableLabel;
};
