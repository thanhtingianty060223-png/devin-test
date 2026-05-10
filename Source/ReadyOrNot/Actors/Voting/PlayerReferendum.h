// Copyright Void Interactive, 2017

#pragma once

#include "Referendum.h"
#include "PlayerReferendum.generated.h"

// Player Referendums target specific players (ie: kick player, mute player, etc)
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API APlayerReferendum : public AReferendum
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Referendum)
	class AReadyOrNotPlayerState* TargetPlayerState;
};