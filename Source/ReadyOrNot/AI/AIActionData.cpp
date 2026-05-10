// Copyright Void Interactive, 2022

#include "AIActionData.h"

#include "AIAction.h"
#include "AIActionConsideration.h"
#include "AIActionGate.h"
#include "AIActionPresetData.h"
#include "ReadyOrNotAIConfig.h"

#include "Characters/CyberneticController.h"
#include "Info/Activities/BaseCombatActivity.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Decision Evaluator"), STAT_AIActionDecisionEvaluator_DetermineBestAction, STATGROUP_AIActionDecisionEvaluator);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Decision Evaluator Considerations"), STAT_AIActionDecisionEvaluator_Considerations, STATGROUP_AIActionDecisionEvaluator);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Decision Evaluator Gates"), STAT_AIActionDecisionEvaluator_Gates, STATGROUP_AIActionDecisionEvaluator);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Decision Evaluator Find Top Scoring"), STAT_AIActionDecisionEvaluator_FindTop, STATGROUP_AIActionDecisionEvaluator);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Increment Success Count"), STAT_AIActionDecisionEvaluator_IncrementSuccessCount, STATGROUP_AIActionDecisionEvaluator);
DECLARE_CYCLE_STAT(TEXT("RoN ~ AI Action Increment Fail Count"), STAT_AIActionDecisionEvaluator_IncrementFailCount, STATGROUP_AIActionDecisionEvaluator);

void ConsiderAction(class ACyberneticController* AIController, FAIActionData& Action, TArray<FAIActionData*>& OutConsideredActions);
TArray<FAIActionData*> FindTopScoringActions(class ACyberneticController* AIController, TArray<FAIActionData*>& InActionsConsidered, uint8 TopN);
void IncrementSuccessCount(class ACyberneticController* AIController, FAIActionData& Action);
void IncrementFailCount(class ACyberneticController* AIController, FAIActionData& Action);

bool FAIActionData::IsValid() const
{
	if (ActionType == EAIAction::None)
		return false;

	if (ActionType == EAIAction::Custom && CustomActionClass == nullptr)
		return false;

	if (Considerations.Num() == 0)
		return false;

	if (Weight <= 0.0f)
		return false;

	return true;
}

void FAIActionData::CreateCustomAction(ACyberneticController* InController)
{
	if (ActionType == EAIAction::Custom && !CustomActions.Find(InController))
	{
		if (CustomActionClass && !CustomActionClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (UAIAction* CustomAction = NewObject<UAIAction>(InController, CustomActionClass.Get()))
			{
				CustomAction->OnCreate(InController);

				InController->CustomActions.Add(CustomAction); // fixes GC destroying this action blueprint
				
				CustomActions.Add(InController, CustomAction);
			}
		}
	}
}

void FAIActionData::Tick(ACyberneticController* InController, float DeltaTime)
{
	// Cooldown
	if (float* CooldownValuePtr = Cooldowns.Find(InController))
	{
		*CooldownValuePtr = FMath::Max(*CooldownValuePtr - DeltaTime, 0.0f);
		if (*CooldownValuePtr <= 0.0f)
		{
			Cooldowns.Remove(InController);
		}
	}

	// Commit time
	if (float* CommitTimeValuePtr = CommitTimes.Find(InController))
	{
		*CommitTimeValuePtr = FMath::Max(*CommitTimeValuePtr - DeltaTime, 0.0f);
		if (*CommitTimeValuePtr <= 0.0f)
		{
			CommitTimes.Remove(InController);
		}
	}

	// Gate cooldown
	for (FAIActionGateData& Gate : Gates)
	{
		if (float* CooldownValuePtr = Gate.Cooldowns.Find(InController))
		{
			*CooldownValuePtr = FMath::Max(*CooldownValuePtr - DeltaTime, 0.0f);
			if (*CooldownValuePtr <= 0.0f)
			{
				Gate.Cooldowns.Remove(InController);
			}
		}
	}
}

void FAIActionData::StartCooldown(ACyberneticController* InController)
{
	if (bUseCooldown)
	{
		if (bCooldownFromConfig)
			Cooldown = AI_CONFIG_GET_FLOAT(CooldownConfigKey, 0.0f);
		
		Cooldowns.Add(InController, FMath::Max(Cooldown, 0.0f));
	}

	for (FAIActionGateData& Gate : Gates)
	{
		Gate.StartCooldown(InController);
	}
}

void FAIActionData::Commit(ACyberneticController* InController)
{
	if (uint32* Count = RunCount.Find(InController))
	{
		(*Count)++;
	}
	else
	{
		RunCount.Add(InController, 1);
	}

	if (bCommitTimeFromConfig)
		CommitTime = AI_CONFIG_GET_FLOAT(CommitTimeConfigKey, 0.0f);
	
	if (CommitTime > 0.0f)
	{
		CommitTimes.Add(InController, FMath::Max(CommitTime, 0.0f));
	}
}

UAIAction* FAIActionData::GetCustomAction(const ACyberneticController* InController) const
{
	if (ActionType != EAIAction::Custom)
		return nullptr;
	
	if (UAIAction* const* FoundActionPtr = CustomActions.Find(InController))
	{
		return *FoundActionPtr;
	}

	return nullptr;
}

FAIActionData& FAIActionDataContainer::GetActionData()
{
	if (Preset)
	{
		return Preset->Action;
	}

	return Data;
}

FAIActionData* AIActionDecisionEvaluator::DetermineBestActionFor(ACyberneticController* AIController, TArray<FAIActionDataContainer>& Actions)
{
	TArray<FAIActionData*> BestActions;
	
	if (Actions.Num() > 0)
	{
		BestActions.Reserve(Actions.Num());
		
		for (FAIActionDataContainer& Action : Actions)
		{
			ConsiderAction(AIController, Action.GetActionData(), BestActions);
		}
		
		BestActions.Sort([&](const FAIActionData& Lhs, const FAIActionData& Rhs)
		{
			return *Lhs.Scores.Find(AIController) > *Rhs.Scores.Find(AIController);
		});
	}
	
	if (BestActions.Num() > 0)
	{
		// Report action successfully considererd
		for (FAIActionData* Action : BestActions)
		{
			AIController->CombatActivity->OnSuccessfullyConsideredAction(Action);
		}
		
		// Report action failed to consider
		for (FAIActionDataContainer& Action : Actions)
		{
			FAIActionData& Data = Action.GetActionData();
			if (&Data != BestActions[0])
			{
				AIController->CombatActivity->OnFailedToConsiderAction(&Data);
			}
		}
		
		return BestActions[0];
	}

	return nullptr;
}

FAIActionData* AIActionDecisionEvaluator::DetermineBestActionFor(ACyberneticController* AIController, TArray<FAIActionData*>& Actions)
{
	TArray<FAIActionData*> BestActions;
	
	if (Actions.Num() > 0)
	{
		BestActions.Reserve(Actions.Num());
		
		for (FAIActionData* Action : Actions)	
		{
			ConsiderAction(AIController, *Action, BestActions);
		}

		BestActions.Sort([&](const FAIActionData& Lhs, const FAIActionData& Rhs)
		{
			return *Lhs.Scores.Find(AIController) > *Rhs.Scores.Find(AIController);
		});
	}
	
	if (BestActions.Num() > 0)
	{
		// Report action successfully considererd
		for (FAIActionData* Action : BestActions)
		{
			AIController->CombatActivity->OnSuccessfullyConsideredAction(Action);
		}
		
		// Report action failed to consider
		for (FAIActionData* Action : Actions)
		{
			if (Action != BestActions[0])
			{
				AIController->CombatActivity->OnFailedToConsiderAction(Action);
			}
		}
		
		return BestActions[0];
	}

	return nullptr;
}

void ConsiderAction(class ACyberneticController* AIController, FAIActionData& Action, TArray<FAIActionData*>& OutConsideredActions)
{
	if (!Action.IsValid())
	{
		return;
	}

	if (AIController->bStopDecisionMaking && !Action.bAlwaysActive)
	{
		return;
	}

	// Only allowed once?
	if (Action.bDoOnce)
	{
		if (Action.RunCount.Find(AIController))
		{
			return;
		}
	}
	
	// Is on cooldown?
	if (const float* ValuePtr = Action.Cooldowns.Find(AIController))
	{
		if (*ValuePtr > 0.0f)
			return;
	}

	// Is this action allowed to be considered when last alive?
	if (AIController->IsLastAlive())
	{
		if (Action.bDisallowWhenLastAlive)
			return;
	}

	// Disable after 'N' failed consideration attempts?
	if (Action.bDisableActionWhenFailedToConsider)
	{
		if (const int32* CountPtr = Action.FailConsiderCount.Find(AIController))
		{
			if (*CountPtr >= Action.DisableActionConsiderCount)
			{
				Action.Scores.Remove(AIController);
				return;
			}
		}
	}

	FAIActionDecisionContext Context;
	Context.Controller = AIController;
	Context.World = AIController->GetWorld();

	// Are all gates open?
	{
		SCOPE_CYCLE_COUNTER(STAT_AIActionDecisionEvaluator_Gates);

		#if WITH_EDITORONLY_DATA
		for (FAIActionGateData& Gate : Action.Gates)
		{
			Gate.IsOpen.FindOrAdd(AIController, false) = false;
		}
		#endif

		bool bAllGatesOpen = true;
		const FAIActionGateData* FailedGate = nullptr;
		for (FAIActionGateData& Gate : Action.Gates)
		{
			if (const float* ValuePtr = Gate.Cooldowns.Find(AIController))
			{
				if (*ValuePtr > 0.0f)
				{
					if (Gate.bLockGateOnCooldown)
					{
						bAllGatesOpen = false;
						FailedGate = &Gate;
						break;
					}
					
					continue;
				}
			}
			
			if (Gate.Type)
			{
				Gate.StartCooldown(AIController);

				bool bCanOpen = Gate.Type->CanOpen(Context);

				if (Gate.bNot)
					bCanOpen = !bCanOpen;

				#if WITH_EDITORONLY_DATA
				Gate.IsOpen.FindOrAdd(AIController, bCanOpen) = bCanOpen;
				#endif
				
				if (!bCanOpen)
				{
					bAllGatesOpen = false;
					FailedGate = &Gate;
					break;
				}
			}
		}

		if (!bAllGatesOpen)
		{
			if (FailedGate->bContributeToFailCount)
			{
				IncrementFailCount(AIController, Action);
			}
			
			return;
		}
	}
	
	float* ActionScorePtr = &Action.Scores.Add(AIController, 0.0f);

	float TotalScore = 0.0f;
	uint8 NumSuccessfulConsiderations = 0;
	
	{
		SCOPE_CYCLE_COUNTER(STAT_AIActionDecisionEvaluator_Considerations);

		if (Action.Considerations.Num() > 0)
		{
			// if the first consideration is a multiplicative one, start off with a score of 1.0 so the first one doesnt fail
			// even though it could return with a non-zero score
			if (Action.Considerations[0].ScoringMethod == EAIConsiderationScoringMethod::Multiplicative)
			{
				TotalScore = 1.0f;
			}
		}
		
		for (FAIActionConsiderationData& C : Action.Considerations)
		{
			if (C.Type)
			{
				float* ConsiderationScorePtr = &C.Scores.Add(AIController, 0.0f);
				
				if (C.Weight <= 0.0f)
				{
					*ConsiderationScorePtr = 0.0f;
					continue;
				}
				
				bool bSuccess = false;
				
				const float Score = C.Type->Score(Context, bSuccess);
				
				if (bSuccess)
				{
					*ConsiderationScorePtr = FMath::Clamp(C.Type->EvaluateResponseCurve(Score), 0.0f, 1.0f);
					*ConsiderationScorePtr *= C.Weight;
					
					NumSuccessfulConsiderations++;

					if (C.ScoringMethod == EAIConsiderationScoringMethod::Additive)
					{
						TotalScore += *ConsiderationScorePtr;
					}
					else if (C.ScoringMethod == EAIConsiderationScoringMethod::Subtractive)
					{
						TotalScore -= *ConsiderationScorePtr;
					}
					else if (C.ScoringMethod == EAIConsiderationScoringMethod::Multiplicative)
					{
						const float ModificationFactor = 1.0f-(1.0f/(float)Action.Considerations.Num());
						const float CompensationFactor = (1.0f-*ConsiderationScorePtr)*ModificationFactor;
						
						TotalScore *= *ConsiderationScorePtr + (CompensationFactor * *ConsiderationScorePtr);
						//TotalScore *= *ConsiderationScorePtr;
					}
					/*
					else if (C.ScoringMethod == EAIConsiderationScoringMethod::Divisive)
					{
						if (*ConsiderationScorePtr > 0.0f)
						{
							TotalScore /= *ConsiderationScorePtr;
						}
					}
					*/
					
					// doesnt work as well as having the option to select the type of a score altering mechanism we want
					// for example the suppression consideration returns 0 most of the time because well... no body is firing at this moment
					// and so it'll return 0 and bring the total score down to 0 and the action will never be considered. So having the
					// option to select with a scoring method we want is useful and we could change it to be an addtive effect
					// instead of a multiplicative one
					
					//const float ModificationFactor = 1.0f-(1.0f/(float)Action.Considerations.Num());
					//const float CompensationFactor = (1.0f-*ConsiderationScorePtr)*ModificationFactor;
					
					//TotalScore *= *ConsiderationScorePtr + (CompensationFactor * *ConsiderationScorePtr);
				}
			}
		}
	}

	if (NumSuccessfulConsiderations == 0)
	{
		IncrementFailCount(AIController, Action);
		return;
	}
	
	const float ActionScore = FMath::Max(TotalScore, 0.0f);

	if (FMath::IsNearlyZero(ActionScore, 0.00001f))
	{
		IncrementFailCount(AIController, Action);
		return;
	}
	
	//ActionScore = ActionScore/(float)NumSuccessfulConsiderations;
	
	*ActionScorePtr = ActionScore * Action.Weight;

	if (*ActionScorePtr <= 0.0f || *ActionScorePtr <= Action.ScoreThreshold)
	{
		IncrementFailCount(AIController, Action);
		return;
	}

	IncrementSuccessCount(AIController, Action);
	OutConsideredActions.Add(&Action);
}

void IncrementSuccessCount(class ACyberneticController* AIController, FAIActionData& Action)
{
	SCOPE_CYCLE_COUNTER(STAT_AIActionDecisionEvaluator_IncrementSuccessCount);
	
	if (int32* CountPtr = Action.SuccessConsiderCount.Find(AIController))
	{
		(*CountPtr)++;
	}
	else
	{
		Action.SuccessConsiderCount.Add(AIController, 1);
	}
}

void IncrementFailCount(class ACyberneticController* AIController, FAIActionData& Action)
{
	SCOPE_CYCLE_COUNTER(STAT_AIActionDecisionEvaluator_IncrementFailCount);
	
	if (int32* CountPtr = Action.FailConsiderCount.Find(AIController))
	{
		(*CountPtr)++;
	}
	else
	{
		Action.FailConsiderCount.Add(AIController, 1);
	}
}
