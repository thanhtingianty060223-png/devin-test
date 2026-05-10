// Copyright Void Interactive, 2022

#include "SuspectController.h"

#include "ReadyOrNotAIConfig.h"
#include "SuspectCharacter.h"
#include "Info/Activities/BaseCombatActivity.h"

float ASuspectController::GetReactionTime(const EActorSenseType& SenseType) const
{
	float ReactionTimeMultiplier = 1.0f;
	if (AwarenessState == EAIAwarenessState::Alerted)
		ReactionTimeMultiplier = FMath::Clamp(AI_CONFIG_GET_FLOAT("SuspectAlertReactionTimeMultiplier", 0.5f), 0.0f, 1.0f);
	else if (AwarenessState == EAIAwarenessState::Suspicious)
		ReactionTimeMultiplier = FMath::Clamp(AI_CONFIG_GET_FLOAT("SuspectSuspiciousReactionTimeMultiplier", 0.25f), 0.0f, 1.0f);
		
	switch (SenseType)
	{
		case EActorSenseType::Sight:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SuspectSightStimulusReactionTime", 0.25f) * ReactionTimeMultiplier, 0.0f, 2.0f); // max half a sec reaction time so they're not too dumb
		case EActorSenseType::Sound:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SuspectSoundStimulusReactionTime", 0.25f) * ReactionTimeMultiplier, 0.0f, 2.0f);
		case EActorSenseType::Damage:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SuspectDamageStimulusReactionTime", 0.1f) * ReactionTimeMultiplier, 0.0f, 2.0f);
		default: return 0.25f;
	}
}

void ASuspectController::ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor)
{
	Super::ProcessStimuli(Stimulus, SensedActor, PerceptionOfActor);
}

void ASuspectController::OnHeardActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus, float ExpiryTime)
{
	Super::OnHeardActor(InActor, InTag, Stimulus, ExpiryTime);

	if (!InActor)
		return;
	
	if (IsTagAggressiveNoise(Stimulus.Tag))
	{
		if (Stimulus.Tag == "SuppressedGunshot")
		{
			GetCharacter()->Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("SuppressedGunshotStress", 0.05f), 0.0f, 1.0f);
		}
		else
		{
			GetCharacter()->Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("GunshotStress", 0.1f), 0.0f, 1.0f);
		}
	}
	
	if (Stimulus.Tag == "ExplosionClose" || Stimulus.Tag == "ExplosionMedium" || Stimulus.Tag == "ExplosionFar" || Stimulus.Tag == "RamDoor" || Stimulus.Tag == "ExplodeDoor")
	{
		OnHeardExplosion(SensedActorToCharacter(InActor), Stimulus.StimulusLocation);
		GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_TRIGGER_ACTIVATED, true);
		return;
	}
	
	const float MaxZ = FMath::Max(GetCharacter()->GetActorLocation().Z, Stimulus.StimulusLocation.Z);
	const float MinZ = FMath::Min(GetCharacter()->GetActorLocation().Z, Stimulus.StimulusLocation.Z);
	
	const float ZHeightDifference = MaxZ - MinZ;
	const float DistToSensedLocation = (GetCharacter()->GetActorLocation() - Stimulus.StimulusLocation).Size();

	// Don't sense stimulus above or below us
	if (ZHeightDifference < 150.0f)
	{
		if (!LastHeardActorTime.Find(InActor))
		{
			LastHeardActorTime.Add(InActor, 1.0f);
		}
		
		if (DistToSensedLocation < 1500.0f)
		{
			if (IsTagInvestigativeNoise(Stimulus.Tag))
			{
				if (!GetTrackedTarget() && GetCharacter()->GetEquippedItem() && !GetCharacter()->IsStunned())
				{
					//if (ASuspectsAndCivilianManager::Get(GetWorld())->CanInvestigate())
					{
						//InvestigateStimulus(Stimulus);
					}
				}
			}
		}
	}
}

void ASuspectController::OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	if (GetCharacter()->IsActive())
	{
		if (SensedCharacter->IsPlayerControlled() && !GetTrackedTarget() && !bHasEverSpottedSWAT)
		{
			if (GetCharacter()->IsWearingExplosiveVest())
			{
				GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::IDLE_BOMB_VEST);
			}
			else
			{
				GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_SEEN, true, 5.0f);
			}
		
			//GetTargetingComp()->ShareTarget(SensedCharacter);
		}

		if (const ACyberneticCharacter* SensedAI = Cast<ACyberneticCharacter>(SensedCharacter))
		{
			// Is sensed suspect alerted? If so, alert this suspect of all their known enemies
			if (AwarenessState < EAIAwarenessState::Alerted)
			{
				if (SensedAI->IsSuspect())
				{
					if (SensedAI->IsDeadOrUnconscious() || SensedAI->IsIncapacitated() || SensedAI->IsPlayingDead())
					{
						if (!TargetingComponent->IsTrackedInKnownFriendlies(const_cast<ACyberneticCharacter*>(SensedAI)))
							GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED);
					}
					
					if (SensedAI->IsActive())
					{
						if (SensedAI->GetCyberneticsController()->GetAwarenessState() > EAIAwarenessState::Unalerted)
						{
							for (AReadyOrNotCharacter* Enemy : SensedAI->GetCyberneticsController()->GetTargetingComp()->KnownEnemies)
							{
								TargetingComponent->AddKnownEnemy(Enemy, true);
							}
							
							if (AwarenessState != EAIAwarenessState::Alerted)
							{
								AwarenessState = EAIAwarenessState::Suspicious;
							}
						}
					}
					else if (!SensedAI->IsActive() || SensedAI->IsPlayingDead())
					{
						if (AwarenessState != EAIAwarenessState::Alerted)
						{
							AwarenessState = EAIAwarenessState::Suspicious;
						}
						
						GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Alert, 30.0f, 1.0f, 0.1f, 1);

						AReadyOrNotCharacter* Enemy = nullptr;
						if (SensedAI->GetCyberneticsController())
						{
							Enemy = SensedAI->GetCyberneticsController()->GetTrackedTarget();
							
							if (!Enemy)
								Enemy = SensedAI->GetCyberneticsController()->GetLastTrackedEnemy();
						}
						else
						{
							if (SensedAI->LastDamageEvent.Instigator)
								Enemy = SensedAI->LastDamageEvent.Instigator->GetPawn<AReadyOrNotCharacter>();
						}

						EAIAction CombatMoveAction;
						if (Enemy)
						{
							//TargetingComponent->SetLastTrackedTarget(Enemy);

							CombatMoveAction = EAIAction::HardCover;
						}
						else
						{
							CombatMoveAction = EAIAction::Flee;
						}
						
						// Force combat move
						if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
						{
							BestCombatMoveAction = UAIArchetypeData::FindActionByType(Archetype->CombatMoveActions, CombatMoveAction);

							if (BestCombatMoveAction)
							{
								BestCombatMoveAction->Scores.Add(this, 1.0f);
								BestCombatMoveAction->Commit(this);

								if (CombatActivity)
									CombatActivity->BeginAction(BestCombatMoveAction);
							}
						}
					}
				}
			}
		}
	}
	
	Super::OnSeenCharacter(SensedCharacter, Stimulus);
}

void ASuspectController::OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnHeardCharacter(SensedCharacter, Stimulus);
	
	AddExposedToStimulusTag(Stimulus.Tag, Stimulus.StimulusLocation, !SensedCharacter->IsOnSWATTeam(), SensedCharacter);
	
	if (Stimulus.Tag == "SWATYellForCompliance" && !GetTrackedTarget() && LineOfSightTo(SensedCharacter))
	{
		GetCharacter()->Stress = 0.0f;
	}

	if (IsCharacterEnemy(SensedCharacter))
		TargetingComponent->LastKnownTargetPosition	= SensedCharacter->GetActorLocation();
	
	if (SensedCharacter->IsActive())
	{
		if (Stimulus.Tag == "Footstep")
		{
			//if (CombatActivity)
				//CombatActivity->ScriptedLookAtActor(SensedCharacter, 4.0f);
		}
	}
}

void ASuspectController::OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnDamagedByCharacter(SensedCharacter, Stimulus);

	GetCharacter()->Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("DamageStress", 0.5f), 0.0f, 1.0f);
	
	if (SensedCharacter->IsPlayerControlled() && !GetTrackedTarget() && !bHasEverSpottedSWAT)
	{
		if (!GetCharacter()->IsArrestedOrSurrendered() && !GetCharacter()->IsStunned())
		{
			if (!GetCharacter()->IsArrestedOrSurrendered())
			{
				GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_SEEN, true, 5.0f);
			}
		}
	}
}

void ASuspectController::OnHeardGunShot(AReadyOrNotCharacter* InInstigator, const FVector& WeaponLocation, const FName& InTag)
{
	Super::OnHeardGunShot(InInstigator, WeaponLocation, InTag);
}

void ASuspectController::OnTrackedTargetStartedReloading(APlayerCharacter* InCharacter)
{
	if (GetCharacter()->IsActive())
	{
		GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_RELOADING);
	}
}

bool ASuspectController::IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsCivilian();
}

bool ASuspectController::IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsOnSWATTeam();
}

bool ASuspectController::IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsSuspect();
}
