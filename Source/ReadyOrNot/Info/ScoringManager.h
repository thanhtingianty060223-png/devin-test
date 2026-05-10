// Void Interactive, 2020

#pragma once

#include "GameFramework/Info.h"
#include "ScoringManager.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AScoringManager : public AInfo, public IListenForDeath, public IListenForGameEnd, public IGatherDebugInterface
{
	GENERATED_BODY()

	AScoringManager();

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	int32 TotalScorePool = 0;
	int32 TotalPrimaryScorePool = 0;
	int32 TotalSecondaryScorePool = 0;
	int32 TotalTertiaryScorePool = 0;
	
	UPROPERTY(Replicated)
	TArray<FScoreGroup> ObjectiveScoreGroups;

	UPROPERTY(Replicated)
	TArray<FScorePenaltyData> PenaltyScoreGroups;
	
	bool bCivilianHasBeenKilledByPlayer = false;
	
	virtual void OnCharacterDied_Implementation(AReadyOrNotCharacter* Victim, AReadyOrNotCharacter* Killer, AActor* Inflictor) override;;

public:
	void AddScoreToPool(UScoringComponent* InScoringComponent, const FName& GroupName, bool bForce = false);
	void RemoveScoreFromPool(UScoringComponent* InScoringComponent);

	bool ContainsScoreComponent(UScoringComponent* InScoringComponent) const;

	void UpdateScorePool();
	
	void RemoveScore(UScoringComponent* InScoringComponent);
	
	void AddOrChangeToScoreGroup(UScoringComponent* InScoringComponent, const FName& NewGroupName);

	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void DisplayBonusesAndPenalties(UScoringComponent* InScoringComponent, bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void DisplayBonuses(UScoringComponent* InScoringComponent, bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void DisplayPenalties(UScoringComponent* InScoringComponent, bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GiveScore(UScoringComponent* InScoringComponent, const FText& ScoreName, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GiveFakeScore(UScoringComponent* InScoringComponent, const FText& ScoreName, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);

	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GiveScores(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GiveAllScores(UScoringComponent* InScoringComponent, bool bOnlyEnabledScore = false, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void TakeScore(UScoringComponent* InScoringComponent, const FText& ScoreName, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScore = true);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void TakeScores(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void TakeAllScores(UScoringComponent* InScoringComponent, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void TakeAllScoresExcept(UScoringComponent* InScoringComponent, const TArray<FText>& ScoreNames, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GivePenalty(UScoringComponent* InScoringComponent, const FText& PenaltyGroupName, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void RevokePenalty(UScoringComponent* InScoringComponent, const FText& PenaltyGroupName);
	bool RevokePenalty(FScorePenalty* InPenalty);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void RevokeAllPenalties(UScoringComponent* InScoringComponent);

	// Do not call every frame, as this does not have a unique identifer to test against if said penalty was already given
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void GiveCustomPenalty(const FText& PenaltyGroupName, int32 PenaltyScore, bool bDisplayScoreOnHUD = false, float DisplayOnHUDDelay = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Scoring Manager")
	void ChangeScoreGroup(UScoringComponent* InScoringComponent, FName NewGroupName);
	
	void UpdateObjectives();

	//void RemoveInvalidObjects();
	
	void LogScores();
	
	UFUNCTION(BlueprintPure)
	static class AScoringManager* Get();

	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	TArray<FScoreGroup> GetScoreGroups() const;
	
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
    TArray<FScorePenaltyData> GetPenaltyScoreGroups() const;

	// Gets the amount of scores given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetGivenScoreCountFromGroup(const TArray<FScoreGroup>& InScoreGroupArray) const;

	// Gets the amount of scores not given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetNonGivenScoreCountFromGroup(const TArray<FScoreGroup>& InScoreGroupArray) const;

	// Gets the amount of scores given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetGivenScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, bool bIncludeHiddenScores = false) const;

	// Gets the total amount of scores of a score group. Excludes hidden scores
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetTotalScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, bool bIncludeHiddenScores = false) const;
	
	// Gets the amount of scores points given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetTotalGivenScoresFromArray(const TArray<FScoreData>& InScoreDataArray, bool bIncludeHiddenScores = false) const;

	// Gets the amount of scores not given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
    int32 GetNonGivenScoreCountFromArray(const TArray<FScoreData>& InScoreDataArray, bool bOnlyEnabled = false) const;

	// Gets the amount of scores points not given to the player
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
    int32 GetTotalNonGivenScoresFromArray(const TArray<FScoreData>& InScoreDataArray) const;

	// Gets the total amount of scores points from the array
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetTotalScoreFromArray(const TArray<FScoreData>& InScoreDataArray) const;
	
	int32 GetTotalScoreFromComponent(const UScoringComponent* InScoringComponent, bool bOnlyEnabled = false, const bool bOnlyGiven = false) const;
	int32 GetScoreFromComponent(const UScoringComponent* InScoringComponent, const FText& InScoreName) const;

	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	bool AllScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const;
	
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	bool AllRequiredScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const;
	
	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	bool AnyScoresGiven(const TArray<FScoreBonus>& InScoreBonusArray) const;

	UFUNCTION(BlueprintPure, Category = "Scoring Manager")
	int32 GetTotalActorsTrackingScore() const;

	UFUNCTION(BlueprintPure)
	static void GetSuspectCount(int32& OutReported, int32& OutArrested, int32& OutKilled, int32& OutTotal);

	UFUNCTION(BlueprintPure)
	static void GetCivilianCount(int32& OutReported, int32& OutInjured, int32& OutKilled, int32& OutArrested, int32& OutTotal);

	UFUNCTION(BlueprintPure)
	static void GetEvidenceCount(int32& EvidenceCollected, int32& TotalEvidence);
	
	UFUNCTION(BlueprintPure)
	static void GetScoreCountFromGroup(const FName& InGroupName, int32& OutScoresGiven, int32& OutTotalScores, bool bRequiredOnly = false);

	UFUNCTION(BlueprintPure)
	static void GetReportCount(int32& ReportedCount, int32& TotalReports);
	
	/*
	 * Getter for the mission state and if all suspects/civilains arrested or killed
	 * bHasClearedMission is if the mission has been fully cleared, all objectives, arrests, kills, evidence, reports have been done
	 * bSoftClearedMission is if all the suspect and civilians have been arrested or killed but not everything has been reported or picked up
	 */
	UFUNCTION(BLueprintPure)
	static void HasClearedMission(bool& bHasClearedMission, bool& bSoftClearedMission, bool& bMissionFailed);

	bool bCachedHasClearedMission = false;
	bool bCachedSoftClearedMission = false;
	bool bCachedMissionFailed = false;
	
	UFUNCTION(BLueprintPure)
	static bool IsOfficialScoring();

	UPROPERTY(Replicated)
	bool bIsOfficialScoring = true;
	
	void CacheHasClearedMission();

	UFUNCTION(BlueprintPure)
	static void GetObjectiveCompletionStatus(int32& ObjectivesComplete, int32& ObjectivesFailed, int32& TotalObjectives);

	UFUNCTION(BlueprintPure)
	void GetTotalScorePool(int32& MaxScore);

	UFUNCTION(BlueprintPure)
	void GetTotalScore(int32& TotalScore, bool bIncludePrimaryScores = true);

	UFUNCTION(BlueprintPure)
	FString CalculateGradeLetterFromScore(int32 Score) const;

	// Score percentage is a value from 0.0 to 1.0
	UFUNCTION(BlueprintPure)
	static FString CalculateGradeLetterFromPercentage(float ScorePercentage);
	
	
	UFUNCTION(BlueprintPure)
	float GetCurrentScoreAsPercentage() const;
	
	UFUNCTION(BlueprintPure)
	FString CalculateGradeLetterFromPlayerScore();

	float GetFinalGradePercentage();
	TSet<FString> GetCompletedGradesUpToScore(float ScorePercentage);
	
	bool AllObjectivesComplete(EObjectiveLevel ObjectiveLevel) const;
	
	static bool HasCivilianBeenKilledByPlayer();

	bool AllSuspectsKilledOrArrested() const;
	bool IsEveryoneKilledOrArrested() const;
	bool AllSuspectsReported() const;
	bool AreAllCiviliansRestrained() const;

	virtual void OnGameEnded_Implementation() override;

	FScoreGroup* GetScoreGroup(UScoringComponent* FromScoringComponent);
	FScoreData* GetScoreData(UScoringComponent* FromScoringComponent);
	const FScoreData* GetScoreData_Const(const UScoringComponent* FromScoringComponent) const;

	TArray<FScoreBonus*> GetBonuses(UScoringComponent* FromScoringComponent);
	TArray<FScorePenalty*> GetPenalties(UScoringComponent* FromScoringComponent);
	
	FScoreBonus* GetBonus(UScoringComponent* FromScoringComponent, const FText& BonusName);
	FScorePenalty* GetPenalty(UScoringComponent* FromScoringComponent, const FText& PenaltyName);

	int32 CalculateTotalPlayerPenaltyScore() const;
	int32 CalculateTotalPlayerScore() const;
	
	void DisplayScore(const FText& ScoreText, int32 Score, bool bGive, float Delay = 0.0f, bool bIgnoreOnNoScore = true);
	
	virtual void GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData) override;

	// Score group names
	// These are FNames as they are not actually needed for localization, they just represent the various available score groups
	static FName SCORE_EVIDENCE_SECURED;
	static FName SCORE_REPORT_SUSPECT;
	static FName SCORE_REPORT_CIVILIAN;
	static FName SCORE_REPORT_DOWNED_OFFICER;
	static FName SCORE_REPORT_INCAPACITATED_BODY;
	static FName SCORE_REPORT_DEAD_BODY;
	static FName SCORE_NO_OFFICERS_DEAD;
	static FName SCORE_NO_OFFICERS_INJURED;
	static FName SCORE_TRAP_DISARMED;
	static FName SCORE_EXFILTRATED_MISSION;

	// Bonus score names
	static FText BONUS_EVIDENCE_SECURED;
	static FText BONUS_SUSPECT_REPORTED;
	static FText BONUS_CIVILIAN_REPORTED;
	static FText BONUS_DOWNED_OFFICER_REPORTED;
	static FText BONUS_INCAPACITATED_BODY_REPORTED;
	static FText BONUS_INCAPACITATED_BODIES_REPORTED;
	static FText BONUS_REPORT_DEAD_BODY_REPORTED;
	static FText BONUS_REPORT_DEAD_BODIES_REPORTED;
	static FText BONUS_NO_OFFICER_INJURED;
	static FText BONUS_TRAP_DISARMED;
	
	// Penalty score names
	static FText PENALTY_UNAUTHORIZED_FORCE;
	static FText PENALTY_UNAUTHORIZED_DEADLY_FORCE;
	static FText PENALTY_FRIENDLY_FIRE;
	static FText PENALTY_FRIENDLY_TEAM_KILL;
	static FText PENALTY_FAILED_TO_REPORT_DOWNED_OFFICER;
	static FText PENALTY_KILLED_INCAP_BODY;
	static FText PENALTY_CIVILIAN_KILLED;
	static FText PENALTY_TRAP_TRIGGERED;
	static FText PENALTY_EXFILTRATED_MISSION;

	void SetupScoringText();


private:
	void DisplayScore_Internal(FText ScoreText, int32 Score, bool bGive);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DisplayScore(const FText& ScoreText, int32 Score, bool bGive);
	void Multicast_DisplayScore_Implementation(const FText& ScoreText, int32 Score, bool bGive);
};
