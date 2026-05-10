// Copyright Void Interactive, 2022

#include "CivilianController.h"

#include "ReadyOrNotAIConfig.h"
#include "Info/Activities/BaseCombatActivity.h"

float ACivilianController::GetReactionTime(const EActorSenseType& SenseType) const
{
	const float ReactionTimeMultiplier = (AwarenessState == EAIAwarenessState::Alerted ? FMath::Clamp(AI_CONFIG_GET_FLOAT("CivilianAlertReactionTimeMultiplier", 0.5f), 0.0f, 1.0f) : 1.0f);
	
	switch (SenseType)
	{
		case EActorSenseType::Sight:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("CivilianSightStimulusReactionTime", 0.25f) * ReactionTimeMultiplier, 0.0f, 1.0f);
		case EActorSenseType::Sound:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("CivilianSoundStimulusReactionTime", 0.25f) * ReactionTimeMultiplier, 0.0f, 1.0f);
		case EActorSenseType::Damage:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("CivilianDamageStimulusReactionTime", 0.1f) * ReactionTimeMultiplier, 0.0f, 1.0f);
		default: return 0.25f;
	}
}

void ACivilianController::OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnSeenCharacter(SensedCharacter, Stimulus);
	
	if (GetCharacter()->IsWearingExplosiveVest())
	{
		GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::IDLE_BOMB_VEST, 45.0f);
	}
	
	if (SensedCharacter->IsPlayerControlled())
	{
		if (!TargetingComponent->IsTrackedByKnownFriendly(SensedCharacter))
		{
			if (!GetCharacter()->IsArrestedOrSurrendered())
			{
				const float Distance = FVector::Distance(SensedCharacter->GetActorLocation(), GetCharacter()->GetActorLocation());

				if (bCanCallForHelp && Distance <= MaxHearingForHelpDistance)
				{
					bCanCallForHelp = false;
					GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_CALL_OUT_FOR_HELP, true, CallForHelpCoolDownDuration);
				}
			}
		}
	}
	else
	{
		// Is fellow civilian alerted?
		if (SensedCharacter->IsCivilian())
		{
			if (SensedCharacter->IsDeadOrUnconscious() || SensedCharacter->IsIncapacitated())
			{
				if (!TargetingComponent->IsTrackedInKnownFriendlies(SensedCharacter))
					GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED);
			}
				
			if (const ACyberneticCharacter* SensedCivilian = Cast<ACyberneticCharacter>(SensedCharacter))
			{
				if (AwarenessState < EAIAwarenessState::Alerted)
				{
					if (SensedCivilian->IsActive())
					{
						if (SensedCivilian->GetCyberneticsController()->GetAwarenessState() > EAIAwarenessState::Unalerted)
						{
							if (AwarenessState != EAIAwarenessState::Alerted)
							{
								AwarenessState = EAIAwarenessState::Suspicious;
							}
						}
					}
					else
					{
						AwarenessState = EAIAwarenessState::Suspicious;
						GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Alert, 30.0f, 1.0f, 0.1f, 1);

						constexpr int32 NumMoves = 3;
						TArray<EAIAction, TInlineAllocator<NumMoves>> CombatMoveActions;

						CombatMoveActions.Add(EAIAction::HardCover);
						CombatMoveActions.Add(EAIAction::Flee);
						CombatMoveActions.Add(EAIAction::Hide);
						
						// Force combat move
						if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
						{
							BestCombatMoveAction = UAIArchetypeData::FindActionByType(Archetype->CombatMoveActions, CombatMoveActions[FMath::RandRange(0, NumMoves-1)]);

							if (GetCombatActivity() && BestCombatMoveAction)
							{
								BestCombatMoveAction->Scores.Add(this, 1.0f);
								BestCombatMoveAction->Commit(this);
								GetCombatActivity()->BeginAction(BestCombatMoveAction);
							}
						}
					}
				}
			}
		}
	}
}

void ACivilianController::OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnHeardCharacter(SensedCharacter, Stimulus);
	
}

void ACivilianController::OnSeenGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation)
{
	Super::OnSeenGrenade(InInstigator, GrenadeLocation);
}

void ACivilianController::OnHeardGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation, const FName& InTag)
{
	if (InTag == "GrenadeExplosion" || InTag == "GrenadeThrown")
	{
		GetCharacter()->PlayMontageFromTable("tp_flinch_grenade");
	}
	else
	{
		if (IsTagAggressiveNoise(InTag))
			GetCharacter()->PlayMontageFromTable("tp_flinch");
	}
}

bool ACivilianController::IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsOnSWATTeam();
}

bool ACivilianController::IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsSuspect();
}

bool ACivilianController::IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsCivilian();
}
