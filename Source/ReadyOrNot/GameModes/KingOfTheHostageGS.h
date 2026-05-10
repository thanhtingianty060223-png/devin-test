// ÂCopyright Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "KingOfTheHostageGS.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AKingOfTheHostageGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:

		UPROPERTY(Replicated, BlueprintReadOnly, Category = KOTH)
		float RedTeam_RoundTimeRemaining;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = KOTH)
		float BlueTeam_RoundTimeRemaining;
	
};
