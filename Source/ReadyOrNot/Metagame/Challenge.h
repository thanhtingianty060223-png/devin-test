// Copyright Void Interactive, 2017

#pragma once
#include "CoreMinimal.h"
#include "Challenge.generated.h"

// Challenges are either global (and assigned by the gamestate's list of challenges) or per-map (and assigned by the leveldata's list of challenges)
// Challenges are designed to be made entirely in BP and implement from different interfaces (ListenForArrest, ListenForDeath, etc)
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UChallenge : public UObject
{
	GENERATED_BODY()

public:

	// The name of this challenge
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenge)
	FText ChallengeName;

	// The description of this challenge. {ChallengeProgressMax} can be inserted in this text.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenge)
	FText ChallengeDescription;

	// The name of the Challenge Progress as recorded in the LevelStatSave
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenge)
	FName ChallengeProgressName;

	// The current progress level towards this challenge
	UPROPERTY(BlueprintReadOnly, Category = Challenge)
	int32 ChallengeProgressCurrent = 0;

	// The maximum number of Challenge Progress
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenge)
	int32 ChallengeProgressMax;

	// Whether or not to show this challenge in the UI. Only shows it if it is complete.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenge)
	bool bHiddenChallenge = false;

	// Whether or not this challenge has been completed
	UPROPERTY(BlueprintReadOnly, Category = Challenge)
	bool bChallengeComplete = false;

	// If true, this is a level-specific challenge (and was added to the challenge manager from the level data)
	UPROPERTY(BlueprintReadOnly, Category = Challenge)
	bool bLevelSpecificChallenge = false;

	/////////////////////////////////////////////////////
	//
	//	Functions

	UFUNCTION(BlueprintCallable, Category = Challenge)
	void IncrementChallengeProgress(int32 IncrementBy = 1);

	UFUNCTION(BlueprintCallable, Category = Challenge)
	void DecrementChallengeProgress(int32 DecrementBy = 1);

	UFUNCTION(BlueprintCallable, Category = Challenge)
	void ResetChallengeProgress();

	UFUNCTION(BlueprintCallable, Category = Challenge)
	void UpdateFromProfile(class UReadyOrNotProfile* Profile);

	UFUNCTION(BlueprintImplementableEvent, Category = Challenge)
	void OnChallengeAchieved();

	UFUNCTION(BlueprintImplementableEvent, Category = Challenge)
	void OnChallengeInit(class AReadyOrNotGameState* gs);
};