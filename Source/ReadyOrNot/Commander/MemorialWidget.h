// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MemorialWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class READYORNOT_API UMemorialWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<class URosterCharacter*>& GetMemorialCharacters() { return MemorialCharacters; }

	UFUNCTION(BlueprintCallable)
	void CloseMemorialWidget();

private:
	UPROPERTY(Transient)
	TArray<class URosterCharacter*> MemorialCharacters;
};
