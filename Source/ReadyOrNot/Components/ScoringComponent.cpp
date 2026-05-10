// Void Interactive, 2020

#include "ScoringComponent.h"

#include "Info/ScoringManager.h"

UScoringComponent::UScoringComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 1.0f;

	bEnabled = true;
	bAutoAddToScorePool = true;

	

	static ConstructorHelpers::FObjectFinder<UDataTable> DataTable(TEXT("DataTable'/Game/Blueprints/DataTables/ScoringGroupDataTable.ScoringGroupDataTable'"));
	ScoreGroupDataTable = DataTable.Object;
	ScoreGroup.DataTable = ScoreGroupDataTable;
}

void UScoringComponent::ApplyScoreTableValues()
{
	if (ScoreGroupDataTable)
	{
		if (FScoringDataTable* ScoringDataTable = ScoreGroup.GetRow<FScoringDataTable>("Apply Score Data"))
		{
			ScoreGroupName = ScoreGroup.RowName;
			ObjectiveLevel = ScoringDataTable->ObjectiveLevel;
			//ScoringData.Scores = ScoringDataTable->ScoreBreakdown;
			ScoringData.Bonuses = ScoringDataTable->Bonuses;
			ScoringData.Penalties = ScoringDataTable->Penalties;
		}
	}
}

void UScoringComponent::BeginPlay()
{
	Super::BeginPlay();

	//ScoreGroupName should not have any spaces in it, as it's used for rownames, and is the key for scoringtable text lookup
	//TODO (Max) Update soft objectives scoregroup row and resave
	FString ScoreGroupString = ScoreGroupName.ToString();
	ScoreGroupString.RemoveSpacesInline();
	ScoreGroupName = *ScoreGroupString;
	ScoreGroup.RowName = ScoreGroupName;

	if (bAutoAddToScorePool)
	{
		AddToScorePool();
	}
}

void UScoringComponent::PostLoad()
{
	Super::PostLoad();

	//ScoreGroupName should not have any spaces in it, as it's used for rownames, and is the key for scoringtable text lookup
	FString ScoreGroupString = ScoreGroupName.ToString();
	ScoreGroupString.RemoveSpacesInline();
	ScoreGroupName = *ScoreGroupString;
	ScoreGroup.RowName = ScoreGroupName;
}

#if WITH_EDITOR
void UScoringComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "DataTable")
	{
		ScoreGroupDataTable = ScoreGroup.DataTable;

		ApplyScoreTableValues();
	}
	else if (PropertyChangedEvent.GetPropertyName() == "RowName")
	{
		FScoringDataTable* Row = ScoreGroup.DataTable->FindRow<FScoringDataTable>(ScoreGroup.RowName, "Score Group Lookup");
		ScoreGroupName = ScoreGroup.RowName;
		ObjectiveLevel = Row->ObjectiveLevel;
	}
}
#endif

void UScoringComponent::AddToScorePool()
{
	AddToScorePool(ScoreGroupName);
}

void UScoringComponent::AddToScorePool(const FName& GroupName, const bool bForce)
{
	if (!bEnabled)
		return;
	
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	ScoreGroup.RowName = GroupName;
	
	ScoringData.FromScoringComponent = this;

	// Apply data table values, if a data table is used
	ApplyScoreTableValues();

	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->AddScoreToPool(this, GroupName, bForce);
}

void UScoringComponent::RemoveFromScorePool()
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->RemoveScoreFromPool(this);
}

/*void UScoringComponent::GiveFullPenalty(const FString& PenaltyName, const bool bDisplayPenaltyOnHUD, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveFullPenalty(this, PenaltyName, bDisplayPenaltyOnHUD, DisplayOnHUDDelay, CustomScoreOnHUD);
}*/

void UScoringComponent::GivePenalty(const FText& PenaltyName, const bool bDisplayPenaltyOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GivePenalty(this, PenaltyName, bDisplayPenaltyOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
}

void UScoringComponent::GiveCustomPenalty(const FText& PenaltyGroupName, const int32 PenaltyScore, const bool bDisplayScoreOnHUD, const float DisplayOnHUDDelay)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveCustomPenalty(PenaltyGroupName, PenaltyScore, bDisplayScoreOnHUD, DisplayOnHUDDelay);
}

void UScoringComponent::RevokePenalty(const FText& PenaltyName)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->RevokePenalty(this, PenaltyName);
}

void UScoringComponent::RevokeAllPenalties()
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->RevokeAllPenalties(this);
}

void UScoringComponent::GiveScore(const FText& ScoreName, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveScore(this, ScoreName, bDisplayScoreOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
}

void UScoringComponent::GiveScores(const TArray<FText>& ScoreNames, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveScores(this, ScoreNames, bDisplayScoreOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
}

void UScoringComponent::GiveAllScores(const bool bOnlyEnabledScore, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveAllScores(this, bOnlyEnabledScore, bDisplayScoreOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
}

void UScoringComponent::GiveFakeScore(const FText& ScoreName, const bool bDisplayScoreOnHUD, const FText& ScoreText, const float DisplayOnHUDDelay, const int32 CustomScoreOnHUD)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->GiveFakeScore(this, ScoreName, bDisplayScoreOnHUD, ScoreText, DisplayOnHUDDelay, CustomScoreOnHUD);
}

void UScoringComponent::TakeScore(const FText& ScoreName, const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScore)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->TakeScore(this, ScoreName, TakeReason, bDisplayScoreOnHUD, bDisableScore);
}

void UScoringComponent::TakeScores(const TArray<FText>& ScoreNames, const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScores)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->TakeScores(this, ScoreNames, TakeReason, bDisplayScoreOnHUD, bDisableScores);
}

void UScoringComponent::TakeAllScores(const FText& TakeReason, const bool bDisplayScoreOnHUD, const bool bDisableScores)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->TakeAllScores(this, TakeReason, bDisplayScoreOnHUD, bDisableScores);	// Only server allowed
}

void UScoringComponent::TakeAllScoresExcept(const TArray<FText>& ScoreNames, const FText& TakeReason, bool bDisplayScoreOnHUD, bool bDisableScores)
{
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->TakeAllScoresExcept(this, ScoreNames, TakeReason, bDisplayScoreOnHUD, bDisableScores);
}

void UScoringComponent::ChangeGroup(const FName& NewGroupName)
{
	if (ScoreGroupName == NewGroupName)
		return;
	
	ScoreGroupName = NewGroupName;

	FString GroupNameNoSpaces = NewGroupName.ToString();
	GroupNameNoSpaces.RemoveSpacesInline();
	
	ScoreGroup.RowName = NewGroupName;
	
	ApplyScoreTableValues();
}

void UScoringComponent::ChangeScoreGroup(const FName& NewGroupName)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;

	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->ChangeScoreGroup(this, NewGroupName);
}

void UScoringComponent::DisplayBonusesAndPenalties(const bool bCondensed, const FText& ScoreTextOverride)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->DisplayBonusesAndPenalties(this, bCondensed, ScoreTextOverride);
}

void UScoringComponent::DisplayBonuses(const bool bCondensed, const FText& ScoreTextOverride)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->DisplayBonuses(this, bCondensed, ScoreTextOverride);
}

void UScoringComponent::DisplayPenalties(const bool bCondensed, const FText& ScoreTextOverride)
{
	// Only server allowed
	if (GetOwnerRole() < ROLE_Authority)
		return;
	
	if (AScoringManager* ScoringManager = AScoringManager::Get())
		ScoringManager->DisplayPenalties(this, bCondensed, ScoreTextOverride);
}

int32 UScoringComponent::GetTotalScore(const bool bOnlyEnabled, const bool bOnlyGiven) const
{
	return ScoringData.GetTotalScore(bOnlyEnabled, bOnlyGiven);
}

int32 UScoringComponent::GetScore(const FText& ScoreName) const
{
	return ScoringData.GetScore(ScoreName);
}
