// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "ReadyOrNotCommandFunctionLibrary.generated.h"

class USWATManager;

PRAGMA_DISABLE_OPTIMIZATION

UCLASS()
class READYORNOT_API UReadyOrNotCommandFunctionLibrary : public UObject
{
    GENERATED_BODY()

public:
    TArray<TArray<FSwatCommand>> CommandHistory;
    TArray<FKey> CommandCombo; // TODO: Remove?
    ETeamType ExecutionTeamType = ETeamType::TT_SQUAD;
    ETeamType ActiveTeamType = ETeamType::TT_SQUAD;
    FHitResult ContextualData;
    FHitResult ContextualData2;
    FHitResult ExecutingContextualData;
    FHitResult LastExecutedCommandContextualData;
    //FSwatCommand CurrentCommand;
    int32 LastSubCommandPageIndex = 0;
    bool ShouldQueue = false;
    bool IsGamePad = false;

    ESwatCommand DefaultCommand = ESwatCommand::SC_FallIn;
    ESwatCommand DefaultDoorOpenCommand = ESwatCommand::SC_MoveAndClear;
    ESwatCommand DefaultDoorUnknownCommand = ESwatCommand::SC_StackUp;
    ESwatCommand DefaultCheckDoorCommand = ESwatCommand::SC_StackUp;
    ESwatCommand DefaultDoorLockedCommand = ESwatCommand::SC_PickLock;
    ESwatCommand DefaultDoorUnlockedCommand = ESwatCommand::SC_OpenAndClear;

    TSubclassOf<class ABaseGrenade> Flashbang;
    TSubclassOf<class ABaseGrenade> Stinger;
    TSubclassOf<class ABaseGrenade> CSGas;

    FKey GetInputOne();
    FKey GetInputTwo();
    FKey GetInputThree();
    FKey GetInputFour();
    FKey GetInputFive();
    FKey GetInputSix();
    FKey ConvertIntToInputKey(int32 Int);

    TArray<FSwatCommand> GetCurrentCommandOptions();
    TArray<FSwatCommand> GetPreviousCommandOptions();

    void DoCommand(FSwatCommand Command, bool bFromQueue = false, ETeamType ActiveTeamOverride = ETeamType::TT_NONE,
                   FHitResult ContextualDataOverride = FHitResult(), bool bOverrideContextualData = false);
    void ExecuteCommand(FSwatCommand Command, bool
                        bFromDefault, bool bOnlyVO);
    void PlayVoiceCommand(FSwatCommand Command, bool bIsSwat, bool bIsFemaleTarget);
    
    void QueueCommand(FSwatCommand Command);

    void CancelCommand();
    void BuildDefaultPageData();
    void BuildQueuedPageData();
    FSwatCommand GetStackUpCommands(ADoor* Door, bool bPlayerInFront);
    FSwatCommand GetBreachCommands(ADoor* Door);
    FSwatCommand GetOpenDoorCommands(const ADoor* Door);
    FSwatCommand GetGamepadDoorCommands(ADoor* Door, APlayerCharacter* LocalPlayerCharacter,
                                        bool bPlayerInFront);
    FSwatCommand GetScanCommands(ADoor* Door, bool bPlayerInFront);
    void BuildDoorPageData(ADoor* Door, TArray<FSwatCommand>& Commands);
    void BuildIndividualCommands(AActor* Target);
    bool DoesSwatTeamHaveItem(ETeamType SwatTeam, TSubclassOf<ABaseItem> Item);
    bool DoesSwatTeamHaveItemType(ETeamType SwatTeam, EItemCategory ItemType);
    bool DoesAnySwatTeamHaveItem(TSubclassOf<ABaseItem> Item);
    bool DoesAnySwatTeamHaveItemType(EItemCategory ItemType);
    bool IsOtherTeamStackedUpOnDoor(ADoor* Door) const;
    void GrabContextData(bool IgnoreCharacters,
                         FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam);
    void GetExecutingBreachAndClearActivities(TArray<UTeamBreachAndClearActivity*>& OutActivities);
    void SetLastCommandPage(TArray<FSwatCommand>& InCommands);
    void SetActiveTeamElement(ETeamType TeamType);
    void CycleSwatElement(bool bNext);
    bool HasQueuedCommandForTeam(ETeamType TeamType);
    bool IsTeamBreachingDoor(ADoor* Door, ETeamType SwatTeam);
    bool IsTeamStackedUpOnDoor(ADoor* Door);
    bool IsOtherTeamStackedUpOnDoor(ADoor* Door, ETeamType& OutTeam);
    bool IsTeamStackedUpOnDoorWithStyle(ETeamType SwatTeam, ADoor* Door,
                                        EStackUpStyle StackUpStyle, bool bPlayerInFront) const;
    void UpdateCommandPageData(APawn* OwningPlayer);
    UFUNCTION()
    void RespondToSWATCommand(FSwatCommand Command, ETeamType TeamType, FHitResult CommandContextualData);
    void SelectCommand(int32 index, bool bAddToComboKeys = true);
    void SelectCommand(const FSwatCommand& SwatCommand);
    void InputKey(FKey Key, bool bAddToComboKeys = true);
    FKey GetCommandInput(FName InName);
    void Reset();
    bool IsIndividualCommands() const { return bIndividualCommands; }
    FString CurrentOfficersName() const { return OfficerName; }

    UWorld* World;
    USWATManager* SwatManager;

private:
    UPROPERTY()
    AActor* LastActorBeforeGoingIntoSubPage = nullptr;
    bool bIndividualCommands = false;
    FString OfficerName;
    TArray<FSwatCommand> GetPostDoorCommands(
        ESwatCommand Clear,
        ESwatCommand ClearWithStinger,
        ESwatCommand ClearWithCSGas,
        ESwatCommand ClearWithLauncher,
        ESwatCommand ClearWithLeader,
        ESwatCommand ClearWithFlashbang
    );
};

PRAGMA_ENABLE_OPTIMIZATION
