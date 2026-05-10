// Copyright Void Interactive, 2022

#pragma once

#include "Engine/DataAsset.h"
#include "AI/AIActionData.h"
#include "AIArchetypeData.generated.h"

DECLARE_STATS_GROUP(TEXT("AIArchetype"), STATGROUP_AIArchetype, STATCAT_Advanced);

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UAIArchetypeData final : public UDataAsset
{
	GENERATED_BODY()

public:
	UAIArchetypeData();
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableAlertActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableAlertActions"))
	TArray<FAIActionDataContainer> AlertActions;
	
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableUnalertActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableUnalertActions"))
	TArray<FAIActionDataContainer> UnalertActions;
	
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableSuspiciousActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableSuspiciousActions"))
	TArray<FAIActionDataContainer> SuspiciousActions;
	
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableContinuousActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableContinuousActions"))
	TArray<FAIActionData_NameOnly> ContinuousActions;
	UPROPERTY()
	TArray<FAIActionDataContainer> ContinuousActionsCache;
	
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableCombatMoveActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableCombatMoveActions"))
	TArray<FAIActionDataContainer> CombatMoveActions;
	
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (InlineEditConditionToggle))
	uint8 bEnableTraitActions : 1;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name", EditCondition = "bEnableTraitActions"))
	TArray<FAITraitActionData> TraitActions;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Name"))
	TArray<FAIActionData> ActionsDump;
	#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	uint8 bOverrideAimSettings : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float FocalPointInterpSpeed = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	EAlphaBlendOption FocalPointInterpCurve = EAlphaBlendOption::ExpOut;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float FocusTurnSpeed = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float TurnDegreesPerSecond = 80.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float ActorRotationInterpStandingSpeed = 8.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float ActorRotationInterpMovingSpeed = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming", meta = (EditCondition = "bOverrideAimSettings"))
	float AimOffsetInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	uint8 bIgnoreDamageHitReactions : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ArrayClamp = 3))
	TArray<EAIAwarenessState> AllowCombatMoveInState = {EAIAwarenessState::Alerted, EAIAwarenessState::Suspicious};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Suppression")
	uint8 bSuppressionRequiresLOSOnLastKnownEnemyPosition : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Overrides", meta = (InlineEditConditionToggle))
	uint8 bAccuracyOverride : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Overrides", meta = (EditCondition = "bAccuracyOverride"))
	float Accuracy = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Overrides", meta = (InlineEditConditionToggle))
	uint8 bMoraleOverride : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Overrides", meta = (EditCondition = "bMoraleOverride"))
	float MinMorale = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Overrides", meta = (EditCondition = "bMoraleOverride"))
	float MaxMorale = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VO")
	uint8 bDisableVO : 1;

	void AuditActions();
	
	void CreateCustomActionsIfNeeded(class ACyberneticController* InController);

	void ClearTransientData();
	void RemoveAIData(class ACyberneticController* AIController);

	void Tick(class ACyberneticController* AIController, float DeltaTime);

	static FAIActionData* FindActionByName(TArray<FAIActionData>& InActions, FName ActionName);
	static FAIActionData* FindActionByType(TArray<FAIActionData>& InActions, EAIAction ActionType);
	static FAIActionData* FindActionByCustomType(TArray<FAIActionData>& InActions, TSubclassOf<UAIAction> CustomAction);
	
	static FAIActionData* FindActionByName(TArray<FAIActionDataContainer>& InActions, FName ActionName);
	static FAIActionData* FindActionByType(TArray<FAIActionDataContainer>& InActions, EAIAction ActionType);
	static FAIActionData* FindActionByCustomType(TArray<FAIActionDataContainer>& InActions, TSubclassOf<UAIAction> CustomAction);

	static int32 GetSuccessfulConsiderCountForAction(const class ACyberneticController* AIController, TArray<FAIActionDataContainer>& InActions, FName ActionName);
	static int32 GetFailedConsiderCountForAction(const class ACyberneticController* AIController, TArray<FAIActionDataContainer>& InActions, FName ActionName);
	
protected:
	virtual void PostLoad() override;
	
	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	
private:
	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Info", meta = (MultiLine = true))
	FText Notes;

	UPROPERTY(EditAnywhere, Transient, Category = "Tools")
	UAIArchetypeData* CopyFromArchetype = nullptr;

	UPROPERTY(EditAnywhere, Transient, Category = "Tools|Preset")
	UAIActionPresetData* PresetToAssign = nullptr;
	UPROPERTY(EditAnywhere, Transient, Category = "Tools|Preset")
	FName ActionToAssignPreset = NAME_None;
	
	void AssignActionPreset();
	#endif
};