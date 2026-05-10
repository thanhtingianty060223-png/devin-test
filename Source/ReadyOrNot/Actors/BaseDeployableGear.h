// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseItem.h"
#include "BaseDeployableGear.generated.h"

// BaseDeployableGear includes things like the ladder, shield and battering ram.
// Notable exceptions include grenade launchers/deployable weapons and the drones.

UCLASS(Blueprintable, BlueprintType, Abstract)
class READYORNOT_API ABaseDeployableGear : public ABaseItem
{
	GENERATED_BODY()

public:
	ABaseDeployableGear();

	virtual void Tick(float DeltaSeconds) override;
};
