// Copyright Void Interactive, 2017

#pragma once

#include "UObject/Interface.h"
#include "ListenForDeath.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UListenForDeath : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IListenForDeath
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ListenForDeath")
	void OnCharacterDied(class AReadyOrNotCharacter* Victim, class AReadyOrNotCharacter* Killer, class AActor* Inflictor);
};
