// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "SettingsMenuGamepad.h"
#include "InputRemappingNodes.h"
#include "ControlsBind.generated.h"

UCLASS()
class READYORNOT_API UControlsBind : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	bool IsAxis = false;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	int BindIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	bool ListeningForKeystrokes = false;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	FKey PendingNewKey;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	class UTextBlock* DescriptionWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	class UTextBlock* BindNotificationWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	FName BindName;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	FText BindFriendlyName = FText::GetEmpty();

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	float AxisScale = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	EInputKeyCategoryV2 KeyCategory = EInputKeyCategoryV2::KE_Character;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind", meta = (ExposeOnSpawn = "true"))
	class USettingsMenuGamepad* SettingsMenu;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	FAxisMappingStruct AxisMappingData;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	FActionMappingStruct ActionMappingData;

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	FText UnboundText = FText::FromString("N/A");

	UPROPERTY(BlueprintReadWrite, Category = "ControlsBind")
	TArray<FText> ConflictBindFriendlyNamesList;

	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	FText GetConflictingKeybindText(FText RequiredKeybindName);

	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void ResetBinding();

	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void SetKeybindingText();

	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void BindingFinished();

	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void AddKeybindsToList(TArray<FKeyBinding> Keybinds, FName MappingName, float Scale, TArray<FAxisMappingStruct>& AxesList, TArray<FActionMappingStruct>& ActionsList);
	
	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void AddConflictingKeybindsToList(FName MappingName, float Scale, TArray<FAxisMappingStruct>& AxesList, TArray<FActionMappingStruct>& ActionsList);
	
	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void GetConflictingKeybindsWhenValid(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList, bool& HasConflict, TArray<FAxisMappingStruct>& ConflictingAxes, TArray<FActionMappingStruct>& ConflictingActions);
	
	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	FText CheckConflictingKeybindsAreUnbindable(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList);
	
	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	void RemoveConflictKeybinds(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList);
	
	UFUNCTION(BlueprintPure, Category = "ControlsBind")
	void GetInputBindingData(int Index, FAxisMappingStruct& AxisMapping, FActionMappingStruct& ActionMapping);
	
	UFUNCTION(BlueprintCallable, Category = "ControlsBind")
	bool RebindKey(FKey NewKey);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "ControlsBind", meta = (BindWidget))
	class UTextBlock* BindingText;

	UPROPERTY(BlueprintReadOnly, Category = "ControlsBind", meta = (BindWidget))
	class UImage* Button_Selected;
};

//bool operator!=(const UControlsBind& A, const UControlsBind& B) {
//	return (A.IsAxis != B.IsAxis) && (A.PendingNewKey != B.PendingNewKey) && !(A.BindFriendlyName.EqualTo(B.BindFriendlyName)) && (A.KeyCategory != B.KeyCategory) && (A.AxisMappingData != B.AxisMappingData) && (A.ActionMappingData != B.ActionMappingData);
//};