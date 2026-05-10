// Copyright Void Interactive, 2021

#include "TeamStackUpActivity.h"

#include "DoorInteractionActivity.h"
#include "Actors/Door.h"
#include "Actors/DoorwayWithoutDoor.h"
#include "Actors/StackUpActor.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/AI/SWATController.h"
#include "Info/Activities/ActivityManagerTemplates.h"

#include "Info/SWATManager.h"
#include "Info/Activities/MoveActivity.h"

#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"
#include "Actors/Items/BallisticsShield.h"

TAutoConsoleVariable<int32> CVarRonStackUpInstantMove(TEXT("a.RonStackUpInstantMove"), 0, TEXT("If enabled the swat will instantly teleport to each activity location instead of moving there"));
TAutoConsoleVariable<int32> CVarRonStackUpDebug(TEXT("a.RonStackUpDebug"), 0, TEXT("If enabled, visualize stack up squad position information"));

#define StackUpDoor					GetSharedData<FSharedStackUpData>()->StackUpDoor
#define DoorChecker					GetSharedData<FSharedStackUpData>()->DoorChecker
#define ActivityId					GetSharedData<FSharedStackUpData>()->ActivityId
#define bNewStackUpDoor				GetSharedData<FSharedStackUpData>()->bNewStackUpDoor
#define bStackOppositeSide			GetSharedData<FSharedStackUpData>()->bStackOppositeSide
#define bHasCheckedDoor				GetSharedData<FSharedStackUpData>()->bHasCheckedDoor
#define DoorCheckResult				GetSharedData<FSharedStackUpData>()->DoorCheckResult
#define bHasStartedCheckingLock		GetSharedData<FSharedStackUpData>()->bHasStartedCheckingLock
#define CheckLocation				GetSharedData<FSharedStackUpData>()->CheckLocation
#define DoorCheckAnimMontage		GetSharedData<FSharedStackUpData>()->DoorCheckAnimMontage
#define DoorToClose					GetSharedData<FSharedStackUpData>()->DoorToClose

UTeamStackUpActivity::UTeamStackUpActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "StackUp");
	bIsProgressActivity = true;
	bFinishActivityWhenOverriden = false;
	bAbortActivityIfCannotReachLocation = true;
	MoveAcceptanceRadius = 10.0f;
	bNoResetDataOnFinish = true;
	bResetStateMachineWhenActivityResumed = false;
	bAllowPartialMove = false;

	ActivityStateMachine->AddState("Stack Up")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::EnterStackupStage))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::ExitStackupStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::PerformStackUpStage))
						.CreateTransition("Check", MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::CanPerformCheck), 0)
						.CreateTransition("Stacked", MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::IsCheckFinished), 1);

	ActivityStateMachine->AddState("Check")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::EnterCheckStage))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::ExitCheckStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::PerformCheckStage))
						.CreateTransition("Stacked", MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::IsCheckFinished), 0);
	
	ActivityStateMachine->AddState("Stacked")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::EnterStackedStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamStackUpActivity::PerformStackedStage));
}

void UTeamStackUpActivity::StartActivity(AAIController* Owner)
{
	if (!StackUpDoor)
	{
		ACTIVITY_FAILED("No valid stack up door");
		return;
	}

	if (StackUpDoor->GetSubDoor())
	{
		if (StackUpDoor->IsNonMainSubdoor())
		{
			ACTIVITY_FAILED("Stack up only works on main subdoors. Trying to stack up on non-main subdoor -> " + StackUpDoor->GetName());
			return;
		}
	}
	
	StackUpDoor->OnDoorOpened.RemoveAll(this);
	StackUpDoor->OnDoorOpened.AddDynamic(this, &UTeamStackUpActivity::OnDoorOpened);
	
	StackUpDoor->OnSubDoorOpened.RemoveAll(this);
	StackUpDoor->OnSubDoorOpened.AddDynamic(this, &UTeamStackUpActivity::OnDoorOpened);

	StackUpDoor->OperatingStates.AddUnique("AI StackUp");

	if (StackUpDoor->GetSubDoor())
		StackUpDoor->GetSubDoor()->OperatingStates.AddUnique("AI StackUp");

	StackUpDoor->bForceClosedDoorNavArea = true;
	
	if (StackUpDoor->GetSubDoor())
		StackUpDoor->GetSubDoor()->bForceClosedDoorNavArea = true;

	if (StackUpDoor->IsDoorwayOnly() ||
		(!StackUpDoor->IsDoorwayOnly() && StackUpDoor->IsOpen()) ||
		StackUpDoor->TeamKnowsDoorLockState(GetCharacter()->IsSuspect()))
	{
		bHasCheckedDoor = true;
		DoorCheckResult = EDoorCheckResult::Unlocked;
		StackUpDoor->SetDoorLockKnowledge(GetCharacter()->IsSuspect(), true);
	}

	if (const ADoor* SubDoor = StackUpDoor->GetSubDoor())
	{
		if (SubDoor->IsDoorwayOnly() ||
			(!SubDoor->IsDoorwayOnly() && SubDoor->IsOpen()) ||
			SubDoor->TeamKnowsDoorLockState(GetCharacter()->IsSuspect()))
		{
			bHasCheckedDoor = true;
			DoorCheckResult = EDoorCheckResult::Unlocked;
			StackUpDoor->SetDoorLockKnowledge(GetCharacter()->IsSuspect(), true);
		}
	}
	
	bDontCalculateStackUp = false;
	
	StackUpDoor->DeactivateBreachBlockers();

	StackUpDoor->CurrentStackUpActivities.AddUnique(this);

	if (StackUpDoor->GetSubDoor())
		StackUpDoor->GetSubDoor()->CurrentStackUpActivities.AddUnique(this);

	Super::StartActivity(Owner);
}

void UTeamStackUpActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!StackUpDoor)
	{
		ACTIVITY_FAILED("No valid stack up door");
		return;
	}

	if (StackUpDoor->IsDoorwayOnly() ||
		(!StackUpDoor->IsDoorwayOnly() && StackUpDoor->IsOpen()) ||
		StackUpDoor->TeamKnowsDoorLockState(GetCharacter()->IsSuspect()))
	{
		bHasCheckedDoor = true;
		if (StackUpDoor->IsLocked())
			DoorCheckResult = EDoorCheckResult::Locked;
		else if (StackUpDoor->IsJammed())
			DoorCheckResult = EDoorCheckResult::Jammed;
		else
			DoorCheckResult = EDoorCheckResult::Unlocked;
	}

	if (const ADoor* SubDoor = StackUpDoor->GetSubDoor())
	{
		if (SubDoor->IsDoorwayOnly() ||
			(!SubDoor->IsDoorwayOnly() && SubDoor->IsOpen()) ||
			SubDoor->TeamKnowsDoorLockState(GetCharacter()->IsSuspect()))
		{
			bHasCheckedDoor = true;
			if (StackUpDoor->IsLocked())
				DoorCheckResult = EDoorCheckResult::Locked;
			else if (StackUpDoor->IsJammed())
				DoorCheckResult = EDoorCheckResult::Jammed;
			else
				DoorCheckResult = EDoorCheckResult::Unlocked;
		}
	}

	if (OccupiedStackUpActor && AllStackUpPathsReady())
	{
		if (const ADoor* LinkedDoor = OccupiedStackUpActor->GetLinkedDoor())
		{
			const bool bAnyEqual = LinkedDoor == StackUpDoor || (LinkedDoor->GetSubDoor() && LinkedDoor->GetSubDoor() == StackUpDoor);
			if (!bAnyEqual)
			{
				bDontCalculateStackUp = false;
				OccupiedStackUpActor->OccupiedBy = nullptr;
				OccupiedStackUpActor = nullptr;
				ChosenStackUpArea = EStackupGenArea::SGA_None;
				CalculateStackUpPosition();
			}
		}
	}

	// leader occupy stackup
	{
		if (const AReadyOrNotCharacter* SquadLeader = GetSquadLeader())
		{
			if (SquadLeader->IsLocalPlayer())
			{
				for (AStackUpActor* StackUpActor : StackUpDoor->GetStackupsForArea(ChosenStackUpArea))
				{
					if (UKismetMathLibrary::IsPointInBox(SquadLeader->GetNavAgentLocation(), StackUpActor->GetActorLocation(), FVector(50.0f)))
					{
						//DrawDebugString(GetWorld(), StackUpDoor->GetDoorMidLocation(), RON_ENUM_TO_STRING(ESquadPosition, StackUpActor->GetSquadPosition()), nullptr, FColor::White, 0.033f, true);
						if (StackUpActor->OccupiedBy)
						{
							if (ACyberneticController* AIController = Cast<ACyberneticController>(StackUpActor->OccupiedBy))
							{
								if (UTeamStackUpActivity* StackUpActivity = AIController->GetActivity<UTeamStackUpActivity>())
								{
									StackUpActor->OccupiedBy = SquadLeader->GetController();

									AStackUpActor* FurthestStackUp = nullptr;
									for (AStackUpActor* StackUpActor2 : StackUpDoor->GetStackupsForArea(ChosenStackUpArea))
									{
										if (!StackUpActor2->OccupiedBy)
										{
											FurthestStackUp = StackUpActor2;
											break;
										}
									}
									
									if (FurthestStackUp)
									{
										FurthestStackUp->OccupiedBy = AIController;
										StackUpActivity->OccupiedStackUpActor = FurthestStackUp;
										StackUpActivity->OverrideSquadPosition = FurthestStackUp->GetSquadPosition();
										StackUpActivity->Location = FurthestStackUp->GetActorLocation();
										StackUpActivity->RequestMoveAsync();
										break;
									}
								}
							}
						}
						else
						{
							StackUpActor->OccupiedBy = SquadLeader->GetController();
						}
					}
					else
					{
						if (StackUpActor->OccupiedBy == SquadLeader->GetController())
						{
							// find the AI who has this one occupied
							ACyberneticController* Controller = nullptr;
							UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
							{
								if (Activity->ActivityId == ActivityId)
								{
									if (Activity->OccupiedStackUpActor == StackUpActor)
									{
										Controller = Activity->OwningController;
										return false;
									}
								}

								return true;
							});
							
							StackUpActor->OccupiedBy = Controller;
						}
					}
				}
			}
		}
	}
	
	// try close door if it is open towards us
	if (GetActiveStateID() <= 2 && ShouldCloseDoorWhenStackingUp())
	{
		if ((bStackOppositeSide && HasTeamReachedPosition(100.0f)) || !bStackOppositeSide)
		{
			if (OverrideSquadPosition == ESquadPosition::SP_Alpha && OccupiedStackUpActor && !StackUpDoor->IsAnyAIOpening() && !StackUpDoor->IsAnyAIClosing())
			{
				if (StubLocation != FVector::ZeroVector && StubLocation == OccupiedStackUpActor->GetActorLocation() && HasReachedLocation(200.0f))
				{
					ADoor* ClosestDoor = StackUpDoor;
					if (StackUpDoor->GetSubDoor())
					{
						const float A = FVector::Distance(StackUpDoor->GetActorLocation(), GetCharacter()->GetNavAgentLocation());
						const float B = FVector::Distance(StackUpDoor->GetSubDoor()->GetActorLocation(), GetCharacter()->GetNavAgentLocation());

						if (B < A)
						{
							ClosestDoor = StackUpDoor->GetSubDoor();
						}
					}

					if (ClosestDoor && !ClosestDoor->IsDoorwayOnly())
					{
						const bool bCommandFront = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
						
						if ((bCommandFront ? ClosestDoor->IsOpen_Backward() : ClosestDoor->IsOpen_Forward()) &&
							!ClosestDoor->IsClosing())
						{
							if (!GetCharacter()->IsAny3PMontageActive() && GetCharacter()->QueuedDoorToClose != ClosestDoor)
							{
								if (GetCharacter()->ToggleDoor(ClosestDoor, false))
								{
									AbortMove();
									DoorToClose = ClosestDoor;
								}
							}
						}
					}

					/*
					const bool bCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
					
					if ((bCommandFront ? StackUpDoor->IsOpen_Backward() : StackUpDoor->IsOpen_Forward()) &&
						!StackUpDoor->IsClosing())
					{
						if (!GetCharacter()->IsAny3PMontageActive() && GetCharacter()->QueuedDoorToClose != StackUpDoor)
						{
							GetCharacter()->ToggleDoor(StackUpDoor, false);
						}
					}

					if (ADoor* SubDoor = StackUpDoor->GetSubDoor())
					{
						const bool bCommandFront_SubDoor = SubDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
						
						if ((bCommandFront_SubDoor ? SubDoor->IsOpen_Backward() : SubDoor->IsOpen_Forward()) &&
							!SubDoor->IsClosing())
						{
							if (!GetCharacter()->IsAny3PMontageActive() && GetCharacter()->QueuedDoorToClose != SubDoor)
							{
								GetCharacter()->ToggleDoor(SubDoor, false);
							}
						}
					}
					*/
				}
			}
		}
	}

	if (CanCollapse())
	{
		if (!OccupiedStackUpActor)
		{
			if (Location != FVector::ZeroVector)
				CalculateStackUpPosition();
		}
		else
		{
			TryCollapse();
			StubLocation = OccupiedStackUpActor->GetActorLocation();
			SetLocation(StubLocation);
		}
	}
	
	if (bIsCollapsing)
	{
		if (Location == FVector::ZeroVector ||
			(Location != FVector::ZeroVector && HasReachedLocation(GetDestinationTolerance())))
		{
			bIsCollapsing = false;
		}
	}
	
	// move up stack up positions if we couldnt project our current one on the navmesh
	if (OccupiedStackUpActor && GetActiveStateID() > 0 && GetActiveStateID() <= 2) // check
	{
		FVector ProjectedLocation = FVector::ZeroVector;
		const bool bCanProject = UReadyOrNotAISystem::ProjectPointToNav(OccupiedStackUpActor->GetActorLocation(), ProjectedLocation, FVector(40.0f, 40.0f, 150.0f));
		if (!bCanProject)
		{
			for (AStackUpActor* StackUpActor : StackUpDoor->GetStackupsForArea(ChosenStackUpArea))
			{
				if (StackUpActor != OccupiedStackUpActor)
				{
					if (StackUpActor->GetSquadPosition() > OccupiedStackUpActor->GetSquadPosition() && !Cast<ASWATController>(StackUpActor->OccupiedBy))
					{
						OccupiedStackUpActor = StackUpActor;
						StubLocation = StackUpActor->GetActorLocation();
						SetLocation(StubLocation, true);
						return;
					}
				}
			}
		}
	}

	if (OccupiedStackUpActor && GetActiveStateID() != 1 && GetActiveStateID() <= 3)
	{
		if (!HasReachedLocation(GetDestinationTolerance()))
		{
			SetLocation(OccupiedStackUpActor->GetActorLocation(), true);
		}
	}
	
	// make sure door checker is alpha
	if (DoorChecker)
	{
		if (GetSquadPositionForCharacter(DoorChecker) != ESquadPosition::SP_Alpha)
		{
			DoorChecker = FindChecker();
		}
	}

	if (DoorToClose)
	{
		if (DoorToClose->IsClosed())
		{
			DoorToClose = nullptr;
		}
	}
}

#if !UE_BUILD_SHIPPING
void UTeamStackUpActivity::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (!HasReachedLocation(GetDestinationTolerance()))
	{
		if (CVarRonStackUpInstantMove.GetValueOnAnyThread() > 0)
		{
			if (OccupiedStackUpActor)
			{
				FVector ProjectedLocation = OccupiedStackUpActor->GetActorLocation();
				UReadyOrNotAISystem::ProjectPointToNav(OccupiedStackUpActor->GetActorLocation(), ProjectedLocation);
				
				GetCharacter()->SetActorLocation(ProjectedLocation + FVector(0.0f, 0.0f, 50.0f));
			}
		}
	}
}
#endif

void UTeamStackUpActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
	Super::GatherDebugString(OutString);

	OutString += AddDebugString("Activity ID", ActivityId.ToString());
    OutString += AddDebugString("Squad Override", RON_ENUM_TO_STRING(ESquadPosition, OverrideSquadPosition));

	float Speed = 0.0f;
	GetOverrideMovementSpeed(Speed);
    OutString += AddDebugString("Speed", FString::Printf(TEXT("%.2f"), Speed));
	
	if (OccupiedStackUpActor)
	{
		OutString += AddDebugString("Stack Up Squad Position", RON_ENUM_TO_STRING(ESquadPosition, OccupiedStackUpActor->GetSquadPosition()));
		OutString += AddDebugString("Occupied Stack Up", OccupiedStackUpActor->GetName());
		OutString += AddDebugString("Depth", FString::FromInt(OccupiedStackUpActor->Depth));
		OutString += AddDebugString("Higher Position Same Depth", (bHigherPositionSameDepth ? "true" : "false"));
		OutString += AddDebugString("Stack Up Area", RON_ENUM_TO_STRING(EStackupGenArea, ChosenStackUpArea));
	}
	#endif
}

void UTeamStackUpActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	if (StackUpDoor)
	{
		StackUpDoor->OperatingStates.Remove("AI StackUp");
		
		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->OperatingStates.Remove("AI StackUp");

		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->bForceClosedDoorNavArea = false;
		
		StackUpDoor->OnDoorOpened.RemoveAll(this);
		StackUpDoor->OnSubDoorOpened.RemoveAll(this);
		
		StackUpDoor->bForceClosedDoorNavArea = false;
		
		StackUpDoor->CurrentStackUpActivities.Remove(this);
		
		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->CurrentStackUpActivities.Remove(this);
	}
	
	GetCharacter()->OnDoorChecked_FromAnimNotify.RemoveAll(this);

	GetCharacter()->StopTPMontageFromTable(DoorCheckAnimMontage);

	if (OccupiedStackUpActor)
	{
		OccupiedStackUpActor->DisableNavBlocker();
	}
}

void UTeamStackUpActivity::FinishedActivity_NoOwner(bool bSuccess)
{
	Super::FinishedActivity_NoOwner(bSuccess);
	
	if (StackUpDoor)
	{
		StackUpDoor->OperatingStates.Remove("AI StackUp");
		
		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->OperatingStates.Remove("AI StackUp");

		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->bForceClosedDoorNavArea = false;
		
		StackUpDoor->OnDoorOpened.RemoveAll(this);
		StackUpDoor->OnSubDoorOpened.RemoveAll(this);
		
		StackUpDoor->bForceClosedDoorNavArea = false;
		
		StackUpDoor->CurrentStackUpActivities.Remove(this);
		
		if (StackUpDoor->GetSubDoor())
			StackUpDoor->GetSubDoor()->CurrentStackUpActivities.Remove(this);
	}

	if (OccupiedStackUpActor)
	{
		OccupiedStackUpActor->DisableNavBlocker();
	}
}

/////// Stack Up Stage events ///////
void UTeamStackUpActivity::EnterStackupStage()
{
	if (bNewStackUpDoor)
	{
		FindStackUpPath();
	}
	else
	{
		bFoundStackUpPath = true;
		if (AllStackUpPathsReady())
		{
			ReSortExistingSwat();
		}
	}
}

void UTeamStackUpActivity::OnSwatSorted(const TArray<ASWATCharacter*>& InSortedSwat, const bool bReversed)
{
	if (InSortedSwat.Num() == 0)
		return;

	GetSharedData<FSharedStackUpData>()->StackUpSortedSwat = InSortedSwat;
	GetSharedData<FSharedStackUpData>()->bWasSplitUp = bReversed;

	const TArray<AStackUpActor*> AllStackUps = StackUpDoor->GetStackupsForArea(EStackupGenArea::SGA_All);
	for (AStackUpActor* StackUpActor : AllStackUps)
	{
		if (StackUpActor)
		{
			StackUpActor->DisableNavBlocker();
			StackUpActor->OccupiedBy = nullptr;
		}
	}
	
	for (ASWATCharacter* Swat : InSortedSwat)
	{
		if (UTeamStackUpActivity* Activity = Swat->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
		{
			Activity->bDontCalculateStackUp = false;
			Activity->bHigherPositionSameDepth = false;

			Activity->ChosenStackUpArea = EStackupGenArea::SGA_None;
			
			if (Activity->OccupiedStackUpActor)
			{
				Activity->OccupiedStackUpActor->OccupiedBy = nullptr;
				Activity->OccupiedStackUpActor = nullptr;
			}
		}
	}
	
	StackUpDoor->DeactivateDoorBlocker();

	for (ASWATCharacter* Swat : InSortedSwat)
	{
		if (UTeamStackUpActivity* Activity = Swat->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
		{
			Activity->CalculateStackUpPosition();

			if (Activity->OccupiedStackUpActor)
			{
				Activity->AbortMove(true);
				
				if (!bNewStackUpDoor)
				{
					Activity->StubLocation = Activity->OccupiedStackUpActor->GetActorLocation();
					Activity->SetLocation(Activity->StubLocation, true);
					//Activity->Location = Activity->OccupiedStackUpActor->GetActorLocation();
					//Activity->RequestMoveAsync();
				}
			}
		}
	}
	
	if (!DoorChecker && !StackUpDoor->IsDoorwayOnly() && !StackUpDoor->IsOpenBeyondIncrementThreshold())
	{
		DoorChecker = FindChecker();
	}
	
	const bool bIsCommandFrontOfDoor = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
	
	GetSharedData<FSharedStackUpData>()->bPreviousCommandFrontOfDoor = bIsCommandFrontOfDoor;
	GetSharedData<FSharedStackUpData>()->PreviousStackUpStyle = GetSharedData<FSharedStackUpData>()->StackUpStyle;
	SharedData->PreviousCommandLocation = SharedData->CommandLocation;
}

bool UTeamStackUpActivity::ShouldCloseDoorWhenStackingUp()
{
	return false; // shit ass feature....
	
	if (StackUpDoor->IsPointsOnOppositeSideOfDoor(SharedData->CommandLocation, GetCharacter()->GetActorLocation()))
		return false;
	
	if (DoesStackUpPathGoThroughDoor())
		return false;
	
	/*
	if (IsCommandFrontOfDoor())
	{
		bool bSubDoorOpenForward = false;
		if (StackUpDoor->GetSubDoor())
		{
			if (StackUpDoor->GetSubDoor()->IsPointInFrontOfDoorway(SharedData->CommandLocation))
			{
				if (StackUpDoor->GetSubDoor()->IsOpen_Forward())
					bSubDoorOpenForward = true;
			}
		}
		
		if (StackUpDoor->IsOpen_Forward() && bSubDoorOpenForward)
			return false;
	}
	else
	{
		bool bSubDoorOpenBackward = false;
		if (StackUpDoor->GetSubDoor())
		{
			if (!StackUpDoor->GetSubDoor()->IsPointInFrontOfDoorway(SharedData->CommandLocation))
			{
				if (StackUpDoor->GetSubDoor()->IsOpen_Backward())
					bSubDoorOpenBackward = true;
			}
		}
		
		if (StackUpDoor->IsOpen_Backward() && bSubDoorOpenBackward)
			return false;
	}
	
	if (StackUpDoor->GetSubDoor())
	{
		if (StackUpDoor->GetSubDoor()->IsPointInFrontOfDoorway(SharedData->CommandLocation))
		{
			if (StackUpDoor->GetSubDoor()->IsOpen_Forward())
				return false;
		}
		else
		{
			if (StackUpDoor->GetSubDoor()->IsOpen_Backward())
				return false;
		}
	}
	*/
	
	return true;
}

void UTeamStackUpActivity::PerformStackUpStage(float DeltaTime, float Uptime)
{
	if (!StackUpDoor)
	{
		ACTIVITY_FAILED("PerformStackUpStage | No valid stack up door");
		return;
	}

	ProgressState = FText::FromStringTable("SwatCommandTable", "Stacking");

	if (Location != FVector::ZeroVector && HasReachedLocation(300.0f))
		StackUpDoor->DeactivateDoorBlocker();

	const TArray<ASWATCharacter*>& StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
	const EStackUpStyle StackUpStyle = GetSharedData<FSharedStackUpData>()->StackUpStyle;

	if (bNewStackUpDoor || StackUpSortedSwat.Num() == 0)
	{
		// for some reason, async path will just fail, even in perfectly flat navmesh geometry with no obstacles :(
		// if so, try to repath again...
		if (StackUpAsyncPathId == 0)
		{
			FindStackUpPath();
		}
		else
		{
			// fail the activity
			if (bFoundStackUpPath && StackUpSortedSwat.Num() == 0 && Uptime > 1.0f)
			{
				ACTIVITY_FAILED("Failed to find stack up path");
				return;
			}
		}
	}
	else
	{
		if (OccupiedStackUpActor)
		{
			StubLocation = OccupiedStackUpActor->GetActorLocation();
			SetLocation(StubLocation);
			//Location = OccupiedStackUpActor->GetActorLocation();
		}
		
		if (Uptime > 1.0f && Location == FVector::ZeroVector)
		{
			bFoundStackUpPath = true;
			if (AllStackUpPathsReady())
			{
				ReSortExistingSwat();

				if (OverrideSquadPosition == ESquadPosition::SP_Alpha ||
					OverrideSquadPosition == ESquadPosition::SP_NONE)
				{
					OnSwatSorted(StackUpSortedSwat, GetSharedData<FSharedStackUpData>()->bWasSplitUp);
				}
			}
		}
	}
	
	if (bNewStackUpDoor && OccupiedStackUpActor)
	{
		float Threshold = 400.0f;
		if (StackUpStyle == EStackUpStyle::Split)
			Threshold = 600.0f;
		
		bool bHasAlphaReachedLocation = false;
		bool bSameStackUpArea = false;
		
		if (StackUpSortedSwat.Num() > 0)
		{
			uint8 Index = StackUpSortedSwat.Find(GetCharacter<ASWATCharacter>());
			if (StackUpSortedSwat.IsValidIndex(Index - 1))
			{
				const ACyberneticCharacter* Alpha = StackUpSortedSwat[Index - 1];
				if (IsValid(Alpha) && Alpha->GetCyberneticsController())
				{
					if (UTeamStackUpActivity* AlphaActivity = Alpha->GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
					{
						bSameStackUpArea = AlphaActivity->ChosenStackUpArea == ChosenStackUpArea;
						bHasAlphaReachedLocation = AlphaActivity->HasReachedLocation(200.0f);
					}
				}
			}

			{
				const bool bNearStackUpLocation = FVector::Distance(OccupiedStackUpActor->GetActorLocation(), GetCharacter()->GetNavAgentLocation()) < Threshold;
				if (StackUpSortedSwat[0] == GetCharacter() || bNearStackUpLocation || bHasAlphaReachedLocation || !bSameStackUpArea)
				{
					StubLocation = OccupiedStackUpActor->GetActorLocation();
					SetLocation(StubLocation, true);
					//Location = OccupiedStackUpActor->GetActorLocation();
					//RequestMoveAsync();
				}
				else
				{
					if (StackUpSortedSwat.Num() > 0)
					{
						Index = StackUpSortedSwat.Find(GetCharacter<ASWATCharacter>());
						if (StackUpSortedSwat.IsValidIndex(Index-1))
						{
							if (const ACyberneticCharacter* Alpha = StackUpSortedSwat[Index-1])
							{
								StubLocation = Alpha->GetNavAgentLocation();
								SetLocation(StubLocation, true);
								
								//Location = Alpha->GetNavAgentLocation();
								//RequestMoveAsync();
							}
						}
					}
				}
			}
		}
	}

	// failsafe
	if (!HasReachedLocation(GetDestinationTolerance()) && GetCharacter()->GetVelocity().Size() < 10.0f)
	{
		if (OccupiedStackUpActor && Uptime > 60.0f)
		{
			FVector ProjectedLocation = OccupiedStackUpActor->GetActorLocation();
			UReadyOrNotAISystem::ProjectPointToNav(OccupiedStackUpActor->GetActorLocation(), ProjectedLocation);
			
			GetCharacter()->SetActorLocation(ProjectedLocation + FVector(0.0f, 0.0f, 50.0f));
		}
	}
}

void UTeamStackUpActivity::ExitStackupStage()
{
}

/////// Check Stage events ///////
bool UTeamStackUpActivity::CanPerformCheck() const
{
	if (!OccupiedStackUpActor)
		return false;

	if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		return false;
	
	if (!DoorChecker)
		return false;
	
	if (!StackUpDoor->IsClosed())
		return false;

	if (bHasCheckedDoor)
		return false;

	if (StubLocation != OccupiedStackUpActor->GetActorLocation())
		return false;
		
	return HasReachedLocation(50.0f);
}

void UTeamStackUpActivity::EnterCheckStage()
{
}

void UTeamStackUpActivity::PerformCheckStage(float DeltaTime, float Uptime)
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "Checking");

	if (bHasCheckedDoor)
	{
		SetLocation(OccupiedStackUpActor->GetActorLocation());
		//Location = OccupiedStackUpActor->GetActorLocation();
		return;
	}

	if (OverrideSquadPosition == ESquadPosition::SP_Alpha && DoorChecker == GetCharacter())
	{
		if (!bHasStartedCheckingLock)
		{
			const bool IsCommandInFrontOfDoor = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
			const bool bIsRightSideOfDoor = StackUpDoor->IsActorRightOfDoorway(GetCharacter());

			FVector DoorLocation = StackUpDoor->GetDoorMidLocation();
			DoorLocation.Z = StackUpDoor->GetActorLocation().Z;
			
			FVector DoorFocalPoint = StackUpDoor->GetActorLocation() + StackUpDoor->GetActorRightVector() * 115.0f + StackUpDoor->GetActorForwardVector() * (StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? -100.0f : 100.0f);

			if (StackUpDoor->IsLocked())
			{
				if (bIsRightSideOfDoor)
				{
					CheckLocation = DoorLocation + (IsCommandInFrontOfDoor ? StackUpDoor->GetActorForwardVector() * 80.0f : StackUpDoor->GetActorForwardVector() * -80.0f) + StackUpDoor->GetActorRightVector() * 110.0f;
					DoorFocalPoint = StackUpDoor->GetActorLocation() + StackUpDoor->GetActorRightVector() * 115.0f + StackUpDoor->GetActorForwardVector() * (StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? -500.0f : 500.0f) + StackUpDoor->GetActorRightVector() * 200.0f;
				}
				else
				{
					CheckLocation = DoorLocation + (IsCommandInFrontOfDoor ? StackUpDoor->GetActorForwardVector() * 80.0f : StackUpDoor->GetActorForwardVector() * -80.0f) + StackUpDoor->GetActorRightVector() * -20.0f;;
				}
			}
			else
			{
				if (bIsRightSideOfDoor)
				{
					CheckLocation = DoorLocation + (IsCommandInFrontOfDoor ? StackUpDoor->GetActorForwardVector() * 100.0f : StackUpDoor->GetActorForwardVector() * -100.0f) + StackUpDoor->GetActorRightVector() * 100.0f;
				}
				else
				{
					CheckLocation = DoorLocation + (IsCommandInFrontOfDoor ? StackUpDoor->GetActorForwardVector() * 80.0f : StackUpDoor->GetActorForwardVector() * -80.0f) + StackUpDoor->GetActorRightVector() * -20.0f;
				}
			}

			bool bCanProject = false;
			if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NewLocationProjected(CheckLocation);
				bCanProject = NavSys->ProjectPointToNavigation(CheckLocation, NewLocationProjected, FVector(75.0f, 75.0f, 150.0f));
				
				Location = CheckLocation;
			}
			
			//DrawDebugBox(GetWorld(), Location, FVector(20.0f), FColor::Cyan, false, 5.0f);

			//const float Distance = FVector::Distance(Location, GetCharacter()->GetActorLocation());
			//LOG_NUMBER(Distance);
			//LOG_NUMBER(GetDestinationTolerance());
			//ULog::Bool(HasReachedLocation(GetDestinationTolerance()), "HasReachedLocation: ");
			const float FocusDotProduct = FVector::DotProduct((OwningController->GetFocalPoint() - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
			const bool bIsFocusingOnPoint = FocusDotProduct >= 0.75f;
			//LOG_NUMBER(FocusDotProduct);

			if ((HasReachedLocation(30.0f) || !bCanProject) && bIsFocusingOnPoint)
			{
				GetCharacter()->OnDoorChecked_FromAnimNotify.RemoveAll(this);
				GetCharacter()->OnDoorChecked_FromAnimNotify.AddDynamic(this, &UTeamStackUpActivity::OnDoorChecked);

				const bool bIsCheckerFrontOfDoor = StackUpDoor->IsActorInFrontOfDoorway(DoorChecker);
				const bool bIsCheckerRightOfDoor = StackUpDoor->IsActorRightOfDoorway(DoorChecker);

				if (StackUpDoor->IsLocked())
				{
					if (bIsCheckerFrontOfDoor)
						DoorCheckAnimMontage = bIsCheckerRightOfDoor ? "tp_swt_checkdoor_r_locked" : "tp_swt_checkdoor_l_locked";
					else
						DoorCheckAnimMontage = bIsCheckerRightOfDoor ? "tp_swt_checkdoor_l_locked" : "tp_swt_checkdoor_r_locked";
				}
				else
				{
					if (bIsCheckerFrontOfDoor)
						DoorCheckAnimMontage = bIsCheckerRightOfDoor ? "tp_swt_checkdoor_r_unlocked" : "tp_swt_checkdoor_l_unlocked";
					else
						DoorCheckAnimMontage = bIsCheckerRightOfDoor ? "tp_swt_checkdoor_l_unlocked" : "tp_swt_checkdoor_r_unlocked";
				}
				
				GetCharacter()->PlayMontageFromTableWithFocalPoint(DoorCheckAnimMontage, DoorFocalPoint);

				if (GetCharacter()->IsTableMontagePlaying(DoorCheckAnimMontage))
				{
					bHasStartedCheckingLock = true;
					AbortMove(true);
				}
			}
		}
	}
}

void UTeamStackUpActivity::EnterStackedStage()
{
	#if WITH_EDITOR
	ensureAlways(OccupiedStackUpActor != nullptr);
	#endif
	
    bDontCalculateStackUp = true;

	bIsSwapping = false;

	if (AllStacked())
	{
		StackUpDoor->DeactivateDoorBlocker();
	}
}

void UTeamStackUpActivity::PerformStackedStage(float DeltaTime, float Uptime)
{
	ProgressState = FText::GetEmpty();

	if (!OccupiedStackUpActor)
	{
		for (AStackUpActor* StackUpActor : StackUpDoor->GetStackupsForArea(ChosenStackUpArea))
		{
			if (StackUpActor->GetSquadPosition() == OverrideSquadPosition)
			{
				OccupiedStackUpActor = StackUpActor;
				OccupiedStackUpActor->OccupiedBy = OwningController;
			}
		}
	}

	GetCharacter()->bDisableTurnInPlace = OverrideSquadPosition == ESquadPosition::SP_Alpha;
}

void UTeamStackUpActivity::ExitCheckStage()
{
	GetCharacter()->OnDoorChecked_FromAnimNotify.RemoveAll(this);
}

void UTeamStackUpActivity::OnDoorChecked()
{
	bHasCheckedDoor = true;
	
	if (StackUpDoor->IsLocked())
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCKED, "", false);
		ProgressState = FText::FromStringTable("SwatCommandTable", "DoorLocked");
		DoorCheckResult = EDoorCheckResult::Locked;
	}
	else if (StackUpDoor->IsJammed())
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_JAMMED, "", false);
		ProgressState = FText::FromStringTable("SwatCommandTable", "DoorJammed");
		DoorCheckResult = EDoorCheckResult::Jammed;
	}
	else
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_UNLOCKED, "", false);
		DoorCheckResult = EDoorCheckResult::Unlocked;
	}

	AbortMove(true);

	if (StackUpDoor->IsLocked())
	{
		StackUpDoor->PlayLockedAnimation();
		StackUpDoor->PlayLockedSound();
	}
	else
	{
		StackUpDoor->PlayHandleAnimation();
	}

	if (FVector::Distance(CheckLocation, OccupiedStackUpActor->GetActorLocation()) > 20.0f)
	{
		float TimeRemaining = 0.0f;
		GetCharacter()->IsTableMontagePlayingWithTimeRemaining(DoorCheckAnimMontage, TimeRemaining);
		if (TimeRemaining-0.6f > 0.0f)
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTeamStackUpActivity::MoveToOriginalLocation, TimeRemaining-0.6f);
		else
			MoveToOriginalLocation();
	}

	StackUpDoor->SetDoorLockKnowledge(GetCharacter()->IsSuspect(), true);
	
	//SyncCheckResults();

	GetCharacter()->OnDoorChecked_FromAnimNotify.RemoveAll(this);
}

bool UTeamStackUpActivity::IsCheckFinished() const
{
	if (!bHasCheckedDoor)
		return false;
	
	if (!OccupiedStackUpActor)
		return false;
	
    //if (Location != FVector::ZeroVector && Location != OccupiedStackUpActor->GetActorLocation())
        //return false;

	if (StubLocation != OccupiedStackUpActor->GetActorLocation())
		return false;
	
    if (GetSharedData<FSharedBreachData>()->StackUpSortedSwat.Num() == 0)
        return false;

	if (OccupiedStackUpActor)
	{
		FVector Blah = FVector::ZeroVector;
		const bool bSuccess = UReadyOrNotAISystem::ProjectPointToNav(OccupiedStackUpActor->GetActorLocation(), Blah, FVector(75.0f, 75.0f, 150.0f));

		if (!bSuccess) // incase the door is open and we cant reach the stack up point
		{
			return HasReachedLocation(150.0f); // failsafe, consider that we reached
		}
	}
	
	return HasReachedLocation(GetDestinationTolerance());
}

void UTeamStackUpActivity::OnDoorOpened()
{
	if (!StackUpDoor)
		return;

	if (GetActiveStateID() == 0)
	{
		const TArray<ASWATCharacter*>& SortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
		
		if (!HasReachedLocation(GetDestinationTolerance()))
		{
			GetSharedData<FSharedStackUpData>()->StackUpStyle = EStackUpStyle::Auto;
			OnSwatSorted(SortedSwat, false);
			
			StackUpDoor->OnDoorOpened.RemoveAll(this);
			StackUpDoor->OnSubDoorOpened.RemoveAll(this);
		}
	}
}

bool UTeamStackUpActivity::DoesStackUpPathGoThroughDoor() const
{
	TArray<FNavPathPoint> PathPoints = StackUpPath;
	if (PathPoints.Num() <= 1)
	{
		return false;
	}
	
	PathPoints.Pop(); // ignore last path point
	
	for (const FNavPathPoint& NavPathPoint : PathPoints)
	{
		FVector DoorLocation = StackUpDoor->GetDoorMidLocation();
		DoorLocation.Z = StackUpDoor->GetActorLocation().Z;
		
		if (FVector::Distance(NavPathPoint.Location, DoorLocation) < 200.0f)
		{
			const bool bIsCommandFrontOfDoor = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
			const bool bPointBehindDoor = bIsCommandFrontOfDoor != StackUpDoor->IsPointInFrontOfDoorway(NavPathPoint.Location);
			if (bPointBehindDoor)
			{
				return true;
			}
		}
	}

	return false;
}

bool UTeamStackUpActivity::IsCommandFrontOfDoor() const
{
	return StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
}

ACyberneticCharacter* UTeamStackUpActivity::GetCharacterAtSquadPositionInStackUpArea(ESquadPosition SquadPosition, const EStackupGenArea StackUpArea) const
{
	TArray<UTeamStackUpActivity*> TotalSwatInStackUpArea = GetTotalSwatInStackUpArea(StackUpArea);
	for (const UTeamStackUpActivity* StackUpActivity : TotalSwatInStackUpArea)
	{
		if (StackUpActivity->OverrideSquadPosition == SquadPosition)
		{
			return StackUpActivity->GetCharacter();
		}
	}

	return nullptr;
}

ACyberneticCharacter* UTeamStackUpActivity::GetCharacterAtHighestSquadPositionInStackUpArea(EStackupGenArea StackUpArea) const
{
	TArray<UTeamStackUpActivity*> TotalSwatInStackUpArea = GetTotalSwatInStackUpArea(StackUpArea);
	if (TotalSwatInStackUpArea.Num() == 1)
		return TotalSwatInStackUpArea[0]->GetCharacter();

	ESquadPosition HighestPosition = ESquadPosition::SP_Alpha;
	ACyberneticCharacter* HighestCharacter = nullptr;
	for (const UTeamStackUpActivity* StackUpActivity : TotalSwatInStackUpArea)
	{
		if (StackUpActivity->OverrideSquadPosition > HighestPosition)
		{
			HighestPosition = StackUpActivity->OverrideSquadPosition;
			HighestCharacter = StackUpActivity->GetCharacter();
		}
	}

	return HighestCharacter;
}

void UTeamStackUpActivity::TryCollapse()
{
	if (!StackUpDoor)
	{
		ACTIVITY_FAILED("No valid door");
		return;
	}

	if (!OccupiedStackUpActor)
	{
		return;
	}

	if (bIsSwapping)
		return;
	
	TArray<AStackUpActor*> StackUpActors = StackUpDoor->GetStackupsForArea(ChosenStackUpArea);
	const int32 Index = StackUpActors.Find(OccupiedStackUpActor);
	AStackUpActor* LowerStackUp = nullptr;
	if (StackUpActors.IsValidIndex(Index-1))
	{
		uint8 Offset = 1;
		while (!LowerStackUp)
		{
			if (!StackUpActors.IsValidIndex(Index-Offset))
				break;
			
			FVector ProjectedLocation = FVector::ZeroVector;
			if (UReadyOrNotAISystem::ProjectPointToNav(StackUpActors[Index-Offset]->GetActorLocation(), ProjectedLocation))
			{
				LowerStackUp = StackUpActors[Index-Offset];
				break;
			}

			Offset++;
		}
	}

	if (AStackUpActor* StackUpActor = LowerStackUp)
	{
		if (StackUpActor == OccupiedStackUpActor)
			return;

		// if for some reason we have multiple things occupied at once
		if (StackUpActor->OccupiedBy == OwningController && OccupiedStackUpActor != StackUpActor)
		{
			OccupiedStackUpActor->OccupiedBy = nullptr;
			OccupiedStackUpActor = StackUpActor;
			return;
		}
		
		bool bOccupiedAIHasLeft = false;
		if (StackUpActor->OccupiedBy)
		{
			if (const ACyberneticController* AIController = Cast<ACyberneticController>(StackUpActor->OccupiedBy))
			{
				if (!AIController->GetActivity<UTeamStackUpActivity>())
				{
					bOccupiedAIHasLeft = true;
				}
			}
		}
		
		if ((!StackUpActor->OccupiedBy || bOccupiedAIHasLeft) && StackUpActor->GetSquadPosition() < OccupiedStackUpActor->GetSquadPosition())
		{
			OccupiedStackUpActor->DisableNavBlocker();
			OccupiedStackUpActor->OccupiedBy = nullptr;
			StackUpActor->OccupiedBy = OwningController;
			
			OccupiedStackUpActor = StackUpActor;

			OverrideSquadPosition = StackUpActor->GetSquadPosition();

			StubLocation = StackUpActor->GetActorLocation();
			SetLocation(StubLocation, true);
			
			//Location = StackUpActor->GetActorLocation();
			//RequestMoveAsync();

			bIsCollapsing = true;

			#if !UE_BUILD_SHIPPING
			ULog::Info(GetNameSafe(GetCharacter()) + ": collapsing");
			#endif
		}
	}
}

bool UTeamStackUpActivity::CanCollapse() const
{
	return GetActiveStateID() >= 2;
}

void UTeamStackUpActivity::FindStackUpPath()
{
	//FVector DoorLocation = StackUpDoor->GetLockpickHighlight()->GetComponentLocation();
	FVector DoorLocation = StackUpDoor->GetActorLocation() + StackUpDoor->GetActorRightVector() * 115.0f;
	
	if (GetSharedData<FSharedStackUpData>()->StackUpStyle == EStackUpStyle::Split)
		DoorLocation = StackUpDoor->GetDoorMidLocation();
	
	const float Offset = (StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? 50.0f : -50.0f);

	DoorLocation.Z = StackUpDoor->GetActorLocation().Z;
	DoorLocation = DoorLocation + StackUpDoor->GetActorForwardVector() * Offset;

	StackUpAsyncPathId = FindPathAsync(DoorLocation);
	
	//DrawDebugBox(GetWorld(), DoorLocation, FVector(10.0f), FColor::Cyan, false, 1.0f);
}

void UTeamStackUpActivity::ReSortExistingSwat()
{
	TArray<ASWATCharacter*> StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
	const EStackUpStyle StackUpStyle = GetSharedData<FSharedStackUpData>()->StackUpStyle;
	
	StackUpSortedSwat.Sort([&](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
	{
		if (const UTeamStackUpActivity* Activity = Lhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
		{
			if (const UTeamStackUpActivity* OtherActivity = Rhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
			{
				return Activity->PreviousSquadPosition < OtherActivity->PreviousSquadPosition;
			}
		}

		return false;
	});

	bool bReversed = false;

	if (StackUpStyle != EStackUpStyle::Auto)
	{
		TArray<ASWATCharacter*> LeftSwat, RightSwat;
		for (ASWATCharacter* Swat : StackUpSortedSwat)
		{
			if (StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation))
			{
				if (StackUpDoor->IsActorRightOfDoorway(Swat))
				{
					LeftSwat.AddUnique(Swat);
				}
				else
				{
					RightSwat.AddUnique(Swat);
				}
			}
			else
			{
				if (StackUpDoor->IsActorRightOfDoorway(Swat))
				{
					RightSwat.AddUnique(Swat);
				}
				else
				{
					LeftSwat.AddUnique(Swat);
				}
			}
		}
		
		LeftSwat.Sort([&](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
		{
			if (const UTeamStackUpActivity* Activity = Lhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
			{
				if (const UTeamStackUpActivity* OtherActivity = Rhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
				{
					return Activity->PreviousSquadPosition < OtherActivity->PreviousSquadPosition;
				}
			}

			return false;
		});

		RightSwat.Sort([&](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
		{
			if (const UTeamStackUpActivity* Activity = Lhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
			{
				if (const UTeamStackUpActivity* OtherActivity = Rhs.GetCyberneticsController<ASWATController>()->GetStackUpActivity())
				{
					return Activity->PreviousSquadPosition < OtherActivity->PreviousSquadPosition;
				}
			}

			return false;
		});

		if (RightSwat.Num() > 0 && LeftSwat.Num() > 0)
		{
			if (StackUpStyle != EStackUpStyle::Split)
			{
				if (StackUpStyle == EStackUpStyle::Left)
				{
					Algo::Reverse(LeftSwat);

					StackUpSortedSwat.Empty(4);
					StackUpSortedSwat += LeftSwat;
					StackUpSortedSwat += RightSwat;
				}
				else
				{
					Algo::Reverse(RightSwat);
					
					StackUpSortedSwat.Empty(4);
					StackUpSortedSwat += RightSwat;
					StackUpSortedSwat += LeftSwat;
				}

				bReversed = true;
			}
			else
			{
				if (LeftSwat.Num() > RightSwat.Num())
				{
					Algo::Reverse(LeftSwat);
					
					StackUpSortedSwat.Empty(4);
					StackUpSortedSwat += LeftSwat;
					StackUpSortedSwat += RightSwat;
				}
				else
				{
					Algo::Reverse(RightSwat);
					
					StackUpSortedSwat.Empty(4);
					StackUpSortedSwat += RightSwat;
					StackUpSortedSwat += LeftSwat;
				}
				
				bReversed = true;
			}
		}
	}

	//ULog::Array_Object((TArray<UObject*>)StackUpSortedSwat);
	
	OnSwatSorted(StackUpSortedSwat, bReversed);
}

bool UTeamStackUpActivity::IsLeaderOccupyingSquadPosition(ESquadPosition Position, EStackupGenArea StackUpArea) const
{
	for (const AStackUpActor* StackUpActor : StackUpDoor->GetStackupsForArea(StackUpArea))
	{
		if (StackUpActor->GetSquadPosition() == Position && !Cast<ACyberneticController>(StackUpActor->OccupiedBy))
		{
			return true;
		}
	}

	return false;
}

bool UTeamStackUpActivity::IsFurthestOccupiedStackUpInArea() const
{
	if (!OccupiedStackUpActor)
		return false;
	
	const AStackUpActor* FurthestStackUp = nullptr;
	ESquadPosition HighestSquadPosition = ESquadPosition::SP_Alpha;
	UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
	{
		if (Activity->ActivityId == ActivityId && Activity->GetStackUpArea() == ChosenStackUpArea)
		{
			if (Activity->OccupiedStackUpActor)
			{
				if (Activity->OccupiedStackUpActor->GetSquadPosition() >= HighestSquadPosition)
				{
					FurthestStackUp = Activity->OccupiedStackUpActor;
					HighestSquadPosition = Activity->OccupiedStackUpActor->GetSquadPosition();
				}
			}
		}

		return true;
	});

	return OccupiedStackUpActor == FurthestStackUp;
}

void UTeamStackUpActivity::OnAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnAsyncPathFound(PathId, ResultType, NavPath);
	
	if (!bNewStackUpDoor && GetSharedData<FSharedStackUpData>()->StackUpSortedSwat.Num() > 0)
	{
		return;
	}
	
	if (NavPath.IsValid())
	{
		if (NavPath->IsWaitingForRepath())
		{
			StackUpAsyncPathId = 0;
			bFoundStackUpPath = false;
			return;
		}
		
		//StackUpPath = *NavPath.Get();
		StackUpPath = NavPath->GetPathPoints();
		StackUpPathLength = NavPath->GetLength();
		bFoundStackUpPath = true;
	}

	if (AllStackUpPathsReady())
	{
		TMap<ASWATCharacter*, float> NavPaths;
		UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
		{
			if (Activity->ActivityId == ActivityId)
			{
				NavPaths.Add(Activity->GetCharacter<ASWATCharacter>(), Activity->StackUpPathLength);
			}

			return true;
		});

		NavPaths.ValueSort([](const float& Lhs, const float& Rhs)
		{
			return Lhs < Rhs;
		});
		
		TArray<ASWATCharacter*> SortedSwat;
		NavPaths.GenerateKeyArray(SortedSwat);

		//ULog::Array_Object((TArray<UObject*>)SortedSwat);

		OnSwatSorted(SortedSwat, false);
	}
}

void UTeamStackUpActivity::CalculateStackUpPosition()
{
	if (bDontCalculateStackUp)
		return;
	
	EStackUpStyle StackUpStyle = GetSharedData<FSharedStackUpData>()->StackUpStyle;
	const EStackUpStyle& PreviousStackUpStyle = GetSharedData<FSharedStackUpData>()->PreviousStackUpStyle;
	const bool bWasSplitUp = GetSharedData<FSharedStackUpData>()->bWasSplitUp;
	const TArray<ASWATCharacter*>& StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;

	if (StackUpSortedSwat.Num() == 0)
		return;
	
	//if (!bNewStackUpDoor && StackUpStyle == PreviousStackUpStyle)
		//return;
	
	const bool bIsCommandFrontOfDoor = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);

	if (StackUpStyle == EStackUpStyle::Auto)
	{
		if (IsCommandFrontOfDoor())
			StackUpStyleOverride = StackUpDoor->IsActorRightOfDoorway(GetCharacter()) ? EStackUpStyle::Left : EStackUpStyle::Right;
		else
			StackUpStyleOverride = StackUpDoor->IsActorRightOfDoorway(GetCharacter()) ? EStackUpStyle::Right : EStackUpStyle::Left;
		
		StackUpStyle = StackUpStyleOverride;
	}
	
	EStackupGenArea StackUpArea;
	
	if (StackUpStyle == EStackUpStyle::Left)
	{
		if (bIsCommandFrontOfDoor)
		{
			StackUpArea = EStackupGenArea::SGA_FrontRight;
		}
		else
		{
			StackUpArea = EStackupGenArea::SGA_BackLeft;
		}
	}
	else if (StackUpStyle == EStackUpStyle::Right)
	{
		if (bIsCommandFrontOfDoor)
		{
			StackUpArea = EStackupGenArea::SGA_FrontLeft;
		}
		else
		{
			StackUpArea = EStackupGenArea::SGA_BackRight;
		}
	}
	else
	{
		const bool bIsOnRight = StackUpDoor->IsPointRightOfDoorway(GetCharacter()->GetActorLocation());
		
		uint8 NumOnOtherSide = 0;
		uint8 MaxOnOneSide = StackUpSortedSwat.Num()/2;
		if (StackUpSortedSwat.Num() == 1)
			MaxOnOneSide = 1;
		
		UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](const UTeamStackUpActivity* Activity)
		{
			if (Activity != this && Activity->ActivityId == ActivityId)
			{
				if (bIsOnRight)
				{
					if (!StackUpDoor->IsPointRightOfDoorway(Activity->GetCharacter()->GetActorLocation()))
					{
						NumOnOtherSide++;
						
						if (NumOnOtherSide >= MaxOnOneSide)
							return false;
					}
				}
				else
				{
					if (StackUpDoor->IsPointRightOfDoorway(Activity->GetCharacter()->GetActorLocation()))
					{
						NumOnOtherSide++;
						
						if (NumOnOtherSide >= MaxOnOneSide)
							return false;
					}
				}
			}

			return true;
		});
		
		if (bIsCommandFrontOfDoor)
		{
			StackUpArea = bIsOnRight ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
			
			if (!bWasSplitUp && NumOnOtherSide < MaxOnOneSide)
			{
				ADoor::FlipStackUpArea(StackUpArea, true, false);
			}
		}
		else
		{
			StackUpArea = bIsOnRight ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
			
			if (!bWasSplitUp && NumOnOtherSide < MaxOnOneSide)
			{
				ADoor::FlipStackUpArea(StackUpArea, true, false);
			}
		}
	}

	TArray<AStackUpActor*> StackUpActors = StackUpDoor->GetStackupsForArea(StackUpArea);
	
	// cap the stack up actors to the amount of swat we have
	if (StackUpActors.Num() > StackUpSortedSwat.Num())
	{
		StackUpActors.SetNum(StackUpSortedSwat.Num());
	}

	if (StackUpActors.Num() > 1)
	{
		if (StackUpStyle == EStackUpStyle::Split && SharedData->CommandTeam != ETeamType::TT_SQUAD)
		{
			StackUpActors.Pop();
		}
	}
	
	if (StackUpActors.Num() == 0)
	{
		ACTIVITY_FAILED("No stack up actors available for " + GetCharacter()->GetName() + " in stack up area " + RON_ENUM_TO_STRING(EStackUpGenArea, StackUpArea));
		
		return;
	}
	
	uint8 Index = StackUpSortedSwat.Find(GetCharacter<ASWATCharacter>());

	if (StackUpStyle == EStackUpStyle::Split)
	{
		EStackupGenArea OtherStackUpArea = StackUpArea;
		ADoor::FlipStackUpArea(OtherStackUpArea, true, false);
		TArray<AStackUpActor*> OtherStackUpActors = StackUpDoor->GetStackupsForArea(OtherStackUpArea);
		
		// only pop if we have enough on the other side
		if (OtherStackUpActors.Num() >= 2)
		{
			if (StackUpActors.Num() == 4)
			{
				StackUpActors.Pop();
				StackUpActors.Pop();
			}
			else if (StackUpActors.Num() == 3)
			{
				StackUpActors.Pop();
			}
		}

		uint8 NumOccupied = 0;
		for (AStackUpActor* StackUpActor : StackUpActors)
		{
			if (StackUpActor->OccupiedBy)
				NumOccupied++;
		}
		
		Index = NumOccupied;
	}

	bool bFlipped = false;
	if (!StackUpActors.IsValidIndex(Index))
	{
		bFlipped = true;
		ADoor::FlipStackUpArea(StackUpArea, true, false);
		StackUpActors = StackUpDoor->GetStackupsForArea(StackUpArea);
		uint8 NumOccupied = 0;
		for (AStackUpActor* StackUpActor : StackUpActors)
		{
			if (StackUpActor->OccupiedBy)
				NumOccupied++;
		}
		
		Index = NumOccupied;
		
		// cap the stack up actors to the amount of swat we have
		if (StackUpActors.Num() > StackUpSortedSwat.Num())
		{
			StackUpActors.SetNum(StackUpSortedSwat.Num());
		}

		if (StackUpActors.Num() > 1)
		{
			if (StackUpStyle == EStackUpStyle::Split && SharedData->CommandTeam != ETeamType::TT_SQUAD)
			{
				StackUpActors.Pop();
			}
		}
	}
	
	if (StackUpStyle == EStackUpStyle::Split)
	{
		EStackupGenArea OtherStackUpArea = StackUpArea;
		ADoor::FlipStackUpArea(OtherStackUpArea, true, false);
		TArray<AStackUpActor*> OtherStackUpActors = StackUpDoor->GetStackupsForArea(OtherStackUpArea);
		
		// only pop if we have enough on the other side
		if (OtherStackUpActors.Num() >= 2)
		{
			if (StackUpActors.Num() == 4)
			{
				StackUpActors.Pop();
				StackUpActors.Pop();
			}
			else if (StackUpActors.Num() == 3)
			{
				StackUpActors.Pop();
			}
		}
	}
	
	uint8 NumOccupied = 0;
	for (AStackUpActor* StackUpActor : StackUpActors)
	{
		if (StackUpActor->OccupiedBy)
			NumOccupied++;
	}
	
	Index = NumOccupied;
	
	bool bBetaSameLevelAsAlpha = false;
	bool bDeltaSameLevelAsCharlie = false;
	bool bAllPositionsSameLevelAsAlpha = false;
	bool bCharlieSameLevelAsBeta = false;
	
	if (StackUpActors.Num() > 1)
		bBetaSameLevelAsAlpha = StackUpActors[1]->Depth == StackUpActors[0]->Depth;

	if (StackUpActors.Num() > 2)
		bCharlieSameLevelAsBeta = StackUpActors[2]->Depth == StackUpActors[1]->Depth;
	
	if (StackUpActors.Num() > 3)
		bDeltaSameLevelAsCharlie = StackUpActors[3]->Depth == StackUpActors[2]->Depth;
	
	if (StackUpActors.Num() > 0)
	{
		bAllPositionsSameLevelAsAlpha =	true;
		
		for (AStackUpActor* StackUpActor : StackUpActors)
		{
			if (StackUpActors[0]->Depth != StackUpActor->Depth)
			{
				bAllPositionsSameLevelAsAlpha =	false;
				break;
			}
		}
	}
	
	const bool bDeltaSameLevelAsCharlieAndBeta = bDeltaSameLevelAsCharlie && bCharlieSameLevelAsBeta;

	AStackUpActor* FurthestUnoccupiedStackUp = nullptr;
	if (bNewStackUpDoor)
	{
		const bool bSameSide = StackUpDoor->IsPointInFrontOfDoorway(GetCharacter()->GetActorLocation()) == bIsCommandFrontOfDoor;
		//const bool bIsOnRight = bIsCommandFrontOfDoor ? !StackUpDoor->IsPointRightOfDoorway(GetCharacter()->GetActorLocation()) : StackUpDoor->IsPointRightOfDoorway(GetCharacter()->GetActorLocation());
		
		const bool bOpposite = DoesStackUpPathGoThroughDoor();// || bStackOppositeSide;
		if (bOpposite)
		{
			if (bStackOppositeSide || !bSameSide)
			{
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (StackUpActor->OccupiedBy == nullptr)
					{
						FurthestUnoccupiedStackUp = StackUpActor;
					}
				}
			}
			else
			{
				FurthestUnoccupiedStackUp = UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation<AStackUpActor>(GetCharacter()->GetNavAgentLocation(), StackUpActors, [](const AStackUpActor* StackUpActor, float Distance)
				{
					if (StackUpActor->OccupiedBy != nullptr)
						return false;

					return true;
				});
			}
		}
		else
		{
			// not in the same stack up area, so find the furthest one
			EStackupGenArea CurrentStackUpArea;
			{
				const FVector DirectionToDoor = (GetCharacter()->GetActorLocation() - StackUpDoor->GetDoorMidLocation()).GetSafeNormal2D();
				const float RightDotProduct = FVector::DotProduct(DirectionToDoor, StackUpDoor->GetActorRightVector());
				const bool bIsOnRightSideOfDoor = RightDotProduct > 0.0f;

				if (bIsCommandFrontOfDoor)
				{
					CurrentStackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
				}
				else
				{
					CurrentStackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
				}
			}
			
			if (StackUpArea != CurrentStackUpArea || StackUpStyle == PreviousStackUpStyle)
			{
				// fill in furthest depth
				AStackUpActor* FurthestStackUpDepth = nullptr;
				uint8 MaxDepth = StackUpSortedSwat.Num()-1;
				uint8 i = 0;
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (MaxDepth == i)
						break;
					
					if (StackUpActor->OccupiedBy == nullptr)
					{
						uint8 Depth = StackUpActor->Depth;

						for (AStackUpActor* OtherStackUpActor : StackUpActors)
						{
							if (OtherStackUpActor != StackUpActor &&
								OtherStackUpActor->OccupiedBy == nullptr &&
								OtherStackUpActor->Depth == Depth &&
								FVector::Distance(OtherStackUpActor->GetActorLocation(), GetCharacter()->GetNavAgentLocation()) > 300.0f)
							{
								FurthestStackUpDepth = OtherStackUpActor;
							}
						}

						if (FurthestStackUpDepth)
						{
							FurthestUnoccupiedStackUp = FurthestStackUpDepth;
							break;
						}
					}
					
					i++;
				}
				
				if (!FurthestUnoccupiedStackUp)
				{
					FurthestUnoccupiedStackUp = UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation<AStackUpActor>(GetCharacter()->GetNavAgentLocation(), StackUpActors, [](const AStackUpActor* StackUpActor, float Distance)
					{
						if (StackUpActor->OccupiedBy != nullptr)
							return false;

						if (Distance < 200.0f)
							return false;

						return true;
					});
				}
			}
			
			if (!FurthestUnoccupiedStackUp)
			{
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (StackUpActor->OccupiedBy == nullptr)
					{
						OccupiedStackUpActor = StackUpActor;
						OccupiedStackUpActor->OccupiedBy = OwningController;
						OverrideSquadPosition = StackUpActor->GetSquadPosition();
						break;
					}
				}
			}
		}
	}
	else
	{
		if (bWasSplitUp && !bFlipped)
		{
			if (StackUpArea != PreviousStackUpArea)
			{
				ESquadPosition Position = ESquadPosition::SP_NONE;
				if (!bBetaSameLevelAsAlpha && Index >= 2)
				{
					Position = Index%2 == 0 ? ESquadPosition::SP_Alpha : ESquadPosition::SP_Beta;
				}
				else if (!bDeltaSameLevelAsCharlie && Index <= 1)
				{
					Position = Index%2 == 0 ? ESquadPosition::SP_Charlie : ESquadPosition::SP_Delta;
				}

				if (Position != ESquadPosition::SP_NONE)
				{
					for (AStackUpActor* StackUpActor : StackUpActors)
					{
						if (StackUpActor->OccupiedBy == nullptr && Position == StackUpActor->GetSquadPosition())
						{
							FurthestUnoccupiedStackUp = StackUpActor;
							break;
						}
					}
				}
			}
			
			if (!FurthestUnoccupiedStackUp)
			{
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (StackUpActor->OccupiedBy == nullptr)
					{
						FurthestUnoccupiedStackUp = StackUpActor;
					}
				}
			}
		}
		else
		{
			ESquadPosition Position = ESquadPosition::SP_NONE;
			if (!bFlipped)
			{
				bool bReversed = false;

				if (!(bAllPositionsSameLevelAsAlpha || bPreviousAllPositionsSameLevelAsAlpha) &&
					bDeltaSameLevelAsCharlieAndBeta &&
					Index >= 1 && PreviousStackUpStyle != EStackUpStyle::Split)
				{
					Position = (ESquadPosition)((StackUpActors.Num()-1)-(Index-1));
					bReversed = true;
				}
				else
				{
					const bool bReverse = StackUpStyle != EStackUpStyle::Split &&
										((bAllPositionsSameLevelAsAlpha && PreviousStackUpStyle != EStackUpStyle::Split));
					if (bReverse)
					{
						//const int32 Missing = 4-StackUpActors.Num();
						//Position = (ESquadPosition)((StackUpActors.Num()-1)-(Index+1)+Missing);
						Position = (ESquadPosition)(StackUpSortedSwat.Num() - Index - 1);
						bReversed = true;
					}
					else
					{
						if (PreviousStackUpStyle != EStackUpStyle::Split)
						{
							if (bBetaSameLevelAsAlpha && Index <= 1)
							{
								Position = (Index%2) == 0 ? ESquadPosition::SP_Beta : ESquadPosition::SP_Alpha;
							}
							else if (bDeltaSameLevelAsCharlie && Index >= 2)
							{
								Position = (Index%2) == 0 ? ESquadPosition::SP_Delta : ESquadPosition::SP_Charlie;
							}
							else if (bCharlieSameLevelAsBeta && Index >= 1)
							{
								Position = (Index%2) == 0 ? ESquadPosition::SP_Charlie : ESquadPosition::SP_Beta;
							}
						}
						else
						{
							if (bBetaSameLevelAsAlpha && Index >= 2)
							{
								Position = (Index%2) == 0 ? ESquadPosition::SP_Beta : ESquadPosition::SP_Alpha;
							}
							else if (bDeltaSameLevelAsCharlie && Index <= 1)
							{
								Position = (Index%2) == 0 ? ESquadPosition::SP_Delta : ESquadPosition::SP_Charlie;
							}
						}
					}
				}
				
				if (StackUpArea == PreviousStackUpArea && StackUpStyle != EStackUpStyle::Split && !bReversed)
				{
					const uint8 Offset = PreviousStackUpStyle == EStackUpStyle::Split && StackUpActors.Num() > 3 ? 2 : 1;
					Position = (ESquadPosition)((uint8)(PreviousSquadPosition)+Offset);
				}
			}
			
			if (Position != ESquadPosition::SP_NONE)
			{
				Position = (ESquadPosition)FMath::Clamp<uint8>((uint8)Position, 0, StackUpActors.Num()-1);
				
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (StackUpActor->OccupiedBy == nullptr && Position == StackUpActor->GetSquadPosition())
					{
						OccupiedStackUpActor = StackUpActor;
						OccupiedStackUpActor->OccupiedBy = OwningController;
						OverrideSquadPosition = StackUpActor->GetSquadPosition();
						break;
					}
				}
			}

			if (!OccupiedStackUpActor)
			{
				for (AStackUpActor* StackUpActor : StackUpActors)
				{
					if (StackUpActor->OccupiedBy == nullptr)
					{
						OccupiedStackUpActor = StackUpActor;
						OccupiedStackUpActor->OccupiedBy = OwningController;
						OverrideSquadPosition = StackUpActor->GetSquadPosition();
						break;
					}
				}
			}
		}
	}
	
	if (FurthestUnoccupiedStackUp)
	{
		OccupiedStackUpActor = FurthestUnoccupiedStackUp;
		OccupiedStackUpActor->OccupiedBy = OwningController;
		OverrideSquadPosition = FurthestUnoccupiedStackUp->GetSquadPosition();
	}

	if (!OccupiedStackUpActor)
	{
		ACTIVITY_FAILED("Unable to occupy a stack up actor for " + GetCharacter()->GetName());
		return;
	}

	OverrideSquadPosition = OccupiedStackUpActor->GetSquadPosition();
	
	ChosenStackUpArea = StackUpArea;
	PreviousStackUpArea = StackUpArea;
	PreviousSquadPosition = OverrideSquadPosition;
	bPreviousAllPositionsSameLevelAsAlpha = bAllPositionsSameLevelAsAlpha;

	for (AStackUpActor* StackUpActor : StackUpActors)
	{
		if (StackUpActor->GetSquadPosition() > OverrideSquadPosition)
		{
			if (StackUpActor->Depth == OccupiedStackUpActor->Depth)
			{
				bHigherPositionSameDepth = true;
				break;
			}
		}
	}
}

void UTeamStackUpActivity::MoveToOriginalLocation()
{
	GetCharacter()->StopTPMontageFromTable(DoorCheckAnimMontage, 0.35f);
	
	if (OccupiedStackUpActor)
	{
		StubLocation = OccupiedStackUpActor->GetActorLocation();
		SetLocation(StubLocation, true);
		
		//Location = OccupiedStackUpActor->GetActorLocation();
		//RequestMoveAsync();
	}
}

void UTeamStackUpActivity::ResetData()
{
	Super::ResetData();

	bDontCalculateStackUp = false;
	bHigherPositionSameDepth = false;
	bFoundStackUpPath = false;
	
	StackUpPath.Empty();
	StackUpPathLength = 0.0f;
	StackUpAsyncPathId = 0;
	
	StackUpStyleOverride = EStackUpStyle::Auto;

	bIsCollapsing = false;

	StubLocation = FVector::ZeroVector;
}

float UTeamStackUpActivity::GetDestinationTolerance() const
{
	return 25.0f;
}

bool UTeamStackUpActivity::CanBePushed() const
{
	if (GetActiveStateID() == 2)
		return true;
	
	return GetCharacter() != DoorChecker;
}

void UTeamStackUpActivity::Transfer(UTeamStackUpActivity* OtherActivity)
{
	OtherActivity->bHigherPositionSameDepth = bHigherPositionSameDepth;
	OtherActivity->ChosenStackUpArea = ChosenStackUpArea;
	OtherActivity->StackUpPath = StackUpPath;
	OtherActivity->StackUpPathLength = StackUpPathLength;
	OtherActivity->StackUpAsyncPathId = 0;
	OtherActivity->OccupiedStackUpActor = OccupiedStackUpActor;
	OtherActivity->OverrideSquadPosition = OverrideSquadPosition;
	OtherActivity->StubLocation = StubLocation;
	
	OtherActivity->bPreviousAllPositionsSameLevelAsAlpha = bPreviousAllPositionsSameLevelAsAlpha;
	OtherActivity->PreviousSquadPosition = PreviousSquadPosition;
	
	OtherActivity->PreviousStackUpArea = PreviousStackUpArea;
	
	if (OtherActivity->OccupiedStackUpActor)
	{
		OtherActivity->OccupiedStackUpActor->OccupiedBy = OccupiedStackUpActor->OccupiedBy;
	}
}

void UTeamStackUpActivity::SwapSquadPositionTo(ESquadPosition SquadPosition, bool bSameSide)
{
	if (ChosenStackUpArea == EStackupGenArea::SGA_None)
		return;
	
	if (!HasReachedLocation(GetDestinationTolerance()))
		return;
	
	EStackupGenArea FlippedStackUpArea = ChosenStackUpArea;
	if (!bSameSide)
		ADoor::FlipStackUpArea(FlippedStackUpArea, true, false);
	TArray<AStackUpActor*> OtherStackUpActors = StackUpDoor->GetStackupsForArea(FlippedStackUpArea);
	AStackUpActor* FoundStackUp = nullptr;
	for (AStackUpActor* Other : OtherStackUpActors)
	{
		if (Other->GetSquadPosition() == SquadPosition)
		{
			if (Other->OccupiedBy)
			{
				if (const ACyberneticController* Controller = Cast<ACyberneticController>(Other->OccupiedBy))
				{
					if (UTeamStackUpActivity* StackUpActivity = Controller->GetActivity<UTeamStackUpActivity>())
					{
						StackUpActivity->SwapSquadPositionTo((ESquadPosition)((uint8)SquadPosition+1), true);
					}
				}
				else
				{
					return;
				}
			}
			
			FoundStackUp = Other;
			break;
		}
	}

	if (!FoundStackUp)
		return;

	PreviousSquadPosition = OverrideSquadPosition;
	OverrideSquadPosition = FoundStackUp->GetSquadPosition();

	if (OccupiedStackUpActor)
	{
		OccupiedStackUpActor->OccupiedBy = nullptr;
	}

	OccupiedStackUpActor = FoundStackUp;
	OccupiedStackUpActor->OccupiedBy = OwningController;

	PreviousStackUpArea = ChosenStackUpArea;
	ChosenStackUpArea = FlippedStackUpArea;
	
	for (const AStackUpActor* StackUpActor : OtherStackUpActors)
	{
		if (StackUpActor->GetSquadPosition() > OverrideSquadPosition)
		{
			if (StackUpActor->Depth == OccupiedStackUpActor->Depth)
			{
				bHigherPositionSameDepth = true;
				break;
			}
		}
	}

	bIsSwapping = true;

	StubLocation = OccupiedStackUpActor->GetActorLocation();
	SetLocation(StubLocation, true);
	//Location = OccupiedStackUpActor->GetActorLocation();
	//RequestMoveAsync();
}

void UTeamStackUpActivity::SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated)
{
	// can't swap with yourself :p
	if (SquadPosition == OverrideSquadPosition)
		return;

	SwapSquadPositionWith(SquadPosition, false, bLeadInitiated);
}

void UTeamStackUpActivity::SwapSquadPositionWith(ESquadPosition SquadPosition, bool bOtherSide, bool bLeadInitiated)
{
	if (!HasReachedLocation(GetDestinationTolerance()))
		return;
		
	if (!OccupiedStackUpActor)
		return;

	UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
	{
		if (Activity != this && Activity->ActivityId == ActivityId)
		{
			const bool bIsSameSide = bOtherSide ? Activity->ChosenStackUpArea != ChosenStackUpArea : Activity->ChosenStackUpArea == ChosenStackUpArea;
			if (Activity->OccupiedStackUpActor && Activity->OverrideSquadPosition == SquadPosition && bIsSameSide)
			{
				Swap(OverrideSquadPosition, Activity->OverrideSquadPosition);

				if (OccupiedStackUpActor)
					OccupiedStackUpActor->OccupiedBy = Activity->OwningController;

				if (Activity->OccupiedStackUpActor)
					Activity->OccupiedStackUpActor->OccupiedBy = OwningController;
				
				Swap(OccupiedStackUpActor, Activity->OccupiedStackUpActor);
				Swap(ChosenStackUpArea, Activity->ChosenStackUpArea);
				Swap(PreviousStackUpArea, Activity->PreviousStackUpArea);
				Swap(PreviousSquadPosition, Activity->PreviousSquadPosition);
				Swap(bHigherPositionSameDepth, Activity->bHigherPositionSameDepth);

				bIsSwapping = true;
				Activity->bIsSwapping = true;

				StubLocation = OccupiedStackUpActor->GetActorLocation();
				SetLocation(StubLocation, true);
				//Location = OccupiedStackUpActor->GetActorLocation();
				//RequestMoveAsync();
				Activity->StubLocation = Activity->OccupiedStackUpActor->GetActorLocation();
				Activity->SetLocation(Activity->StubLocation);
				//Activity->Location = Activity->OccupiedStackUpActor->GetActorLocation();
				//Activity->RequestMoveAsync();

				if (!bLeadInitiated)
					GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SWAP);
				
				return false;
			}
		}
		
		return true;
	});
}

bool UTeamStackUpActivity::CanSwapSquadPositions() const
{
	return HasReachedLocation(GetDestinationTolerance()) && GetActiveStateID() != 1;
}

bool UTeamStackUpActivity::GetLeanOverride(float& LeanOverride) const
{
	if (GetActiveStateID() < 2) // stacked
		return false;
	
	// lean on open door
	if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
	{
		if (StackUpDoor->IsOpen())
		{
			const ADoor* ClosestDoor = StackUpDoor;
			bool bInvert = false;
			if (StackUpDoor->GetSubDoor())
			{
				const float A = FVector::Distance(StackUpDoor->GetActorLocation(), GetCharacter()->GetNavAgentLocation());
				const float B = FVector::Distance(StackUpDoor->GetSubDoor()->GetActorLocation(), GetCharacter()->GetNavAgentLocation());

				if (B < A)
				{
					ClosestDoor = StackUpDoor->GetSubDoor();
					bInvert = true;
				}
			}
			
			FTransform Transform;
			Transform.SetLocation(ClosestDoor->GetDoorMidLocation());
			Transform.SetRotation(ClosestDoor->GetActorRotation().Quaternion());

			const float DoorWidth = ClosestDoor->GetDoorSize().Y - 5.0f;
			
			const FVector Extent = FVector(300.0f, DoorWidth, 120.0f);
			
			//DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 1.0f);
			
			const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->GetActorLocation(), Transform, Extent);
			const bool bFocalPointInsideThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->Rep_FocalPoint, Transform, Extent);
			if (bInsideDoorThreshold && bFocalPointInsideThreshold)
				return false;

			float LeanDirection;
			if (IsCommandFrontOfDoor())
			{
				LeanDirection = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? 1.0f : -1.0f;
			}
			else
			{
				LeanDirection = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? -1.0f : 1.0f;
			}

			if (GetSharedData<FSharedStackUpData>()->StackingRoomPosition == EDoorRoomPosition::Center && !ClosestDoor->IsDoorwayOnly() && !ClosestDoor->GetSubDoor())
			{
				EDoorRoomPosition InsideRoomPosition = EDoorRoomPosition::Center;
				if (ClosestDoor->FrontRoomPosition == GetSharedData<FSharedStackUpData>()->StackingRoomPosition && IsCommandFrontOfDoor())
				{
					InsideRoomPosition = ClosestDoor->FrontRoomPosition;
				}
				else if (ClosestDoor->BackRoomPosition == GetSharedData<FSharedStackUpData>()->StackingRoomPosition && !IsCommandFrontOfDoor())
				{
					InsideRoomPosition = ClosestDoor->BackRoomPosition;
				}

				switch (InsideRoomPosition)
				{
					case EDoorRoomPosition::Center:
					case EDoorRoomPosition::CornerLeft:
					case EDoorRoomPosition::CornerRight:
						LeanDirection = IsCommandFrontOfDoor() ? 1.0f : -1.0f;
					break;
					
					case EDoorRoomPosition::Hallway:
					case EDoorRoomPosition::HallwayLeft:
					case EDoorRoomPosition::HallwayRight:
					break;
					
					default: break;
				}
			}
			
			const float FinalLeanDirection = LeanDirection;
			
			LeanOverride = FinalLeanDirection;
			if (bInvert)
				LeanOverride = -LeanOverride;
			return true;
		}
		
		return false;
	}
	
	return false;
}

bool UTeamStackUpActivity::GetLowReadyOverride(bool& bLowReady) const
{
	if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
	{
		bLowReady = false;
		return true;
	}

	return false;
}

bool UTeamStackUpActivity::ShouldDisableMoveRequest() const
{
	if (DoorToClose && GetActiveStateID() <= 2)
	{
		return !DoorToClose->IsClosed() && !DoorToClose->IsClosing();
	}
	
	return false;
}

bool UTeamStackUpActivity::AllStacked(bool bLocationCheck) const
{
	bool bAnyNotStacked = false;

	UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
	{
		if (Activity->ActivityId == ActivityId && Activity->ActivityStatus != EActivityStatus::Complete)
		{
			const int32 ID = Activity->GetActiveStateID();
			const bool bReachedLocation = bLocationCheck ? HasReachedLocation(GetDestinationTolerance()) : false;
			if (ID >= 0 && ID < 2 && !bReachedLocation)
			{
				bAnyNotStacked = true;
				return false;
			}
		}

		return true;
	});

	return !bAnyNotStacked;
}

bool UTeamStackUpActivity::AllStackUpPathsReady() const
{
	//LOG_NUMBER(GetSharedData<FSharedStackUpData>()->NumInTeam);
	//return GetSharedData<FSharedStackUpData>()->NumReady >= GetSharedData<FSharedStackUpData>()->NumInTeam;
	
	bool bAnyPathNotFound = false;

	UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
	{
		if (Activity->ActivityId == ActivityId)
		{
			if (!Activity->bFoundStackUpPath)
			{
				bAnyPathNotFound = true;
				return false;
			}
		}

		return true;
	});

	return !bAnyPathNotFound;
}

ACyberneticCharacter* UTeamStackUpActivity::FindChecker() const
{
	const bool bIsCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);

	ACyberneticCharacter* CheckerA = GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, bIsCommandFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight);
	ACyberneticCharacter* CheckerB = GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, bIsCommandFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);

	/*
	#if WITH_EDITOR
	if (CheckerA)
		ensureAlways(GetSquadPositionForCharacter(CheckerA) == ESquadPosition::SP_Alpha);
	if (CheckerB)
		ensureAlways(GetSquadPositionForCharacter(CheckerB) == ESquadPosition::SP_Alpha);
	#endif
	*/

	if (!CheckerA && !CheckerB)
		return nullptr;

	if (CheckerA && !CheckerB)
		return CheckerA;

	if (!CheckerA && CheckerB)
		return CheckerB;

	if (CheckerA == CheckerB)
		return CheckerA;

	if (CheckerA->GetInventoryComponent()->GetInventoryItemOfClass(ABallisticsShield::StaticClass()))
		return CheckerB;
	
	if (CheckerB->GetInventoryComponent()->GetInventoryItemOfClass(ABallisticsShield::StaticClass()))
		return CheckerA;

	const FVector DoorLocation = StackUpDoor->GetActorLocation() + StackUpDoor->GetActorRightVector() * 115.0f;
	
	const float A = (DoorLocation - CheckerA->GetActorLocation()).SizeSquared2D();
	const float B = (DoorLocation - CheckerB->GetActorLocation()).SizeSquared2D();
	if (A < B)
	{
		return CheckerA;
	}
	
	return CheckerB;
}

ESquadPosition UTeamStackUpActivity::GetSquadPositionForCharacter(const ACyberneticCharacter* InSwatCharacter) const
{
	if (!InSwatCharacter)
		return ESquadPosition::SP_NONE;

	if (InSwatCharacter->IsDeadOrUnconscious())
		return ESquadPosition::SP_NONE;
	
	if (InSwatCharacter->GetCyberneticsController())
	{
		if (const UTeamStackUpActivity* TSA = InSwatCharacter->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
		{
			return TSA->OverrideSquadPosition;
		}
	}

	return ESquadPosition::SP_NONE;
}

bool UTeamStackUpActivity::GetOverrideMovementSpeed(float& OutMovementSpeed) const
{
	if (GetActiveStateID() == 0)
	{
		if (Location != FVector::ZeroVector)
		{
			const TArray<ASWATCharacter*>& StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
			
			ACyberneticCharacter* Leader = nullptr;
			if (StackUpSortedSwat.Num() > 0)
			{
				uint8 Index = StackUpSortedSwat.Find(GetCharacter<ASWATCharacter>());
				if (StackUpSortedSwat.IsValidIndex(Index-1))
				{
					if (ACyberneticCharacter* Alpha = StackUpSortedSwat[Index-1])
					{
						Leader = Alpha;
					}
				}
			}

			if (!bNewStackUpDoor)
			{
				Leader = nullptr;
				if (bHigherPositionSameDepth)
					Leader = GetCharacterAtSquadPosition((ESquadPosition)((uint8)OverrideSquadPosition+1));
			}
			
			/*
			const ACyberneticCharacter* Leader = nullptr;
			if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
				Leader = GetCharacterAtSquadPosition((ESquadPosition)((uint8)OverrideSquadPosition-1));
			*/

			const float MaxAlphaSpeed = bNewStackUpDoor ? 240.0f : 200.0f;
			OutMovementSpeed = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 300.0f), FVector2D(150.0f, MaxAlphaSpeed), FVector::Distance(Location, GetCharacter()->GetNavAgentLocation()));
			
			if (Leader && Leader->bIsMoving)
			{
				const float DistanceToLeader = FVector::Distance(Leader->GetNavAgentLocation(), GetCharacter()->GetNavAgentLocation());
			
				float MaxSpeed = FMath::Clamp(Leader->GetCharacterMovement()->GetMaxSpeed() * 0.95f, 150.0f, 250.0f);
				if (DistanceToLeader > 400.0f)
					MaxSpeed = Leader->GetCharacterMovement()->GetMaxSpeed();

				OutMovementSpeed = MaxSpeed;
				//OutMovementSpeed = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 300.0f), FVector2D(110.0f, MaxSpeed), Distance);
				//ULog::Info(GetCharacter()->GetName() + " my leader is: " + Leader->GetName() + " following at speed: " + FString::SanitizeFloat(OutMovementSpeed));
			}
			
			return true;
		}

		return false;
	}
	
	if (DoorChecker == GetCharacter() && GetActiveStateID() > 0 && !HasReachedLocation(GetDestinationTolerance()) && HasTeamReachedPosition())
	{
		OutMovementSpeed = 150.0f;
		return true;
	}

	if (GetActiveStateID() <= 2)
	{
		OutMovementSpeed = 150.0f;
		return true;
	}
	
	return false;
}

void UTeamStackUpActivity::ResumeActivity()
{
	Super::ResumeActivity();

	if (OccupiedStackUpActor)
	{
		OccupiedStackUpActor->DisableNavBlocker();

		StubLocation = OccupiedStackUpActor->GetActorLocation();
		SetLocation(StubLocation, true);
		//Location = OccupiedStackUpActor->GetActorLocation();
		//RequestMoveAsync();
	}
}

bool UTeamStackUpActivity::ShouldForceStrafe() const
{
	if (bIsSwapping)
		return false;
	
    if (GetActiveStateID() >= 1 && GetActiveStateID() <= 2 && HasReachedLocation(100.0f))
    {
        return OverrideSquadPosition == ESquadPosition::SP_Alpha || IsFurthestOccupiedStackUpInArea();
    }
	
	return false;
}

bool UTeamStackUpActivity::ShouldForceNoStrafe() const
{
	if (bIsSwapping)
		return true;
	
	if (DoorChecker == GetCharacter() && GetActiveStateID() == 1)
	{
		return true;
	}
	
	return false;
}

bool UTeamStackUpActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (OwningController->GetTrackedTarget() && OverrideSquadPosition != ESquadPosition::SP_Alpha)
	{
		return false;
	}
	
	const TArray<ASWATCharacter*>& StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
	
	if (HasReachedLocation(150.0f) && StackUpSortedSwat.Num() > 2 && OverrideSquadPosition > ESquadPosition::SP_Alpha)
	{
		float FurthestDistance = 0.0f;
		const AStackUpActor* FurthestStackUp = nullptr;
		
		UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
		{
			if (Activity->ActivityId == ActivityId && Activity->GetStackUpArea() == ChosenStackUpArea)
			{
				if (Activity->OccupiedStackUpActor)
				{
					const float Distance = FVector::Distance(Activity->StackUpDoor->GetActorLocation(), Activity->OccupiedStackUpActor->GetActorLocation());
					if (Distance > FurthestDistance)
					{
						FurthestDistance = Distance;
						FurthestStackUp = Activity->OccupiedStackUpActor;
					}
				}
			}

			return true;
		});

		const bool bHasCustomFocusLocation = OwningController->GetTargetingComp()->CustomFocusActor != nullptr || 
											OwningController->GetTargetingComp()->CustomFocusLocation != FVector::ZeroVector;

		const bool bCharlieOccupied = GetCharacterAtSquadPosition(ESquadPosition::SP_Charlie) || IsLeaderOccupyingSquadPosition(ESquadPosition::SP_Charlie);
		const bool bDeltaOccupied = GetCharacterAtSquadPosition(ESquadPosition::SP_Delta) || IsLeaderOccupyingSquadPosition(ESquadPosition::SP_Delta);
		
		const bool bIsFurthestOccupiedStackUp = FurthestStackUp && FurthestStackUp->OccupiedBy == OwningController;
		if ((OverrideSquadPosition == ESquadPosition::SP_Delta || bIsFurthestOccupiedStackUp) ||
			((OverrideSquadPosition == ESquadPosition::SP_Beta && !bCharlieOccupied) || bHasCustomFocusLocation) ||
			((OverrideSquadPosition == ESquadPosition::SP_Charlie && !bDeltaOccupied) || bHasCustomFocusLocation) &&
			HasReachedLocation(200.0f))
		{
			return false;
		}

		if (StackUpSortedSwat.Num() > 0)
		{
			if (StackUpSortedSwat.Last() == GetCharacter() && !HasReachedLocation(200.0f))
			{
				return false;
			}
		}
	}

	if (StackUpDoor->IsPointsOnOppositeSideOfDoor(SharedData->CommandLocation, GetCharacter()->GetActorLocation()))
	{
		return false;
	}

	FocalPoint = GetDoorFocalPoint();
	return true;
}

TArray<UTeamStackUpActivity*> UTeamStackUpActivity::GetTotalSwatInStackUpArea(const EStackupGenArea StackUpArea) const
{
	if (StackUpArea == EStackupGenArea::SGA_None)
		return {};

	TArray<UTeamStackUpActivity*> StackUpActivities;
	
	UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
	{
		if (Activity->ActivityId == ActivityId)
		{
			StackUpActivities.Add(Activity);
		}

		return true;
	});
	
	TArray<ASWATCharacter*> FinalCharacters;
	FinalCharacters.Reserve(StackUpActivities.Num());
	
	if (StackUpArea == EStackupGenArea::SGA_All)
	{
		return StackUpActivities;
	}

	TArray<UTeamStackUpActivity*> StackUpActivities_Local;
	for (UTeamStackUpActivity* Activity : StackUpActivities)
	{
		if (Activity->ChosenStackUpArea == EStackupGenArea::SGA_None ||
			Activity->ChosenStackUpArea == EStackupGenArea::SGA_All ||
			Activity->ChosenStackUpArea == StackUpArea)
		{
			StackUpActivities_Local.Add(Activity);
		}
	}

	StackUpActivities_Local.Sort([&](const UTeamStackUpActivity& Lhs, const UTeamStackUpActivity& Rhs)
	{
		return Lhs.OverrideSquadPosition < Rhs.OverrideSquadPosition;
	});

	return StackUpActivities_Local;
}

void UTeamStackUpActivity::AbortActivityOnPathNotFound()
{
	Super::AbortActivityOnPathNotFound();

	if (GetActiveStateID() > 4)
	{
		if (OwningController)
			OwningController->FinishActivity(this, false, true);
		
		return;
	}
		
	if (USWATManager* SWATManager = USWATManager::Get(this))
	{
		SWATManager->PlaySpeechWithSharedCooldown(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_POSITION, GetCharacter(), 1.75f);
	}
}

FVector UTeamStackUpActivity::GetDoorFocalPoint() const
{
	FVector FocalPoint = FVector::ZeroVector;
	
	if (StackUpDoor->IsPointsOnOppositeSideOfDoor(SharedData->CommandLocation, GetCharacter()->GetActorLocation()))
	{
		return FocalPoint;
	}
	
	const ADoor* ClosestDoor = StackUpDoor;
	if (StackUpDoor->GetSubDoor())
	{
		const float A = FVector::Distance(StackUpDoor->GetActorLocation(), GetCharacter()->GetNavAgentLocation());
		const float B = FVector::Distance(StackUpDoor->GetSubDoor()->GetActorLocation(), GetCharacter()->GetNavAgentLocation());

		if (B < A)
		{
			ClosestDoor = StackUpDoor->GetSubDoor();
		}
	}
	
	if (ClosestDoor)
	{
		if (FVector::Distance(ClosestDoor->GetDoorMidLocation(), GetCharacter()->GetActorLocation()) > 1000.0f)
			return FVector::ZeroVector;

		const bool bFrontOfDoor = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);

		if (ClosestDoor->IsDoorwayOnly() || ClosestDoor->IsOpenBeyondCloseThreshold())
		{
			FocalPoint = ClosestDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation());
		}
		else
		{
			if (ClosestDoor->GetSubDoor())
			{
				FocalPoint = ClosestDoor->GetDoorMidLocation() + ClosestDoor->GetActorRightVector() * 40.0f;
			}
			else
			{
				FocalPoint = ClosestDoor->GetDoorMidLocation();
			}
		}
		
		FocalPoint.Z = GetCharacter()->GetActorLocation().Z + 60.0f;

		const EDoorRoomPosition StackingRoomPosition = GetSharedData<FSharedStackUpData>()->StackingRoomPosition;
		EDoorRoomPosition InsideRoomPosition = EDoorRoomPosition::Center;

		bool bInHallway = false;
		if (ClosestDoor->FrontRoomPosition == StackingRoomPosition && bFrontOfDoor)
		{
			bInHallway = ClosestDoor->BackRoomPosition == EDoorRoomPosition::Hallway ||
						ClosestDoor->BackRoomPosition == EDoorRoomPosition::HallwayLeft ||
						ClosestDoor->BackRoomPosition == EDoorRoomPosition::HallwayRight;

			InsideRoomPosition = ClosestDoor->FrontRoomPosition;
		}
		else if (ClosestDoor->BackRoomPosition == StackingRoomPosition && !bFrontOfDoor)
		{
			bInHallway = ClosestDoor->FrontRoomPosition == EDoorRoomPosition::Hallway ||
						ClosestDoor->FrontRoomPosition == EDoorRoomPosition::HallwayLeft ||
						ClosestDoor->FrontRoomPosition == EDoorRoomPosition::HallwayRight;
			
			InsideRoomPosition = ClosestDoor->BackRoomPosition;
		}

		if (OverrideSquadPosition == ESquadPosition::SP_Alpha && ClosestDoor->IsOpenBeyondIncrementThreshold() && HasReachedLocation(100.0f))
		{
			FocalPoint = ClosestDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation());
			FocalPoint += bFrontOfDoor ? -ClosestDoor->GetActorForwardVector() * 100.0f : ClosestDoor->GetActorForwardVector() * 100.0f;
			
			float RightOffset = 0.0f;

			if (StackingRoomPosition == EDoorRoomPosition::CornerRight || StackingRoomPosition == EDoorRoomPosition::CornerLeft)
			{
				if (bInHallway)
				{
					RightOffset = StackingRoomPosition == EDoorRoomPosition::CornerRight ? -150.0f : 150.0f;
				}
				else
				{
					if (StackingRoomPosition == EDoorRoomPosition::CornerRight)
					{
						if (ClosestDoor->IsActorRightOfDoorway(GetCharacter()))
						{
							RightOffset = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? 0.0f : -50.0f;
						}
						else
						{
							RightOffset = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? 50.0f : 0.0f;
						}
					}
					else if (StackingRoomPosition == EDoorRoomPosition::CornerLeft)
					{
						if (ClosestDoor->IsActorRightOfDoorway(GetCharacter()))
						{
							RightOffset = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? 50.0f : 0.0f;
						}
						else
						{
							RightOffset = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? 0.0f : -50.0f;
						}
					}
					
					//RightOffset = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? -50.0f : 50.0f;
				}
			}
			else
			{
				const bool bWideDoorway = ClosestDoor->IsDoorwayOnly() && ClosestDoor->GetDoorSize().Y > 100.0f;

				float Offset = ClosestDoor->IsDoorwayOnly() ? 50.0f : 200.0f;
				float Offset2 = IsCommandFrontOfDoor() ? 0.0f : -25.0f;
				switch (InsideRoomPosition)
				{
					case EDoorRoomPosition::Center:
					case EDoorRoomPosition::CornerLeft:
					case EDoorRoomPosition::CornerRight:
						RightOffset = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? Offset2 : Offset;
					break;
					
					case EDoorRoomPosition::Hallway:
					case EDoorRoomPosition::HallwayLeft:
					case EDoorRoomPosition::HallwayRight:
						if (ClosestDoor->IsActorRightOfDoorway(GetCharacter()))
							RightOffset = Offset2;
					break;
					
					default: break;
				}

				if (bInHallway && ClosestDoor->IsDoorwayOnly())
				{
					if (bWideDoorway)
					{
						Offset = IsCommandFrontOfDoor() ? -50.0f : 50.0f;
						Offset2 = IsCommandFrontOfDoor() ? 50.0f : -50.0f;
						RightOffset = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? Offset2 : Offset;
					}
					else
					{
						RightOffset = 0.0f;
					}
				}
			}
			
			if (ClosestDoor->GetSubDoor())
			{
				RightOffset = ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? -35.0f : 35.0f;
			}
				
			FocalPoint += ClosestDoor->GetActorRightVector() * RightOffset;
			
			FocalPoint.Z = FMath::Max(ClosestDoor->GetDoorMidLocation().Z, GetCharacter()->GetActorLocation().Z + 20.0f);
		}
		
		if (FocalPoint.Z > GetCharacter()->GetActorLocation().Z+50.0f)
			FocalPoint.Z = FocalPoint.Z - (FocalPoint.Z-(GetCharacter()->GetActorLocation().Z+50.0f));
	}

	return FocalPoint;
}

#undef StackUpDoor
#undef DoorChecker
#undef ActivityId
#undef bNewStackUpDoor
#undef bStackOppositeSide
#undef bHasCheckedDoor
#undef DoorCheckResult
#undef bHasStartedCheckingLock
#undef CheckLocation
#undef DoorCheckAnimMontage
#undef DoorToClose
