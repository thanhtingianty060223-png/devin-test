// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RosterManager.h"
#include "RosterReviewWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API URosterReviewWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable)
	TArray<URosterCharacter*> GetAllCharacters();
	
	UFUNCTION(BlueprintCallable)
	TArray<URosterCharacter*> GetCharactersSortedByReviewScore() const;

	UFUNCTION(BlueprintCallable)
	void OpenRoster();

	UFUNCTION(BlueprintCallable)
	FText GetTherapistReminderPrompt();
	
	UFUNCTION(BlueprintImplementableEvent)
	void AddSquadCharacters(const TMap<ERosterSquadPosition, URosterCharacter*>& Characters);
	
	UFUNCTION(BlueprintImplementableEvent)
	void AddRemovedCharacters(const TArray<URosterCharacter*>& Characters);

	UFUNCTION(BlueprintImplementableEvent)
	void AddIncapacitatedCharacters(const TArray<URosterCharacter*>& Characters);

	UFUNCTION(BlueprintImplementableEvent)
	void AddReturningCharacters(const TArray<URosterCharacter*>& Characters);
	
private:
	UPROPERTY()
	URosterManager* RosterManager;
};
