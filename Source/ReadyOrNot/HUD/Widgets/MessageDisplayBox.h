// Copyright Void Interactive, 2017

#pragma once

#include "Blueprint/UserWidget.h"
#include "MessageDisplayBox.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UMessageDisplayBox : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = Text, Meta = (ExposeOnSpawn = true))
	FString MessageTxt = "This is an example message...";
	UPROPERTY(BlueprintReadWrite, Category = Text, Meta = (ExposeOnSpawn = true))
	FString ButtonTxt = "OK";
	UPROPERTY(BlueprintReadWrite, Category = Text, Meta = (ExposeOnSpawn = true))
	bool bShouldQuitOnButtonPress = false;
	
	
};
