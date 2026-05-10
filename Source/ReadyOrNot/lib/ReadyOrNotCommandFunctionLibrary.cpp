// Copyright Void Interactive, 2023

#include "ReadyOrNotCommandFunctionLibrary.h"

#include "Actors/BaseGrenade.h"
#include "Actors/StackUpActor.h"
#include "Actors/ThrownEvidenceActor.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Gameplay/IncapacitatedHuman.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/BreachingShotgun.h"
#include "Actors/Items/C2Explosive.h"
#include "Actors/Items/DoorRam.h"
#include "Actors/Items/GrenadeLauncher.h"
#include "Info/SWATManager.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "Subsystems/AchievementSubsystem.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Info/Activities/Team/TeamFallinActivity.h"

#define LOCTEXT_NAMESPACE "SwatCommandWidget"

PRAGMA_DISABLE_OPTIMIZATION

// TODO: no TActorIterators or TObjectIterators

FKey UReadyOrNotCommandFunctionLibrary::GetInputOne()
{
    return GetCommandInput("SwatInputKeyOne");
}

FKey UReadyOrNotCommandFunctionLibrary::GetInputTwo()
{
    return GetCommandInput("SwatInputKeyTwo");
}

FKey UReadyOrNotCommandFunctionLibrary::GetInputThree()
{
    return GetCommandInput("SwatInputKeyThree");
}

FKey UReadyOrNotCommandFunctionLibrary::GetInputFour()
{
    return GetCommandInput("SwatInputKeyFour");
}

FKey UReadyOrNotCommandFunctionLibrary::GetInputFive()
{
    return FKey();
}

FKey UReadyOrNotCommandFunctionLibrary::GetInputSix()
{
    return FKey();
}

static void RecursiveSet(TArray<FSwatCommand>& InCommands)
{
    for (FSwatCommand& SwatCommand : InCommands)
    {
        if (SwatCommand.SubCommands.Num() > 0)
            RecursiveSet(SwatCommand.SubCommands);

        SwatCommand.bUseSecondaryContextData = true;
    }
}

void UReadyOrNotCommandFunctionLibrary::BuildDefaultPageData()
{
    if (!SwatManager || !World)
    {
        return;
    }

    if (CommandHistory.Num() != 0 && !(CommandHistory.Num() == 1 && bIndividualCommands))
    {
        return;
    }

    const bool bIsSinglePlayer = World->GetNetDriver() == nullptr;

    if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
        return;

    bIndividualCommands = false;

    TArray<FSwatCommand> Commands = TArray<FSwatCommand>();

    Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveTo"), ESwatCommand::SC_MoveTo, true));
    Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cover"), ESwatCommand::SC_Cover, true));
    Commands.Add(FSwatCommand(
        SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType)
            ? FText::FromStringTable("SwatCommandTable", "Resume")
            : FText::FromStringTable("SwatCommandTable", "Hold"), SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType)
                                                                      ? ESwatCommand::SC_Resume
                                                                      : ESwatCommand::SC_Hold, false));

    FSwatCommand DeployFlashbangCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "DeployFlashbang"), ESwatCommand::SC_DeployFlashbang,
        true, DoesSwatTeamHaveItemType(ActiveTeamType,
                                       EItemCategory::IC_Flashbang));
    FSwatCommand DeployStingerCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "DeployStinger"), ESwatCommand::SC_DeployStinger, true,
        DoesSwatTeamHaveItemType(ActiveTeamType,
                                 EItemCategory::IC_Stingball));
    FSwatCommand DeployCSGasCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "DeployCSGas"), ESwatCommand::SC_DeployCSGas, true,
        DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas));

    FSwatCommand DeployChemLightCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "DeployChemlight"), ESwatCommand::SC_DeployChemlight,
        true);

    FSwatCommand DeployShield = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "DeployShield"), ESwatCommand::SC_DeployShield, false,
        DoesSwatTeamHaveItem(ActiveTeamType,
                             ABallisticsShield::StaticClass()));

    if (IsGamePad)
    {
        Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Deploy..."), ESwatCommand::PC_Deploy,
                                  {
                                      DeployShield,
                                      DeployStingerCommand,
                                      DeployCSGasCommand,
                                      DeployChemLightCommand,
                                      DeployFlashbangCommand
                                  }));
    }
    else
    {
        Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Deploy..."), ESwatCommand::PC_Deploy,
                                  {
                                      DeployFlashbangCommand,
                                      DeployStingerCommand,
                                      DeployCSGasCommand,
                                      DeployChemLightCommand,
                                      DeployShield
                                  }));
    }

    const FVector Location = ContextualData.Location + ContextualData.ImpactNormal * 50.0f;

    bool bIsOutside = false;
    bool bHasTAA = false;
    if (const AThreatAwarenessActor* TAA = UThreatAwarenessSubsystem::Get(World)->GetNearestThreatForLocation(
        Location, 500.0f, 200.0f, true))
    {
        bIsOutside = TAA->bIsOutside;
        bHasTAA = true;
    }

    Commands.Add(FSwatCommand(
        bIsOutside || !bHasTAA
            ? FText::FromStringTable("SwatCommandTable", "SearchArea")
            : FText::FromStringTable("SwatCommandTable", "SearchRoom"), ESwatCommand::SC_SearchAndSecureRoom, true,
        bHasTAA,
        false));

    if (SwatManager->GetSWATCount() > 0)
    {
        Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "FallIn"), ESwatCommand::PC_FallIn,
                                  {
                                      FSwatCommand(
                                          FText::FromStringTable("SwatCommandTable", "SingleFile"),
                                          ESwatCommand::SC_FallIn_Snake, false,
                                          true),
                                      FSwatCommand(
                                          FText::FromStringTable("SwatCommandTable", "DoubleFile"),
                                          ESwatCommand::SC_FallIn_HalfSnake,
                                          false, true),
                                      FSwatCommand(
                                          FText::FromStringTable("SwatCommandTable", "Diamond"),
                                          ESwatCommand::SC_FallIn_Diamond, false,
                                          true),
                                      FSwatCommand(
                                          FText::FromStringTable("SwatCommandTable", "Wedge"),
                                          ESwatCommand::SC_FallIn_Flock, false, true)
                                  }, true));
    }
    else
    {
        Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "FallIn"), ESwatCommand::SC_FallIn, true,
                                  true));
    }

    CommandHistory.Add(Commands);
}

void UReadyOrNotCommandFunctionLibrary::BuildQueuedPageData()
{
    if (!SwatManager)
    {
        return;
    }

    TArray<FSwatCommand> Commands = TArray<FSwatCommand>();

    Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Execute"), ESwatCommand::SC_Execute, false));
    Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cancel"), ESwatCommand::SC_Cancel, false));

    CommandHistory.Empty();
    CommandHistory.Add(Commands);
}

FSwatCommand UReadyOrNotCommandFunctionLibrary::GetStackUpCommands(ADoor* Door, bool bPlayerInFront)
{
    FSwatCommand StackUpCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "StackUp"),
                                               ESwatCommand::PC_StackUp);

    const bool bIsDoorCenterFed_OppositeSide = bPlayerInFront
                                                   ? Door->BackRoomPosition == EDoorRoomPosition::Center
                                                   : Door->FrontRoomPosition == EDoorRoomPosition::Center;

    const bool bIsDoorHallwayCenter_OppositeSide = bPlayerInFront
                                                       ? Door->BackRoomPosition == EDoorRoomPosition::Hallway
                                                       : Door->FrontRoomPosition == EDoorRoomPosition::Hallway;

    TArray<FSwatCommand> StackUpSubCommands;
    if (bIsDoorCenterFed_OppositeSide || bIsDoorHallwayCenter_OppositeSide)
    {
        const bool bAnyLeftStackPoints = bPlayerInFront
                                             ? Door->FrontRightStackUpPoints.Num() > 0
                                             : Door->BackLeftStackUpPoints.Num() > 0;
        const bool bAnyRightStackPoints = bPlayerInFront
                                              ? Door->FrontLeftStackUpPoints.Num() > 0
                                              : Door->BackRightStackUpPoints.Num() > 0;

        bool bSplitEnabled = bAnyLeftStackPoints && bAnyRightStackPoints && SwatManager->GetSWATCount() > 1 && !
            IsTeamStackedUpOnDoorWithStyle(ActiveTeamType, Door, EStackUpStyle::Split,
                                           bPlayerInFront);
        bool bLeftEnabled = bAnyLeftStackPoints && !IsTeamStackedUpOnDoorWithStyle(
            ActiveTeamType, Door, EStackUpStyle::Left, bPlayerInFront);
        bool bRightEnabled = bAnyRightStackPoints && !IsTeamStackedUpOnDoorWithStyle(
            ActiveTeamType, Door, EStackUpStyle::Right, bPlayerInFront);

        // enable for coop play
        if (SwatManager->GetSWATCount() == 0)
        {
            bSplitEnabled = true;
            bLeftEnabled = true;
            bRightEnabled = true;
        }

        FSwatCommand SplitAutoCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Auto"),
                                             ESwatCommand::SC_StackUp, false, true, false, false);
        FSwatCommand SplitLeftCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Left"), ESwatCommand::SC_StackUpLeft, false, bLeftEnabled,
            false, false);
        FSwatCommand SplitRightCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Right"), ESwatCommand::SC_StackUpRight, false, bRightEnabled,
            false, false);

        StackUpSubCommands.Add(FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Split"), ESwatCommand::SC_StackUpSplit, false, bSplitEnabled,
            false, false));

        if (IsGamePad)
        {
            StackUpSubCommands.Add(SplitRightCommand);
            StackUpSubCommands.Add(SplitAutoCommand);
            StackUpSubCommands.Add(SplitLeftCommand);
        }
        else
        {
            StackUpSubCommands.Add(SplitLeftCommand);
            StackUpSubCommands.Add(SplitRightCommand);
            StackUpSubCommands.Add(SplitAutoCommand);
        }

        StackUpCommand.SubCommands = StackUpSubCommands;
        return StackUpCommand;
    }
    StackUpCommand.CommandType = ESwatCommand::SC_StackUp;
    return StackUpCommand;
    // StackUpCommand_DoorwayOnly.CommandText = FText::FromStringTable("SwatCommandTable", "StackUp");
    // StackUpCommand_DoorwayOnly.CommandType = ESwatCommand::SC_StackUp;
}

FSwatCommand UReadyOrNotCommandFunctionLibrary::GetBreachCommands(ADoor* Door)
{
    FSwatCommand BreachCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Breach..."), ESwatCommand::PC_Breach,
                                       false, !Door->IsDoorwayOnly() && !Door->IsOpen());

    FSwatCommand KickCommands = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Kick..."), ESwatCommand::PC_Kick,
        GetPostDoorCommands(ESwatCommand::SC_KickAndClear,
                            ESwatCommand::SC_KickAndClearStinger,
                            ESwatCommand::SC_KickAndClearCSGas,
                            ESwatCommand::SC_KickAndClearLauncher,
                            ESwatCommand::SC_KickAndClearLeader,
                            ESwatCommand::SC_KickAndClearFlashbang), Door->CanKickDoor(), false, false);

    FSwatCommand ShotgunCommands = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Shotgun..."), ESwatCommand::PC_Shotgun,
        GetPostDoorCommands(
            ESwatCommand::SC_ShotgunClear,
            ESwatCommand::SC_ShotgunClearStinger,
            ESwatCommand::SC_ShotgunClearCSGas,
            ESwatCommand::SC_ShotgunClearLauncher,
            ESwatCommand::SC_ShotgunClearLeader,
            ESwatCommand::SC_ShotgunClearFlashbang), DoesSwatTeamHaveItemType(
            ActiveTeamType, EItemCategory::IC_BreachingShotgun),
        false, false);

    FSwatCommand C2Commands = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "C2..."), ESwatCommand::PC_C2, GetPostDoorCommands(
            ESwatCommand::SC_C2Clear,
            ESwatCommand::SC_C2ClearStinger,
            ESwatCommand::SC_C2ClearCSGas,
            ESwatCommand::SC_C2ClearLauncher,
            ESwatCommand::SC_C2ClearLeader,
            ESwatCommand::SC_C2ClearFlashbang), DoesSwatTeamHaveItemType(ActiveTeamType,
                                                                         EItemCategory::IC_C2Explosive), false, false);

    FSwatCommand RamCommands = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Ram..."), ESwatCommand::PC_Ram,
        GetPostDoorCommands(
            ESwatCommand::SC_RamAndClear,
            ESwatCommand::SC_RamAndClearStinger,
            ESwatCommand::SC_RamAndClearCSGas,
            ESwatCommand::SC_RamAndClearLauncher,
            ESwatCommand::SC_RamAndClearLeader,
            ESwatCommand::SC_RamAndClearFlashbang), DoesSwatTeamHaveItemType(ActiveTeamType,
                                                                             EItemCategory::IC_BatteringRam), false,
        false);

    FSwatCommand LeaderCommands = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Leader..."), ESwatCommand::PC_Leader,
        GetPostDoorCommands(
            ESwatCommand::SC_LeaderAndClear,
            ESwatCommand::SC_LeaderAndClear,
            ESwatCommand::SC_LeaderAndClearCSGas,
            ESwatCommand::SC_LeaderAndClearLauncher,
            ESwatCommand::SC_LeaderAndClearLeader,
            ESwatCommand::SC_LeaderAndClearFlashbang), true, false, false);

    if (IsGamePad)
    {
        BreachCommands.SubCommands.Add(KickCommands);
        BreachCommands.SubCommands.Add(RamCommands);
        BreachCommands.SubCommands.Add(ShotgunCommands);
        BreachCommands.SubCommands.Add(LeaderCommands);
        BreachCommands.SubCommands.Add(C2Commands);
    }
    else
    {
        BreachCommands.SubCommands.Add(KickCommands);
        BreachCommands.SubCommands.Add(ShotgunCommands);
        BreachCommands.SubCommands.Add(C2Commands);
        BreachCommands.SubCommands.Add(RamCommands);
        BreachCommands.SubCommands.Add(LeaderCommands);
    }

    return BreachCommands;
}

FSwatCommand UReadyOrNotCommandFunctionLibrary::GetOpenDoorCommands(const ADoor* Door)
{
    if (Door->GetSWATKnowsLockState() && Door->IsLocked())
    {
        return FSwatCommand(FText::FromStringTable("SwatCommandTable", "PickLock"), ESwatCommand::SC_PickLock, false,
                            true);
    }

    if (Door->IsOpenBeyondCloseThreshold())
    {
        return FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Move..."), ESwatCommand::PC_Move,
            GetPostDoorCommands(ESwatCommand::SC_MoveAndClear,
                                ESwatCommand::SC_MoveAndClearStinger,
                                ESwatCommand::SC_MoveAndClearCSGas,
                                ESwatCommand::SC_MoveAndClearLauncher,
                                ESwatCommand::SC_MoveAndClearLeader,
                                ESwatCommand::SC_MoveAndClearFlashbang)
        );
    }

    return FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Open..."), ESwatCommand::PC_Open,
        GetPostDoorCommands(ESwatCommand::SC_OpenAndClear,
                            ESwatCommand::SC_OpenAndClearStinger,
                            ESwatCommand::SC_OpenAndClearCSGas,
                            ESwatCommand::SC_OpenAndClearLauncher,
                            ESwatCommand::SC_OpenAndClearLeader,
                            ESwatCommand::SC_OpenAndClearFlashbang)
    );
}

FSwatCommand UReadyOrNotCommandFunctionLibrary::GetGamepadDoorCommands(ADoor* Door,
                                                                       APlayerCharacter* LocalPlayerCharacter,
                                                                       bool bPlayerInFront)
{
    const bool bTrapInFront = Door->GetAttachedTrap()
                                  ? Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation())
                                  : false;
    const bool bCanDisarmTrap = !Door->IsLocked() && !Door->IsJammed() && Door->GetAttachedTrap() && ((Door->
            GetAttachedTrap()->TrapStatus == ETrapState::TS_Live && (Door->DoesSWATKnowTrapState() || bTrapInFront ==
                bPlayerInFront || Door->IsOpen())) || Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Activated ||
        Door->AnyBottomDoorChunksBroken());

    const bool bDoorWedgeCommandEnabled = Door->IsClosed() && (Door->bDoorJammed || (!Door->bDoorJammed &&
        DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Doorjam)));


    FSwatCommand DoorCommand = FSwatCommand(
        Door->IsOpen()
            ? FText::FromStringTable("SwatCommandTable", "CloseDoor")
            : FText::FromStringTable("SwatCommandTable", "OpenDoor"),
        Door->CanCloseDoor(LocalPlayerCharacter) ? ESwatCommand::SC_CloseDoor : ESwatCommand::SC_OpenDoor, false,
        Door->CanOpenDoor());

    if (Door->GetSWATKnowsLockState() && Door->IsLocked())
    {
        DoorCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "PickLock"), ESwatCommand::SC_PickLock,
                                   false,
                                   true);
    }
    return FSwatCommand(FText::FromStringTable("SwatCommandTable", "Door..."), ESwatCommand::PC_Door,
                        {
                            DoorCommand,
                            FSwatCommand(
                                FText::FromStringTable("SwatCommandTable", "MirrorUnderDoor"),
                                ESwatCommand::SC_DeployMirrorgun, false,
                                Door->IsClosed()),
                            FSwatCommand(
                                Door->bDoorJammed
                                    ? FText::FromStringTable("SwatCommandTable", "RemoveWedge")
                                    : FText::FromStringTable("SwatCommandTable", "WedgeDoor"),
                                Door->bDoorJammed ? ESwatCommand::SC_RemoveDoorJam : ESwatCommand::SC_DeployDoorJam,
                                false, bDoorWedgeCommandEnabled)
                        }, true, false, false);
}

FSwatCommand UReadyOrNotCommandFunctionLibrary::GetScanCommands(ADoor* Door, const bool bPlayerInFront)
{
    const bool bIsDoorCenterFed_OppositeSide = bPlayerInFront
                                                   ? Door->BackRoomPosition == EDoorRoomPosition::Center
                                                   : Door->FrontRoomPosition == EDoorRoomPosition::Center;
    EStackupGenArea StackUpArea;
    if (bPlayerInFront)
    {
        StackUpArea = EStackupGenArea::SGA_FrontRight;
    }
    else
    {
        StackUpArea = EStackupGenArea::SGA_BackRight;
    }

    const TArray<AStackUpActor*>& StackUpActors = Door->GetStackupsForArea(StackUpArea);

    bool bIsSplit = false;
    bool bSameTeam = true;

    TArray<AStackUpActor*> CurrentSideStackUps = Door->GetStackupsForArea(
        bPlayerInFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);
    bool bAnyStackedLeft = false;
    for (const AStackUpActor* StackUpActor : CurrentSideStackUps)
    {
        if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
        {
            if (ActiveTeamType != ETeamType::TT_SQUAD)
            {
                bSameTeam = ActiveTeamType == Cast<ACyberneticController>(StackUpActor->OccupiedBy)->GetTeam();
            }
            bAnyStackedLeft = true;
            break;
        }
    }
    CurrentSideStackUps = Door->GetStackupsForArea(bPlayerInFront
                                                       ? EStackupGenArea::SGA_FrontRight
                                                       : EStackupGenArea::SGA_BackRight);
    bool bAnyStackedRight = false;
    for (const AStackUpActor* StackUpActor : CurrentSideStackUps)
    {
        if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
        {
            if (ActiveTeamType != ETeamType::TT_SQUAD)
            {
                bSameTeam = ActiveTeamType == Cast<ACyberneticController>(StackUpActor->OccupiedBy)->GetTeam();
            }
            bAnyStackedRight = true;
            break;
        }
    }

    bIsSplit = bAnyStackedLeft && bAnyStackedRight;

    bool bCanScan = bIsDoorCenterFed_OppositeSide &&
        StackUpActors.Num() > 0 &&
        (!bIsSplit && bSameTeam) &&
        !((Door->IsLocked() && Door->TeamKnowsDoorLockState(false)) || Door->IsJammed() || Door->IsDoorBroken());

    return FSwatCommand(FText::FromStringTable("SwatCommandTable", "Scan..."), ESwatCommand::PC_Scan,
                        {
                            FSwatCommand(
                                FText::FromStringTable("SwatCommandTable", "Slide"), ESwatCommand::SC_Slide, false,
                                Door->IsOpenBeyondCloseThreshold()),
                            FSwatCommand(
                                FText::FromStringTable("SwatCommandTable", "PIE"), ESwatCommand::SC_Slice, false, true),
                            FSwatCommand(
                                FText::FromStringTable("SwatCommandTable", "Peek"), ESwatCommand::SC_Snap, false,
                                !IsTeamStackedUpOnDoor(Door)),
                        }, bCanScan);
}

void UReadyOrNotCommandFunctionLibrary::BuildDoorPageData(ADoor* Door, TArray<FSwatCommand>& Commands)
{
    APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));

    if (!SwatManager || !Door || !LocalPlayerCharacter)
    {
        return;
    }

    const bool bIsSinglePlayer = World->GetNetDriver() == nullptr;

    if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
    {
        return;
    }

    if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
    {
        Door = Door->GetSubDoor();
    }


    const bool bPlayerInFront = Door->IsPointInFrontOfDoorway(LocalPlayerCharacter->GetActorLocation());


    //LOG_NUMBER(FVector::Distance(Door->GetDoorMidLocation(), LocalPlayerCharacter->GetActorLocation()));

    // if(!Door->IsDoorwayOnly())
    // {
    // 	StackUpCommand.SubCommands = StackUpSubCommands;
    // }
    //FSwatCommand StackUpCommand = Commands.Add(Door->IsDoorwayOnly() ? StackUpCommand_DoorwayOnly : StackUpCommand);

    bool bLauncherHasAmmo = SwatManager->GetSWATCount() == 0;
    if (ASWATCharacter* Swat = SwatManager->GetSwatWithItem(ActiveTeamType, AGrenadeLauncher::StaticClass()))
    {
        if (AGrenadeLauncher* Launcher = Swat->GetInventoryComponent()->GetInventoryItemOfClass_Native<
            AGrenadeLauncher>(AGrenadeLauncher::StaticClass(), false))
        {
            bLauncherHasAmmo = Launcher->HasAmmo();
        }
    }


    // if (!IsGamePad && !Door->IsDoorwayOnly() && !Door->IsDoorBroken())
    // {
    // 	Commands.Add(FSwatCommand(
    // 		FText::FromStringTable("SwatCommandTable", "MirrorUnderDoor"), ESwatCommand::SC_DeployMirrorgun, false, Door->IsClosed()));
    //
    // 	if (bCanDisarmTrap)
    // 	{
    // 		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "DisarmTrap"), ESwatCommand::SC_DisarmTrap, false, true));
    // 	}
    //
    // 	Commands.Add(FSwatCommand(
    // 		Door->bDoorJammed ? FText::FromStringTable("SwatCommandTable", "RemoveWedge") : FText::FromStringTable("SwatCommandTable", "WedgeDoor"),
    // 		Door->bDoorJammed ? ESwatCommand::SC_RemoveDoorJam : ESwatCommand::SC_DeployDoorJam, false,
    // 		bDoorWedgeCommandEnabled));
    // }

    FSwatCommand CoverCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cover"), ESwatCommand::SC_Cover, true);

    FSwatCommand ToggleDoorCommand = FSwatCommand(
        Door->IsOpen()
            ? FText::FromStringTable("SwatCommandTable", "CloseDoor")
            : FText::FromStringTable("SwatCommandTable", "OpenDoor"),
        Door->CanCloseDoor(LocalPlayerCharacter) ? ESwatCommand::SC_CloseDoor : ESwatCommand::SC_OpenDoor, false,
        Door->CanOpenDoor());
    ToggleDoorCommand.bEnabled = !((Door->IsLocked() && Door->TeamKnowsDoorLockState(false)) || Door->IsJammed() ||
        Door->IsDoorBroken());

    Commands.Add(ToggleDoorCommand);


    // we cannot stack up red/blue on the same door
    ETeamType StackedUpTeam = ActiveTeamType;
    if (IsOtherTeamStackedUpOnDoor(Door, StackedUpTeam))
    {
        SetActiveTeamElement(StackedUpTeam);
    }

    if (IsGamePad)
    {
        Commands = {
            GetOpenDoorCommands(Door),
            GetBreachCommands(Door),
            GetScanCommands(Door, bPlayerInFront),
            CoverCommand,
            GetGamepadDoorCommands(Door, LocalPlayerCharacter, bPlayerInFront),
            GetStackUpCommands(Door, bPlayerInFront)
        };
    }

    for (FSwatCommand& Command : Commands)
    {
        bool bIsTeamBreaching = IsTeamBreachingDoor(Door, ActiveTeamType);
        bool bCanIssueOrderOnThisSide = (Door->bCanIssueOrdersOnFrontSide && bPlayerInFront) || (Door->
            bCanIssueOrdersOnBackSide && !bPlayerInFront);
        if (Command.bEnabled)
        {
            Command.bEnabled = !bIsTeamBreaching && bCanIssueOrderOnThisSide;
        }
    }


    CommandHistory.Empty();
    CommandHistory.Add(Commands);
}


void UReadyOrNotCommandFunctionLibrary::BuildIndividualCommands(AActor* Target)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
                                         FString::Printf(TEXT("BuildDoorPageData: %d"), CommandHistory.Num()));
    }

    ACyberneticCharacter* TargetCharacter = Cast<ACyberneticCharacter>(Target);

    if (!SwatManager || !TargetCharacter)
    {
        return;
    }

    bIndividualCommands = true;

    ESwatCommand DefaultHumanSwatCommand = ESwatCommand::SC_None;

    if (!TargetCharacter->IsOnSWATTeam())
    {
        if ((TargetCharacter->IsArrested() || TargetCharacter->IsDeadOrUnconscious() || TargetCharacter->
            IsPlayingDead()) && !TargetCharacter->HasBeenReported())
        {
            DefaultHumanSwatCommand = ESwatCommand::SC_DoReportTarget;
        }
        else
        {
            if (TargetCharacter->CanArrest() && !TargetCharacter->IsPlayingDead())
            {
                DefaultHumanSwatCommand = ESwatCommand::SC_DoArrestTarget;
            }
        }
    }

    if (DefaultHumanSwatCommand != ESwatCommand::SC_None)
    {
        if (SwatManager)
        {
            SwatManager->CurrentDefaultCommand = DefaultHumanSwatCommand;
        }
    }


    //bOverrideActiveTeamType = true; // TODO: Check with SwatCommandWidget
    //OverrideActiveTeamType = ETeamType::TT_NONE;

    OfficerName = "";
    if (TargetCharacter->IsOnSWATTeam())
    {
        const ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(TargetCharacter);
        if (SwatCharacter)
        {
            OfficerName = SwatCharacter->GetSwatCharacterName().ToString();
        }
    }

    TArray<FSwatCommand> Commands = TArray<FSwatCommand>();

    APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));

    if (TargetCharacter->IsOnSWATTeam() && SwatManager)
    {
        if (!TargetCharacter->IsActive())
        {
            return;
        }

        bool bHasTeamActivity = false;
        bool bTeamActivityConsiderForFocusBlocking = false;
        bool bCanSwapNow = false;
        bool bIsSplit = false;
        bool bCurrentIsLeft = false;
        bool bIsStackUp = false;
        bool bIsClearing = false;
        uint8 NumSameTeamActivity = 0;
        FGuid Id;
        ESquadPosition SquadPosition = ESquadPosition::SP_NONE;
        EStackupGenArea CurrentStackUpArea = EStackupGenArea::SGA_None;
        ESquadPosition MaxOppositeSideSquadPosition = ESquadPosition::SP_NONE;
        ESquadPosition MaxCurrentSideSquadPosition = ESquadPosition::SP_NONE;
        ESquadPosition MaxOverrideSquadPosition = ESquadPosition::SP_NONE;
        uint8 NumOppositeStackUps = 0;
        if (ACyberneticController* Controller = TargetCharacter->GetCyberneticsController())
        {
            if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
            {
                bCanSwapNow = Activity->CanSwapSquadPositions();
                Id = Activity->SharedData->ActivityId;
                NumSameTeamActivity = 1;
                bHasTeamActivity = true;
                SquadPosition = Activity->OverrideSquadPosition;

                bTeamActivityConsiderForFocusBlocking = Cast<UTeamStackUpActivity>(Activity) || Cast<
                    UTeamFallinActivity>(Activity);

                UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Other)
                {
                    if (Other != Activity && Other->SharedData->ActivityId == Activity->SharedData->ActivityId && !Other
                        ->IsActivityComplete())
                    {
                        if (MaxOverrideSquadPosition == ESquadPosition::SP_NONE)
                        {
                            MaxOverrideSquadPosition = Other->OverrideSquadPosition;
                        }
                        else
                        {
                            if (Other->OverrideSquadPosition > MaxOverrideSquadPosition)
                            {
                                MaxOverrideSquadPosition = Other->OverrideSquadPosition;
                            }
                        }
                    }

                    return true;
                });
            }

            if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
            {
                bIsStackUp = true;

                CurrentStackUpArea = Activity->GetStackUpArea();

                if (Activity->GetStackUpDoor()->IsPointInFrontOfDoorway(Activity->SharedData->CommandLocation))
                {
                    if (CurrentStackUpArea == EStackupGenArea::SGA_FrontRight)
                    {
                        bCurrentIsLeft = true;
                    }
                }
                else
                {
                    if (CurrentStackUpArea == EStackupGenArea::SGA_BackLeft)
                    {
                        bCurrentIsLeft = true;
                    }
                }

                EStackupGenArea OppositeStackUpArea = CurrentStackUpArea;
                TArray<AStackUpActor*> CurrentSideStackUps = Activity->GetStackUpDoor()->GetStackupsForArea(
                    CurrentStackUpArea);
                for (AStackUpActor* StackUpActor : CurrentSideStackUps)
                {
                    if (StackUpActor->OccupiedBy)
                    {
                        if (MaxCurrentSideSquadPosition == ESquadPosition::SP_NONE ||
                            (StackUpActor->GetSquadPosition() > MaxCurrentSideSquadPosition))
                        {
                            MaxCurrentSideSquadPosition = StackUpActor->GetSquadPosition();
                        }
                    }
                }

                ADoor::FlipStackUpArea(OppositeStackUpArea, true, false);

                TArray<AStackUpActor*> OppositeSideStackUps = Activity->GetStackUpDoor()->GetStackupsForArea(
                    OppositeStackUpArea);
                NumOppositeStackUps = OppositeSideStackUps.Num();
                for (AStackUpActor* StackUpActor : OppositeSideStackUps)
                {
                    if (StackUpActor->OccupiedBy)
                    {
                        if (MaxOppositeSideSquadPosition == ESquadPosition::SP_NONE ||
                            (StackUpActor->GetSquadPosition() > MaxOppositeSideSquadPosition))
                        {
                            MaxOppositeSideSquadPosition = StackUpActor->GetSquadPosition();
                        }
                    }
                }

                if (MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE)
                {
                    bIsSplit = true;
                }
            }

            if (UTeamBreachAndClearActivity* Activity = Controller->GetCurrentActivity<UTeamBreachAndClearActivity>())
            {
                bIsClearing = Activity->GetActiveStateID() >= 5;
            }
        }

        UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Activity)
        {
            if (Activity->SharedData->ActivityId == Id)
            {
                NumSameTeamActivity++;
            }

            return true;
        });

        TArray<FSwatCommand> MoveToSquadPositionCommands;

        if (!IsGamePad)
        {
            const FSwatCommand MovePositionTable[4] =
            {
                // Double spaces cos the text feels a bit cramped
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "MoveToAlphaOtherSide"), ESwatCommand::SC_MoveToAlpha,
                    TargetCharacter,
                    true, MaxOppositeSideSquadPosition == ESquadPosition::SP_NONE || MaxOppositeSideSquadPosition >=
                    ESquadPosition::SP_Alpha),
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "MoveToBravoOtherSide"), ESwatCommand::SC_MoveToBeta,
                    TargetCharacter,
                    true, MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >=
                    ESquadPosition::SP_Alpha),
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "MoveToCharlieOtherSide"),
                    ESwatCommand::SC_MoveToCharlie,
                    TargetCharacter, true,
                    MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >=
                    ESquadPosition::SP_Beta),
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "MoveToDeltaOtherSide"), ESwatCommand::SC_MoveToDelta,
                    TargetCharacter,
                    true, MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >=
                    ESquadPosition::SP_Charlie),
            };
            if (bHasTeamActivity && bIsStackUp && NumOppositeStackUps > 0)
            {
                for (uint8 i = 0; i < 4; i++)
                {
                    if (MovePositionTable[i].bEnabled)
                    {
                        MoveToSquadPositionCommands.Add(MovePositionTable[i]);
                    }
                }
            }
        }

        bool bCanMove = !bHasTeamActivity || (bHasTeamActivity && bCanSwapNow && !bIsClearing);
        FSwatCommand MoveIndividualCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Move..."), ESwatCommand::PC_Move,
            {
                FSwatCommand(FText::FromStringTable("SwatCommandTable", "Here"), ESwatCommand::SC_MoveTo_Individual,
                             TargetCharacter,
                             true, true),
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "HereThenBack"),
                    ESwatCommand::SC_MoveToAndBack_Individual,
                    TargetCharacter, true, true)
            }, bCanMove, true);

        if (MoveToSquadPositionCommands.Num() > 0)
        {
            MoveIndividualCommand.SubCommands += MoveToSquadPositionCommands;
        }

        Commands.Add(MoveIndividualCommand); // 1

        bool bHasCustomFocus = TargetCharacter->GetCyberneticsController()->GetTargetingComp()->CustomFocusLocation !=
            FVector::ZeroVector ||
            TargetCharacter->GetCyberneticsController()->GetTargetingComp()->CustomFocusActor != nullptr;

        bool bCanFocus = false;
        if (bHasTeamActivity)
        {
            if (!bTeamActivityConsiderForFocusBlocking)
            {
                bCanFocus = true;
            }
            else
            {
                bCanFocus = SquadPosition != ESquadPosition::SP_Alpha && !bIsClearing;
            }
        }
        else
        {
            bCanFocus = true;
        }

        FSwatCommand FocusIndividualCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "Focus..."), ESwatCommand::PC_Focus,
            {
                FSwatCommand(FText::FromStringTable("SwatCommandTable", "Here"), ESwatCommand::SC_Focus_Individual,
                             TargetCharacter,
                             true, true),
                FSwatCommand(FText::FromStringTable("SwatCommandTable", "Door"), ESwatCommand::SC_FocusDoor_Individual,
                             TargetCharacter, true, false),
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "Unfocus"), ESwatCommand::SC_UnFocus_Individual,
                    TargetCharacter, false, bHasCustomFocus),
                FSwatCommand(FText::FromStringTable("SwatCommandTable", "Target"),
                             ESwatCommand::SC_FocusTarget_Individual,
                             TargetCharacter, true, false),
            }, bCanFocus, true);

        if (!IsGamePad)
        {
            FocusIndividualCommand.SubCommands.Insert(
                FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "MyPosition"),
                    ESwatCommand::SC_Focus_MyPosition_Individual,
                    TargetCharacter, true, true), 1);
        }

        Commands.Add(FocusIndividualCommand); //2

        const FSwatCommand CurrentCommandTable[4] =
        {
            FSwatCommand(FText::FromStringTable("SwatCommandTable", "Alpha"), ESwatCommand::SC_SwapWithAlpha,
                         TargetCharacter, true,
                         SquadPosition != ESquadPosition::SP_Alpha && MaxOverrideSquadPosition >=
                         ESquadPosition::SP_Alpha),
            FSwatCommand(FText::FromStringTable("SwatCommandTable", "Bravo"), ESwatCommand::SC_SwapWithBeta,
                         TargetCharacter, true,
                         SquadPosition != ESquadPosition::SP_Beta && MaxOverrideSquadPosition >=
                         ESquadPosition::SP_Beta),
            FSwatCommand(FText::FromStringTable("SwatCommandTable", "Charlie"), ESwatCommand::SC_SwapWithCharlie,
                         TargetCharacter,
                         true, SquadPosition != ESquadPosition::SP_Charlie && MaxOverrideSquadPosition >=
                         ESquadPosition::SP_Charlie),
            FSwatCommand(FText::FromStringTable("SwatCommandTable", "Delta"), ESwatCommand::SC_SwapWithDelta,
                         TargetCharacter, true,
                         SquadPosition != ESquadPosition::SP_Delta && MaxOverrideSquadPosition >=
                         ESquadPosition::SP_Delta),
        };

        TArray<FSwatCommand> FinalSwapCommands;
        FSwatCommand SwapCommand; //3

        if (bIsStackUp)
        {
            TArray<FSwatCommand> CurrentSideCommands;
            if (MaxCurrentSideSquadPosition != ESquadPosition::SP_NONE)
            {
                for (uint8 i = 0; i <= static_cast<uint8>(MaxCurrentSideSquadPosition); i++)
                {
                    CurrentSideCommands.Add(CurrentCommandTable[i]);
                }
            }

            FinalSwapCommands = CurrentSideCommands;

            if (bIsSplit)
            {
                TArray<FSwatCommand> OppositeSideCommands;

                if (!IsGamePad)
                {
                    const FSwatCommand LeftCommandTable[4] =
                    {
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToAlphaLeft"),
                            ESwatCommand::SC_SwapWithAlphaOpposite,
                            TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToBravoLeft"),
                            ESwatCommand::SC_SwapWithBetaOpposite,
                            TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToCharlieLeft"),
                            ESwatCommand::SC_SwapWithCharlieOpposite, TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToDeltaLeft"),
                            ESwatCommand::SC_SwapWithDeltaOpposite,
                            TargetCharacter, true, true)
                    };

                    const FSwatCommand RightCommandTable[4] =
                    {
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToAlphaRight"),
                            ESwatCommand::SC_SwapWithAlphaOpposite,
                            TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToBravoRight"),
                            ESwatCommand::SC_SwapWithBetaOpposite,
                            TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToCharlieRight"),
                            ESwatCommand::SC_SwapWithCharlieOpposite, TargetCharacter, true, true),
                        FSwatCommand(
                            FText::FromStringTable("SwatCommandTable", "MoveToDeltaRight"),
                            ESwatCommand::SC_SwapWithDeltaOpposite,
                            TargetCharacter, true, true)
                    };

                    if (MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE)
                    {
                        for (uint8 i = 0; i <= static_cast<uint8>(MaxOppositeSideSquadPosition); i++)
                        {
                            if (bCurrentIsLeft)
                            {
                                OppositeSideCommands.Add(RightCommandTable[i]);
                            }
                            else
                            {
                                OppositeSideCommands.Add(LeftCommandTable[i]);
                            }
                        }
                    }
                }

                FinalSwapCommands += OppositeSideCommands;

                if (CurrentSideCommands.Num() == 1 && OppositeSideCommands.Num() == 1)
                {
                    FText CommandText = FText::Format(FText::FromStringTable("SwatCommandTable", "SwapWithName"),
                                                      OppositeSideCommands[0].CommandText);
                    SwapCommand = FSwatCommand(CommandText, OppositeSideCommands[0].CommandType,
                                               false,
                                               bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 &&
                                               FinalSwapCommands.Num() > 0, true);
                }
                else
                {
                    SwapCommand = FSwatCommand(
                        FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::PC_SwapWith,
                        FinalSwapCommands,
                        bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1,
                        true);
                }
            }
            else
            {
                SwapCommand = FSwatCommand(
                    FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::PC_SwapWith,
                    FinalSwapCommands,
                    bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1, true);
            }
        }
        else
        {
            for (uint8 i = 0; i < 4; i++)
            {
                FinalSwapCommands.Add(CurrentCommandTable[i]);
            }

            SwapCommand = FSwatCommand(
                FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::PC_SwapWith, FinalSwapCommands,
                bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1, true);
        }

        const FVector Location = ContextualData.Location + ContextualData.ImpactNormal * 50.0f;

        bool bIsOutside = false;
        bool bHasTAA = false;
        if (const AThreatAwarenessActor* TAA = UThreatAwarenessSubsystem::Get(World)->GetNearestThreatForLocation(
            Location, 500.0f, 200.0f, true))
        {
            bIsOutside = TAA->bIsOutside;
            bHasTAA = true;
        }

        FSwatCommand SearchCommand = FSwatCommand(
            bIsOutside || !bHasTAA
                ? FText::FromStringTable("SwatCommandTable", "SearchArea")
                : FText::FromStringTable("SwatCommandTable", "SearchRoom"),
            ESwatCommand::SC_SearchAndSecureRoom_Individual, true, !bIsClearing);
        //Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "SearchRoom"), ESwatCommand::SC_SearchAndSecureRoom_Individual, true, !bIsClearing));

        if (TargetCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Shield)) // TODO???
        {
            bool bHasShieldEquipped = TargetCharacter->GetInventoryComponent()->GetEquippedItem<ABallisticsShield>() !=
                nullptr;

            Commands.Add(FSwatCommand(
                bHasShieldEquipped
                    ? FText::FromStringTable("SwatCommandTable", "HolsterShield")
                    : FText::FromStringTable("SwatCommandTable", "DeployShield"),
                bHasShieldEquipped ? ESwatCommand::SC_HolsterShield : ESwatCommand::SC_DeployShield, TargetCharacter,
                false, true));
        }

        if (IsGamePad)
        {
            Commands = {
                MoveIndividualCommand,
                FocusIndividualCommand,
                SearchCommand,
                SwapCommand
            };
        }
    }
    else if ((TargetCharacter->CanArrest() || TargetCharacter->IsArrested()))
    {
        int32 NumPlayers = World->GetGameState<AReadyOrNotGameState>()->GetNumPlayers();
        bool bSwatAvailable = SwatManager->GetSWATCount() != 0 || !SwatManager->IsSWATTeamDead() || NumPlayers > 1;
        bool bIsMoving = TargetCharacter->IsArrestedOrSurrendered() && TargetCharacter->bIsMoving;
        bool bIsUnresponsive = TargetCharacter->IsDeadOrUnconscious() || TargetCharacter->IsPlayingDead() ||
            TargetCharacter->IsInRagdoll();
        const FVector DirectionToInstigator = (LocalPlayerCharacter->GetActorLocation() - TargetCharacter->
            GetActorLocation()).GetSafeNormal();
        const float Dot = FVector::DotProduct(TargetCharacter->GetActorForwardVector(), DirectionToInstigator);
        const bool bIsFacingPlayer = Dot > 0.25f;

        FSwatCommand MoveCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Move..."),
                                                ESwatCommand::PC_Move,
                                                {
                                                    FSwatCommand(
                                                        FText::FromStringTable("SwatCommandTable", "Here"),
                                                        ESwatCommand::SC_MoveTo_Individual,
                                                        TargetCharacter, true),
                                                    FSwatCommand(
                                                        FText::FromStringTable("SwatCommandTable", "MyPosition"),
                                                        ESwatCommand::SC_MoveTo_MyPosition_Individual, TargetCharacter,
                                                        true),
                                                    FSwatCommand(
                                                        FText::FromStringTable("SwatCommandTable", "Stop"),
                                                        ESwatCommand::SC_MoveTo_Stop_Individual,
                                                        TargetCharacter, true, bIsMoving),
                                                }, TargetCharacter->IsArrestedOrSurrendered() && !bIsUnresponsive,
                                                true);

        FSwatCommand ArrestCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Restrain"),
                                                  ESwatCommand::SC_DoArrestTarget, false,
                                                  TargetCharacter->CanArrest());
        FSwatCommand TurnAroundCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "TurnAround"), ESwatCommand::SC_TurnAround_Individual,
            TargetCharacter, true,
            TargetCharacter->CanArrest() && bIsFacingPlayer && !bIsUnresponsive);
        FSwatCommand MoveToExitCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "MoveToExit"), ESwatCommand::SC_MoveTo_Exit_Individual,
            TargetCharacter,
            true, TargetCharacter->IsCivilian() && TargetCharacter->CanArrest() && !bIsUnresponsive);

        FSwatCommand DeployTaserCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "DeployTaser"), ESwatCommand::SC_DeployTaser,
            TargetCharacter, false,
            TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(
                ActiveTeamType, EItemCategory::IC_Taser));
        FSwatCommand DeployPeppersprayCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "DeployPepperspray"),
            ESwatCommand::SC_DeployPepperspray, TargetCharacter, true,
            TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(
                ActiveTeamType, EItemCategory::IC_OCSpray));
        FSwatCommand DeployPepperballCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "DeployPepperball"),
            ESwatCommand::SC_DeployPepperball, TargetCharacter, true,
            TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(
                ActiveTeamType, EItemCategory::IC_Pepperball));
        FSwatCommand DeployBeanbagCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "DeployBeanbag"), ESwatCommand::SC_DeployBeanbag,
            TargetCharacter, true,
            TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(
                ActiveTeamType, EItemCategory::IC_Beanbag));
        FSwatCommand MeleeTargetCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "MeleeTarget"), ESwatCommand::SC_MeleeTarget,
            TargetCharacter, true, TargetCharacter->IsActive());

        TArray<FSwatCommand> DeployCommandSubCommands = IsGamePad
                                                            ? TArray<FSwatCommand>{
                                                                DeployTaserCommand,
                                                                DeployPeppersprayCommand,
                                                                DeployPepperballCommand,
                                                                MeleeTargetCommand,
                                                                DeployBeanbagCommand,
                                                            }
                                                            : TArray<FSwatCommand>{
                                                                DeployTaserCommand,
                                                                DeployPeppersprayCommand,
                                                                DeployPepperballCommand,
                                                                DeployBeanbagCommand,
                                                                MeleeTargetCommand
                                                            };

        FSwatCommand DeployCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Deploy..."),
                                                  ESwatCommand::PC_Deploy,
                                                  {
                                                      DeployCommandSubCommands
                                                  }, TargetCharacter->IsActive() && bSwatAvailable && !bIsUnresponsive,
                                                  true);

        if (IsGamePad)
        {
            Commands = {
                MoveCommand,
                DeployCommand,
                TurnAroundCommand,
                MoveToExitCommand,
                ArrestCommand
            };
        }
        else
        {
            Commands = {
                MoveCommand,
                ArrestCommand,
                TurnAroundCommand,
                MoveToExitCommand,
                DeployCommand
            };
        }
    }

    if (Commands.Num() != 0)
    {
        CommandHistory.Empty();
        CommandHistory.Add(Commands);
    }
}

bool UReadyOrNotCommandFunctionLibrary::IsOtherTeamStackedUpOnDoor(ADoor* Door) const
{
    // gold overwrites so ignore this
    if (ActiveTeamType == ETeamType::TT_SQUAD)
    {
        return false;
    }
    for (TActorIterator<ASWATCharacter> It(World); It; ++It)
    {
        ASWATCharacter* Swat = *It;
        if (Swat->GetCyberneticsController())
        {
            UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<
                UTeamStackUpActivity>();
            if (StackUpActivity)
            {
                if (!StackUpActivity->GetStackUpDoor())
                {
                    continue;
                }

                if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() ==
                    Door)
                {
                    if (ActiveTeamType == ETeamType::TT_SERT_BLUE && StackUpActivity->SharedData->CommandTeam ==
                        ETeamType::TT_SERT_RED)
                    {
                        return true;
                    }
                    if (ActiveTeamType == ETeamType::TT_SERT_RED && StackUpActivity->SharedData->CommandTeam ==
                        ETeamType::TT_SERT_BLUE)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void UReadyOrNotCommandFunctionLibrary::DoCommand(FSwatCommand Command,
                                                  bool bFromQueue, ETeamType ActiveTeamOverride,
                                                  FHitResult ContextualDataOverride, bool bOverrideContextualData)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));

    if (ActiveTeamOverride != ETeamType::TT_NONE)
    {
        ExecutionTeamType = ActiveTeamOverride;
    }

    if (bOverrideContextualData)
    {
        ContextualData = ContextualDataOverride;
    }
    else
    {
        ContextualData = ExecutingContextualData;
    }

    AActor* ContextualActor = ContextualData.GetActor();
    if (ContextualActor)
    {
        V_LOGM(LogReadyOrNot, "Executing command on %s (Override: %d)", *ContextualActor->GetName(),
               bOverrideContextualData);
    }

    if (ADoor* Door = Cast<ADoor>(ContextualActor))
    {
        if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
        {
            Door = Door->GetSubDoor();
            ContextualActor = Door;
        }
    }

    if (Command.CommandTarget)
    {
        ContextualActor = Command.CommandTarget;
    }


    // Move the location away from the wall/floor it hits so LOS required for command location activities work
    const FVector Location = ContextualData.Location + ContextualData.ImpactNormal * 50.0f;
    ContextualData.Location = Location;

    DrawDebugPoint(World, Location, 15.0f, FColor::Green, false, 10.0f);

    LastExecutedCommandContextualData = ContextualData;

    if (SwatManager)
    {
        SwatManager->TimeSincePlayerIssuedCommand = 0.0f;
    }

    ACyberneticCharacter* ContextAI = Cast<ACyberneticCharacter>(ContextualActor);

    switch (Command.CommandType)
    {
    case ESwatCommand::SC_MoveTo: SwatManager->GiveMoveCommand(ExecutionTeamType, Location);
        break;
    case ESwatCommand::SC_FallIn: SwatManager->GiveFallInCommand(ExecutionTeamType);
        break;
    case ESwatCommand::SC_FallIn_Snake: SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Snake);
        break;
    case ESwatCommand::SC_FallIn_HalfSnake: SwatManager->
            GiveFallInCommand(ExecutionTeamType, EFallInPattern::HalfSnake);
        break;
    case ESwatCommand::SC_FallIn_Diamond: SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Diamond);
        break;
    case ESwatCommand::SC_FallIn_Flock: SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Flock);
        break;
    case ESwatCommand::SC_Cover: SwatManager->GiveCoverAreaCommand(ExecutionTeamType, Location);
        break;
    case ESwatCommand::SC_Hold: SwatManager->GiveHoldCommand(ExecutionTeamType);
        break;
    case ESwatCommand::SC_Resume: SwatManager->RemoveHoldCommand(ExecutionTeamType);
        break;
    case ESwatCommand::SC_DeployFlashbang: SwatManager->GiveDeployGrenadeAtLocation(
            ExecutionTeamType, ContextualData.Location, Flashbang);
        break;
    case ESwatCommand::SC_DeployStinger: SwatManager->GiveDeployGrenadeAtLocation(
            ExecutionTeamType, ContextualData.Location, Stinger);
        break;
    case ESwatCommand::SC_DeployCSGas: SwatManager->GiveDeployGrenadeAtLocation(
            ExecutionTeamType, ContextualData.Location, CSGas);
        break;
    case ESwatCommand::SC_DeployChemlight: SwatManager->GiveDropChemlightAtLocation(
            ExecutionTeamType, ContextualData.Location);
        break;
    case ESwatCommand::SC_DoCollectEvidence: SwatManager->
            GiveCollectEvidenceCommand(ContextualActor, ExecutionTeamType);
        break;
    case ESwatCommand::SC_DoArrestTarget: SwatManager->
            GiveRestrainCommand(ContextualActor, ExecutionTeamType, Location);
        break;
    case ESwatCommand::SC_DoReportTarget: SwatManager->GiveReportTargetCommand(ContextualActor, ExecutionTeamType);
        break;
    case ESwatCommand::SC_DisarmStandaloneTrap: SwatManager->GiveDisarmStandaloneTrapCommand(
            ContextualActor, ExecutionTeamType);
        break;
    case ESwatCommand::SC_KillMe:
        UAchievementStatics::UnlockAchievement(World, EAchievement::THE_DEVIL, false);
        SwatManager->GiveMoveCommand(ExecutionTeamType, Location);
        break;
    case ESwatCommand::SC_StackUp: SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType,
                                                                   ContextualData.Location, ContextualData.ImpactNormal,
                                                                   true);
        break;
    case ESwatCommand::SC_StackUpSplit: SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType,
                                                                        ContextualData.Location,
                                                                        ContextualData.ImpactNormal, true,
                                                                        EStackUpStyle::Split);
        break;
    case ESwatCommand::SC_StackUpLeft: SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType,
                                                                       ContextualData.Location,
                                                                       ContextualData.ImpactNormal, true,
                                                                       EStackUpStyle::Left);
        break;
    case ESwatCommand::SC_StackUpRight: SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType,
                                                                        ContextualData.Location,
                                                                        ContextualData.ImpactNormal, true,
                                                                        EStackUpStyle::Right);
        break;
    case ESwatCommand::SC_PickLock: SwatManager->GivePickLockCommand(ContextualActor, ExecutionTeamType,
                                                                     ContextualData.Location);
        break;
    case ESwatCommand::SC_RemoveDoorJam: SwatManager->GiveRemoveWedgeCommand(
            ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal);
        break;
    case ESwatCommand::SC_DeployMirrorgun: SwatManager->GiveCheckForContactsCommand(
            ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal);
        break;
    case ESwatCommand::SC_DeployDoorJam: SwatManager->GiveWedgeDoorCommand(
            ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal);
        break;
    case ESwatCommand::SC_CheckForTrap: SwatManager->GiveCheckForTrapsCommand(
            ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal);
        break;
    case ESwatCommand::SC_DisarmTrap: SwatManager->GiveDisarmTrapOnDoorCommand(
            ContextualActor, ExecutionTeamType, ContextualData.Location);
        break;
    case ESwatCommand::SC_CloseDoor: SwatManager->GiveCloseDoorCommand(ContextualActor, ExecutionTeamType,
                                                                       ContextualData.Location);
        break;
    case ESwatCommand::SC_OpenDoor: SwatManager->GiveOpenDoorCommand(ContextualActor, ExecutionTeamType,
                                                                     ContextualData.Location);
        break;
    case ESwatCommand::SC_Slide: SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType,
                                                                  ContextualData.Location, EDoorScanMethod::Slide);
        break;
    case ESwatCommand::SC_Slice: SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType,
                                                                  ContextualData.Location, EDoorScanMethod::Slice);
        break;
    case ESwatCommand::SC_Snap: SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType,
                                                                 ContextualData.Location, EDoorScanMethod::Snap);
        break;
    case ESwatCommand::SC_SearchAndSecure: SwatManager->GiveSearchAndSecureCommand(
            ExecutionTeamType, ContextualData.Location);
        break;
    case ESwatCommand::SC_SearchAndSecureRoom: SwatManager->GiveSearchAndSecureCommand(
            ExecutionTeamType, ContextualData.Location, true);
        break;
    case ESwatCommand::SC_SearchAndSecureRoom_Individual: SwatManager->GiveSearchAndSecureCommand_Individual(
            ContextualActor, ContextualData.Location, true);
        break;
    case ESwatCommand::SC_MoveAndClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, nullptr);
        break;
    case ESwatCommand::SC_MoveAndClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, Flashbang);
        break;
    case ESwatCommand::SC_MoveAndClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, Stinger);
        break;
    case ESwatCommand::SC_MoveAndClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, CSGas);
        break;
    case ESwatCommand::SC_MoveAndClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass());
        break;
    case ESwatCommand::SC_MoveAndClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_OpenAndClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, nullptr);
        break;
    case ESwatCommand::SC_OpenAndClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, Flashbang);
        break;
    case ESwatCommand::SC_OpenAndClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, Stinger);
        break;
    case ESwatCommand::SC_OpenAndClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, CSGas);
        break;
    case ESwatCommand::SC_OpenAndClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass());
        break;
    case ESwatCommand::SC_OpenAndClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_KickAndClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, nullptr);
        break;
    case ESwatCommand::SC_KickAndClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, Flashbang);
        break;
    case ESwatCommand::SC_KickAndClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, Stinger);
        break;
    case ESwatCommand::SC_KickAndClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, CSGas);
        break;
    case ESwatCommand::SC_KickAndClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass());
        break;
    case ESwatCommand::SC_KickAndClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_ShotgunClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, nullptr,
            ABreachingShotgun::StaticClass());
        break;
    case ESwatCommand::SC_ShotgunClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location,
            Flashbang, ABreachingShotgun::StaticClass());
        break;
    case ESwatCommand::SC_ShotgunClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, Stinger,
            ABreachingShotgun::StaticClass());
        break;
    case ESwatCommand::SC_ShotgunClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, CSGas,
            ABreachingShotgun::StaticClass());
        break;
    case ESwatCommand::SC_ShotgunClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass(), ABreachingShotgun::StaticClass());
        break;
    case ESwatCommand::SC_ShotgunClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_RamAndClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, nullptr,
            ADoorRam::StaticClass());
        break;
    case ESwatCommand::SC_RamAndClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, Flashbang,
            ADoorRam::StaticClass());
        break;
    case ESwatCommand::SC_RamAndClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, Stinger,
            ADoorRam::StaticClass());
        break;
    case ESwatCommand::SC_RamAndClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, CSGas,
            ADoorRam::StaticClass());
        break;
    case ESwatCommand::SC_RamAndClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass(), ADoorRam::StaticClass());
        break;
    case ESwatCommand::SC_RamAndClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_C2Clear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, nullptr,
            AC2Explosive::StaticClass());
        break;
    case ESwatCommand::SC_C2ClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, Flashbang,
            AC2Explosive::StaticClass());
        break;
    case ESwatCommand::SC_C2ClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, Stinger,
            AC2Explosive::StaticClass());
        break;
    case ESwatCommand::SC_C2ClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, CSGas,
            AC2Explosive::StaticClass());
        break;
    case ESwatCommand::SC_C2ClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass(), AC2Explosive::StaticClass());
        break;
    case ESwatCommand::SC_C2ClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, false, true);
        break;
    case ESwatCommand::SC_LeaderAndClear: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, true, false);
        break;
    case ESwatCommand::SC_LeaderAndClearFlashbang: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location,
            Flashbang, nullptr, true, false);
        break;
    case ESwatCommand::SC_LeaderAndClearStinger: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, Stinger,
            nullptr, true, false);
        break;
    case ESwatCommand::SC_LeaderAndClearCSGas: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, CSGas,
            nullptr, true, false);
        break;
    case ESwatCommand::SC_LeaderAndClearLauncher: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location,
            AGrenadeLauncher::StaticClass(), nullptr, true, false);
        break;
    case ESwatCommand::SC_LeaderAndClearLeader: SwatManager->GiveBreachAndClearCommand(
            Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, nullptr,
            nullptr, true, true);
        break;
    case ESwatCommand::SC_DeployTaser: SwatManager->GiveDeployNonLethalItemAtTargetCommand(
            Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Taser);
        break;
    case ESwatCommand::SC_DeployPepperspray: SwatManager->GiveDeployNonLethalItemAtTargetCommand(
            Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_OCSpray);
        break;
    case ESwatCommand::SC_DeployPepperball: SwatManager->GiveDeployNonLethalItemAtTargetCommand(
            Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Pepperball);
        break;
    case ESwatCommand::SC_DeployBeanbag: SwatManager->GiveDeployNonLethalItemAtTargetCommand(
            Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Beanbag);
        break;
    case ESwatCommand::SC_MeleeTarget: SwatManager->GiveDeployNonLethalItemAtTargetCommand(
            Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_None);
        break;
    case ESwatCommand::SC_DeployShield:
    case ESwatCommand::SC_HolsterShield:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (ACyberneticCharacter* Character = Controller->GetCharacter())
                {
                    if (Character->GetInventoryComponent()->IsEquippingItem())
                    {
                        break;
                    }

                    if (Character->GetInventoryComponent()->GetEquippedItem<ABallisticsShield>())
                    {
                        Character->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Primary);
                    }
                    else
                    {
                        Character->GetInventoryComponent()->EquipItemOfClass(ABallisticsShield::StaticClass());
                    }
                }
            }
        }
        else
        {
            SwatManager->GiveDeployShield(ActiveTeamType);
        }
        break;
    case ESwatCommand::SC_Focus_Individual:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                Controller->GetTargetingComp()->CustomFocusActor = nullptr;
                Controller->GetTargetingComp()->CustomFocusLocation = ContextualData.Location;
                ContextAI->PlayRawVO(FMath::RandBool()
                                         ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                         : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
            }
        }
        break;
    case ESwatCommand::SC_Focus_MyPosition_Individual:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
                Controller->GetTargetingComp()->CustomFocusActor = nullptr;
                Controller->GetTargetingComp()->CustomFocusLocation = LocalPlayer->GetActorLocation();
                ContextAI->PlayRawVO(FMath::RandBool()
                                         ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                         : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
            }
        }
        break;
    case ESwatCommand::SC_UnFocus_Individual:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                Controller->GetTargetingComp()->CustomFocusActor = nullptr;
                Controller->GetTargetingComp()->CustomFocusLocation = FVector::ZeroVector;

                ContextAI->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_FORGOT);
            }
        }
        break;
    case ESwatCommand::SC_FocusDoor_Individual:

        if (ContextAI && Cast<ADoor>(Command.CommandTarget2))
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                Controller->GetTargetingComp()->CustomFocusActor = Command.CommandTarget2;
            }
        }
        break;
    case ESwatCommand::SC_FocusTarget_Individual:
        if (ContextAI && Command.CommandTarget2)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                Controller->GetTargetingComp()->CustomFocusActor = Command.CommandTarget2;

                ContextAI->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_FOCUS);
            }
        }
        break;
    case ESwatCommand::SC_SwapWithAlpha:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Alpha, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithAlphaOpposite:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Alpha, true, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithBeta:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Beta, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithBetaOpposite:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Beta, true, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithCharlie:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Charlie, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithCharlieOpposite:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Charlie, true, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithDelta:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Delta, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_SwapWithDeltaOpposite:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionWith(ESquadPosition::SP_Delta, true, true);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_MoveToAlpha:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionTo(ESquadPosition::SP_Alpha);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_MoveToBeta:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionTo(ESquadPosition::SP_Beta);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_MoveToCharlie:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionTo(ESquadPosition::SP_Charlie);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_MoveToDelta:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                if (UTeamStackUpActivity* Activity = Controller->GetCurrentActivity<UTeamStackUpActivity>())
                {
                    Activity->SwapSquadPositionTo(ESquadPosition::SP_Delta);

                    ContextAI->PlayRawVO(FMath::RandBool()
                                             ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC
                                             : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
                }
            }
        }
        break;
    case ESwatCommand::SC_MoveTo_Individual: // todo: dont allow this if swat is moving in to arrest them
        if (Player)
        {
            Player->Server_GiveAIMoveTo(ContextAI, Location);
        }
        break;
    case ESwatCommand::SC_MoveTo_MyPosition_Individual:
        if (Player)
        {
            Player->Server_GiveAIMoveTo(ContextAI, Player->GetNavAgentLocation());
        }
        break;
    case ESwatCommand::SC_MoveToAndBack_Individual:
        if (ContextAI)
        {
            if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
            {
                Controller->GiveMoveTo(Location, false);

                ContextAI->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_MOVE_TO);
            }
        }
        break;

    case ESwatCommand::SC_MoveTo_Stop_Individual:
        if (Player)
        {
            Player->Server_StopAIMoveTo(ContextAI);
        }
        break;

    case ESwatCommand::SC_MoveTo_Exit_Individual:
        if (Player)
        {
            Player->Server_GiveAIMoveToExit(ContextAI);
        }
        break;
    case ESwatCommand::SC_TurnAround_Individual:
        if (Player)
        {
            Player->Server_GiveAITurnAroundOrder(ContextAI);
        }
        break;
    case ESwatCommand::SC_None: break;
    default: break;
    }

    // TODO:
    // OnSwatCommandIssued.Broadcast(Command.CommandType, ActiveTeamOverride, ContextualActor);
}

bool UReadyOrNotCommandFunctionLibrary::DoesSwatTeamHaveItem(ETeamType SwatTeam,
                                                             TSubclassOf<ABaseItem> Item)
{
    if (!SwatManager)
    {
        return false;
    }

    return SwatManager->GetSwatWithItem(SwatTeam, Item) != nullptr;
}

bool UReadyOrNotCommandFunctionLibrary::DoesSwatTeamHaveItemType(ETeamType SwatTeam,
                                                                 EItemCategory ItemType)
{
    if (!SwatManager)
    {
        return false;
    }

    return SwatManager->GetSwatWithItemType(SwatTeam, ItemType) != nullptr;
}

bool UReadyOrNotCommandFunctionLibrary::DoesAnySwatTeamHaveItem(TSubclassOf<ABaseItem> Item)
{
    for (TActorIterator<ASWATCharacter> It(World); It; ++It)
    {
        ASWATCharacter* Swat = *It;
        if (Swat->IsDeadOrUnconscious())
        {
            continue;
        }

        if (Swat->GetInventoryComponent()->GetInventoryItemOfClass(Item))
        {
            return true;
        }
    }
    return false;
}

bool UReadyOrNotCommandFunctionLibrary::DoesAnySwatTeamHaveItemType(EItemCategory ItemType)
{
    for (TActorIterator<ASWATCharacter> It(World); It; ++It)
    {
        ASWATCharacter* Swat = *It;
        if (Swat->IsDeadOrUnconscious())
        {
            continue;
        }

        if (Swat->GetInventoryComponent()->GetInventoryItemOfType(ItemType))
        {
            return true;
        }
    }
    return false;
}

void UReadyOrNotCommandFunctionLibrary::GrabContextData(bool IgnoreCharacters,
                                                        FCollisionQueryParams CollisionQueryParams)
{
    AReadyOrNotGameState* GameState = World ? World->GetGameState<AReadyOrNotGameState>() : nullptr;
    if (GameState == nullptr)
    {
        return;
    }

    APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
    if (!LocalPlayerCharacter || !LocalPlayerCharacter->GetFirstPersonCameraComponent())
    {
        return;
    }

    if (CollisionQueryParams.OwnerTag == FCollisionQueryParams::DefaultQueryParam.OwnerTag)
    {
        CollisionQueryParams = LocalPlayerCharacter->GetCollisionQueryParameters();
    }

    CollisionQueryParams.bTraceComplex = true;

    if (GameState)
    {
        for (const AReadyOrNotCharacter* Character : GameState->AllReadyOrNotCharacters)
        {
            //CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)Character->GetInventoryComponent()->GetInventoryItems());

            if (IgnoreCharacters) // ignore everyone if we're in a submenu
            {
                CollisionQueryParams.AddIgnoredActor(Character);
                continue;
            }

            if ((Character->IsDeadOrUnconscious() || Character->IsIncapacitated()) && (Character->IsArrested() ||
                Character->IsArrestedAndDead()))
            {
                CollisionQueryParams.AddIgnoredActor(Character);
                continue;
            }

            if (!Character->IsActive() && Character->IsOnSWATTeam())
            {
                CollisionQueryParams.AddIgnoredActor(Character);
            }
        }
    }

    if (Cast<ASWATCharacter>(LastActorBeforeGoingIntoSubPage) && IgnoreCharacters)
    {
        CollisionQueryParams.AddIgnoredActor(LastActorBeforeGoingIntoSubPage);
    }

    if (SwatManager)
    {
        CollisionQueryParams.AddIgnoredActors(static_cast<TArray<AActor*>>(SwatManager->SwatTrailers));
    }

    const FVector TraceStart = LocalPlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
    const FVector TraceEnd = TraceStart + LocalPlayerCharacter->GetFirstPersonCameraComponent()->GetForwardVector() *
        10000.0f;

    FCollisionObjectQueryParams CollisionObjectQueryParams;
    CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
    CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
    CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
    CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
    CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_ITEM);

    // Multi line trace to support targeting specific actors through open doors and doorways
    TArray<FHitResult> HitResults;
    World->LineTraceMultiByObjectType(HitResults, TraceStart, TraceEnd, CollisionObjectQueryParams,
                                      CollisionQueryParams);

    // Remove all volumes
    HitResults.RemoveAll([&](const FHitResult& Element)
    {
        if (Cast<AVolume>(Element.GetActor()))
        {
            return true;
        }

        return false;
    });

    // If first hit is something that a command can't be issued on (wall, etc..), use it. We dont want to target command actors through walls and such
    if (HitResults.Num() > 0)
    {
        if ((!HitResults[0].GetActor() && HitResults[0].bBlockingHit) || (HitResults[0].GetActor() && !HitResults[0].
            GetActor()->Implements<UCanIssueCommandOn>()))
        {
            ContextualData = HitResults[0];
            return;
        }
    }

    /*
    HitResults.RemoveAll([&](const FHitResult& Element)
    {
        if (const ABaseItem* Item = Cast<ABaseItem>(Element.GetActor()))
        {
            return Item->bInInventory;
        }

        return false;
    });
    */

    // remove all doors that are close to the player
    HitResults.RemoveAll([&](const FHitResult& Element)
    {
        if (ADoor* FoundDoor = Cast<ADoor>(Element.GetActor()))
        {
            if (!IsTeamStackedUpOnDoor(FoundDoor))
            {
                if (FoundDoor->IsDoorwayOnly() || FoundDoor->IsOpenAtOrBeyond(0.75f))
                {
                    const float Distance = FVector::Distance(
                        FoundDoor->CalculateClosestPoint(LocalPlayerCharacter->GetActorLocation()),
                        LocalPlayerCharacter->GetActorLocation());

                    if (Distance < 100.0f)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    });

    uint8 NonCommandableIndex = 0;
    FHitResult* NonCommandableHit = nullptr;
    for (FHitResult& Hit : HitResults)
    {
        if ((!Hit.GetActor() && Hit.bBlockingHit) || // BSP brushes
            (Hit.GetActor() && !Hit.GetActor()->Implements<UCanIssueCommandOn>()))
        // walls, floors, anything static that blocks
        {
            NonCommandableHit = &Hit;
            break;
        }

        NonCommandableIndex++;
    }

    FHitResult* FoundDoorHitResult = HitResults.FindByPredicate([&](const FHitResult& Element)
    {
        if (const ADoor* FoundDoor = Cast<ADoor>(Element.GetActor()))
        {
            if (Element.GetActor()->Implements<UCanIssueCommandOn>() &&
                ICanIssueCommandOn::Execute_CanIssueCommand(Element.GetActor()))
            {
                return true;
            }
        }

        return false;
    });

    // if found a non commandable actor lower in the array memory than the door, exit early and use the wall hit
    if (NonCommandableHit && FoundDoorHitResult)
    {
        if (NonCommandableHit < FoundDoorHitResult)
        {
            ContextualData = *NonCommandableHit;
            return;
        }
    }

    FHitResult* FoundDoorHitResult2 = nullptr;

    // find the second door (only if looking through a doorway)
    if (FoundDoorHitResult)
    {
        if (ADoor* Door = Cast<ADoor>(FoundDoorHitResult->GetActor()))
        {
            if (FoundDoorHitResult->GetComponent() == Door->GetDoorway())
            {
                if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
                {
                    FoundDoorHitResult->HitObjectHandle = Door->GetSubDoor();
                }

                const bool bCanTechnicallySee = !Door->IsDoorwayOnly() && Door->IsOpenAtOrBeyond(0.75f) && !Door->
                    IsActorRightOfDoorway(LocalPlayerCharacter);

                if (Door->IsDoorwayOnly() || bCanTechnicallySee)
                {
                    if (HitResults.Num() > 1 && HitResults[1].GetActor())
                    {
                        if (ADoor* FoundDoor = Cast<ADoor>(HitResults[1].GetActor()))
                        {
                            if (FoundDoor != Door && FoundDoor != Door->GetSubDoor())
                            {
                                if (HitResults[1].GetActor()->Implements<UCanIssueCommandOn>() &&
                                    ICanIssueCommandOn::Execute_CanIssueCommand(HitResults[1].GetActor()))
                                {
                                    FoundDoorHitResult2 = &HitResults[1];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Find non-door actors that commands can be issued on
    FHitResult* FoundTargetHitResult = HitResults.FindByPredicate([&](const FHitResult& Element)
    {
        if (Element.GetActor() && !Cast<ADoor>(Element.GetActor()))
        {
            if (Element.GetActor()->Implements<UCanIssueCommandOn>() && ICanIssueCommandOn::Execute_CanIssueCommand(
                Element.GetActor()))
            {
                FHitResult HitResult;
                CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
                CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
                CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
                CollisionObjectQueryParams.RemoveObjectTypesToQuery(ECC_DOORWAY);
                World->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, CollisionObjectQueryParams,
                                                   CollisionQueryParams);

                /*
                ULog::Info("tracing to " + Element.GetActor()->GetName() + ". hit: " + (HitResult.bBlockingHit ? "true" : "false"));
                ULog::ObjectName(HitResult.GetActor());
                ULog::ObjectName(HitResult.GetComponent());
                if (HitResult.GetActor())
                {
                    ULog::Bool(HitResult.GetActor() == Element.GetActor(), "hit actor: ");
                }
                */

                return HitResult.GetActor() == Element.GetActor();
            }
        }

        return false;
    });

    if (!FoundTargetHitResult)
    {
        if (HitResults.Num() > 1)
        {
            if (ABaseItem* Item = Cast<ABaseItem>(HitResults[0].GetActor()))
            {
                if (Item->GetOwnerCharacter() && Item->bInInventory)
                {
                    for (uint8 i = 1; i < HitResults.Num(); i++)
                    {
                        if (ABaseItem* OtherItem = Cast<ABaseItem>(HitResults[i].GetActor()))
                        {
                            if (OtherItem->GetOwnerCharacter() && OtherItem->bInInventory)
                            {
                            }
                        }
                        else
                        {
                            if (Cast<AReadyOrNotCharacter>(HitResults[i].GetActor()))
                            {
                                FoundTargetHitResult = &HitResults[i];
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    // if found a non commandable actor lower in the array memory than the target character, exit early and use the wall hit
    if (NonCommandableHit && FoundTargetHitResult)
    {
        if (NonCommandableHit < FoundTargetHitResult)
        {
            ContextualData = *NonCommandableHit;
            return;
        }
    }

    // LastContextActor = ContextualData.GetActor();

    if (!FoundTargetHitResult)
    {
        if (FoundDoorHitResult)
        {
            ContextualData = *FoundDoorHitResult;
        }
        else
        {
            if (HitResults.Num() > 0)
            {
                ContextualData = HitResults[0];
            }
            else
            {
                ContextualData = FHitResult();
            }
        }

        if (FoundDoorHitResult2)
        {
            ContextualData2 = *FoundDoorHitResult2;
        }
        else
        {
            ContextualData2 = FHitResult();
        }
    }
    else
    {
        ContextualData = *FoundTargetHitResult;
    }
}

void UReadyOrNotCommandFunctionLibrary::ExecuteCommand(const FSwatCommand Command, bool bFromDefault, bool bOnlyVO)
{
    AReadyOrNotGameState* GameState = World ? World->GetGameState<AReadyOrNotGameState>() : nullptr;

    if (Command.bGrabContextualDataOnExecute)
    {
        APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
        FCollisionQueryParams QueryParams = LocalPlayer
                                                ? LocalPlayer->GetCollisionQueryParameters()
                                                : FCollisionQueryParams::DefaultQueryParam;
        QueryParams.OwnerTag = "CommandWidget";

        const bool bIgnoreAllDoors = Cast<ASWATCharacter>(Command.CommandTarget) != nullptr;

        if (bIgnoreAllDoors && GameState)
        {
            QueryParams.AddIgnoredActors(static_cast<TArray<AActor*>>(GameState->AllDoors));
        }

        GrabContextData(CommandHistory.Num() > 1, QueryParams);
    }

    ExecutingContextualData = Command.bUseSecondaryContextData ? ContextualData2 : ContextualData;
    if (ContextualData.GetActor())
    {
        V_LOGM(LogReadyOrNot, "Executing command (Target: %s, Grab Contextual Data: %d)",
               *ContextualData.GetActor()->GetName(), Command.bGrabContextualDataOnExecute);
    }

    ExecutionTeamType = ActiveTeamType;

    // bHoldPageUntilExecuted = false; // TODO: Check with SwatCommandWidget
    // UFMODBlueprintStatics::PlayEvent2D(this, ExecuteCommandEvent, true);

    bool bIsFemaleTarget = false;
    if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(ExecutingContextualData.GetActor()))
    {
        bIsFemaleTarget = AI->bFemale;
    }

    AActor* ContextualActor = ExecutingContextualData.GetActor();

    if (ADoor* Door = Cast<ADoor>(ContextualActor))
    {
        if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
        {
            Door = Door->GetSubDoor();
            ContextualActor = Door;
        }
    }

    if (Command.CommandTarget)
    {
        ContextualActor = Command.CommandTarget;
    }

    const bool bIsSwat = Cast<ASWATCharacter>(ContextualActor) != nullptr;

    APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));

    TArray<UTeamBreachAndClearActivity*> OutBreachAndClearActivities;
    GetExecutingBreachAndClearActivities(OutBreachAndClearActivities);

    PlayVoiceCommand(Command, bIsSwat, bIsFemaleTarget);

    switch (Command.CommandType)
    {
    case ESwatCommand::SC_None: break;

    case ESwatCommand::SC_KillMe:
        SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_KILL_ME);
        // No need to do anything else, they will kill you anyway
        return;
    case ESwatCommand::SC_Execute:
        {
            FQueuedSwatCommand* ActiveTeam_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ActiveTeamType);
            FQueuedSwatCommand* Red_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SERT_RED);
            FQueuedSwatCommand* Blue_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.
                                                                      Find(ETeamType::TT_SERT_BLUE);

            if (ActiveTeamType == ETeamType::TT_SQUAD)
            {
                if (ActiveTeam_QueuedSwatCommand)
                {
                    DoCommand(ActiveTeam_QueuedSwatCommand->Command, true, ETeamType::TT_SQUAD,
                              ActiveTeam_QueuedSwatCommand->ContextualData, true);

                    SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);
                    SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
                    SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
                }
                else if (Red_QueuedSwatCommand && Blue_QueuedSwatCommand)
                {
                    DoCommand(Blue_QueuedSwatCommand->Command, true, ETeamType::TT_SERT_BLUE,
                              Blue_QueuedSwatCommand->ContextualData, true);
                    DoCommand(Red_QueuedSwatCommand->Command, true, ETeamType::TT_SERT_RED,
                              Red_QueuedSwatCommand->ContextualData, true);

                    SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
                    SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
                }
            }
            else
            {
                if (ActiveTeam_QueuedSwatCommand)
                {
                    DoCommand(ActiveTeam_QueuedSwatCommand->Command, true, ActiveTeamType,
                              ActiveTeam_QueuedSwatCommand->ContextualData, true);
                    SwatManager->QueuedSwatCommandMap.Remove(ActiveTeamType);
                }
            }
        }
        break;
    case ESwatCommand::SC_Cancel:

        if (ActiveTeamType == ETeamType::TT_SQUAD)
        {
            SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);
            SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
            SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
        }
        else
        {
            SwatManager->QueuedSwatCommandMap.Remove(ActiveTeamType);
        }
        break;
    default: break;
    }

    // OnCommandIssued(Command.Index, Command, bFromDefault); // TODO: Check with SwatCommandWidget

    if (!bOnlyVO)
    {
        float Delay = 1.0f;
        if (SwatManager->SquadLeader && !Cast<ASWATCharacter>(SwatManager->SquadLeader))
        {
            if (SwatManager->SquadLeader->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_RED_TEAM ||
                SwatManager->SquadLeader->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_BLUE_TEAM ||
                SwatManager->SquadLeader->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_GOLD_TEAM)
            {
                Delay *= 1.5f;
            }
            else
            {
                if (const USoundSource* Sound = SwatManager->SquadLeader->VoiceSoundSource)
                {
                    Delay = Sound->GetCurrentSoundLength();
                }
            }
        }

        if (Delay <= 0.0f)
        {
            Delay = 1.0f;
        }

        // Allow multiple timers just store this temp, we want every queued command to execute
        FTimerHandle TH_RespondToSwatCommand;
        World->GetTimerManager().SetTimer(TH_RespondToSwatCommand,
                                          FTimerDelegate::CreateUObject(
                                              this, &UReadyOrNotCommandFunctionLibrary::RespondToSWATCommand,
                                              Command, ActiveTeamType, ContextualData), Delay,
                                          false);
    }

    // CloseCommandMenu(); // TODO: Check with SwatCommandWidget
}


void UReadyOrNotCommandFunctionLibrary::PlayVoiceCommand(FSwatCommand Command,
                                                         bool bIsSwat, bool bIsFemaleTarget)
{
    AActor* ContextualActor = ContextualData.GetActor();

    switch (Command.CommandType)
    {
    case ESwatCommand::SC_None: break;
    case ESwatCommand::SC_Roger: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND,
            "", false);
        break;
    case ESwatCommand::SC_Negative: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC, "", false);
        break;
    case ESwatCommand::SC_MoveTo: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_TO);
        break;
    case ESwatCommand::SC_MoveTo_Individual: SwatManager->PlaySwatCommandVoiceLine(
            bIsSwat ? VO_SWAT_COMMAND::CALL_SC_MOVE_TO : VO_SWAT_GENERAL::CALL_ORDER_MOVE, "", false);
        break;
    case ESwatCommand::SC_MoveTo_Exit_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_GENERAL::CALL_EXIT, "", false);
        break;
    case ESwatCommand::SC_FallIn: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN);
        break;
    case ESwatCommand::SC_FallIn_Snake: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN);
        break;
    case ESwatCommand::SC_FallIn_HalfSnake: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN);
        break;
    case ESwatCommand::SC_FallIn_Diamond: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_FALL_IN_DIAMOND : VO_SWAT_COMMAND::CALL_SC_FALL_IN);
        break;
    case ESwatCommand::SC_FallIn_Flock: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN);
        break;
    case ESwatCommand::SC_Cover: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_COVER);
        break;
    case ESwatCommand::SC_Hold: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_HOLD);
        break;
    case ESwatCommand::SC_Resume: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RESUME);
        break;
    case ESwatCommand::SC_DeployFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DEPLOY_FLASHBANG);
        break;
    case ESwatCommand::SC_DeployStinger: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_STINGER);
        break;
    case ESwatCommand::SC_DeployCSGas: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_CSGAS);
        break;
    case ESwatCommand::SC_DeployChemlight: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DEPLOY_CHEMLIGHT);
        break;
    case ESwatCommand::SC_DeployShield: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_SHIELD);
        break;
    case ESwatCommand::SC_HolsterShield: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RESUME);
        break;
    case ESwatCommand::SC_DoCollectEvidence: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_COLLECT_EVIDENCE);
        break;
    case ESwatCommand::SC_DoArrestTarget:
        {
            FString VO;
            if (bIsFemaleTarget)
            {
                VO = FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_ARREST_FEMALE : VO_SWAT_COMMAND::CALL_SC_ARREST;
            }
            else
            {
                VO = FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_ARREST_MALE : VO_SWAT_COMMAND::CALL_SC_ARREST;
            }

            SwatManager->PlaySwatCommandVoiceLine(VO);
        }
        break;
    case ESwatCommand::SC_DoReportTarget: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DO_REPORT_TARGET);
        break;
    case ESwatCommand::SC_DisarmStandaloneTrap: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DISARM_TRAP);
        return;
    case ESwatCommand::SC_KillMe:
        SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_KILL_ME);
        break;
    case ESwatCommand::SC_StackUp: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP);
        break;
    case ESwatCommand::SC_StackUpSplit:
        {
            if (IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)))
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_SPLIT, "", false);
            }
            else
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_SPLIT);
            }
        }
        break;
    case ESwatCommand::SC_StackUpLeft:
        {
            if (IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)))
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_LEFT, "", false);
            }
            else
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_LEFT);
            }
        }
        break;
    case ESwatCommand::SC_StackUpRight:
        {
            if (IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)))
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_RIGHT, "", false);
            }
            else
            {
                SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP_RIGHT);
            }
        }
        break;
    case ESwatCommand::SC_PickLock: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_PICK_LOCK, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RemoveDoorJam: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_REMOVE_DOOR_JAM, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_DeployMirrorgun: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DEPLOY_MIRRORGUN, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_DeployDoorJam: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DEPLOY_DOOR_JAM, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_CheckForTrap: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_CHECK_FOR_TRAP, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_DisarmTrap: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_DISARM_TRAP, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_CloseDoor: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_CLOSE_DOOR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenDoor: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_DOOR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_Slide: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_SLIDE, "",
                                                                       !IsTeamStackedUpOnDoor(
                                                                           Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_Slice: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_PIE, "",
                                                                       !IsTeamStackedUpOnDoor(
                                                                           Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_Snap: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_PEEK, "",
                                                                      !IsTeamStackedUpOnDoor(
                                                                          Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_SearchAndSecure: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE);
        break;
    case ESwatCommand::SC_SearchAndSecureRoom: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE);
        break;
    case ESwatCommand::SC_MoveAndClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_MoveAndClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_MoveAndClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_MoveAndClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_MoveAndClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_MoveAndClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_OpenAndClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_KickAndClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_ShotgunClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_RamAndClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2Clear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2ClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2ClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2ClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2ClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_C2ClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LEADER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClear: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClearFlashbang: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_FLASHBANG, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClearStinger: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_STINGER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClearCSGas: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_CSGAS, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClearLauncher: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_LAUNCHER, "",
            !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_LeaderAndClearLeader: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor)));
        break;
    case ESwatCommand::SC_SwapWithAlpha: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithBeta: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithCharlie: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithDelta: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithAlphaOpposite: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithBetaOpposite: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithCharlieOpposite: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_SwapWithDeltaOpposite: SwatManager->PlaySwatCommandVoiceLine(
            FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA : VO_SWAT_COMMAND::CALL_SC_SWAP);
        break;
    case ESwatCommand::SC_MoveToAlpha: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA);
        break;
    case ESwatCommand::SC_MoveToBeta: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA);
        break;
    case ESwatCommand::SC_MoveToCharlie: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE);
        break;
    case ESwatCommand::SC_MoveToDelta: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA);
        break;
    case ESwatCommand::SC_DeployTaser: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_TASER);
        break;
    case ESwatCommand::SC_DeployPepperspray: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_SPRAY);
        break;
    case ESwatCommand::SC_DeployPepperball: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_PEPPER);
        break;
    case ESwatCommand::SC_DeployBeanbag: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_BEANBAG);
        break;
    case ESwatCommand::SC_MeleeTarget: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL);
        break;
    case ESwatCommand::SC_Execute: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_EXECUTE);
        break;
    case ESwatCommand::SC_Cancel: SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_CANCEL);
        break;
    case ESwatCommand::SC_SearchAndSecureRoom_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE, "", false);
        break;
    case ESwatCommand::SC_MoveToAndBack_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_MOVE_TO, "", false);
        break;
    case ESwatCommand::SC_MoveTo_MyPosition_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_GENERAL::CALL_ORDER_MOVE_TO_ME, "", false);
        break;
    case ESwatCommand::SC_MoveTo_Stop_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_HOLD, "", false);
        break;
    case ESwatCommand::SC_TurnAround_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_SC_TURN_AROUND, "", false);
        break;
    case ESwatCommand::SC_Focus_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_FOCUS, "", false);
        break;
    case ESwatCommand::SC_Focus_MyPosition_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_FOCUS, "", false);
        break;
    case ESwatCommand::SC_FocusDoor_Individual:
        {
            bool bIsDoorwayOrOpenThreshold = false;
            bool bIsHallway = false;
            if (const ADoor* Door = Cast<ADoor>(Command.CommandTarget2))
            {
                bIsDoorwayOrOpenThreshold = Door->IsDoorwayOnly() || Door->IsOpenBeyondIncrementThreshold();

                if (Door->IsPointInFrontOfDoorway(ExecutingContextualData.Location))
                {
                    bIsHallway = Door->BackRoomPosition == EDoorRoomPosition::Hallway ||
                        Door->BackRoomPosition == EDoorRoomPosition::HallwayLeft ||
                        Door->BackRoomPosition == EDoorRoomPosition::HallwayRight;
                }
                else
                {
                    bIsHallway = Door->FrontRoomPosition == EDoorRoomPosition::Hallway ||
                        Door->FrontRoomPosition == EDoorRoomPosition::HallwayLeft ||
                        Door->FrontRoomPosition == EDoorRoomPosition::HallwayRight;
                }
            }

            FString VO = VO_SWAT_COMMAND::CALL_FOCUS_DOOR;
            if (bIsDoorwayOrOpenThreshold)
            {
                if (bIsHallway)
                {
                    VO = VO_SWAT_COMMAND::CALL_FOCUS_HALLWAY;
                }
                else
                {
                    VO = VO_SWAT_COMMAND::CALL_FOCUS_OPENING;
                }
            }

            SwatManager->PlaySwatCommandVoiceLine(VO, "", false);
        }
        break;
    case ESwatCommand::SC_FocusTarget_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_FOCUS_TARGET, "", false);
        break;
    case ESwatCommand::SC_UnFocus_Individual: SwatManager->PlaySwatCommandVoiceLine(
            VO_SWAT_COMMAND::CALL_UNFOCUS, "", false);
        break;
    default: break;
    }
}


void UReadyOrNotCommandFunctionLibrary::GetExecutingBreachAndClearActivities(
    TArray<UTeamBreachAndClearActivity*>& OutActivities)
{
    for (TActorIterator<ASWATCharacter> It(World); It; ++It)
    {
        ASWATCharacter* Swat = *It;
        if (Swat->GetCyberneticsController())
        {
            UTeamBreachAndClearActivity* BreachAndClearActivity = Swat->GetCyberneticsController()->GetCurrentActivity<
                UTeamBreachAndClearActivity>();
            if (BreachAndClearActivity)
            {
                if (BreachAndClearActivity->GetCharacter()->GetTeam() == ActiveTeamType || ActiveTeamType ==
                    ETeamType::TT_SQUAD)
                {
                    OutActivities.Add(BreachAndClearActivity);
                }
            }
        }
    }
}

void UReadyOrNotCommandFunctionLibrary::QueueCommand(FSwatCommand Command)
{
    FQueuedSwatCommand QueuedSwatCommand;
    QueuedSwatCommand.ContextualData = ContextualData;
    QueuedSwatCommand.Command = Command;
    QueuedSwatCommand.Command.bGrabContextualDataOnExecute = false;
    SwatManager->QueuedSwatCommandMap.Add(ActiveTeamType, QueuedSwatCommand);

    // Means we have broken up the teams into two queues, remove gold and transfer it to the opposite team that this widget is active on
    if (ActiveTeamType != ETeamType::TT_SQUAD)
    {
        ETeamType OppositeActiveTeam = (ActiveTeamType == ETeamType::TT_SERT_RED
                                            ? ETeamType::TT_SERT_BLUE
                                            : ETeamType::TT_SERT_RED);
        FQueuedSwatCommand* CachedQueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SQUAD);
        if (CachedQueuedSwatCommand)
        {
            SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);

            SwatManager->QueuedSwatCommandMap.Add(OppositeActiveTeam, *CachedQueuedSwatCommand);
        }
    }

    // Stack up on door if queuing a breaching command
    if (Command.CommandType == ESwatCommand::SC_OpenAndClear || Command.CommandType ==
        ESwatCommand::SC_OpenAndClearFlashbang || Command.CommandType == ESwatCommand::SC_OpenAndClearStinger || Command
        .CommandType == ESwatCommand::SC_OpenAndClearCSGas || Command.CommandType ==
        ESwatCommand::SC_OpenAndClearLauncher || Command.CommandType == ESwatCommand::SC_OpenAndClearLeader ||
        Command.CommandType == ESwatCommand::SC_MoveAndClear || Command.CommandType ==
        ESwatCommand::SC_MoveAndClearFlashbang || Command.CommandType == ESwatCommand::SC_MoveAndClearStinger || Command
        .CommandType == ESwatCommand::SC_MoveAndClearCSGas || Command.CommandType ==
        ESwatCommand::SC_MoveAndClearLauncher || Command.CommandType == ESwatCommand::SC_MoveAndClearLeader ||
        Command.CommandType == ESwatCommand::SC_KickAndClear || Command.CommandType ==
        ESwatCommand::SC_KickAndClearFlashbang || Command.CommandType == ESwatCommand::SC_KickAndClearStinger || Command
        .CommandType == ESwatCommand::SC_KickAndClearCSGas || Command.CommandType ==
        ESwatCommand::SC_KickAndClearLauncher || Command.CommandType == ESwatCommand::SC_KickAndClearLeader ||
        Command.CommandType == ESwatCommand::SC_C2Clear || Command.CommandType == ESwatCommand::SC_C2ClearFlashbang ||
        Command.CommandType == ESwatCommand::SC_C2ClearStinger || Command.CommandType == ESwatCommand::SC_C2ClearCSGas
        || Command.CommandType == ESwatCommand::SC_C2ClearLauncher || Command.CommandType ==
        ESwatCommand::SC_C2ClearLeader ||
        Command.CommandType == ESwatCommand::SC_ShotgunClear || Command.CommandType ==
        ESwatCommand::SC_ShotgunClearFlashbang || Command.CommandType == ESwatCommand::SC_ShotgunClearStinger || Command
        .CommandType == ESwatCommand::SC_ShotgunClearCSGas || Command.CommandType ==
        ESwatCommand::SC_ShotgunClearLauncher || Command.CommandType == ESwatCommand::SC_ShotgunClearLeader ||
        Command.CommandType == ESwatCommand::SC_RamAndClear || Command.CommandType ==
        ESwatCommand::SC_RamAndClearFlashbang || Command.CommandType == ESwatCommand::SC_RamAndClearStinger || Command.
        CommandType == ESwatCommand::SC_RamAndClearCSGas || Command.CommandType == ESwatCommand::SC_RamAndClearLauncher
        || Command.CommandType == ESwatCommand::SC_RamAndClearLeader ||
        Command.CommandType == ESwatCommand::SC_LeaderAndClear || Command.CommandType ==
        ESwatCommand::SC_LeaderAndClearFlashbang || Command.CommandType == ESwatCommand::SC_LeaderAndClearStinger ||
        Command.CommandType == ESwatCommand::SC_LeaderAndClearCSGas || Command.CommandType ==
        ESwatCommand::SC_LeaderAndClearLauncher || Command.CommandType == ESwatCommand::SC_LeaderAndClearLeader)
    {
        FSwatCommand StackupCommand = FSwatCommand(
            FText::FromStringTable("SwatCommandTable", "StackUp"), ESwatCommand::SC_StackUp, nullptr, false, true);
        StackupCommand.InputKey = GetInputOne();

        ExecuteCommand(StackupCommand, false,
                       IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualData.GetActor())));

        // OnCommandIssued(Command.Index, Command, false); // TODO: Check with SwatCommandWidget
    }

    // TODO is SwatManager->GetLeadCharacter() really what we want?
    UpdateCommandPageData(SwatManager->GetSquadLeader());
}

void UReadyOrNotCommandFunctionLibrary::CancelCommand()
{
    if(CommandHistory.Num() > 1)
    {
        CommandHistory.Pop();
        LastSubCommandPageIndex = FMath::Clamp(LastSubCommandPageIndex - 1, 0, LastSubCommandPageIndex);
    }
}

void UReadyOrNotCommandFunctionLibrary::UpdateCommandPageData(APawn* OwningPlayer)
{
    APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(OwningPlayer);

    if(CommandHistory.Num()>1)
    {
        return;
    }
    
    if (!LocalPlayerCharacter)
    {
        return;
    }

    if (!SwatManager)
    {
        return;
    }

    bool bHadIndividualCommands = bIndividualCommands;
    bIndividualCommands = false;
    
    if (SwatManager->IsSWATTeamDead())
    {
        //bOverrideActiveTeamType = true;
        ActiveTeamType = ETeamType::TT_NONE;
        SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;
        SwatManager->CurrentDefaultCommand = ESwatCommand::SC_None;

        LocalPlayerCharacter->bLookingAtTarget = false;
        LocalPlayerCharacter->bLookingAtHuman = false;
        LocalPlayerCharacter->bLookingAtDoor = false;
        LocalPlayerCharacter->bLookingAtEvidenceItem = false;

        return;
    }

    if ((ActiveTeamType == ETeamType::TT_SQUAD && (SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_RED) || SwatManager->
            IsSWATTeamDead(ETeamType::TT_SERT_BLUE))) ||
        (ActiveTeamType == ETeamType::TT_SERT_RED && SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_RED)) ||
        (ActiveTeamType == ETeamType::TT_SERT_BLUE && SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_BLUE)))
    {
        CycleSwatElement(true);
    }

    if (SwatManager->IsCharacterKnownEnemy(LocalPlayerCharacter))
    {
        SwatManager->CurrentDefaultCommand = ESwatCommand::SC_KillMe;
        //OverrideActiveTeamType = ETeamType::TT_NONE;
        GetCurrentCommandOptions().Empty();

        CommandHistory = TArray<TArray<FSwatCommand>>{
            TArray<FSwatCommand>{
                FSwatCommand(FText::FromStringTable("SwatCommandTable", "KillMe"), ESwatCommand::SC_KillMe,
                             LocalPlayerCharacter, false)
            }
        };

        return;
    }

    if (HasQueuedCommandForTeam(ActiveTeamType))
    {
        GrabContextData(CommandHistory.Num() > 1);
        if (ContextualData.GetActor())
        {
            if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(ContextualData.GetActor()))
            {
                LocalPlayerCharacter->bLookingAtTarget = true;
                LocalPlayerCharacter->bLookingAtHuman = true;
                BuildIndividualCommands(AICharacter);
                return;
            }
        }
        
        BuildQueuedPageData();
        if (SwatManager)
        {
            SwatManager->CurrentDefaultCommand = ESwatCommand::SC_Execute;
        }
        return;
    }


    GrabContextData(CommandHistory.Num() > 1);

    // TODO: Is if starting at row 3687 in SwatCommandWidget.cpp necessary?

    if (CommandHistory.Num() > 1)
    {
        return;
    }

    UBpGameplayHelperLib::LoadDefaultCommands(DefaultCommand/*, DefaultHumanCommand*/, DefaultDoorUnknownCommand,
                                              DefaultDoorOpenCommand, DefaultDoorLockedCommand,
                                              DefaultDoorUnlockedCommand);

    SwatManager->CurrentDefaultCommand = DefaultCommand;

    LocalPlayerCharacter->bLookingAtTarget = false;
    LocalPlayerCharacter->bLookingAtHuman = false;
    LocalPlayerCharacter->bLookingAtDoor = false;

    if (!ContextualData.GetActor())
    {
        BuildDefaultPageData();
        return;
    }

    if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(ContextualData.GetActor()))
    {
        LocalPlayerCharacter->bLookingAtTarget = true;
        LocalPlayerCharacter->bLookingAtHuman = true;
        BuildIndividualCommands(AICharacter);
    }
    else if (ADoor* Door = Cast<ADoor>(ContextualData.GetActor()))
    {
        if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
        {
            Door = Door->GetSubDoor();
        }

        const bool bPlayerInFront = Door->IsPointInFrontOfDoorway(LocalPlayerCharacter->GetActorLocation());
        const bool bCanIssueOrderOnThisSide = (Door->bCanIssueOrdersOnFrontSide && bPlayerInFront) || (Door->
            bCanIssueOrdersOnBackSide && !bPlayerInFront);
        if (bCanIssueOrderOnThisSide)
        {
            LocalPlayerCharacter->bLookingAtTarget = true;
            LocalPlayerCharacter->bLookingAtDoor = true;

            const bool bDoorUnknownToSWAT = !Door->GetSWATKnowsLockState();
            const bool bDoorIsLocked = Door->GetSWATKnowsLockState() && Door->IsLocked();
            const bool bDoorCanCheck = bDoorUnknownToSWAT && IsTeamStackedUpOnDoor(Door);

            const bool bTrapInFront = Door->GetAttachedTrap()
                                          ? Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation())
                                          : false;
            const bool bSameSideAsTrap = bTrapInFront == bPlayerInFront;

            const bool bCanDisarmTrap = Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus ==
                ETrapState::TS_Live;
            const bool bCanGiveDisarmTrapCommand = bCanDisarmTrap && (bSameSideAsTrap || (!bSameSideAsTrap && (Door->
                IsOpen() || Door->TeamKnowsDoorTrapState(false))));

            const ESwatCommand DoorCommand = (bCanGiveDisarmTrapCommand
                                                  ? ESwatCommand::SC_DisarmTrap
                                                  : Door->IsDoorwayOnly()
                                                  ? ESwatCommand::SC_MoveAndClear
                                                  : (Door->IsOpen()
                                                         ? DefaultDoorOpenCommand
                                                         : bDoorCanCheck
                                                         ? DefaultCheckDoorCommand
                                                         : bDoorUnknownToSWAT
                                                         ? DefaultDoorUnknownCommand
                                                         : (bDoorIsLocked
                                                                ? DefaultDoorLockedCommand
                                                                : DefaultDoorUnlockedCommand)));

            if (SwatManager)
                SwatManager->CurrentDefaultCommand = DoorCommand;

            ADoor* OtherDoor = Cast<ADoor>(ContextualData2.GetActor());

            TArray<FSwatCommand> DoorCommands;
            TArray<FSwatCommand> OtherDoorCommands;
            TArray<FSwatCommand> FinalCommands;
            BuildDoorPageData(Door, DoorCommands);
            BuildDoorPageData(OtherDoor, OtherDoorCommands);

            RecursiveSet(OtherDoorCommands);

            if (IsValid(OtherDoor))
            {
                if (Door->IsDoorwayOnly())
                {
                    FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Doorway..."),
                                                   ESwatCommand::PC_DoorWay, DoorCommands));
                }
                else
                {
                    FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Door..."),
                                                   ESwatCommand::PC_Door, DoorCommands));
                }

                if (OtherDoor->IsDoorwayOnly())
                {
                    FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "OtherDoorway..."),
                                                   ESwatCommand::PC_OtherDoorWay, OtherDoorCommands));
                }
                else
                {
                    FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "OtherDoor..."),
                                                   ESwatCommand::PC_OtherDoor, OtherDoorCommands));
                }
            }
            else
            {
                FinalCommands = DoorCommands;
            }

            if (FinalCommands.Num() > 0)
            {
                CommandHistory.Empty();
                CommandHistory.Add(FinalCommands);
            }
        }
    }
    else if (AIncapacitatedHuman* IncapacitatedHuman = Cast<AIncapacitatedHuman>(ContextualData.GetActor()))
    {
        if (!IncapacitatedHuman->HasBeenReported())
        {
            SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoReportTarget;
        }
    }
    else if (ATrapActor* TrapActor = Cast<ATrapActor>(ContextualData.GetActor()))
    {
        if (!TrapActor->IsHidden())
        {
            SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DisarmStandaloneTrap;
        }
    }
    else if (ABaseItem* BaseItem = Cast<ABaseItem>(ContextualData.GetActor()))
    {
        if (BaseItem->IsEvidence() && !BaseItem->IsHidden())
        {
            SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoCollectEvidence;

            LocalPlayerCharacter->bLookingAtEvidenceItem = true;
        }
    }
    else if (AThrownEvidenceActor* ThrownEvidence = Cast<AThrownEvidenceActor>(ContextualData.GetActor()))
    {
        if (ThrownEvidence->OwningItem)
        {
            if (ThrownEvidence->OwningItem->IsEvidence() && !ThrownEvidence->OwningItem->IsHidden())
            {
                SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoCollectEvidence;

                LocalPlayerCharacter->bLookingAtEvidenceItem = true;
            }
        }
    }
    else if (AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(ContextualData.GetActor()))
    {
        if (!EvidenceActor->IsHidden())
        {
            SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoCollectEvidence;

            LocalPlayerCharacter->bLookingAtEvidenceItem = true;
        }
    }

    if ((CommandHistory.Num() == 1 && bHadIndividualCommands && !bIndividualCommands))
    {
        CommandHistory.Empty();
    }

    if (CommandHistory.Num() == 0 || CommandHistory[CommandHistory.Num() - 1].Num() == 0)
    {
        BuildDefaultPageData();
    }
}


void UReadyOrNotCommandFunctionLibrary::CycleSwatElement(bool bNext)
{
    if (SwatManager->IsSWATTeamDead())
    {
        ActiveTeamType = ETeamType::TT_NONE;
        SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;

        // CloseCommandMenu();

        return;
    }

    ETeamType PreviousTeamType = ActiveTeamType;

    TArray<ETeamType> SwatElements = {ETeamType::TT_SERT_RED, ETeamType::TT_SERT_BLUE, ETeamType::TT_SQUAD};
    for (int32 i = 0; i < SwatElements.Num(); i++)
    {
        if (SwatManager->IsSWATTeamDead(SwatElements[i]))
        {
            SwatElements.RemoveAt(i);
            if (i > 0)
            {
                i--;
            }
        }
    }

    int32 Idx;
    if (SwatElements.Find(ActiveTeamType, Idx))
    {
        if (bNext)
        {
            if (SwatElements.IsValidIndex(Idx + 1))
            {
                ActiveTeamType = SwatElements[Idx + 1];
            }
            else
            {
                ActiveTeamType = SwatElements[0];
            }
        }
        else
        {
            if (SwatElements.IsValidIndex(Idx - 1))
            {
                ActiveTeamType = SwatElements[Idx - 1];
            }
            else
            {
                ActiveTeamType = SwatElements[SwatElements.Num() - 1];
            }
        }
    }
    else
    {
        if (SwatElements.Contains(ETeamType::TT_SQUAD))
        {
            ActiveTeamType = ETeamType::TT_SQUAD;
        }
        else
        {
            ActiveTeamType = ETeamType::TT_NONE;
        }
    }

    if (PreviousTeamType != ActiveTeamType)
    {
        Reset();
    }

    SetActiveTeamElement(ActiveTeamType);
}

void UReadyOrNotCommandFunctionLibrary::SetActiveTeamElement(ETeamType TeamType)
{
    ActiveTeamType = TeamType;
    if (ActiveTeamType == ETeamType::TT_NONE)
    {
        SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;

        // CloseCommandMenu();

        return;
    }

    // TODO
    // UReadyOrNotFunctionLibrary::ResumeCallbackTimer(this, TH_UpdatePageData);

    SwatManager->ActiveCommandTeam = ActiveTeamType;

    // TODO is SwatManager->GetLeadCharacter() really what we want?
    UpdateCommandPageData(SwatManager->GetSquadLeader());
}

void UReadyOrNotCommandFunctionLibrary::SetLastCommandPage(TArray<FSwatCommand>& InCommands)
{
    for (int32 i = 0; i < InCommands.Num(); i++)
    {
        InCommands[i].InputKey = ConvertIntToInputKey(i + 1);
    }
}

FKey UReadyOrNotCommandFunctionLibrary::ConvertIntToInputKey(const int32 Int)
{
    switch (Int)
    {
    case 1: return GetInputOne();
    case 2: return GetInputTwo();
    case 3: return GetInputThree();
    case 4: return GetInputFour();
    case 5: return GetInputFive();
    case 6: return GetInputSix();
    default: EKeys::Invalid;
    }

    return EKeys::Invalid;
}

TArray<FSwatCommand> UReadyOrNotCommandFunctionLibrary::GetCurrentCommandOptions()
{
    if (CommandHistory.Num() < 1)
    {
        return TArray<FSwatCommand>();
    }
    return CommandHistory[CommandHistory.Num() - 1];
}

TArray<FSwatCommand> UReadyOrNotCommandFunctionLibrary::GetPreviousCommandOptions()
{
    if (CommandHistory.Num() < 2)
    {
        return TArray<FSwatCommand>();
    }
    return CommandHistory[CommandHistory.Num() - 1];
}

bool UReadyOrNotCommandFunctionLibrary::HasQueuedCommandForTeam(ETeamType TeamType)
{
    if (SwatManager->IsCharacterKnownEnemy(SwatManager->GetSquadLeader()))
    {
        return true;
    }

    bool bHasRedTeamCommand = false;
    bool bHasBlueTeamCommand = false;
    bool bHasGoldTeamCommand = false;
    for (auto& k : SwatManager->QueuedSwatCommandMap)
    {
        if (k.Key == ETeamType::TT_SERT_RED)
        {
            bHasRedTeamCommand = true;
        }
        if (k.Key == ETeamType::TT_SERT_BLUE)
        {
            bHasBlueTeamCommand = true;
        }
        if (k.Key == ETeamType::TT_SQUAD)
        {
            bHasGoldTeamCommand = true;
        }
    }
    // if both blue and red have queued commands then gold does
    if (bHasRedTeamCommand && bHasBlueTeamCommand)
    {
        bHasGoldTeamCommand = true;
    }

    switch (TeamType)
    {
    case ETeamType::TT_SERT_RED:
        return bHasRedTeamCommand;
    case ETeamType::TT_SERT_BLUE:
        return bHasBlueTeamCommand;
    case ETeamType::TT_SQUAD:
        return bHasGoldTeamCommand;
    default: ;
    }
    return false;
}

bool UReadyOrNotCommandFunctionLibrary::IsTeamBreachingDoor(ADoor* Door, ETeamType SwatTeam)
{
    if (SwatManager)
    {
        for (ASWATCharacter* Swat : SwatManager->SwatAI)
        {
            if (SwatManager->IsSWATValid(Swat) && (Swat->GetTeam() == SwatTeam || SwatTeam == ETeamType::TT_SQUAD))
            {
                if (const UTeamBreachAndClearActivity* Activity = Swat->GetCyberneticsController()->GetCurrentActivity<
                    UTeamBreachAndClearActivity>())
                {
                    if (Activity->GetSharedData<FSharedBreachData>()->StackUpDoor == Door)
                    {
                        return Activity->GetSharedData<FSharedBreachData>()->bIsBreaching;
                    }
                }
            }
        }
    }

    return false;
}

bool UReadyOrNotCommandFunctionLibrary::IsTeamStackedUpOnDoor(ADoor* Door)
{
    for (TActorIterator<ASWATCharacter> It(World); It; ++It)
    {
        ASWATCharacter* Swat = *It;
        if (Swat->GetCyberneticsController())
        {
            UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<
                UTeamStackUpActivity>();
            if (StackUpActivity)
            {
                if (!StackUpActivity->GetStackUpDoor())
                {
                    continue;
                }

                if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() ==
                    Door)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool UReadyOrNotCommandFunctionLibrary::IsOtherTeamStackedUpOnDoor(ADoor* Door, ETeamType& OutTeam)
{
    OutTeam = ETeamType::TT_SQUAD;

    // gold overwrites so ignore this
    if (ActiveTeamType == ETeamType::TT_SQUAD)
    {
        return false;
    }

    if (SwatManager)
    {
        for (ASWATCharacter* Swat : SwatManager->SwatAI)
        {
            if (SwatManager->IsSWATValid(Swat))
            {
                if (UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<
                    UTeamStackUpActivity>())
                {
                    if (!StackUpActivity->GetStackUpDoor())
                    {
                        continue;
                    }

                    if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() ==
                        Door)
                    {
                        if (StackUpActivity->SharedData->CommandTeam == ETeamType::TT_SERT_RED)
                        {
                            OutTeam = ETeamType::TT_SERT_RED;
                            return true;
                        }

                        if (StackUpActivity->SharedData->CommandTeam == ETeamType::TT_SERT_BLUE)
                        {
                            OutTeam = ETeamType::TT_SERT_BLUE;
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}


bool UReadyOrNotCommandFunctionLibrary::IsTeamStackedUpOnDoorWithStyle(ETeamType SwatTeam, ADoor* Door,
                                                                       EStackUpStyle StackUpStyle,
                                                                       bool bPlayerInFront) const
{
    if (!SwatManager)
    {
        return false;
    }

    for (ASWATCharacter* Swat : SwatManager->SwatAI)
    {
        if (SwatManager->IsSWATValid(Swat) && (Swat->GetTeam() == SwatTeam || SwatTeam == ETeamType::TT_SQUAD))
        {
            if (const UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<
                UTeamStackUpActivity>())
            {
                if (!StackUpActivity->GetStackUpDoor())
                {
                    continue;
                }

                const bool bSameSide = Door->IsPointInFrontOfDoorway(StackUpActivity->SharedData->CommandLocation) ==
                    bPlayerInFront;
                if (!bSameSide)
                {
                    return false;
                }

                const bool bSameTeam = StackUpActivity->GetCharacter()->GetTeam() == ActiveTeamType || ActiveTeamType ==
                    ETeamType::TT_SQUAD;
                if (!bSameTeam)
                {
                    return false;
                }

                if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() ==
                    Door)
                {
                    if (StackUpActivity->SharedData->CommandTeam != SwatTeam)
                    {
                        return false;
                    }

                    uint8 NumOccupiedRight = 0;
                    uint8 NumOccupiedLeft = 0;
                    if (Door->IsPointInFrontOfDoorway(StackUpActivity->SharedData->CommandLocation))
                    {
                        for (AStackUpActor* StackUp : Door->FrontLeftStackUpPoints)
                        {
                            if (Cast<ACyberneticController>(StackUp->OccupiedBy))
                            {
                                NumOccupiedRight++;
                            }
                        }

                        for (AStackUpActor* StackUp : Door->FrontRightStackUpPoints)
                        {
                            if (Cast<ACyberneticController>(StackUp->OccupiedBy))
                            {
                                NumOccupiedLeft++;
                            }
                        }
                    }
                    else
                    {
                        for (AStackUpActor* StackUp : Door->BackRightStackUpPoints)
                        {
                            if (Cast<ACyberneticController>(StackUp->OccupiedBy))
                            {
                                NumOccupiedRight++;
                            }
                        }

                        for (AStackUpActor* StackUp : Door->BackLeftStackUpPoints)
                        {
                            if (Cast<ACyberneticController>(StackUp->OccupiedBy))
                            {
                                NumOccupiedLeft++;
                            }
                        }
                    }

                    bool bLeftFullyOccupied;
                    bool bRightFullyOccupied;

                    if (Door->IsPointInFrontOfDoorway(StackUpActivity->SharedData->CommandLocation))
                    {
                        bLeftFullyOccupied = Door->FrontRightStackUpPoints.Num() == NumOccupiedLeft;
                        bRightFullyOccupied = Door->FrontLeftStackUpPoints.Num() == NumOccupiedRight;
                    }
                    else
                    {
                        bLeftFullyOccupied = Door->BackLeftStackUpPoints.Num() == NumOccupiedLeft;
                        bRightFullyOccupied = Door->BackRightStackUpPoints.Num() == NumOccupiedRight;
                    }

                    if (StackUpStyle == EStackUpStyle::Split)
                    {
                        if (NumOccupiedLeft > 0 && NumOccupiedRight > 0)
                        {
                            if (NumOccupiedRight > NumOccupiedLeft)
                            {
                                if (bLeftFullyOccupied)
                                {
                                    return true;
                                }
                            }
                            else if (NumOccupiedLeft > NumOccupiedRight)
                            {
                                if (bRightFullyOccupied)
                                {
                                    return true;
                                }
                            }
                            else
                            {
                                if (bLeftFullyOccupied || bRightFullyOccupied)
                                {
                                    return true;
                                }
                            }

                            if (bLeftFullyOccupied && bRightFullyOccupied)
                            {
                                return true;
                            }
                        }
                    }
                    else if (StackUpStyle == EStackUpStyle::Right)
                    {
                        if (bRightFullyOccupied)
                        {
                            return true;
                        }
                    }
                    else if (StackUpStyle == EStackUpStyle::Left)
                    {
                        if (bLeftFullyOccupied)
                        {
                            return true;
                        }
                    }

                    if (NumOccupiedLeft > 0 || NumOccupiedRight > 0)
                    {
                        if (StackUpActivity->GetSharedData<FSharedStackUpData>()->StackUpStyle == StackUpStyle)
                        // todo: update shared data pointer, if we have half the team change commands if originally given a squad team command
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

void UReadyOrNotCommandFunctionLibrary::RespondToSWATCommand(FSwatCommand Command, ETeamType TeamType,
                                                             FHitResult CommandContextualData)
{
    DoCommand(Command, false, TeamType, CommandContextualData, true);
}

void UReadyOrNotCommandFunctionLibrary::SelectCommand(int32 index,
                                                      bool bAddToComboKeys)
{
    const TArray<FSwatCommand> CurrentCommandOptions = GetCurrentCommandOptions();
    if (CurrentCommandOptions.Num() < 1 || index < 0)
    {
        return;
    }
    const FSwatCommand SwatCommand = GetCurrentCommandOptions()[index];

    if (SwatCommand.bEnabled)
    {
        if (SwatCommand.SubCommands.Num() == 0)
        {
            SelectCommand(SwatCommand);
            return;
        }

        if (bAddToComboKeys)
        {
            CommandCombo.Add(FKey());
        }

        LastSubCommandPageIndex++;

        CommandHistory.Add(SwatCommand.SubCommands);
    }
}

void UReadyOrNotCommandFunctionLibrary::SelectCommand(const FSwatCommand& SwatCommand)
{
    GrabContextData(CommandHistory.Num() > 1);
    if (ShouldQueue)
    {
        QueueCommand(SwatCommand);
    }
    else
    {
        ExecuteCommand(SwatCommand, true, false);
    }
}

void UReadyOrNotCommandFunctionLibrary::InputKey(FKey Key,
                                                 bool bAddToComboKeys)
{
    // TODO
    // if (Key == GetInputBack())
    // {
    // 	if (LastSubCommandPageIndex > 0)
    // 	{
    // 		if (CommandCombo.Num() > 0)
    // 			CommandCombo.RemoveAt(CommandCombo.Num() - 1);

    // 		LastSubCommandPageIndex = FMath::Clamp(LastSubCommandPageIndex - 1, 0, LastSubCommandPageIndex);
    // 		GetCurrentCommandOptions() = PreviousGetCurrentCommandOptions();
    // 		
    // 		SetLastCommandPage(GetCurrentCommandOptions());

    // 		OnPageViewUpdate();
    // 		
    // 		UFMODBlueprintStatics::PlayEvent2D(this, OpenSubCommandMenuEvent, true);
    // 	}
    // 	return;
    // }

    for (FSwatCommand SwatCommand : GetCurrentCommandOptions())
    {
        if (Key == SwatCommand.InputKey && SwatCommand.bEnabled)
        {
            // only pause if the command is actually enabled.. otherwise it can lock on a disabled menu option
            // UReadyOrNotFunctionLibrary::PauseCallbackTimer(this, TH_UpdatePageData);

            GrabContextData(CommandHistory.Num() > 1);
            if (SwatCommand.SubCommands.Num() == 0)
            {
                if (UReadyOrNotStatics::GetReadyOrNotPlayerController()->IsInputKeyDown(
                    UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("HoldGoCode")))
                {
                    QueueCommand(SwatCommand);
                }
                else
                {
                    ExecuteCommand(SwatCommand, true, false);
                }
                break;
            }

            if (bAddToComboKeys)
            {
                CommandCombo.Add(Key);
            }

            GetCurrentCommandOptions() = SwatCommand.SubCommands;
            LastActorBeforeGoingIntoSubPage = ContextualData.GetActor();

            // UFMODBlueprintStatics::PlayEvent2D(this, OpenSubCommandMenuEvent, true);

            break;
        }
    }
}

FKey UReadyOrNotCommandFunctionLibrary::GetCommandInput(FName InName)
{
    UInputSettings* Settings = const_cast<UInputSettings*>(GetDefault<UInputSettings>());
    const TArray<FInputActionKeyMapping>& Axes = Settings->GetActionMappings();

    for (const FInputActionKeyMapping& Mapping : Axes)
    {
        if (Mapping.ActionName == InName)
        {
            return Mapping.Key;
        }
    }
    return FKey();
}

void UReadyOrNotCommandFunctionLibrary::Reset()
{
    bIndividualCommands = false;
    CommandHistory.Empty();
    LastSubCommandPageIndex = 0;
    GetCurrentCommandOptions() = TArray<FSwatCommand>();
    CommandCombo.Empty();
}

TArray<FSwatCommand> UReadyOrNotCommandFunctionLibrary::GetPostDoorCommands(ESwatCommand Clear,
                                                                            ESwatCommand ClearWithStinger,
                                                                            ESwatCommand ClearWithCSGas,
                                                                            ESwatCommand ClearWithLauncher,
                                                                            ESwatCommand ClearWithLeader,
                                                                            ESwatCommand ClearWithFlashbang)
{
    bool bLauncherHasAmmo = SwatManager->GetSWATCount() == 0;

    FSwatCommand ClearCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "Clear"),
        Clear, false, true, false, false);

    FSwatCommand ClearWithStingerCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "ClearWithStinger"),
        ClearWithStinger, false,
        DoesSwatTeamHaveItemType(
            ActiveTeamType, EItemCategory::IC_Stingball),
        false, false);

    FSwatCommand ClearWithCSGasCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"),
        ClearWithCSGas, false,
        DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas),
        false, false);

    FSwatCommand ClearWithLauncherCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"),
        ClearWithLauncher, false, bLauncherHasAmmo,
        false, false);

    FSwatCommand ClearWithLeaderCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "ClearWithLeader"),
        ClearWithLeader, false, true);

    FSwatCommand ClearWithFlashbangCommand = FSwatCommand(
        FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"),
        ClearWithFlashbang, false,
        DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang),
        false, false);

    if (IsGamePad)
    {
        return TArray<FSwatCommand>{
            ClearCommand,
            ClearWithStingerCommand,
            ClearWithCSGasCommand,
            ClearWithLauncherCommand,
            ClearWithLeaderCommand,
            ClearWithFlashbangCommand
        };
    }

    return TArray<FSwatCommand>{
        ClearCommand,
        ClearWithFlashbangCommand,
        ClearWithStingerCommand,
        ClearWithCSGasCommand,
        ClearWithLauncherCommand,
        ClearWithLeaderCommand
    };
}

/* Missing from SwatCommandWidget 
FText::FromStringTable("SwatCommandTable", "Queuing...")
FText::FromStringTable("SwatCommandTable", "QueueCommand")
FText::FromStringTable("SwatCommandTable", "Acknowledge...")
FText::FromStringTable("SwatCommandTable", "Roger")
FText::FromStringTable("SwatCommandTable", "Negative")
FText::FromStringTable("SwatCommandTable", "SearchAndSecure")
FText::FromStringTable("SwatCommandTable", "Search Room")
FText::FromStringTable("SwatCommandTable", "Doorway")
*/

PRAGMA_ENABLE_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
