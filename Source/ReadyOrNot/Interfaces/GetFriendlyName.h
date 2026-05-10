// Copyright Void Interactive, 2017

#pragma once

#include "UObject/Interface.h"
#include "GetFriendlyName.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UGetFriendlyName : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IGetFriendlyName
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|GetFriendlyName")
	FString GetFriendlyName();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|GetFriendlyName")
	UTexture2D* GetFriendlyIcon();
};
