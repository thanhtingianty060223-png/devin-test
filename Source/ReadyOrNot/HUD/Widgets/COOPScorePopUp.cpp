// Void Interactive, 2020

#include "COOPScorePopUp.h"

void UCOOPScorePopUp::PlayRewardSound()
{
	UFMODBlueprintStatics::PlayEvent2D(this, Reward, false);
}
