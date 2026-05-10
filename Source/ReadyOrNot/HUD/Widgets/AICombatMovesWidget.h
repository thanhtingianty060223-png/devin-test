// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "AICombatMovesWidgetEntry.h"
#include "UObject/Object.h"
#include "AICombatMovesWidget.generated.h"

UCLASS()
class READYORNOT_API UAICombatMovesWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AI Debug Widget", meta = (BindWidget), Transient)
	class UVerticalBox* CombatMoves_VerticalBox = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "AI Debug Widget", meta = (BindWidget), Transient)
	class UTextBlock* AIName_TextBlock = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "AI Debug Widget", meta = (BindWidget), Transient)
	class UButton* NextAI_Button = nullptr;

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void SetAIToFocus(ACyberneticCharacter* CyberneticCharacter);

	UFUNCTION(BlueprintCallable)
	void OnNextAIButtonClicked();

	UPROPERTY(BlueprintReadWrite, Category = "Combat Moves Widget")
	ACyberneticCharacter* CurrentAI = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAICombatMovesWidgetEntry> WidgetEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UUserWidget> AIWorldWidgetClass;
};