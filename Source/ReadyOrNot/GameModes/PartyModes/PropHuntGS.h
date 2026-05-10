// Copyright Void Interactive, 2023

#pragma once

#include "ReadyOrNotGameState.h"
#include "PropHuntGS.generated.h"

UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API APropHuntGS final : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Prop Hunt")
	TArray<UStaticMesh*> AvailableProps;
};
