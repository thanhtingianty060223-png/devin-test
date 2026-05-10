// Copyright Void Interactive, 2022

#include "AI/Archetypes/AIArchetypeData.h"

#include "ReadyOrNotAIConfig.h"
#include "AI/AIAction.h"
#include "AI/AIActionPresetData.h"
#include "Characters/CyberneticController.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Archetype ~ Cooldown Tick"), STAT_AIArchetype_CooldownTick, STATGROUP_AIArchetype);

void RemoveAIActionData_Internal(FAIActionData& Action, ACyberneticController* AIController)
{
	for (FAIActionGateData& Gate : Action.Gates)
	{
		Gate.Cooldowns.Remove(AIController);

		#if WITH_EDITORONLY_DATA
		Gate.IsOpen.Remove(AIController);
		#endif
	}
	
	for (FAIActionConsiderationData& C : Action.Considerations)
	{
		C.Scores.Remove(AIController);
	}

	if (UAIAction** CustomActionPtr = Action.CustomActions.Find(AIController))
	{
		if (*CustomActionPtr)
		{
			(*CustomActionPtr)->ConditionalBeginDestroy();
			(*CustomActionPtr) = nullptr;
		}
	}
	
	Action.Scores.Remove(AIController);
	Action.Cooldowns.Remove(AIController);
	Action.CommitTimes.Remove(AIController);
	Action.CustomActions.Remove(AIController);

	Action.RunCount.Remove(AIController);

	Action.SuccessConsiderCount.Remove(AIController);
	Action.FailConsiderCount.Remove(AIController);
}

void RemoveAIData_Internal(TArray<FAIActionDataContainer>& Actions, ACyberneticController* AIController)
{
	for (FAIActionDataContainer& Data : Actions)
	{
		RemoveAIActionData_Internal(Data.GetActionData(), AIController);
	}
}

void ClearActionData_Internal(FAIActionData& Action)
{
	for (FAIActionGateData& Gate : Action.Gates)
	{
		Gate.Cooldowns.Empty();

		#if WITH_EDITORONLY_DATA
		Gate.IsOpen.Empty();
		#endif
	}
	
	for (FAIActionConsiderationData& C : Action.Considerations)
	{
		C.Scores.Empty();
	}

	for (auto& It : Action.CustomActions)
	{
		if (It.Value)
		{
			It.Value->ConditionalBeginDestroy();
			It.Value = nullptr;
		}
	}

	Action.Scores.Empty();
	Action.Cooldowns.Empty();
	Action.CommitTimes.Empty();
	Action.CustomActions.Empty();

	Action.RunCount.Empty();
	
	Action.SuccessConsiderCount.Empty();
	Action.FailConsiderCount.Empty();
}

void ClearTransientData_Internal(TArray<FAIActionDataContainer>& Actions)
{
	for (FAIActionDataContainer& Data : Actions)
	{
		ClearActionData_Internal(Data.GetActionData());
	}
}

UAIArchetypeData::UAIArchetypeData()
{
	bSuppressionRequiresLOSOnLastKnownEnemyPosition = true;
}

void UAIArchetypeData::AuditActions()
{
	const auto IsActionTypeCombatMove = [](FAIActionDataContainer& Action)
	{
		const FAIActionData& Data = Action.GetActionData();
		return Data.ActionType == EAIAction::Duel ||
				Data.ActionType == EAIAction::Flank ||
				Data.ActionType == EAIAction::Flee ||
				Data.ActionType == EAIAction::Hide ||
				Data.ActionType == EAIAction::HideExit ||
				Data.ActionType == EAIAction::Rush ||
				Data.ActionType == EAIAction::Push ||
				Data.ActionType == EAIAction::Reposition ||
				Data.ActionType == EAIAction::Suppress ||
				Data.ActionType == EAIAction::HardCover;
	};
	
	// remove all combat move actions that are outside of the combat moves list
	UnalertActions.RemoveAll([&](FAIActionDataContainer& Action)
	{
		return IsActionTypeCombatMove(Action);
	});
	
	SuspiciousActions.RemoveAll([&](FAIActionDataContainer& Action)
	{
		return IsActionTypeCombatMove(Action);
	});
	
	AlertActions.RemoveAll([&](FAIActionDataContainer& Action)
	{
		return IsActionTypeCombatMove(Action);
	});

	for (FAITraitActionData& Trait : TraitActions)
	{
		Trait.Actions.RemoveAll([&](FAIActionDataContainer& Action)
		{
			return IsActionTypeCombatMove(Action);
		});
	}

	// use the native gates instead

	/*
	const TSubclassOf<UAGValidTarget> ValidTargetClass = StaticLoadClass(UAGValidTarget::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprints/AI/Gates/AG_ValidTarget.AG_ValidTarget_C'"));

	for (FAIActionDataContainer Action : AlertActions)
	{
		for (FAIActionGateData& G : Action.GetActionData().Gates)
		{
			if (UAGValidTarget* BpValidTarget = Cast<UAGValidTarget>(G.Type))
			{
				if (BpValidTarget->GetClass() == ValidTargetClass)
				{
					UAGValidTarget* NewType = NewObject<UAGValidTarget>(this, UAGValidTarget::StaticClass());
					NewType->bAllowEnemy = BpValidTarget->bAllowEnemy;
					NewType->bAllowFriendly = BpValidTarget->bAllowFriendly;
					NewType->bAllowNeutral = BpValidTarget->bAllowNeutral;
					NewType->bAllowLastTrackedTarget = BpValidTarget->bAllowLastTrackedTarget;
					
					G.Type = NewType;
				}
			}
		}
	}
	*/
}

void UAIArchetypeData::CreateCustomActionsIfNeeded(ACyberneticController* InController)
{
	for (FAIActionDataContainer& Data : AlertActions)
	{
		Data.GetActionData().CreateCustomAction(InController);
	}
	
	for (FAIActionDataContainer& Data : UnalertActions)
	{
		Data.GetActionData().CreateCustomAction(InController);
	}

	for (FAIActionDataContainer& Data : SuspiciousActions)
	{
		Data.GetActionData().CreateCustomAction(InController);
	}
	
	for (FAIActionDataContainer& Data : CombatMoveActions)
	{
		Data.GetActionData().CreateCustomAction(InController);
	}
	
	for (FAIActionDataContainer& Data : ContinuousActionsCache)
	{
		Data.GetActionData().CreateCustomAction(InController);
	}
	
	for (FAITraitActionData& TraitAction : TraitActions)
	{
		for (FAIActionDataContainer& Action : TraitAction.Actions)
		{
			Action.GetActionData().CreateCustomAction(InController);
		}
	}
}

void UAIArchetypeData::ClearTransientData()
{
	ClearTransientData_Internal(AlertActions);
	ClearTransientData_Internal(UnalertActions);
	ClearTransientData_Internal(SuspiciousActions);
	ClearTransientData_Internal(CombatMoveActions);
	ClearTransientData_Internal(ContinuousActionsCache);
	
	for (FAITraitActionData& TraitAction : TraitActions)
	{
		for (FAIActionDataContainer& Action : TraitAction.Actions)
		{
			ClearActionData_Internal(Action.GetActionData());
		}
	}
}

void UAIArchetypeData::RemoveAIData(ACyberneticController* AIController)
{
	RemoveAIData_Internal(AlertActions, AIController);
	RemoveAIData_Internal(UnalertActions, AIController);
	RemoveAIData_Internal(SuspiciousActions, AIController);
	RemoveAIData_Internal(CombatMoveActions, AIController);
	RemoveAIData_Internal(ContinuousActionsCache, AIController);
	
	for (FAITraitActionData& TraitAction : TraitActions)
	{
		for (FAIActionDataContainer& Action : TraitAction.Actions)
		{
			RemoveAIActionData_Internal(Action.GetActionData(), AIController);
		}
	}
}

void UAIArchetypeData::Tick(ACyberneticController* AIController, float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AIArchetype_CooldownTick);
	
	for (FAIActionDataContainer& Data : AlertActions)
	{
		Data.GetActionData().Tick(AIController, DeltaTime);
	}
	
	for (FAIActionDataContainer& Data : UnalertActions)
	{
		Data.GetActionData().Tick(AIController, DeltaTime);
	}

	for (FAIActionDataContainer& Data : SuspiciousActions)
	{
		Data.GetActionData().Tick(AIController, DeltaTime);
	}
	
	for (FAIActionDataContainer& Data : CombatMoveActions)
	{
		Data.GetActionData().Tick(AIController, DeltaTime);
	}
	
	for (FAIActionDataContainer& Data : ContinuousActionsCache)
	{
		Data.GetActionData().Tick(AIController, DeltaTime);
	}
	
	for (FAITraitActionData& TraitAction : TraitActions)
	{
		for (FAIActionDataContainer& Action : TraitAction.Actions)
		{
			Action.GetActionData().Tick(AIController, DeltaTime);
		}
	}
}

FAIActionData* UAIArchetypeData::FindActionByName(TArray<FAIActionData>& InActions, const FName ActionName)
{
	for (FAIActionData& Action : InActions)
	{
		if (Action.Name == ActionName)
		{
			return &Action;
		}
	}

	return nullptr;
}

FAIActionData* UAIArchetypeData::FindActionByType(TArray<FAIActionData>& InActions, EAIAction ActionType)
{
	for (FAIActionData& Action : InActions)
	{
		if (Action.ActionType == ActionType)
		{
			return &Action;
		}
	}

	return nullptr;
}

FAIActionData* UAIArchetypeData::FindActionByCustomType(TArray<FAIActionData>& InActions, TSubclassOf<UAIAction> CustomAction)
{
	for (FAIActionData& Action : InActions)
	{
		if (Action.ActionType == EAIAction::Custom &&
			Action.CustomActionClass == CustomAction)
		{
			return &Action;
		}
	}

	return nullptr;
}

FAIActionData* UAIArchetypeData::FindActionByName(TArray<FAIActionDataContainer>& InActions, FName ActionName)
{
	for (FAIActionDataContainer& Data : InActions)
	{
		FAIActionData& Action = Data.GetActionData();
		if (Action.Name == ActionName)
		{
			return &Action;
		}
	}

	return nullptr;
}

FAIActionData* UAIArchetypeData::FindActionByType(TArray<FAIActionDataContainer>& InActions, EAIAction ActionType)
{
	for (FAIActionDataContainer& Data : InActions)
	{
		FAIActionData& Action = Data.GetActionData();
		if (Action.ActionType == ActionType)
		{
			return &Action;
		}
	}

	return nullptr;
}

FAIActionData* UAIArchetypeData::FindActionByCustomType(TArray<FAIActionDataContainer>& InActions, TSubclassOf<UAIAction> CustomAction)
{
	for (FAIActionDataContainer& Data : InActions)
	{
		FAIActionData& Action = Data.GetActionData();
		if (Action.ActionType == EAIAction::Custom &&
			Action.CustomActionClass == CustomAction)
		{
			return &Action;
		}
	}

	return nullptr;
}

int32 UAIArchetypeData::GetSuccessfulConsiderCountForAction(const ACyberneticController* AIController, TArray<FAIActionDataContainer>& InActions, FName ActionName)
{
	if (FAIActionData* Data = FindActionByName(InActions, ActionName))
	{
		if (const int32* Count = Data->SuccessConsiderCount.Find(AIController))
		{
			return *Count;
		}
	}
	
	return -1;
}

int32 UAIArchetypeData::GetFailedConsiderCountForAction(const class ACyberneticController* AIController, TArray<FAIActionDataContainer>& InActions, FName ActionName)
{
	if (FAIActionData* Data = FindActionByName(InActions, ActionName))
	{
		if (const int32* Count = Data->FailConsiderCount.Find(AIController))
		{
			return *Count;
		}
	}
	
	return -1;
}

void UAIArchetypeData::PostLoad()
{
	Super::PostLoad();
	
	ClearTransientData();

	// Copy over continous actions into their own list
	ContinuousActionsCache = {};
	for (const FAIActionData_NameOnly& ActionName : ContinuousActions)
	{
		for (FAIActionDataContainer& Data : AlertActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.Name == ActionName.Name)
			{
				ContinuousActionsCache.Add(Data);
				break;
			}
		}
	}
}

void UAIArchetypeData::PreSave(const ITargetPlatform* TargetPlatform)
{
	ClearTransientData();
	
	// Copy over continous actions into their own list
	ContinuousActionsCache = {};
	for (const FAIActionData_NameOnly& ActionName : ContinuousActions)
	{
		for (FAIActionDataContainer& Data : AlertActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.Name == ActionName.Name)
			{
				ContinuousActionsCache.Add(Data);
				break;
			}
		}
	}
	
	Super::PreSave(TargetPlatform);
}

#if WITH_EDITOR
void UAIArchetypeData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "Name" || PropertyChangedEvent.GetPropertyName() == "Preset")
	{
		const auto UpdateName = [&](TArray<FAIActionDataContainer>& ActionDataContainer)
		{
			for (FAIActionDataContainer& Action : ActionDataContainer)
			{
				if (Action.Preset)
				{
					Action.Name = *(Action.Preset->Action.Name.ToString() + " (PRESET)");
				}
				else
				{
					Action.Name = Action.Data.Name;
				}
			}
		};

		UpdateName(AlertActions);
		UpdateName(UnalertActions);
		UpdateName(SuspiciousActions);
		UpdateName(CombatMoveActions);

		for (FAITraitActionData& TraitAction : TraitActions)
			UpdateName(TraitAction.Actions);
	}

	if (PropertyChangedEvent.GetPropertyName() == "ActionToAssignPreset")
	{
		AssignActionPreset();
	}

	if (PropertyChangedEvent.GetPropertyName() == "CopyFromArchetype")
	{
		if (CopyFromArchetype)
		{
			Name = CopyFromArchetype->Name;

			bEnableAlertActions = CopyFromArchetype->bEnableAlertActions;
			AlertActions = CopyFromArchetype->AlertActions;

			bEnableUnalertActions = CopyFromArchetype->bEnableUnalertActions;
			UnalertActions = CopyFromArchetype->UnalertActions;

			bEnableSuspiciousActions = CopyFromArchetype->bEnableSuspiciousActions;
			SuspiciousActions = CopyFromArchetype->SuspiciousActions;

			bEnableContinuousActions = CopyFromArchetype->bEnableContinuousActions;
			ContinuousActions = CopyFromArchetype->ContinuousActions;

			bEnableCombatMoveActions = CopyFromArchetype->bEnableCombatMoveActions;
			CombatMoveActions = CopyFromArchetype->CombatMoveActions;
			
			bEnableTraitActions = CopyFromArchetype->bEnableTraitActions;
			TraitActions = CopyFromArchetype->TraitActions;

			ActionsDump = CopyFromArchetype->ActionsDump;

			bOverrideAimSettings = CopyFromArchetype->bOverrideAimSettings;
			FocalPointInterpSpeed = CopyFromArchetype->FocalPointInterpSpeed;
			FocalPointInterpCurve = CopyFromArchetype->FocalPointInterpCurve;
			FocusTurnSpeed = CopyFromArchetype->FocusTurnSpeed;
			TurnDegreesPerSecond = CopyFromArchetype->TurnDegreesPerSecond;
			ActorRotationInterpStandingSpeed = CopyFromArchetype->ActorRotationInterpStandingSpeed;
			ActorRotationInterpMovingSpeed = CopyFromArchetype->ActorRotationInterpMovingSpeed;
			AimOffsetInterpSpeed = CopyFromArchetype->AimOffsetInterpSpeed;

			bIgnoreDamageHitReactions = CopyFromArchetype->bIgnoreDamageHitReactions;
			AllowCombatMoveInState = CopyFromArchetype->AllowCombatMoveInState;
			//bOnlyAllowCombatMovesWhilstAlert = CopyFromArchetype->bOnlyAllowCombatMovesWhilstAlert;
			bAccuracyOverride = CopyFromArchetype->bAccuracyOverride;
			Accuracy = CopyFromArchetype->Accuracy;

			bDisableVO = CopyFromArchetype->bDisableVO;

			Notes = CopyFromArchetype->Notes;

			CopyFromArchetype = nullptr;
		}
	}

	if (PropertyChangedEvent.GetPropertyName() == "bCommitTimeFromConfig" ||
		PropertyChangedEvent.GetPropertyName() == "CommitTimeConfigKey")
	{
		GConfig->LoadFile(FPaths::ProjectConfigDir() + "AILevelData.ini");
		
		for (FAIActionDataContainer& Data : AlertActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.bCommitTimeFromConfig)
			{
				Action.CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CommitTimeConfigKey);
			}

			if (Action.bCooldownFromConfig)
			{
				Action.Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CooldownConfigKey);
			}
		}
		
		for (FAIActionDataContainer& Data : UnalertActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.bCommitTimeFromConfig)
			{
				Action.CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CommitTimeConfigKey);
			}
			
			if (Action.bCooldownFromConfig)
			{
				Action.Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CooldownConfigKey);
			}
		}
		
		for (FAIActionDataContainer& Data : SuspiciousActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.bCommitTimeFromConfig)
			{
				Action.CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CommitTimeConfigKey);
			}
			
			if (Action.bCooldownFromConfig)
			{
				Action.Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CooldownConfigKey);
			}
		}
		
		for (FAIActionDataContainer& Data : CombatMoveActions)
		{
			FAIActionData& Action = Data.GetActionData();
			if (Action.bCommitTimeFromConfig)
			{
				Action.CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CommitTimeConfigKey);
			}
			
			if (Action.bCooldownFromConfig)
			{
				Action.Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CooldownConfigKey);
			}
		}
		
		for (FAITraitActionData& TraitAction : TraitActions)
		{
			for (FAIActionDataContainer& Action : TraitAction.Actions)
			{
				if (Action.GetActionData().bCommitTimeFromConfig)
				{
					Action.GetActionData().CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.GetActionData().CommitTimeConfigKey);
				}
				
				if (Action.GetActionData().bCooldownFromConfig)
				{
					Action.GetActionData().Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.GetActionData().CooldownConfigKey);
				}
			}
		}

		for (FAIActionData& Action : ActionsDump)
		{
			if (Action.bCommitTimeFromConfig)
			{
				Action.CommitTime = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CommitTimeConfigKey);
			}
			
			if (Action.bCooldownFromConfig)
			{
				Action.Cooldown = UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + "AILevelData.ini", "Global", Action.CooldownConfigKey);
			}
		}
	}
}
#endif

#if WITH_EDITORONLY_DATA
void UAIArchetypeData::AssignActionPreset()
{
	if (ActionToAssignPreset == NAME_None)
		return;
	
	for (FAIActionDataContainer& Data : AlertActions)
	{
		FAIActionData& Action = Data.Data;
		if (Action.Name == ActionToAssignPreset)
		{
			Data.Preset = PresetToAssign;
			break;
		}
	}
	
	for (FAIActionDataContainer& Data : UnalertActions)
	{
		FAIActionData& Action = Data.Data;
		if (Action.Name == ActionToAssignPreset)
		{
			Data.Preset = PresetToAssign;
			break;
		}
	}
	
	for (FAIActionDataContainer& Data : SuspiciousActions)
	{
		FAIActionData& Action = Data.Data;
		if (Action.Name == ActionToAssignPreset)
		{
			Data.Preset = PresetToAssign;
			break;
		}
	}
	
	for (FAIActionDataContainer& Data : CombatMoveActions)
	{
		FAIActionData& Action = Data.Data;
		if (Action.Name == ActionToAssignPreset)
		{
			Data.Preset = PresetToAssign;
			break;
		}
	}

	for (FAITraitActionData& TraitAction : TraitActions)
	{
		for (FAIActionDataContainer& Action : TraitAction.Actions)
		{
			if (Action.GetActionData().Name == ActionToAssignPreset)
			{
				Action.Preset = PresetToAssign;
				break;
			}
		}
	}

	ActionToAssignPreset = NAME_None;
	PresetToAssign = nullptr;
}
#endif
