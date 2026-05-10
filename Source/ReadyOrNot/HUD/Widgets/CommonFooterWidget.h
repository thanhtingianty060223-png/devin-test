// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonFooterWidget.generated.h"

// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDismissInputAction);

/**
 * A generic footer component for HUD widgets.
 */
UCLASS()
class READYORNOT_API UCommonFooterWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Dismiss Input Action"))
	void BP_OnDismissInputAction();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Dismiss Input Action Progress"))
	void BP_DismissInputActionProgress(float HeldTime);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void RefreshWidget();

	/** Get the text to display in the footer. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetFooterText() const;

	UPROPERTY(BlueprintAssignable)
	FOnDismissInputAction OnDismissInputAction;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bind the dismiss input action. */
	virtual void BindDismissInputAction();

	/** Unbind the dismiss input action. */
	virtual void UnbindDismissInputAction();

	/** Helper function to bind to input method change events */
	virtual void BindInputMethodChangedDelegate();

	/** Helper function to unbind from input method change events */
	virtual void UnbindInputMethodChangedDelegate();

	/** Called via delegate when the input method changes */
	UFUNCTION()
	virtual void OnInputMethodChanged(const ECommonInputType InputMethod);

	/** Called via delegate when the dismiss input action is triggered */
	UFUNCTION()
	virtual void OnDismissInputActionNative();

	/** Handle held input actions for the toggle action. */
	UFUNCTION()
	virtual void OnDismissInputActionProgressNative(float HeldPercent);

private:
	/** Get the base format for the footer text. */
	FText GetBaseFormat(const FCommonInputActionDataBase* DismissInputActionData, const FCommonInputTypeInfo& DismissInputInfo) const;

	FText GetInputKey(const FCommonInputTypeInfo& DismissInputInfo) const;

protected:
	/** The common input action data for dismissing the widget. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (RowType = CommonInputActionDataBase))
	FDataTableRowHandle DismissInputActionSettings;

	/** Whether or not this widget should consume input or allow it to pass through. */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bConsumeInput = false;

	/**
	 * DANGER! Be very, very careful with this. Unless you absolutely know what you're doing, this is not the property you're looking for.
	 *
	 * True to register the action bound to this button as a "persistent" binding. False (default) will register a standard activation-based binding.
	 * A persistent binding ignores the standard ruleset for UI input routing - the binding will be live immediately upon construction of the button.
	 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bIsPersistent = false;

	/** Whether or not to override the input action text. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	bool bOverrideInputActionText = false;

	/** The text to display for the input action. (if overriden) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "bOverrideInputActionText"))
	FText OverrideInputActionText;

private:
	/** The input action handle for the dismiss action. */
	FUIActionBindingHandle DismissInputActionHandle;

	/** Whether or not a gamepad is being used. */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	bool bUsingGamepad = false;
};
