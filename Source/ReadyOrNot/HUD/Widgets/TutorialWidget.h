// Copyright Void Interactive, 2023

#pragma once

#include "Structs.h"
#include "CommonActivatableWidget.h"
#include "TutorialWidget.generated.h"

UCLASS()
class READYORNOT_API UTutorialWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** Show the tooltip content. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowMainWidget();

	/** Hide the tooltip content. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HideMainWidget();

	/** Refresh the tooltip content. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void RefreshWidget();

	/** Set the tutorial content. */
	UFUNCTION(BlueprintCallable)
	void SetData(const FTutorialWidgetData& InData);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Helper function to bind to input method change events */
	virtual void BindInputMethodChangedDelegate();

	/** Helper function to unbind from input method change events */
	virtual void UnbindInputMethodChangedDelegate();

	/** Delegate for when the input method changes. */
	UFUNCTION()
	virtual void OnInputMethodChanged(const ECommonInputType InputMethod);

private:
	/** Get the title text. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetTitle() const;

	/** Generate a rich text description given the current input type and provided input actions. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetDescription() const;

protected:
	/** Whether or not to show the footer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowFooter = true;

	/** Whether or not a gamepad is being used. */
	UPROPERTY(BlueprintReadOnly)
	bool bUsingGamepad = false;

private:
	/** The tooltip content. */
	FTutorialWidgetData Data;
};
