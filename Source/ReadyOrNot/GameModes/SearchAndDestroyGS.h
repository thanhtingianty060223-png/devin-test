// Copyright Void Interactive, 2021

#pragma once

#include "ReadyOrNotGameState.h"
#include "SearchAndDestroyGS.generated.h"


UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ASearchAndDestroyGS : public AReadyOrNotGameState
{
	GENERATED_BODY()




public:

UPROPERTY(Replicated, BlueprintReadWrite, Category = "S&D")
	float BombCountActive;

UPROPERTY(Replicated, BlueprintReadWrite, Category = "S&D")
	bool bBombPlanted;

UPROPERTY(Replicated, BlueprintReadWrite, Category = "S&D")
	bool bBombDetonate;

UPROPERTY(Replicated, BlueprintReadWrite, Category = "S&D")
	bool bBombDefused;

UPROPERTY(Replicated, BlueprintReadWrite, Category = "S&D")
	APlayerCharacter* bHasBomb;

};


