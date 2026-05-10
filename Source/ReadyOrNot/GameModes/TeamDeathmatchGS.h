 // Copyright Void Interactive, 2017

#pragma once

#include "ReadyOrNotGameState.h"
#include "TeamDeathmatchGS.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ATeamDeathmatchGS : public AReadyOrNotGameState
{
	GENERATED_BODY()
	
public:
	virtual int32 GetCurrentSwatScore() override { return GetTeamScore(ETeamType::TT_SERT_BLUE); }
	virtual int32 GetCurrentSuspectScore() override { return GetTeamScore(ETeamType::TT_SERT_RED); }
	virtual int32 GetMaxSwatScore() override { return Scorelimit; }
	virtual int32 GetMaxSuspectScore() override { return Scorelimit; }
};
