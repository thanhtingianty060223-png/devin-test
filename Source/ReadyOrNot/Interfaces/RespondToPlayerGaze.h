// Copyright Void Interactive, 2017

#pragma once

#include "UObject/Interface.h"
#include "RespondToPlayerGaze.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API URespondToPlayerGaze : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IRespondToPlayerGaze
{
	GENERATED_BODY()

public:
	// Executed client side, on the local client only.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interfaces|RespondToPlayerGaze")
	void OnPlayerGazeStarted(class APlayerCharacter* Gazer);

	// Executed client side, on the local client only.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interfaces|RespondToPlayerGaze")
	void OnPlayerGazeEnded(class APlayerCharacter* Gazer);

};
