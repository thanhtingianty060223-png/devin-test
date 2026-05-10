// Void Interactive, 2020

#include "ScoringManager.h"

#include "Actors/Gameplay/CollectedEvidenceActor.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/CivilianCharacter.h"
#include "Characters/AI/SuspectCharacter.h"

#include "Components/ScoringComponent.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "Objectives/NeutralizeSuspectByTag.h"
#include "Objectives/RescueCivilianByTag.h"

#include "Internationalization/StringTableRegistry.h"

// TODO (Max): Sort this out, no need to have here and in SetupScoringText

FName AScoringManager::SCORE_EVIDENCE_SECURED = "EvidenceSecured";
FName AScoringManager::SCORE_REPORT_SUSPECT = "SuspectsSecured";
FName AScoringManager::SCORE_REPORT_CIVILIAN = "CiviliansSecured";
FName AScoringManager::SCORE_REPORT_DOWNED_OFFICER = "DownedOfficersReported";
FName AScoringManager::SCORE_REPORT_INCAPACITATED_BODY = "IncapacitatedBodiesReported";
FName AScoringManager::SCORE_REPORT_DEAD_BODY = "DeadBodiesReported";
FName AScoringManager::SCORE_NO_OFFICERS_DEAD = "NoOfficersDead";
FName AScoringManager::SCORE_NO_OFFICERS_INJURED = "NoOfficersInjured";
FName AScoringManager::SCORE_TRAP_DISARMED = "TrapsDisarmed";
FName AScoringManager::SCORE_EXFILTRATED_MISSION = "ExfiltratedMission";

FText AScoringManager::BONUS_EVIDENCE_SECURED = LOCTABLE("ScoringTable", "EvidenceSecured");
FText AScoringManager::BONUS_SUSPECT_REPORTED = LOCTABLE("ScoringTable", "SuspectReported");
FText AScoringManager::BONUS_CIVILIAN_REPORTED = LOCTABLE("ScoringTable", "CivilianReported");
FText AScoringManager::BONUS_DOWNED_OFFICER_REPORTED = LOCTABLE("ScoringTable", "DownedOfficerReported");
FText AScoringManager::BONUS_INCAPACITATED_BODY_REPORTED = LOCTABLE("ScoringTable", "IncapacitatedBodyReported");
FText AScoringManager::BONUS_INCAPACITATED_BODIES_REPORTED = LOCTABLE("ScoringTable", "IncapacitatedBodiesReported");
FText AScoringManager::BONUS_REPORT_DEAD_BODY_REPORTED = LOCTABLE("ScoringTable", "DeadBodyReported");
FText AScoringManager::BONUS_REPORT_DEAD_BODIES_REPORTED = LOCTABLE("ScoringTable", "DeadBodiesReported");
FText AScoringManager::BONUS_NO_OFFICER_INJURED = LOCTABLE("ScoringTable", "NoInjury");
FText AScoringManager::BONUS_TRAP_DISARMED = LOCTABLE("ScoringTable", "TrapDisarmed");

FText AScoringManager::PENALTY_UNAUTHORIZED_FORCE = LOCTABLE("ScoringTable", "UnauthorizedUseofForce");
FText AScoringManager::PENALTY_UNAUTHORIZED_DEADLY_FORCE = LOCTABLE("ScoringTable", "UnauthorizedUseofDeadlyForce");
FText AScoringManager::PENALTY_FRIENDLY_FIRE = LOCTABLE("ScoringTable", "FriendlyFire");
FText AScoringManager::PENALTY_FRIENDLY_TEAM_KILL = LOCTABLE("ScoringTable", "FriendlyTeamKill");
FText AScoringManager::PENALTY_FAILED_TO_REPORT_DOWNED_OFFICER = LOCTABLE("ScoringTable", "FailedToReportADownedOfficer");
FText AScoringManager::PENALTY_KILLED_INCAP_BODY = LOCTABLE("ScoringTable", "KilledanIncapacitatedHuman");
FText AScoringManager::PENALTY_CIVILIAN_KILLED = LOCTABLE("ScoringTable", "CivilianKilled");
FText AScoringManager::PENALTY_TRAP_TRIGGERED = LOCTABLE("ScoringTable", "TrapTriggered");
FText AScoringManager::PENALTY_EXFILTRATED_MISSION = LOCTABLE("ScoringTable", "ExfiltratedMission");

void AScoringManager::SetupScoringText()
{
	SCORE_EVIDENCE_SECURED = "EvidenceSecured";
	SCORE_REPORT_SUSPECT = "SuspectsSecured";
	SCORE_REPORT_CIVILIAN = "CiviliansSecured";
	SCORE_REPORT_DOWNED_OFFICER = "DownedOfficersReported";
	SCORE_REPORT_INCAPACITATED_BODY = "IncapacitatedBodiesReported";
	SCORE_REPORT_DEAD_BODY = "DeadBodiesReported";
	SCORE_NO_OFFICERS_DEAD = "NoOfficersDead";
	SCORE_NO_OFFICERS_INJURED = "NoOfficersInjured";
	SCORE_TRAP_DISARMED = "TrapsDisarmed";
	SCORE_EXFILTRATED_MISSION = "ExfiltratedMission";

	BONUS_EVIDENCE_SECURED = LOCTABLE("ScoringTable", "EvidenceSecured");
	BONUS_SUSPECT_REPORTED = LOCTABLE("ScoringTable", "SuspectReported");
	BONUS_CIVILIAN_REPORTED = LOCTABLE("ScoringTable", "CivilianReported");
	BONUS_DOWNED_OFFICER_REPORTED = LOCTABLE("ScoringTable", "DownedOfficerReported");
	BONUS_INCAPACITATED_BODY_REPORTED = LOCTABLE("ScoringTable", "IncapacitatedBodyReported");
	BONUS_INCAPACITATED_BODIES_REPORTED = LOCTABLE("ScoringTable", "IncapacitatedBodiesReported");
	BONUS_REPORT_DEAD_BODY_REPORTED = LOCTABLE("ScoringTable", "DeadBodyReported");
	BONUS_REPORT_DEAD_BODIES_REPORTED = LOCTABLE("ScoringTable", "DeadBodiesReported");
	BONUS_NO_OFFICER_INJURED = LOCTABLE("ScoringTable", "NoInjury");
	BONUS_TRAP_DISARMED = LOCTABLE("ScoringTable", "TrapDisarmed");

	PENALTY_UNAUTHORIZED_FORCE = LOCTABLE("ScoringTable", "UnauthorizedUseofForce");
	PENALTY_UNAUTHORIZED_DEADLY_FORCE = LOCTABLE("ScoringTable", "UnauthorizedUseofDeadlyForce");
	PENALTY_FRIENDLY_FIRE = LOCTABLE("ScoringTable", "FriendlyFire");
	PENALTY_FRIENDLY_TEAM_KILL = LOCTABLE("ScoringTable", "FriendlyTeamKill");
	PENALTY_FAILED_TO_REPORT_DOWNED_OFFICER = LOCTABLE("ScoringTable", "FailedToReportADownedOfficer");
	PENALTY_KILLED_INCAP_BODY = LOCTABLE("ScoringTable", "KilledanIncapacitatedHuman");
	PENALTY_CIVILIAN_KILLED = LOCTABLE("ScoringTable", "CivilianKilled");
	PENALTY_TRAP_TRIGGERED = LOCTABLE("ScoringTable", "TrapTriggered");
	PENALTY_EXFILTRATED_MISSION = LOCTABLE("ScoringTable", "ExfiltratedMission");
}


TAutoConsoleVariable<int32> CVarScoringSystemDebug(TEXT("Scoring.Debug"), 0, TEXT("Toggle scoring system debugging (Logs to console). 0 = Disabled, 1 = Enabled"));

AScoringManager::AScoringManager()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.1f;

	SetCanBeDamaged(false);
	NetUpdateFrequency = 1.0f;
	NetPriority = 2.0f;

	bReplicates = true;
	bAlwaysRelevant = true;
	bIsOfficialScoring = true;

	SetupScoringText();
}

void AScoringManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->AddDeathListener(this);
		GameState->AddGameEndListener(this);
	}
}

void AScoringManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AScoringManager, ObjectiveScoreGroups);
	DOREPLIFETIME(AScoringManager, PenaltyScoreGroups);
	DOREPLIFETIME(AScoringManager, TotalScorePool);
	DOREPLIFETIME(AScoringManager, bIsOfficialScoring);
}

void AScoringManager::UpdateScorePool()
{
	int32 TotalPrimaryScore = 0, TotalSecondaryScore = 0, TotalTertiaryScore = 0;

	ObjectiveScoreGroups.RemoveAll([](const FScoreGroup& ScoreGroup)
	{
		return ScoreGroup.Scores.Num() == 0 && ScoreGroup.InactiveScores.Num() == 0;
	});

	ObjectiveScoreGroups.Sort([](const FScoreGroup& Lhs, const FScoreGroup& Rhs)
	{
		return Lhs.OrderPriority < Rhs.OrderPriority;
	});
	
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		TArray<FScoreData> ScoreDataArray;
		ScoreDataArray.Reserve(10);
		ScoreDataArray.Append(ScoreGroup.Scores);
		ScoreDataArray.Append(ScoreGroup.InactiveScores);
		
		for (const FScoreData& InScoringData : ScoreDataArray)
		{
			if (InScoringData.FromScoringComponent)
			{
				switch (ScoreGroup.ObjectiveLevel)
				{
					case EObjectiveLevel::PrimaryObjective:		TotalPrimaryScore += InScoringData.GetTotalScore(false, false); break;
					case EObjectiveLevel::SecondaryObjective:	TotalSecondaryScore += InScoringData.GetTotalScore(false, false); break;
					case EObjectiveLevel::TertiaryObjective:	TotalTertiaryScore += InScoringData.GetTotalScore(false, false); break;
					default: break;
				}
			}
		}
	}
	
	TotalPrimaryScorePool = TotalPrimaryScore;
	TotalSecondaryScorePool = TotalSecondaryScore;
	TotalTertiaryScorePool = TotalTertiaryScore;
	TotalScorePool = TotalPrimaryScore + TotalSecondaryScore + TotalTertiaryScore;

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::LineBreak_Symbol("============");
		ULog::Number(TotalPrimaryScorePool, "Primary Score Pool: ");
		ULog::Number(TotalSecondaryScorePool, "Secondary Score Pool: ");
		ULog::Number(TotalTertiaryScorePool, "Tertiary Score Pool: ");
		ULog::Number(TotalScorePool, "Total Score Pool: ");
		ULog::LineBreak_Symbol("============");
	}
	#endif

	CacheHasClearedMission();
}

void AScoringManager::RemoveScore(UScoringComponent* InScoringComponent)
{
	if (!InScoringComponent)
		return;

	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		int32 FoundIndex = -1;
		for (const FScoreData& ScoreData : ScoreGroup.Scores)
		{
			FoundIndex++;
					
			if (ScoreData.FromScoringComponent == InScoringComponent)
			{
				ScoreGroup.Scores.RemoveAt(FoundIndex);

				break;
			}
		}
	}

	// Remove all score groups that have no score data in them
	ObjectiveScoreGroups.RemoveAll([&](const FScoreGroup& ScoreGroup)
	{
		return ScoreGroup.Scores.Num() == 0 && ScoreGroup.InactiveScores.Num() == 0;
	});
}

void AScoringManager::AddOrChangeToScoreGroup(UScoringComponent* InScoringComponent, const FName& NewGroupName)
{
	if (!InScoringComponent)
		return;

	#if WITH_EDITOR
	ensure(NewGroupName != NAME_None);
	#endif

	// Find the current group we're in
	if (FScoreGroup* CurrentScoreGroup = GetScoreGroup(InScoringComponent))
	{
		// Add our scoring data to the inactive list of that group
		// Remove from the real scores list
		int32 FoundIndex = -1;
		for (const FScoreData& ScoreData : CurrentScoreGroup->Scores)
		{
			FoundIndex++;
	
			if (ScoreData.FromScoringComponent == InScoringComponent)
			{
				CurrentScoreGroup->InactiveScores.AddUnique(ScoreData);
				CurrentScoreGroup->Scores.RemoveAt(FoundIndex);
				
				break;
			}
		}
	}

	// Find the new group we want to move to
	FScoreGroup* FoundScoreGroup = ObjectiveScoreGroups.FindByPredicate([&](const FScoreGroup& Element)
	{
		return Element.GroupName == NewGroupName;
	});

	InScoringComponent->ChangeGroup(NewGroupName);
	
	// If exists, add the new scoring data to the real scores list
	if (FoundScoreGroup)
	{
		FoundScoreGroup->Scores.AddUnique(InScoringComponent->ScoringData); // ScoringData is updated from the component itself
	}
	// Else, make a new score group and initialize it from the scoring data table
	else
	{
		const int32 Index = ObjectiveScoreGroups.AddUnique({NewGroupName});
		FScoreGroup& ScoreGroup = ObjectiveScoreGroups[Index];

		ScoreGroup.Scores.AddUnique(InScoringComponent->ScoringData);
		ScoreGroup.ObjectiveLevel = InScoringComponent->ObjectiveLevel;

		if (const FScoringDataTable* ScoringDataRow = InScoringComponent->ScoreGroup.GetRow<FScoringDataTable>("Update Score Group"))
		{
			ScoreGroup.bRequiredToClearMission = ScoringDataRow->bRequiredToClearMission;
			ScoreGroup.bRequiredToSoftClearMission = ScoringDataRow->bRequiredToSoftClearMission;
			ScoreGroup.OrderPriority = ScoringDataRow->OrderPriority;
		}
	}

	// Remove all score groups that have no score data in them
	ObjectiveScoreGroups.RemoveAll([&](const FScoreGroup& Element)
	{
		return Element.Scores.Num() == 0 && Element.InactiveScores.Num() == 0;
	});
}

void AScoringManager::DisplayBonusesAndPenalties(UScoringComponent* InScoringComponent, const bool bCondensed, const FText& ScoreTextOverride)
{
	if (!InScoringComponent)
		return;

	if (bCondensed)
	{
		int32 RunningTotal_Bonus = 0;
		for (FScoreBonus* Bonus : GetBonuses(InScoringComponent))
		{
			if (Bonus->bGiven && !Bonus->bWasDisplayedOnHUD)
			{
				RunningTotal_Bonus += Bonus->Score;
				Bonus->bWasDisplayedOnHUD = true;
			}
		}

		int32 RunningTotal_Penalty = 0;
		for (FScorePenalty* Penalty : GetPenalties(InScoringComponent))
		{
			if (Penalty->bGiven && !Penalty->bWasDisplayedOnHUD)
			{
				RunningTotal_Penalty += Penalty->Score;
				Penalty->bWasDisplayedOnHUD = true;
			}
		}

		const int32 RunningTotal = RunningTotal_Bonus - RunningTotal_Penalty;
		
		FText ScoreGroupNameLocalized = FText::FromStringTable("ScoringTable", InScoringComponent->ScoreGroupName.ToString());		
		DisplayScore(ScoreTextOverride.IsEmpty() ? ScoreGroupNameLocalized : ScoreTextOverride, FMath::Abs(RunningTotal), RunningTotal > 0);
	}
	else
	{
		DisplayBonuses(InScoringComponent, false, FText::GetEmpty());
		DisplayPenalties(InScoringComponent, false, FText::GetEmpty());
	}
}

void AScoringManager::DisplayBonuses(UScoringComponent* InScoringComponent, const bool bCondensed, const FText& ScoreTextOverride)
{
	if (!InScoringComponent)
		return;

	if (bCondensed)
	{
		int32 RunningTotal = 0;
		for (FScoreBonus* Bonus : GetBonuses(InScoringComponent))
		{
			if (Bonus->bGiven && !Bonus->bWasDisplayedOnHUD)
			{
				RunningTotal += Bonus->Score;
				Bonus->bWasDisplayedOnHUD = true;
			}
		}
		
		FText ScoreGroupNameLocalized = FText::FromStringTable("ScoringTable", InScoringComponent->ScoreGroupName.ToString());		
		DisplayScore(ScoreTextOverride.IsEmpty() ? ScoreGroupNameLocalized : ScoreTextOverride, RunningTotal, true);
	}
	else
	{
		for (FScoreBonus* Bonus : GetBonuses(InScoringComponent))
		{
			if (Bonus->bGiven && !Bonus->bWasDisplayedOnHUD)
			{
				DisplayScore(Bonus->ScoreTextOnHUD, Bonus->Score, true);
				Bonus->bWasDisplayedOnHUD = true;
			}
		}
	}
}

void AScoringManager::DisplayPenalties(UScoringComponent* InScoringComponent, const bool bCondensed, const FText& ScoreTextOverride)
{
	if (!InScoringComponent)
		return;

	if (bCondensed)
	{
		int32 RunningTotal = 0;
		FText PenaltyName = ScoreTextOverride;
		for (FScorePenalty* Penalty : GetPenalties(InScoringComponent))
		{
			if (Penalty->bGiven && !Penalty->bWasDisplayedOnHUD)
			{
				RunningTotal += Penalty->Score;
				PenaltyName = Penalty->ScoreName;
				Penalty->bWasDisplayedOnHUD = true;
			}
		}

		DisplayScore(PenaltyName, RunningTotal, false);
	}
	else
	{
		for (FScorePenalty* Penalty : GetPenalties(InScoringComponent))
		{
			if (Penalty->bGiven && !Penalty->bWasDisplayedOnHUD)
			{
				DisplayScore(Penalty->ScoreTextOnHUD, Penalty->Score, false);
				Penalty->bWasDisplayedOnHUD = true;
			}
		}
	}
}

void AScoringManager::GiveScore(UScoringComponent* InScoringComponent, const FText& ScoreName, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	if (!InScoringComponent)
		return;

	FScoreBonus* Bonus = GetBonus(InScoringComponent, ScoreName);
	if (!Bonus)
		return;

	if (!Bonus->bEnabled || Bonus->bGiven)
		return;

	const int32 BonusScore = Bonus->Score;
	
	if (BonusScore == 0)
		return;

	Bonus->bGiven = true;
	Bonus->ScoreTextOnHUD = ScoreText.IsEmpty() ? ScoreName : ScoreText;

	CalculateTotalPlayerScore();

	if (bDisplayScoreOnHUD && !Bonus->bWasDisplayedOnHUD)
	{
		Bonus->bWasDisplayedOnHUD = true;
		
		const int32 ScoreDisplayedOnHUD = CustomScoreOnHUD > -1 ? CustomScoreOnHUD : BonusScore;
		DisplayScore(Bonus->ScoreTextOnHUD, ScoreDisplayedOnHUD, true, DisplayOnHUDDelay);
	}

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Number(BonusScore, "Score Given from " + InScoringComponent->GetOwner()->GetName() + ": ");

		LogScores();
	}
	#endif
}

void AScoringManager::GiveFakeScore(UScoringComponent* InScoringComponent, const FText& ScoreName, bool bDisplayScoreOnHUD, const FText& ScoreText, float DisplayOnHUDDelay, int32 CustomScoreOnHUD)
{
	if (!InScoringComponent)
		return;

	FScoreBonus* Bonus = GetBonus(InScoringComponent, ScoreName);
	if (!Bonus)
		return;

	if (!Bonus->bEnabled || Bonus->bGiven)
		return;

	const int32 BonusScore = Bonus->Score;
	
	if (BonusScore == 0)
		return;

	Bonus->ScoreTextOnHUD = ScoreText.IsEmpty() ? ScoreName : ScoreText;

	if (bDisplayScoreOnHUD)
	{
		const int32 ScoreDisplayedOnHUD = CustomScoreOnHUD > -1 ? CustomScoreOnHUD : BonusScore;
		DisplayScore(Bonus->ScoreTextOnHUD, ScoreDisplayedOnHUD, true, DisplayOnHUDDelay);
	}
}

void AScoringManager::GiveScores(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	if (!InScoringComponent)
		return;

	for (const FText& ScoreName : ScoreNames)
	{
		GiveScore(InScoringComponent, ScoreName, bDisplayScoreOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
	}
}

void AScoringManager::GiveAllScores(UScoringComponent* InScoringComponent, const bool bOnlyEnabledScore, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	if (!InScoringComponent)
		return;
	
	int32 TotalGiveScore = 0;
	for (FScoreBonus* ScoreBonus : GetBonuses(InScoringComponent))
	{
		if (bOnlyEnabledScore)
		{
			if (ScoreBonus->bEnabled && !ScoreBonus->bGiven)
			{
				TotalGiveScore += ScoreBonus->Score;
				ScoreBonus->bGiven = true;
				ScoreBonus->ScoreTextOnHUD = ScoreText.IsEmpty() ? ScoreBonus->ScoreName : ScoreText;
			}
		}
		else
		{
			if (!ScoreBonus->bGiven)
			{
				TotalGiveScore += ScoreBonus->Score;
				ScoreBonus->bGiven = true;
				ScoreBonus->ScoreTextOnHUD = ScoreText.IsEmpty() ? ScoreBonus->ScoreName : ScoreText;
			}
		}
	}
	
	CalculateTotalPlayerScore();

	if (TotalGiveScore == 0)
		return;

	if (bDisplayScoreOnHUD)
	{
		const int32 ScoreDisplayedOnHUD = CustomScoreOnHUD > -1 ? CustomScoreOnHUD : TotalGiveScore;

		DisplayScore(ScoreText, ScoreDisplayedOnHUD, true, DisplayOnHUDDelay);
	}

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Number(TotalGiveScore, "Score Given from " + InScoringComponent->GetOwner()->GetName() + ": ");

		LogScores();
	}
	#endif
}

void AScoringManager::TakeScore(UScoringComponent* InScoringComponent, const FText& ScoreName, const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScore)
{
	if (!InScoringComponent)
		return;
	
	FScoreBonus* Bonus = GetBonus(InScoringComponent, ScoreName);
	if (!Bonus)
		return;

	if (!Bonus->bGiven)
		return;
	
	const int32 CalculatedScore = Bonus->Score;
	
	if (CalculatedScore == 0)
		return;

	if (bDisableScore)
	{
		Bonus->bEnabled = false;
		Bonus->bGiven = false;
		Bonus->bWasDisplayedOnHUD = false;
	}

	CalculateTotalPlayerScore();

	if (bDisplayScoreOnHUD)
	{
		DisplayScore(TakeReason, CalculatedScore, false);
	}

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Info(FString::FromInt(CalculatedScore) + " Score given back to " + InScoringComponent->GetOwner()->GetName());

		LogScores();
	}
	#endif
}

void AScoringManager::TakeScores(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScores)
{
	if (!InScoringComponent)
		return;

	for (const FText& ScoreName : ScoreNames)
	{
		TakeScore(InScoringComponent, ScoreName, TakeReason, bDisplayScoreOnHUD, bDisableScores);
	}
}

void AScoringManager::TakeAllScores(UScoringComponent* InScoringComponent, const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScores)
{
	// Take away everything
	TakeAllScoresExcept(InScoringComponent, {}, TakeReason, bDisplayScoreOnHUD, bDisableScores);
}

void AScoringManager::TakeAllScoresExcept(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, const FText& TakeReason, bool bDisplayScoreOnHUD, bool bDisableScores)
{
	if (!InScoringComponent)
		return;

	int32 TotalTakeAwayScore = 0;

	for (FScoreBonus* ScoreBonus : GetBonuses(InScoringComponent))
	{
		//if (ScoreNames.Num() == 0 || !ScoreNames.Contains(ScoreBonus->ScoreName))
		if (ScoreNames.Num() == 0 || !ScoreNames.ContainsByPredicate([&](const FText& Element){ return Element.EqualTo(ScoreBonus->ScoreName); }))
		{
			if (ScoreBonus->bGiven/* || (Breakdown.bPenaltyOnly && !Breakdown.bPenaltyGiven)*/)
			{
				/*if (ScoreBonus.bTaken)
					return;*/

				TotalTakeAwayScore += ScoreBonus->Score;

				if (bDisableScores)
				{
					ScoreBonus->bEnabled = false;
					ScoreBonus->bGiven = false;
					ScoreBonus->bWasDisplayedOnHUD = false;
				}
			}
		}
	}

	CalculateTotalPlayerScore();

	if (TotalTakeAwayScore == 0 && !InScoringComponent->bShowPopupOnTakeAllScoresWithNoChange)
		return;
	
	if (bDisplayScoreOnHUD)
	{
		DisplayScore(TakeReason, TotalTakeAwayScore, false, 0.f, false);
	}

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Info(FString::FromInt(TotalTakeAwayScore) + " Score given back to " + InScoringComponent->GetOwner()->GetName());

		LogScores();
	}
	#endif
}

void AScoringManager::GivePenalty(UScoringComponent* InScoringComponent, const FText& PenaltyGroupName, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	if (!InScoringComponent)
		return;

	FScorePenalty* Penalty = GetPenalty(InScoringComponent, PenaltyGroupName);
	if (!Penalty)
		return;

	if (!Penalty->bEnabled || Penalty->bGiven)
		return;

	if (Penalty->Score == 0)
		return;

	Penalty->bGiven = true;
	Penalty->ScoreTextOnHUD = ScoreText.IsEmpty() ? PenaltyGroupName : ScoreText;

	InScoringComponent->LastGivenPenalty = *Penalty;

	const int32 Index = PenaltyScoreGroups.AddUnique({PenaltyGroupName});
	FScorePenaltyData& PenaltyGroup = PenaltyScoreGroups[Index];
	
	PenaltyGroup.Score += Penalty->Score;
	PenaltyGroup.PenaltyCount++;

	CalculateTotalPlayerScore();

	if (bDisplayScoreOnHUD && !Penalty->bWasDisplayedOnHUD)
	{
		Penalty->bWasDisplayedOnHUD = true;
		
		const int32 ScoreDisplayedOnHUD = CustomScoreOnHUD > -1 ? CustomScoreOnHUD : Penalty->Score;
		
		DisplayScore(Penalty->ScoreTextOnHUD, ScoreDisplayedOnHUD, false, DisplayOnHUDDelay);
	}
}

void AScoringManager::RevokePenalty(UScoringComponent* InScoringComponent, const FText& PenaltyGroupName)
{
	if (!InScoringComponent)
		return;

	if (RevokePenalty(GetPenalty(InScoringComponent, PenaltyGroupName)))
	{
		InScoringComponent->LastGivenPenalty = FScorePenalty();

		#if WITH_EDITOR
		if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
		{
			ULog::Info(FString::FromInt(GetPenalty(InScoringComponent, PenaltyGroupName)->Score) + " Score given back to " + InScoringComponent->GetOwner()->GetName());

			LogScores();
		}
		#endif
	}
}

bool AScoringManager::RevokePenalty(FScorePenalty* InPenalty)
{
	if (!InPenalty)
		return false;

	if (!InPenalty->bEnabled || !InPenalty->bGiven)
		return false;

	if (InPenalty->Score == 0)
		return false;

	InPenalty->bGiven = false;
	InPenalty->bWasDisplayedOnHUD = false;

	const int32 PenaltyIndex = PenaltyScoreGroups.Find({InPenalty->ScoreName});
	if (PenaltyScoreGroups.IsValidIndex(PenaltyIndex))
	{
		FScorePenaltyData& PenaltyGroup = PenaltyScoreGroups[PenaltyIndex];
		
		PenaltyGroup.Score -= InPenalty->Score;
		PenaltyGroup.PenaltyCount--;
	}

	CalculateTotalPlayerScore();

	return true;
}

void AScoringManager::RevokeAllPenalties(UScoringComponent* InScoringComponent)
{
	if (!InScoringComponent)
		return;

	TArray<FScorePenalty*> Penalties = GetPenalties(InScoringComponent);
	if (Penalties.Num() == 0)
		return;

	for (FScorePenalty* Penalty : Penalties)
	{
		RevokePenalty(Penalty);
	}

	CalculateTotalPlayerScore();
}

void AScoringManager::GiveCustomPenalty(const FText& PenaltyGroupName, const int32 PenaltyScore, const bool bDisplayScoreOnHUD, const float DisplayOnHUDDelay)
{
	if (PenaltyScore == 0)
		return;
	
	const int32 Index = PenaltyScoreGroups.AddUnique({PenaltyGroupName});
	FScorePenaltyData& PenaltyGroup = PenaltyScoreGroups[Index];
	PenaltyGroup.Score += PenaltyScore;
	PenaltyGroup.PenaltyCount++;

	CalculateTotalPlayerScore();

	if (bDisplayScoreOnHUD)
	{
		DisplayScore(PenaltyGroupName, PenaltyScore, false, DisplayOnHUDDelay);
	}
}

void AScoringManager::ChangeScoreGroup(UScoringComponent* InScoringComponent, const FName NewGroupName)
{
	if (!InScoringComponent)
		return;

	AddOrChangeToScoreGroup(InScoringComponent, NewGroupName);
	
	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::LineBreak_Symbol("============");
		ULog::Success("Score Data for " + InScoringComponent->GetOwner()->GetName() + " has been updated");
		ULog::Number(InScoringComponent->GetTotalScore(), "New total score: ");
		ULog::Info("New group name: " + NewGroupName.ToString());
		ULog::LineBreak_Symbol("============");
	}
	#endif

	UpdateScorePool();
}

void AScoringManager::OnCharacterDied_Implementation(AReadyOrNotCharacter* Victim, AReadyOrNotCharacter* Killer, AActor* Inflictor)
{
	if (!Victim || !Killer)
		return;
	
	if (Killer->IsPlayerControlled() && Victim->IsCivilian())
	{
		bCivilianHasBeenKilledByPlayer = true;
	}
	
	for (TActorIterator<ARescueCivilianByTag> It(GetWorld()); It; ++It)
	{
		ARescueCivilianByTag* Objective = *It;

		if (Victim->Tags.Contains(Objective->CivilianTag))
		{
			Objective->ObjectiveFailed();
		}
	}

	if (bCivilianHasBeenKilledByPlayer)
	{
		for (TActorIterator<ARescueAllOfTheCivilians>It(GetWorld()); It; ++It)
		{
			It->ObjectiveFailed();
		}
	}
	
	CacheHasClearedMission();
}

void AScoringManager::AddScoreToPool(UScoringComponent* InScoringComponent, const FName& GroupName, const bool bForce)
{
	if (!InScoringComponent)
		return;

	if (!InScoringComponent->bEnabled)
		return;

	if (ContainsScoreComponent(InScoringComponent) && !bForce)
		return;

	if (InScoringComponent->GetTotalScore() == 0)
		return;
	
	AddOrChangeToScoreGroup(InScoringComponent, GroupName);

	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Info(InScoringComponent->GetOwner()->GetName() + " added " + FString::FromInt(InScoringComponent->GetTotalScore()) + " score to score pool");
	}
	#endif
	
	UpdateScorePool();
}

void AScoringManager::RemoveScoreFromPool(UScoringComponent* InScoringComponent)
{
	if (!InScoringComponent)
		return;

	if (!ContainsScoreComponent(InScoringComponent))
		return;

	TakeAllScores(InScoringComponent);
	RemoveScore(InScoringComponent);
	
	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::Info(InScoringComponent->GetOwner()->GetName() + " removed " + FString::FromInt(InScoringComponent->GetTotalScore()) + " score from score pool");
	}
	#endif
	
	UpdateScorePool();
}

bool AScoringManager::ContainsScoreComponent(UScoringComponent* InScoringComponent) const
{
	if (!InScoringComponent)
		return false;

	for (const FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (const FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == InScoringComponent)
				return true;
		}
	}

	return false;
}

void AScoringManager::UpdateObjectives()
{
	for (TActorIterator<AObjective> It(GetWorld()); It; ++It)
	{
		AObjective* Obj = *It;
		
		if (Obj->IsObjectiveInProgress())
			Obj->TickObjective();
	}

	CacheHasClearedMission();
}

AScoringManager* AScoringManager::Get()
{
	if (!UBpGameplayHelperLib::GetWorldStatic())
		return nullptr;

	if (const AReadyOrNotGameState* GS = UBpGameplayHelperLib::GetWorldStatic()->GetGameState<AReadyOrNotGameState>())
	{
		return GS->GetScoringManager();
	}
	
	return nullptr;
}

TArray<FScoreGroup> AScoringManager::GetScoreGroups() const
{
	return ObjectiveScoreGroups;
}

TArray<FScorePenaltyData> AScoringManager::GetPenaltyScoreGroups() const
{
	return PenaltyScoreGroups;
}

int32 AScoringManager::GetGivenScoreCountFromGroup(const TArray<FScoreGroup>& InScoreGroupArray) const
{
	int32 TotalGivenCount = 0;
	for (const FScoreGroup& ScoreGroup : InScoreGroupArray)
	{
		TotalGivenCount += GetGivenScoreCountFromArray(ScoreGroup.Scores); 
	}

	return TotalGivenCount;
}

int32 AScoringManager::GetNonGivenScoreCountFromGroup(const TArray<FScoreGroup>& InScoreGroupArray) const
{
	int32 TotalNonGivenCount = 0;
	for (const FScoreGroup& ScoreGroup : InScoreGroupArray)
	{
		TotalNonGivenCount += GetNonGivenScoreCountFromArray(ScoreGroup.Scores, true); 
	}

	return TotalNonGivenCount;
}

int32 AScoringManager::GetGivenScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, const bool bIncludeHiddenScores) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 ScoreCounter = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		if (!ScoreData.bHiddenScore || (ScoreData.bHiddenScore && bIncludeHiddenScores))
		{
			//if (AllScoresGiven(ScoreData.Scores))
			if (AllRequiredScoresGiven(ScoreData.Bonuses))
			{
				ScoreCounter++;
			}
		}
	}

	return ScoreCounter;
}

int32 AScoringManager::GetTotalScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, const bool bIncludeHiddenScores) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 ScoreCounter = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		if (!ScoreData.bHiddenScore || (ScoreData.bHiddenScore && bIncludeHiddenScores))
		{
			ScoreCounter++;
		}
	}

	return ScoreCounter;
}

int32 AScoringManager::GetTotalGivenScoresFromArray(const TArray<FScoreData>& InScoreDataArray, const bool bIncludeHiddenScores) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 RunningTotal = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		if (!ScoreData.bHiddenScore || (ScoreData.bHiddenScore && bIncludeHiddenScores))
		{
			RunningTotal += ScoreData.GetTotalScore(true, true);
		}
	}
	
	return RunningTotal;
}

int32 AScoringManager::GetNonGivenScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, const bool bOnlyEnabled) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 ScoreCounter = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		if (bOnlyEnabled)
		{
			for (const FScoreBonus& ScoreBonus : ScoreData.Bonuses)
			{
				if (ScoreBonus.bEnabled && !ScoreBonus.bGiven)
				{
					ScoreCounter++;
				}
			}
		}
		else
		{
			if (!AllScoresGiven(ScoreData.Bonuses))
			{
				ScoreCounter++;
			}
		}
	}

	return ScoreCounter;
}

int32 AScoringManager::GetTotalNonGivenScoresFromArray(const TArray<FScoreData>& InScoreDataArray) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 RunningTotal = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		const int32 Difference = ScoreData.GetTotalScore(true, true) - ScoreData.GetTotalScore(false, false);
		RunningTotal += Difference;
	}

	return RunningTotal;
}

int32 AScoringManager::GetTotalScoreFromArray(const TArray<FScoreData>& InScoreDataArray) const
{
	if (InScoreDataArray.Num() == 0)
		return 0;
	
	int32 RunningTotal = 0;

	for (const FScoreData& ScoreData : InScoreDataArray)
	{
		RunningTotal += ScoreData.GetTotalScore(false, false);
	}

	return RunningTotal;
}

int32 AScoringManager::GetTotalScoreFromComponent(const UScoringComponent* InScoringComponent, const bool bOnlyEnabled, const bool bOnlyGiven) const
{
	if (!InScoringComponent)
		return 0;

	int32 RunningTotal = 0;

	if (const FScoreData* ScoreData = GetScoreData_Const(InScoringComponent))
	{
		RunningTotal += ScoreData->GetTotalScore(bOnlyEnabled, bOnlyGiven);
	}
	
	return RunningTotal;
}

int32 AScoringManager::GetScoreFromComponent(const UScoringComponent* InScoringComponent, const FText& InScoreName) const
{
	if (!InScoringComponent)
		return 0;

	int32 RunningTotal = 0;

	if (const FScoreData* ScoreData = GetScoreData_Const(InScoringComponent))
	{
		RunningTotal += ScoreData->GetScore(InScoreName);
	}
	
	return RunningTotal;
}

bool AScoringManager::AllScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const
{
	if (InScoreBonusArray.Num() == 0)
		return false;

	for (const FScoreBonus& ScoreBreakdown : InScoreBonusArray)
	{
		if (!ScoreBreakdown.bEnabled || (ScoreBreakdown.bEnabled && !ScoreBreakdown.bGiven))
		{
			return false;
		}
    }

	return true;
}

bool AScoringManager::AllRequiredScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const
{
	if (InScoreBonusArray.Num() == 0)
		return false;

	for (const FScoreBonus& ScoreBreakdown : InScoreBonusArray)
	{
		if (ScoreBreakdown.bRequired && (!ScoreBreakdown.bEnabled || (ScoreBreakdown.bEnabled && !ScoreBreakdown.bGiven)))
		{
			return false;
		}
	}

	return true;
}

bool AScoringManager::AnyScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const
{
	if (InScoreBonusArray.Num() == 0)
		return false;

	for (const FScoreBonus& ScoreBreakdown : InScoreBonusArray)
	{
		if (ScoreBreakdown.bGiven)
		{
			return true;
		}
	}

	return false;
}

int32 AScoringManager::GetTotalActorsTrackingScore() const
{
	int32 TotalCount = 0;

	for (const FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		TotalCount += ScoreGroup.Scores.Num();
	}

	return TotalCount;
}


void AScoringManager::GetSuspectCount(int32& OutReported, int32& OutArrested, int32& OutKilled, int32& OutTotal)
{
	OutReported = 0;
	OutArrested = 0;
	OutKilled = 0;
	OutTotal = 0;
	
	for (TActorIterator<ASuspectCharacter> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const ASuspectCharacter* AICharacter = *It;
		
		OutTotal++;
		
		if (AICharacter->IsDeadOrUnconscious() || AICharacter->IsIncapacitated())
		{	
			OutKilled++;
		}
		else if (AICharacter->IsArrested())
		{
			OutArrested++;
		}
		else if (AICharacter->HasBeenReported())
		{
			OutReported++;
		}
	}
}


void AScoringManager::GetCivilianCount(int32& OutReported, int32& OutInjured, int32& OutKilled, int32& OutArrested, int32& OutTotal)
{
	OutReported = 0;
	OutArrested = 0;
	OutKilled = 0;
	OutTotal = 0;
	
	for (TActorIterator<ACivilianCharacter> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const ACivilianCharacter* AICharacter = *It;
		
		OutTotal++;

		if (AICharacter->IsInjured())
		{
			OutInjured++;
		}
		
		if (AICharacter->IsDeadOrUnconscious() || AICharacter->IsIncapacitated())
		{	
			OutKilled++;
		}
		else if (AICharacter->IsArrested())
		{
			OutArrested++;
		}
		else if (AICharacter->HasBeenReported())
		{
			OutReported++;
		}
	}
}

void AScoringManager::GetEvidenceCount(int32& EvidenceCollected, int32& TotalEvidence)
{
	GetScoreCountFromGroup(SCORE_EVIDENCE_SECURED, EvidenceCollected, TotalEvidence);
}

void AScoringManager::GetScoreCountFromGroup(const FName& InGroupName, int32& OutScoresGiven, int32& OutTotalScores, bool bRequiredOnly)
{
	if (!Get())
		return;
	
	OutScoresGiven = 0;
	OutTotalScores = 0;

	for (FScoreGroup& ScoreGroup : Get()->ObjectiveScoreGroups)
	{
		if (ScoreGroup.GroupName == InGroupName)
		{
			for (FScoreData& ScoreData : ScoreGroup.Scores)
			{
				if (bRequiredOnly ? ScoreData.HasGivenAllRequiredScores() : ScoreData.HasGivenAllScores())
				{
					OutScoresGiven++;
				}

				OutTotalScores++;
			}
		}
	}
}

void AScoringManager::GetReportCount(int32& ReportedCount, int32& TotalReports)
{
	if (!Get())
		return;

	int32 SuspectsReported, SuspectsTotal;
	int32 CiviliansReported, CiviliansTotal;

	GetScoreCountFromGroup(SCORE_REPORT_SUSPECT, SuspectsReported, SuspectsTotal);
	GetScoreCountFromGroup(SCORE_REPORT_CIVILIAN, CiviliansReported, CiviliansTotal);
	
	ReportedCount = SuspectsReported + CiviliansReported;
	TotalReports = SuspectsTotal + CiviliansTotal;
}

void AScoringManager::HasClearedMission(bool& bHasClearedMission, bool& bSoftClearedMission, bool& bMissionFailed)
{
	if (!Get())
		return;

	bHasClearedMission = Get()->bCachedHasClearedMission;
	bSoftClearedMission = Get()->bCachedSoftClearedMission;
	bMissionFailed = Get()->bCachedMissionFailed;
}

bool AScoringManager::IsOfficialScoring()
{
	if (!Get())
		return false;
	
	return Get()->bIsOfficialScoring;
}

void AScoringManager::CacheHasClearedMission()
{
	bool bAllRequiredScoresGiven = false;
	bool bAllRequiredScoresGivenForSoftComplete = false;
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		if (ScoreGroup.bRequiredToClearMission || ScoreGroup.bRequiredToSoftClearMission)
		{
			int32 ScoresGiven, ScoresTotal = 0;

			GetScoreCountFromGroup(ScoreGroup.GroupName, ScoresGiven, ScoresTotal, true);

			if (ScoresGiven < ScoresTotal)
			{
				bAllRequiredScoresGiven = false;
				
				if (ScoreGroup.bRequiredToSoftClearMission)
					bAllRequiredScoresGivenForSoftComplete = false;
				
				break;
			}
			
			bAllRequiredScoresGiven = true;
			bAllRequiredScoresGivenForSoftComplete = true;
		}
	}

	bCachedHasClearedMission = bAllRequiredScoresGiven;
	bCachedSoftClearedMission = bAllRequiredScoresGivenForSoftComplete;
	bCachedMissionFailed = HasCivilianBeenKilledByPlayer();

	for (TActorIterator<AObjective> It(GetWorld()); It; ++It)
	{
		const AObjective* Obj = *It;
		if (Obj->IsObjectiveFailed() && Obj->bFailureEndsMission)
		{
			bCachedMissionFailed = true;
		}
	}
}

void AScoringManager::GetObjectiveCompletionStatus(int32& ObjectivesComplete, int32& ObjectivesFailed, int32& TotalObjectives)
{
	ObjectivesComplete = 0;
	ObjectivesFailed = 0;
	TotalObjectives = 0;
	
	for (TActorIterator<AObjective> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const AObjective* Obj = *It;
		
		TotalObjectives++;
		
		if (Obj->IsObjectiveCompleted())
		{
			ObjectivesComplete++;
		}
		else if (Obj->IsObjectiveFailed())
		{
			ObjectivesFailed++;
		}
	}
}

void AScoringManager::GetTotalScore(int32& TotalScore, const bool bIncludePrimaryScores)
{
	TotalScore = CalculateTotalPlayerScore();

	TotalScore = FMath::Clamp(TotalScore, 0, TotalScorePool);
}

void AScoringManager::GetTotalScorePool(int32& MaxScore)
{
	MaxScore = TotalScorePool;
}

FString AScoringManager::CalculateGradeLetterFromScore(const int32 Score) const
{
	if (Score <= 0)
		return FString("F");

	const float ScorePercentage = FMath::Clamp((float)Score/(float)TotalScorePool, 0.0f, 1.0f);

	if (ScorePercentage < 0.5f && AllObjectivesComplete(EObjectiveLevel::PrimaryObjective))
		return FString("E");
	
	return CalculateGradeLetterFromPercentage(ScorePercentage);
}

FString AScoringManager::CalculateGradeLetterFromPercentage(float ScorePercentage) 
{
	ScorePercentage = FMath::Clamp(ScorePercentage, 0.0f, 1.0f);
	
	if (ScorePercentage >= 1.0f)
	{
		return FString("S");
	}

	if (ScorePercentage >= 0.95f)
	{
		return FString("A+");
	}

	if (ScorePercentage >= 0.9f)
	{
		return FString("A");
	}

	if (ScorePercentage >= 0.8f)
	{
		return FString("B");
	}

	if (ScorePercentage >= 0.7f)
	{
		return FString("C");
	}

	if (ScorePercentage >= 0.6f)
	{
		return FString("D");
	}

	if (ScorePercentage >= 0.5f)
	{
		return FString("E");
	}

	return FString("F");
}

float AScoringManager::GetCurrentScoreAsPercentage() const
{
	return FMath::Clamp((float)CalculateTotalPlayerScore()/(float)TotalScorePool, 0.0f, 1.0f);
}

FString AScoringManager::CalculateGradeLetterFromPlayerScore()
{
	return CalculateGradeLetterFromScore(FMath::Clamp(CalculateTotalPlayerScore(), 0, TotalScorePool));
}

float AScoringManager::GetFinalGradePercentage()
{
	int32 Score = FMath::Clamp(CalculateTotalPlayerScore(), 0, TotalScorePool);
	
	float ScorePercentage = FMath::Clamp((float)Score/(float)TotalScorePool, 0.0f, 1.0f);

	// Minimum E if all objectives completed
	if (ScorePercentage < 0.5f && AllObjectivesComplete(EObjectiveLevel::PrimaryObjective))
		ScorePercentage = 0.5f;

	return ScorePercentage;
}

TSet<FString> AScoringManager::GetCompletedGradesUpToScore(float ScorePercentage)
{
	TSet<FString> CompletedGrades;
	
	if (ScorePercentage >= 1.0f)
		CompletedGrades.Add("s");

	if (ScorePercentage >= 0.95f)
		CompletedGrades.Add("aplus");

	if (ScorePercentage >= 0.9f)
		CompletedGrades.Add("a");
	
	if (ScorePercentage >= 0.8f)
		CompletedGrades.Add("b");

	if (ScorePercentage >= 0.7f)
		CompletedGrades.Add("c");

	if (ScorePercentage >= 0.6f)
		CompletedGrades.Add("d");

	if (ScorePercentage >= 0.5f)
		CompletedGrades.Add("e");

	CompletedGrades.Add("f");
	return CompletedGrades;
}

void AScoringManager::DisplayScore(const FText& ScoreText, int32 Score, bool bGive, float Delay, bool bIgnoreOnNoScore)
{
	if (Score == 0 && bIgnoreOnNoScore)
		return;
	
	if (Delay <= 0.0f)
	{
		DisplayScore_Internal(ScoreText, Score, bGive);
	}
	else
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &AScoringManager::DisplayScore_Internal, ScoreText, Score, bGive), Delay);
	}
}

void AScoringManager::DisplayScore_Internal(const FText ScoreText, const int32 Score, const bool bGive)
{
	// done this way so clients can show score and because const FText& cannot be used from a timer
	Multicast_DisplayScore(ScoreText, FMath::Abs(Score), bGive);
}

void AScoringManager::Multicast_DisplayScore_Implementation(const FText& ScoreText, const int32 Score, const bool bGive)
{
	bool bHUDVisible = true;
	EScoreReadoutMode ScoreReadoutMode = EScoreReadoutMode::OnlyNegative;
	UBpGameplayHelperLib::LoadScoreReadoutSetting(ScoreReadoutMode);
	UBpGameplayHelperLib::LoadShowHUDSetting(bHUDVisible);
	
	if (bHUDVisible)
	{
		if (ScoreReadoutMode == EScoreReadoutMode::AllScores || (bGive && ScoreReadoutMode == EScoreReadoutMode::OnlyPositive) || (!bGive && ScoreReadoutMode == EScoreReadoutMode::OnlyNegative))
		{
			if (const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				if (PlayerCharacter->HumanCharacterWidget_V2)
				{
					PlayerCharacter->HumanCharacterWidget_V2->AddScorePopup(ScoreText, Score, bGive);
				}
			}
		}
	}
}

bool AScoringManager::AllObjectivesComplete(const EObjectiveLevel ObjectiveLevel) const
{
	bool bAllObjectivesComplete = true;
	for (const FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		if (ScoreGroup.ObjectiveLevel == ObjectiveLevel)
		{
			for (const FScoreData& ScoreData : ScoreGroup.Scores)
			{
				if (!AllScoresGiven(ScoreData.Bonuses))
				{
					bAllObjectivesComplete = false;
					break;
				}
			}
		}
	}

	return bAllObjectivesComplete;
}

bool AScoringManager::HasCivilianBeenKilledByPlayer()
{
	if (!Get())
		return false;
	
	return Get()->bCivilianHasBeenKilledByPlayer;
}

bool AScoringManager::AllSuspectsKilledOrArrested() const
{
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI && AI->IsSuspect())
		{
			if (!AI->IsArrested() && !AI->IsDeadOrUnconscious() && !AI->IsIncapacitated())
			{
				return false;
			}
		}
	}
	
	return true;
}

bool AScoringManager::AllSuspectsReported() const
{
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI && AI->IsSuspect() && !AI->HasBeenReported())
		{
			return false;
		}
	}
	
	return true;
}

bool AScoringManager::IsEveryoneKilledOrArrested() const
{
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI && AI->IsSuspect() || AI->IsCivilian())
		{
			if (!AI->IsArrested() && !AI->IsDeadOrUnconscious() && !AI->IsIncapacitated())
			{
				return false;
			}
		}
	}
	
	return true;
}

bool AScoringManager::AreAllCiviliansRestrained() const
{
	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!IsValid(GameState))
		return false;
	
	for (ACyberneticCharacter* AI : GameState->AllAICharacters)
	{
		if (IsValid(AI) && AI->IsCivilian() && !AI->IsArrested())
		{
			return false;
		}
	}
	
	return true;
}

void AScoringManager::OnGameEnded_Implementation()
{
	if (UReadyOrNotGameInstance::bIsBuildPirated)
	{
		GiveCustomPenalty(FText::FromString("1deta1riP 1em1aG1"), 114117114);
	}
}

FScoreData* AScoringManager::GetScoreData(UScoringComponent* FromScoringComponent)
{
	if (!FromScoringComponent)
		return nullptr;

	TArray<FScoreData*> Array;

	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				return &ScoreData;
			}
		}
	}

	return nullptr;
}

const FScoreData* AScoringManager::GetScoreData_Const(const UScoringComponent* FromScoringComponent) const
{
	if (!FromScoringComponent)
		return nullptr;

	TArray<FScoreData*> Array;

	for (const FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (const FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				return &ScoreData;
			}
		}
	}

	return nullptr;
}

FScoreGroup* AScoringManager::GetScoreGroup(UScoringComponent* FromScoringComponent)
{
	if (!FromScoringComponent)
		return nullptr;

	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (const FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				return &ScoreGroup;
			}
		}
	}

	return nullptr;
}

TArray<FScoreBonus*> AScoringManager::GetBonuses(UScoringComponent* FromScoringComponent)
{
	if (!FromScoringComponent)
		return TArray<FScoreBonus*>();

	TArray<FScoreBonus*> Array;
	
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScoreBonus& Breakdown : ScoreData.Bonuses)
				{
					Array.Add(&Breakdown);
				}
			}
		}

		// TODO: include inactive param
		/*for (FScoreData& ScoreData : ScoreGroup.InactiveScores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScoreBonus& Breakdown : ScoreData.Bonuses)
				{
					Array.Add(&Breakdown);
				}
			}
		}*/
	}

	return Array;
}

TArray<FScorePenalty*> AScoringManager::GetPenalties(UScoringComponent* FromScoringComponent)
{
	if (!FromScoringComponent)
		return TArray<FScorePenalty*>();

	TArray<FScorePenalty*> Array;
	
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScorePenalty& Breakdown : ScoreData.Penalties)
				{
					Array.Add(&Breakdown);
				}
			}
		}
		
		for (FScoreData& ScoreData : ScoreGroup.InactiveScores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScorePenalty& Breakdown : ScoreData.Penalties)
				{
					Array.Add(&Breakdown);
				}
			}
		}
	}

	return Array;
}

FScoreBonus* AScoringManager::GetBonus(UScoringComponent* FromScoringComponent, const FText& BonusName)
{
	if (!FromScoringComponent)
		return nullptr;

	TArray<FScorePenalty*> Array;
	
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScoreBonus& Bonus : ScoreData.Bonuses)
				{
					if (Bonus.ScoreName.EqualTo(BonusName))
						return &Bonus;
				}
			}
		}
	}

	return nullptr;
}

FScorePenalty* AScoringManager::GetPenalty(UScoringComponent* FromScoringComponent, const FText& PenaltyName)
{
	if (!FromScoringComponent)
		return nullptr;

	TArray<FScorePenalty*> Array;
	
	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent == FromScoringComponent)
			{
				for (FScorePenalty& Breakdown : ScoreData.Penalties)
				{
					if (Breakdown.ScoreName.EqualTo(PenaltyName))
						return &Breakdown;
				}
			}
		}
	}

	return nullptr;
}

int32 AScoringManager::CalculateTotalPlayerPenaltyScore() const
{
	int32 RunningTotal = 0;
	for (const FScorePenaltyData& PenaltyData : PenaltyScoreGroups)
	{
		RunningTotal += PenaltyData.Score;
	}
	
	//TotalPlayerPenaltyScore = RunningTotal;
	return RunningTotal;
}

// TODO: switch objective level
int32 AScoringManager::CalculateTotalPlayerScore() const
{
	if (UReadyOrNotGameInstance::bIsBuildPirated)
	{
		//TotalPlayerScore = 0;
		return 0;
	}
	
	const int32 TotalPenaltyScore = CalculateTotalPlayerPenaltyScore();

	int32 RunningTotal = 0;
	//int32 RunningPrimaryTotal = 0;
	//int32 RunningSecondaryTotal = 0;
	//int32 RunningTertiaryTotal = 0;
	for (const FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		RunningTotal += GetTotalGivenScoresFromArray(ScoreGroup.Scores);
		
		/*switch (ScoreGroup.ObjectiveLevel)
		{
			case EObjectiveLevel::PrimaryObjective:		RunningPrimaryTotal += GetTotalGivenScoresFromArray(ScoreGroup.Scores); break;
			case EObjectiveLevel::SecondaryObjective:	RunningSecondaryTotal = GetTotalGivenScoresFromArray(ScoreGroup.Scores); break;
			case EObjectiveLevel::TertiaryObjective:	RunningTertiaryTotal = GetTotalGivenScoresFromArray(ScoreGroup.Scores); break;
			default: break;
		}*/
	}

	/*
	TotalPlayerPrimaryScore = RunningPrimaryTotal;
	TotalPlayerSecondaryScore = RunningSecondaryTotal;
	TotalPlayerTertiaryScore = RunningTertiaryTotal;
	TotalPlayerScore = RunningTotal - TotalPlayerPenaltyScore;
	*/

	//CacheHasClearedMission();

	return RunningTotal - TotalPenaltyScore;
}

void AScoringManager::GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData)
{
	#if !UE_BUILD_SHIPPING
	OutDebugData.Empty(50);

	OutDebugData.Add({"Score Pool", FText::AsNumber(TotalScorePool)});
	OutDebugData.Add({"Total Player Score", FText::AsNumber(CalculateTotalPlayerScore())});
	OutDebugData.Add({"Total Player Penalty Score", FText::AsNumber(CalculateTotalPlayerPenaltyScore())});
	OutDebugData.Add({"Total Score Actors", FText::AsNumber(GetTotalActorsTrackingScore())});
	OutDebugData.Add({"Current Grade", FText::FromString(CalculateGradeLetterFromPlayerScore())});
	OutDebugData.Add({"", FText::FromString("")});

	for (FScoreGroup& ScoreGroup : ObjectiveScoreGroups)
	{
		for (FScoreData& ScoreData : ScoreGroup.Scores)
		{
			if (ScoreData.FromScoringComponent && ScoreData.FromScoringComponent->GetOwner())
				OutDebugData.Add({ScoreData.FromScoringComponent->GetOwner()->GetName(), FText::FromString(ScoreData.HasGivenAllRequiredScores() ? "True" : "False")});
		}
	}
	#endif
}

void AScoringManager::LogScores()
{
	#if WITH_EDITOR
	if (CVarScoringSystemDebug.GetValueOnAnyThread() > 0)
	{
		ULog::LineBreak_Symbol("============");
		//ULog::Number(TotalPlayerPrimaryScore, "Total Player Primary Score: ");
		//ULog::Number(TotalPlayerSecondaryScore, "Total Player Secondary Score: ");
		//ULog::Number(TotalPlayerTertiaryScore, "Total Player Tertiary Score: ");
		ULog::Number(CalculateTotalPlayerScore(), "Total Player Score: ");
		ULog::Number(GetNonGivenScoreCountFromGroup(ObjectiveScoreGroups), "Total Scores Left: ");
		ULog::Number(GetGivenScoreCountFromGroup(ObjectiveScoreGroups), "Total Scores Given: ");
		ULog::Info("Score Grade: " + CalculateGradeLetterFromPlayerScore());
		ULog::LineBreak_Symbol("============");
	}
	#endif
}
