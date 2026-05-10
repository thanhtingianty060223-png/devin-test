// Copyright Void Interactive, 2023

#include "Gamepad/ControlsBind.h"
#include "Components/TextBlock.h"
#include "GameFramework/InputSettings.h"
#include "SettingsMenuGamepad.h"

FText UControlsBind::GetConflictingKeybindText(FText RequiredKeybindName)
{
	// Get name of key to display in keybind conflict popup
	FText ConflictingBindFormat = FText::FromString("{0} is currently bound to {1}. This action cannot be unbound. Please assign a new key to {1}.");
	ConflictingBindFormat.Format(ConflictingBindFormat, RequiredKeybindName, PendingNewKey.GetDisplayName());
	return ConflictingBindFormat;
}

void UControlsBind::ResetBinding()
{
	// Reset key in mapping
	BindingText->SetText(UnboundText);

	UInputSettings*	InputSettings = UInputSettings::GetInputSettings();

	if (IsAxis)
	{
		bool Success = false;
		UInputRemappingNodes::RemoveAxisMapping(AxisMappingData, Success);
		AxisMappingData.Key = FKey();
	}
	else
	{
		bool Success = false;
		UInputRemappingNodes::RemoveActionMapping(ActionMappingData, Success);
		ActionMappingData.Key = FKey();
	}

	InputSettings->SaveKeyMappings();
	InputSettings->ForceRebuildKeymaps();
	
}

void UControlsBind::SetKeybindingText()
{
	// Set binding text to readable key name in menu
	if (IsAxis)
	{
		FString KeyName = UReadyOrNotFunctionLibrary::ConvertUnrealKeyNameToRonKeyName(AxisMappingData.Key);

		if (KeyName == "none")
		{
			//ResetBinding();
			BindingText->SetText(UnboundText);
		}
		else
		{
			BindingText->SetText(FText::FromString(KeyName));
		}
	}
	else
	{
		FString KeyName = UReadyOrNotFunctionLibrary::ConvertUnrealKeyNameToRonKeyName(ActionMappingData.Key);
		if (KeyName == "none")
		{
			//ResetBinding();
			BindingText->SetText(UnboundText);
		}
		else
		{
			BindingText->SetText(FText::FromString(KeyName));
		}
	}
}

void UControlsBind::BindingFinished()
{
	ListeningForKeystrokes = false;
	Button_Selected->SetVisibility(ESlateVisibility::Hidden);
	DescriptionWidget->SetVisibility(ESlateVisibility::Visible);
	BindNotificationWidget->SetVisibility(ESlateVisibility::Hidden);
	SetKeybindingText();
}

void UControlsBind::AddKeybindsToList(TArray<FKeyBinding> Keybinds, FName MappingName, float Scale, TArray<FAxisMappingStruct>& AxesList, TArray<FActionMappingStruct>& ActionsList)
{
	// Loop through keybinds to check if bind already exists, add them to list of conflicting binds
	for (FKeyBinding Keybind : Keybinds)
	{
		if ((MappingName == Keybind.BindingName) && !(Keybind.FriendlyName.EqualTo(BindFriendlyName)))
		{
			if (Keybind.Axis)
			{
				if (Scale == Keybind.AxisScale)
				{
					FAxisMappingStruct NewAxisMapping = FAxisMappingStruct(Keybind.BindingName, PendingNewKey, Keybind.AxisScale);
					AxesList.Add(NewAxisMapping);
				}
			}
			else
			{
				FActionMappingStruct NewActionMapping = FActionMappingStruct(Keybind.BindingName, PendingNewKey, false, false, false, false);
				ActionsList.Add(NewActionMapping);
			}
			ConflictBindFriendlyNamesList.Add(Keybind.FriendlyName);
		}
	}
}

void UControlsBind::AddConflictingKeybindsToList(FName MappingName, float Scale, TArray<FAxisMappingStruct>& AxesList, TArray<FActionMappingStruct>& ActionsList)
{
	// Go through keybindstructs in set category and check for conflicts
	switch (KeyCategory)
	{
		case EInputKeyCategoryV2::KE_Shared:
			AddKeybindsToList(SettingsMenu->CharacterControls, MappingName, Scale, AxesList, ActionsList);
			AddKeybindsToList(SettingsMenu->DroneControls, MappingName, Scale, AxesList, ActionsList);
			AddKeybindsToList(SettingsMenu->SharedControls, MappingName, Scale, AxesList, ActionsList);
			break;
		case EInputKeyCategoryV2::KE_Character:
			AddKeybindsToList(SettingsMenu->CharacterControls, MappingName, Scale, AxesList, ActionsList);
			AddKeybindsToList(SettingsMenu->SharedControls, MappingName, Scale, AxesList, ActionsList);
			break;
		case EInputKeyCategoryV2::KE_Drone:
			AddKeybindsToList(SettingsMenu->DroneControls, MappingName, Scale, AxesList, ActionsList);
			AddKeybindsToList(SettingsMenu->SharedControls, MappingName, Scale, AxesList, ActionsList);
			break;
		default:
			break;
	}
}

void UControlsBind::GetConflictingKeybindsWhenValid(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList, bool& HasConflict, TArray<FAxisMappingStruct>& ConflictingAxes, TArray<FActionMappingStruct>& ConflictingActions)
{
	// Loop through all mappings and check if name + scale is already bound
	TArray<FAxisMappingStruct>	 OutputAxes;
	TArray<FActionMappingStruct> OutputActions;

	ConflictBindFriendlyNamesList.Empty();
	for (FAxisMappingStruct AxisMapping : AxesList)
	{
		AddConflictingKeybindsToList(AxisMapping.MappingName, AxisMapping.Scale, OutputAxes, OutputActions);
	}
	for (FActionMappingStruct ActionMapping : ActionsList)
	{
		AddConflictingKeybindsToList(ActionMapping.MappingName, 1.0, OutputAxes, OutputActions);
	}
	ConflictingAxes = OutputAxes;
	ConflictingActions = OutputActions;

	HasConflict = (OutputAxes.Num() > 0 || OutputActions.Num() > 0);
}

FText UControlsBind::CheckConflictingKeybindsAreUnbindable(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList)
{
	// Loop through keybinds/mappings to check if they exist in the UnbindableControls list
	for (FAxisMappingStruct AxisMapping : AxesList)
	{
		for (FKeyBinding Keybind : SettingsMenu->UnbindableControls)
		{
			if ((Keybind.BindingName == AxisMapping.MappingName) && (Keybind.AxisScale == AxisMapping.Scale))
			{
				BindFriendlyName = Keybind.FriendlyName;
				return BindFriendlyName;
			}
		}
	}
	for (FActionMappingStruct ActionMapping : ActionsList)
	{
		for (FKeyBinding Keybind : SettingsMenu->UnbindableControls)
		{
			if (Keybind.BindingName == ActionMapping.MappingName)
			{
				BindFriendlyName = Keybind.FriendlyName;
				return BindFriendlyName;
			}
		}
	}
	BindFriendlyName = FText::FromString("");
	return BindFriendlyName;
}

void UControlsBind::RemoveConflictKeybinds(TArray<FAxisMappingStruct> AxesList, TArray<FActionMappingStruct> ActionsList)
{
	// For each mapping, loop through the controlsbinds and check if the mapping already exists in it, then remove that mapping
	for (FAxisMappingStruct AxisMapping : AxesList)
	{
		FAxisMappingStruct	   TempAxisMapping = AxisMapping;
		UControlsBind*		   TempAxisToRemove = nullptr;
		TArray<UControlsBind*> AxesBinds;
		SettingsMenu->AxesControlBinds.GenerateKeyArray(AxesBinds);
		for (UControlsBind* ControlBinds : AxesBinds)
		{
			if (ControlBinds->AxisMappingData == TempAxisMapping)
			{
				TempAxisToRemove = ControlBinds;
				break;
			}
		}
		if (TempAxisToRemove != nullptr)
		{
			bool Success;
			UInputRemappingNodes::RemoveAxisMapping(TempAxisMapping, Success);
			ResetBinding();
			SettingsMenu->AxesControlBinds.Remove(TempAxisToRemove);
			TempAxisToRemove = nullptr;
		}
	}

	for (FActionMappingStruct ActionMapping : ActionsList)
	{
		FActionMappingStruct   TempActionMapping = ActionMapping;
		UControlsBind*		   TempActionToRemove = nullptr;
		TArray<UControlsBind*> ActionBinds;
		SettingsMenu->ActionsControlBinds.GenerateKeyArray(ActionBinds);
		for (UControlsBind* ControlBinds : ActionBinds)
		{
			if (ControlBinds->ActionMappingData == TempActionMapping)
			{
				TempActionToRemove = ControlBinds;
				break;
			}
		}
		if (TempActionToRemove != nullptr)
		{
			bool Success;
			UInputRemappingNodes::RemoveActionMapping(TempActionMapping, Success);
			ResetBinding();
			SettingsMenu->ActionsControlBinds.Remove(TempActionToRemove);
			TempActionToRemove = nullptr;
		}
	}
}

void UControlsBind::GetInputBindingData(int Index, FAxisMappingStruct& AxisMapping, FActionMappingStruct& ActionMapping)
{
	// Get array of all mappings. If there aren't any mappings in the array, set the binding text to N/A and return a mapping without a key 
	// else, return the mapping at [Index] in the array
	bool Success;

	if (IsAxis)
	{
		FAxisMappingStruct FilterData;
		FilterData.MappingName = BindName;
		FilterData.Scale = AxisScale;
		TArray<EAxisMappingFilter> Filters = TArray<EAxisMappingFilter>{ EAxisMappingFilter::Name, EAxisMappingFilter::Scale, EAxisMappingFilter::IsNotGamepad };
		TArray<FAxisMappingStruct> TempAxisMappings;
		UInputRemappingNodes::GetAllAxisMappings(FilterData, Filters, Success, TempAxisMappings);
		if (Success)
		{
			if (Index >= TempAxisMappings.Num())
			{
				BindingText->SetText(UnboundText);
				AxisMapping = FilterData;
				return;
			}
			AxisMapping = TempAxisMappings[Index];
			return;
		}
		return;
	}

	FActionMappingStruct FilterData;
	FilterData.MappingName = BindName;
	TArray<EActionMappingFilter> Filters = TArray<EActionMappingFilter>{ EActionMappingFilter::Name };
	TArray<FActionMappingStruct> TempActionMappings;
	UInputRemappingNodes::GetAllActionMappings(FilterData, Filters, Success, TempActionMappings);
	if (Success)
	{
		if (Index >= TempActionMappings.Num())
		{
			BindingText->SetText(UnboundText);
			ActionMapping = FilterData;
			return;
		}
		ActionMapping = TempActionMappings[Index];
		return;
	}
}

bool UControlsBind::RebindKey(FKey NewKey)
{
	// Get input data and check if the keys are the same for index 0 & 1
	// If not, create a mapping with the incoming key and add it to input settings
	// Replace the old mapping with the new one and check if this ControlsBind already exists in the ControlsBind map
	// If yes, check that the mapping key isn't null and add this ControlsBind & the mapping to the ControlsBind map
	FAxisMappingStruct	 FirstAxisMapping;
	FActionMappingStruct FirstActionMapping;
	FAxisMappingStruct	 SecondAxisMapping;
	FActionMappingStruct SecondActionMapping;
	bool				 Success;
	UInputSettings*		 InputSettings = UInputSettings::GetInputSettings();

	GetInputBindingData(BindIndex, FirstAxisMapping, FirstActionMapping);
	int OtherBindIndex = (BindIndex == 0) ? 1 : 0;
	GetInputBindingData(OtherBindIndex, SecondAxisMapping, SecondActionMapping);

	if (IsAxis)
	{
		if (FirstAxisMapping.Key == NewKey || SecondAxisMapping.Key == NewKey)
		{
			return false;
		}
		
		FAxisMappingStruct NewAxisMapping = AxisMappingData;
		NewAxisMapping.Key = NewKey;
	
		FInputAxisKeyMapping OldAxisKeyMapping = FInputAxisKeyMapping(AxisMappingData.MappingName, AxisMappingData.Key, AxisMappingData.Scale);
		FInputAxisKeyMapping NewAxisKeyMapping = FInputAxisKeyMapping(NewAxisMapping.MappingName, NewAxisMapping.Key, AxisMappingData.Scale);

		UInputRemappingNodes::RemoveAxisMapping(AxisMappingData, Success);
		UInputRemappingNodes::CreateNewAxisMapping(NewAxisMapping, Success);

		InputSettings->SaveKeyMappings();
		InputSettings->ForceRebuildKeymaps();
		
		AxisMappingData = NewAxisMapping;
		if (SettingsMenu->AxesControlBinds.Contains(this))
		{
			if (!AxisMappingData.Key.IsValid())
			{
				SettingsMenu->AxesControlBinds.Add(this, AxisMappingData);
				return true;
			}
		}
		return true;
	}

	if (FirstActionMapping.Key == NewKey || SecondActionMapping.Key == NewKey)
	{
		return false;
	}

	FActionMappingStruct NewActionMapping = ActionMappingData;
	NewActionMapping.Key = NewKey;
	
	FInputActionKeyMapping OldActionKeyMapping = FInputActionKeyMapping(ActionMappingData.MappingName, ActionMappingData.Key, false, false, false, false);
	FInputActionKeyMapping NewActionKeyMapping = FInputActionKeyMapping(NewActionMapping.MappingName, NewActionMapping.Key, false, false, false, false);

	UInputRemappingNodes::RemoveActionMapping(ActionMappingData, Success);
	UInputRemappingNodes::CreateNewActionMapping(NewActionMapping, Success);

	InputSettings->SaveKeyMappings();
	InputSettings->ForceRebuildKeymaps();
	
	ActionMappingData = NewActionMapping;

	if (SettingsMenu->ActionsControlBinds.Contains(this))
	{
		if (!ActionMappingData.Key.IsValid())
		{
			SettingsMenu->ActionsControlBinds.Add(this, ActionMappingData);
			return true;
		}
	}
	return true;
}
