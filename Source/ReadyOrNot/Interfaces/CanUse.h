// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CanUse.generated.h"

/**
 * 
 */
UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UCanUse : public UInterface
{
	GENERATED_BODY()
};

// TODO: Delete
class READYORNOT_API ICanUse
{
	GENERATED_BODY()

public:
	///////////////////////////////////////////////////
	//
	//	Basic stuff

	// Returns true if we can use this thing now
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse")
	bool CanUse(class APlayerCharacter* User);

	// If this returns false, this item should not be considered for trace
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse")
	bool IsAvailableForUse();

	// Returns true if we can use this thing now
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse")
	bool StartUse(class APlayerCharacter* User);

	// Stopped holding use
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse")
	void EndUse(class APlayerCharacter* User);

	///////////////////////////////////////////////////
	//
	//	User interface stuff

	// If true, override the button prompt text (it will use "TO INTERACT" instead)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	bool OverridesUseButtonPromptText();

	// If true, this uses "HOLD" instead of "PRESS" in the button prompt
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	bool UsesHoldButtonPrompt();

	// Get the text used for the button prompt
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	FText GetUseButtonPromptText();

	// If true, plays the icon complete animation when used
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	bool PlaysUseIconComplete();

	// Returns the component to bolt the icon to
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	USceneComponent* GetUseIconBoltComponent();

	// Returns the components we have to be looking at in order to use this object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|CanUse|User Interface")
	TArray<USceneComponent*> GetUseViewComponents();
	
};

