#include "Challenge.h"
#include "ReadyOrNot.h"
#include "Metagame/Profile.h"

void UChallenge::IncrementChallengeProgress(int32 IncrementBy)
{
	if (bChallengeComplete)
	{
		return;
	}

	ChallengeProgressCurrent += IncrementBy;
	if (ChallengeProgressCurrent >= ChallengeProgressMax)
	{
		ChallengeProgressCurrent = ChallengeProgressMax;
		bChallengeComplete = true;
		OnChallengeAchieved();
	}
}

void UChallenge::DecrementChallengeProgress(int32 DecrementBy)
{
	if (bChallengeComplete)
	{
		return;
	}

	ChallengeProgressCurrent -= DecrementBy;
	if (ChallengeProgressCurrent >= ChallengeProgressMax)
	{
		ChallengeProgressCurrent = ChallengeProgressMax;
		bChallengeComplete = true;
		OnChallengeAchieved();
	}
}

void UChallenge::ResetChallengeProgress()
{
	bChallengeComplete = false;
	ChallengeProgressCurrent = 0;
}

void UChallenge::UpdateFromProfile(UReadyOrNotProfile* Profile)
{
	if (!Profile)
	{
		return;
	}

	ChallengeProgressCurrent = Profile->ChallengeProgress.FindOrAdd(ChallengeProgressName);
	if (ChallengeProgressCurrent >= ChallengeProgressMax)
	{
		ChallengeProgressCurrent = ChallengeProgressMax;
		bChallengeComplete = true;
	}
}