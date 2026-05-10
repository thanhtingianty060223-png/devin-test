// Copyright Void Interactive, 2023

#include "ChallengeManager.h"
#include "Challenge.h"
#include "ReadyOrNotGameState.h"
#include "Data/LevelData.h"
#include "Profile.h"

UChallengeManager* UChallengeManager::Get(UObject* Context)
{
	return Context->GetWorld()->GetSubsystem<UChallengeManager>();
}

bool UChallengeManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UChallengeManager::InitChallenges(class AReadyOrNotGameState* GameState, FLevelDataLookupTable LevelData)
{
	int32 i;
	UChallenge* NewChallenge;

	// Load up the current profile
	if (GameState)
	{
		Profile = GameState->GetCurrentProfile();
	}
	else
	{
		Profile = nullptr;
	}

	// Add level specific challenges first so they show up in the UI first
	for (i = 0; i < LevelData.Challenges.Num(); i++)
	{
		NewChallenge = NewObject<UChallenge>(this, LevelData.Challenges[i].Get());
		if (NewChallenge)
		{
			NewChallenge->bLevelSpecificChallenge = true;
			NewChallenge->UpdateFromProfile(Profile);
			NewChallenge->OnChallengeInit(GameState);
			Challenges.Add(NewChallenge);
		}
	}

	// Add game-mode specific challenges second
	if (GameState)
	{
		for (i = 0; i < GameState->GameModeChallenges.Num(); i++)
		{
			NewChallenge = NewObject<UChallenge>(this, GameState->GameModeChallenges[i].Get());
			if (NewChallenge)
			{
				NewChallenge->UpdateFromProfile(Profile);
				NewChallenge->OnChallengeInit(GameState);
				Challenges.Add(NewChallenge);
			}
		}
	}
}

void UChallengeManager::SaveChallenges()
{
	if (!Profile)
	{
		return;
	}

	for (int32 i = 0; i < Challenges.Num(); i++)
	{
		Profile->ChallengeProgress[Challenges[i]->ChallengeProgressName] = Challenges[i]->ChallengeProgressCurrent;
	}
	// don't do this the data that was loaded is out of date!!
	// commenting because im pretty sure challenges are nto even used
	//Profile->SaveProfile();
}