// Copyright Void Interactive, 2023

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ChallengeManager.generated.h"

// The ChallengeManager exists on the client
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UChallengeManager final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UChallengeManager* Get(UObject* Context);

	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	
	UFUNCTION(BlueprintCallable)
	void InitChallenges(class AReadyOrNotGameState* GameState, FLevelDataLookupTable LevelData);

	UFUNCTION(BlueprintCallable)
	void SaveChallenges();

	UPROPERTY(BlueprintReadOnly)
	TArray<class UChallenge*> Challenges;

	UPROPERTY(BlueprintReadOnly)
	class UReadyOrNotProfile* Profile;
};