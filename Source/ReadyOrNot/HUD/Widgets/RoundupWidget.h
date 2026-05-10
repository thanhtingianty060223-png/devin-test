// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RoundupWidget.generated.h"

class UMetaGameProfile;
class UCustomizationDataBase;
class ULoadoutCustomization;
class UCommanderProfile;
class URosterCharacter;
class URosterManager;

UCLASS()
class READYORNOT_API URoundupWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** Show and activate the widget. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowWidget();

	/** Hide and deactivate the widget. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void HideWidget();

	/** Toggle the main contents collapsed/expanded. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ToggleContent();

	/** Collapse the main contents. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CollapseContent();

	/** Expand the main contents. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ExpandContent();

	/** Hide unlock notifications section. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void HideRoundupUnlocks();

	/** Add a new unlock notification to the widget. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void AddRoundupUnlock(const FText& Unlock);

	/** Hide action notifications section. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void HideRoundupActions();

	/** Add a new action notification to the widget. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void AddRoundupAction(const FText& Action);

	/** Refresh the contextual UI elements. (not content) */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void RefreshWidget();

protected:
	/** Setup the roundup unlocks and actions. */
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	/** Helper function to bind to input method change events */
	virtual void BindInputMethodChangedDelegate();

	/** Helper function to unbind from input method change events */
	virtual void UnbindInputMethodChangedDelegate();

	/** Called via delegate when the input method changes */
	UFUNCTION()
	virtual void OnInputMethodChanged(const ECommonInputType InputMethod);

private:
	/** Add a new unlock notification. */
	void AddUnlock(const FText& Unlock);

	/** Add a new action notification. */
	void AddAction(const FText& Action);

// Commander Mode Notifications
public:
	/** Whether or not we have loaded an existing or new commander mode save. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsNewCommanderModeSave() const;

protected:
	virtual void SetupNewCommanderModeSaveNotification();

// Unlock Notifications
protected:
	/** Check for and setup any new unlocked items. */
	virtual void SetupUnlockNotifications();

private:
	/** Add an unlock notification if evidence was unlocked */
	void CheckIfEvidenceUnlocked();

	/** Add an unlock notification if a customization item was unlocked */
	void CheckIfCustomizationUnlocked();

	/** Add an unlock notification if a mission was unlocked */
	void CheckIfMissionUnlocked();

	/** Add an unlock notification if a roster slot was unlocked */
	void CheckIfRosterSlotUnlocked();

	/** Check if the player has new tags to have unlocked an item. */
	bool IsNewUnlock(const TArray<FName>& ItemRequiredTags) const;

// Action Notifications
protected:
	/** Check for and setup any officer updates. */
	virtual void SetupActionNotifications();

private:
	/** Add an action notification if emotional stress increased */
	void CheckIfEmotionalStressIncreased();

	/** Add an action notification if a previous officer's status changed */
	void CheckIfPreviousOfficersStatusChanged();

	/** Add an action notification if a current officer's status changed */
	void CheckIfCurrentOfficersStatusChanged();

	/** Add an action notification if mission ended due to exfil */
	void CheckIfExfiltratedFromMission();

public:
	/** Whether or not the roundup is collapsed or expanded. */
	UPROPERTY(BlueprintReadWrite)
	bool bIsCollapsed = false;

protected:
	/** Unlock notifications to display in the roundup. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FText> Unlocks;

	/** Action notifications to display in the roundup. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FText> Actions;

	/** Whether or not a gamepad is being used. */
	UPROPERTY(BlueprintReadOnly)
	bool bUsingGamepad = false;

private:
	/** The meta game profile for the local player. */
	UPROPERTY()
	UMetaGameProfile* MetaGameProfile;

	/** The current commander profile for the local player. */
	UPROPERTY()
	UCommanderProfile* CommanderProfile;

	/** The roster manager for the local player. */
	UPROPERTY()
	URosterManager* RosterManager;

	/** String table ID for the localization. */
	const FName DebriefStringTableId = "/Game/Localization/Game/en/loc_Debrief.loc_Debrief";
};
