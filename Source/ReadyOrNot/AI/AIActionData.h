// Copyright Void Interactive, 2022

#pragma once

#include "AIDataTypes.h"
#include "AIActionData.generated.h"

UENUM(BlueprintType)
enum class EAIAction : uint8
{
	None,
	FireWeapon,
	Melee,
	HardCover			UMETA(DisplayName = "Hard Cover (Combat Move)"),
	Hide				UMETA(DisplayName = "Hide (Combat Move)"),
	HideExit			UMETA(DisplayName = "Hide Exit"),
	Surrender,
	FakeSurrender,
	PlayDead,
	Flee				UMETA(DisplayName = "Flee (Combat Move)"),
	Rush				UMETA(DisplayName = "Rush (Combat Move)"),
	Flank				UMETA(DisplayName = "Flank (Combat Move)"),
	Duel				UMETA(DisplayName = "Duel (Combat Move)"),
	Suppress			UMETA(DisplayName = "Suppress (Combat Move)"),
	Push				UMETA(DisplayName = "Push (Combat Move)"),
	Reposition			UMETA(DisplayName = "Reposition (Combat Move)"),
	TraverseHole,
	Investigate,
	PickUpItem,
	Suicide,
	NeverFakeSuicide	UMETA(DisplayName = "Suicide (No Fake)"),
	Custom
};

UENUM(BlueprintType)
enum class EAIConsiderationScoringMethod : uint8
{
	Additive,
	Subtractive,
	Multiplicative,
	//Divisive
};

USTRUCT(BlueprintType)
struct FAIActionDecisionContext
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	class ACyberneticController* Controller = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UWorld* World = nullptr;
};

USTRUCT(BlueprintType)
struct FAIActionGateData
{
	GENERATED_BODY()

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	FString Name = "";
	#endif
	
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadOnly)
	class UAIActionGate* Type = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bNot = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bUseCooldown = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, EditCondition = "bUseCooldown"))
	float Cooldown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLockGateOnCooldown = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bContributeToFailCount = false;
	
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, float> Cooldowns;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, bool> IsOpen;
	#endif
	
	void StartCooldown(class ACyberneticController* InController)
	{
		if (bUseCooldown)
		{
			Cooldowns.Add(InController, FMath::Max(Cooldown, 0.0f));
		}
	}
};
	
USTRUCT(BlueprintType)
struct FAIActionConsiderationData
{
	GENERATED_BODY()
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	FString Name = "";
	#endif

	// Coefficient for the final score calculated for this specific consideration
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f))
	float Weight = 1.0f;

	// The scoring method to use, where each method pools separately (i.e. multiplicative scores multiply eachother separate from additive scores which add to eachother)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAIConsiderationScoringMethod ScoringMethod = EAIConsiderationScoringMethod::Additive;
	
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadOnly)
	class UAIActionConsideration* Type = nullptr;

	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, float> Scores;
	
	FORCEINLINE bool IsValid() const
	{
		if (!Type)
			return false;
		
		if (Weight <= 0.0f)
			return false;

		return true;
	}
};

USTRUCT(BlueprintType)
struct FAIActionData_NameOnly
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FName Name = "";
};

USTRUCT(BlueprintType)
struct FAIActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName Name = "";
	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FText Description;
	#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAIAction ActionType = EAIAction::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "ActionType == EAIAction::Custom"))
	TSubclassOf<class UAIAction> CustomActionClass = nullptr;

	// Coefficient for the score sum calculated from all considerations
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f))
	float Weight = 1.0f;
	// The minimum score this action must surpass in order to be considered
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f))
	float ScoreThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bDisallowWhenLastAlive : 1;
	
	// When enabled, the action cannot be performed again once committed to (does not count completion)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bDoOnce : 1;
	// Should this action still be considered when we globally stop decision making
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bAlwaysActive : 1;
	// Should this action not stop even when switching awareness states
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bContinueBetweenAwarenessStates : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	uint8 bDisableActionWhenFailedToConsider : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1, EditCondition = "bDisableActionWhenFailedToConsider"))
	int32 DisableActionConsiderCount = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bCommitUntilEnd : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, EditCondition = "!bCommitUntilEnd && !bCommitTimeFromConfig"))
	float CommitTime = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bCommitUntilEnd", InlineEditConditionToggle))
	uint8 bCommitTimeFromConfig : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bCommitTimeFromConfig"))
	FString CommitTimeConfigKey = "";
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bCanInterruptAnyAction : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Name", EditCondition = "!bCanInterruptAnyAction"))
	TArray<FAIActionData_NameOnly> CommitInterrupts;

	// Enable to check for cooldowns for this action, which prevent consideration if the specified time has not elapsed once performed
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 bUseCooldown : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, EditCondition = "bUseCooldown"))
	float Cooldown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseCooldown", InlineEditConditionToggle))
	uint8 bCooldownFromConfig : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bCooldownFromConfig"))
	FString CooldownConfigKey = "";

	// Gates prevent an action from being considered/performed if any of them fail to open, even if we are already performing said action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FAIActionGateData> Gates;

	// Considerations contribute to the score of this action, which ultimately decides which action is taken
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FAIActionConsiderationData> Considerations;
	
	#if WITH_EDITORONLY_DATA
	// Editor storage for unused/stashed considerations that are not calculated
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Name"), DisplayName = "Considerations (Dump)")
	TArray<FAIActionConsiderationData> Considerations_Dump;
	#endif

	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, float> Scores;
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, float> Cooldowns;
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, float> CommitTimes;
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, int32> SuccessConsiderCount;
	UPROPERTY(VisibleInstanceOnly, Transient, EditFixedSize)
	TMap<class ACyberneticController*, int32> FailConsiderCount;
	UPROPERTY(Transient)
	TMap<class ACyberneticController*, class UAIAction*> CustomActions;
	UPROPERTY(Transient)
	TMap<class ACyberneticController*, uint32> RunCount;
	
	bool IsValid() const;

	void CreateCustomAction(class ACyberneticController* InController);
	void Tick(class ACyberneticController* InController, float DeltaTime);
	
	void StartCooldown(class ACyberneticController* InController);
	void Commit(class ACyberneticController* InController);

	UAIAction* GetCustomAction(const class ACyberneticController* InController) const;
};

USTRUCT(BlueprintType)
struct FAIActionDataContainer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName Name = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Name", EditCondition = "Preset == nullptr", EditConditionHides))
	FAIActionData Data;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UAIActionPresetData* Preset = nullptr;

	FAIActionData& GetActionData();
};

USTRUCT(BlueprintType)
struct FAITraitActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Name = "";
	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FText Description;
	#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EAIAwarenessState> AllowedInAwarenessState = {EAIAwarenessState::Unalerted, EAIAwarenessState::Suspicious, EAIAwarenessState::Alerted};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Name"))
	TArray<FAIActionDataContainer> Actions;
};

DECLARE_STATS_GROUP(TEXT("AIActionDecisionEvaluator"), STATGROUP_AIActionDecisionEvaluator, STATCAT_Advanced);

class READYORNOT_API AIActionDecisionEvaluator
{
public:
	static FAIActionData* DetermineBestActionFor(class ACyberneticController* AIController, TArray<FAIActionDataContainer>& Actions);
	static FAIActionData* DetermineBestActionFor(class ACyberneticController* AIController, TArray<FAIActionData*>& Actions);
};
