// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DebugDisplayWidget.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UDebugDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDebugTitle(const FString& NewTitle);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	int32 AddDebugText(const FString& NewText);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDebugText(int32 ID, const FString& NewText);
};
