// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "PlayerActionPromptWidget.generated.h"

/**
 * A text that appears on the player's HUD when focusing on interactable actors in the world
 */
UCLASS()
class READYORNOT_API UPlayerActionPromptWidget : public UBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Player Action Prompt")
	void UpdateActionSlot(const FText& InText , bool clearText, bool bAnimate = true, bool bLoopAnimation = false);

	UFUNCTION(BlueprintCallable, Category = "Player Action Prompt")
	void UpdateText(const FText& InText, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Player Action Prompt")
	void ClearText();

	UFUNCTION(BlueprintPure, Category = "Player Action Prompt")
	FORCEINLINE bool IsInUse() const { return bInUse; }

protected:
	void NativePreConstruct() override;
	void NativeConstruct() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Action Prompt|Exposed Properties", meta = (ExposeOnSpawn = true))
	FText ActionText = FText::FromString("Press <Red>[Key]</> to <Red>[Action]</>");
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Action Prompt|Required Widgets", meta = (BindWidget), Transient)
	class URichTextBlock* Action_RichText = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Action Prompt|Required Widget Anims", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Anim_OnShow = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Player Action Prompt|Data")
	uint8 bInUse : 1;
};
