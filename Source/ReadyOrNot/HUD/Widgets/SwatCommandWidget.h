// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "Structs.h"
#include "SwatCommandWidget.generated.h"

DECLARE_STATS_GROUP(TEXT("Swat Command Widget"), STATGROUP_SwatCommandWidget, STATCAT_Advanced);

UCLASS()
class READYORNOT_API USwatCommandWidget final : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	void BuildDefaultPageData(TArray<FSwatCommand>& Commands);
	void BuildQueuedPageData(TArray<FSwatCommand>& Commands);
	void BuildDoorPageData(ADoor* Door, TArray<FSwatCommand>& Commands);
	void BuildTargetPageData(AActor* Target, TArray<FSwatCommand>& Commands, FString PageTitle = "");
	void BuildTrainingPageData(TArray<FSwatCommand>& Commands);

	void GetExecutingBreachAndClearActivities(TArray<class UTeamBreachAndClearActivity*>& OutActivities);

	bool IsOtherTeamStackedUpOnDoor(ADoor* Door, ETeamType& OutTeam);
	
	bool IsTeamStackedUpOnDoor(ADoor* Door);
	bool IsTeamBreachingDoor(ADoor* Door, ETeamType SwatTeam);
	bool IsTeamStackedUpOnDoorWithStyle(ETeamType SwatTeam, ADoor* Door, EStackUpStyle StackUpStyle, bool bPlayerInFront) const;

	bool RequiresPageViewUpdate();
	TArray<FSwatCommand> LastPageUpdateCommandList;
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Page View Update")
	void OnPageViewUpdateBP();
	
	void OnPageViewUpdate();

	FLinearColor GetTeamColor() const;
	
	void UpdateDirectory();

	void SetLastCommandPage(TArray<FSwatCommand>& InCommands);

	UPROPERTY(BlueprintReadOnly)
	FString DirectoryStringOverride = "";
	
	UPROPERTY(BlueprintReadWrite)
	FString DirectoryString = "";

	UPROPERTY()
	AActor* LastActorBeforeGoingIntoSubPage = nullptr;

	bool bHoldPageUntilExecuted = false;
	
	UPROPERTY(BlueprintReadOnly)
	int32 LastSubCommandPageIndex = 0;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FSwatCommand> ParentCommands;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FSwatCommand> ActiveCommandPage;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FSwatCommand> PreviousActiveCommandPage;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FKey> CommandCombo;

	UPROPERTY(BlueprintReadOnly)
	ETeamType ActiveTeamType = ETeamType::TT_SQUAD;
	UPROPERTY(BlueprintReadOnly)
	ETeamType PreviousActiveTeamType = ETeamType::TT_SQUAD;
	UPROPERTY(BlueprintReadOnly)
	ETeamType OverrideActiveTeamType = ETeamType::TT_SQUAD;
	UPROPERTY(BlueprintReadOnly)
	uint8 bOverrideActiveTeamType : 1;
	
	UPROPERTY(BlueprintReadOnly)
	ETeamType ExecutionTeamType = ETeamType::TT_SQUAD;

	//UPROPERTY(BlueprintReadOnly)
	//ESwatCommand DefaultHumanCommand = ESwatCommand::SC_Focus;

	UPROPERTY(BlueprintReadOnly)
	ESwatCommand DefaultDoorOpenCommand = ESwatCommand::SC_MoveAndClear;
	
	UPROPERTY(BlueprintReadOnly)
	ESwatCommand DefaultDoorUnknownCommand = ESwatCommand::SC_StackUp;

	UPROPERTY(BlueprintReadOnly)
	ESwatCommand DefaultCheckDoorCommand = ESwatCommand::SC_StackUp;
	
	UPROPERTY(BlueprintReadOnly)
	ESwatCommand DefaultDoorLockedCommand = ESwatCommand::SC_PickLock;

	UPROPERTY(BlueprintReadOnly)
	ESwatCommand DefaultDoorUnlockedCommand = ESwatCommand::SC_OpenAndClear;

	bool bHasEverOpened = false;
	
	void ExecuteCommand(FSwatCommand Command, bool bFromDefault = false, bool bOnlyVO = false);

	// store this because of the delay between execution and do
	FHitResult ExecutingContextualData;

	UFUNCTION(BlueprintPure)
	bool HasQueuedCommandForTeam(ETeamType TeamType) const;
	
	UFUNCTION(BlueprintPure)
	bool HasQueuedCommandForActiveTeam() const;

	bool CanQueue() const;
	void QueueCommand(FSwatCommand Command);
	
	UFUNCTION()
	void DoCommand(FSwatCommand Command, bool bFromQueue = false, ETeamType ActiveTeamOverride = ETeamType::TT_NONE, FHitResult ContextualDataOverride = FHitResult(), bool bOverrideContextualData = false);

	UFUNCTION(BlueprintPure)
    bool GetSubCommands(FSwatCommand Command, TArray<FSwatCommand>& OutSubCommands);
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	AActor* LastContextActor = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FHitResult ContextualData;
	
	FHitResult ContextualData2;

	UPROPERTY()
	FHitResult LastExecutedCommandContextualData;

	UPROPERTY(EditAnywhere)
	UFMODEvent* OpenMenuEvent = nullptr;

	UPROPERTY(EditAnywhere)
	UFMODEvent* OpenSubCommandMenuEvent = nullptr;

	UPROPERTY(EditAnywhere)
	UFMODEvent* ExecuteCommandEvent = nullptr;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> Flashbang;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> Stinger;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> CSGas;

	bool DoesAnySwatTeamHaveItem(TSubclassOf<ABaseItem> Item) const;
	bool DoesAnySwatTeamHaveItemType(EItemCategory ItemType) const;
	
	bool DoesSwatTeamHaveItem(ETeamType SwatTeam, TSubclassOf<ABaseItem> Item) const;
	bool DoesSwatTeamHaveItemType(ETeamType SwatTeam, EItemCategory ItemType) const;
	bool DoesLeaderHaveItemType(EItemCategory ItemType) const;

	UPROPERTY(BlueprintReadOnly)
	uint8 bHoldingQueueCommandKey : 1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USizeBox* SizeBox = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* VB_Commands = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* VB_Queue = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* VB_SwatCommand = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_1 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_2 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_3 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_4 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_5 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_6 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_7 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_8 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_9 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandEntry_10 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* SwatCommandIssued = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandEntryWidget* Back = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* txt_QueueBinding = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* txt_QueueStatus = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* CommandDirectoryText = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* DivTop = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* DivBottom = nullptr;

	UFUNCTION(BlueprintPure)
	ETeamType GetActiveTeam() const;
	
	UFUNCTION(BlueprintPure)
	class USwatCommandEntryWidget* IndexToEntryWidget(uint8 Index) const;

	UFUNCTION(BlueprintCallable)
	void InitEntryWidget(USwatCommandEntryWidget* Entry, const FSwatCommand& InSwatCommand, ETeamType Team, bool bLast);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor RedTeamColor = FLinearColor::FromSRGBColor(FColor::FromHex("F85533E6"));
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor BlueTeamColor = FLinearColor::FromSRGBColor(FColor::FromHex("589CF3E6"));
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor GoldTeamColor = FLinearColor::FromSRGBColor(FColor::FromHex("FFF455E6"));

public:
	UFUNCTION(BlueprintPure)
	FKey GetCommandInput(FName InName) const;
	
	UFUNCTION(BlueprintPure)
	FKey GetInputOne() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputTwo() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputThree() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputFour() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputFive() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputSix() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputSeven() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputEight() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputNine() const;
	UFUNCTION(BlueprintPure)
	FKey GetInputBack() const;
	
	UFUNCTION(BlueprintPure)
	FKey ConvertIntToInputKey(int32 Int) const;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPostUpdateSwatCommands, USwatCommandWidget*, Widget, TArray<FSwatCommand>&, SwatCommands);
	FOnPostUpdateSwatCommands OnPostUpdateSwatCommands;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSwatCommandIssued, ESwatCommand, SwatCommand, ETeamType, TeamType, AActor*, ContextActor);
	FOnSwatCommandIssued OnSwatCommandIssued;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwatCommandQueued, FQueuedSwatCommand, QueuedSwatCommand, ETeamType, TeamType);
	FOnSwatCommandQueued OnSwatCommandQueued;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSwatElementChanged, ETeamType, TeamType);
	FOnSwatElementChanged OnSwatElementChanged;

	UFUNCTION(BlueprintNativeEvent)
	void OnOpen();
	void OnOpen_Implementation();
	UFUNCTION(BlueprintNativeEvent)
	void OnClose();
	void OnClose_Implementation();
	UFUNCTION(BlueprintNativeEvent)
	void OnInputKey();
	void OnInputKey_Implementation();
	UFUNCTION(BlueprintNativeEvent)
	void OnCommandIssued(int32 Index, const FSwatCommand& Command, bool bFromDefault = false);
	void OnCommandIssued_Implementation(int32 Index, const FSwatCommand& Command, bool bFromDefault = false);
	
	void InputKey(FKey Key, bool bAddToComboKeys = true);
	void OpenCommandMenu();
	void CloseCommandMenu();
	void GrabContextData(FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam);
	void CycleSwatElement(bool bNext, bool bPlayVO = false);
	void IssueDefaultCommand();
	void SetActiveTeamElement(ETeamType TeamType);

	AActor* GetLastContextActor() const;
	
private:
	float TimeSinceLastPageUpdate = 0.0f;

	void UpdateCommandPageData();

	void IssueIncapHumanDefaultCommand(class AIncapacitatedHuman* IncapHuman);
	void IssueHumanDefaultCommand(class ACyberneticCharacter* AICharacter);
	void IssueDoorDefaultCommand(class ADoor* Door);
	void IssueCollectEvidenceCommand(AActor* EvidenceActor);
	void IssueDisarmTrapCommand(class ATrapActor* TrapActor);
	void IssueDisarmTrapCommand(class ATrapActorAttachedToDoor* TrapActor);
	void IssueExfilCommand(class AExfilActor* TrapActor);
	void IssueGlobalDefaultCommand();

	/** Override the default command according to Training state. */
	void UpdateTrainingDefaultCommand() const;

	/** Check if command can be issued given the current training activities. */
	void CanCommandBeIssuedForActivities(FSwatCommand& Command, const TArray<struct FSwatCommandData>& CommandsToIssue);
};
