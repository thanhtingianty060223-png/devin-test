// © Void Interactive, 2017

#pragma once

#include "UObject/Interface.h"
#include "ListenForGameStart.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UListenForGameStart : public UInterface
{
	GENERATED_BODY()
};

// IListenForGameStart::OnGameStarted occurs after all of the spawning has executed
class READYORNOT_API IListenForGameStart
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ListenForGameStart")
		void OnGameStarted();
};