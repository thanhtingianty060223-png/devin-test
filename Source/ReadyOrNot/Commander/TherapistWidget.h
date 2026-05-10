// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TherapistWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UTherapistWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<class URosterCharacter*>& GetCharactersInTherapy() { return CharactersInTherapy; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetTherapistText();
	
	UFUNCTION(BlueprintCallable)
	void CloseTherapistWidget();

private:
	UPROPERTY(Transient)
	TArray<class URosterCharacter*> CharactersInTherapy;
};
