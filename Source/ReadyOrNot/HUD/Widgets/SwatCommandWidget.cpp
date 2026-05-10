// Copyright Void Interactive, 2023

#include "SwatCommandWidget.h"

#include "ReadyOrNotAIConfig.h"
#include "SwatCommandEntryWidget.h"
#include "TextWidget.h"
#include "Info/SWATManager.h"

#include "Actors/Door.h"
#include "Actors/Gameplay/IncapacitatedHuman.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"

#include "Actors/BaseGrenade.h"
#include "Actors/StackUpActor.h"
#include "Actors/ThrownEvidenceActor.h"
#include "Actors/Environment/ActivityTriggerVolume.h"
#include "Actors/Gameplay/ExfilActor.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/BreachingShotgun.h"
#include "Actors/Items/C2Explosive.h"
#include "Actors/Items/DoorRam.h"
#include "Actors/Items/GrenadeLauncher.h"

#include "Characters/AI/SWATCharacter.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Data/ActivityData.h"
#include "GameModes/CoopGS.h"
#include "GameModes/TrainingGM.h"
#include "Info/ScoringManager.h"
#include "Info/Activities/ActivityManagerTemplates.h"
#include "Info/Activities/MoveToActivity.h"
#include "Info/Activities/ScanDoorActivity.h"

#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/TeamFallinActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "Info/Activities/MoveToExitActivity.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "Subsystems/AchievementSubsystem.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Command Widget ~ Tick"), STAT_CommandWidgetTick, STATGROUP_SwatCommandWidget);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Command Widget ~ Page View Update"), STAT_CommandWidget_PageViewUpdate, STATGROUP_SwatCommandWidget);

void USwatCommandWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ActiveTeamType = ETeamType::TT_SQUAD;
	
	Back->bBack = true;
}

void USwatCommandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if(UReadyOrNotFunctionLibrary::IsUsingGamepad(UReadyOrNotStatics::GetReadyOrNotPlayerController()))
	{
		return;
	}
	
	SCOPE_CYCLE_COUNTER(STAT_CommandWidgetTick);

	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!LocalPlayerCharacter)
		return;
	
	USWATManager* SwatManager = USWATManager::Get(this);
	AReadyOrNotGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AReadyOrNotGameState>() : nullptr;
	
	if (SwatManager)
	{
		SwatManager->ActiveCommandTeam = ActiveTeamType;

		if (SwatManager->GetSWATCount() == 0)
		{
			bOverrideActiveTeamType = true;
			OverrideActiveTeamType = ETeamType::TT_NONE;
		}
	}

	TimeSinceLastPageUpdate -= InDeltaTime;
	if (TimeSinceLastPageUpdate <= 0.0f)
	{
		TimeSinceLastPageUpdate = 0.15f;
		UpdateCommandPageData();
	}

	if (!SizeBox->IsVisible())
	{
		return;
	}

	if (LastSubCommandPageIndex > 0 && DirectoryStringOverride != "")
	{
		DirectoryStringOverride = "";
		OnPageViewUpdate();
	}

	if (SwatManager)
	{
		const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;
		
		if (bIsSinglePlayer && SwatManager->IsSWATTeamDead() && !bOverrideActiveTeamType)
		{
			ActiveTeamType = ETeamType::TT_NONE;
			SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_None;

			LocalPlayerCharacter->bLookingAtTarget = false;
			LocalPlayerCharacter->bLookingAtHuman = false;
			LocalPlayerCharacter->bLookingAtDoor = false;
			LocalPlayerCharacter->bLookingAtEvidenceItem = false;

			CloseCommandMenu();

			return;
		}
	}

	if (GameState)
	{
		if (GameState->bInPlanningMenu)
		{
			CloseCommandMenu();
			return;
		}
	}

	if (LocalPlayerCharacter->bMirroring)
	{
		CloseCommandMenu();
		return;
	}

	bHoldingQueueCommandKey = GetOwningPlayer()->IsInputKeyDown(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("HoldGoCode"));

	const bool bHasQueueCommand = HasQueuedCommandForActiveTeam();
	
	txt_QueueStatus->SetText(bHoldingQueueCommandKey && !bHasQueueCommand ? FText::FromStringTable("SwatCommandTable", "Queuing...") : FText::FromStringTable("SwatCommandTable", "QueueCommand"));

	VB_Queue->SetVisibility(bHasQueueCommand || bOverrideActiveTeamType ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	Back->SetVisibility(CommandCombo.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void USwatCommandWidget::BuildDefaultPageData(TArray<FSwatCommand>& Commands)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;

	PreviousActiveCommandPage = Commands;
	
	Commands.Empty();

	const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;

	if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
		return;
	
	if (!bIsSinglePlayer)
	{
		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Acknowledge..."), ESwatCommand::SC_None,
		{
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "Roger"), ESwatCommand::SC_Roger),
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "Negative"), ESwatCommand::SC_Negative),
		}));
	}
	
	Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveTo"), ESwatCommand::SC_MoveTo, true));
	if (SwatManager->GetSWATCount() > 0)
	{
		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "FallIn"), ESwatCommand::SC_None,
		{
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "SingleFile"), ESwatCommand::SC_FallIn_Snake, false, true),
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "DoubleFile"), ESwatCommand::SC_FallIn_HalfSnake, false, true),
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "Diamond"), ESwatCommand::SC_FallIn_Diamond, false, true),
			FSwatCommand(FText::FromStringTable("SwatCommandTable", "Wedge"), ESwatCommand::SC_FallIn_Flock, false, true)
		}, true));
	}
	else
	{
		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "FallIn"), ESwatCommand::SC_FallIn, true, true));
	}
	
	Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cover"), ESwatCommand::SC_Cover, true));
	Commands.Add(FSwatCommand(SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType) ? FText::FromStringTable("SwatCommandTable", "Resume") : FText::FromStringTable("SwatCommandTable", "Hold"), SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType) ? ESwatCommand::SC_Resume : ESwatCommand::SC_Hold, false));
	
	if (Cast<ATrapActor>(ContextualData.GetActor()))
		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "DisarmTrap"), ESwatCommand::SC_DisarmStandaloneTrap, false));
	
	Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Deploy..."), ESwatCommand::SC_None,
	{
        FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployFlashbang"), ESwatCommand::SC_DeployFlashbang, true, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang)),
        FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployStinger"), ESwatCommand::SC_DeployStinger, true, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball)),
        FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployCSGas"), ESwatCommand::SC_DeployCSGas, true, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas)),
        FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployChemlight"), ESwatCommand::SC_DeployChemlight, true),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployShield"), ESwatCommand::SC_DeployShield, false, DoesSwatTeamHaveItem(ActiveTeamType, ABallisticsShield::StaticClass()))
    }));

	if (const ACoopGS* GS = GetWorld()->GetGameState<ACoopGS>())
	{
		uint8 NumActive = GetWorld()->GetGameState<AReadyOrNotGameState>()->NumSuspectsActive + GetWorld()->GetGameState<AReadyOrNotGameState>()->NumCiviliansActive;
		for (AEvidenceActor* E : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllEvidenceActors)
		{
			if (E && !E->IsEvidenceCollected())
			{
				NumActive++;
			}
		}
		
		for (ABaseItem* I : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems)
		{
			if (I && I->IsEvidence() && !I->bHasBeenCleared)
			{
				NumActive++;
			}
		}
		
		const bool bOneOrTwoAIOrEvidenceLeft = NumActive <= 2;

		bool bEveryKilledOrArrested = false;
		if (const AScoringManager* ScoringManager = GS->GetScoringManager())
		{
			bEveryKilledOrArrested = ScoringManager->IsEveryoneKilledOrArrested();
		}
		
		if (bEveryKilledOrArrested || GS->RoomData->ClearedRooms.Num() == GS->RoomData->Rooms.Num() || bOneOrTwoAIOrEvidenceLeft)
		{
			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "SearchAndSecure"), ESwatCommand::SC_SearchAndSecure, true, true)); // todo: enable check
		}
		else
		{
			const FVector Location = ContextualData.Location + ContextualData.ImpactNormal * 50.0f;

			bool bIsOutside = false;
			bool bHasTAA = false;
			if (const AThreatAwarenessActor* TAA = UThreatAwarenessSubsystem::Get(this)->GetNearestThreatForLocation(Location, 500.0f, 200.0f, true))
			{
				bIsOutside = TAA->bIsOutside;
				bHasTAA = true;
			}
			
			Commands.Add(FSwatCommand(bIsOutside || !bHasTAA ? FText::FromStringTable("SwatCommandTable", "SearchArea") : FText::FromStringTable("SwatCommandTable", "SearchRoom"), ESwatCommand::SC_SearchAndSecureRoom, true, bHasTAA, false));
		}
	}
}

void USwatCommandWidget::BuildQueuedPageData(TArray<FSwatCommand>& Commands)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;
	
	const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;

	if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
		return;
	
	PreviousActiveCommandPage = Commands;

	Commands.Empty();
	
	Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Execute"), ESwatCommand::SC_Execute, false));
	Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cancel"), ESwatCommand::SC_Cancel, false));
}

void USwatCommandWidget::BuildDoorPageData(ADoor* Door, TArray<FSwatCommand>& Commands)
{
	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager || !Door || !LocalPlayerCharacter)
		return;
	
	const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;

	if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
		return;
	
	if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
	{
		Door = Door->GetSubDoor();
	}

	PreviousActiveCommandPage = Commands;

	Commands.Empty();
	
	bool bDoorIsLocked = Door->GetSWATKnowsLockState() && Door->IsLocked();
	
	const bool bTrapInFront = Door->GetAttachedTrap() ? Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation()) : false;
	const bool bPlayerInFront = Door->IsPointInFrontOfDoorway(LocalPlayerCharacter->GetActorLocation());
	
	const bool bCanDisarmTrap = !Door->IsLocked() && !Door->IsJammed() && Door->GetAttachedTrap() && ((Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live && (Door->DoesSWATKnowTrapState() || bTrapInFront == bPlayerInFront || Door->IsOpen())) || Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Activated || Door->AnyBottomDoorChunksBroken());
	
	const bool bDoorWedgeCommandEnabled = Door->IsClosed() && (Door->bDoorJammed || (!Door->bDoorJammed && DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Doorjam)));
	
	FSwatCommand StackUpCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "StackUp"), ESwatCommand::SC_None);
	FSwatCommand StackUpCommand_DoorwayOnly = FSwatCommand(FText::FromStringTable("SwatCommandTable", "StackUp"), ESwatCommand::SC_None);

	const bool bIsDoorCenterFed_OppositeSide = bPlayerInFront ? Door->BackRoomPosition == EDoorRoomPosition::Center : Door->FrontRoomPosition == EDoorRoomPosition::Center;
	const bool bIsDoorHallwayCenter_OppositeSide = bPlayerInFront ? Door->BackRoomPosition == EDoorRoomPosition::Hallway : Door->FrontRoomPosition == EDoorRoomPosition::Hallway;
	//const bool bIsDoorCenterFed_FromThisSide = bPlayerInFront ? Door->FrontRoomPosition == EDoorRoomPosition::Center : Door->BackRoomPosition == EDoorRoomPosition::Center;

	const bool bAnyLeftStackPoints = bPlayerInFront ? Door->FrontRightStackUpPoints.Num() > 0 : Door->BackLeftStackUpPoints.Num() > 0;
	const bool bAnyRightStackPoints = bPlayerInFront ? Door->FrontLeftStackUpPoints.Num() > 0 : Door->BackRightStackUpPoints.Num() > 0;
	if (bIsDoorCenterFed_OppositeSide || bIsDoorHallwayCenter_OppositeSide)
	{
		TArray<FSwatCommand> StackUpSubCommands;
		bool bSplitEnabled = bAnyLeftStackPoints && bAnyRightStackPoints && SwatManager->GetSWATCount() > 1 && !IsTeamStackedUpOnDoorWithStyle(ActiveTeamType, Door, EStackUpStyle::Split, bPlayerInFront);
		bool bLeftEnabled = bAnyLeftStackPoints && !IsTeamStackedUpOnDoorWithStyle(ActiveTeamType, Door, EStackUpStyle::Left, bPlayerInFront);
		bool bRightEnabled = bAnyRightStackPoints && !IsTeamStackedUpOnDoorWithStyle(ActiveTeamType, Door, EStackUpStyle::Right, bPlayerInFront);

		// enable for coop play
		if (SwatManager->GetSWATCount() == 0)
		{
			bSplitEnabled = true;
			bLeftEnabled = true;
			bRightEnabled = true;
		}
		
		StackUpSubCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Split"), ESwatCommand::SC_StackUpSplit, false, bSplitEnabled, false, false));
		StackUpSubCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Left"), ESwatCommand::SC_StackUpLeft, false, bLeftEnabled, false, false));
		StackUpSubCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Right"), ESwatCommand::SC_StackUpRight, false, bRightEnabled, false, false));
		StackUpSubCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Auto"), ESwatCommand::SC_StackUp, false, true, false, false));
		StackUpCommand.SubCommands = StackUpSubCommands;
		StackUpCommand_DoorwayOnly.SubCommands = StackUpSubCommands;
	}
	else
	{
		StackUpCommand.CommandType = ESwatCommand::SC_StackUp;

		StackUpCommand_DoorwayOnly.CommandText = FText::FromStringTable("SwatCommandTable", "StackUp");
		StackUpCommand_DoorwayOnly.CommandType = ESwatCommand::SC_StackUp;
	}
	
	//LOG_NUMBER(FVector::Distance(Door->GetDoorMidLocation(), LocalPlayerCharacter->GetActorLocation()));
	
	Commands.Add(Door->IsDoorwayOnly() ? StackUpCommand_DoorwayOnly : StackUpCommand);

	bool bLauncherHasAmmo = SwatManager->GetSWATCount() == 0;
	if (ASWATCharacter* Swat = SwatManager->GetSwatWithItem(ActiveTeamType, AGrenadeLauncher::StaticClass()))
	{
		if (AGrenadeLauncher* Launcher = Swat->GetInventoryComponent()->GetInventoryItemOfClass_Native<AGrenadeLauncher>(AGrenadeLauncher::StaticClass(), false))
		{
			bLauncherHasAmmo = Launcher->HasAmmo();
		}
	}
	
	FSwatCommand OpenMoveCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Open..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_OpenAndClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_OpenAndClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_OpenAndClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_OpenAndClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_OpenAndClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_OpenAndClearLeader, false, true),
	}, !bDoorIsLocked);

	FSwatCommand OpenMoveCommands_Doorway = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Move..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_MoveAndClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_MoveAndClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_MoveAndClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_MoveAndClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_MoveAndClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_MoveAndClearLeader, false, true, false, false)
	});
	
	Commands.Add(bDoorIsLocked ? FSwatCommand(FText::FromStringTable("SwatCommandTable", "PickLock"), ESwatCommand::SC_PickLock, false, bDoorIsLocked) : (Door->IsOpenBeyondCloseThreshold() ? OpenMoveCommands_Doorway : OpenMoveCommands));

	bool bCanBreach = !Door->IsDoorwayOnly() && !Door->IsOpen();
	
	FSwatCommand KickCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Kick..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_KickAndClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_KickAndClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_KickAndClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_KickAndClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_KickAndClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_KickAndClearLeader, false, true, false, false)
	}, Door->CanKickDoor(), false, false);

	FSwatCommand ShotgunCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Shotgun..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_ShotgunClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_ShotgunClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_ShotgunClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_ShotgunClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_ShotgunClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_ShotgunClearLeader, false, true, false, false)
	}, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_BreachingShotgun), false, false);
	
	FSwatCommand C2Commands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "C2..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_C2Clear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_C2ClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_C2ClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_C2ClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_C2ClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_C2ClearLeader, false, true, false, false)
	}, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_C2Explosive), false, false);
	
	FSwatCommand RamCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Ram..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_RamAndClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_RamAndClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_RamAndClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_RamAndClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_RamAndClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_RamAndClearLeader, false, true, false, false)
	}, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_BatteringRam), false, false);
	
	FSwatCommand LeaderCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Leader..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Clear"), ESwatCommand::SC_LeaderAndClear, false, true, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithFlashbang"), ESwatCommand::SC_LeaderAndClearFlashbang, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Flashbang), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithStinger"), ESwatCommand::SC_LeaderAndClearStinger, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Stingball), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithCSGas"), ESwatCommand::SC_LeaderAndClearCSGas, false, DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_CSGas), false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLauncher"), ESwatCommand::SC_LeaderAndClearLauncher, false, bLauncherHasAmmo, false, false),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "ClearWithLeader"), ESwatCommand::SC_LeaderAndClearLeader, false, true, false, false)
	}, true, false, false);

	FSwatCommand BreachCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Breach..."), ESwatCommand::SC_None);

	BreachCommands.SubCommands.Add(KickCommands);
	BreachCommands.SubCommands.Add(ShotgunCommands);
	BreachCommands.SubCommands.Add(C2Commands);
	BreachCommands.SubCommands.Add(RamCommands);
	BreachCommands.SubCommands.Add(LeaderCommands);

	BreachCommands.bEnabled = bCanBreach;

	Commands.Add(BreachCommands);
	
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
	
	TArray<AStackUpActor*> CurrentSideStackUps = Door->GetStackupsForArea(bPlayerInFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);
	bool bAnyStackedLeft = false;
    for (const AStackUpActor* StackUpActor : CurrentSideStackUps)
    {
        if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
        {
        	if (ActiveTeamType != ETeamType::TT_SQUAD)
				bSameTeam = ActiveTeamType == Cast<ACyberneticController>(StackUpActor->OccupiedBy)->GetTeam();
        	bAnyStackedLeft = true;
        	break;
        }
    }
	CurrentSideStackUps = Door->GetStackupsForArea(bPlayerInFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight);
	bool bAnyStackedRight = false;
    for (const AStackUpActor* StackUpActor : CurrentSideStackUps)
    {
        if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
        {
        	if (ActiveTeamType != ETeamType::TT_SQUAD)
				bSameTeam = ActiveTeamType == Cast<ACyberneticController>(StackUpActor->OccupiedBy)->GetTeam();
        	bAnyStackedRight = true;
        	break;
        }
    }

	bIsSplit = bAnyStackedLeft && bAnyStackedRight;

	bool bCanScan = bIsDoorCenterFed_OppositeSide &&
					StackUpActors.Num() > 0 &&
					(!bIsSplit && bSameTeam) &&
					!((Door->IsLocked() && Door->TeamKnowsDoorLockState(false)) || Door->IsJammed() || Door->IsDoorBroken());
	
	FSwatCommand ScanCommands = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Scan..."), ESwatCommand::SC_None,
	{
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Slide"), ESwatCommand::SC_Slide, false, Door->IsOpenBeyondCloseThreshold()),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "PIE"), ESwatCommand::SC_Slice, false, true),
		FSwatCommand(FText::FromStringTable("SwatCommandTable", "Peek"), ESwatCommand::SC_Snap, false, !IsTeamStackedUpOnDoor(Door)),
	}, bCanScan);
	
	Commands.Add(ScanCommands);

	if (!Door->IsDoorwayOnly() && !Door->IsDoorBroken())
	{
		Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "MirrorUnderDoor"), ESwatCommand::SC_DeployMirrorgun, false, Door->IsClosed()));

		if (bCanDisarmTrap)
			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "DisarmTrap"), ESwatCommand::SC_DisarmTrap, false, true));
		
		Commands.Add(FSwatCommand(Door->bDoorJammed ? FText::FromStringTable("SwatCommandTable", "RemoveWedge") : FText::FromStringTable("SwatCommandTable", "WedgeDoor"), Door->bDoorJammed ? ESwatCommand::SC_RemoveDoorJam : ESwatCommand::SC_DeployDoorJam, false, bDoorWedgeCommandEnabled));
	}
	
	FSwatCommand CoverCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Cover"), ESwatCommand::SC_Cover, true);
	Commands.Add(CoverCommand);

	if (!Door->IsDoorwayOnly())
	{
		FSwatCommand ToggleDoorCommand = FSwatCommand(Door->IsOpen() ? FText::FromStringTable("SwatCommandTable", "CloseDoor") : FText::FromStringTable("SwatCommandTable", "OpenDoor"), Door->CanCloseDoor(LocalPlayerCharacter) ? ESwatCommand::SC_CloseDoor : ESwatCommand::SC_OpenDoor, false, Door->CanOpenDoor());
		ToggleDoorCommand.bEnabled = !((Door->IsLocked() && Door->TeamKnowsDoorLockState(false)) || Door->IsJammed() || Door->IsDoorBroken());
		
		Commands.Add(ToggleDoorCommand);
	}

	// we cannot stack up red/blue on the same door
	ETeamType StackedUpTeam = ActiveTeamType;
	if (IsOtherTeamStackedUpOnDoor(Door, StackedUpTeam))
	{
		SetActiveTeamElement(StackedUpTeam);
	}

	for (FSwatCommand& Command : Commands)
	{
		bool bIsTeamBreaching = IsTeamBreachingDoor(Door, ActiveTeamType);
		bool bCanIssueOrderOnThisSide = (Door->bCanIssueOrdersOnFrontSide && bPlayerInFront) || (Door->bCanIssueOrdersOnBackSide && !bPlayerInFront);
		if (Command.bEnabled)
			Command.bEnabled = !bIsTeamBreaching && bCanIssueOrderOnThisSide;
	}
}

void USwatCommandWidget::BuildTargetPageData(AActor* Target, TArray<FSwatCommand>& Commands, FString PageTitle)
{
	if (!Target)
		return;

	DirectoryStringOverride = PageTitle;
	
	PreviousActiveCommandPage = Commands;

	Commands.Empty();
	
	USWATManager* SwatManager = USWATManager::Get(this);
	AReadyOrNotGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AReadyOrNotGameState>() : nullptr;
	
	if (ACyberneticCharacter* TargetCharacter = Cast<ACyberneticCharacter>(Target))
	{
		if (TargetCharacter->IsOnSWATTeam() && SwatManager)
		{
			if (!TargetCharacter->IsActive())
				return;
			
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

					bTeamActivityConsiderForFocusBlocking = Cast<UTeamStackUpActivity>(Activity) || Cast<UTeamFallinActivity>(Activity);

					UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Other)
					{
						if (Other != Activity && Other->SharedData->ActivityId == Activity->SharedData->ActivityId && !Other->IsActivityComplete())
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
							bCurrentIsLeft = true;
					}
					else
					{
						if (CurrentStackUpArea == EStackupGenArea::SGA_BackLeft)
							bCurrentIsLeft = true;
					}

					EStackupGenArea OppositeStackUpArea = CurrentStackUpArea;
					TArray<AStackUpActor*> CurrentSideStackUps = Activity->GetStackUpDoor()->GetStackupsForArea(CurrentStackUpArea);
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
					
					TArray<AStackUpActor*> OppositeSideStackUps = Activity->GetStackUpDoor()->GetStackupsForArea(OppositeStackUpArea);
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
			
			const FSwatCommand MovePositionTable[4] =
			{
				// Double spaces cos the text feels a bit cramped
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToAlphaOtherSide"), ESwatCommand::SC_MoveToAlpha, TargetCharacter, true, MaxOppositeSideSquadPosition == ESquadPosition::SP_NONE || MaxOppositeSideSquadPosition >= ESquadPosition::SP_Alpha),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToBravoOtherSide"), ESwatCommand::SC_MoveToBeta, TargetCharacter, true, MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >= ESquadPosition::SP_Alpha),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToCharlieOtherSide"), ESwatCommand::SC_MoveToCharlie, TargetCharacter, true, MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >= ESquadPosition::SP_Beta),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToDeltaOtherSide"), ESwatCommand::SC_MoveToDelta, TargetCharacter, true, MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE && MaxOppositeSideSquadPosition >= ESquadPosition::SP_Charlie),
			};

			TArray<FSwatCommand> MoveToSquadPositionCommands;
			if (bHasTeamActivity && bIsStackUp && NumOppositeStackUps > 0)
			{
				for (uint8 i = 0; i < 4; i++)
				{
					if (MovePositionTable[i].bEnabled)
						MoveToSquadPositionCommands.Add(MovePositionTable[i]);
				}
			}

			bool bCanMove = !bHasTeamActivity || (bHasTeamActivity && bCanSwapNow && !bIsClearing);
			FSwatCommand MoveIndividualCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "Move..."), ESwatCommand::SC_None,
			{
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Here"), ESwatCommand::SC_MoveTo_Individual, TargetCharacter, true, true),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "HereThenBack"), ESwatCommand::SC_MoveToAndBack_Individual, TargetCharacter, true, true)
			}, bCanMove, true);

			if (MoveToSquadPositionCommands.Num() > 0)
				MoveIndividualCommand.SubCommands += MoveToSquadPositionCommands;

			Commands.Add(MoveIndividualCommand);

			bool bHasCustomFocus = TargetCharacter->GetCyberneticsController()->GetTargetingComp()->CustomFocusLocation != FVector::ZeroVector ||
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
			
			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Focus..."), ESwatCommand::SC_None,
			{
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Here"), ESwatCommand::SC_Focus_Individual, TargetCharacter, true, true),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MyPosition"), ESwatCommand::SC_Focus_MyPosition_Individual, TargetCharacter, true, true),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Door"), ESwatCommand::SC_FocusDoor_Individual, TargetCharacter, true, false),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Target"), ESwatCommand::SC_FocusTarget_Individual, TargetCharacter, true, false),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Unfocus"), ESwatCommand::SC_UnFocus_Individual, TargetCharacter, false, bHasCustomFocus)
			}, bCanFocus, true));
			
			const FSwatCommand CurrentCommandTable[4] =
			{
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Alpha"), ESwatCommand::SC_SwapWithAlpha, TargetCharacter, true, SquadPosition != ESquadPosition::SP_Alpha && MaxOverrideSquadPosition >= ESquadPosition::SP_Alpha),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Bravo"), ESwatCommand::SC_SwapWithBeta, TargetCharacter, true, SquadPosition != ESquadPosition::SP_Beta && MaxOverrideSquadPosition >= ESquadPosition::SP_Beta),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Charlie"), ESwatCommand::SC_SwapWithCharlie, TargetCharacter, true, SquadPosition != ESquadPosition::SP_Charlie && MaxOverrideSquadPosition >= ESquadPosition::SP_Charlie),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Delta"), ESwatCommand::SC_SwapWithDelta, TargetCharacter, true, SquadPosition != ESquadPosition::SP_Delta && MaxOverrideSquadPosition >= ESquadPosition::SP_Delta),
			};
			
			TArray<FSwatCommand> FinalSwapCommands;
			
			if (bIsStackUp)
			{
				TArray<FSwatCommand> CurrentSideCommands;
				if (MaxCurrentSideSquadPosition != ESquadPosition::SP_NONE)
				{
					for (uint8 i = 0; i <= (uint8)MaxCurrentSideSquadPosition; i++)
					{
						CurrentSideCommands.Add(CurrentCommandTable[i]);
					}
				}
				
				FinalSwapCommands = CurrentSideCommands;

				if (bIsSplit)
				{
					const FSwatCommand LeftCommandTable[4] =
					{
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToAlphaLeft"), ESwatCommand::SC_SwapWithAlphaOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToBravoLeft"), ESwatCommand::SC_SwapWithBetaOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToCharlieLeft"), ESwatCommand::SC_SwapWithCharlieOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToDeltaLeft"), ESwatCommand::SC_SwapWithDeltaOpposite, TargetCharacter, true, true)
					};
				
					const FSwatCommand RightCommandTable[4] =
					{
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToAlphaRight"), ESwatCommand::SC_SwapWithAlphaOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToBravoRight"), ESwatCommand::SC_SwapWithBetaOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToCharlieRight"), ESwatCommand::SC_SwapWithCharlieOpposite, TargetCharacter, true, true),
						FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToDeltaRight"), ESwatCommand::SC_SwapWithDeltaOpposite, TargetCharacter, true, true)
					};
					
					TArray<FSwatCommand> OppositeSideCommands;

					if (MaxOppositeSideSquadPosition != ESquadPosition::SP_NONE)
					{
						for (uint8 i = 0; i <= (uint8)MaxOppositeSideSquadPosition; i++)
						{
							if (bCurrentIsLeft)
								OppositeSideCommands.Add(RightCommandTable[i]);
							else
								OppositeSideCommands.Add(LeftCommandTable[i]);
						}
					}

					FinalSwapCommands += OppositeSideCommands;

					if (CurrentSideCommands.Num() == 1 && OppositeSideCommands.Num() == 1)
					{
						FText CommandText = FText::Format(FText::FromStringTable("SwatCommandTable", "SwapWithName"), OppositeSideCommands[0].CommandText);
						Commands.Add(FSwatCommand(CommandText, OppositeSideCommands[0].CommandType, false, bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 0, true));
					}
					else
					{
						Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::SC_None, FinalSwapCommands, bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1, true));
					}
				}
				else
				{
					Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::SC_None, FinalSwapCommands, bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1, true));
				}
			}
			else
			{
				for (uint8 i = 0; i < 4; i++)
				{
					FinalSwapCommands.Add(CurrentCommandTable[i]);
				}
				
				Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "SwapWith..."), ESwatCommand::SC_None, FinalSwapCommands, bCanSwapNow && bHasTeamActivity && NumSameTeamActivity > 1 && FinalSwapCommands.Num() > 1, true));
			}
			
			const FVector Location = ContextualData.Location + ContextualData.ImpactNormal * 50.0f;

			bool bIsOutside = false;
			bool bHasTAA = false;
			if (const AThreatAwarenessActor* TAA = UThreatAwarenessSubsystem::Get(this)->GetNearestThreatForLocation(Location, 500.0f, 200.0f, true))
			{
				bIsOutside = TAA->bIsOutside;
				bHasTAA = true;
			}
			
			Commands.Add(FSwatCommand(bIsOutside || !bHasTAA ? FText::FromStringTable("SwatCommandTable", "SearchArea") : FText::FromStringTable("SwatCommandTable", "SearchRoom"), ESwatCommand::SC_SearchAndSecureRoom_Individual, true, !bIsClearing));
			//Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Search Room"), ESwatCommand::SC_SearchAndSecureRoom_Individual, true, !bIsClearing));

			if (TargetCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Shield))
			{
				bool bHasShieldEquipped = TargetCharacter->GetInventoryComponent()->GetEquippedItem<ABallisticsShield>() != nullptr;

				Commands.Add(FSwatCommand(bHasShieldEquipped ? FText::FromStringTable("SwatCommandTable", "HolsterShield") : FText::FromStringTable("SwatCommandTable", "DeployShield"), bHasShieldEquipped ? ESwatCommand::SC_HolsterShield : ESwatCommand::SC_DeployShield, TargetCharacter, false, true));
			}
		}
		else
		{
			if (SwatManager && GameState)
			{
				int32 NumPlayers = GetWorld()->GetGameState<AReadyOrNotGameState>()->GetNumPlayers();
				bool bSwatAvailable = SwatManager->GetSWATCount() != 0 || !SwatManager->IsSWATTeamDead() || NumPlayers > 1;
				
				Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Restrain"), ESwatCommand::SC_DoArrestTarget, false, TargetCharacter->CanArrest() && bSwatAvailable));
			}
			
			bool bIsMoving = TargetCharacter->IsArrestedOrSurrendered() && TargetCharacter->bIsMoving;
			
			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Move..."), ESwatCommand::SC_None,
			{
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Here"), ESwatCommand::SC_MoveTo_Individual, TargetCharacter, true),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "MyPosition"), ESwatCommand::SC_MoveTo_MyPosition_Individual, TargetCharacter, true),
				FSwatCommand(FText::FromStringTable("SwatCommandTable", "Stop"), ESwatCommand::SC_MoveTo_Stop_Individual, TargetCharacter, true, bIsMoving),
			}, TargetCharacter->IsArrestedOrSurrendered(), true));

			if (SwatManager && GameState)
			{
				int32 NumPlayers = GetWorld()->GetGameState<AReadyOrNotGameState>()->GetNumPlayers();
				bool bSwatAvailable = SwatManager->GetSWATCount() != 0 || !SwatManager->IsSWATTeamDead() || NumPlayers > 1;
				
				Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Deploy..."), ESwatCommand::SC_None,
				{
				   FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployTaser"), ESwatCommand::SC_DeployTaser, TargetCharacter, false, TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Taser)),
				   FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployPepperspray"), ESwatCommand::SC_DeployPepperspray, TargetCharacter, true, TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_OCSpray)),
				   FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployPepperball"), ESwatCommand::SC_DeployPepperball, TargetCharacter, true, TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Pepperball)),
				   FSwatCommand(FText::FromStringTable("SwatCommandTable", "DeployBeanbag"), ESwatCommand::SC_DeployBeanbag, TargetCharacter, true, TargetCharacter->IsActive() && DoesSwatTeamHaveItemType(ActiveTeamType, EItemCategory::IC_Beanbag)),
				   FSwatCommand(FText::FromStringTable("SwatCommandTable", "MeleeTarget"), ESwatCommand::SC_MeleeTarget, TargetCharacter, true, TargetCharacter->IsActive()),
				}, TargetCharacter->IsActive() && bSwatAvailable && !TargetCharacter->IsInRagdoll(), true));
			}

			LOCAL_PLAYER;
			const FVector DirectionToInstigator = (LocalPlayer->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal();
			const float Dot = FVector::DotProduct(TargetCharacter->GetActorForwardVector(), DirectionToInstigator);
			const bool bIsFacingPlayer = Dot > 0.25f;

			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "TurnAround"), ESwatCommand::SC_TurnAround_Individual, TargetCharacter, true, TargetCharacter->CanArrest() && bIsFacingPlayer && !TargetCharacter->IsInRagdoll()));
			Commands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "MoveToExit"), ESwatCommand::SC_MoveTo_Exit_Individual, TargetCharacter, true, TargetCharacter->IsCivilian() && TargetCharacter->CanArrest()));
		}
	}
}

void USwatCommandWidget::BuildTrainingPageData(TArray<FSwatCommand>& Commands)
{
	ATrainingGM* TrainingGameMode = GetWorld()->GetAuthGameMode<ATrainingGM>();
	if (!TrainingGameMode)
		return;

	const TArray<FSwatCommandData> CommandsToIssue = TrainingGameMode->GetCurrentCommandsToIssue();

	// If there are no active/incomplete "Issue SWAT Command" activities, then we can't override anything
	if (CommandsToIssue.Num() == 0)
		return;

	// Disable all commands except for the one(s) matching the commands to issue
	for (FSwatCommand& Command : Commands)
	{
		CanCommandBeIssuedForActivities(Command, CommandsToIssue);
	}
}

void USwatCommandWidget::GetExecutingBreachAndClearActivities(TArray<UTeamBreachAndClearActivity*>& OutActivities)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		for (ASWATCharacter* Swat : SwatManager->SwatAI)
		{
			if (SwatManager->IsSWATValid(Swat))
			{
				UTeamBreachAndClearActivity* BreachAndClearActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>();
				if (BreachAndClearActivity)
				{
					if (BreachAndClearActivity->GetCharacter()->GetTeam() == ActiveTeamType || ActiveTeamType == ETeamType::TT_SQUAD)
					{
						OutActivities.Add(BreachAndClearActivity);
					}
				}
			}
		}
	}
}

bool USwatCommandWidget::IsOtherTeamStackedUpOnDoor(ADoor* Door, ETeamType& OutTeam)
{
	OutTeam = ETeamType::TT_SQUAD;
	
	// gold overwrites so ignore this
	if (ActiveTeamType == ETeamType::TT_SQUAD)
		return false;
	
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		for (ASWATCharacter* Swat : SwatManager->SwatAI)
		{
			if (SwatManager->IsSWATValid(Swat))
			{
				if (UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
				{
					if (!StackUpActivity->GetStackUpDoor())
						continue;

					if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() == Door)
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

bool USwatCommandWidget::IsTeamBreachingDoor(ADoor* Door, ETeamType SwatTeam)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		for (ASWATCharacter* Swat : SwatManager->SwatAI)
		{
			if (SwatManager->IsSWATValid(Swat) && (Swat->GetTeam() == SwatTeam || SwatTeam == ETeamType::TT_SQUAD))
			{
				if (const UTeamBreachAndClearActivity* Activity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
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

bool USwatCommandWidget::IsTeamStackedUpOnDoor(ADoor* Door)
{
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		for (ASWATCharacter* Swat : SwatManager->SwatAI)
		{
			if (SwatManager->IsSWATValid(Swat))
			{
				if (UTeamBreachAndClearActivity* BreachAndClearActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
				{
					if (BreachAndClearActivity->GetStackUpDoor() == Door || BreachAndClearActivity->GetStackUpDoor()->GetSubDoor() == Door)
					{
						if (BreachAndClearActivity->GetActiveStateID() >= 5) // clear
						{
							return false;
						}
					}
				}
				
				UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>();
				if (StackUpActivity)
				{
					if (!StackUpActivity->GetStackUpDoor())
						continue;

					if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() == Door)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool USwatCommandWidget::IsTeamStackedUpOnDoorWithStyle(ETeamType SwatTeam, ADoor* Door, EStackUpStyle StackUpStyle, bool bPlayerInFront) const
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return false;
	
	for (ASWATCharacter* Swat : SwatManager->SwatAI)
	{
		if (SwatManager->IsSWATValid(Swat) && (Swat->GetTeam() == SwatTeam || SwatTeam == ETeamType::TT_SQUAD))
		{
			if (const UTeamStackUpActivity* StackUpActivity = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
			{
				if (!StackUpActivity->GetStackUpDoor())
					continue;

				const bool bSameSide = Door->IsPointInFrontOfDoorway(StackUpActivity->SharedData->CommandLocation) == bPlayerInFront;
				if (!bSameSide)
					return false;

				const bool bSameTeam = StackUpActivity->GetCharacter()->GetTeam() == ActiveTeamType || ActiveTeamType == ETeamType::TT_SQUAD;
				if (!bSameTeam)
					return false;

				if (StackUpActivity->GetStackUpDoor() == Door || StackUpActivity->GetStackUpDoor()->GetSubDoor() == Door)
				{
					if (StackUpActivity->SharedData->CommandTeam != SwatTeam)
						return false;
					
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
									return true;
							}
							else if (NumOccupiedLeft > NumOccupiedRight)
							{
								if (bRightFullyOccupied)
									return true;
							}
							else
							{
								if (bLeftFullyOccupied || bRightFullyOccupied)
									return true;
							}

							if (bLeftFullyOccupied && bRightFullyOccupied)
								return true;
						}
					}
					else if (StackUpStyle == EStackUpStyle::Right)
					{
						if (bRightFullyOccupied)
							return true;
					}
					else if (StackUpStyle == EStackUpStyle::Left)
					{
						if (bLeftFullyOccupied)
							return true;
					}

					if (NumOccupiedLeft > 0 || NumOccupiedRight > 0)
					{
						if (StackUpActivity->GetSharedData<FSharedStackUpData>()->StackUpStyle == StackUpStyle) // todo: update shared data pointer, if we have half the team change commands if originally given a squad team command
							return true;
					}
				}
			}
		}
	}

	return false;
}

bool USwatCommandWidget::RequiresPageViewUpdate()
{
	return LastPageUpdateCommandList != ActiveCommandPage;
}

void USwatCommandWidget::OnPageViewUpdate()
{
	SCOPE_CYCLE_COUNTER(STAT_CommandWidget_PageViewUpdate);
	
	Back->bBack = true;
	Back->SwatCommand.InputKey = GetInputBack();

	const ETeamType Team = bOverrideActiveTeamType ? OverrideActiveTeamType : ActiveTeamType;

	Back->UpdateCommandEntry(Back->SwatCommand, Team);

	// collapse all entry widgets
	{
		SwatCommandEntry_1->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_2->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_3->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_4->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_5->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_6->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_7->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_8->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_9->SetVisibility(ESlateVisibility::Collapsed);
		SwatCommandEntry_10->SetVisibility(ESlateVisibility::Collapsed);
	}

	uint8 i = 0;
	for (const FSwatCommand& C : ActiveCommandPage)
	{
		if (USwatCommandEntryWidget* Entry = IndexToEntryWidget(i))
		{
			InitEntryWidget(Entry, C, Team, i == ActiveCommandPage.Num()-1);
			
			Entry->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		
		i++;
	}

	UpdateDirectory();

	txt_QueueBinding->SetText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("HoldGoCode").GetDisplayName());

	DivTop->SetColorAndOpacity(GetTeamColor());
	DivBottom->SetColorAndOpacity(GetTeamColor());
}

FLinearColor USwatCommandWidget::GetTeamColor() const
{
	switch (GetActiveTeam())
	{
		case ETeamType::TT_NONE:			return FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
		case ETeamType::TT_SERT_RED:		return RedTeamColor;
		case ETeamType::TT_SERT_BLUE:		return BlueTeamColor;
		case ETeamType::TT_SUSPECT:			return RedTeamColor;
		case ETeamType::TT_CIVILIAN:		return BlueTeamColor;
		case ETeamType::TT_SQUAD:			return GoldTeamColor;
		default:							return FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
	}
}

void USwatCommandWidget::UpdateDirectory()
{
	if (DirectoryStringOverride.IsEmpty())
	{
		if (LastSubCommandPageIndex > 0)
		{
			for (uint8 i = 0; i < LastSubCommandPageIndex; i++)
			{
				if (i > 0)
				{
					DirectoryString += " / " + ParentCommands[i].CommandText.ToString().Replace(TEXT("..."), TEXT(""));
				}
				else
				{
					DirectoryString = ParentCommands[i].CommandText.ToString().Replace(TEXT("..."), TEXT(""));
				}
			}
		}
		else
		{
			CommandDirectoryText->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
	}
	else
	{
		DirectoryString = DirectoryStringOverride;
	}
	
	CommandDirectoryText->SetText(FText::FromString(DirectoryString));

	const ETeamType Team = bOverrideActiveTeamType ? OverrideActiveTeamType : ActiveTeamType;
	
	switch (Team)
	{
		case ETeamType::TT_NONE:			CommandDirectoryText->SetTextColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.9f)); break;
		case ETeamType::TT_SERT_RED:		CommandDirectoryText->SetTextColor(RedTeamColor); break;
		case ETeamType::TT_SERT_BLUE:		CommandDirectoryText->SetTextColor(BlueTeamColor); break;
		case ETeamType::TT_SUSPECT:			CommandDirectoryText->SetTextColor(RedTeamColor); break;
		case ETeamType::TT_CIVILIAN:		CommandDirectoryText->SetTextColor(BlueTeamColor); break;
		case ETeamType::TT_SQUAD:			CommandDirectoryText->SetTextColor(GoldTeamColor); break;
		default:							CommandDirectoryText->SetTextColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.9f)); break;
	}
	
	CommandDirectoryText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

ETeamType USwatCommandWidget::GetActiveTeam() const
{
	return bOverrideActiveTeamType ? OverrideActiveTeamType : ActiveTeamType;
}

USwatCommandEntryWidget* USwatCommandWidget::IndexToEntryWidget(uint8 Index) const
{
	switch (Index)
	{
		case 0:		return SwatCommandEntry_1;
		case 1:		return SwatCommandEntry_2;
		case 2:		return SwatCommandEntry_3;
		case 3:		return SwatCommandEntry_4;
		case 4:		return SwatCommandEntry_5;
		case 5:		return SwatCommandEntry_6;
		case 6:		return SwatCommandEntry_7;
		case 7:		return SwatCommandEntry_8;
		case 8:		return SwatCommandEntry_9;
		default:	return SwatCommandEntry_10;
	}
}

void USwatCommandWidget::InitEntryWidget(USwatCommandEntryWidget* Entry, const FSwatCommand& InSwatCommand, ETeamType Team, bool bLast)
{
	Entry->SwatCommand = InSwatCommand;
	Entry->ActiveTeamType = Team;
	Entry->RedTeamColor = RedTeamColor;
	Entry->BlueTeamColor = BlueTeamColor;
	Entry->GoldTeamColor = GoldTeamColor;
	Entry->bLast = bLast;

	Entry->UpdateCommandEntry(InSwatCommand, Team);
}

void USwatCommandWidget::SetLastCommandPage(TArray<FSwatCommand>& InCommands)
{
	for (int32 i = 0; i < InCommands.Num(); i++)
	{
		InCommands[i].InputKey = ConvertIntToInputKey(i+1);
		InCommands[i].Index = i;
	}

	LastPageUpdateCommandList = InCommands;
}

void USwatCommandWidget::ExecuteCommand(FSwatCommand Command, bool bFromDefault, bool bOnlyVO)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	AReadyOrNotGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AReadyOrNotGameState>() : nullptr;
	
	if (Command.bGrabContextualDataOnExecute)
	{
		LOCAL_PLAYER;
		FCollisionQueryParams QueryParams = LocalPlayer ? LocalPlayer->GetCollisionQueryParameters() : FCollisionQueryParams::DefaultQueryParam;
		QueryParams.OwnerTag = "CommandWidget";
		
		const bool bIgnoreAllDoors = Cast<ASWATCharacter>(Command.CommandTarget) != nullptr;
		
		if (bIgnoreAllDoors && GameState)
		{
			QueryParams.AddIgnoredActors((TArray<AActor*>)GameState->AllDoors);
		}
		
		GrabContextData(QueryParams);
	}

	ExecutingContextualData = Command.bUseSecondaryContextData ? ContextualData2 : ContextualData;
	if (ContextualData.GetActor())
	{
		V_LOGM(LogReadyOrNot, "Executing command (Target: %s, Grab Contextual Data: %d)", *ContextualData.GetActor()->GetName(), Command.bGrabContextualDataOnExecute);
	}

	ExecutionTeamType = ActiveTeamType;

	bHoldPageUntilExecuted = false;
	
	UFMODBlueprintStatics::PlayEvent2D(this, ExecuteCommandEvent, true);

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

	LOCAL_PLAYER;
	
	TArray<UTeamBreachAndClearActivity*> OutBreachAndClearActivities;
	GetExecutingBreachAndClearActivities(OutBreachAndClearActivities);
	switch (Command.CommandType)
	{
		case ESwatCommand::SC_None: break;
		case ESwatCommand::SC_Roger:					SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND, "", false); break;
		case ESwatCommand::SC_Negative:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC, "", false); break;
		case ESwatCommand::SC_MoveTo:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_TO); break;
		case ESwatCommand::SC_MoveTo_Individual:		SwatManager->PlaySwatCommandVoiceLine(bIsSwat ? VO_SWAT_COMMAND::CALL_SC_MOVE_TO : VO_SWAT_GENERAL::CALL_ORDER_MOVE, "", false); break;
		case ESwatCommand::SC_MoveTo_Exit_Individual:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_GENERAL::CALL_EXIT, "", false); break;
		case ESwatCommand::SC_FallIn:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN); break;
		case ESwatCommand::SC_FallIn_Snake:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN); break;
		case ESwatCommand::SC_FallIn_HalfSnake:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN); break;
		case ESwatCommand::SC_FallIn_Diamond:			SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_FALL_IN_DIAMOND : VO_SWAT_COMMAND::CALL_SC_FALL_IN); break;
		case ESwatCommand::SC_FallIn_Flock:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_FALL_IN); break;
		case ESwatCommand::SC_Cover:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_COVER); break;
		case ESwatCommand::SC_Hold:						SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_HOLD); break;
		case ESwatCommand::SC_Resume:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RESUME); break;
		case ESwatCommand::SC_DeployFlashbang:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_FLASHBANG); break;
		case ESwatCommand::SC_DeployStinger:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_STINGER); break;
		case ESwatCommand::SC_DeployCSGas:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_CSGAS); break;
		case ESwatCommand::SC_DeployChemlight:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_CHEMLIGHT); break;
		case ESwatCommand::SC_DeployShield:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_SHIELD); break;
		case ESwatCommand::SC_HolsterShield:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RESUME); break;
		case ESwatCommand::SC_DoCollectEvidence:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_COLLECT_EVIDENCE); break;
		case ESwatCommand::SC_DoArrestTarget:
		{
			FString VO;
			if (bIsFemaleTarget)
				VO = FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_ARREST_FEMALE : VO_SWAT_COMMAND::CALL_SC_ARREST;
			else
				VO = FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_ARREST_MALE : VO_SWAT_COMMAND::CALL_SC_ARREST;
				
			SwatManager->PlaySwatCommandVoiceLine(VO);
		}
		break;
		case ESwatCommand::SC_DoReportTarget:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DO_REPORT_TARGET); break;
		case ESwatCommand::SC_DisarmStandaloneTrap:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DISARM_TRAP); break;
		case ESwatCommand::SC_KillMe:
			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_KILL_ME);
			DoCommand(Command);
			CloseCommandMenu();
			return;
		case ESwatCommand::SC_StackUp:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_STACK_UP); break;
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
		case ESwatCommand::SC_PickLock:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_PICK_LOCK, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RemoveDoorJam:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_REMOVE_DOOR_JAM, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_DeployMirrorgun:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_MIRRORGUN, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_DeployDoorJam:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DEPLOY_DOOR_JAM, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_CheckForTrap:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_CHECK_FOR_TRAP, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_DisarmTrap:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_DISARM_TRAP, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_CloseDoor:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_CLOSE_DOOR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenDoor:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_DOOR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_Slide:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_SLIDE, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_Slice:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_PIE, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_Snap:						SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SCAN_DOOR_PEEK, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_SearchAndSecure:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE); break;
		case ESwatCommand::SC_SearchAndSecureRoom:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE); break;
		case ESwatCommand::SC_MoveAndClear:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_MoveAndClearFlashbang:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_MoveAndClearStinger:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_MoveAndClearCSGas:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_MoveAndClearLauncher:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_MoveAndClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClear:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClearFlashbang:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClearStinger:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClearCSGas:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClearLauncher:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_OpenAndClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClear:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClearFlashbang:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClearStinger:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClearCSGas:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClearLauncher:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_KickAndClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClear:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClearFlashbang:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClearStinger:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClearCSGas:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClearLauncher:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_ShotgunClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClear:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClearFlashbang:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClearStinger:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClearCSGas:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClearLauncher:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_RamAndClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2Clear:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2ClearFlashbang:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2ClearStinger:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2ClearCSGas:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2ClearLauncher:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_C2ClearLeader:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LEADER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClear:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClearFlashbang:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_FLASHBANG, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClearStinger:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_STINGER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClearCSGas:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_CSGAS, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClearLauncher:	SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_LAUNCHER, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_LeaderAndClearLeader:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR, "", !IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualActor))); break;
		case ESwatCommand::SC_SwapWithAlpha:			SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA : VO_SWAT_COMMAND::CALL_SC_SWAP); break;
		case ESwatCommand::SC_SwapWithBeta:				SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA : VO_SWAT_COMMAND::CALL_SC_SWAP); break;
		case ESwatCommand::SC_SwapWithCharlie:			SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE : VO_SWAT_COMMAND::CALL_SC_SWAP); break;
		case ESwatCommand::SC_SwapWithDelta:			SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA : VO_SWAT_COMMAND::CALL_SC_SWAP); break;
		case ESwatCommand::SC_SwapWithAlphaOpposite:	SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA : VO_SWAT_COMMAND::CALL_SC_SWAP);break;
		case ESwatCommand::SC_SwapWithBetaOpposite:		SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA : VO_SWAT_COMMAND::CALL_SC_SWAP);break;
		case ESwatCommand::SC_SwapWithCharlieOpposite:	SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE : VO_SWAT_COMMAND::CALL_SC_SWAP);break;
		case ESwatCommand::SC_SwapWithDeltaOpposite:	SwatManager->PlaySwatCommandVoiceLine(FMath::RandBool() ? VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA : VO_SWAT_COMMAND::CALL_SC_SWAP);break;
		case ESwatCommand::SC_MoveToAlpha:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA); break;
		case ESwatCommand::SC_MoveToBeta:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA); break;
		case ESwatCommand::SC_MoveToCharlie:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE); break;
		case ESwatCommand::SC_MoveToDelta:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA); break;
		case ESwatCommand::SC_DeployTaser:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_TASER); break;
		case ESwatCommand::SC_DeployPepperspray:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_SPRAY); break;
		case ESwatCommand::SC_DeployPepperball:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_PEPPER); break;
		case ESwatCommand::SC_DeployBeanbag:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_BEANBAG); break;
		case ESwatCommand::SC_MeleeTarget:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL); break;
		case ESwatCommand::SC_Execute:
		{
			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_EXECUTE);
			FQueuedSwatCommand* ActiveTeam_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ActiveTeamType);
			FQueuedSwatCommand* Red_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SERT_RED);
			FQueuedSwatCommand* Blue_QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SERT_BLUE);

			if (ActiveTeamType == ETeamType::TT_SQUAD)
			{
				if (ActiveTeam_QueuedSwatCommand)
				{
					DoCommand(ActiveTeam_QueuedSwatCommand->Command, true, ETeamType::TT_SQUAD, ActiveTeam_QueuedSwatCommand->ContextualData, true);

					SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);
					SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
					SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
				}
				else if (Red_QueuedSwatCommand && Blue_QueuedSwatCommand)
				{
					DoCommand(Blue_QueuedSwatCommand->Command, true, ETeamType::TT_SERT_BLUE, Blue_QueuedSwatCommand->ContextualData, true);
					DoCommand(Red_QueuedSwatCommand->Command, true, ETeamType::TT_SERT_RED, Red_QueuedSwatCommand->ContextualData, true);
					
					SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
					SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
				}
			}
			else
			{
				if (ActiveTeam_QueuedSwatCommand)
				{
					DoCommand(ActiveTeam_QueuedSwatCommand->Command, true, ActiveTeamType, ActiveTeam_QueuedSwatCommand->ContextualData, true);
					SwatManager->QueuedSwatCommandMap.Remove(ActiveTeamType);
				}
			}
		}
		break;
		
		case ESwatCommand::SC_Cancel:
		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_CANCEL);
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

		case ESwatCommand::SC_SearchAndSecureRoom_Individual:		SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE, "", false); break;
		case ESwatCommand::SC_MoveToAndBack_Individual:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_MOVE_TO, "", false); break;
		case ESwatCommand::SC_MoveTo_MyPosition_Individual:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_GENERAL::CALL_ORDER_MOVE_TO_ME, "", false); break;
		case ESwatCommand::SC_MoveTo_Stop_Individual:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_HOLD, "", false); break;
		case ESwatCommand::SC_TurnAround_Individual:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_SC_TURN_AROUND, "", false); break;
		case ESwatCommand::SC_Focus_Individual:						SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_FOCUS, "", false); break;
		case ESwatCommand::SC_Focus_MyPosition_Individual:			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_FOCUS, "", false); break;
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
		case ESwatCommand::SC_FocusTarget_Individual:				SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_FOCUS_TARGET, "", false); break;
		case ESwatCommand::SC_UnFocus_Individual:					SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_UNFOCUS, "", false); break;
		default: break;
	}

	OnCommandIssued(Command.Index, Command, bFromDefault);

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
					Delay = Sound->GetCurrentSoundLength();
			}
		}

		if (Delay <= 0.0f)
			Delay = 1.0f;

		// Allow multiple timers just store this temp, we want every queued command to execute
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &USwatCommandWidget::DoCommand, Command, false, ActiveTeamType, ExecutingContextualData, true), Delay, false);
	}
	
	CloseCommandMenu();
}

bool USwatCommandWidget::HasQueuedCommandForTeam(ETeamType TeamType) const
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return false;
	
	if (SwatManager->IsCharacterKnownEnemy(SwatManager->GetSquadLeader()))
		return true;
	
	bool bHasRedTeamCommand = false;
	bool bHasBlueTeamCommand = false;
	bool bHasGoldTeamCommand = false;
	
	for (const auto& k : SwatManager->QueuedSwatCommandMap)
	{
		if (k.Key == ETeamType::TT_SERT_RED) bHasRedTeamCommand = true;
		if (k.Key == ETeamType::TT_SERT_BLUE) bHasBlueTeamCommand = true;
		if (k.Key == ETeamType::TT_SQUAD) bHasGoldTeamCommand = true;
	}
	
	// if both blue and red have queued commands then gold does
	if (bHasRedTeamCommand && bHasBlueTeamCommand) bHasGoldTeamCommand = true;
	
	switch (TeamType)
	{
		case ETeamType::TT_SERT_RED:	return bHasRedTeamCommand;
		case ETeamType::TT_SERT_BLUE:	return bHasBlueTeamCommand;
		case ETeamType::TT_SQUAD:		return bHasGoldTeamCommand;
		default: ;
	}
	
	return false;
}

bool USwatCommandWidget::HasQueuedCommandForActiveTeam() const
{
	return HasQueuedCommandForTeam(ActiveTeamType);
}

bool USwatCommandWidget::CanQueue() const
{
	if (const USWATManager* SwatManager = USWATManager::Get(this))
	{
		if (SwatManager->CurrentDefaultCommand == ESwatCommand::SC_KillMe)
		{
			return false;
		}
	}
	
	return UReadyOrNotStatics::GetReadyOrNotPlayerController()->IsInputKeyDown(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("HoldGoCode")) &&
			!bOverrideActiveTeamType;
}

void USwatCommandWidget::QueueCommand(FSwatCommand Command)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;
	
	if (bOverrideActiveTeamType)
		return;
	
	FQueuedSwatCommand QueuedSwatCommand;
	QueuedSwatCommand.ContextualData = ContextualData;
	QueuedSwatCommand.Command = Command;
	QueuedSwatCommand.Command.bGrabContextualDataOnExecute = false;
	SwatManager->QueuedSwatCommandMap.Add(ActiveTeamType, QueuedSwatCommand);

	OnSwatCommandQueued.Broadcast(QueuedSwatCommand, ActiveTeamType);

	// Means we have broken up the teams into two queues, remove gold and transfer it to the opposite team that this widget is active on
	if (ActiveTeamType != ETeamType::TT_SQUAD)
	{
		ETeamType OppositeActiveTeam = (ActiveTeamType == ETeamType::TT_SERT_RED ? ETeamType::TT_SERT_BLUE : ETeamType::TT_SERT_RED);
		FQueuedSwatCommand* CachedQueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SQUAD);
		if (CachedQueuedSwatCommand)
		{
			SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);
			
			SwatManager->QueuedSwatCommandMap.Add(OppositeActiveTeam, *CachedQueuedSwatCommand);
			
			OnSwatCommandQueued.Broadcast(*CachedQueuedSwatCommand, OppositeActiveTeam);
		}
	}

	// Stack up on door if queuing a breaching command
	if (Command.CommandType == ESwatCommand::SC_OpenAndClear || Command.CommandType == ESwatCommand::SC_OpenAndClearFlashbang || Command.CommandType == ESwatCommand::SC_OpenAndClearStinger || Command.CommandType == ESwatCommand::SC_OpenAndClearCSGas || Command.CommandType == ESwatCommand::SC_OpenAndClearLauncher || Command.CommandType == ESwatCommand::SC_OpenAndClearLeader ||
		Command.CommandType == ESwatCommand::SC_MoveAndClear || Command.CommandType == ESwatCommand::SC_MoveAndClearFlashbang || Command.CommandType == ESwatCommand::SC_MoveAndClearStinger || Command.CommandType == ESwatCommand::SC_MoveAndClearCSGas || Command.CommandType == ESwatCommand::SC_MoveAndClearLauncher || Command.CommandType == ESwatCommand::SC_MoveAndClearLeader ||
		Command.CommandType == ESwatCommand::SC_KickAndClear || Command.CommandType == ESwatCommand::SC_KickAndClearFlashbang || Command.CommandType == ESwatCommand::SC_KickAndClearStinger || Command.CommandType == ESwatCommand::SC_KickAndClearCSGas || Command.CommandType == ESwatCommand::SC_KickAndClearLauncher || Command.CommandType == ESwatCommand::SC_KickAndClearLeader ||
		Command.CommandType == ESwatCommand::SC_C2Clear || Command.CommandType == ESwatCommand::SC_C2ClearFlashbang || Command.CommandType == ESwatCommand::SC_C2ClearStinger || Command.CommandType == ESwatCommand::SC_C2ClearCSGas || Command.CommandType == ESwatCommand::SC_C2ClearLauncher || Command.CommandType == ESwatCommand::SC_C2ClearLeader ||
		Command.CommandType == ESwatCommand::SC_ShotgunClear || Command.CommandType == ESwatCommand::SC_ShotgunClearFlashbang || Command.CommandType == ESwatCommand::SC_ShotgunClearStinger || Command.CommandType == ESwatCommand::SC_ShotgunClearCSGas || Command.CommandType == ESwatCommand::SC_ShotgunClearLauncher || Command.CommandType == ESwatCommand::SC_ShotgunClearLeader ||
		Command.CommandType == ESwatCommand::SC_RamAndClear || Command.CommandType == ESwatCommand::SC_RamAndClearFlashbang || Command.CommandType == ESwatCommand::SC_RamAndClearStinger || Command.CommandType == ESwatCommand::SC_RamAndClearCSGas || Command.CommandType == ESwatCommand::SC_RamAndClearLauncher || Command.CommandType == ESwatCommand::SC_RamAndClearLeader ||
		Command.CommandType == ESwatCommand::SC_LeaderAndClear || Command.CommandType == ESwatCommand::SC_LeaderAndClearFlashbang || Command.CommandType == ESwatCommand::SC_LeaderAndClearStinger || Command.CommandType == ESwatCommand::SC_LeaderAndClearCSGas || Command.CommandType == ESwatCommand::SC_LeaderAndClearLauncher || Command.CommandType == ESwatCommand::SC_LeaderAndClearLeader)
	{
		FSwatCommand StackupCommand = FSwatCommand(FText::FromStringTable("SwatCommandTable", "StackUp"), ESwatCommand::SC_StackUp, nullptr, false, true);
		StackupCommand.InputKey = GetInputOne();
		
		ExecuteCommand(StackupCommand, false, IsTeamStackedUpOnDoor(Cast<ADoor>(ContextualData.GetActor())));
		
		OnCommandIssued(Command.Index, Command, false);
	}
	
	UpdateCommandPageData();
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

void USwatCommandWidget::UpdateCommandPageData()
{
	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!LocalPlayerCharacter)
		return;
	
	USWATManager* SwatManager = USWATManager::Get(this);

	if (SwatManager)
	{
		const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;
		
		if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
		{
			bOverrideActiveTeamType = true;
			ActiveTeamType = ETeamType::TT_NONE;
			SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_None;

			LocalPlayerCharacter->bLookingAtTarget = false;
			LocalPlayerCharacter->bLookingAtHuman = false;
			LocalPlayerCharacter->bLookingAtDoor = false;
			LocalPlayerCharacter->bLookingAtEvidenceItem = false;
			
			//return;
		}

		if ((ActiveTeamType == ETeamType::TT_SQUAD && (SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_RED) || SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_BLUE))) ||
			(ActiveTeamType == ETeamType::TT_SERT_RED && SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_RED)) ||
			(ActiveTeamType == ETeamType::TT_SERT_BLUE && SwatManager->IsSWATTeamDead(ETeamType::TT_SERT_BLUE)))
		{
			CycleSwatElement(true);
		}
		
		if (SwatManager->IsCharacterKnownEnemy(LocalPlayerCharacter))
		{
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_KillMe;
			
			bOverrideActiveTeamType = false;
			OverrideActiveTeamType = ETeamType::TT_NONE;
			
			DirectoryStringOverride = "";
			
			ActiveCommandPage.Empty();
			ActiveCommandPage.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "KillMe"), ESwatCommand::SC_KillMe, LocalPlayerCharacter, false));

			if (RequiresPageViewUpdate())
			{
				SetLastCommandPage(ActiveCommandPage);
				OnPageViewUpdate();
			}
			
			return;
		}
	}

	if (HasQueuedCommandForTeam(ActiveTeamType))
	{
		BuildQueuedPageData(ActiveCommandPage);

		if (SwatManager)
		{
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_Execute;
		}
	}
	else
	{
		GrabContextData();

		if (LastSubCommandPageIndex > 0)
		{
			if (LastActorBeforeGoingIntoSubPage == ContextualData.GetActor() || bHoldPageUntilExecuted)
			{
				// rebuild page data for indiviual Focus commands

				bool bAnyChange = false;
				if (Cast<ASWATCharacter>(LastActorBeforeGoingIntoSubPage))
				{
					for (FSwatCommand& Command : ActiveCommandPage)
					{
						const FSwatCommand CommandCopy = Command;
						
						if (Command.CommandType == ESwatCommand::SC_FocusDoor_Individual)
						{
							Command.CommandTarget2 = nullptr;
							Command.CommandText = FText::FromStringTable("SwatCommandTable", "Door");
							if (ADoor* Door = Cast<ADoor>(ContextualData.GetActor()))
							{
								Command.CommandTarget2 = Door;
								if (Door->IsDoorwayOnly())
									Command.CommandText = FText::FromStringTable("SwatCommandTable", "Doorway");
							}
							
							Command.bEnabled = Command.CommandTarget2 != nullptr;
						}
						else if (Command.CommandType == ESwatCommand::SC_FocusTarget_Individual)
						{
							Command.CommandTarget2 = nullptr;
							if (AReadyOrNotCharacter* Target = Cast<AReadyOrNotCharacter>(ContextualData.GetActor()))
							{
								if (!Target->IsOnSWATTeam())
								{
									Command.CommandTarget2 = Target;
								}
							}
							
							Command.bEnabled = Command.CommandTarget2 != nullptr;
						}
						else if (Command.CommandType == ESwatCommand::SC_SwapWithAlpha ||
								Command.CommandType == ESwatCommand::SC_SwapWithBeta ||
								Command.CommandType == ESwatCommand::SC_SwapWithCharlie ||
								Command.CommandType == ESwatCommand::SC_SwapWithDelta)
						{
							ESquadPosition SquadPosition = ESquadPosition::SP_NONE;
							ESquadPosition MaxOverrideSquadPosition = ESquadPosition::SP_NONE;
							if (ACyberneticController* Controller = Cast<ACyberneticCharacter>(Command.CommandTarget)->GetCyberneticsController())
							{
								if (UTeamBaseActivity* Activity = Controller->GetCurrentActivity<UTeamBaseActivity>())
								{
									SquadPosition = Activity->OverrideSquadPosition;
									
									UActivityManager::IterateAllActivitiesOfType<UTeamBaseActivity>([&](UTeamBaseActivity* Other)
									{
										if (Other != Activity && Other->SharedData->ActivityId == Activity->SharedData->ActivityId && !Other->IsActivityComplete())
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
							}

							Command.bEnabled = (Command.CommandType == ESwatCommand::SC_SwapWithAlpha && SquadPosition != ESquadPosition::SP_Alpha && MaxOverrideSquadPosition >= ESquadPosition::SP_Alpha) ||
												(Command.CommandType == ESwatCommand::SC_SwapWithBeta && SquadPosition != ESquadPosition::SP_Beta && MaxOverrideSquadPosition >= ESquadPosition::SP_Beta) ||
												(Command.CommandType == ESwatCommand::SC_SwapWithCharlie && SquadPosition != ESquadPosition::SP_Charlie && MaxOverrideSquadPosition >= ESquadPosition::SP_Charlie) ||
												(Command.CommandType == ESwatCommand::SC_SwapWithDelta && SquadPosition != ESquadPosition::SP_Delta && MaxOverrideSquadPosition >= ESquadPosition::SP_Delta);
						}
						
						if (Command.bEnabled != CommandCopy.bEnabled)
						{
							bAnyChange = true;
						}
					}
				}
				else if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(LastActorBeforeGoingIntoSubPage))
				{
					for (FSwatCommand& Command : ActiveCommandPage)
					{
						if (Command.CommandType == ESwatCommand::SC_TurnAround_Individual)
						{
							LOCAL_PLAYER;
							const FVector DirectionToInstigator = (LocalPlayer->GetActorLocation() - AI->GetActorLocation()).GetSafeNormal();
							const float Dot = FVector::DotProduct(AI->GetActorForwardVector(), DirectionToInstigator);
							const bool bIsFacingPlayer = Dot > 0.25f;
							
							Command.bEnabled = bIsFacingPlayer;
							bAnyChange = true;
						}
					}
				}
				else if (Cast<ADoor>(LastActorBeforeGoingIntoSubPage))
				{
					BuildTrainingPageData(ActiveCommandPage);
					bAnyChange = true;
				}

				if (bAnyChange)
				{
					OnPageViewUpdate();
				}
				
				return;
			}
		}

		bOverrideActiveTeamType = SwatManager ? SwatManager->GetSWATCount() == 0 : true;
		DirectoryStringOverride = "";		
		
		LastActorBeforeGoingIntoSubPage = nullptr;
		LastSubCommandPageIndex = 0;
		ParentCommands.Empty();
		CommandCombo.Empty();
		
		BuildDefaultPageData(ActiveCommandPage);
		
		ESwatCommand DefaultCommand;
		UBpGameplayHelperLib::LoadDefaultCommands(DefaultCommand/*, DefaultHumanCommand*/, DefaultDoorUnknownCommand, DefaultDoorOpenCommand, DefaultDoorLockedCommand, DefaultDoorUnlockedCommand);

		if (SwatManager)
		{
			if (DefaultCommand == ESwatCommand::SC_Hold)
			{
				if (SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType))
				{
					DefaultCommand = ESwatCommand::SC_Resume;
				}
			}
			else if (DefaultCommand == ESwatCommand::SC_Resume)
			{
				if (!SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType))
				{
					DefaultCommand = ESwatCommand::SC_Hold;
				}
			}
			
			SwatManager->CurrentDefaultCommand = DefaultCommand;
		}

		LocalPlayerCharacter->bLookingAtTarget = false;
		LocalPlayerCharacter->bLookingAtHuman = false;
		LocalPlayerCharacter->bLookingAtDoor = false;

		if (ContextualData.GetActor())
		{
			if (const ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(ContextualData.GetActor()))
			{
				ESwatCommand DefaultHumanSwatCommand = ESwatCommand::SC_None;
				
				if (!AICharacter->IsOnSWATTeam())
				{
					if ((AICharacter->IsArrested() || AICharacter->IsDeadOrUnconscious() || AICharacter->IsPlayingDead()) && !AICharacter->HasBeenReported())
					{
						DefaultHumanSwatCommand = ESwatCommand::SC_DoReportTarget;
					}
					else
					{
						if (AICharacter->CanArrest() && !AICharacter->IsPlayingDead())
						{
							DefaultHumanSwatCommand = ESwatCommand::SC_DoArrestTarget;
						}
					}
				}
				
				if (DefaultHumanSwatCommand != ESwatCommand::SC_None)
				{
					if (SwatManager)
						SwatManager->CurrentDefaultCommand = DefaultHumanSwatCommand;
				}
				
				LocalPlayerCharacter->bLookingAtTarget = true;
				LocalPlayerCharacter->bLookingAtHuman = true;

				bOverrideActiveTeamType = true;
				OverrideActiveTeamType = ETeamType::TT_NONE;
				
				FString PageTitle = "";
				if (AICharacter->IsOnSWATTeam())
				{
					const ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(AICharacter);
					if (SwatCharacter)
					{
						PageTitle = SwatCharacter->GetSwatCharacterName().ToString();
					}
				}
				
				BuildTargetPageData(ContextualData.GetActor(), ActiveCommandPage, PageTitle);
			}
			else if (ADoor* Door = Cast<ADoor>(ContextualData.GetActor()))
			{
				if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
				{
					Door = Door->GetSubDoor();
				}
				
				const bool bPlayerInFront = Door->IsPointInFrontOfDoorway(LocalPlayerCharacter->GetActorLocation());
				const bool bCanIssueOrderOnThisSide = (Door->bCanIssueOrdersOnFrontSide && bPlayerInFront) || (Door->bCanIssueOrdersOnBackSide && !bPlayerInFront);
				if (bCanIssueOrderOnThisSide)
				{
					LocalPlayerCharacter->bLookingAtTarget = true;
					LocalPlayerCharacter->bLookingAtDoor = true;
					
					const bool bDoorUnknownToSWAT = !Door->GetSWATKnowsLockState();
					const bool bDoorIsLocked = Door->GetSWATKnowsLockState() && Door->IsLocked();
					const bool bDoorCanCheck = bDoorUnknownToSWAT && IsTeamStackedUpOnDoor(Door);
					
					const bool bTrapInFront = Door->GetAttachedTrap() ? Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation()) : false;
					const bool bSameSideAsTrap = bTrapInFront == bPlayerInFront;
		
					//const bool bCanDisarmTrap = Door->GetAttachedTrap() && ((Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live && (Door->DoesSWATKnowTrapState() || bTrapInFront == bPlayerInFront || Door->IsOpen())) || Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Activated || Door->AnyBottomDoorChunksBroken());
					//const bool bCanDisarmTrap = Door->GetAttachedTrap() && ((Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live && (Door->DoesSWATKnowTrapState() || bTrapInFront == bPlayerInFront || Door->IsOpen())) || (Door->GetAttachedTrap()->bCanUseMultitoolWhenActivated && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Activated) || Door->AnyBottomDoorChunksBroken());
					const bool bCanDisarmTrap = Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live;
					const bool bCanGiveDisarmTrapCommand = bCanDisarmTrap && (bSameSideAsTrap || (!bSameSideAsTrap && (Door->IsOpen() || Door->TeamKnowsDoorTrapState(false))));

					const ESwatCommand DoorCommand = (bCanGiveDisarmTrapCommand ? ESwatCommand::SC_DisarmTrap : Door->IsDoorwayOnly() ? ESwatCommand::SC_MoveAndClear : (Door->IsOpen() ? DefaultDoorOpenCommand : bDoorCanCheck ? DefaultCheckDoorCommand : bDoorUnknownToSWAT ? DefaultDoorUnknownCommand : (bDoorIsLocked ? DefaultDoorLockedCommand : DefaultDoorUnlockedCommand)));

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
							FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Doorway..."), ESwatCommand::SC_None, DoorCommands));
						}
						else
						{
							FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "Door..."), ESwatCommand::SC_None, DoorCommands));
						}
						
						if (OtherDoor->IsDoorwayOnly())
						{
							FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "OtherDoorway..."), ESwatCommand::SC_None, OtherDoorCommands));
						}
						else
						{
							FinalCommands.Add(FSwatCommand(FText::FromStringTable("SwatCommandTable", "OtherDoor..."), ESwatCommand::SC_None, OtherDoorCommands));
						}
					}
					else
					{
						FinalCommands = DoorCommands;
					}

					PreviousActiveCommandPage = ActiveCommandPage;
					ActiveCommandPage = FinalCommands;
				}
			}
			else if (AIncapacitatedHuman* IncapacitatedHuman = Cast<AIncapacitatedHuman>(ContextualData.GetActor()))
			{
				if (!IncapacitatedHuman->HasBeenReported())
				{
					if (SwatManager)
						SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoReportTarget;
				}
			}
			else if (ATrapActor* TrapActor = Cast<ATrapActor>(ContextualData.GetActor()))
			{
				if (!TrapActor->IsHidden())
				{
					if (SwatManager)
						SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DisarmStandaloneTrap;
				}
			}
			else if (ABaseItem* BaseItem = Cast<ABaseItem>(ContextualData.GetActor()))
			{
				if (BaseItem->IsEvidence() && !BaseItem->IsHidden())
				{
					if (SwatManager)
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
						if (SwatManager)
							SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoCollectEvidence;

						LocalPlayerCharacter->bLookingAtEvidenceItem = true;
					}
				}
			}
			else if (AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(ContextualData.GetActor()))
			{
				if (!EvidenceActor->IsHidden())
				{
					if (SwatManager)
						SwatManager->CurrentDefaultCommand = ESwatCommand::SC_DoCollectEvidence;

					LocalPlayerCharacter->bLookingAtEvidenceItem = true;
				}
			}
			else if (AExfilActor* ExfilActor = Cast<AExfilActor>(ContextualData.GetActor()))
			{
				if (SwatManager)
					SwatManager->CurrentDefaultCommand = ESwatCommand::SC_MoveTo;
			}
		}
	}

	// Manual Training/ActivityTriggerVolume Override
	UpdateTrainingDefaultCommand();
	BuildTrainingPageData(ActiveCommandPage);

	OnPostUpdateSwatCommands.Broadcast(this, ActiveCommandPage);
	if (RequiresPageViewUpdate())
	{
		SetLastCommandPage(ActiveCommandPage);
		OnPageViewUpdate();
	}
}

void USwatCommandWidget::IssueIncapHumanDefaultCommand(AIncapacitatedHuman* IncapHuman)
{
	if (!IncapHuman)
		return;
	
	if (!IncapHuman->HasBeenReported())
	{
		ESwatCommand IncapHumanDefaultCommand = ESwatCommand::SC_DoReportTarget;
		FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(IncapHumanDefaultCommand);

		FSwatCommand DefaultReportSwatCommand = FSwatCommand(CommandAsString, IncapHumanDefaultCommand, false);
		
		ExecuteCommand(DefaultReportSwatCommand, true);
	}
}

void USwatCommandWidget::IssueHumanDefaultCommand(ACyberneticCharacter* AICharacter)
{
	if (!AICharacter)
		return;
	
	ESwatCommand DefaultHumanSwatCommand = ((AICharacter->IsArrested() || AICharacter->IsDeadOrUnconscious() || AICharacter->IsPlayingDead()) && !AICharacter->HasBeenReported() ? ESwatCommand::SC_DoReportTarget : (AICharacter->CanArrest() && !AICharacter->IsPlayingDead() ? ESwatCommand::SC_DoArrestTarget : ESwatCommand::SC_None));
	if (DefaultHumanSwatCommand == ESwatCommand::SC_None)
	{
		IssueGlobalDefaultCommand();
		return;
	}
	
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(DefaultHumanSwatCommand);

	FSwatCommand HumanSwatCommand = FSwatCommand(CommandAsString, DefaultHumanSwatCommand, false, true);
	
	ExecuteCommand(HumanSwatCommand, true);
}

void USwatCommandWidget::IssueDoorDefaultCommand(ADoor* Door)
{
	if (!Door)
		return;
	
	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!LocalPlayerCharacter)
		return;
	
	const bool bDoorUnknownToSWAT = !Door->GetSWATKnowsLockState();
	const bool bDoorIsLocked = Door->GetSWATKnowsLockState() && Door->IsLocked();
	const bool bDoorCanCheck = bDoorUnknownToSWAT && IsTeamStackedUpOnDoor(Door);
	
	const bool bTrapInFront = Door->GetAttachedTrap() ? Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation()) : false;
	const bool bPlayerInFront = Door->IsPointInFrontOfDoorway(LocalPlayerCharacter->GetActorLocation());
	
	const bool bSameSideAsTrap = bTrapInFront == bPlayerInFront;
	
	const bool bCanIssueOrderOnThisSide = (Door->bCanIssueOrdersOnFrontSide && bPlayerInFront) || (Door->bCanIssueOrdersOnBackSide && !bPlayerInFront);
	if (!bCanIssueOrderOnThisSide)
		return;
	
	//const bool bCanDisarmTrap = Door->GetAttachedTrap() && ((Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live && (Door->DoesSWATKnowTrapState() || bTrapInFront == bPlayerInFront || Door->IsOpen())) || (Door->GetAttachedTrap()->bCanUseMultitoolWhenActivated && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Activated) || Door->AnyBottomDoorChunksBroken());
	const bool bCanDisarmTrap = Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live;
	const bool bCanGiveDisarmTrapCommand = bCanDisarmTrap && (bSameSideAsTrap || (!bSameSideAsTrap && (Door->IsOpen() || Door->TeamKnowsDoorTrapState(false))));

    const ESwatCommand DoorCommand = (bCanGiveDisarmTrapCommand ? ESwatCommand::SC_DisarmTrap : (Door->IsDoorwayOnly() ? ESwatCommand::SC_MoveAndClear : Door->IsOpen() ? DefaultDoorOpenCommand : bDoorCanCheck ? DefaultCheckDoorCommand : bDoorUnknownToSWAT ? DefaultDoorUnknownCommand : (bDoorIsLocked ? DefaultDoorLockedCommand : DefaultDoorUnlockedCommand)));
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(DoorCommand);

	FSwatCommand DefaultDoorSwatCommand = FSwatCommand(CommandAsString, DoorCommand, false);
	
	ExecuteCommand(DefaultDoorSwatCommand, true);
}

void USwatCommandWidget::IssueCollectEvidenceCommand(AActor* EvidenceActor)
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	if (!EvidenceActor)
		return;
	
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(ESwatCommand::SC_DoCollectEvidence);
	FSwatCommand CollectEvidenceCommand = FSwatCommand(CommandAsString, ESwatCommand::SC_DoCollectEvidence, EvidenceActor, true);
			
	ExecuteCommand(CollectEvidenceCommand, true);
}

void USwatCommandWidget::IssueDisarmTrapCommand(ATrapActor* TrapActor)
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	if (!TrapActor)
		return;
	
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(ESwatCommand::SC_DisarmStandaloneTrap);
	FSwatCommand DisarmtrapCommand = FSwatCommand(CommandAsString, ESwatCommand::SC_DisarmStandaloneTrap, TrapActor, true);
			
	ExecuteCommand(DisarmtrapCommand, true);
}

void USwatCommandWidget::IssueDisarmTrapCommand(ATrapActorAttachedToDoor* TrapActor)
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	if (!TrapActor)
		return;
	
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(ESwatCommand::SC_DisarmTrap);
	FSwatCommand DisarmtrapCommand = FSwatCommand(CommandAsString, ESwatCommand::SC_DisarmTrap, TrapActor, true);
			
	ExecuteCommand(DisarmtrapCommand, true);
}

void USwatCommandWidget::IssueExfilCommand(AExfilActor* ExfilActor)
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	if (!ExfilActor)
		return;
	
	FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(ESwatCommand::SC_MoveTo);
	FSwatCommand ExfilCommand = FSwatCommand(CommandAsString, ESwatCommand::SC_MoveTo, ExfilActor, true);
			
	ExecuteCommand(ExfilCommand, true);
}

void USwatCommandWidget::IssueGlobalDefaultCommand()
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;

	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;
	
	if (SwatManager->CurrentDefaultCommand == ESwatCommand::SC_Hold)
	{
		if (SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType))
		{
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_Resume;
		}
	}
	else if (SwatManager->CurrentDefaultCommand == ESwatCommand::SC_Resume)
	{
		if (!SwatManager->IsSWATTeamHoldingPosition(ActiveTeamType))
		{
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_Hold;
		}
	}

	const FText CommandAsString = UReadyOrNotFunctionLibrary::SwatCommandToText(SwatManager->CurrentDefaultCommand);
	const FSwatCommand DefaultGlobalSwatCommand = FSwatCommand(CommandAsString, SwatManager->CurrentDefaultCommand, true);

	ExecuteCommand(DefaultGlobalSwatCommand, true);
}

void USwatCommandWidget::UpdateTrainingDefaultCommand() const
{
	ATrainingGM* TrainingGameMode = GetWorld()->GetAuthGameMode<ATrainingGM>();
	if (!TrainingGameMode)
		return;
	
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;

	const TArray<FSwatCommandData> CommandsToIssue = TrainingGameMode->GetCurrentCommandsToIssue();

	// If there are no active/incomplete "Issue SWAT Command" activities, then we can't override anything
	if (CommandsToIssue.Num() == 0)
	{
		// Unless the Player's Command Menu is locked then override the default command to None
		APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
		if (LocalPlayerCharacter && LocalPlayerCharacter->IsCommandMenuLocked())
		{
			SwatManager->CurrentDefaultCommand = ESwatCommand::SC_None;
		}
		return;
	}

	for (const FSwatCommandData& CommandToIssue : CommandsToIssue)
	{
		const bool bCorrectCommand = CommandToIssue.Command == SwatManager->CurrentDefaultCommand;
		const bool bCorrectTeam = CommandToIssue.Team == SwatManager->ActiveCommandTeam || CommandToIssue.Team == ETeamType::TT_NONE;
		const bool bCorrectTarget = CommandToIssue.Target.Get() == ContextualData.GetActor() || CommandToIssue.Target.Get() == nullptr;
		const bool bCorrectQueue = !CommandToIssue.bQueue;

		if (bCorrectCommand && bCorrectTeam && bCorrectTarget && bCorrectQueue)
			return;
	}

	// Failsafe to prevent the Player from queueing a command that is not valid for the current activity
	if (HasQueuedCommandForActiveTeam() && CommandsToIssue.Num() == 0)
	{
		SwatManager->CurrentDefaultCommand = ESwatCommand::SC_Execute;
		return;
	}

	// If the current default command does not match any command to issue, then override it to None
	SwatManager->CurrentDefaultCommand = ESwatCommand::SC_None;
}

void USwatCommandWidget::CanCommandBeIssuedForActivities(FSwatCommand& Command, const TArray<FSwatCommandData>& CommandsToIssue)
{
	// Skip already disabled commands (so we don't enable impossible commands)
	if (!Command.bEnabled)
		return;

	const bool bIsParentCommand = Command.CommandType == ESwatCommand::SC_None && Command.SubCommands.Num() > 0;
	if (bIsParentCommand)
	{
		// Recursively check all sub-commands
		for (FSwatCommand& SubCommand : Command.SubCommands)
		{
			CanCommandBeIssuedForActivities(SubCommand, CommandsToIssue);
		}

		for (const FSwatCommand& SubCommand : Command.SubCommands)
		{
			// Return early if at least one sub-command is enabled
			if (SubCommand.bEnabled)
				return;
		}

		// Disable parent command if no sub-commands are enabled
		Command.bEnabled = false;
		return;
	}

	for (const FSwatCommandData& CommandToIssue : CommandsToIssue)
	{
		const bool bCorrectCommand = CommandToIssue.Command == Command.CommandType;
		const bool bCorrectTeam = CommandToIssue.Team == ActiveTeamType || CommandToIssue.Team == ETeamType::TT_NONE;
		const bool bCorrectTarget = CommandToIssue.Target.Get() == ContextualData.GetActor() || CommandToIssue.Target.Get() == nullptr;
		const bool bCorrectQueue = CommandToIssue.bQueue == bHoldingQueueCommandKey;

		// Don't disable the command if it is one of the current commands to issue
		if (bCorrectCommand && bCorrectTeam && bCorrectTarget && bCorrectQueue)
			return;
	}

	// Failsafe to prevent the Player from queueing a command that is not valid for the current activity
	if (HasQueuedCommandForActiveTeam() && Command.CommandType == ESwatCommand::SC_Execute && CommandsToIssue.Num() == 0)
		return;

	// Disable the command if it is not one of the current commands to issue
	Command.bEnabled = false;
}

void USwatCommandWidget::DoCommand(FSwatCommand Command, bool bFromQueue, ETeamType ActiveTeamOverride, FHitResult ContextualDataOverride, bool bOverrideContextualData)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	
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
		V_LOGM(LogReadyOrNot, "Executing command on %s (Override: %d)", *ContextualActor->GetName(), bOverrideContextualData);
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

	DrawDebugPoint(GetWorld(), Location, 15.0f, FColor::Green, false, 10.0f);

	LastExecutedCommandContextualData = ContextualData;

	if (SwatManager)
		SwatManager->TimeSincePlayerIssuedCommand = 0.0f;
	
	ACyberneticCharacter* ContextAI = Cast<ACyberneticCharacter>(ContextualActor);

	switch (Command.CommandType)
	{
		case ESwatCommand::SC_MoveTo: 								SwatManager->GiveMoveCommand(ExecutionTeamType, Location); break;
		case ESwatCommand::SC_FallIn: 								SwatManager->GiveFallInCommand(ExecutionTeamType); break;
		case ESwatCommand::SC_FallIn_Snake: 						SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Snake); break;
		case ESwatCommand::SC_FallIn_HalfSnake: 					SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::HalfSnake); break;
		case ESwatCommand::SC_FallIn_Diamond: 						SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Diamond); break;
		case ESwatCommand::SC_FallIn_Flock: 						SwatManager->GiveFallInCommand(ExecutionTeamType, EFallInPattern::Flock); break;
		case ESwatCommand::SC_Cover: 								SwatManager->GiveCoverAreaCommand(ExecutionTeamType, Location); break;
		case ESwatCommand::SC_Hold: 								SwatManager->GiveHoldCommand(ExecutionTeamType); break;
		case ESwatCommand::SC_Resume: 								SwatManager->RemoveHoldCommand(ExecutionTeamType); break;
		case ESwatCommand::SC_DeployFlashbang: 						SwatManager->GiveDeployGrenadeAtLocation(ExecutionTeamType, ContextualData.Location, Flashbang); break;
		case ESwatCommand::SC_DeployStinger: 						SwatManager->GiveDeployGrenadeAtLocation(ExecutionTeamType, ContextualData.Location, Stinger); break;
		case ESwatCommand::SC_DeployCSGas: 							SwatManager->GiveDeployGrenadeAtLocation(ExecutionTeamType, ContextualData.Location, CSGas); break;
		case ESwatCommand::SC_DeployChemlight: 						SwatManager->GiveDropChemlightAtLocation(ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_DoCollectEvidence: 					SwatManager->GiveCollectEvidenceCommand(ContextualActor, ExecutionTeamType); break;
		case ESwatCommand::SC_DoArrestTarget: 						SwatManager->GiveRestrainCommand(ContextualActor, ExecutionTeamType, Location); break;
		case ESwatCommand::SC_DoReportTarget: 						SwatManager->GiveReportTargetCommand(ContextualActor, ExecutionTeamType); break;
		case ESwatCommand::SC_DisarmStandaloneTrap: 				SwatManager->GiveDisarmStandaloneTrapCommand(ContextualActor, ExecutionTeamType); break;
		case ESwatCommand::SC_KillMe:
			UAchievementStatics::UnlockAchievement(GetWorld(), EAchievement::THE_DEVIL, false);
			SwatManager->GiveMoveCommand(ExecutionTeamType, Location); break;
		case ESwatCommand::SC_StackUp: 								SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.ImpactNormal, true); break;
		case ESwatCommand::SC_StackUpSplit: 						SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.ImpactNormal, true, EStackUpStyle::Split); break;
		case ESwatCommand::SC_StackUpLeft: 							SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.ImpactNormal, true, EStackUpStyle::Left); break;
		case ESwatCommand::SC_StackUpRight: 						SwatManager->GiveStackUpCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.ImpactNormal, true, EStackUpStyle::Right); break;
		case ESwatCommand::SC_PickLock: 							SwatManager->GivePickLockCommand(ContextualActor, ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_RemoveDoorJam: 						SwatManager->GiveRemoveWedgeCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal); break;
		case ESwatCommand::SC_DeployMirrorgun: 						SwatManager->GiveCheckForContactsCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal); break;
		case ESwatCommand::SC_DeployDoorJam: 						SwatManager->GiveWedgeDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal); break;
		case ESwatCommand::SC_CheckForTrap: 						SwatManager->GiveCheckForTrapsCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, ContextualData.Normal); break;
		case ESwatCommand::SC_DisarmTrap: 							SwatManager->GiveDisarmTrapOnDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_CloseDoor: 							SwatManager->GiveCloseDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_OpenDoor: 							SwatManager->GiveOpenDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_Slide: 								SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, EDoorScanMethod::Slide); break;
		case ESwatCommand::SC_Slice: 								SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, EDoorScanMethod::Slice); break;
		case ESwatCommand::SC_Snap: 								SwatManager->GiveScanDoorCommand(ContextualActor, ExecutionTeamType, ContextualData.Location, EDoorScanMethod::Snap); break;
		case ESwatCommand::SC_SearchAndSecure: 						SwatManager->GiveSearchAndSecureCommand(ExecutionTeamType, ContextualData.Location); break;
		case ESwatCommand::SC_SearchAndSecureRoom: 					SwatManager->GiveSearchAndSecureCommand(ExecutionTeamType, ContextualData.Location, true); break;
		case ESwatCommand::SC_SearchAndSecureRoom_Individual:		SwatManager->GiveSearchAndSecureCommand_Individual(ContextualActor, ContextualData.Location, true); break;
		case ESwatCommand::SC_MoveAndClear: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, nullptr); break;
		case ESwatCommand::SC_MoveAndClearFlashbang: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, Flashbang); break;
		case ESwatCommand::SC_MoveAndClearStinger: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, Stinger); break;
		case ESwatCommand::SC_MoveAndClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, CSGas); break;
		case ESwatCommand::SC_MoveAndClearLauncher: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass()); break;
		case ESwatCommand::SC_MoveAndClearLeader: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Move, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_OpenAndClear: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, nullptr); break;
		case ESwatCommand::SC_OpenAndClearFlashbang: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, Flashbang); break;
		case ESwatCommand::SC_OpenAndClearStinger: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, Stinger); break;
		case ESwatCommand::SC_OpenAndClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, CSGas); break;
		case ESwatCommand::SC_OpenAndClearLauncher: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass()); break;
		case ESwatCommand::SC_OpenAndClearLeader: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Open, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_KickAndClear: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, nullptr); break;
		case ESwatCommand::SC_KickAndClearFlashbang: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, Flashbang); break;
		case ESwatCommand::SC_KickAndClearStinger: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, Stinger); break;
		case ESwatCommand::SC_KickAndClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, CSGas); break;
		case ESwatCommand::SC_KickAndClearLauncher: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass()); break;
		case ESwatCommand::SC_KickAndClearLeader: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Kick, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_ShotgunClear: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, nullptr, ABreachingShotgun::StaticClass()); break;
		case ESwatCommand::SC_ShotgunClearFlashbang:				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, Flashbang, ABreachingShotgun::StaticClass()); break;
		case ESwatCommand::SC_ShotgunClearStinger: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, Stinger, ABreachingShotgun::StaticClass()); break;
		case ESwatCommand::SC_ShotgunClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, CSGas, ABreachingShotgun::StaticClass()); break;
		case ESwatCommand::SC_ShotgunClearLauncher: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass(), ABreachingShotgun::StaticClass()); break;
		case ESwatCommand::SC_ShotgunClearLeader: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Shotgun, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_RamAndClear: 							SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, nullptr, ADoorRam::StaticClass()); break;
		case ESwatCommand::SC_RamAndClearFlashbang: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, Flashbang, ADoorRam::StaticClass()); break;
		case ESwatCommand::SC_RamAndClearStinger: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, Stinger, ADoorRam::StaticClass()); break;
		case ESwatCommand::SC_RamAndClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, CSGas, ADoorRam::StaticClass()); break;
		case ESwatCommand::SC_RamAndClearLauncher: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass(), ADoorRam::StaticClass()); break;
		case ESwatCommand::SC_RamAndClearLeader: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Ram, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_C2Clear: 								SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, nullptr, AC2Explosive::StaticClass()); break;
		case ESwatCommand::SC_C2ClearFlashbang: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, Flashbang, AC2Explosive::StaticClass()); break;
		case ESwatCommand::SC_C2ClearStinger: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, Stinger, AC2Explosive::StaticClass()); break;
		case ESwatCommand::SC_C2ClearCSGas: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, CSGas, AC2Explosive::StaticClass()); break;
		case ESwatCommand::SC_C2ClearLauncher: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass(), AC2Explosive::StaticClass()); break;
		case ESwatCommand::SC_C2ClearLeader: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::C2, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, false, true); break;
		case ESwatCommand::SC_LeaderAndClear: 						SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, true, false); break;
		case ESwatCommand::SC_LeaderAndClearFlashbang: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, Flashbang, nullptr, true, false); break;
		case ESwatCommand::SC_LeaderAndClearStinger: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, Stinger, nullptr, true, false); break;
		case ESwatCommand::SC_LeaderAndClearCSGas: 					SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, CSGas, nullptr, true, false); break;
		case ESwatCommand::SC_LeaderAndClearLauncher: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, AGrenadeLauncher::StaticClass(), nullptr, true, false); break;
		case ESwatCommand::SC_LeaderAndClearLeader: 				SwatManager->GiveBreachAndClearCommand(Cast<ADoor>(ContextualActor), EDoorBreachType::Leader, ExecutionTeamType, ContextualData.Location, nullptr, nullptr, true, true); break;
		case ESwatCommand::SC_DeployTaser:							SwatManager->GiveDeployNonLethalItemAtTargetCommand(Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Taser); break;
		case ESwatCommand::SC_DeployPepperspray:					SwatManager->GiveDeployNonLethalItemAtTargetCommand(Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_OCSpray); break;
		case ESwatCommand::SC_DeployPepperball:						SwatManager->GiveDeployNonLethalItemAtTargetCommand(Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Pepperball); break;
		case ESwatCommand::SC_DeployBeanbag:						SwatManager->GiveDeployNonLethalItemAtTargetCommand(Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_Beanbag); break;
		case ESwatCommand::SC_MeleeTarget:							SwatManager->GiveDeployNonLethalItemAtTargetCommand(Cast<AReadyOrNotCharacter>(ContextualActor), ExecutionTeamType, EItemCategory::IC_None); break;
		case ESwatCommand::SC_DeployShield:
		case ESwatCommand::SC_HolsterShield:
			if (ContextAI)
			{
				if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
				{
					if (ACyberneticCharacter* Character = Controller->GetCharacter())
					{
						if (Character->GetInventoryComponent()->IsEquippingItem())
							break;
						
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
					ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
				}
			}
		break;
		case ESwatCommand::SC_Focus_MyPosition_Individual:
			if (ContextAI)
			{
				if (ACyberneticController* Controller = ContextAI->GetCyberneticsController())
				{
					LOCAL_PLAYER;
					Controller->GetTargetingComp()->CustomFocusActor = nullptr;
					Controller->GetTargetingComp()->CustomFocusLocation = LocalPlayer->GetActorLocation();
					ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
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
						
						ContextAI->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC : VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND);
					}
				}
			}
		break;
		case ESwatCommand::SC_MoveTo_Individual: // todo: dont allow this if swat is moving in to arrest them
			if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
			{
				Player->Server_GiveAIMoveTo(ContextAI, Location);
			}
		break;
		case ESwatCommand::SC_MoveTo_MyPosition_Individual:
			if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
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
			if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
			{
				Player->Server_StopAIMoveTo(ContextAI);
			}
		break;
		
		case ESwatCommand::SC_MoveTo_Exit_Individual:
			if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
			{
				Player->Server_GiveAIMoveToExit(ContextAI);
			}
		break;
		case ESwatCommand::SC_TurnAround_Individual:
			if (APlayerCharacter* Player = GetOwningPlayerPawn<APlayerCharacter>())
			{
				Player->Server_GiveAITurnAroundOrder(ContextAI);
			}
		break;
		case ESwatCommand::SC_None: break;
		default: break;
	}

	OnSwatCommandIssued.Broadcast(Command.CommandType, ActiveTeamOverride, ContextualActor);
}

bool USwatCommandWidget::GetSubCommands(FSwatCommand Command, TArray<FSwatCommand>& OutSubCommands)
{
	if (Command.SubCommands.Num() > 0)
	{
		OutSubCommands = Command.SubCommands;
		return true;
	}
	
	return false;
}

bool USwatCommandWidget::DoesAnySwatTeamHaveItem(TSubclassOf<ABaseItem> Item) const
{
	for (TActorIterator<ASWATCharacter>It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Swat = *It;
		if (Swat->IsDeadOrUnconscious())
			continue;

		if (Swat->GetInventoryComponent()->GetInventoryItemOfClass(Item))
		{
			return true;
		}
	}
	return false;
}

bool USwatCommandWidget::DoesAnySwatTeamHaveItemType(EItemCategory ItemType) const
{
	for (TActorIterator<ASWATCharacter>It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Swat = *It;
		if (Swat->IsDeadOrUnconscious())
			continue;

		if (Swat->GetInventoryComponent()->GetInventoryItemOfType(ItemType))
		{
			return true;
		}
	}
	return false;
}

bool USwatCommandWidget::DoesSwatTeamHaveItem(ETeamType SwatTeam, TSubclassOf<ABaseItem> Item) const
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return false;
	
	if (SwatManager->GetSWATCount() == 0)
		return true; // allow it so we can use all the commands in coop multiplayer
	
	return SwatManager->GetSwatWithItem(SwatTeam, Item) != nullptr;
}

bool USwatCommandWidget::DoesSwatTeamHaveItemType(ETeamType SwatTeam, EItemCategory ItemType) const
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return true;
	
	if (SwatManager->GetSWATCount() == 0)
		return true; // allow it so we can use all the commands in coop multiplayer
	
	return SwatManager->GetSwatWithItemType(SwatTeam, ItemType) != nullptr;
}

bool USwatCommandWidget::DoesLeaderHaveItemType(EItemCategory ItemType) const
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return false;
	
	if (SwatManager->GetSWATCount() == 0)
		return true; // allow it so we can use all the commands in coop multiplayer
	
	if (const AReadyOrNotCharacter* SquadLeader = SwatManager->GetSquadLeader())
	{
		return SquadLeader->GetInventoryComponent()->GetInventoryItemOfType(ItemType) != nullptr;
	}
	
	return false;
}

FKey USwatCommandWidget::GetCommandInput(FName InName) const
{
	if (const UInputSettings* Settings = GetDefault<UInputSettings>())
	{
		for (const FInputActionKeyMapping& Mapping : Settings->GetActionMappings())
		{
			if (Mapping.ActionName == InName)
			{
				return Mapping.Key;
			}
		}
	}

	return FKey();
}

FKey USwatCommandWidget::GetInputOne() const
{
	return GetCommandInput("SwatInputKeyOne");
}

FKey USwatCommandWidget::GetInputTwo() const
{
	return GetCommandInput("SwatInputKeyTwo");
}

FKey USwatCommandWidget::GetInputThree() const
{
	return GetCommandInput("SwatInputKeyThree");
}

FKey USwatCommandWidget::GetInputFour() const
{
	return GetCommandInput("SwatInputKeyFour");
}

FKey USwatCommandWidget::GetInputFive() const
{
	return GetCommandInput("SwatInputKeyFive");
}

FKey USwatCommandWidget::GetInputSix() const
{
	return GetCommandInput("SwatInputKeySix");
}

FKey USwatCommandWidget::GetInputSeven() const
{
	return GetCommandInput("SwatInputKeySeven");
}

FKey USwatCommandWidget::GetInputEight() const
{
	return GetCommandInput("SwatInputKeyEight");
}

FKey USwatCommandWidget::GetInputNine() const
{
	return GetCommandInput("SwatInputKeyNine");
}

FKey USwatCommandWidget::GetInputBack() const
{
	return GetCommandInput("SwatInputKeyBack");
}

FKey USwatCommandWidget::ConvertIntToInputKey(const int32 Int) const
{
	switch (Int)
	{
		case 1: return GetInputOne();
		case 2: return GetInputTwo();
		case 3: return GetInputThree();
		case 4: return GetInputFour();
		case 5: return GetInputFive();
		case 6: return GetInputSix();
		case 7: return GetInputSeven();
		case 8: return GetInputEight();
		case 9: return GetInputNine();
		default: return EKeys::Invalid;
	}
}

void USwatCommandWidget::OnOpen_Implementation()
{
	SwatCommandIssued->StopAllAnimations();
	SwatCommandIssued->SetVisibility(ESlateVisibility::Collapsed);
}

void USwatCommandWidget::OnClose_Implementation()
{
}

void USwatCommandWidget::OnInputKey_Implementation()
{
}

void USwatCommandWidget::OnCommandIssued_Implementation(int32 Index, const FSwatCommand& Command, const bool bFromDefault)
{
	SwatCommandIssued->StopAllAnimations();
	
	if (bFromDefault)
	{
		SwatCommandIssued->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	InitEntryWidget(SwatCommandIssued, Command, GetActiveTeam(), Index == ActiveCommandPage.Num()-1);

	SwatCommandIssued->SetText(LastSubCommandPageIndex == 0 || Command.bCommandTextAsIssuedText ? Command.CommandText : UReadyOrNotFunctionLibrary::SwatCommandToText(Command.CommandType));

	FWidgetTransform T;
	T.Translation = UReadyOrNotFunctionLibrary::GetViewportPositionOfWidget(GetOwningPlayer(), VB_Commands, IndexToEntryWidget(Index));
	SwatCommandIssued->SetRenderTransform(T);

	SwatCommandIssued->PlayFlashAnimation();
}

void USwatCommandWidget::InputKey(FKey Key, bool bAddToComboKeys)
{
	if (Key == GetInputBack())
	{
		if (LastSubCommandPageIndex > 0)
		{
			if (CommandCombo.Num() > 0)
				CommandCombo.RemoveAt(CommandCombo.Num() - 1);

			LastSubCommandPageIndex = FMath::Clamp(LastSubCommandPageIndex - 1, 0, LastSubCommandPageIndex);
			ActiveCommandPage = PreviousActiveCommandPage;
			bHoldPageUntilExecuted = false;
			
			if (ParentCommands.Num() > 0)
				ParentCommands.Pop();
			
			SetLastCommandPage(ActiveCommandPage);

			UpdateCommandPageData();
			OnPageViewUpdate();
			
			UFMODBlueprintStatics::PlayEvent2D(this, OpenSubCommandMenuEvent, true);
		}

		OnInputKey();
		return;
	}

	for (FSwatCommand SwatCommand : ActiveCommandPage)
	{	
		if (Key == SwatCommand.InputKey && SwatCommand.bEnabled)
		{
			GrabContextData();
			
			if (SwatCommand.SubCommands.Num() == 0)
			{
				if (CanQueue())
				{
					QueueCommand(SwatCommand);
				}
				else
				{
					ExecuteCommand(SwatCommand);
				}
				
				OnInputKey();
				
				break;
			}

			if (bAddToComboKeys)
				CommandCombo.Add(Key);

			LastSubCommandPageIndex++;
			ParentCommands.Push(SwatCommand);
			PreviousActiveCommandPage = ActiveCommandPage;
			ActiveCommandPage = SwatCommand.SubCommands;
			LastActorBeforeGoingIntoSubPage = ContextualData.GetActor();
			bHoldPageUntilExecuted = SwatCommand.bHoldPageUntilExecute;
			
			if (RequiresPageViewUpdate())
			{
				SetLastCommandPage(ActiveCommandPage);
				OnPageViewUpdate();
			}

			UFMODBlueprintStatics::PlayEvent2D(this, OpenSubCommandMenuEvent, true);

			OnInputKey();
			break;
		}
	}
}

void USwatCommandWidget::OpenCommandMenu()
{
	USWATManager* SwatManager = USWATManager::Get(this);
	
	if (SwatManager)
	{
		//const bool bIsSinglePlayer = GetWorld()->GetNetDriver() == nullptr;
		
		//if (bIsSinglePlayer && SwatManager->IsSWATTeamDead())
			//return;
		
		SwatManager->WaitingReplyDelay = SwatManager->MaxWaitingReplyDelay;
	}

	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!LocalPlayerCharacter)
		return;
	
	if (LocalPlayerCharacter->bMirroring)
	{
		return;
	}

	if (LocalPlayerCharacter->bIsSwatCommandOpen)
		return;
	
	LocalPlayerCharacter->bIsSwatCommandOpen = true;

	CommandCombo.Empty();
	LastSubCommandPageIndex = 0;
	ParentCommands.Empty();
	ContextualData = {};
	ContextualData2 = {};
	bHoldPageUntilExecuted = false;
	LastContextActor = nullptr;

	UpdateCommandPageData();

	SizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RequiresPageViewUpdate())
	{
		SetLastCommandPage(ActiveCommandPage);
		OnPageViewUpdate();
	}

	UFMODBlueprintStatics::PlayEvent2D(this, OpenMenuEvent, true);

	if (SwatManager && SwatManager->GetSWATCount() > 0)
	{
		if (!bOverrideActiveTeamType && SwatManager->PrefixCooldown <= 0.0f)
		{
			if (PreviousActiveTeamType != ActiveTeamType || !bHasEverOpened)
			{
				FString VO_Prefix = "";
				switch (ActiveTeamType)
				{
					case ETeamType::TT_SERT_RED:		VO_Prefix = VO_SWAT_GENERAL::CALL_RED_TEAM; break;
					case ETeamType::TT_SERT_BLUE:		VO_Prefix = VO_SWAT_GENERAL::CALL_BLUE_TEAM; break;
					case ETeamType::TT_SQUAD:			VO_Prefix = VO_SWAT_GENERAL::CALL_GOLD_TEAM; break;
					default: break;
				}
				
				SwatManager->PrefixCooldown = 0.6f;

				SwatManager->GetSquadLeader()->PlayRawVO(VO_Prefix);
			}
		}
	}

	OnOpen();
	
	bHasEverOpened = true;
}

void USwatCommandWidget::CloseCommandMenu()
{
	SizeBox->SetVisibility(ESlateVisibility::Collapsed);

	PreviousActiveTeamType = ActiveTeamType;
	
	if (APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		LocalPlayerCharacter->bDisableInventoryInput = false;
		
		if (LocalPlayerCharacter->bIsSwatCommandOpen)
		{
			LocalPlayerCharacter->bIsSwatCommandOpen = false;
			LocalPlayerCharacter->bDisableInventoryInput = true;
			UReadyOrNotFunctionLibrary::StartTimerForCallback(LocalPlayerCharacter, &APlayerCharacter::EnableInventoryInput, 0.3f);
		}
	}
	
	OnClose();
}

void USwatCommandWidget::GrabContextData(FCollisionQueryParams CollisionQueryParams)
{
	AReadyOrNotGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AReadyOrNotGameState>() : nullptr;
	
	if (APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (!LocalPlayerCharacter->GetFirstPersonCameraComponent())
			return;
		
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

				if (LastSubCommandPageIndex > 0) // ignore everyone if we're in a submenu
				{
					CollisionQueryParams.AddIgnoredActor(Character);
					continue;
				}

				if ((Character->IsDeadOrUnconscious() || Character->IsIncapacitated()) && (Character->IsArrested() || Character->IsArrestedAndDead()))
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

		if (Cast<ASWATCharacter>(LastActorBeforeGoingIntoSubPage) && LastSubCommandPageIndex > 0)
			CollisionQueryParams.AddIgnoredActor(LastActorBeforeGoingIntoSubPage);
		
		if (const USWATManager* SwatManager = USWATManager::Get(this))
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)SwatManager->SwatTrailers);
		
		const FVector TraceStart = LocalPlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
		const FVector TraceEnd = TraceStart + LocalPlayerCharacter->GetFirstPersonCameraComponent()->GetForwardVector() * 10000.0f;
		
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_ITEM);

		// Multi line trace to support targeting specific actors through open doors and doorways
		TArray<FHitResult> HitResults;
		GetWorld()->LineTraceMultiByObjectType(HitResults, TraceStart, TraceEnd, CollisionObjectQueryParams, CollisionQueryParams);
		
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
			if ((!HitResults[0].GetActor() && HitResults[0].bBlockingHit) || (HitResults[0].GetActor() && !HitResults[0].GetActor()->Implements<UCanIssueCommandOn>()))
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
						const float Distance = FVector::Distance(FoundDoor->CalculateClosestPoint(LocalPlayerCharacter->GetActorLocation()), LocalPlayerCharacter->GetActorLocation());
						
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
				(Hit.GetActor() && !Hit.GetActor()->Implements<UCanIssueCommandOn>())) // walls, floors, anything static that blocks
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
						// ##UE5UPGRADE## Compatibility
						FoundDoorHitResult->HitObjectHandle = Door->GetSubDoor();


					const bool bCanTechnicallySee = !Door->IsDoorwayOnly() && Door->IsOpenAtOrBeyond(0.75f) && !Door->IsActorRightOfDoorway(LocalPlayerCharacter);
					
					if (Door->IsDoorwayOnly() || bCanTechnicallySee)
					{
						if (HitResults.Num() > 1 && HitResults[1].GetActor())
						{
							if (ADoor* FoundDoor = Cast<ADoor>(HitResults[1].GetActor()))
							{
								if (FoundDoor != Door && FoundDoor != Door->GetSubDoor())
								{
									if (HitResults[1].GetActor()->Implements<UCanIssueCommandOn>() && ICanIssueCommandOn::Execute_CanIssueCommand(HitResults[1].GetActor()))
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
				if (Element.GetActor()->Implements<UCanIssueCommandOn>() && ICanIssueCommandOn::Execute_CanIssueCommand(Element.GetActor()))
				{
					FHitResult HitResult;
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
					CollisionObjectQueryParams.RemoveObjectTypesToQuery(ECC_DOORWAY);
					GetWorld()->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, CollisionObjectQueryParams, CollisionQueryParams);

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
									continue;
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
		
		LastContextActor = ContextualData.GetActor();

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
					ContextualData = FHitResult();
			}
			
			if (FoundDoorHitResult2)
				ContextualData2 = *FoundDoorHitResult2;
			else
				ContextualData2 = FHitResult();
		}
		else
		{
			ContextualData = *FoundTargetHitResult;
		}
	}
}

void USwatCommandWidget::CycleSwatElement(bool bNext, bool bPlayVO)
{
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;
	
	if (SwatManager->GetSWATCount() == 0)
		return;
	
	if (SwatManager->IsSWATTeamDead())
	{
		ActiveTeamType = ETeamType::TT_NONE;
		SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;

		CloseCommandMenu();

		return;
	}

	TArray<ETeamType> SwatElements = {ETeamType::TT_SERT_RED, ETeamType::TT_SERT_BLUE, ETeamType::TT_SQUAD};
	for (int32 i = 0; i < SwatElements.Num(); i++)
	{
		if (SwatManager->IsSWATTeamDead(SwatElements[i]))
		{
			SwatElements.RemoveAt(i);
			if (i > 0)
				i--;
		}
	}

	ETeamType NewTeamType;
	int32 Idx;
	if (SwatElements.Find(ActiveTeamType, Idx))
	{
		if (bNext)
		{
			if (SwatElements.IsValidIndex(Idx + 1))
			{
				NewTeamType = SwatElements[Idx + 1];
			}
			else
			{
				NewTeamType = SwatElements[0];
			}
		}
		else
		{
			if (SwatElements.IsValidIndex(Idx - 1))
			{
				NewTeamType = SwatElements[Idx - 1];
			}
			else
			{
				NewTeamType = SwatElements[SwatElements.Num() - 1];
			}
		}
	}
	else
	{
		if (SwatElements.Contains(ETeamType::TT_SQUAD))
		{
			NewTeamType = ETeamType::TT_SQUAD;
		} else
		{
			NewTeamType = ETeamType::TT_NONE;
		}
	}
	
	SetActiveTeamElement(NewTeamType);

	if (bPlayVO)
	{
		if (SizeBox->IsVisible() && !bOverrideActiveTeamType)
		{
			if (SwatManager->PrefixCooldown <= 0.0f)
			{
				FString VO_Prefix = "";
				switch (ActiveTeamType)
				{
					case ETeamType::TT_SERT_RED:		VO_Prefix = VO_SWAT_GENERAL::CALL_RED_TEAM; break;
					case ETeamType::TT_SERT_BLUE:		VO_Prefix = VO_SWAT_GENERAL::CALL_BLUE_TEAM; break;
					case ETeamType::TT_SQUAD:			VO_Prefix = VO_SWAT_GENERAL::CALL_GOLD_TEAM; break;
					default: break;
				}

				SwatManager->PrefixCooldown = 0.6f;
				
				const bool bLastVOWasPrefix = (SwatManager->GetSquadLeader()->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_RED_TEAM && SwatManager->ActiveCommandTeam == ETeamType::TT_SERT_RED) ||
												(SwatManager->GetSquadLeader()->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_BLUE_TEAM && SwatManager->ActiveCommandTeam == ETeamType::TT_SERT_BLUE) ||
												(SwatManager->GetSquadLeader()->LastVoiceLinePlayed == VO_SWAT_GENERAL::CALL_GOLD_TEAM && SwatManager->ActiveCommandTeam == ETeamType::TT_SQUAD);
				
				if (!SwatManager->GetSquadLeader()->VoiceSoundSource && !bLastVOWasPrefix)
				{
					SwatManager->GetSquadLeader()->PlayRawVO(VO_Prefix, "", false);
				}
			}
		}
	}
}

void USwatCommandWidget::IssueDefaultCommand()
{
	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (!LocalPlayerCharacter)
		return;
	
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	USWATManager* SwatManager = USWATManager::Get(this);
	if (!SwatManager)
		return;

	if (SwatManager->IsSWATTeamDead())
		return;

	ATrainingGM* TrainingGameMode = GetWorld()->GetAuthGameMode<ATrainingGM>();
	if (TrainingGameMode)
	{
		IssueGlobalDefaultCommand();
		return;
	}

	if (SwatManager->IsCharacterKnownEnemy(LocalPlayerCharacter))
	{
		SwatManager->CurrentDefaultCommand = ESwatCommand::SC_KillMe;

		ExecuteCommand(FSwatCommand(FText::FromStringTable("SwatCommandTable", "KillMe"), ESwatCommand::SC_KillMe, GetOwningPlayerPawn(), false));

		return;
	}

	if (HasQueuedCommandForActiveTeam())
	{
		const FQueuedSwatCommand* QueuedSwatCommand = SwatManager->QueuedSwatCommandMap.Find(ActiveTeamType);
		
		if (ActiveTeamType == ETeamType::TT_SQUAD)
		{
			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_EXECUTE);
			
			if (QueuedSwatCommand)
			{
				DoCommand(QueuedSwatCommand->Command, true, ETeamType::TT_NONE, QueuedSwatCommand->ContextualData, true);
				SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SQUAD);
				SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
				SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
				return;
			}
			
			const FQueuedSwatCommand* QueuedSwatCommand_Red = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SERT_RED);
			const FQueuedSwatCommand* QueuedSwatCommand_Blue = SwatManager->QueuedSwatCommandMap.Find(ETeamType::TT_SERT_BLUE);

			if (QueuedSwatCommand_Red && QueuedSwatCommand_Blue)
			{
				DoCommand(QueuedSwatCommand_Blue->Command, true, ETeamType::TT_SERT_BLUE, QueuedSwatCommand_Blue->ContextualData, true);
				DoCommand(QueuedSwatCommand_Red->Command, true, ETeamType::TT_SERT_RED, QueuedSwatCommand_Red->ContextualData, true);
				SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_RED);
				SwatManager->QueuedSwatCommandMap.Remove(ETeamType::TT_SERT_BLUE);
			}
		}
		else if (QueuedSwatCommand)
		{
			SwatManager->PlaySwatCommandVoiceLine(VO_SWAT_COMMAND::CALL_EXECUTE);
			DoCommand(QueuedSwatCommand->Command, true, ETeamType::TT_NONE, QueuedSwatCommand->ContextualData, true);
			SwatManager->QueuedSwatCommandMap.Remove(ActiveTeamType);
		}
	}
	else
	{
		GrabContextData();

		if (ContextualData.GetActor())
		{
			ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(ContextualData.GetActor());
			
			if (AICharacter && !AICharacter->IsOnSWATTeam())
			{
				IssueHumanDefaultCommand(AICharacter);
			}
			else if (ADoor* Door = Cast<ADoor>(ContextualData.GetActor()))
			{
				IssueDoorDefaultCommand(Door);
			}
			else if (AIncapacitatedHuman* IncapacitatedHuman = Cast<AIncapacitatedHuman>(ContextualData.GetActor()))
			{
				if (!IncapacitatedHuman->HasBeenReported())
				{
					IssueIncapHumanDefaultCommand(IncapacitatedHuman);
				}
				else
				{
					IssueGlobalDefaultCommand();
				}
			}
			else if (ABaseItem* BaseItem = Cast<ABaseItem>(ContextualData.GetActor()))
			{
				if (BaseItem->IsEvidence())
				{
					IssueCollectEvidenceCommand(BaseItem);
				}
				else
				{
					IssueGlobalDefaultCommand();
				}
			}
			else if (AThrownEvidenceActor* ThrownEvidence = Cast<AThrownEvidenceActor>(ContextualData.GetActor()))
			{
				IssueCollectEvidenceCommand(ThrownEvidence->OwningItem);
			}
			else if (AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(ContextualData.GetActor()))
			{
				IssueCollectEvidenceCommand(EvidenceActor);
			}
			else if (ATrapActorAttachedToDoor* DoorTrapActor = Cast<ATrapActorAttachedToDoor>(ContextualData.GetActor()))
			{
				IssueDisarmTrapCommand(DoorTrapActor);
			}
			else if (ATrapActor* TrapActor = Cast<ATrapActor>(ContextualData.GetActor()))
			{
				IssueDisarmTrapCommand(TrapActor);
			}
			else if (AExfilActor* ExfilActor = Cast<AExfilActor>(ContextualData.GetActor()))
			{
				IssueExfilCommand(ExfilActor);
			}
			else
			{
				IssueGlobalDefaultCommand();
			}
		}
		else
        {
        	IssueGlobalDefaultCommand();
        }
	}
}

void USwatCommandWidget::SetActiveTeamElement(ETeamType TeamType)
{
	if (TeamType == ActiveTeamType)
		return;
	
	PreviousActiveTeamType = ActiveTeamType;
	ActiveTeamType = TeamType;

	USWATManager* SwatManager = USWATManager::Get(this);

	if (SwatManager)
		SwatManager->ActiveCommandTeam = TeamType;
	
	if (ActiveTeamType == ETeamType::TT_NONE)
	{
		if (SwatManager)
			SwatManager->ActiveCommandTeam = ETeamType::TT_NONE;

		CloseCommandMenu();
		
		return;
	}

	LastActorBeforeGoingIntoSubPage = nullptr;
	LastSubCommandPageIndex = 0;
	ParentCommands.Empty();
	CommandCombo.Empty();
	
	UpdateCommandPageData();
	
	SetLastCommandPage(ActiveCommandPage);

	OnPageViewUpdate();

	OnSwatElementChanged.Broadcast(ActiveTeamType);
}

AActor* USwatCommandWidget::GetLastContextActor() const
{
	return LastContextActor;
}
