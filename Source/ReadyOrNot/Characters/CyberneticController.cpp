// Copyright Void Interactive, 2021

#include "CyberneticController.h"

#include "ReadyOrNotGameMode.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Touch.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"

#include "Info/SuspectsAndCivilianManager.h"
#include "Info/Activities/BaseActivity.h"
#include "Info/WorldBuildingPlacementActor.h"
#include "Info/SWATManager.h"
#include "Info/Activities/InvestigateActivity.h"
#include "Info/Activities/MoveActivity.h"
#include "Info/Activities/CombatMove/CivilianFleeCombatMove.h"
#include "Info/Activities/CombatMove/ChargeCombatMove.h"

#include "Components/ReadyOrNotPathFollowingComp.h"
#include "Components/MoraleComponent.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/CyberneticCharacter.h"

#include "Actors/Door.h"
#include "Actors/Gameplay/FlashLightTrackingPoint.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "NavigationSystem.h"
#include "NavLinkCustomComponent.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotAISystem.h"
#include "Actors/BaseGrenade.h"
#include "Actors/CoverLandmark.h"
#include "Actors/CoverLandmarkProxy.h"
#include "Actors/Gameplay/IncapacitatedHuman.h"

#include "Actors/Items/MeleeWeapon.h"
#include "AI/AIAction.h"
#include "Animation/MoveStyle/RoNMoveStyleComponent.h"
#include "Info/ConversationManager.h"

#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/Activities/CivilianCombatActivity.h"
#include "Info/Activities/InvestigateStimulusActivity.h"
#include "Info/Activities/MoveToActivity.h"
#include "Info/Activities/MoveToExitActivity.h"
#include "Info/Activities/SuspectCombatActivity.h"
#include "Info/Activities/SwatCombatActivity.h"
#include "Info/Activities/TakeCoverActivity.h"
#include "Info/Activities/TakeCoverAtLandmarkActivity.h"
#include "Info/Activities/TargetNextCivilianActivity.h"

#include "Interfaces/ReceiveAISenseUpdates.h"

#include "Navigation/ReadyOrNotNavQueries.h"
#include "Navigation/PathFollowingComponent.h"

#include "Senses/ReadyOrNotAISenseConfig_Sight.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Cybernetic Controller Tick"), STAT_CyberneticControllerTick, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Cybernetic Controller Activity Work"), STAT_CyberneticControllerActivityWork, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Utility Decision Making"), STAT_CyberneticController_AIUtilityDecisionMaking, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Determine Interrupt Action"), STAT_CyberneticController_DetermineInterruptAction, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Line Of Sight"), STAT_CyberneticController_AILineOfSight, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Look At"), STAT_CyberneticController_LookAt, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Debug Info"), STAT_CyberneticController_DebugPrint, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Perform Combat Activity"), STAT_CyberneticController_PerformCombatActivity, STATGROUP_CyberneticController);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Perform Current Activity"), STAT_CyberneticController_PerformCurrentActivity, STATGROUP_CyberneticController);

TAutoConsoleVariable<int32> CVarRonToggleAIDebugLines(TEXT("a.RonToggleAIDebugLines"), 0, TEXT("0 = No draw all AI debug lines. 1 = Draw all AI debug lines"));
TAutoConsoleVariable<int32> CVarRonDrawAIDebug(TEXT("a.RonDrawAIDebug"), 0, TEXT("Draw AI Debug"));
TAutoConsoleVariable<int32> CVarRonDrawFocalPoint(TEXT("AI.DrawFocalPoint"), 0, TEXT("Draw AI focal points"));
TAutoConsoleVariable<int32> CVarRonDrawHeadTrackingPoint(TEXT("AI.DrawHeadTrackingPoint"), 0, TEXT("Draw AI head tracking point"));
TAutoConsoleVariable<int32> CVarUtilityAIDebug(TEXT("AI.Utility.Debug"), 0, TEXT("Draw Utility AI debug information"));
TAutoConsoleVariable<int32> CVarAlertAIWhenLastAlive(TEXT("AI.AlertWhenLastAlive"), 1, TEXT("Alert AI when they are the last one alive"));

ACyberneticController::ACyberneticController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UReadyOrNotPathFollowingComp>(TEXT("PathFollowingComponent")))
{
	bWantsPlayerState = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;

	// Setup the perception component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception Component"));

	SightConfig = CreateDefaultSubobject<UReadyOrNotAISenseConfig_Sight>(TEXT("Sight Config"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->SetMaxAge(10.0f);
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	TouchConfig = CreateDefaultSubobject<UAISenseConfig_Touch>(TEXT("Touch Config"));
	AIPerceptionComponent->ConfigureSense(*TouchConfig);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("Damage Config"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing Config"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->HearingRange = 3000;
	HearingConfig->SetMaxAge(10.0f);

	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	MoraleComponent = CreateDefaultSubobject<UMoraleComponent>(TEXT("Morale Component"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting Component"));
	
	bCanCallForHelp = true;

	bCanOpenDoorThroughNavLink = true;

	bAttachToPawn = true;
}

bool ACyberneticController::TickUtilityAI(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_CyberneticController_AIUtilityDecisionMaking);
	
	if (CombatActivity)
	{
		if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
		{
			TimeSinceThink += DeltaTime;
			if (TimeSinceThink > UtilityAIThinkRate)
			{
				TimeSinceThink = 0.0f;
				
				if (AwarenessState != EAIAwarenessState::Unalerted)
				{
					// Force end unalert actions
					if (BestUnalertAction && !BestUnalertAction->bContinueBetweenAwarenessStates)
					{
						CombatActivity->EndAction(BestUnalertAction);
						BestUnalertAction->StartCooldown(this);
						BestUnalertAction = nullptr;
						BestAction = nullptr;
					}
				}

				if (AwarenessState != EAIAwarenessState::Suspicious)
				{
					// Force end suspicious actions
					if (BestSuspiciousAction && !BestSuspiciousAction->bContinueBetweenAwarenessStates)
					{
						CombatActivity->EndAction(BestSuspiciousAction);
						BestSuspiciousAction->StartCooldown(this);
						BestSuspiciousAction = nullptr;
						BestAction = nullptr;
					}
				}
				
				//if (AwarenessState > EAIAwarenessState::Unalerted)
				{
					const bool bCanDoCombatMove = Archetype->AllowCombatMoveInState.Contains(AwarenessState);
					// !Archetype->bOnlyAllowCombatMovesWhilstAlert || (Archetype->bOnlyAllowCombatMovesWhilstAlert && AwarenessState == EAIAwarenessState::Alerted);
					
					if (Archetype->bEnableCombatMoveActions && bCanDoCombatMove)
					{
						bool bIsCommitingToCombatMove = false;
						if (BestCombatMoveAction)
						{
							if (BestCombatMoveAction->bCommitUntilEnd)
							{
								bIsCommitingToCombatMove = true;
							}
							else
							{
								if (const float* Time = BestCombatMoveAction->CommitTimes.Find(this))
								{
									if (*Time > 0.0f)
									{
										bIsCommitingToCombatMove = true;
									}
								}
							}
						}
						
						const bool bCanEvaluateCombatMoveDecision = !BestCombatMoveAction || (BestCombatMoveAction && !bIsCommitingToCombatMove);

						if (bCanEvaluateCombatMoveDecision)
						{
							if (FAIActionData* ChosenAction = AIActionDecisionEvaluator::DetermineBestActionFor(this, Archetype->CombatMoveActions))
							{
								BestCombatMoveAction = ChosenAction;
								BestCombatMoveAction->Commit(this);
								CombatActivity->BeginAction(BestCombatMoveAction);
							}
						}
						else
						{
							DetermineBestInterruptAction(Archetype->CombatMoveActions, BestCombatMoveAction);
						}
					}
				}

				TArray<FAIActionDataContainer>* CurrentActionsForAwarenessState = nullptr;
				FAIActionData** BestAwarenessAction = nullptr;
				
				if (AwarenessState == EAIAwarenessState::Unalerted)
				{
					if (Archetype->bEnableUnalertActions)
					{
						CurrentActionsForAwarenessState = &Archetype->UnalertActions;
						BestAwarenessAction = &BestUnalertAction;
					}
				}
				else if (AwarenessState == EAIAwarenessState::Suspicious)
				{
					if (Archetype->bEnableSuspiciousActions)
					{
						CurrentActionsForAwarenessState = &Archetype->SuspiciousActions;
						BestAwarenessAction = &BestSuspiciousAction;
					}
				}
				else
				{
					if (Archetype->bEnableAlertActions)
					{
						CurrentActionsForAwarenessState = &Archetype->AlertActions;
						
						if (Archetype->bEnableContinuousActions)
						{
							BestContinuousAction = AIActionDecisionEvaluator::DetermineBestActionFor(this, Archetype->ContinuousActionsCache);

							if (BestContinuousAction)
							{
								CombatActivity->BeginAction(BestContinuousAction);
							}
						}
					}
				}

				if (CurrentActionsForAwarenessState)
				{
					if (!BestAction)
					{
						BestAction = AIActionDecisionEvaluator::DetermineBestActionFor(this, *CurrentActionsForAwarenessState);
						if (BestAwarenessAction)
							*BestAwarenessAction = BestAction;

						if (BestAction)
						{
							BestAction->Commit(this);
							CombatActivity->BeginAction(BestAction);
						}
					}
					else
					{
						DetermineBestInterruptAction(*CurrentActionsForAwarenessState, BestAction);
					}
				}
				
				if (Archetype->bEnableTraitActions)
				{
					FAIActionData* TopTraitAction = nullptr;
						
					for (FAITraitActionData& TraitAction : Archetype->TraitActions)
					{
						if (TraitAction.AllowedInAwarenessState.Contains(AwarenessState))
						{
							if (FAIActionData* BestTraitAction = AIActionDecisionEvaluator::DetermineBestActionFor(this, TraitAction.Actions))
							{
								if (!TopTraitAction || BestTraitAction->Scores.Find(this) > TopTraitAction->Scores.Find(this))
									TopTraitAction = BestTraitAction;
							}
						}
					}

					if (!BestAction)
					{
						if (TopTraitAction)
						{
							BestAction = TopTraitAction;
							BestAction->Commit(this);
							CombatActivity->BeginAction(BestAction);
						}
					}
					else
					{
						if (TopTraitAction)
						{							
							if (const float* InterruptScoreActionPtr = TopTraitAction->Scores.Find(this))
							{
								if (const float* BestActionScorePtr = BestAction->Scores.Find(this))
								{
									if (*InterruptScoreActionPtr > *BestActionScorePtr)
									{
										// Stop current action
										BestAction->CommitTimes.Remove(this);
										CombatActivity->EndAction(BestAction);
										BestAction->StartCooldown(this);

										// Switch to best interrupt action
										BestAction = TopTraitAction;
										BestAction->Commit(this);
										CombatActivity->BeginAction(BestAction);
									}
								}
							}
						}
					}
				}

				return true;
			}
		}
	}

	return false;
}

void ACyberneticController::BeginPlay()
{
	Super::BeginPlay();
	
	AIPerceptionComponent->OnPerceptionUpdated.RemoveAll(this);
	AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &ACyberneticController::OnPerceptionUpdated);
	
	bPendingNextActivityRoute = true;

	for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		Door->OnDoorExploded.AddDynamic(this, &ACyberneticController::OnDoorExploded);
		//Door->OnDoorKicked.AddDynamic(this, &ACyberneticController::OnDoorKicked);
	}

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.UseGrouping = false;
	FString Name = "InvestigateStimulusActivity_" + FText::AsNumber(FGuid::NewGuid().A, &FormattingOptions).ToString();
	InvestigateStimulusActivity = NewObject<UInvestigateStimulusActivity>(this, FName(Name));

	TeamMoveActivity = NewObject<UMoveActivity>(this, UMoveActivity::StaticClass());
	MoveToActivity = NewObject<UMoveToActivity>(this, UMoveToActivity::StaticClass());
	MoveToActivity->bAbortActivityIfProjectedLocationFailed = true;
	MoveToActivity->bAbortActivityIfCannotReachLocation = true;
	MoveToActivity->bAbortMoveWhenActivityOverriden = true;
	MoveToActivity->bAbortIfNotMovingForAWhile = true;
	PushMoveToActivity = NewObject<UMoveToActivity>(this, UMoveToActivity::StaticClass());
	PushMoveToActivity->bAbortActivityIfProjectedLocationFailed = true;
	PushMoveToActivity->bAbortActivityIfCannotReachLocation = true;
	PushMoveToActivity->bAbortMoveWhenActivityOverriden = true;
	PushMoveToActivity->bAbortIfNotMovingForAWhile = true;
	AvoidanceMoveToActivity = NewObject<UMoveToActivity>(this, UMoveToActivity::StaticClass());
	AvoidanceMoveToActivity->bAbortActivityIfProjectedLocationFailed = true;
	AvoidanceMoveToActivity->bAbortActivityIfCannotReachLocation = true;
	AvoidanceMoveToActivity->bAbortMoveWhenActivityOverriden = true;
	AvoidanceMoveToActivity->bAbortIfNotMovingForAWhile = true;
	MoveToExitActivity = NewObject<UMoveToExitActivity>(this, UMoveToExitActivity::StaticClass());
	TargetNextCivilianActivity = NewObject<UTargetNextCivilianActivity>(this, UTargetNextCivilianActivity::StaticClass());
	
	AggressiveTags.Empty(15);
	AggressiveTags.Add("Gunshot");
	AggressiveTags.Add("SuppressedGunshot");
	AggressiveTags.Add("RamDoor");
	AggressiveTags.Add("ExplosionClose");
	AggressiveTags.Add("ExplosionMedium");
	AggressiveTags.Add("ExplosionFar");
	AggressiveTags.Add("BulletHitSurface");
	AggressiveTags.Add("AlarmTrap");
	AggressiveTags.Add("GrenadeBounce");
	AggressiveTags.Add("GrenadeExplosion");
	AggressiveTags.Add("TrapAlert");
	AggressiveTags.Add("KickDoor");
	AggressiveTags.Add("ExplodeDoor");
	AggressiveTags.Add("SWATYellForCompliance");
	AggressiveTags.Add("DeathScream");

	InvestigativeTags.Empty(15);
	InvestigativeTags = AggressiveTags;
	InvestigativeTags.Remove("SWATYellForCompliance");
	InvestigativeTags.Remove("GrenadeExplosion");
	InvestigativeTags.Remove("ExplosionClose");
	InvestigativeTags.Remove("RamDoor");
	InvestigativeTags.Remove("KickDoor");
	InvestigativeTags.Add("Footstep");
}

void ACyberneticController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentActivity)
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(CurrentActivity);
	}

	for (const UBaseActivity* Activity : ActivityQueue)
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(Activity);
	}
	
	GetWorld()->GetTimerManager().ClearAllTimersForObject(GetCombatActivity());

	for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		Door->OnDoorExploded.RemoveAll(this);
	}

	if (InvestigateStimulusActivity)
	{
		InvestigateStimulusActivity->ConditionalBeginDestroy();
		InvestigateStimulusActivity = nullptr;
	}
}

void ACyberneticController::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

#ifndef NO_BUENO
	SCOPE_CYCLE_COUNTER(STAT_CyberneticControllerTick);

	if (!IsValid(GetCharacter()))
	{
		ActorSightSenseMap.Empty();
		ActorSoundSenseMap.Empty();
		
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		
		ClearActivityList();
		
		if (CombatActivity)
		{
			CombatActivity->ConditionalBeginDestroy();
			CombatActivity = nullptr;
		}
		
		Destroy();
		return;
	}

	if (GetCharacter()->bDeactivated)
		return;
	
	if (!UReadyOrNotSignificanceManager::IsActorRelevant(GetCharacter()))
		return;

	if (GetCharacter()->IsBeingArrested() || GetCharacter()->IsIncapacitated())
	{
		FinishActivity(CurrentActivity, false, true);
		
		ClearActivityList();

		if (CombatActivity)
		{
			CombatActivity->FinishCombatMove();
			CombatActivity->FinishedActivity(false);
			CombatActivity->ConditionalBeginDestroy();
			CombatActivity = nullptr;
			
			GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::ForcedScript);
		}

		TargetFocalPoint = FVector::ZeroVector;
		GetCharacter()->Rep_FocalPoint = FVector::ZeroVector;
	}

	if (IsCivilian() && MoraleComponent->GetMorale() <= 0.001f && !GetCharacter()->IsStunned())
	{
		GetCharacter()->Surrender();
	}

	TargetingComponent->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	
	const bool bIsMoving = IsMoving();
	
	GetCharacter()->bIsMoving = bIsMoving;
	
	if (!bIsMoving)
	{
		TimeMoving = 0.0f;
		TimeSinceLastMove += DeltaTime;
	}
	else
	{
		TimeMoving += DeltaTime;
		TimeSinceLastMove = 0.0f;
	}

	if (bHasEverSpottedEnemyBefore)
	{
		TimeSinceFirstSpottedEnemy += DeltaTime;
	}

	TimeSinceLastExposedToAggressiveStimulus += DeltaTime;
	TimeSinceLastExposedToAnyStimulus += DeltaTime;
	TimeSinceLastExposedToSightStimulus += DeltaTime;
	TimeSinceLastExposedToSoundStimulus += DeltaTime;

	CallDelay = FMath::Max(CallDelay - DeltaTime, 0.0f);

	TimeUntilRecentlySeenCharactersClear = FMath::Max(TimeUntilRecentlySeenCharactersClear - DeltaTime, 0.0f);
	
	if (TimeUntilRecentlySeenCharactersClear <= 0.0f)
	{
		TimeUntilRecentlySeenCharactersClear = 3.0f;
		
		RecentlySeenCivilians.Empty();
		RecentlySeenSuspects.Empty();
		RecentlySeenSwat.Empty();
	}

	// awareness state has changed
	if (PreviousAwarenessState != AwarenessState)
	{
		OnAwarenessStateChanged();
		OnAwarenessStateChangedDelegate.Broadcast(this, PreviousAwarenessState, AwarenessState);
	}

	PreviousAwarenessState = AwarenessState;
	
	// Force alert if last one in team alive
	bool bAlertWhenLastAlive = false;
	if (AReadyOrNotGameMode* GM = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>())
	{
		if (IsSuspect())
			bAlertWhenLastAlive = GM->ShouldAlertSuspectWhenLastAlive();
		else if (IsCivilian())
			bAlertWhenLastAlive = GM->ShouldAlertCivilianWhenLastAlive();
	}

	#if WITH_EDITOR
	if (CVarAlertAIWhenLastAlive.GetValueOnAnyThread() == 0)
		bAlertWhenLastAlive = false;
	#endif
	
	if (IsLastAlive() && bAlertWhenLastAlive)
	{
		if (CombatActivity)
		{
			if (AwarenessState != EAIAwarenessState::Alerted)
			{
				GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Alert, 30.0f, 1.0f, 0.1f, 1);
				
				bStopDecisionMaking = false;
				
				// Force stop current combat move
				if (BestCombatMoveAction)
				{
					CombatActivity->EndAction(BestCombatMoveAction);
					BestCombatMoveAction->StartCooldown(this);
					BestCombatMoveAction = nullptr;
				}
				
				AwarenessState = EAIAwarenessState::Alerted;
			}
		
			// Force stop current combat move
			if (BestCombatMoveAction && BestCombatMoveAction->bDisallowWhenLastAlive)
			{
				CombatActivity->EndAction(BestCombatMoveAction);
				BestCombatMoveAction->StartCooldown(this);
				BestCombatMoveAction = nullptr;
			}
		}
	}
	else
	{
		if (IsSWAT())
		{
			if (TimeSinceLastExposedToAggressiveStimulus < 12.0f || RecentlySeenSuspects.Num() > 0 || GetTrackedTarget())
			{
				if (AwarenessState != EAIAwarenessState::Alerted)
					GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Alert, 30.0f, 1.0f, 0.1f, 1);

				AwarenessState = EAIAwarenessState::Alerted;
			}
			else
			{
				AwarenessState = EAIAwarenessState::Unalerted;
			}
		}
		else
		{
			if (GetCharacter()->IsArrestedOrSurrendered())
			{
				AwarenessState = EAIAwarenessState::Alerted;
				GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Sad, 30.0f, 1.0f, 0.1f, 1);
			}
			else
			{
				if (AwarenessState == EAIAwarenessState::Alerted &&
					TimeSinceLastExposedToAggressiveStimulus > 90.0f)
				{
					AwarenessState = EAIAwarenessState::Suspicious;
				}
				
				// Eventually fall back to unalerted phase...
				if (AwarenessState == EAIAwarenessState::Suspicious &&
					TimeSinceLastExposedToAggressiveStimulus > 180.0f)
				{
					AwarenessState = EAIAwarenessState::Unalerted;
				}
				
				if (RecentlySeenSwat.Num() > 0 || bHasEverSpottedEnemyBefore)
				{
					if (AwarenessState != EAIAwarenessState::Alerted)
						GetCharacter()->Multicast_ChangeFaceEmotion(ECharacterEmotion::Alert, 30.0f, 1.0f, 0.1f, 1);

					bHasEverSpottedSWAT = true;
					AwarenessState = EAIAwarenessState::Alerted;
				}
			}
		}
	}
	
	switch (AwarenessState)
	{
		case EAIAwarenessState::Unalerted:		UnalertTime += DeltaTime; break;
		case EAIAwarenessState::Suspicious:		SuspiciousTime += DeltaTime; break;
		case EAIAwarenessState::Alerted:		AlertTime += DeltaTime; break;
		default: break;
	}
	
	if (/*GetCharacter()->IsArrested() ||*/
		GetCharacter()->IsBeingArrested() ||
		GetCharacter()->IsDeadOrUnconscious() ||
		GetCharacter()->IsIncapacitated() ||
		GetCharacter()->IsCarried())
	{
		ActorSightSenseMap.Empty();
		ActorSoundSenseMap.Empty();
		GetCharacter()->Rep_HeadFocalPoint = FVector::ZeroVector;

		FinishActivity(CurrentActivity, false, true);
		
		ClearActivityList();

		if (CombatActivity)
		{
			CombatActivity->FinishCombatMove();
		}
		
		GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::ForcedScript);
		return;
	}

	// Sight config updates
	if (UAISenseConfig_Sight* SightSenseConfig = Cast<UAISenseConfig_Sight>(AIPerceptionComponent->GetSenseConfig(AIPerceptionComponent->GetDominantSenseID())))
	{
		if (AwarenessState == EAIAwarenessState::Alerted)
		{
			if (!bInAlertedStimulusState)
			{
				bInAlertedStimulusState = true;
				
				const float Range = AI_CONFIG_GET_FLOAT("AlertedSightRange", 10000.0f); // 100m

				SightSenseConfig->PeripheralVisionAngleDegrees = 160.0f; // (160 Half Angle = 320 Degrees Vision)
				SightSenseConfig->SightRadius = FMath::Max(Range, 5000.0f);
				SightSenseConfig->LoseSightRadius = FMath::Max(Range * 2.0f, 10000.0f);

				AIPerceptionComponent->RequestStimuliListenerUpdate();

				#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "Sight Radius %f", SightSenseConfig->SightRadius);
				#endif
			}
		}
		else if (AwarenessState == EAIAwarenessState::Suspicious)
		{
			if (!bInSuspiciousStimulusState)
			{
				bInSuspiciousStimulusState = true;

				const float Range = AI_CONFIG_GET_FLOAT("SuspiciousSightRange", 6000.0f); // 60m

				SightSenseConfig->PeripheralVisionAngleDegrees = 160.0f; // (160 Half Angle = 320 Degrees Vision)
				SightSenseConfig->SightRadius = FMath::Max(Range, 2000.0f);
				SightSenseConfig->LoseSightRadius = FMath::Max(Range * 2.0f, 4000.0f);

				AIPerceptionComponent->RequestStimuliListenerUpdate();

				#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "Sight Radius %f", SightSenseConfig->SightRadius);
				#endif
			}
		}
		else
		{
			if (bInAlertedStimulusState || bInSuspiciousStimulusState)
			{
				bInAlertedStimulusState = false;
				bInSuspiciousStimulusState = false;

				const float Range = AI_CONFIG_GET_FLOAT("UnalertedSightRange", 5000.0f); // 50m
				
				SightSenseConfig->PeripheralVisionAngleDegrees = 110.0f; // (110 Half Angle = 200 Degrees Vision)
				SightSenseConfig->SightRadius = FMath::Max(Range, 1000.0f);
				SightSenseConfig->LoseSightRadius = FMath::Max(Range * 2.0f, 2000.0f);

				AIPerceptionComponent->RequestStimuliListenerUpdate();
				
				#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "Sight Radius %f", SightSenseConfig->SightRadius);
				#endif
			}
		}
		
		const bool bIsMoveStyleStrafe = GetCharacter()->MoveStyle && GetCharacter()->MoveStyle->bIsStrafing;

		if (GetCharacter()->MoveStyle->ActiveGaitName == "sprint" && !bIsMoveStyleStrafe && IsMoving())
			SightSenseConfig->PeripheralVisionAngleDegrees /= 2;
	}

	for (auto& k : DamagedBy)
	{
		k.Value += DeltaTime;
	}
	
	TArray<AActor*> ActorsToRemoveFromMap;
	for (auto& k : LastHeardActorTime)
	{
		k.Value -= DeltaTime;

		if (k.Value <= 0.0f)
			ActorsToRemoveFromMap.Add(k.Key);
	}

	for (AActor* A : ActorsToRemoveFromMap)
	{
		LastHeardActorTime.Remove(A);
	}
	
	ActorsToRemoveFromMap.Empty();
	
	ActorSightSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		if (!Element.Actor)
		{
			return true;
		}
		
		FActorPerceptionBlueprintInfo PerceptionOfActor;
		if (AIPerceptionComponent->GetActorsPerception(Element.Actor, PerceptionOfActor))
		{
			for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
			{
				if (!Stimulus.IsValid())
					continue;
				
				if (Stimulus.Type.Name == RON_SENSE_SIGHT)
				{
					// Forgot old/stale/unseen actors
					if (Stimulus.GetAge() > Element.SenseForgetTime || Stimulus.IsExpired())
					{
						return true;
					}
				}
			}
		}
		else
		{
			return true;
		}

		return false;
	});
	
	ActorSoundSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		if (!Element.Actor)
		{
			return true;
		}
		
		FActorPerceptionBlueprintInfo PerceptionOfActor;
		if (AIPerceptionComponent->GetActorsPerception(Element.Actor, PerceptionOfActor))
		{
			for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
			{
				if (!Stimulus.IsValid())
					continue;
				
				if (Stimulus.Type.Name == RON_SENSE_HEARING)
				{
					// Forgot old/stale/unseen actors
					if (Stimulus.GetAge() > Element.SenseForgetTime || Stimulus.IsExpired())
					{
						return true;
					}
				}
			}
		}
		else
		{
			return true;
		}

		return false;
	});
	
	ActorDamageSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		if (!Element.Actor)
		{
			return true;
		}
		
		FActorPerceptionBlueprintInfo PerceptionOfActor;
		if (AIPerceptionComponent->GetActorsPerception(Element.Actor, PerceptionOfActor))
		{
			for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
			{
				if (!Stimulus.IsValid())
					continue;
				
				if (Stimulus.Type.Name == RON_SENSE_DAMAGE)
				{
					// Forgot old/stale/unseen actors
					if (Stimulus.GetAge() > Element.SenseForgetTime || Stimulus.IsExpired())
					{
						return true;
					}
				}
			}
		}
		else
		{
			return true;
		}

		return false;
	});

	TArray<FActorSense> SightSenseToRemove;
	for (FActorSense& Sense : ActorSightSenseMap)
	{
		Sense.SenseTime += DeltaTime;

		//LOG_NUMBER(Sense.SenseTime);

		if (Sense.SenseTime > Sense.SenseReactionTime)
		{
			OnSeenActor(Sense.Actor, Sense.Tag, Sense.Stimulus);
			SightSenseToRemove.Add(Sense);
		}
	}

	for (const FActorSense& Sense : SightSenseToRemove)
	{
		ActorSightSenseMap.Remove(Sense);
	}

	TArray<FActorSense> SoundSenseToRemove;
	for (FActorSense& Sense : ActorSoundSenseMap)
	{
		Sense.SenseTime += DeltaTime;

		//LOG_NUMBER(Sense.SenseTime);
				
		if (Sense.SenseTime > Sense.SenseReactionTime)
		{
			OnHeardActor(Sense.Actor, Sense.Tag, Sense.Stimulus, Sense.SenseForgetTime);
			SoundSenseToRemove.Add(Sense);
		}
	}
	
	for (const FActorSense& Sense : SoundSenseToRemove)
	{
		ActorSoundSenseMap.Remove(Sense);
	}
	
	TArray<FActorSense> DamageSenseToRemove;
	for (FActorSense& Sense : ActorDamageSenseMap)
	{
		Sense.SenseTime += DeltaTime;

		//LOG_NUMBER(Sense.SenseTime);
				
		if (Sense.SenseTime > Sense.SenseReactionTime)
		{
			OnDamagedByActor(Sense.Actor, Sense.Tag, Sense.Stimulus);
			DamageSenseToRemove.Add(Sense);
		}
	}
	
	for (const FActorSense& Sense : DamageSenseToRemove)
	{
		ActorDamageSenseMap.Remove(Sense);
	}
	
	TArray<FName> TagsToRemove;
	for (auto& It : ExposedToStimulusTags)
	{
		It.Value.TimeSinceExposed += DeltaTime;

		if (It.Value.TimeSinceExposed >= AIPerceptionComponent->GetSenseConfig(UAISense::GetSenseID<UAISense_Hearing>())->GetMaxAge())
		{
			TagsToRemove.AddUnique(It.Key);
		}
	}

	for (const FName& Tag : TagsToRemove)
	{
		ExposedToStimulusTags.Remove(Tag);
	}

	ExposedToStimulusTags.ValueSort([](const FExposedToNoise& Lhs, const FExposedToNoise& Rhs)
	{
		return Lhs.TimeSinceExposed < Rhs.TimeSinceExposed;
	});

	if (CombatActivity && !bStopUtilityTick)
	{
		if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
		{
			Archetype->Tick(this, DeltaTime);
			
			if (BestContinuousAction)
			{
				CombatActivity->PerformAction(BestContinuousAction, DeltaTime);
			}
				
			if (BestAction)
			{
				if (BestAction->bCommitUntilEnd)
				{
					if (!CombatActivity->PerformAction(BestAction, DeltaTime))
					{
						CombatActivity->EndAction(BestAction);
						BestAction->StartCooldown(this);
						BestAction = nullptr;
					}
				}
				else
				{
					CombatActivity->PerformAction(BestAction, DeltaTime);

					bool bForceEnd = false;
					if (const UAIAction* CustomAction = BestAction->GetCustomAction(this))
					{
						if (CustomAction->WantsAbort())
						{
							bForceEnd = true;
						}
					}

					if (!BestAction->CommitTimes.Find(this) || bForceEnd)
					{
						CombatActivity->EndAction(BestAction);
						BestAction->StartCooldown(this);
						BestAction = nullptr;
					}
				}
			}

			if (BestCombatMoveAction)
			{
				if (!CombatActivity->PerformAction(BestCombatMoveAction, DeltaTime))
				{
					CombatActivity->EndAction(BestCombatMoveAction);
					BestCombatMoveAction->StartCooldown(this);
					BestCombatMoveAction = nullptr;
				}
			}
		}
	}
	
	// don't perform anything, we can't run any logic anyway
	if (IsValid(GetCharacter()) && !GetCharacter()->IsActive() && !GetCharacter()->bSurrenderComplete && !GetCharacter()->bArrestComplete)
	{
		FinishActivity(CurrentActivity, false, true);
		CurrentActivity = nullptr;

		if (CombatActivity)
		{
			CombatActivity->FinishCombatMove();
		}
		
		ClearActivityList();
		return;
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_CyberneticControllerActivityWork);

		if (CombatActivity)
		{
			SCOPE_CYCLE_COUNTER(STAT_CyberneticController_PerformCombatActivity);
			
			PerformActivity(CombatActivity, DeltaTime);
		}

		if (CurrentActivity)
		{
			SCOPE_CYCLE_COUNTER(STAT_CyberneticController_PerformCurrentActivity);
			
			if (CurrentActivity->MaxActivityTime > 0.0f &&
				CurrentActivity->GetElapsedActivityTime() > CurrentActivity->MaxActivityTime)
			{
				#if !UE_BUILD_SHIPPING
				ULog::Info(CurrentActivity->GetName() + " | Reached Max Activity Time (" + FString::SanitizeFloat(CurrentActivity->MaxActivityTime) + "secs )");
				#endif
				
				FinishActivity(CurrentActivity, true, true);
			}
			else
			{
				PerformActivity(CurrentActivity, DeltaTime);
			}
		}
		else
		{
			UBaseActivity* NextActivity = nullptr;
			
			if (ActivityQueue.Num() > 0)
			{
				NextActivity = ActivityQueue[0];
				ActivityQueue.RemoveAt(0);
			}

			CurrentActivity = NextActivity;
			
			if (CurrentActivity)
			{
				if (CurrentActivity->HasStartedActivity() || CurrentActivity->IsActivityPaused())
				{
					CurrentActivity->ResumeActivity();
				}
				else
				{
					if (CurrentActivity->InitActivity(this))
						CurrentActivity->StartActivity(this);
				}
			}
		}
	}

	// perform world building activities
	{
		const bool bTrackingStimulus = GetTrackedTarget() || bHasEverSpottedEnemyBefore || bEverHeardAggressiveStimulus;
		
		// slight delay before performing first activities so everything can be initalized etc
		// if activity route has a drop weapon activity but the AI doesn't have the weapon equipped yet this can bork
		if (!bTrackingStimulus && AwarenessState == EAIAwarenessState::Unalerted && GetGameTimeSinceCreation() > 1.0f)
		{
			const int32 MaxWorldBuilding = AI_CONFIG_GET_INT("MaxWorldBuildingActivities", 0);
			const int32 NumPerformingWorldBuilding = USuspectsAndCivilianManager::Get(this)->GetNumPerformingWorldBuilding();

			bool bReachedMaxGlobalLimit = NumPerformingWorldBuilding >= MaxWorldBuilding;
			
			if (MaxWorldBuilding == 0 || NumPerformingWorldBuilding == 0)
				bReachedMaxGlobalLimit = false;
			
			const bool bOnCooldown = UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TH_WorldBuildingRouteCooldown);
			
			if (!bReachedMaxGlobalLimit && !bOnCooldown)
			{
				FActivityRouteCollection* Route = &GetCharacter()->ActivityRouteCollection;
				
				if (const FActivityRoute* CurrentRoute = Route->GetCurrentActivity())
				{
					if (!HasActivityType(UWorldBuildingActivity::StaticClass()))
					{
						if (CurrentRoute->WorldBuildingPlacementActor &&
							(!CurrentRoute->WorldBuildingPlacementActor->InUseByController || CurrentRoute->WorldBuildingPlacementActor->InUseByController == this))
						{
							if (CurrentRoute->WorldBuildingPlacementActor->GiveNextActivityForController(this, *CurrentRoute))
							{
								bPendingNextActivityRoute = false;
							}
						}
					}
				}
				else
				{
					if (!HasActivityType(UWorldBuildingActivity::StaticClass()))
					{
						if (Route->bReturnToOriginalSpot)
						{
							GiveMoveTo(GetCharacter()->OriginalSpawnLocation);
						}
						
						bPendingNextActivityRoute = false;
						
						UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_WorldBuildingRouteCooldown, this, &ACyberneticController::RestartActivityRouteCollection, Route->Cooldown, false);
					}
				}
			}
		}
	}

	// Suspects and civlians avoid doors
	if (!CurrentActivity && (!CombatActivity || !CombatActivity->GetCombatMoveActivity()) && !BestAction)
	{
		if (IsSuspect() || IsCivilian())
		{
			AThreatAwarenessActor* TAA = GetTargetingComp()->GetNearestThreat();
			FRoom* CurrentRoom = nullptr;
			if (TAA)
			{
				CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(TAA->OwningRoom);
			}

			if (CurrentRoom)
			{
				for (ADoor* Door : CurrentRoom->AdditionalRootDoors)
				{
					if (Door && !Door->IsDoorwayOnly() )
					{
						FTransform Transform;
						Transform.SetLocation(Door->GetDoorMesh()->GetComponentLocation() + Door->GetDoorMesh()->GetRightVector() * 60.0f + FVector::UpVector * 115.0f);
						Transform.SetRotation(Door->GetDoorMesh()->GetComponentQuat());

						const float DoorWidth = Door->GetDoorSize().Y + 15.0f;
						
						FVector Extent = FVector(85.0f, DoorWidth, 120.0f);

						//DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 1.0f);
						
						const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->GetActorLocation(), Transform, Extent);

						if (bInsideDoorThreshold)
						{
							if (Door->IsActorInFrontOfDoor(GetCharacter()))
							{
								GiveMoveTo(Transform.GetLocation() + Door->GetDoorMesh()->GetForwardVector() * 150.0f);
							}
							else
							{
								GiveMoveTo(Transform.GetLocation() + Door->GetDoorMesh()->GetForwardVector() * -150.0f);
							}
						}
					}
				}
			}
		}
	}
#endif // NO_BUENO
}

void ACyberneticController::PushCharacter(FVector ToLocation, bool bNoMoveBack)
{
	if (!GetCharacter() || !GetWorld())
		return;

	// Don't try to be pushed, since we're melee'n, we wanna be up close
	if (GetCharacter()->GetEquippedItem<AMeleeWeapon>()) 
	{
		return;
	}

	if (BestContinuousAction)
	{
		if (BestContinuousAction->ActionType == EAIAction::Melee)
			return;
	}

	if (GetCombatActivity())
	{
		if (GetCombatActivity()->IsRunningCombatMoveActivity(UChargeCombatMove::StaticClass()))
			return;
		
		if (GetCombatActivity()->GetCombatEngagementType() == ECombatEngagementType::Melee)
			return;
	}

	if (GetCurrentActivity())
	{
		if (!GetCurrentActivity()->CanBePushed())
			return;
	}

	if (GetCharacter()->IsFullBodyMontagePlaying())
		return;
	
	if (bNoMoveBack)
	{
		if (GetCurrentActivity())
		{
			GetCurrentActivity()->SetLocation(ToLocation);
		}
	}
	
	FHitResult WallTest;
	FCollisionObjectQueryParams CollisionObjectParams;
	CollisionObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
	GetWorld()->LineTraceSingleByObjectType(WallTest, GetCharacter()->GetActorLocation(), ToLocation, CollisionObjectParams, CollisionQueryParams);

	#if !UE_BUILD_SHIPPING
	if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
		DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), GetImmediateMoveDestination(), FColor::Yellow, false, 0.1f, 0 , 1);
	#endif

	if (WallTest.bBlockingHit)
	{
		ToLocation = WallTest.ImpactPoint;
	}

	if (!HasActivityType(UMoveToActivity::StaticClass()))
	{
		PushMoveToActivity->SetLocation(ToLocation);
		
		if (PushMoveToActivity->GetLocation() != FVector::ZeroVector)
		{
			UActivityManager::GiveActivityTo(PushMoveToActivity, GetCharacter(), true, false);
		}
	}
	else if (UMoveToActivity* MoveActivity = GetCurrentActivity<UMoveToActivity>())
	{
		MoveActivity->SetLocation(ToLocation);
	}
}

#if !UE_BUILD_SHIPPING
void ACyberneticController::DisplayAIDebugInfo(float DeltaTime)
{
	if (CVarUtilityAIDebug.GetValueOnAnyThread() > 0)
	{
		UAIArchetypeData* Archetype = GetCharacter()->Archetype;
		if (Archetype && CombatActivity)
		{
			SCOPE_CYCLE_COUNTER(STAT_CyberneticController_DebugPrint);
			
			FString DebugMessage = Archetype->Name;

			if (Archetype->Name.IsEmpty())
				DebugMessage = Archetype->GetName() + " | " + FString::Printf(TEXT("Tick Rate: %.3f"), PrimaryActorTick.TickInterval);

			DebugMessage += " | " + RON_ENUM_TO_STRING(EAIAwarenessState, AwarenessState);
			
			//DebugMessage += LINE_TERMINATOR;
			//DebugMessage += " | " + FString("Tick Rate: ") + FString::SanitizeFloat(PrimaryActorTick.TickInterval);
			
			if (TargetingComponent->GetTrackingType() == ETargetingCompTracking::TCT_TrackingVisibleTarget && GetTrackedTarget())
				DebugMessage += " | Tracking: " + GetNameSafe(GetTrackedTarget());
			else
				DebugMessage += " | Tracking: " + RON_ENUM_TO_STRING(ETargetingCompTracking, TargetingComponent->GetTrackingType());

			#if WITH_EDITOR
			DebugMessage += LINE_TERMINATOR;
			DebugMessage += GetNameSafe(GetCharacter());
			#endif
			
			DebugMessage += LINE_TERMINATOR;
			DebugMessage += "Move Style: " + GetCharacter()->MoveStyle->ActiveMoveStyle.Name.ToString();

			// is it an override?
			if (GetCharacter()->bOverridingMoveStyle)
				DebugMessage += " (Override)";
				
			DebugMessage += LINE_TERMINATOR;
			DebugMessage += "Morale: " + FString::Printf(TEXT("%.2f"), MoraleComponent->GetMorale()) + " | Stress: " + FString::Printf(TEXT("%.2f"), GetCharacter()->Stress);
			
			uint32 NumDamageDisplay = 0;
			{
				for (const auto& It : MoraleComponent->GetMoraleDamageHistory())
				{
					if (It.Key != NAME_None)
					{
						if (It.Value.Delta > 0.0f && It.Value.TimeSinceChange < 5.0f)
						{
							NumDamageDisplay++;
							
							DebugMessage += LINE_TERMINATOR;
							DebugMessage += It.Key.ToString() + " Morale Damage: " + FString::Printf(TEXT("%.3f"), It.Value.Delta);

						}
					}
				}
			}
			
			{
				uint32 Num = MoraleComponent->GetMoraleGainHistory().Num();
				if (NumDamageDisplay > 0)
				{
					DebugMessage += LINE_TERMINATOR;
					DebugMessage += "                           ";
				}
				
				uint32 i = 0;
				for (const auto& It : MoraleComponent->GetMoraleGainHistory())
				{
					if (It.Key != NAME_None)
					{
						if (It.Value.Delta > 0.0f && It.Value.TimeSinceChange < 5.0f)
						{
							if (i == 0)
							{
								DebugMessage += LINE_TERMINATOR;
							}
							
							DebugMessage += It.Key.ToString() + " Morale Gain: " + FString::Printf(TEXT("%.3f"), It.Value.Delta);

							if (i > 0 && i < Num)
							{
								DebugMessage += LINE_TERMINATOR;
								DebugMessage += "                           ";
							}
							
							i++;
						}
					}
				}
			}

			if (GetCharacter()->FactionManager)
			{
				FString FactionDebugInfo = GetCharacter()->FactionManager->GetFactionDebugInfo(GetCharacter());
				if (!FactionDebugInfo.IsEmpty())
				{
					DebugMessage += LINE_TERMINATOR;
					DebugMessage += FactionDebugInfo;
				}
			}
			
			DebugMessage += "\n--------------\n";

			if (Archetype->bEnableAlertActions || Archetype->bEnableUnalertActions)
			{
				if (BestAction)
				{
					if (float* ScorePtr = BestAction->Scores.Find(this))
					{
						const FString BestActionInfo = FString::Printf(TEXT("Best Action: %s | %.3f"), *BestAction->Name.ToString(), *ScorePtr);
						DebugMessage += BestActionInfo;
						
						FString DebugInfo = CombatActivity->DebugActionData(BestAction);
						if (!DebugInfo.IsEmpty())
						{
							DebugMessage += LINE_TERMINATOR;
							DebugMessage += DebugInfo;
						}
						
						DebugMessage += "\n--------------\n";
					}
				}
				else
				{
					const FString BestActionInfo = FString::Printf(TEXT("Best Action: None"));
					DebugMessage += BestActionInfo;
					DebugMessage += "\n--------------\n";
				}
			}

			if (Archetype->bEnableContinuousActions)
			{
				if (BestContinuousAction)
				{
					if (float* ScorePtr = BestContinuousAction->Scores.Find(this))
					{
						const FString BestActionInfo = FString::Printf(TEXT("Best Continuous Action: %s | %.3f"), *BestContinuousAction->Name.ToString(), *ScorePtr);
						DebugMessage += BestActionInfo;
						
						FString DebugInfo = CombatActivity->DebugActionData(BestContinuousAction);
						if (!DebugInfo.IsEmpty())
						{
							DebugMessage += LINE_TERMINATOR;
							DebugMessage += DebugInfo;
						}
						
						DebugMessage += "\n--------------\n";
					}
				}
				else
				{
					const FString BestActionInfo = FString::Printf(TEXT("Best Continuous Action: None"));
					DebugMessage += BestActionInfo;
					DebugMessage += "\n--------------\n";
				}
			}
			
			if (Archetype->bEnableCombatMoveActions)
			{
				if (BestCombatMoveAction)
				{
					if (float* ScorePtr = BestCombatMoveAction->Scores.Find(this))
					{
						DebugMessage += FString::Printf(TEXT("Best Combat Move Action: %s | %.3f\n"), *BestCombatMoveAction->Name.ToString(), *ScorePtr);;
						if (float* Time = BestCombatMoveAction->CommitTimes.Find(this))
							DebugMessage += FString::Printf(TEXT("Commit Time Remaining: %.3f\n"), *Time);
						
						FString DebugInfo = CombatActivity->DebugActionData(BestCombatMoveAction);
						if (!DebugInfo.IsEmpty())
						{
							DebugMessage += DebugInfo;
						}
						
						if (UBaseCombatMoveActivity* CombatMoveActivity = CombatActivity->GetCombatMoveActivity())
						{
							FString CombatMoveDebugInfo;
							CombatMoveActivity->GatherDebugString(CombatMoveDebugInfo);
							
							DebugMessage += CombatMoveDebugInfo;
						}
					}
				}
				else
				{
					DebugMessage += "Best Combat Move Action: None";
				}
			}
			
			if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
			{
				const float Distance = FVector::Distance(CameraManager->GetCameraLocation(), GetCharacter()->GetActorLocation());
				if (UReadyOrNotSignificanceManager::IsActorRelevant(GetCharacter()) && Distance < 3000.0f)
				{
					DrawDebugString(GetWorld(), GetCharacter()->GetMesh()->GetBoneLocation("spine_3"), DebugMessage, nullptr, FColor::White, DeltaTime - 0.01f, true);
				}
			}
		
			CombatActivity->UnableToFireReason = "";
		}
		
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const float Distance = FVector::Distance(CameraManager->GetCameraLocation(), GetCharacter()->GetActorLocation());
			if (UReadyOrNotSignificanceManager::IsActorRelevant(GetCharacter()) && Distance < 3000.0f)
			{
				if (TargetingComponent->bHasLOSToTarget)
				{
					if (ABaseMagazineWeapon* bmw = Cast<ABaseMagazineWeapon>(GetCharacter()->GetEquippedItem()))
					{
						const FVector& FireDirection = bmw->GetBulletSpawn()->GetForwardVector();
						const FVector& DirectionToFocalPoint = (GetCharacter()->Rep_FocalPoint - bmw->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
						const float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);
							
						DrawDebugString(GetWorld(), GetCharacter()->GetMesh()->GetBoneLocation("head"), "Aiming: " + FString::Printf(TEXT("%.3f"), TargetRotationDelta), nullptr, FColor::White, DeltaTime - 0.01f, true);
					}
				}
			}
		}
	}
}
#endif

bool ACyberneticController::LineOfSightTo(const AActor* Other, FVector ViewPoint, bool bAlternateChecks) const
{
	SCOPE_CYCLE_COUNTER(STAT_CyberneticController_AILineOfSight)
	if (!GetCharacter())
		return false;
	
	if (!Other)
		return false;

	FCollisionQueryParams CollisionParams;
	CollisionParams = GetCharacter()->GetCollisionQueryParameters();
	CollisionParams.AddIgnoredActor(Other);
	CollisionParams.TraceTag = SCENE_QUERY_STAT(LineOfSight);
	CollisionParams.bTraceComplex = true;

	if (const AReadyOrNotCharacter* OtherCharacter = Cast<AReadyOrNotCharacter>(Other))
	{
		CollisionParams.AddIgnoredActors(OtherCharacter->GetCollisionIgnoredActors());
		CollisionParams.AddIgnoredComponents(OtherCharacter->GetCollisionIgnoredComponents());
	}

	if (ViewPoint == FVector::ZeroVector)
	{
		FRotator ViewRotation;
		GetActorEyesViewPoint(ViewPoint, ViewRotation);

		// if we still don't have a view point we simply fail
		if (ViewPoint == FVector::ZeroVector)
		{
			return false;
		}
	}

	FVector TargetLocation = Other->GetTargetLocation(GetPawn());
	FVector RightVector = Other->GetActorRightVector();

	if (TargetLocation == FVector::ZeroVector || ViewPoint == FVector::ZeroVector)
		return false;

	FHitResult HeadHit;
	GetWorld()->LineTraceSingleByChannel(HeadHit, ViewPoint, TargetLocation, ECC_Visibility, CollisionParams);
	//DrawDebugLine(GetWorld(), HeadHit.TraceStart,HeadHit.TraceEnd, FColor::Green, false, 1.0f);
	if (!HeadHit.bBlockingHit && RightVector != FVector::ZeroVector)
	{
		FHitResult HeadHitReverse;
		GetWorld()->LineTraceSingleByChannel(HeadHitReverse, TargetLocation + RightVector * 5.0f, ViewPoint + RightVector * 5.0f, ECC_Visibility, CollisionParams);
		return !HeadHitReverse.bBlockingHit;
	}
	
	return !HeadHit.bBlockingHit;
}

void ACyberneticController::OnPerceptionUpdated(const TArray<AActor*>& TestActors)
{
#ifndef NO_BUENO
	if (!GetCharacter())
		return;

	if (GetCharacter()->IsDeadOrUnconscious() || GetCharacter()->IsInRagdoll())
		return;

	if (GetCharacter()->bDeactivated)
		return;

	if (bDisableSensePerception)
		return;

	NewEnemies = 0;
	NewNeutrals = 0;
	
	for (AActor* Actor : TestActors)
	{
		if (!IsValid(Actor))
			continue;
		
		if (Actor == this || Actor == GetPawn())
			continue;

		FActorPerceptionBlueprintInfo PerceptionOfActor;
		AIPerceptionComponent->GetActorsPerception(Actor, PerceptionOfActor);

		for (FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
		{
			if (!Stimulus.IsValid())
				continue;

			if (Stimulus.IsExpired())
				continue;
			
			if (Stimulus.StimulusLocation == FVector::ZeroVector)
			{
				Stimulus.StimulusLocation = Actor->GetActorLocation();
			}
			
			LatestStimulus = Stimulus;
			
			if (Stimulus.Type.Name == RON_SENSE_SIGHT)
			{
				if (Stimulus.GetAge() <= LatestSightStimulus.GetAge())
				{
					LatestSightStimulus = Stimulus;
					ProcessActorSightStimulus(Actor, Stimulus);
				}
			}
			else if (Stimulus.Type.Name == RON_SENSE_HEARING)
			{
				if (Stimulus.GetAge() <= LatestHearingStimulus.GetAge())
				{
					LatestHearingStimulus = Stimulus;
					ProcessActorHearingStimulus(Actor, Stimulus);
				}
			}
			else if (Stimulus.Type.Name == RON_SENSE_DAMAGE)
			{
				if (Stimulus.GetAge() <= LatestDamageStimulus.GetAge())
				{
					LatestDamageStimulus = Stimulus;
					ProcessActorDamageStimulus(Actor, Stimulus);
				}
			}

			ProcessStimuli(Stimulus, Actor, PerceptionOfActor);
		}
	}

	if (IsSWAT())
	{
		OnRespondToSpottedSuspectAndCivilian();
	}
#endif
}

void ACyberneticController::ProcessActorSightStimulus(AActor* SensedActor, FAIStimulus Stimulus)
{
	AActor* Actor = SensedActor;
	
	// Notify the actor that it has been sensed by this AI
	{
		AActor* OverridenSensedActor = nullptr;
		if (Actor->Implements<UReceiveAISenseUpdates>())
		{
			IReceiveAISenseUpdates::Execute_OnAIPerceptionSense(Actor, this, Stimulus, OverridenSensedActor);
		}

		if (OverridenSensedActor)
		{
			Actor = OverridenSensedActor;
		}
	}

	// Broadcast sight sense updates to activties
	{
		AActor* OverridenSensedActor = Actor;

		if (GetCurrentActivity())
			IReceiveAISenseUpdates::Execute_OnAIPerceptionSense(GetCurrentActivity(), this, Stimulus, OverridenSensedActor);

		if (GetCombatActivity())
		{
			IReceiveAISenseUpdates::Execute_OnAIPerceptionSense(GetCombatActivity(), this, Stimulus, OverridenSensedActor);

			if (GetCombatActivity()->GetCombatMoveActivity())
				IReceiveAISenseUpdates::Execute_OnAIPerceptionSense(GetCombatActivity()->GetCombatMoveActivity(), this, Stimulus, OverridenSensedActor);
		}
	}
}

void ACyberneticController::ProcessActorHearingStimulus(AActor* SensedActor, FAIStimulus Stimulus)
{
	AActor* Actor = SensedActor;
	
	// Notify the actor that it has been sensed by this AI
	{
		AActor* OverridenSensedActor = nullptr;
		if (Actor->Implements<UReceiveAISenseUpdates>())
		{
			IReceiveAISenseUpdates::Execute_OnAIHearingSense(Actor, this, Stimulus, OverridenSensedActor);
		}

		if (OverridenSensedActor)
		{
			Actor = OverridenSensedActor;
		}
	}
	
	// Broadcast hearing sense updates to activties
	{
		AActor* OverridenSensedActor = Actor;

		if (GetCurrentActivity())
			IReceiveAISenseUpdates::Execute_OnAIHearingSense(GetCurrentActivity(), this, Stimulus, OverridenSensedActor);

		if (GetCombatActivity())
		{
			IReceiveAISenseUpdates::Execute_OnAIHearingSense(GetCombatActivity(), this, Stimulus, OverridenSensedActor);

			if (GetCombatActivity()->GetCombatMoveActivity())
				IReceiveAISenseUpdates::Execute_OnAIHearingSense(GetCombatActivity()->GetCombatMoveActivity(), this, Stimulus, OverridenSensedActor);
		}
	}
}

void ACyberneticController::ProcessActorDamageStimulus(AActor* SensedActor, FAIStimulus Stimulus)
{
	AActor* Actor = SensedActor;
	
	// Notify the actor that it has been sensed by this AI
	{
		AActor* OverridenSensedActor = nullptr;
		if (Actor->Implements<UReceiveAISenseUpdates>())
		{
			IReceiveAISenseUpdates::Execute_OnAIDamageSense(Actor, this, Stimulus, OverridenSensedActor);
		}

		if (OverridenSensedActor)
		{
			Actor = OverridenSensedActor;
		}
	}
	
	// Broadcast damage sense updates to activties
	{
		AActor* OverridenSensedActor = Actor;

		if (GetCurrentActivity())
			IReceiveAISenseUpdates::Execute_OnAIDamageSense(GetCurrentActivity(), this, Stimulus, OverridenSensedActor);

		if (GetCombatActivity())
		{
			IReceiveAISenseUpdates::Execute_OnAIDamageSense(GetCombatActivity(), this, Stimulus, OverridenSensedActor);

			if (GetCombatActivity()->GetCombatMoveActivity())
				IReceiveAISenseUpdates::Execute_OnAIDamageSense(GetCombatActivity()->GetCombatMoveActivity(), this, Stimulus, OverridenSensedActor);
		}
	}
}

void ACyberneticController::ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor)
{
}

AReadyOrNotCharacter* ACyberneticController::SensedActorToCharacter(AActor* SensedActor) const
{
	const AController* SensedController = Cast<AController>(SensedActor);
	if (!SensedController)
	{
		// Sensed a player and not a controller
		if (const AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(SensedActor))
		{
			SensedController = SensedCharacter->GetController();
		}
	}

	AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(SensedActor);
	if (SensedController && !SensedCharacter)
	{
		SensedCharacter = Cast<AReadyOrNotCharacter>(SensedController->GetPawn());
	}
	
	if (IsValid(SensedCharacter))
	{
		// Don't deal with inactive people
		//if (!SensedCharacter->IsActive())
		//{
		//	SensedCharacter = nullptr;
		//}
	}
	else
	{
		SensedCharacter = nullptr;
	}

	if (SensedCharacter)
	{
		#if !UE_BUILD_SHIPPING
		if (SensedCharacter->HasNoTarget())
		{
			SensedCharacter = nullptr;
		}
		#endif
	}

	return SensedCharacter;
}

void ACyberneticController::OnRespondToSpottedSuspectAndCivilian()
{
	if(NewEnemies == 0 && NewNeutrals == 0)
		return;

	FString VoiceLine = "";

	// New Enemies
	if(NewEnemies >= 1 && NewNeutrals == 0)
	{
		if(NewEnemies > 1)
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_SUSPECTS;
		}
		else
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_SUSPECT;
		}
	}
	// New Neutrals
	else if(NewNeutrals >= 1 && NewEnemies == 0)
	{
		if(NewNeutrals > 1)
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS;
		}
		else
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN;
		}
	}
	// Mixed group found
	else if(NewEnemies >= 1 && NewNeutrals >= 1)
	{
		if(NewEnemies == 1 &&  NewNeutrals == 1)
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN_AND_SUSPECT;
		}
		else
		{
			VoiceLine = VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS_AND_SUSPECTS;
		}			
	}

	if (CallDelay <= 0.0f && VoiceLine.IsEmpty() == false)
	{
		GetCharacter()->PlayRawVO(VoiceLine);
		CallDelay = 2.0f;
	}
}

void ACyberneticController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(InPawn))
	{
		AICharacter->OnAIFinishSpawning.RemoveAll(this);
		AICharacter->OnAIFinishSpawning.AddDynamic(this, &ACyberneticController::OnAIFinishSpawning);
		
		// Apply a team ID so the perception comp can associate with neutral/friendlies/enemies etc
		switch (GetTeam())
		{
			case ETeamType::TT_SQUAD:			SetGenericTeamId(FGenericTeamId(1)); break;
			case ETeamType::TT_SERT_BLUE:		SetGenericTeamId(FGenericTeamId(1)); break;
			case ETeamType::TT_SERT_RED:		SetGenericTeamId(FGenericTeamId(1)); break;
			case ETeamType::TT_SUSPECT:			SetGenericTeamId(FGenericTeamId(2)); break;
			case ETeamType::TT_CIVILIAN:		SetGenericTeamId(FGenericTeamId(3)); break;
			default: break;
		}
		
		MoraleComponent->OwnerCharacter = AICharacter;
		
		if (IsSWAT())
		{
			DESTROY_COMPONENT(MoraleComponent)
		}
		else
		{
			if (IsSuspect())
			{
				const float MinMorale = AI_CONFIG_GET_FLOAT("SuspectMinMorale", 0.5f);
				const float MaxMorale = AI_CONFIG_GET_FLOAT("SuspectMaxMorale", 1.0f);

				MoraleComponent->StartingMorale = FMath::RandRange(MinMorale, MaxMorale);
				MoraleComponent->SetResource(MoraleComponent->StartingMorale);
				
				GetCharacter()->StartingStress = AI_CONFIG_GET_FLOAT("SuspectStartingStress", 0.25f);
				GetCharacter()->Stress = GetCharacter()->StartingStress;
			}
			else
			{
				const float MinMorale = AI_CONFIG_GET_FLOAT("CivilianMinMorale", 0.5f);
				const float MaxMorale = AI_CONFIG_GET_FLOAT("CivilianMaxMorale", 1.0f);
				
				MoraleComponent->StartingMorale = FMath::RandRange(MinMorale, MaxMorale);
				MoraleComponent->SetResource(MoraleComponent->StartingMorale);
				
				GetCharacter()->StartingStress = AI_CONFIG_GET_FLOAT("CivilianStartingStress", 0.25f);
				GetCharacter()->Stress = GetCharacter()->StartingStress;
			}
		}
		
		if (IsSWAT())
		{
			if (!GetCombatActivity<USwatCombatActivity>())
			{
				CombatActivity = NewObject<USwatCombatActivity>(this, USwatCombatActivity::StaticClass());
				if (CombatActivity->InitActivity(this))
					CombatActivity->StartActivity(this);
			}
		}
		
		if (IsSuspect())
		{
			if (!GetCombatActivity<USuspectCombatActivity>())
			{
				CombatActivity = NewObject<USuspectCombatActivity>(this, USuspectCombatActivity::StaticClass());
				if (CombatActivity->InitActivity(this))
					CombatActivity->StartActivity(this);
			}
		}
		else if (IsCivilian())
		{
			if (!GetCombatActivity<UCivilianCombatActivity>())
			{
				CombatActivity = NewObject<UCivilianCombatActivity>(this, UCivilianCombatActivity::StaticClass());
				if (CombatActivity->InitActivity(this))
					CombatActivity->StartActivity(this);
			}
		}
		
		if (IsSuspect())
		{
			USuspectsAndCivilianManager::Get(this)->Suspects.AddUnique(AICharacter);
		}
		else if (IsCivilian())
		{
			USuspectsAndCivilianManager::Get(this)->Civilians.AddUnique(AICharacter);
		}

		TargetingComponent->EngagementTimeUntilReachedLastBoneZone = FMath::Clamp(AI_CONFIG_GET_FLOAT("EngagementTimeUntilReachedLastBoneZone", 4.0f), 0.01f, 60.0f);
		TargetingComponent->BoneRetargetingRate = FMath::Clamp(AI_CONFIG_GET_FLOAT("BoneRetargetingRate", 1.0f), 0.1f, 5.0f);
		
		if (IsSuspect())
		{
			TargetingComponent->RequiredTimeTrackingTarget = FMath::Max(AI_CONFIG_GET_FLOAT("SuspectRequiredTimeSpentOnTarget"), 0.0f);
			TargetingComponent->LastKnownTrackingTime = AI_CONFIG_GET_FLOAT("SuspectTrackLastKnownPositionTime", 60.0f);
		}
		else if (IsSWAT())
		{
			TargetingComponent->RequiredTimeTrackingTarget = FMath::Max(AI_CONFIG_GET_FLOAT("SWATRequiredTimeSpentOnTarget"), 0.0f);
			TargetingComponent->LastKnownTrackingTime = AI_CONFIG_GET_FLOAT("SwatTrackLastKnownPositionTime", 60.0f);
		}
		
		if (UAISenseConfig_Sight* SightSenseConfig = Cast<UAISenseConfig_Sight>(AIPerceptionComponent->GetSenseConfig(AIPerceptionComponent->GetDominantSenseID())))
		{
			const float Range = AI_CONFIG_GET_FLOAT("UnalertedSightRange", 5000.0f); // 50m
			
			SightSenseConfig->PeripheralVisionAngleDegrees = 110.0f; // (110 Half Angle = 200 Degrees Vision)
			SightSenseConfig->SightRadius = FMath::Max(Range, 1000.0f);
			SightSenseConfig->LoseSightRadius = FMath::Max(Range * 2.0f, 2000.0f);

			AIPerceptionComponent->RequestStimuliListenerUpdate();
		}
	}
}

void ACyberneticController::OnUnPossess()
{
	CustomActions.Empty();
	
	if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
	{
		Archetype->RemoveAIData(this);
	}

	if (CurrentActivity)
	{
		CurrentActivity->FinishedActivity_NoOwner(false);
	}
	
	ClearActivityList();

	Super::OnUnPossess();
}

void ACyberneticController::OnAIFinishSpawning()
{
	if (GetCharacter())
	{
		if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
		{
			// Spawn all custom actions
			Archetype->CreateCustomActionsIfNeeded(this);

			// Override morale if specified
			if (Archetype->bMoraleOverride)
			{
				MoraleComponent->StartingMorale = FMath::RandRange(Archetype->MinMorale, Archetype->MaxMorale);
				MoraleComponent->SetMaxResource(1.0f);
				MoraleComponent->SetResource(MoraleComponent->StartingMorale);
			}

			Archetype->AuditActions();
		}
		
		GetCharacter()->ActivityRouteCollection.ActivityRoutes.RemoveAll([&](FActivityRoute& Route)
		{
			return Route.WorldBuildingPlacementActor == nullptr;
		});

		// try landmark spawn first, then worldbuilding activity
		if (ACoverLandmark* Landmark = GetCharacter()->GetSpawnData()->SpawnInLandmark.LoadSynchronous())
		{
			FVector EntryLocation = Landmark->GetActorLocation();
			if (Landmark->EntryPoints.Num() > 0)
			{
				if (const ACoverLandmarkProxy* Proxy = Landmark->EntryPoints[FMath::RandRange(0, Landmark->EntryPoints.Num()-1)])
				{
					EntryLocation = Proxy->GetActorLocation();
				}
			}
			
			FVector Projected = EntryLocation;
			UReadyOrNotAISystem::ProjectPointToNav(Projected, Projected);
			
			GetCharacter()->SetActorLocation(Projected + FVector(0.0f, 0.0f, 70.0f));

			CombatActivity->StartRunningCombatMove(CombatActivity->HardCoverCombatMove);
			CombatActivity->HardCoverCombatMove->GiveTakeCoverAtLandmarkActivity(Landmark);
		}
		else
		{
			if (GetCharacter()->ActivityRouteCollection.bSpawnAtFirstRoute)
			{
				const int32 MaxWorldBuilding = AI_CONFIG_GET_INT("MaxWorldBuildingActivities", 0);
				const int32 NumPerformingWorldBuilding = USuspectsAndCivilianManager::Get(this)->GetNumPerformingWorldBuilding();

				bool bReachedMaxGlobalLimit = NumPerformingWorldBuilding >= MaxWorldBuilding;
				
				if (MaxWorldBuilding == 0 || NumPerformingWorldBuilding == 0)
					bReachedMaxGlobalLimit = false;
				
				if (GetCharacter()->ActivityRouteCollection.ActivityRoutes.Num() > 0 && !bReachedMaxGlobalLimit)
				{
					FVector Projected = GetCharacter()->ActivityRouteCollection.ActivityRoutes[0].WorldBuildingPlacementActor->GetActorLocation();
					UReadyOrNotAISystem::ProjectPointToNav(Projected, Projected);
					
					GetCharacter()->SetActorLocation(Projected + FVector(0.0f, 0.0f, 70.0f));
				}
			}
		}
		
		GetCharacter()->ActivityRouteCollection.ActivityIdx = 0;
	}
}

void ACyberneticController::TryStopCurrentConversation()
{
	// if currently playing a conversation, immediately stop
	if (AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		ls->GetConversationManager()->StopConversationForSpeaker(GetCharacter());
	}
}

void ACyberneticController::OnAwarenessStateChanged()
{
	TryStopCurrentConversation();
}

void ACyberneticController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Moved to RCharacterMovementComponent so it can be included in ScopedMovementUpdates
}

void ACyberneticController::LookAt(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_CyberneticController_LookAt);
	
	if (ACyberneticCharacter* OwningCharacter = GetCharacter())
	{
		if (OwningCharacter->IsArrested())
			return;
		
		if (!UReadyOrNotSignificanceManager::IsActorRelevant(GetCharacter()))
			return;
		
		EAIFocusPriority::Type FocusType = EAIFocusPriority::Montage;
		
		// Montage has highest priority. If playing a montage with a focal point, use it
		FVector FocalPoint = GetFocalPointForPriority(FocusType);

		#if !UE_BUILD_SHIPPING
		FString CurrentFocusPriority = "Montage";
		FColor CurrentFocusColor = FColor::Blue;
		#endif

		// Gameplay 2nd priority
		if (!FAISystem::IsValidLocation(FocalPoint))
		{
			FocusType = EAIFocusPriority::Gameplay;
			FocalPoint = GetFocalPointForPriority(FocusType);

			#if !UE_BUILD_SHIPPING
			CurrentFocusPriority = "Gameplay";
			CurrentFocusColor = FColor::Yellow;
			#endif
		}

		// Move 3rd priority
		if (!FAISystem::IsValidLocation(FocalPoint))
		{
			if (GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Moving)
			{
				FocusType = EAIFocusPriority::Move;
				FocalPoint = GetFocalPointForPriority(FocusType);
				FocalPoint.Z += 100.0f;
				
				#if !UE_BUILD_SHIPPING
				CurrentFocusPriority = "Move";
				CurrentFocusColor = FColor::Cyan;
				#endif
			}
		}

		if (!FAISystem::IsValidLocation(FocalPoint))
		{
			FocusType = EAIFocusPriority::Default;
			FocalPoint = GetFocalPointForPriority(FocusType);

			#if !UE_BUILD_SHIPPING
			CurrentFocusPriority = "Default";
			CurrentFocusColor = FColor::White;
			#endif
		}

		if (!FAISystem::IsValidLocation(FocalPoint))
		{
			//FocusType = EAIFocusPriority::Default;
			FocalPoint = GetFocalPoint();

			#if !UE_BUILD_SHIPPING
			CurrentFocusPriority = "Highest Priority";
			CurrentFocusColor = FColor::White;
			#endif
		}
		
		if (FocalPoint == FVector::ZeroVector || FocalPoint == FAISystem::InvalidLocation)
		{
			FocalPoint = GetCharacter()->GetActorLocation() + FVector::UpVector * 45.0f + GetCharacter()->GetActorForwardVector() * 200.0f;
			//FocalPoint = FVector::ZeroVector;
		}

		bool bIsFocalPointBehind = FVector::DotProduct((CurrentFocalVector - OwningCharacter->GetActorLocation()).GetSafeNormal(), (FocalPoint - OwningCharacter->GetActorLocation()).GetSafeNormal()) < 0.0f;
		bool bCanMoveReset = FocusType != EAIFocusPriority::Move || (FocusType == EAIFocusPriority::Move && PreviousFocusType != EAIFocusPriority::Move) ||
							(FocusType == EAIFocusPriority::Move && PreviousFocusType == EAIFocusPriority::Move && bIsFocalPointBehind);

		if (!TargetFocalPoint.Equals(FocalPoint, 1.0f) && bCanMoveReset)
		{
			if (FVector::Distance(TargetFocalPoint, FocalPoint) > 100.0f)
			{
				StartingFocalVector = CurrentFocalVector;
				FocalVectorAlpha = 0.0f;
			}
			
			TargetFocalPoint = FocalPoint;

			DistanceToCurrentFocalPoint = FMath::Max(FVector::Distance(CurrentFocalVector, OwningCharacter->GetActorLocation()), 100.0f);
			
			bFocalPointIsRight = FVector::DotProduct(OwningCharacter->GetActorRightVector(), (FocalPoint - OwningCharacter->GetActorLocation()).GetSafeNormal()) > 0.0f;
			
			bShouldTurn = bIsFocalPointBehind;
			bStartFocalInterp = FVector::DotProduct(OwningCharacter->GetActorForwardVector(), (FocalPoint - OwningCharacter->GetActorLocation()).GetSafeNormal()) > 0.7f;

			//ULog::Info("Focal Point Changed " + FocalPoint.ToString());
		}

		const bool bNoSmoothTurn = !OwningCharacter->IsArrestedOrSurrendered() && (OwningCharacter->GetMesh()->IsPlayingRootMotion() || OwningCharacter->IsAnimationBlocking() || OwningCharacter->IsTakingCover() || OwningCharacter->IsTakingHostage() || OwningCharacter->GetEquippedItem<AMeleeWeapon>() != nullptr);
		
		if (bNoSmoothTurn)
		{
			CurrentFocalVector = FocalPoint;
			
			bShouldTurn = false;
			bStartFocalInterp = false;
			FocalVectorAlpha = 1.0f;
		}
		else
		{
			if (bShouldTurn)
			{
				if (!bStartFocalInterp)
				{
					if (FVector::DotProduct((CurrentFocalVector - OwningCharacter->GetActorLocation()).GetSafeNormal2D(), (FocalPoint - OwningCharacter->GetActorLocation()).GetSafeNormal2D()) > 0.7f)
					{
						bStartFocalInterp = true;
						StartingFocalVector = CurrentFocalVector;
						FocalVectorAlpha = 0.0f;
						bShouldTurn = false;
					}
					else
					{
						const float DegreesPerSecond = FMath::Clamp(FMath::Abs(OwningCharacter->TurnDegreesPerSecond), 1.0f, 180.0f);
						FVector RotatedFocalPoint = GetCharacter()->GetActorLocation() + FVector::UpVector * 45.0f + OwningCharacter->GetActorForwardVector().RotateAngleAxis(bFocalPointIsRight ? DegreesPerSecond : -DegreesPerSecond, FVector::UpVector) * DistanceToCurrentFocalPoint;

						CurrentFocalVector = FMath::VInterpTo(CurrentFocalVector, RotatedFocalPoint, DeltaTime, OwningCharacter->FocusTurnSpeed);
					}
				}
				else
				{
					bShouldTurn = false;
				}
			}
			else
			{
				if (FocalVectorAlpha >= 1.0f)
				{
					CurrentFocalVector = FocalPoint;
				}
				else
				{
					const float Alpha = FAlphaBlend::AlphaToBlendOption(FocalVectorAlpha, OwningCharacter->FocalPointInterpCurve);
					CurrentFocalVector = FMath::Lerp(StartingFocalVector, FocalPoint, Alpha);
				}
				
				FocalVectorAlpha += DeltaTime * OwningCharacter->FocalPointInterpSpeed;
			}
		}
	
		#if !UE_BUILD_SHIPPING
		if (CVarRonDrawFocalPoint.GetValueOnAnyThread() > 0)
		{
			if (CurrentFocalVector != FVector::ZeroVector && FAISystem::IsValidLocation(CurrentFocalVector))
			{
				DrawDebugBox(GetWorld(), FocalPoint, FVector(5.0f), FColor::White);
				DrawDebugBox(GetWorld(), CurrentFocalVector, FVector(10.0f), CurrentFocusColor);
				DrawDebugLine(GetWorld(), OwningCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), CurrentFocalVector, FColor::Red);
				DrawDebugString(GetWorld(), CurrentFocalVector, CurrentFocusPriority + " Focal Point: " + CurrentFocalVector.ToCompactString(), nullptr, FColor::White, DeltaTime + 0.01f, true);
			}
		}
		#endif
		
		OwningCharacter->Rep_FocalPoint = CurrentFocalVector;
		OwningCharacter->Rep_FocalActor = GetFocusActor();

		PreviousFocusType = FocusType;
		
		if (UInteractionsData::IsPairedInteractionPlayingOn(OwningCharacter))
			return;
		
		FRotator NewControlRotation = GetControlRotation();
		
		if (FAISystem::IsValidLocation(CurrentFocalVector))
		{
			NewControlRotation = (CurrentFocalVector - OwningCharacter->GetActorLocation()).Rotation();
		}
		else if (bSetControlRotationFromPawnOrientation)
		{
			NewControlRotation = OwningCharacter->GetActorRotation();
		}

		// Don't pitch view unless looking at another pawn
		if (NewControlRotation.Pitch != 0.0f && Cast<APawn>(GetFocusActor()) == nullptr)
		{
			NewControlRotation.Pitch = 0.0f;
		}

		SetControlRotation(NewControlRotation);
		
		const FRotator CurrentPawnRotation = OwningCharacter->GetActorRotation();

		//const float RotationRate = IsMoving() && GetCharacter()->GetVelocity().Size() > 0.0f ? FMath::Clamp(GetCharacter()->GetVelocity().Size(), 0.0f, 300.0f) : 100.0f;

		/*
		{
			if (GetRONPathFollowingComp()->GetPath().IsValid() &&
				GetRONPathFollowingComp()->IsUsingSplinePathing())
			{
				const FSplineCurves* Spline = GetRONPathFollowingComp()->GetSplineCurvePath();

				const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), GetCharacter()->GetMovementComponent()->GetActorFeetLocation());
				const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
				const FVector LocalLocation = FNavMeshSplinePath::GetLocationAtDistanceAlongSpline(Spline, Distance + 300.0f); // 3m look ahead

				const FVector HeadTrackingLocation = LocalLocation;
				
				TargetingComponent->HeadTrackingLocation = HeadTrackingLocation;
				TargetingComponent->HeadTrackingLocation.Z = GetCharacter()->GetMesh()->GetBoneLocation("head").Z;

				#if !UE_BUILD_SHIPPING
				if (CVarRonDrawHeadTrackingPoint.GetValueOnAnyThread() > 0)
				{
					DrawDebugSphere(GetWorld(), HeadTrackingLocation + FVector(0.0f, 0.0f, 30.0f), 20.0f, 12, FColor::Yellow, false, GetWorld()->GetDeltaSeconds() + 0.005f, 0, 0.5f);
				}
				#endif
			}
		}
		*/
		
		// Alex 22.04.22 - make sure to always face target, turn in place system takes care of rotating character
		const bool bCanLookAt = !OwningCharacter->GetMesh()->IsPlayingRootMotion() ||
								GetTargetingComp()->IsTrackingMontagePosition();
		
		if (bCanLookAt)
		{
			const float RotationRate = IsMoving() && GetCharacter()->GetVelocity().Size() > 100.0f ? OwningCharacter->ActorRotationInterpMovingSpeed : OwningCharacter->ActorRotationInterpStandingSpeed;
			const FRotator NewRotation = FMath::RInterpTo(CurrentPawnRotation, FRotator(0.0f, NewControlRotation.Yaw, 0.0f), DeltaTime, RotationRate);
			OwningCharacter->SetActorRotation(NewRotation);
		}
	}
}

bool ACyberneticController::IsSWAT() const
{
	return GetTeam() == ETeamType::TT_SERT_RED || GetTeam() == ETeamType::TT_SERT_BLUE || GetTeam() == ETeamType::TT_SQUAD;
}

bool ACyberneticController::IsCivilian() const
{
	return GetTeam() == ETeamType::TT_CIVILIAN;
}

bool ACyberneticController::IsSuspect() const
{
	return GetTeam() == ETeamType::TT_SUSPECT;
}

TSubclassOf<UNavigationQueryFilter> ACyberneticController::GetNavQueryFilter() const
{
	return GetNavQueryFilter(GetTeam());
}

TSubclassOf<UNavigationQueryFilter> ACyberneticController::GetNavQueryFilter(const ETeamType SpecificTeam) const
{
	if (SpecificTeam == ETeamType::TT_SERT_RED || SpecificTeam == ETeamType::TT_SERT_BLUE || SpecificTeam == ETeamType::TT_SQUAD)
	{
		switch (GetCharacter()->GetSquadPosition())
		{
			case ESquadPosition::SP_Alpha:			return UNavQuery_SwatAlpha::StaticClass();
			case ESquadPosition::SP_Beta:			return UNavQuery_SwatBeta::StaticClass();
			case ESquadPosition::SP_Charlie:		return UNavQuery_SwatCharlie::StaticClass();
			case ESquadPosition::SP_Delta:			return UNavQuery_SwatDelta::StaticClass();
			default:								return UNavQuery_Swat::StaticClass();
		}
	}
	
	switch (SpecificTeam)
	{
		case ETeamType::TT_SUSPECT:		return UNavQuery_Suspect::StaticClass();
		case ETeamType::TT_CIVILIAN:	return UNavQuery_Civilian::StaticClass();
		default:						return UNavigationQueryFilter::StaticClass();
	}
}

void ACyberneticController::ClearLastHeardDoorKick()
{
	LastHeardDoorKick = nullptr;
}

bool ACyberneticController::IsMoving() const
{
	// just consider moving as paused also... need to fix this 
	return GetPathFollowingComponent()->GetStatus() != EPathFollowingStatus::Idle;
}

bool ACyberneticController::IsActivelyMovingOnPath() const
{
	if (!GetPathFollowingComponent()->GetCurrentRequestId().IsValid())
		return false;
	
	return !GetCurrentActivity();
}

bool ACyberneticController::DoesPathGoThroughDoor(ADoor*& Door) const
{
	return GetRONPathFollowingComp()->DoesCurrentPathGoThroughDoor(Door);
}

bool ACyberneticController::DoesPathGoThroughDoor(FNavPathSharedPtr NavPath, ADoor*& Door) const
{
	return GetRONPathFollowingComp()->DoesPathGoThroughDoor(NavPath, Door);
}

void ACyberneticController::PerformActivity(UBaseActivity* InActivity, const float DeltaTime)
{
	if (InActivity->CanTick())
	{
		#if !UE_BUILD_SHIPPING
		InActivity->PerformActivity_Debug(DeltaTime);
		#endif
		
		InActivity->PerformActivity(DeltaTime);

		if (!InActivity->IsActivityComplete())
		{
			InActivity->ActivityStateMachine->Tick(DeltaTime);
			
			InActivity->OnPerformActivity.Broadcast(InActivity, this, DeltaTime);
		}
	}
}

void ACyberneticController::SpottedEnemy(AReadyOrNotCharacter* Enemy)
{
	if (!Enemy)
		return;

	bHasEverSpottedEnemyBefore = true;
	
	if (TargetingComponent)
	{
		TargetingComponent->AddKnownEnemy(Enemy);
		TargetingComponent->AddCharacterToSeenMap(Enemy);
	}
	
	GetCharacter()->OnSpottedEnemy.Broadcast(GetCharacter(), Enemy);
}

void ACyberneticController::SpottedFriendly(AReadyOrNotCharacter* Friendly)
{
	if (Friendly == GetCharacter())
		return;
	
	if (TargetingComponent)
	{
		TargetingComponent->AddKnownFriendly(Friendly);
		TargetingComponent->AddCharacterToSeenMap(Friendly);
		
		if (!TargetingComponent->IsTrackedInKnownFriendlies(Friendly))
		{
			if (Friendly->IsActive())
			{
				if (MoraleComponent)
					MoraleComponent->IncreaseResource(AI_CONFIG_GET_FLOAT("SuspectFriendlySpottedMorale.Gain", 0.05f));
			}
		}
	}

	GetCharacter()->OnSpottedFriendly.Broadcast(GetCharacter(), Friendly);
}

void ACyberneticController::SpottedNeutral(AReadyOrNotCharacter* Neutral)
{
	if (!Neutral)
		return;
	
	if (TargetingComponent)
	{
		TargetingComponent->AddKnownNeutral(Neutral);
		TargetingComponent->AddCharacterToSeenMap(Neutral);
	}
	
	GetCharacter()->OnSpottedNeutral.Broadcast(GetCharacter(), Neutral);
}

void ACyberneticController::InstigatedAnyDamage(const float Damage, const UDamageType* DamageType, AActor* DamagedActor, AActor* DamageCauser)
{
	Super::InstigatedAnyDamage(Damage, DamageType, DamagedActor, DamageCauser);

	if (const AReadyOrNotCharacter* DamagedCharacter = Cast<AReadyOrNotCharacter>(DamagedActor))
	{
		if (DamagedCharacter->IsOnSWATTeam())
			GetCharacter()->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::HIT_THE_PLAYER, 5.0f);
	}
}

FVector ACyberneticController::GetFocalPointOnActor(const AActor* Actor) const
{
	const AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(Actor);
	if (pc)
	{
		return pc->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	}

	const AThreatAwarenessActor* ThreatAwarenessActor = Cast<AThreatAwarenessActor>(Actor);
	if (ThreatAwarenessActor)
	{
		return ThreatAwarenessActor->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
	}

	const ADoor* Door = Cast<ADoor>(Actor);
	if (Door)
	{
		return Door->GetDoorMidLocation();
	}
	
	return Actor ? Actor->GetActorLocation() : FAISystem::InvalidLocation;
}

bool ACyberneticController::AddActivity(UBaseActivity* Activity, bool bOverrideCurrentActivity)
{
	 if (!Activity)
		return false;

	if (!GetCharacter())
		return false;

	// No activities allowed. Can't do anything when inactive (except surrender and arrest, allow those)
	if (!GetCharacter()->IsActive() && !GetCharacter()->bSurrenderComplete && !GetCharacter()->bArrestComplete)
		return false;

	// precautionary measure to ensure only select activites are allowed to be performed whilst arrested. For example, Move To
	if (GetCharacter()->bArrestComplete && !Activity->bAllowWhileArrested)
		return false;

	// Already added, don't give again
	if (CurrentActivity == Activity)
		return false;

	if (CurrentActivity == nullptr)
	{
		CurrentActivity = Activity;

		if (CurrentActivity->InitActivity(this))
		{
			if (!CurrentActivity->HasStartedActivity())
			{
				CurrentActivity->StartActivity(this);
			}
		}

		return true;
	}

	if (bOverrideCurrentActivity)
	{
		if (CurrentActivity)
		{
			const bool bCanOverride = CurrentActivity->CanOverrideActivity() && CurrentActivity->CanBeOverridenBy(Activity);
			if (!bCanOverride)
			{
				if (!ActivityQueue.Contains(Activity))
					ActivityQueue.Insert(Activity, 0);
				
				return true;
			}
			
			CurrentActivity->ActivityOverriden(Activity);
		}
		
		UBaseActivity* tActivity = CurrentActivity;
		CurrentActivity = Activity;
		if (tActivity != Activity)
		{
			if (!ActivityQueue.Contains(tActivity))
				ActivityQueue.Insert(tActivity, 0);
		}

		if (CurrentActivity->InitActivity(this))
		{
			if (!CurrentActivity->HasStartedActivity())
			{
				CurrentActivity->StartActivity(this);
			}
		}

		return true;
	}
	
	ActivityQueue.AddUnique(Activity);
	return true;
}

void ACyberneticController::ClearActivityList(const bool bKeepCurrent)
{
	for (UBaseActivity* Activity : ActivityQueue)
	{
		if (Activity)
		{
			Activity->OnClearedFromQueue();
		} 
	}
	
	ActivityQueue.Empty();

	if (!bKeepCurrent)
	{
		if (CurrentActivity && CurrentActivity->CanBeCleared())
		{
			FinishActivity(CurrentActivity, false, true);
		}
	}
}

bool ACyberneticController::IsCharacterKnownEnemy(AReadyOrNotCharacter* InCharacter) const
{
	if (!InCharacter)
		return false;
	
	for (const AReadyOrNotCharacter* Enemy : TargetingComponent->KnownEnemies)
	{
		if (Enemy == InCharacter)
			return true;
	}

	return false;
}

int32 ACyberneticController::GetSuccessConsiderCountForAction(FName Action) const
{
	if (UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
	{
		if (AwarenessState > EAIAwarenessState::Unalerted)
		{
			int32 Count = Archetype->GetSuccessfulConsiderCountForAction(this, Archetype->CombatMoveActions, Action);
			if (Count <= -1)
			{
				if (AwarenessState == EAIAwarenessState::Suspicious)
					Count = Archetype->GetSuccessfulConsiderCountForAction(this, Archetype->SuspiciousActions, Action);
				else if (AwarenessState == EAIAwarenessState::Alerted)
					Count = Archetype->GetSuccessfulConsiderCountForAction(this, Archetype->AlertActions, Action);
			}

			return Count;
		}
		
		return Archetype->GetSuccessfulConsiderCountForAction(this, Archetype->UnalertActions, Action);
	}

	return -1;
}

int32 ACyberneticController::GetFailedConsiderCountForAction(FName Action) const
{
	if (UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
	{
		if (AwarenessState > EAIAwarenessState::Unalerted)
		{
			int32 Count = Archetype->GetFailedConsiderCountForAction(this, Archetype->CombatMoveActions, Action);
			if (Count <= -1)
			{
				if (AwarenessState == EAIAwarenessState::Suspicious)
					Count = Archetype->GetFailedConsiderCountForAction(this, Archetype->SuspiciousActions, Action);
				else if (AwarenessState == EAIAwarenessState::Alerted)
					Count = Archetype->GetFailedConsiderCountForAction(this, Archetype->AlertActions, Action);
			}

			return Count;
		}
		
		return Archetype->GetFailedConsiderCountForAction(this, Archetype->UnalertActions, Action);
	}

	return -1;
}

void ACyberneticController::FinishActivity(UBaseActivity* Activity, const bool bSuccess, const bool bForce)
{
	if (!Activity)
		return;
	
	// combat activities cannot be finished from here, must be manually done
	if (Cast<UBaseCombatActivity>(Activity))
		return;
	
	// combat move activities can only be finished by the combat activity
	if (Cast<UBaseCombatMoveActivity>(Activity))
		return;

	if (Activity != CurrentActivity && !ActivityQueue.Contains(Activity))
		return;
	
	if (!Activity->CanFinishActivity() && !bForce)
		return;

	// Cannot finish something that hasnt been started yet
	if (Activity->ActivityStatus == EActivityStatus::Uninitialized)
		return;
	
	// Make sure we dont finish the activity multiple times
	if (Activity->ActivityStatus == EActivityStatus::Complete)
		return;

	// Make sure we still possess a character, to prevent crashes if GetCharacter is used in there
	if (GetCharacter())
		Activity->FinishedActivity(bSuccess);
	else
		Activity->FinishedActivity_NoOwner(bSuccess);

	ActivityQueue.Remove(Activity);
	
	if (CurrentActivity == Activity)
	{
		CurrentActivity = nullptr;
	
		if (ActivityQueue.Num() > 0)
		{
			CurrentActivity = ActivityQueue[0];
			ActivityQueue.RemoveAt(0);
		}
		
		if (CurrentActivity)
		{
			if (CurrentActivity->HasStartedActivity() || CurrentActivity->IsActivityPaused())
			{
				CurrentActivity->ResumeActivity();
			}
			else
			{
				if (CurrentActivity->InitActivity(this))
					CurrentActivity->StartActivity(this);
			}
		}
	}
}

UBaseActivity* ACyberneticController::GetActivity(const TSubclassOf<UBaseActivity> ActivityType) const
{
	if (CurrentActivity && CurrentActivity->GetClass() == ActivityType)
	{
		return CurrentActivity;
	}
	
	for (UBaseActivity* Activity : ActivityQueue)
	{
		if (Activity && Activity->GetClass() == ActivityType)
		{
			return Activity;
		}
	}
	
	return nullptr;
}

bool ACyberneticController::HasActivityType(TSubclassOf<UBaseActivity> Type)
{
	if (CurrentActivity && CurrentActivity->GetClass() == Type)
	{
		return true;
	}
	
	for (const UBaseActivity* Activity : ActivityQueue)
	{
		if (Activity && Activity->GetClass() == Type)
		{
			return true;
		}
	}
	
	return false;
}

void ACyberneticController::RemoveActivitiesOfType(TSubclassOf<UBaseActivity> Type, bool bClearCurrent)
{
	if (bClearCurrent)
	{
		if (CurrentActivity && CurrentActivity->GetClass() == Type)
		{
			FinishActivity(CurrentActivity, false);
		}
	}
	
	for (int32 i = 0; i < ActivityQueue.Num(); i++)
	{
		if (ActivityQueue[i] && ActivityQueue[i]->GetClass() == Type)
		{
			ActivityQueue.RemoveAt(i);
		}
	}
}

void ACyberneticController::RemoveAllActivitiesExcept(const TSubclassOf<UBaseActivity> ActivityType)
{
	if (CurrentActivity && CurrentActivity->GetClass() != ActivityType)
	{
		FinishActivity(CurrentActivity, false, true);
	}
	
	for (int32 i = 0; i < ActivityQueue.Num(); i++)
	{
		if (ActivityQueue[i] && ActivityQueue[i]->GetClass() != ActivityType)
		{
			ActivityQueue.RemoveAt(i);
		}
	}
}

FString ACyberneticController::GetActivityQueueAsString()
{
	FString rt = "";
	rt += "Current: " + (CurrentActivity ? CurrentActivity->GetName() : "None");
	for (UBaseActivity* activity : ActivityQueue)
	{
		if (activity)
		{
			rt += "\r\n";
			rt += activity->GetName();
		}
	}
	return rt;
}

void ACyberneticController::OnKnownEnemyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	TargetingComponent->TimeSinceLastEnemyDeath = 0.0f;
	OnKnownEnemyKilled_Blueprint(InstigatorCharacter, KilledCharacter);
}

void ACyberneticController::OnKnownEnemyIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	TargetingComponent->TimeSinceLastEnemyDeath = 0.0f;
	OnKnownEnemyIncapacitated_Blueprint(IncapacitatedCharacter);
}

void ACyberneticController::OnKnownEnemyTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	TargetingComponent->TimeSinceLastEnemyTookDamage = 0.0f;
	OnKnownEnemyTakeDamage_Blueprint(InstigatorCharacter, DamagedCharacter, DamageCauser, Damage, HealthRemaining);
}

void ACyberneticController::OnKnownEnemyStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	TargetingComponent->TimeSinceLastEnemyStunned = 0.0f;
	OnKnownEnemyStunned_Blueprint(StunnedCharacter, Duration, StunType, DamageCauser);
}

void ACyberneticController::OnKnownFriendlyKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	TargetingComponent->TimeSinceLastFriendlyDeath = 0.0f;
	OnKnownFriendlyKilled_Blueprint(InstigatorCharacter, KilledCharacter);
}

void ACyberneticController::OnKnownFriendlyIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	TargetingComponent->TimeSinceLastFriendlyDeath = 0.0f;
	OnKnownFriendlyIncapacitated_Blueprint(IncapacitatedCharacter);
}

void ACyberneticController::OnKnownFriendlyTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	TargetingComponent->TimeSinceLastFriendlyTookDamage = 0.0f;
	OnKnownFriendlyTakeDamage_Blueprint(InstigatorCharacter, DamagedCharacter, DamageCauser, Damage, HealthRemaining);
}

void ACyberneticController::OnKnownFriendlyStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	TargetingComponent->TimeSinceLastFriendlyStunned = 0.0f;
	OnKnownFriendlyStunned_Blueprint(StunnedCharacter, Duration, StunType, DamageCauser);
}

void ACyberneticController::OnKnownNeutralKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	OnKnownNeutralKilled_Blueprint(InstigatorCharacter, KilledCharacter);
}

void ACyberneticController::OnKnownNeutralIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	OnKnownNeutralIncapacitated_Blueprint(IncapacitatedCharacter);
}

void ACyberneticController::OnKnownNeutralTakeDamage(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	OnKnownNeutralTakeDamage_Blueprint(InstigatorCharacter, DamagedCharacter, DamageCauser, Damage, HealthRemaining);
}

void ACyberneticController::OnKnownNeutralStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	OnKnownNeutralStunned_Blueprint(StunnedCharacter, Duration, StunType, DamageCauser);
}

void ACyberneticController::GiveMoveTo(FVector Location, bool bClearAllActivities)
{
	if (Location == FVector::ZeroVector)
		return;

	if (bClearAllActivities)
	{
		ClearActivityList();
	}

	MoveToActivity->SetLocation(Location, CurrentActivity == MoveToActivity);
	
	if (MoveToActivity->GetLocation() != FVector::ZeroVector && CurrentActivity != MoveToActivity)
	{
		UActivityManager::GiveActivityTo(MoveToActivity, GetCharacter(), true);
	}
}

void ACyberneticController::NextActivityOnRoute()
{
	if (!GetCharacter())
		return;
	
	if (bPendingNextActivityRoute)
		return;
	
	FActivityRouteCollection* Route = &GetCharacter()->ActivityRouteCollection;
	
	Route->ActivityIdx++;

	if (const FActivityRoute* CurrentRoute = Route->GetCurrentActivity())
	{
		if (CurrentRoute->WorldBuildingPlacementActor)
		{
			bPendingNextActivityRoute = true;
		}
	}
	else
	{
		if (!HasActivityType(UWorldBuildingActivity::StaticClass()))
		{
			if (Route->bReturnToOriginalSpot)
			{
				GiveMoveTo(GetCharacter()->OriginalSpawnLocation);
			}

			bPendingNextActivityRoute = false;
			
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_WorldBuildingRouteCooldown, this, &ACyberneticController::RestartActivityRouteCollection, Route->Cooldown, false);
		}
	}
}

void ACyberneticController::RestartActivityRouteCollection()
{
	bPendingNextActivityRoute = true;

	ACyberneticCharacter* CyberneticCharacter = GetCharacter();
	if (CyberneticCharacter)
	{
		CyberneticCharacter->ActivityRouteCollection.ActivityIdx = 0;
	}
}

bool ACyberneticController::IsMovingForRequests(TArray<FAIRequestID> Requests) const
{
	for (FAIRequestID& request : Requests)
	{
		if (request.IsValid() && request == GetPathFollowingComponent()->GetCurrentRequestId())
		{
			return true;
		}
	}
	return false;
}

bool ACyberneticController::IsMovingForRequest(int32 RequestID) const
{
	if (RequestID < 0)
		return false;
	
	return RequestID == GetPathFollowingComponent()->GetCurrentRequestId();
}

void ACyberneticController::PickupItem(ABaseItem* Item, bool bEquipItem)
{
	if (GetCharacter()->IsSurrendered() || GetCharacter()->IsArrested())
		return;

	GetCharacter()->GetInventoryComponent()->AddInventoryItem(Item);
	// don't holster deployables.. it drops them
	if (bEquipItem && !Item->bDeployable)
	{
		GetCharacter()->GetInventoryComponent()->PutItemInHands(Item);
	}
}

void ACyberneticController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (CurrentActivity)
	{
		CurrentActivity->OnPathMoveCompleted(this, RequestID, Result);
	}

	if (GetCombatActivity() &&
		GetCombatActivity()->GetCombatMoveActivity())
	{
		GetCombatActivity()->GetCombatMoveActivity()->OnPathMoveCompleted(this, RequestID, Result);
	}
	
	MoveRequestIdToPathIdMap.Remove(RequestID.GetID());
	
	OnMoveCompletedDelegate.ExecuteIfBound(this, RequestID, Result);
	OnMoveComplete.Broadcast(this, RequestID.GetID());
}

AReadyOrNotCharacter* ACyberneticController::GetTrackedTarget()
{
	return GetTargetingComp() ? GetTargetingComp()->GetTrackedTarget() : nullptr;
}

AReadyOrNotCharacter* ACyberneticController::GetLastTrackedEnemy()
{
	return GetTargetingComp() ? GetTargetingComp()->GetLastTrackedTarget() : nullptr;
}

float ACyberneticController::GetReactionTime(const EActorSenseType& SenseType) const
{
	switch (SenseType)
	{
		case EActorSenseType::Sight:	return 0.25f;
		case EActorSenseType::Sound:	return 0.25f;
		case EActorSenseType::Damage:	return 0.1f;
		default:						return 0.25f;
	}
}

void ACyberneticController::AddActorSightSense(FActorSense& NewActorSense)
{
	const float DistanceInMeters = FVector::Distance(NewActorSense.Stimulus.ReceiverLocation, NewActorSense.Stimulus.StimulusLocation)/100.0f;
	const int32 Laps = DistanceInMeters/20.0f;
	const float ReactionTimeIncrease = IsSuspect() ? AI_CONFIG_GET_FLOAT("SuspectSightReactionTimeIncreasePerTwentyMeters") : IsSWAT() ? AI_CONFIG_GET_FLOAT("SwatSightReactionTimeIncreasePerTwentyMeters") : 0.0f;
	const float DistanceReactionTime = Laps * FMath::Max(ReactionTimeIncrease, 0.0f);

	NewActorSense.SenseReactionTime += DistanceReactionTime;
	
	//LOG_NUMBER(DistanceInMeters);
	//LOG_NUMBER(Laps);
	//LOG_NUMBER(DistanceReactionTime);
	//LOG_NUMBER(NewActorSense.SenseReactionTime);
		
	ActorSightSenseMap.AddUnique(NewActorSense);
}

void ACyberneticController::AddActorSoundSense(FActorSense& NewActorSense)
{
	ActorSoundSenseMap.AddUnique(NewActorSense);
}

void ACyberneticController::AddActorDamageSense(FActorSense& NewActorSense)
{
	ActorDamageSenseMap.AddUnique(NewActorSense);
}

void ACyberneticController::RemoveActorSightSense(AActor* InActor, const FName& InTag)
{
	if (!InActor)
		return;

	ActorSightSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		return !Element.Actor || (Element.Actor == InActor && Element.Tag == InTag);
	});
}

void ACyberneticController::RemoveActorSoundSense(AActor* InActor, const FName& InTag)
{
	if (!InActor)
		return;

	ActorSoundSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		return !Element.Actor || (Element.Actor == InActor && Element.Tag == InTag);
	});
}

void ACyberneticController::RemoveActorDamageSense(AActor* InActor, const FName& InTag)
{
	if (!InActor)
		return;

	ActorDamageSenseMap.RemoveAll([&](const FActorSense& Element)
	{
		return !Element.Actor || (Element.Actor == InActor && Element.Tag == InTag);
	});
}

float ACyberneticController::GetSightSenseTimeForActor(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return 0.0f;

	for (const FActorSense& Sense : ActorSightSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return Sense.SenseTime;
		}
	}

	return 0.0f;
}

float ACyberneticController::GetSoundSenseTimeForActor(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return 0.0f;

	for (const FActorSense& Sense : ActorSoundSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return Sense.SenseTime;
		}
	}

	return 0.0f;
}

float ACyberneticController::GetDamageSenseTimeForActor(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return 0.0f;

	for (const FActorSense& Sense : ActorDamageSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return Sense.SenseTime;
		}
	}

	return 0.0f;
}

bool ACyberneticController::IsSightReactingToActor(AActor* InActor) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorSightSenseMap)
	{
		if (Sense.Actor == InActor)
		{
			FActorPerceptionBlueprintInfo PerceptionOfActor;
			if (AIPerceptionComponent->GetActorsPerception(Sense.Actor, PerceptionOfActor))
			{
				for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
				{
					if (!Stimulus.IsValid())
						continue;
				
					if (Stimulus.Type.Name == RON_SENSE_SIGHT)
					{
						// Forgot old/stale/unseen actors
						if (Stimulus.GetAge() > Sense.SenseForgetTime || Stimulus.IsExpired())
						{
							return false;
						}
					}
				}
			}
			else
			{
				return false;
			}
		
			return true;
		}
	}
	
	return false;
}

bool ACyberneticController::IsSoundReactingToActor(AActor* InActor) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorSoundSenseMap)
	{
		if (Sense.Actor == InActor)
		{
			return true;
		}
	}
	
	return false;
}

bool ACyberneticController::IsDamageReactingActor(AActor* InActor) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorDamageSenseMap)
	{
		if (Sense.Actor == InActor)
		{
			return true;
		}
	}
	
	return false;
}

bool ACyberneticController::IsSightReactingToActorWithTag(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorSightSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return true;
		}
	}

	return false;
}

bool ACyberneticController::IsSoundReactingToActorWithTag(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorSoundSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return true;
		}
	}

	return false;
}

bool ACyberneticController::IsDamageReactingToActorWithTag(AActor* InActor, const FName& InTag) const
{
	if (!InActor)
		return false;

	for (const FActorSense& Sense : ActorDamageSenseMap)
	{
		if (Sense.Actor == InActor && Sense.Tag == InTag)
		{
			return true;
		}
	}

	return false;
}

void ACyberneticController::OnSeenActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus)
{
	if (!InActor)
		return;

	LastSensedActor = InActor;
	GetCharacter()->OnSensedActor.Broadcast(InActor);

	/*
	#if !UE_BUILD_SHIPPING
	if (InTag.IsNone())
		ULog::Info(GetName() + " seen " + InActor->GetName());
	else
		ULog::Info(GetName() + " seen " + InActor->GetName() + " | Tag: " + InTag.ToString());
	#endif
	*/
	
	TryStopCurrentConversation();
	
	TimeSinceLastExposedToAnyStimulus = 0.0f;
	TimeSinceLastExposedToSightStimulus = 0.0f;
	
	if (AReadyOrNotCharacter* SensedCharacter = SensedActorToCharacter(InActor))
	{
		LastSensedCharacter = SensedCharacter;
		GetCharacter()->OnSensedCharacter.Broadcast(SensedCharacter);
		
		OnSeenCharacter(SensedCharacter, Stimulus);
		return;
	}
	
	if (const ABaseGrenade* Grenade = Cast<ABaseGrenade>(InActor))
	{
		if (!Grenade->bUsed || Grenade->bHasEverDetonated)
			return;
		
		OnSeenGrenade(Grenade->ThrownBy, Grenade->GetItemLocation());
		return;
	}

	if (const AFlashLightTrackingPoint* FlashLightTrackingPoint = Cast<AFlashLightTrackingPoint>(InActor))
	{
		OnSeenFlashlight(FlashLightTrackingPoint->GetOwnerCharacter(), FlashLightTrackingPoint->GetActorLocation());
		return;
	}
	
	if (AIncapacitatedHuman* IncapHuman = Cast<AIncapacitatedHuman>(InActor))
	{
		OnSeenIncapHuman(IncapHuman);
		return;
	}
}

void ACyberneticController::OnHeardActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus, float ExpiryTime)
{
	if (!InActor)
		return;
	
	LastSensedActor = InActor;
	GetCharacter()->OnSensedActor.Broadcast(InActor);

	/*
	#if !UE_BUILD_SHIPPING
	if (InTag.IsNone())
		ULog::Info(GetName() + " heard " + InActor->GetName());
	else
		ULog::Info(GetName() + " heard " + InActor->GetName() + " | Tag: " + InTag.ToString());
	#endif
	*/
	
	TimeSinceLastExposedToAnyStimulus = 0.0f;
	TimeSinceLastExposedToSoundStimulus = 0.0f;

	if (!IsSWAT())
	{
		if (AwarenessState != EAIAwarenessState::Alerted)
			AwarenessState = EAIAwarenessState::Suspicious;
	}
	
	if (AReadyOrNotCharacter* SensedCharacter = SensedActorToCharacter(InActor))
	{
		LastSensedCharacter = SensedCharacter;
		GetCharacter()->OnSensedCharacter.Broadcast(SensedCharacter);

		HeardActorInstigator = SensedCharacter;
		
		AddExposedToStimulusTag(InTag, InActor->GetActorLocation(), IsCharacterFriendly(SensedCharacter) || IsCharacterNeutral(SensedCharacter), SensedCharacter, ExpiryTime);
		
		OnHeardCharacter(SensedCharacter, Stimulus);
		return;
	}
	
	if (const ABaseGrenade* Grenade = Cast<ABaseGrenade>(InActor))
	{
		if (Grenade->ThrownBy && !AReadyOrNotCharacter::IsOnSameTeam(Grenade->ThrownBy, GetCharacter()))
		{
			OnHeardGrenade(Grenade->ThrownBy, Grenade->GetItemLocation(), InTag);
			AddExposedToStimulusTag(InTag, Grenade->GetItemLocation(), IsCharacterFriendly(Grenade->ThrownBy) || IsCharacterNeutral(Grenade->ThrownBy), Grenade->ThrownBy, ExpiryTime);
			HeardActorInstigator = Grenade->ThrownBy;
		}
		
		return;
	}

	if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(InActor))
	{
		OnHeardGunShot(Weapon->GetOwnerCharacter(), Weapon->GetItemLocation(), InTag);
		if (Weapon->GetOwnerCharacter())
		{
			AddExposedToStimulusTag(InTag, Weapon->GetItemLocation(), IsCharacterFriendly(Weapon->GetOwnerCharacter()) || IsCharacterNeutral(Weapon->GetOwnerCharacter()), Weapon->GetOwnerCharacter(), ExpiryTime);
			HeardActorInstigator = Weapon->GetOwnerCharacter();
		}
		return;
	}

	if (ADoor* Door = Cast<ADoor>(InActor))
	{
		OnHeardDoor(Door->GetLastDoorUser(), Door, InTag);
		if (Door->GetLastDoorUser())
		{
			AddExposedToStimulusTag(InTag, Door->GetDoorMidLocation(), IsCharacterFriendly(Door->GetLastDoorUser()) || IsCharacterNeutral(Door->GetLastDoorUser()), Door->GetLastDoorUser(), ExpiryTime);
			HeardActorInstigator = Door->GetLastDoorUser();
		}
		return;
	}
	
	AddExposedToStimulusTag(InTag, Stimulus.StimulusLocation, false, nullptr, ExpiryTime);
}

void ACyberneticController::OnDamagedByActor(AActor* InActor, const FName& InTag, const FAIStimulus& Stimulus)
{
	if (!InActor)
		return;
	
	TryStopCurrentConversation();
	
	// always alert when damaged by anything
	AwarenessState = EAIAwarenessState::Alerted;
	
	GetCharacter()->OnSensedActor.Broadcast(InActor);
	
	TimeSinceLastExposedToAnyStimulus = 0.0f;
	TimeSinceLastExposedToAggressiveStimulus = 0.0f;

	if (LineOfSightTo(InActor))
	{
		OnSeenActor(InActor, InTag, Stimulus);
	}
	
	if (AReadyOrNotCharacter* SensedCharacter = SensedActorToCharacter(InActor))
	{
		LastSensedCharacter = SensedCharacter;
		GetCharacter()->OnSensedCharacter.Broadcast(SensedCharacter);
		
		OnDamagedByCharacter(SensedCharacter, Stimulus);
		return;
	}

	/*
	if (ABaseItem* SensedItem = Cast<ABaseItem>(InActor))
	{
		return;
	}
	*/
}

void ACyberneticController::AbortMove(bool bKeepVelocity)
{
	GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::ForcedScript, FAIRequestID::CurrentRequest, bKeepVelocity ? EPathFollowingVelocityMode::Keep : EPathFollowingVelocityMode::Reset);
}

void ACyberneticController::DetermineBestInterruptAction(TArray<FAIActionDataContainer>& InActions, FAIActionData*& OutBestAction)
{
	SCOPE_CYCLE_COUNTER(STAT_CyberneticController_DetermineInterruptAction);
	
	// Find all actions that are specified in the commit interrupts list
	TArray<FAIActionData*> InterruptActions;
	InterruptActions.Reserve(InActions.Num());
	
	for (FAIActionDataContainer& Action : InActions)
	{
		FAIActionData* RealAction = &Action.GetActionData();
		
		if (RealAction->bCanInterruptAnyAction)
		{
			InterruptActions.Add(RealAction);
			break;
		}
	}
	
	if (OutBestAction->CommitInterrupts.Num() > 0)
	{
		InterruptActions.Add(OutBestAction);
		
		for (const FAIActionData_NameOnly& ActionName : OutBestAction->CommitInterrupts)
		{
			for (FAIActionDataContainer& Action : InActions)
			{
				FAIActionData* RealAction = &Action.GetActionData();
	
				if (RealAction->Name == ActionName.Name)
				{
					InterruptActions.Add(RealAction);
					break;
				}
			}
		}
	}
	
	if (FAIActionData* BestInterruptAction = AIActionDecisionEvaluator::DetermineBestActionFor(this, InterruptActions))
	{
		if (BestInterruptAction != OutBestAction)
		{
			if (const float* InterruptScoreActionPtr = BestInterruptAction->Scores.Find(this))
			{
				if (const float* BestActionScorePtr = OutBestAction->Scores.Find(this))
				{
					if (*InterruptScoreActionPtr > *BestActionScorePtr)
					{
						// Stop current action
						OutBestAction->CommitTimes.Remove(this);
						CombatActivity->EndAction(OutBestAction);
						OutBestAction->StartCooldown(this);

						// Switch to best interrupt action
						OutBestAction = BestInterruptAction;
						
						OutBestAction->Commit(this);
						CombatActivity->BeginAction(OutBestAction);
					}
				}
			}
		}
	}
}

bool ACyberneticController::IsPerformingAction(FAIActionData* Action) const
{
	if (!Action)
		return false;
	
	return Action == BestAction || Action == BestContinuousAction || Action == BestSuspiciousAction || Action == BestUnalertAction || Action == BestCombatMoveAction;
}

bool ACyberneticController::IsPerformingCustomAction(FName BlueprintTag) const
{
	const auto Lambda = [&BlueprintTag, this](const FAIActionData* InAction)
	{
		if (InAction)
		{
			if (const UAIAction* Action = InAction->GetCustomAction(this))
			{
				if (Action->HasTag(BlueprintTag))
					return true;
			}
		}
		
		return false;
	};

	return  Lambda(BestAction) || Lambda(BestContinuousAction) || Lambda(BestSuspiciousAction) ||
			Lambda(BestUnalertAction) || Lambda(BestCombatMoveAction);
}

void ACyberneticController::ForceEndAction(FAIActionData*& InAction)
{
	if (!CombatActivity)
		return;
	
	if (InAction)
	{
		CombatActivity->EndAction(InAction);
		InAction->StartCooldown(this);
		InAction = nullptr;
	}
}

void ACyberneticController::ForceEndAllActions()
{
	ForceEndAction(BestAction);
	ForceEndAction(BestUnalertAction);
	ForceEndAction(BestSuspiciousAction);
	ForceEndAction(BestCombatMoveAction);
	ForceEndAction(BestContinuousAction);
}

bool ACyberneticController::IsLastAlive() const
{
	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return false;

	if (GameState->bCharactersDirty)
		GameState->UpdateActiveControllers();
	
	if (IsSuspect())	return GetWorld()->GetGameState<AReadyOrNotGameState>()->NumSuspectsActive <= 1;
	if (IsCivilian())	return GetWorld()->GetGameState<AReadyOrNotGameState>()->NumCiviliansActive <= 1;
	if (IsSWAT())		return GetWorld()->GetGameState<AReadyOrNotGameState>()->NumSwatActive <= 1;

	return false;
}

bool ACyberneticController::HasBeenExposedToAggressiveNoise(float SinceSeconds, float MaxDistance, int32 TargetTypeMask) const
{
	if (TimeSinceLastExposedToAggressiveStimulus > SinceSeconds)
	{
		return false;
	}

	for (const auto& k : ExposedToStimulusTags)
	{
		if (k.Value.bAggressive && DoesCharacterMatchTargetType(k.Value.Instigator, TargetTypeMask))
		{
			if (MaxDistance > 0.0f)
			{
				if (k.Value.HeardAtDistance < MaxDistance)
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
	}
	
	return false;
}

bool ACyberneticController::HasBeenExposedToAggressiveNoise_Tag(FName& OutTag, float SinceSeconds, float MaxDistance, int32 TargetTypeMask) const
{
	if (TimeSinceLastExposedToAggressiveStimulus > SinceSeconds)
	{
		return false;
	}

	for (const auto k : ExposedToStimulusTags)
	{
		if (k.Value.bAggressive && DoesCharacterMatchTargetType(k.Value.Instigator, TargetTypeMask))
		{
			if (MaxDistance > 0.0f)
			{
				if (k.Value.HeardAtDistance < MaxDistance)
				{
					OutTag = k.Key;
					return true;
				}
			}
			else
			{
				OutTag = k.Key;
				return true;
			}			
		}
	}
	
	return false;
}

bool ACyberneticController::HasBeenExposedToAnyNoise(float SinceSeconds, float MaxDistance, int32 TargetTypeMask) const
{
	if (TimeSinceLastExposedToSoundStimulus > SinceSeconds)
	{
		return false;
	}

	for (const auto& k : ExposedToStimulusTags)
	{
		if (DoesCharacterMatchTargetType(k.Value.Instigator, TargetTypeMask))
		{
			if (MaxDistance > 0.0f)
			{
				if (k.Value.HeardAtDistance < MaxDistance)
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
	}
	
	return false;
}

bool ACyberneticController::HasBeenExposedToAnyNoise_Tag(FName& OutTag, float SinceSeconds, float MaxDistance, int32 TargetTypeMask) const
{
	if (TimeSinceLastExposedToSoundStimulus > SinceSeconds)
	{
		return false;
	}

	for (const auto k : ExposedToStimulusTags)
	{
		if (DoesCharacterMatchTargetType(k.Value.Instigator, TargetTypeMask))
		{
			if (MaxDistance > 0.0f)
			{
				if (k.Value.HeardAtDistance < MaxDistance)
				{
					OutTag = k.Key;
					return true;
				}
			}
			else
			{
				OutTag = k.Key;
				return true;
			}			
		}
	}
	
	return false;
}

bool ACyberneticController::IsTagAggressiveNoise(const FName& Tag) const
{
	return AggressiveTags.Contains(Tag);
}

bool ACyberneticController::IsTagInvestigativeNoise(const FName& Tag) const
{
	return InvestigativeTags.Contains(Tag);
}

void ACyberneticController::AddExposedToStimulusTag(const FName& Tag, FVector StimulusLocation, bool bFriendly, AReadyOrNotCharacter* StimulusInstigator, float ExpiryTime)
{
	FExposedToNoise ExposedToNoise;
	ExposedToNoise.Tag = Tag;
	ExposedToNoise.StimulusLocation = StimulusLocation;
	ExposedToNoise.HeardAtDistance = FVector::Distance(GetPawn()->GetActorLocation(), StimulusLocation);
	ExposedToNoise.bAggressive = IsTagAggressiveNoise(Tag);
	ExposedToNoise.bFriendly = bFriendly;
	ExposedToNoise.Instigator = StimulusInstigator;
	ExposedToNoise.ExpiryTime = ExpiryTime;

	if (!ExposedToStimulusTags.Find(Tag))
	{
		ExposedToStimulusTags.Add(Tag, ExposedToNoise);
	}
	else
	{
		ExposedToStimulusTags[Tag] = ExposedToNoise;
	}

	if (ExposedToNoise.bAggressive)
	{
		bEverHeardAggressiveStimulus = true;
		TimeSinceLastExposedToAggressiveStimulus = 0.0f;
	}

	TimeSinceLastExposedToAnyStimulus = 0.0f;
	
	TargetingComponent->SetLastHeardNoiseLocation(ExposedToNoise);
}

int32 ACyberneticController::RequestMoveAsync(FVector Location, bool bProjectToNavigation, float AcceptanceRadius)
{
	if (CurrentActivity)
		return -1;
	
	if (Location == FVector::ZeroVector)
		return -1;
	
	if (!GetRONPathFollowingComp())
		return -1;

	if (ShouldPostponePathUpdates())
		return -1;

	// Combat moves always have authority on movement
	if (CombatActivity && CombatActivity->GetCombatMoveActivity())
		return -1;
	
	const ACyberneticCharacter* CyberneticCharacter = GetCharacter();
	if (!CyberneticCharacter)
		return -1;
	
	// No move request allowed when in a paired interaction
	if (CyberneticCharacter->bIsPairedInteractionPlaying)
		return -1;

	if (CyberneticCharacter->IsMovementLocked())
		return -1;

	if (CyberneticCharacter->IsDeadOrUnconscious() && CyberneticCharacter->IsArrestedOrSurrendered())
		return -1;

	if (CyberneticCharacter->IsCarried())
		return -1;
	
	if (GetCharacter()->ReasonsToStandStill.Num() > 0)
		return -1;
	
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(GetCharacter()->GetMovementComponent()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetNavQueryFilter());
			const FVector StartLocation = GetCharacter()->GetActorLocation();
			const FVector EndLocation = Location;
				
			FNavLocation StartLocationProjected(StartLocation);
			FNavLocation EndLocationProjected(EndLocation);

			if (bProjectToNavigation)
			{
				// if they're moving project this...stops the path finding getting caught up on the navlinks by projecting to the start of the link
				NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 200.0f));
				NavSys->ProjectPointToNavigation(EndLocation, EndLocationProjected, FVector(75.0f, 75.0f, 200.0f));
			}
			
			FNavPathQueryDelegate NavDelegate;
			NavDelegate.BindUObject(this, &ACyberneticController::OnPathFound);
			
			const uint32 ID = NavSys->FindPathAsync(GetCharacter()->GetNavAgentPropertiesRef(), CreatePathFindingQuery(QueryFilter, NavData, StartLocationProjected, EndLocationProjected, true, this), NavDelegate, EPathFindingMode::Hierarchical);
			AsyncPathRequestIDs.Add(ID);
			LastAcceptanceRadius = AcceptanceRadius;

			#if !UE_BUILD_SHIPPING
			ULog::Info(FString::Printf(TEXT("[%s][%s][%s] Requesting Async Path to %s (ID %d)!"), *GetName(), *GetName(), *FString(__FUNCTION__), *EndLocationProjected.Location.ToString(), ID));
			#endif

			return ID;
		}
	}

	return -1;
}

void ACyberneticController::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	AsyncPathRequestIDs.Remove(PathId);
	
	if (ShouldPostponePathUpdates())
		return;

	if (!GetCharacter())
		return;

	if (GetCharacter()->IsMovementLocked())
		return;

	if (GetCharacter()->IsArrestedOrSurrendered())
		return;
	
	if (GetCharacter()->ReasonsToStandStill.Num() > 0)
		return;

	if (NavPath.IsValid())
	{
		NavPath->EnableRecalculationOnInvalidation(true);
		NavPath->SetIgnoreInvalidation(false);
		
		if (ResultType == ENavigationQueryResult::Success)
		{
			const FAIMoveRequest MoveRequest = CreateMoveRequest(NavPath->GetQueryData().EndLocation, LastAcceptanceRadius, GetNavQueryFilter(), true, true, false, false, false);

			const FAIRequestID& RequestID = RequestMove(MoveRequest, NavPath);

			LastPathID = PathId;
			LastRequestID = RequestID;
			LastMoveRequest = MoveRequest;

			MoveRequestIdToPathIdMap.Add(RequestID.GetID(), PathId);
			
			#if !UE_BUILD_SHIPPING
			ULog::Info(GetName() + " | Requesting Move: " + MoveRequest.ToString());
			#endif
		}
	}

	ERonNavigationQueryResult Result;
	switch (ResultType)
	{
		case ENavigationQueryResult::Invalid:	Result = ERonNavigationQueryResult::Invalid; break;
		case ENavigationQueryResult::Error:		Result = ERonNavigationQueryResult::Error; break;
		case ENavigationQueryResult::Fail:		Result = ERonNavigationQueryResult::Fail; break;
		case ENavigationQueryResult::Success:	Result = ERonNavigationQueryResult::Success; break;
		default:								Result = ERonNavigationQueryResult::Invalid; break;
	}
	
	OnAsyncPathFound.Broadcast(PathId, Result);
}

FPathFindingQuery ACyberneticController::CreatePathFindingQuery(const FSharedConstNavQueryFilter& NavQueryFilter, const ANavigationData* NavData, const FNavLocation& InStartLocation, const FNavLocation& InEndLocation, const bool bAllowPartialPaths, TWeakObjectPtr<const UObject> Owner)
{	
	FPathFindingQuery PathFindingQuery;
	PathFindingQuery.QueryFilter = NavQueryFilter;
	PathFindingQuery.NavData = NavData;
	PathFindingQuery.StartLocation = InStartLocation.Location;
	PathFindingQuery.EndLocation = InEndLocation.Location;
	PathFindingQuery.bAllowPartialPaths = bAllowPartialPaths;
	PathFindingQuery.Owner = Owner;

	return PathFindingQuery;
}

FAIMoveRequest ACyberneticController::CreateMoveRequest(const FVector& MoveLocation, const float AcceptanceRadius, const TSubclassOf<UNavigationQueryFilter> NavQueryFilter, const bool bUsePathfinding, const bool bCanStrafe, const bool bAllowPartialPath, const bool bProjectGoalLocation, const bool bReachTestIncludesAgentRadius)
{
	FAIMoveRequest MoveRequest(MoveLocation);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetNavigationFilter(NavQueryFilter);
	MoveRequest.SetUsePathfinding(bUsePathfinding);
	MoveRequest.SetCanStrafe(bCanStrafe);
	MoveRequest.SetAllowPartialPath(bAllowPartialPath);
	MoveRequest.SetProjectGoalLocation(bProjectGoalLocation);
	MoveRequest.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);

	return MoveRequest;
}

bool ACyberneticController::HasRecentlySeenSwat(FVector& OutLocation) const
{
	if (RecentlySeenSwat.Num() > 0 && RecentlySeenSwat.Last())
	{
		OutLocation = RecentlySeenSwat.Last()->GetActorLocation();
	}
	
	return RecentlySeenSwat.Num() > 0;
}

bool ACyberneticController::DoesPathGoPastKnownEnemy(FNavPathSharedPtr NavPath)
{
    if (FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
    	if (NavPath)
    	{
    		TArray<FNavPathPoint> NavPathPoints = NavPath->GetPathPoints();
    		// get point between path points in case its a straight line
    		TArray<FNavPathPoint> AdditionalPathPoints;
    		FHitResult Hit;
    		FCollisionObjectQueryParams CollisionObjectParams;
    		CollisionObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    		GetWorld()->LineTraceSingleByObjectType(Hit, GetTargetingComp()->GetLastKnownEnemyPosition(), NavPath->GetDestinationLocation(), CollisionObjectParams);
    		if (!Hit.bBlockingHit)
    			return true;
    		
    		for (int32 i = 1; i < NavPathPoints.Num(); i++)
    		{
    			FNavPathPoint Pt = NavPathPoints[i];
    			FNavPathPoint PrevPt = NavPathPoints[i-1];
    			FVector v1 = UKismetMathLibrary::FindLookAtRotation(PrevPt.Location, Pt.Location).Vector();
    			FVector v2 = UKismetMathLibrary::FindLookAtRotation(PrevPt.Location, GetTargetingComp()->GetLastKnownEnemyPosition()).Vector();
		
    			float DotProduct2D = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
    			if (DotProduct2D > 0.70f)
    			{
    				#if !UE_BUILD_SHIPPING
    				if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
    				{
    					DrawDebugLine(GetWorld(), PrevPt.Location, Pt.Location, FColor::Red, false, 5.0f, 0, 1);
    					DrawDebugPoint(GetWorld(), PrevPt.Location + FVector(0.0f, 0.0f, 25.0f), 10.0f, FColor::Red, false, 5.0f, 0);
    				}
					#endif

    				return true;
    			}

    			#if !UE_BUILD_SHIPPING
    			if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
    			{
    				DrawDebugPoint(GetWorld(), PrevPt.Location  + FVector(0.0f, 0.0f, 25.0f), 10.0f, FColor::Green, false, 5.0f, 0);
    				DrawDebugLine(GetWorld(), PrevPt.Location, Pt.Location, FColor::Green, false, 5.0f, 0, 1);
    			}
    			#endif
    		}
    	}
    }
    
   return false;
}

FSpawnData ACyberneticController::GetSpawnData()
{
	if (GetCharacter())
	{
		return *GetCharacter()->GetSpawnData();
	}
	return FSpawnData();
}

void ACyberneticController::OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	if (!SensedCharacter)
		return;
	
	switch (SensedCharacter->GetTeam())
	{
		case ETeamType::TT_NONE: break;
		case ETeamType::TT_SERT_RED:	RecentlySeenSwat.AddUnique(SensedCharacter); break;
		case ETeamType::TT_SERT_BLUE:	RecentlySeenSwat.AddUnique(SensedCharacter); break;
		case ETeamType::TT_SUSPECT:		RecentlySeenSuspects.AddUnique(SensedCharacter); break;
		case ETeamType::TT_CIVILIAN:	RecentlySeenCivilians.AddUnique(SensedCharacter); break;
		case ETeamType::TT_SQUAD:		RecentlySeenSwat.AddUnique(SensedCharacter); break;
		default: break;
	}

	if (CanSpotCharacter(SensedCharacter))
	{
		if (IsCharacterNeutral(SensedCharacter))
		{
			SpottedNeutral(SensedCharacter);
		}

		if (IsCharacterEnemy(SensedCharacter))
		{
			SpottedEnemy(SensedCharacter);
		}

		if (IsCharacterFriendly(SensedCharacter))
		{
			SpottedFriendly(SensedCharacter);
		}
	}
}

void ACyberneticController::OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	if (!GetCharacter()->IsActive())
	{
		return;
	}

	if (Stimulus.Tag == "Footstep")
	{
		return;
	}

	// ignore stimulus above or below us (floors of a building for example)
	const float ZHeightDifference = FMath::Abs(Stimulus.ReceiverLocation.Z - Stimulus.StimulusLocation.Z);

	if (ZHeightDifference > 150.0f)
	{
		return;
	}
	
	if (SensedCharacter->IsOnSWATTeam() && AwarenessState >= EAIAwarenessState::Suspicious)
	{
		if (!bHasEverHeardSwat)
		{
			bHasEverHeardSwat = true;

			const float Chance = AI_CONFIG_GET_FLOAT("ChanceToPlayVoicelineWhenHeardSwat", 0.7f);
			if (FMath::FRand() <= Chance)
			{
				GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::HEARD_SWAT, true, 30.0f);
			}
		}
	}
}

void ACyberneticController::OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
}

void ACyberneticController::InvestigateStimulus(const FAIStimulus& Stimulus)
{
	// Don't investigate stimulus above or below us
	{
		const float MaxZ = FMath::Max(Stimulus.ReceiverLocation.Z, Stimulus.StimulusLocation.Z);
		const float MinZ = FMath::Min(Stimulus.ReceiverLocation.Z, Stimulus.StimulusLocation.Z);
		
		const float ZHeightDifference = MaxZ - MinZ;

		if (ZHeightDifference > 150.0f)
		{
			return;
		}
	}
	
	if (GetCurrentActivity<UInvestigateStimulusActivity>() == InvestigateStimulusActivity)
	{
		FinishActivity(InvestigateStimulusActivity, false, true);
	}

	InvestigateStimulusActivity->ResetData();
	InvestigateStimulusActivity->Stimulus = Stimulus;
	InvestigateStimulusActivity->Location = Stimulus.StimulusLocation;
	
	UActivityManager::GiveActivityTo(InvestigateStimulusActivity, GetCharacter(), false);
}

bool ACyberneticController::DoesCharacterMatchTargetType(AReadyOrNotCharacter* InCharacter, int32 TargetTypeMask) const
{
	if (!InCharacter)
		return false;

	if ((EAITargetType)TargetTypeMask == EAITargetType::None)
	{
		return true;
	}
	
	if (TargetTypeMask & (int32)EAITargetType::Enemy)
	{
		if (IsCharacterEnemy(InCharacter))
			return true;
	}
	
	if (TargetTypeMask & (int32)EAITargetType::Neutral)
	{
		if (IsCharacterNeutral(InCharacter))
			return true;
	}
	
	if (TargetTypeMask & (int32)EAITargetType::Friendly)
	{
		if (IsCharacterFriendly(InCharacter))
			return true;
	}

	return false;
}

void ACyberneticController::OnDoorExploded(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!IsValid(Door))
		return;

	if (!GetCharacter())
		return;
	
	if (!GetCharacter()->IsActive())
		return;
	
	const float DistanceToDoor = FVector::Distance(GetCharacter()->GetActorLocation(), Door->GetActorLocation());
	if (DistanceToDoor < 1000.0f)
	{
		// Make sure on same level
		const float MaxZ = FMath::Max(GetCharacter()->GetActorLocation().Z, Door->GetDoorMidLocation().Z);
		const float MinZ = FMath::Min(GetCharacter()->GetActorLocation().Z, Door->GetDoorMidLocation().Z);
		
		const float ZHeightDifference = MaxZ - MinZ;
		if (ZHeightDifference < 150.0f)
		{
			if (GetCombatActivity())
			{
				if (IsSuspect())
				{
					if (DistanceToDoor < 500.0f)
					{
						GetCombatActivity()->TryPlayDead(false, true);
					}
				}
			}
		}
	}
}

void ACyberneticController::OnHeardSWAT(AReadyOrNotCharacter* HeardCharacter)
{
	if (!HeardCharacter)
		return;

	if (!HeardCharacter->IsOnSWATTeam())
		return;

	if (GetCombatActivity())
	{
		if (IsSuspect())
		{
			if (FMath::FRand() < 0.7f)
			{
				FCoverInstigatorStimulus InstigatorStimulus;
				InstigatorStimulus.InstigatorCharacter = HeardCharacter;
				InstigatorStimulus.ThreatTransform = HeardCharacter->GetActorTransform();
					
				GetCombatActivity()->TryMoveIntoCover(InstigatorStimulus, false);
			}
			else
			{
				if (!GetCombatActivity()->TryMoveIntoCoverLandmark(HeardCharacter->GetActorLocation(), 1000.0f, HeardCharacter))
				{
					FCoverInstigatorStimulus InstigatorStimulus;
					InstigatorStimulus.InstigatorCharacter = HeardCharacter;
					InstigatorStimulus.ThreatTransform = HeardCharacter->GetActorTransform();
					
					GetCombatActivity()->TryMoveIntoCover(InstigatorStimulus, false);
				}
			}
		}
		else if (IsCivilian())
		{
			GetCombatActivity()->TryMoveIntoCoverLandmark(HeardCharacter->GetActorLocation(), 1000.0f, HeardCharacter);
		}
	}
}

void ACyberneticController::OnHeardGunShot(AReadyOrNotCharacter* InInstigator, const FVector& WeaponLocation, const FName& InTag)
{
	if (!InInstigator || WeaponLocation == FVector::ZeroVector)
		return;

	if (GetCharacter() && GetCharacter()->IsArrestedOrSurrendered())
	{
		GetCharacter()->PlayMontageFromTable(IsSuspect() ? "tp_flinch_suspect" : "tp_flinch");
		
		GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_HESITATE, true, 10.0f);			
	}
	
	// Only care about the gunshot if not seen anyone
	if (GetTrackedTarget())
		return;

	if (!GetCharacter()->IsArrestedOrSurrendered())
	{
		const float Chance = AI_CONFIG_GET_FLOAT("ChanceToPlayVoicelineWhenHeardSwat", 0.7f);
		if (FMath::FRand() <= Chance)
		{
			if (InInstigator->IsOnSWATTeam())
			{
				GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::HEARD_SWAT, true, 10.0f);			
			}
		}
	}
}

void ACyberneticController::OnHeardDoor(AReadyOrNotCharacter* InInstigator, ADoor* Door, const FName& InTag)
{
	if (!InInstigator || !Door)
		return;

	// Only care about the door if not seen anyone or doing anything
	if (GetTrackedTarget() || (GetCurrentActivity() && !GetCurrentActivity<UInvestigateActivity>()))
		return;

	if (InTag == ADoor::KICK_DOOR_NOISE_TAG)
	{
		LastHeardDoorKick = Door;
		
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_LastHeardDoorKick);
		
		const float HearingAge = AIPerceptionComponent->GetSenseConfig(UAISense::GetSenseID<UAISense_Hearing>())->GetMaxAge();
		if (HearingAge > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_LastHeardDoorKick, this, &ACyberneticController::ClearLastHeardDoorKick, HearingAge);
		}

		GetCharacter()->IncreaseStress(FMath::Clamp(AI_CONFIG_GET_FLOAT("DoorKickStress", 0.1f), 0.0f, 1.0f));
		
		const float Damage = AI_CONFIG_GET_FLOAT("KickDoorMorale.Damage") * (Door->IsOpenBeyond(0.5f) ? AI_CONFIG_GET_FLOAT("KickDoorMorale.SuccessMultiplier", 2.0f) : 1.0f);
		const float InnerRadius = AI_CONFIG_GET_FLOAT("KickDoorMorale.DamageInnerRadius");
		const float OuterRadius = AI_CONFIG_GET_FLOAT("KickDoorMorale.DamageOuterRadius");
		const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("KickDoorMorale.DamageFalloffCurve"));

		UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, Door->GetDoorMidLocation(), Damage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT}, Curve, "Kick Door");
	}
}

void ACyberneticController::OnHeardGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation, const FName& InTag)
{
	if (GetCharacter()->IsArrestedOrSurrendered() && !GetCharacter()->IsPlayingDead() && GetCharacter()->TimeSinceLastPlayDead > 2.0f)
	{
		GetCharacter()->PlayMontageFromTable("tp_flinch_grenade");
		return;
	}

	if (!InInstigator || GrenadeLocation == FVector::ZeroVector)
		return;

	// Only care about the grenade if not seen anyone or doing anything
	if (GetTrackedTarget() || (GetCurrentActivity() && !GetCurrentActivity<UInvestigateActivity>()))
		return;
}

void ACyberneticController::OnHeardExplosion(AReadyOrNotCharacter* InInstigator, const FVector& ExplosionLocation)
{
}

bool ACyberneticController::IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return true;
}

bool ACyberneticController::IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return false;
}

bool ACyberneticController::IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return false;
}

void ACyberneticController::OnSeenGrenade(AReadyOrNotCharacter* InInstigator, const FVector& GrenadeLocation)
{
}

void ACyberneticController::OnSeenFlashlight(AReadyOrNotCharacter* InInstigator, const FVector& FlashlightLocation)
{
	if (!InInstigator || FlashlightLocation == FVector::ZeroVector)
		return;
	
	// Only care about the flashlight if not seen anyone or doing anything
	if (GetTrackedTarget() || (GetCurrentActivity() && !GetCurrentActivity<UInvestigateActivity>()))
		return;

	if (!IsSuspect())
		return;

	if (GetCharacter()->IsActive())
	{
		GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_SEEN, true, FlashLightSeenCoolDownDuration);
	}

	if (AwarenessState == EAIAwarenessState::Unalerted)
		AwarenessState = EAIAwarenessState::Suspicious;
	
	// Force combat move
	if (UAIArchetypeData* Archetype = GetCharacter()->Archetype)
	{
		BestCombatMoveAction = UAIArchetypeData::FindActionByType(Archetype->CombatMoveActions, FMath::RandBool() ? EAIAction::Flee : EAIAction::HardCover);

		if (BestCombatMoveAction)
		{
			BestCombatMoveAction->Scores.Add(this, 1.0f);
			BestCombatMoveAction->Commit(this);

			if (CombatActivity)
				CombatActivity->BeginAction(BestCombatMoveAction);
		}
	}
}

void ACyberneticController::OnSeenIncapHuman(AIncapacitatedHuman* IncapHuman)
{
}

void ACyberneticController::OnTrackedTargetKilled(AReadyOrNotCharacter* Insitgator, AReadyOrNotCharacter* KilledCharacter)
{
}

void ACyberneticController::OnTrackedTargetIncapacitated(AReadyOrNotCharacter* KilledCharacter)
{
}

void ACyberneticController::OnTrackedTargetExitedSurrender(ACyberneticCharacter* InCharacter, ESurrenderExitType ExitType)
{
}

void ACyberneticController::OnTrackedTargetStartedReloading(APlayerCharacter* InCharacter)
{
}

void ACyberneticController::MoveToCover(const FCoverInstigatorStimulus& InstigatorStimulus, const bool bRequireLOS)
{
	if (GetCombatActivity())
	{
		if (IsSuspect())
		{
			GetCombatActivity()->TryMoveIntoCover(InstigatorStimulus, bRequireLOS);
		}
		else
		{
			GetCombatActivity()->TryMoveIntoCoverLandmark(InstigatorStimulus.ThreatTransform.GetLocation(), InstigatorStimulus.SearchRadius, InstigatorStimulus.InstigatorCharacter);
		}
	}
}

ETeamType ACyberneticController::GetTeam() const
{
	if (GetCharacter())
	{
		return GetCharacter()->GetTeam();
	}
	
	return ETeamType::TT_NONE;
}

void ACyberneticController::AbortCover()
{
	if (UTakeCoverActivity* TakeCoverActivity = GetCurrentActivity<UTakeCoverActivity>())
	{
		TakeCoverActivity->AbortCoverNow();
	}
}

void ACyberneticController::AbortCoverLandmark()
{
	if (UTakeCoverAtLandmarkActivity* TakeCoverAtLandmarkActivity = GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
	{
		TakeCoverAtLandmarkActivity->AbortCoverNow();
	}
}

void ACyberneticController::OnStunDamageTaken(EStunType StunType)
{
	// TODO: Likely want to transition to alerted regardless of stun type
	// for now, gonna do gas, as it's possible for AI to be gassed from a thrown grenade and yet not become alerted
	switch (StunType)
	{
	case EStunType::ST_Gassed:
		AwarenessState = EAIAwarenessState::Alerted;
		TimeSinceLastExposedToAnyStimulus = 0.0f;
		TimeSinceLastExposedToAggressiveStimulus = 0.0f;
		break;
		
	default:
		break;
	}
}