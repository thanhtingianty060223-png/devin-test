// Void Interactive, 2020

#pragma once

#include "Components/ActorComponent.h"
#include "ScoringComponent.generated.h"

UENUM(BlueprintType)
enum class EObjectiveLevel : uint8
{
	PrimaryObjective,
	SecondaryObjective,
	TertiaryObjective
};

USTRUCT(BlueprintType)
struct FScoreBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (EditCondition = "bEnabled"))
	FText ScoreName = FText::FromString("None");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (EditCondition = "bEnabled"))
	int32 Score = 0;

	// Is this score required to be considered "given"?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (EditCondition = "bEnabled"))
	uint8 bRequired : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	uint8 bGiven : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	uint8 bWasDisplayedOnHUD : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	FText ScoreTextOnHUD;
};

USTRUCT(BlueprintType)
struct FScorePenalty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (EditCondition = "bEnabled"))
	FText ScoreName = FText::FromString("None");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (EditCondition = "bEnabled"))
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	uint8 bGiven : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	uint8 bWasDisplayedOnHUD : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	FText ScoreTextOnHUD;

	bool IsValid() const
	{
		return !ScoreName.EqualTo(FText::FromString("None"));
	}
};

USTRUCT(BlueprintType)
struct FScoreData
{
	GENERATED_BODY()

	FScoreData()
	{
		Bonuses = {};
		Penalties = {};
		//Scores = {};
		FromScoringComponent = nullptr;
		bHiddenScore = false;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (TitleProperty = "ScoreName"))
	TArray<FScoreBonus> Bonuses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score", meta = (TitleProperty = "ScoreName"))
	TArray<FScorePenalty> Penalties;

	// Deprecated
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	//TArray<FScoreBreakdown> Scores;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	uint8 bHiddenScore : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	class UScoringComponent* FromScoringComponent = nullptr;

	bool AnyPenaltiesGiven() const
	{
		for (const FScorePenalty& ScorePenalty : Penalties)
		{
			if (ScorePenalty.bEnabled && ScorePenalty.bGiven)
			{
				return true;
			}
		}

		return false;
	}

	bool HasGivenAllScores() const
	{
		bool bAllGiven = true;
		for (const FScoreBonus& ScoreBreakdown : Bonuses)
		{
			if (ScoreBreakdown.bEnabled && !ScoreBreakdown.bGiven /*&& !ScoreBreakdown.bPenaltyOnly*/)
			{
				bAllGiven = false;
			}
		}

		return bAllGiven;
	}
	
	bool HasGivenAllRequiredScores() const
	{
		bool bAllGiven = true;
		for (const FScoreBonus& ScoreBreakdown : Bonuses)
		{
			if (ScoreBreakdown.bEnabled && ScoreBreakdown.bRequired && !ScoreBreakdown.bGiven/* && !ScoreBreakdown.bPenaltyOnly*/)
			{
				bAllGiven = false;
			}
		}

		return bAllGiven;
	}

	int32 GetTotalScore(const bool bOnlyEnabled = false, const bool bOnlyGiven = false, const bool bIncludePenalties = false) const
	{
		int32 RunningTotal = 0;

		// TODO: refactor

		for (const FScoreBonus& ScoreBreakdown : Bonuses)
		{
			if (bOnlyEnabled)
			{
				if (ScoreBreakdown.bEnabled)
				{
					if (bOnlyGiven)
					{
						if (ScoreBreakdown.bGiven)
							RunningTotal += ScoreBreakdown.Score;
					}
					else
					{
						RunningTotal += ScoreBreakdown.Score;
					}
				}
			}
			else
			{
				if (bOnlyGiven)
				{
					if (ScoreBreakdown.bGiven)
						RunningTotal += ScoreBreakdown.Score;
				}
				else
				{
					RunningTotal += ScoreBreakdown.Score;
				}
			}
		}

		if (bIncludePenalties)
		{
			for (const FScorePenalty& ScoreBreakdown : Penalties)
			{
				if (bOnlyEnabled)
				{
					if (ScoreBreakdown.bEnabled)
					{
						if (bOnlyGiven)
						{
							if (ScoreBreakdown.bGiven)
								RunningTotal += ScoreBreakdown.Score;
						}
						else
						{
							RunningTotal += ScoreBreakdown.Score;
						}
					}
				}
				else
				{
					if (bOnlyGiven)
					{
						if (ScoreBreakdown.bGiven)
							RunningTotal += ScoreBreakdown.Score;
					}
					else
					{
						RunningTotal += ScoreBreakdown.Score;
					}
				}
			}
		}

		/*
		for (const FScoreBreakdown& ScoreBreakdown : Scores)
		{
			//if (ScoreBreakdown.bPenaltyOnly && !bIncludePenaltyOnly)
			//	continue;

			//V_LOGM(LogReadyOrNot, "Adding Score To Pool %s %d", *ScoreBreakdown.ScoreName, ScoreBreakdown.Score);
			
			if (bOnlyEnabled)
			{
				if (ScoreBreakdown.bEnabled)
				{
					if (bOnlyGiven)
					{
						if (ScoreBreakdown.bGiven)
							RunningTotal += ScoreBreakdown.Score;
					}
					else
					{
						RunningTotal += ScoreBreakdown.Score;
					}
				}
			}
			else
			{
				if (bOnlyGiven)
				{
					if (ScoreBreakdown.bGiven)
						RunningTotal += ScoreBreakdown.Score;
				}
				else
				{
					RunningTotal += ScoreBreakdown.Score;
				}
			}
		}
		*/

		return RunningTotal;
	}

	int32 GetScore(const FText& ScoreName, const bool bOnlyEnabled = false) const
	{
		for (const FScoreBonus& ScoreBreakdown : Bonuses)
		{
			if (bOnlyEnabled)
			{
				if (ScoreBreakdown.bEnabled && ScoreBreakdown.ScoreName.EqualTo(ScoreName))
					return ScoreBreakdown.Score;
			}
			else
			{
				if (ScoreBreakdown.ScoreName.EqualTo(ScoreName))
					return ScoreBreakdown.Score;
			}
		}

		return bOnlyEnabled ? 0 : -1;
	}

	friend bool operator==(const FScoreData& LHS, const FScoreData& RHS)
	{
		return LHS.FromScoringComponent == RHS.FromScoringComponent;
	}
};

USTRUCT(BlueprintType)
struct FScorePenaltyData
{
	GENERATED_BODY()

	FScorePenaltyData() {}

	FScorePenaltyData(const FText& InGroupName)
	{
		GroupName = InGroupName;
	}

	UPROPERTY(BlueprintReadWrite, Category = "Score")
	FText GroupName = FText::FromString("None");

	UPROPERTY(BlueprintReadWrite, Category = "Score")
	int32 Score = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Score")
	int32 PenaltyCount = 0;
	
	//UPROPERTY(BlueprintReadWrite, Category = "Score")
	//TArray<UScoringComponent*> GivenFrom;

	friend bool operator==(const FScorePenaltyData& LHS, const FScorePenaltyData& RHS)
	{
		return LHS.GroupName.EqualTo(RHS.GroupName);
	}
};

USTRUCT(BlueprintType)
struct FScoreGroup
{
	GENERATED_BODY()

	FScoreGroup()
	{
		bRequiredToClearMission = true;
		bRequiredToSoftClearMission = false;
	}

	FScoreGroup(const FName& InGroupName)
	{
		GroupName = InGroupName;
		//GroupName = FText::FromStringTable("ScoringTable", GroupRowName.ToString());
		bRequiredToClearMission = true;
		bRequiredToSoftClearMission = false;
	}

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	// FText GroupName = FText::FromString("None");

	// This is unlocalised FName of the row from ScoringGroupDataTable
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	FName GroupName = "MissionObjectives";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	EObjectiveLevel ObjectiveLevel = EObjectiveLevel::SecondaryObjective;

	UPROPERTY(BlueprintReadWrite, Category = "Score")
	uint8 bRequiredToClearMission : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Score")
	uint8 bRequiredToSoftClearMission : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 OrderPriority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	TArray<FScoreData> Scores;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	TArray<FScoreData> InactiveScores;

	friend bool operator==(const FScoreGroup& LHS, const FScoreGroup& RHS)
	{
		return LHS.GroupName == RHS.GroupName;
	}
};

USTRUCT(BlueprintType)
struct FScoringDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")
	FText Name = FText::FromString("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")
	EObjectiveLevel ObjectiveLevel = EObjectiveLevel::SecondaryObjective;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (TitleProperty = "ScoreName"))
	TArray<FScoreBonus> Bonuses;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (TitleProperty = "ScoreName"))
	TArray<FScorePenalty> Penalties;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")
	uint8 bRequiredToClearMission : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")
	uint8 bRequiredToSoftClearMission : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring")
	int32 OrderPriority = 0;
};

UCLASS(ClassGroup=(Scoring), meta=(BlueprintSpawnableComponent), HideCategories = ("Sockets", "Tags", "Activation", "Cooking", "Asset User Data", "Collision"))
class READYORNOT_API UScoringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UScoringComponent();

	void AddToScorePool();
	void AddToScorePool(const FName& GroupName, bool bForce = false);
	void RemoveFromScorePool();

	void GivePenalty(const FText& PenaltyName, bool bDisplayPenaltyOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	void GiveCustomPenalty(const FText& PenaltyGroupName, const int32 PenaltyScore, const bool bDisplayScoreOnHUD, const float DisplayOnHUDDelay);
	void RevokePenalty(const FText& PenaltyName);
	void RevokeAllPenalties();
	
	void GiveScore(const FText& ScoreName, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	void GiveScores(const TArray<FText>& ScoreNames, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	void GiveAllScores(bool bOnlyEnabledScore = false, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	void GiveFakeScore(const FText& ScoreName, bool bDisplayScoreOnHUD = false, const FText& ScoreText = FText::GetEmpty(), float DisplayOnHUDDelay = 0.0f, int32 CustomScoreOnHUD = -1);
	
	void TakeScore(const FText& ScoreName, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScore = true);
	void TakeScores(const TArray<FText>& ScoreNames, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);
	void TakeAllScores(const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);
	void TakeAllScoresExcept(const TArray<FText>& ScoreNames, const FText& TakeReason = FText::GetEmpty(), bool bDisplayScoreOnHUD = false, bool bDisableScores = true);

	void ChangeGroup(const FName& NewGroupName);
	void ChangeScoreGroup(const FName& NewGroupName);

	void DisplayBonusesAndPenalties(bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	void DisplayBonuses(bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	void DisplayPenalties(bool bCondensed = false, const FText& ScoreTextOverride = FText::GetEmpty());
	
	//FScoreBonus* GetScoreBonus(const FString& ScoreName);
	
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetTotalScore(bool bOnlyEnabled = false, const bool bOnlyGiven = false) const;
	
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore(const FText& ScoreName) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled"))
	FScoreData ScoringData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled"))
	FDataTableRowHandle ScoreGroup;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled && ScoreGroupDataTable == nullptr"))
	FName ScoreGroupName = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled && ScoreGroupDataTable == nullptr"))
	EObjectiveLevel ObjectiveLevel = EObjectiveLevel::SecondaryObjective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled"))
	uint8 bAutoAddToScorePool : 1;

	/** If enabled, will still show score popup when TakeAllScores is called, even if no scores are actually taken back */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score", meta = (EditCondition = "bEnabled"))
	bool bShowPopupOnTakeAllScoresWithNoChange = false;
	// Note for above, only done for TakeAllScores to make sure objective fails are shown, wanted to make minimal changes to scoring system


	FScorePenalty LastGivenPenalty;
	
protected:
	virtual void BeginPlay() override;
	virtual void PostLoad() override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

private:
	void ApplyScoreTableValues();
	
	UPROPERTY()
	const UDataTable* ScoreGroupDataTable = nullptr;
};
