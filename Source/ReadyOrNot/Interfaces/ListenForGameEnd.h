// Void Interactive, 2020

#pragma once

#include "UObject/Interface.h"
#include "ListenForGameEnd.generated.h"

UINTERFACE(MinimalAPI)
class UListenForGameEnd : public UInterface
{
	GENERATED_BODY()
};

// IListenForGameEnd::OnGameEnded occurs after the match has ended
class READYORNOT_API IListenForGameEnd
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ListenForGameEnd")
    void OnGameEnded();
};
