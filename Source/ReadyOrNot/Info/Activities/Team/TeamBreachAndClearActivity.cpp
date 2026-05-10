// Copyright Void Interactive, 2021

#include "TeamBreachAndClearActivity.h"

#include "DoorBreachActivity.h"
#include "DeployItemAtLocationActivity.h"
#include "DeployWedgeActivity.h"
#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"

#include "Actors/Door.h"
#include "Actors/Items/C2Explosive.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Actors/Gameplay/ThrownChemlight.h"
#include "Actors/BaseGrenade.h"
#include "Actors/CoverLandmark.h"
#include "Actors/StackUpActor.h"
#include "Actors/Items/GrenadeLauncher.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"
#include "Actors/Projectiles/DamageProjectiles/GrenadeProjectile.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/AI/SWATController.h"

#include "Info/SWATManager.h"
#include "Info/Activities/ActivityManagerTemplates.h"
#include "Info/Activities/SearchLandmarkActivity.h"

#include "Actors/Gameplay/VisibilityBlockingVolume.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "Navigation/ReadyOrNotNavQueries.h"

#define StackUpDoor                 GetSharedData<FSharedStackUpData>()->StackUpDoor
#define DoorChecker                 GetSharedData<FSharedStackUpData>()->DoorChecker
#define bNewStackUpDoor             GetSharedData<FSharedStackUpData>()->bNewStackUpDoor
#define bStackOppositeSide          GetSharedData<FSharedStackUpData>()->bStackOppositeSide
#define bHasCheckedDoor             GetSharedData<FSharedStackUpData>()->bHasCheckedDoor
#define DoorCheckResult             GetSharedData<FSharedStackUpData>()->DoorCheckResult
#define bHasStartedCheckingLock     GetSharedData<FSharedStackUpData>()->bHasStartedCheckingLock
#define DoorBreachType              GetSharedData<FSharedBreachData>()->DoorBreachType
#define DoorUser                    GetSharedData<FSharedBreachData>()->DoorUser
#define DoorBreacher                GetSharedData<FSharedBreachData>()->DoorBreacher
#define DoorScanner                 GetSharedData<FSharedBreachData>()->DoorScanner
#define ShieldUser                  GetSharedData<FSharedBreachData>()->ShieldUser
#define DoorUseItemClass            GetSharedData<FSharedBreachData>()->DoorUseItemClass
#define DoorBreachItemClass         GetSharedData<FSharedBreachData>()->DoorBreachItemClass
#define bIsLeaderThrow              GetSharedData<FSharedBreachData>()->bIsLeaderThrow
#define bIsLeaderBreach             GetSharedData<FSharedBreachData>()->bIsLeaderBreach
#define bHasBreacherBreached        GetSharedData<FSharedBreachData>()->bHasBreacherBreached
#define bHasUserBreached            GetSharedData<FSharedBreachData>()->bHasUserBreached
#define bHasLeaderBreached          GetSharedData<FSharedBreachData>()->bHasLeaderBreached
#define bLeaderUsedItem             GetSharedData<FSharedBreachData>()->bLeaderUsedItem
#define bBreacherReady              GetSharedData<FSharedBreachData>()->bBreacherReady
#define bIsBreaching                GetSharedData<FSharedBreachData>()->bIsBreaching
#define DoorUseActivity             GetSharedData<FSharedBreachData>()->DoorUseActivity
#define DoorBreachActivity          GetSharedData<FSharedBreachData>()->DoorBreachActivity
#define DoorScanActivity            GetSharedData<FSharedBreachData>()->DoorScanActivity
#define BreachingTime               GetSharedData<FSharedBreachData>()->BreachingTime
#define ClearingTime                GetSharedData<FSharedBreachData>()->ClearingTime

extern TAutoConsoleVariable<int32> CVarRonDisplaySwatDebug;

UTeamBreachAndClearActivity::UTeamBreachAndClearActivity()
{
    ActivityName = 	FText::FromStringTable("SwatCommandTable", "BreachAndClear");

    bFinishActivityWhenOverriden = false;
    bResetStateMachineWhenActivityResumed = false;

    bAbortActivityIfCannotReachLocation = true;

    bPauseIfTrackingEnemy = false;

    bAllowPartialMove = false;

    // The Stack Up and Check events are binded in the parent class UTeamStackUpActivity
    ActivityStateMachine->GetState("Stack Up")
                        .CreateTransition("Breach", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::CanPerformBreach));
    
    ActivityStateMachine->GetState("Stacked")
                        .CreateTransition("Breach", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::CanPerformBreach));
	
    ActivityStateMachine->AddState("Breach")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::EnterBreachStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::PerformBreachStage))
                        .CreateTransition("Scan", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::ShouldScan), 1)
                        .CreateTransition("Clear", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::CanPerformClear));
    
    ActivityStateMachine->AddState("Scan")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::EnterScanStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::PerformScanStage))
                        .CreateTransition("Clear", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::IsScanFinished));

    ActivityStateMachine->AddState("Clear")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::EnterClearStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::PerformClearStage))
                        .CreateTransition("Cleared", MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::IsFinishedClearing));
    
    ActivityStateMachine->AddState("Cleared")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTeamBreachAndClearActivity::EnterClearedStage));
}

void UTeamBreachAndClearActivity::StartActivity(AAIController* Owner)
{
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("No valid stack up door");
        return;
    }
    
    Super::StartActivity(Owner);

    if (DoorBreachType != EDoorBreachType::Open ||
        (DoorBreachType == EDoorBreachType::Open && !StackUpDoor->IsLocked() && !StackUpDoor->IsJammed()))
    {
        bHasCheckedDoor = true;
        DoorCheckResult = EDoorCheckResult::Unlocked;
        StackUpDoor->SetDoorLockKnowledge(GetCharacter()->IsSuspect(), true);
    }

    if (DoorBreachType == EDoorBreachType::Move)
    {
        bHasUserBreached = true;
        bHasBreacherBreached = !DoorBreachItemClass && !bIsLeaderThrow;
    }
    
    if (bIsLeaderThrow)
    {
        if (AReadyOrNotCharacter* SquadLeader = GetSquadLeader())
        {
            SquadLeader->Event_OnItemPrimaryUse.RemoveAll(this);
            SquadLeader->Event_OnItemPrimaryUse.AddDynamic(this, &UTeamBreachAndClearActivity::OnLeaderItemPrimaryUse);
        }
    }
    
    QueryParameters = GetCharacter()->GetCollisionQueryParameters();
    QueryParameters.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
    QueryParameters.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
    
    if (AReadyOrNotLevelScript* LS = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
        QueryParameters.AddIgnoredActors((TArray<AActor*>)LS->VisibilityBlockingVolumesInLevel);
    
    QueryParameters.bTraceComplex = true;
}

void UTeamBreachAndClearActivity::PerformActivity(const float DeltaTime)
{
    if (!IsValid(StackUpDoor))
    {
        ACTIVITY_FAILED("No valid stack up door");
        return;
    }
    
    if (DoorUser)
    {
        // If the door user is somehow killed, start clearing immediately (if the door has been opened)
        if (DoorUser->IsDeadOrUnconscious())
        {
            if (!StackUpDoor->IsHalfwayOpen())
            {
                ACTIVITY_FAILED("Door user is dead", true);
                return;
            }
        }
    }

    if (DoorBreacher)
    {
        // If the door breacher is somehow killed, start clearing immediately (if the door has been opened)
        if (DoorBreacher->IsDeadOrUnconscious())
        {
            if (!StackUpDoor->IsHalfwayOpen())
            {
                ACTIVITY_FAILED("Door breacher is dead", true);
                return;
            }
        }
    }

    // remove invalid swat
    GetSharedData<FSharedBreachData>()->StackUpSortedSwat.RemoveAll([](const ASWATCharacter* Element)
    {
        return !Element || !Element->IsActive();
    });

    // are we stacking up on a room that we just passed through?
    // this is a hack, need to find the real issue...
    if (GetSharedData<FSharedBreachData>()->bIsAuto && GetActiveStateID() == 0)
    {
        if (const FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
        {
            if (Room->bClearedBySwat)
            {
                OwningController->FinishActivity(this, true, true);
                return;
            }
        }
    }

    // failsafe
    if (DoorBreachType == EDoorBreachType::Move && !StackUpDoor->IsOpen())
    {
        DoorBreachType = EDoorBreachType::Open;
    }

    if (!DoorChecker && ShouldCheckDoorBeforeBreach())
    {
        DoorChecker = FindChecker();
    }

    Super::PerformActivity(DeltaTime);
}

#if !UE_BUILD_SHIPPING
void UTeamBreachAndClearActivity::PerformActivity_Debug(const float DeltaTime)
{
    Super::PerformActivity_Debug(DeltaTime);

    if (CVarRonDisplaySwatDebug.GetValueOnAnyThread() == 0)
        return;
    
    if (Cast<ASWATCharacter>(GetSquadLeader()))
        DrawDebugLine(GetWorld(), StackUpDoor->GetDoorMidLocation(), StackUpDoor->GetDoorMidLocation() + FVector::UpVector * 10000.0f, FColor::Orange, false, 1.0f, 0, 2.0f);
    
    if (GetActiveStateID() == 5)
    {
        if (ChosenClearPoints)
        {
            StackUpDoor->DrawClearPointsV2(*ChosenClearPoints);
        }

        if (CurrentClearPoint)
        {
            FColor PointColor = FColor::White;
            switch (CurrentClearPoint->Direction)
            {
                case EClearDirection::Right:
                    PointColor = FColor::Yellow;
                break;
                
                case EClearDirection::Forward:
                    PointColor = FColor::Magenta;
                break;
                
                default:
                break;
            }
            
            DrawDebugBox(GetWorld(), FVector(CurrentClearPoint->Location), FVector(15.0f), PointColor, false, 0.15f);
            DrawDebugString(GetWorld(), FVector(CurrentClearPoint->Location), FString::FromInt(CurrentClearPoint->Stage), nullptr, FColor::White, 0.15f, true);
        }

        //DrawDebugDirectionalArrow(GetWorld(), GetCharacter()->GetActorLocation(), GetCharacter()->GetActorLocation() + OwningController->GetTargetingComp()->ThreatTrackingIgnoredDirection * 100.0f, 100.0f, FColor::Red, false, DeltaTime+0.05f);
    }

    /*
    TArray<ASWATCharacter*> StackUpSortedSwat = GetSharedData<FSharedStackUpData>()->StackUpSortedSwat;
    StackUpSortedSwat.Remove(Cast<ASWATCharacter>(DoorChecker));
    StackUpSortedSwat.Remove(Cast<ASWATCharacter>(DoorUser));
    StackUpSortedSwat.Remove(Cast<ASWATCharacter>(DoorBreacher));
    if (StackUpSortedSwat.Num()  == 0)
    {
        if (DoorChecker)
        {
            StackUpSortedSwat.Add(Cast<ASWATCharacter>(DoorChecker));
        }
        
        if (StackUpSortedSwat.Num()  == 0)
            return;
    }
    
    if (StackUpDoor && StackUpSortedSwat[0] == GetCharacter())
    {
        FString DebugMessage;
    
        DebugMessage += AddDebugString("Locked", StackUpDoor->IsLocked() ? "true" : "false");
        DebugMessage += AddDebugString("Has Frame", StackUpDoor->bHasFrame ? "true" : "false");
        DebugMessage += AddDebugString("Front Of Door", StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation) ? "true" : "false");
        DebugMessage += AddDebugString("Door Checker", DoorChecker ? DoorChecker->GetName() : "None");
        DebugMessage += AddDebugString("Has Checked Door", bHasCheckedDoor ? "true" : "false");
        DebugMessage += AddDebugString("Breacher Ready", bBreacherReady ? "true" : "false");
        DebugMessage += AddDebugString("Has Breacher Breached", bHasBreacherBreached ? "true" : "false");
        DebugMessage += AddDebugString("Door Check Result", RON_ENUM_TO_STRING(EDoorCheckResult, DoorCheckResult));
        
        DebugMessage += AddDebugString("Door Breach Type", RON_ENUM_TO_STRING(EDoorBreachType, DoorBreachType));
        
        DebugMessage += AddDebugString("Door User", DoorUser ? DoorUser->GetName() : "None");
        DebugMessage += AddDebugString("Door Breacher", DoorBreacher ? DoorBreacher->GetName() : "None");
        
        DebugMessage += AddDebugString("Door Use Item", GetNameSafe(DoorUseItemClass));
        DebugMessage += AddDebugString("Door Breach Item", GetNameSafe(DoorBreachItemClass));
        
        if (DoorUseActivity)
            DebugMessage += AddDebugString("Door Use Activity", DoorUseActivity ? DoorUseActivity->GetName() : "None");
        
        if (DoorBreachActivity)
            DebugMessage += AddDebugString("Door Breach Activity", DoorBreachActivity ? DoorBreachActivity->GetName() : "None");

        if (GetActiveStateName() == "Breach")
            DebugMessage += AddDebugString("Breaching Time", FString::Printf(TEXT("%.2f"), BreachingTime));
        
        if (GetActiveStateName() == "Clear")
            DebugMessage += AddDebugString("Clearing Time", FString::SanitizeFloat(ClearingTime));

        FVector DebugLocation = StackUpDoor->GetDoorMidLocation();
        if (StackUpDoor->GetSubDoor())
            DebugLocation = StackUpDoor->GetDoorMidLocation() + StackUpDoor->GetActorRightVector() * 65.0f;
        
        DrawDebugString(GetWorld(), DebugLocation + FVector::UpVector * 50.0f, DebugMessage, nullptr, FColor::White, OwningController->GetActorTickInterval(), true);
    }
    */
}
#endif

void UTeamBreachAndClearActivity::FinishedActivity(bool bSuccess)
{
    Super::FinishedActivity(bSuccess);
    
	GetCharacter()->MoveIgnoreActorRemove(StackUpDoor);
    
    OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);

    CurrentClearPoint = nullptr;
    
    OnCleared.Clear();

    StackUpDoor->DeactivateDoorBlocker();

    if (GetCharacter()->GetEquippedItem<ABallisticsShield>())
        EquipWeapon();

    TryDeactivateDoorBlocker(GetSharedData<FSharedBreachData>()->Room);
    TryDeactivateDoorBlocker(GetSharedData<FSharedBreachData>()->CurrentRoom);
    
    StackUpDoor->DeactivateBreachBlockers();
}

void UTeamBreachAndClearActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
    Super::GatherDebugString(OutString);

    OutString += AddDebugString("Leader Passed Threshold", bHasLeaderEverPassedThreshold ? "true" : "false");
    OutString += AddDebugString("Is Swapping", bIsSwapping ? "true" : "false");
    
    if (GetActiveStateName() == "Clear")
    {
        OutString += AddDebugString("Has Clearing Point", CurrentClearPoint ? "true" : "false");
        OutString += AddDebugString("Current Clear Stage", FString::FromInt(CurrentClearStage));
        OutString += AddDebugString("Stage Limit", FString::FromInt(StageLimit));
        OutString += AddDebugString("Clearing Leader", GetNameSafe(ClearingLeader));
        OutString += AddDebugString("Clearing Index", FString::FromInt(GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Find(GetCharacter<ASWATCharacter>())+1));
    }
    
    OutString += AddDebugString("Is Finished Clearing", IsFinishedClearing() ? "true" : "false");
    #endif
}

bool UTeamBreachAndClearActivity::CanPerformCheck() const
{
    if (!OccupiedStackUpActor)
        return false;
    
    if (ShouldCheckDoorBeforeBreach())
    {
        return Super::CanPerformCheck();
    }

    return false;
}

float UTeamBreachAndClearActivity::GetDestinationTolerance() const
{
    return 25.0f + ((uint8)OverrideSquadPosition * 3.0f);
}

TSubclassOf<UNavigationQueryFilter> UTeamBreachAndClearActivity::GetNavigationQueryOverride()
{
    if (GetActiveStateID() >= 3) // breach
    {
        return UNavQuery_SwatBreachAndClear::StaticClass();
    }
    
    return nullptr;
}

bool UTeamBreachAndClearActivity::GetOverrideMovementSpeed(float& OutMovementSpeed) const
{
    if (bIsCollapsing)
    {
        OutMovementSpeed = 200.0f;
        return true;
    }
    
    if (GetActiveStateID() >= 5) // Clear
    {
        if (LocalClearingTime < 3.0f)
        {
            OutMovementSpeed = 240.0f; // go full speed boi, dont clog up traffic
            return true;
        }
        
        if (NearestThreat)
        {
            OutMovementSpeed = 150.0f;
            return true;
        }

        if (bHasEverPassedThreshold)
        {
            OutMovementSpeed = 220.0f;
            
            if (ClearingLeader)
            {
                OutMovementSpeed = ClearingLeader->GetMovementComponent()->GetMaxSpeed() * 0.9f;
                
                if (ClearingLeader && ClearingLeader->GetCyberneticsController())
                {
                    if (const USearchLandmarkActivity* Activity = ClearingLeader->GetCyberneticsController()->GetCurrentActivity<USearchLandmarkActivity>())
                    {
                        if (FVector::Distance(GetCharacter()->GetNavAgentLocation(), Activity->CoverLandmark->GetActorLocation()) < 300.0f)
                        {
                            OutMovementSpeed *= 0.35f;
                        }
                    }
                }
            }

            if (CurrentClearStage >= 2)
            {
                if (FMath::Abs(GetCharacter()->QuickLeanAmount) > 0.0f)
                    OutMovementSpeed /= 1.75f;
            }

            OutMovementSpeed = FMath::Clamp(OutMovementSpeed, 80.0f, 240.0f);
            
            return true;
        }

        OutMovementSpeed = 240.0f;
        return true;
    }
    
    if (bIsSwapping && GetActiveStateID() == 3) // breach
    {
        OutMovementSpeed = 200.0f;
        return true;
    }

    return Super::GetOverrideMovementSpeed(OutMovementSpeed);
}

void UTeamBreachAndClearActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
    Super::ActivityOverriden(OverridingActivity);

    Location = FVector::ZeroVector;
}

bool UTeamBreachAndClearActivity::CanTick() const
{
    // if enemy is in threshold stop ticking and deal with the threat first
    if (GetActiveStateID() >= 3) // Scan, Clear
    {
        if (bHasBreacherBreached || StackUpDoor->IsOpenBeyondIncrementThreshold())
        {
            if (const FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
            {
                // get all characters within room
                TArray<ACyberneticCharacter*, TFixedAllocator<64>> AIInRoom;
                for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
                {
                    if (AI && AI->IsActive() && !AI->IsOnSWATTeam())
                    {
                        if (const AThreatAwarenessActor* TAA = AI->GetCyberneticsController()->GetTargetingComp()->GetNearestThreat())
                        {
                            if (TAA->OwningRoom == Room->Name)
                            {
                                AIInRoom.AddUnique(AI);
                            }
                        }
                    }
                }
                
                for (const ACyberneticCharacter* AI : AIInRoom)
                {
                    FTransform Transform;
                    Transform.SetLocation(StackUpDoor->GetDoorMidLocation());
                    Transform.SetRotation(StackUpDoor->GetActorRotation().Quaternion());

                    const float DoorWidth = StackUpDoor->GetDoorSize().Y + 15.0f;

                    const FVector Extent = FVector(300.0f, DoorWidth, 120.0f);

                    //DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Yellow, false, 0.1f);
                    
                    const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(AI->GetActorLocation(), Transform, Extent);

                    if (bInsideDoorThreshold)
                    {
                        return false;
                    }
                }
            }
		}
    }
    
    return Super::CanTick();
}

void UTeamBreachAndClearActivity::ResumeActivity()
{
    Location = FVector::ZeroVector;
    
    if (GetActiveStateID() <= 3)
    {
        Super::ResumeActivity();
    }
}

bool UTeamBreachAndClearActivity::CanBePushed() const
{
    if (GetActiveStateID() >= 4 && LocalClearingTime < 3.0f)
        return false;
    
    if (bHasEverPassedThreshold)
        return true;

    if (IsDoorBreacher() || IsDoorUser())
        return GetActiveStateID() < 3;
    
    return GetActiveStateID() <= 3;
}

void UTeamBreachAndClearActivity::ResetData()
{
    Super::ResetData();

    TimeSinceLastClearingTick = 0.0f;
	CornerFocalPoint = FVector::ZeroVector;
	ChosenClearPoints = nullptr;
    LocalClearingTime = 0.0f;
	CurrentClearStage = 0;
    StageLimit = 0;
    bHasLeaderEverPassedThreshold = false;
	bHasEverPassedThreshold = false;
    bDroppedChemlightHalfway = false;
    bHasLOSToClearPoint = false;
    bHasLetGoOfStackUp = false;
    bHasEverEquippedShieldWhileClearing = false;
    bHasTrackedAnyoneWhileClearing = false;
    bNextStageIsOccupied = false;
    bCalledOutAreaClear = false;
	CurrentClearPoint = nullptr;
    NearestThreat = nullptr;
    ClearingLeader = nullptr;
    AIBlockingClearingPath = nullptr;
    OnCleared.Clear();
}

bool UTeamBreachAndClearActivity::IsBreaching() const
{
    return GetActiveStateID() > 2 && GetActiveStateID() <= 3;
}

bool UTeamBreachAndClearActivity::GiveDoorBreachActivity(UDoorBreachActivity* InDoorBreachActivity, ABaseItem* InBreachItem)
{
    OccupiedStackUpActor->DisableNavBlocker();
    
    InDoorBreachActivity->Door = StackUpDoor;
    InDoorBreachActivity->CommandLocation = SharedData->CommandLocation;
    InDoorBreachActivity->OriginalLocation = OccupiedStackUpActor->GetActorLocation();
    InDoorBreachActivity->BreachItem = InBreachItem;
    InDoorBreachActivity->bReturnToPositionAfterInteraction = false;
    
    if (!InDoorBreachActivity->OnBreachFinished.IsAlreadyBound(this, &UTeamBreachAndClearActivity::OnDoorBreachFinished))
        InDoorBreachActivity->OnBreachFinished.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorBreachFinished);
    
    if (!InDoorBreachActivity->OnFinishActivity.IsAlreadyBound(this, &UTeamBreachAndClearActivity::OnDoorBreachActivityFinished))
        InDoorBreachActivity->OnFinishActivity.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorBreachActivityFinished);

    return UActivityManager::GiveActivityTo(InDoorBreachActivity, GetCharacter(), true, false);
}

bool UTeamBreachAndClearActivity::CanUseDoor() const
{
    if (DoorUseActivity == OwningController->GetCurrentActivity())
        return false;
    
    if (DoorBreachType == EDoorBreachType::Move)
        return false;
    
	// If someone is already using this door, wait for them to finish
	if (UActivityManager::AnyAIHasActivity<UDoorInteractionActivity>([&](const UDoorInteractionActivity* Activity)
	{
	    if (Activity->GetCharacter() != DoorChecker &&
	        Activity->GetCharacter() != DoorBreacher &&
	        Activity->GetCharacter() != DoorUser)
	    {
            return Activity->Door == StackUpDoor;
	    }
	    
	    return false;
	}))
	{
		return false;
	}
    
    // Can't use door when breaching with leader
    if (bIsLeaderBreach)
        return false;
    
    // Can't use door when door user has already breached
    if (bHasUserBreached)
        return false;
    
    // Can't use door if there isn't one
    if (StackUpDoor->IsDoorwayOnly())
        return false;
    
    // Can't use door if already using it
    if (DoorUseActivity == OwningController->GetCurrentActivity())
        return false;
        
    // Can use door when not breaching with an item (i.e grenade, flashbang)
    if (!DoorBreacher)
        return true;

    if (GetSharedData<FSharedBreachData>()->NumInTeam == 1)
        return true;

    if (DoorBreacher == DoorUser)
        return true;

    const bool bIsBreacherBreaching = DoorBreachActivity && bIsBreaching;
    const bool bIsBreacherReady = DoorBreachActivity && bBreacherReady;
    
    // Can use door when we haven't breached yet and the breacher isn't performing a breach
    if (!bHasBreacherBreached && !bIsBreacherBreaching && bIsBreacherReady)
        return true;

    /*
    if (!bIsLeaderThrow)
    {
        if (!bIsBreacherReady && DoorBreacher != DoorUser)
            return false;
    }
    */

    return false;
}

void UTeamBreachAndClearActivity::PerformStackedStage(float DeltaTime, float Uptime)
{
    Super::PerformStackedStage(DeltaTime, Uptime);
    
    if (DoorBreachItemClass && !DoorBreacher ||
        DoorUseItemClass && !DoorUser)
    {
        SetupDoorUsers();
    }

    if (IsDoorUser())
    {
        if (GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, ChosenStackUpArea) != DoorUser)
        {
            SwapSquadPositionWith(ESquadPosition::SP_Alpha);
        }
    }
        
    if (!(IsDoorUser() && IsDoorBreacher()))
    {
        // grenade launcher case
        if (const ACyberneticCharacter* BetaMale = GetCharacterAtSquadPosition(ESquadPosition::SP_Beta))
        {
            const bool bIsGrenadeLauncher = DoorBreachItemClass && DoorBreachItemClass->GetDefaultObject()->IsA(AGrenadeLauncher::StaticClass());
            if (IsDoorBreacher() && bIsGrenadeLauncher)
            {
                if (BetaMale != DoorBreacher)
                {
                    SwapSquadPositionWith(ESquadPosition::SP_Beta);
                }
            }
        }
    }
    
    if (HasTeamReachedPosition() && !IsAnyoneSwapping())
    {
        const bool bCanDisarmTrap = StackUpDoor->IsTrapLive() && StackUpDoor->IsLocationSameSideAsTrap(SharedData->CommandLocation);
        if (bCanDisarmTrap)
        {
            StackUpDoor->SetDoorTrapKnowledge(GetCharacter()->IsSuspect(), true);

            if (USWATManager* SWATManager = USWATManager::Get(this))
            {
                SWATManager->GiveDisarmTrapOnDoorCommand(StackUpDoor, SharedData->CommandTeam, SharedData->CommandLocation);
            }
        }
    }
}

void UTeamBreachAndClearActivity::OnLeaderItemPrimaryUse(AReadyOrNotCharacter* ItemOwner, ABaseItem* Item)
{
    if (ABaseGrenade* Grenade = Cast<ABaseGrenade>(Item))
    {
        Grenade->OnGrenadeDetonated.RemoveAll(this);
        Grenade->OnGrenadeDetonated.AddDynamic(this, &UTeamBreachAndClearActivity::OnLeaderGrenadeDetonated);

        return;
    }
    
    if (const AGrenadeLauncher* GrenadeLauncher = Cast<AGrenadeLauncher>(Item))
    {
        if (AGrenadeProjectile* GrenadeProjectile = Cast<AGrenadeProjectile>(GrenadeLauncher->LastSpawnedProjectile))
        {
            GrenadeProjectile->OnGrenadeDetonated.RemoveAll(this);
            GrenadeProjectile->OnGrenadeDetonated.AddDynamic(this, &UTeamBreachAndClearActivity::OnLeaderGrenadeProjectileDetonated);
        }

        return;
    }
}

void UTeamBreachAndClearActivity::OnLeaderGrenadeDetonated(ABaseGrenade* InGrenade)
{
    //DrawDebugPoint(GetWorld(), CommandLocation + CommandNormal * 50.0f, 20.0f, FColor::Green, false, 1.0f);
    //const FVector& GrenadeLocation = InGrenade->GetItemLocation();
	//const bool bGrenadeInFront = FVector::DotProduct(StackUpDoor->GetDoorway()->GetForwardVector().GetSafeNormal2D(), ((CommandLocation + CommandNormal * 50.0f) - GrenadeLocation).GetSafeNormal2D()) < 0.0f;
    
    const bool bCommandInFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
    const bool bGrenadeInFront = StackUpDoor->IsPointInFrontOfDoorway(InGrenade->GetItemLocation());
    
    if (bCommandInFront != bGrenadeInFront)
    {
        bHasLeaderBreached = true;
        bLeaderUsedItem = true;
    }
}

void UTeamBreachAndClearActivity::OnLeaderGrenadeProjectileDetonated(AGrenadeProjectile* InGrenadeProjectile)
{
    const bool bCommandInFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
    const bool bGrenadeInFront = StackUpDoor->IsPointInFrontOfDoorway(InGrenadeProjectile->GetBulletMesh()->GetComponentLocation());
    
    if (bCommandInFront != bGrenadeInFront)
    {
        bHasLeaderBreached = true;
        bLeaderUsedItem = true;
    }
}

void UTeamBreachAndClearActivity::OnDoorBreacherReady()
{
    bBreacherReady = true;
}

void UTeamBreachAndClearActivity::OnDoorBreacherBreaching()
{
    bIsBreaching = true;
}

void UTeamBreachAndClearActivity::OnDoorBreachActivityFinished(UBaseActivity* InActivity, ACyberneticController* InController)
{
    InActivity->OnFinishActivity.RemoveAll(this);
    
    OnDoorBreachFinished(InActivity, InController);
    
    if (InActivity == DoorBreachActivity)
        DoorBreachActivity = nullptr;
    
    if (InActivity == DoorUseActivity)
        DoorUseActivity = nullptr;
}

void UTeamBreachAndClearActivity::OnDoorBreachFinished(UBaseActivity* InActivity, ACyberneticController* InController)
{
    StackUpDoor->DeactivateDoorBlocker();
    
    if (InActivity == DoorBreachActivity)
    {
        DoorBreachActivity->OnBreachFinished.RemoveAll(this);
        bHasBreacherBreached = true;
    }
    
    if (InActivity == DoorUseActivity)
    {
        DoorUseActivity->OnBreachFinished.RemoveAll(this);
        bHasUserBreached = true;
    }
}

void UTeamBreachAndClearActivity::SortSwatForClearing()
{
    if (GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Num() == 0)
    {
        const TArray<ASWATCharacter*>& StackUpSortedSwat = GetSharedData<FSharedBreachData>()->StackUpSortedSwat;
        if (StackUpSortedSwat.Num() == 0)
        {
            #if !UE_BUILD_SHIPPING
            ULog::Error("Swat were not sorted for stack up, major error!");
            #endif
            
            return;
        }
        
        const bool bIsCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
        
        TArray<ASWATCharacter*> LeftSwat, RightSwat;
        for (ASWATCharacter* Swat : StackUpSortedSwat)
        {
		    if (!Swat->GetCyberneticsController<ASWATController>())
		        continue;
            
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
			if (const UTeamStackUpActivity* Activity = Lhs.GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
			{
				if (const UTeamStackUpActivity* OtherActivity = Rhs.GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
				{
					return Activity->OverrideSquadPosition < OtherActivity->OverrideSquadPosition;
				}
			}

			return false;
		});

		RightSwat.Sort([&](const ASWATCharacter& Lhs, const ASWATCharacter& Rhs)
		{
			if (const UTeamStackUpActivity* Activity = Lhs.GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
			{
				if (const UTeamStackUpActivity* OtherActivity = Rhs.GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
				{
					return Activity->OverrideSquadPosition < OtherActivity->OverrideSquadPosition;
				}
			}

			return false;
		});
		
        TArray<ASWATCharacter*> FinalSwat;

        if (ASWATCharacter* Scanner = Cast<ASWATCharacter>(DoorScanner))
        {
            FinalSwat.Add(Scanner);
        }

        if (ASWATCharacter* ShieldGuy = Cast<ASWATCharacter>(ShieldUser))
        {
            FinalSwat.AddUnique(ShieldGuy);
        }
        
        const uint8 TotalNum = LeftSwat.Num() + RightSwat.Num();

        if (LeftSwat.Contains(DoorScanner) || LeftSwat.Contains(ShieldUser))
        {
            LeftSwat.Remove(Cast<ASWATCharacter>(DoorScanner));
            LeftSwat.Remove(Cast<ASWATCharacter>(ShieldUser));
            
            for (uint8 i = 0; i < TotalNum; i++)
            {
                if (RightSwat.IsValidIndex(i))
                {
                    FinalSwat.Add(RightSwat[i]);
                }
                
                if (LeftSwat.IsValidIndex(i))
                {
                    FinalSwat.Add(LeftSwat[i]);
                }
            }
            
            if (Cast<ASWATCharacter>(DoorScanner))
                LeftSwat.Insert(Cast<ASWATCharacter>(DoorScanner), 0);
            
            if (Cast<ASWATCharacter>(ShieldUser))
                LeftSwat.Insert(Cast<ASWATCharacter>(ShieldUser), 0);
        }
        else
        {
            RightSwat.Remove(Cast<ASWATCharacter>(DoorScanner));
            RightSwat.Remove(Cast<ASWATCharacter>(ShieldUser));
            
            for (uint8 i = 0; i < TotalNum; i++)
            {
                if (LeftSwat.IsValidIndex(i))
                {
                    FinalSwat.Add(LeftSwat[i]);
                }
                
                if (RightSwat.IsValidIndex(i))
                {
                    FinalSwat.Add(RightSwat[i]);
                }
            }

            if (Cast<ASWATCharacter>(DoorScanner))
                RightSwat.Insert(Cast<ASWATCharacter>(DoorScanner), 0);
            
            if (Cast<ASWATCharacter>(ShieldUser))
                RightSwat.Insert(Cast<ASWATCharacter>(ShieldUser), 0);
        }

        ASWATCharacter* FirstMan = nullptr;
        
        if (DoorScanner)
            FirstMan = Cast<ASWATCharacter>(DoorScanner);
        
        if (ShieldUser && !DoorScanner)
            FirstMan = Cast<ASWATCharacter>(ShieldUser);
        
        if (FirstMan && FinalSwat.Num() > 1)
        {
            const int32 Index = FinalSwat.Find(FirstMan);

            if (Index != INDEX_NONE)
            {
                if (FinalSwat[0] != FirstMan)
                {
                    Swap(FinalSwat[0], FinalSwat[Index]);
                }
            }
        }

        GetSharedData<FSharedBreachData>()->ClearingSortedSwat = FinalSwat;

        //ULog::Array_Object((TArray<UObject*>)GetSharedData<FSharedBreachData>()->ClearingSortedSwat, "Clearing Sorted Swat: ");

        const bool bAnySideEmpty = LeftSwat.Num() == 0 || RightSwat.Num() == 0;
        const bool bRightFullLeftEmpty = RightSwat.Num() > 0 && LeftSwat.Num() == 0;
        const bool bLeftFullRightEmpty = LeftSwat.Num() > 0 && RightSwat.Num() == 0;
        const bool bWideDoorway = StackUpDoor->IsDoorwayOnly() && StackUpDoor->GetDoorSize().Y > 100.0f;
        const bool bCanButtonHook = (bAnySideEmpty || bWideDoorway) && (bRightFullLeftEmpty || bLeftFullRightEmpty || StackUpDoor->IsDoorwayOnly());
        if (!bCanButtonHook)
            GetSharedData<FSharedBreachData>()->FirstEntryMethod = EEntryMethod::Flow;

        EEntryMethod TempEntryMethod = GetSharedData<FSharedBreachData>()->FirstEntryMethod;
        
        enum class EDirection : uint8
        {
            None,
            Left,
            Right
        };
        
        EDirection LastDirection = EDirection::None;

        if (ASWATCharacter* Scanner = FinalSwat[0])
        {
            if (LeftSwat.Contains(Scanner))
            {
                LastDirection = bIsCommandFront ? EDirection::Right : EDirection::Left;
            }
            else if (RightSwat.Contains(Scanner))
            {
                LastDirection = bIsCommandFront ? EDirection::Left : EDirection::Right;
            }
        }

        TArray<ASWATCharacter*> LeftPath, RightPath;

        for (ASWATCharacter* Swat : FinalSwat)
        {
            if (!Swat->GetCyberneticsController())
                continue;
            
            if (UTeamBreachAndClearActivity* BreachAndClearActivity = Swat->GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>())
            {
                BreachAndClearActivity->ClearingLeader = nullptr;
                
                if (RightSwat.Contains(Swat))
                {
                    if (TempEntryMethod == EEntryMethod::ButtonHook)
                    {
                        BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontRightClearPoints;
                        if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontLeftClearPoints;
                        LastDirection = bIsCommandFront ? EDirection::Left : EDirection::Right;
                        LeftPath.AddUnique(Swat);
                    }
                    else
                    {
                        if (LastDirection == EDirection::Left || LastDirection == EDirection::None)
                        {
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontRightClearPoints;
                            if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                                BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontLeftClearPoints;
                            LastDirection = EDirection::Right;
                            RightPath.AddUnique(Swat);
                        }
                        else
                        {
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontLeftClearPoints;
                            if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                                BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontRightClearPoints;
                            LastDirection = EDirection::Left;
                            LeftPath.AddUnique(Swat);
                        }
                    }
                }
                else
                {
                    if (TempEntryMethod == EEntryMethod::ButtonHook)
                    {
                        BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontLeftClearPoints;
                        if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontLeftClearPoints;
                        LastDirection = bIsCommandFront ? EDirection::Right : EDirection::Left;
                        RightPath.AddUnique(Swat);
                    }
                    else
                    {
                        if (LastDirection == EDirection::Right || LastDirection == EDirection::None)
                        {
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontLeftClearPoints;
                            if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                                BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontRightClearPoints;
                            LastDirection = EDirection::Left;
                            LeftPath.AddUnique(Swat);
                        }
                        else
                        {
                            BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackRightClearPoints : &StackUpDoor->FrontRightClearPoints;
                            if (BreachAndClearActivity->ChosenClearPoints->Num() <= 1)
                                BreachAndClearActivity->ChosenClearPoints = bIsCommandFront ? &StackUpDoor->BackLeftClearPoints : &StackUpDoor->FrontLeftClearPoints;
                            LastDirection = EDirection::Right;
                            RightPath.AddUnique(Swat);
                        }
                    }
                }
            }

            //const bool bSplit = LeftSwat.Num() > 0 && RightSwat.Num() > 0;
            if (TempEntryMethod == EEntryMethod::ButtonHook)
            {
                // Flip the last direction
                //LastDirection = LastDirection == EDirection::Left ? EDirection::Right : EDirection::Left;
                TempEntryMethod = EEntryMethod::Flow;
            }
        }
    }
}

bool UTeamBreachAndClearActivity::CanSwapSquadPositions() const
{
    if (DoorBreacher == GetCharacter())
        return false;

    if (DoorUser == GetCharacter())
        return false;

    if (GetActiveStateID() == 1 && DoorChecker == GetCharacter())
        return false;

    return HasReachedLocation(GetDestinationTolerance()) && GetActiveStateID() > 0 && GetActiveStateID() <= 2;
}

ACyberneticCharacter* UTeamBreachAndClearActivity::FindBreacher(ACyberneticCharacter* InDoorUser) const
{
    if (!DoorBreachItemClass)
        return nullptr;
    
	const bool bIsCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
    
    if (DoorBreachItemClass->GetDefaultObject()->IsA(AGrenadeLauncher::StaticClass()))
    {
        return GetCharacterWithItem(AGrenadeLauncher::StaticClass());
    }

    // try to find charlie first, if failed, find the last guy
    bool bHasDelta = GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Delta, bIsCommandFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft) != nullptr;
    ACyberneticCharacter* NewBreacher = GetCharacterAtSquadPositionInStackUpArea(bHasDelta ? ESquadPosition::SP_Charlie : ESquadPosition::SP_Beta, bIsCommandFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);
    if (NewBreacher == InDoorUser && SharedData->NumInTeam > 1)
        NewBreacher = nullptr;
    
    if (!NewBreacher)
    {
        bHasDelta = GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Delta, bIsCommandFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight) != nullptr;
        NewBreacher = GetCharacterAtSquadPositionInStackUpArea(bHasDelta ? ESquadPosition::SP_Charlie : ESquadPosition::SP_Beta, bIsCommandFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight);
        if (NewBreacher == InDoorUser && SharedData->NumInTeam > 1)
            NewBreacher = nullptr;
    }
        
    if (!NewBreacher)
    {
        NewBreacher = GetCharacterAtHighestSquadPositionInStackUpArea(bIsCommandFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);
        if (NewBreacher == InDoorUser && SharedData->NumInTeam > 1)
            NewBreacher = nullptr;
    }
    
    if (!NewBreacher)
    {
        NewBreacher = GetCharacterAtHighestSquadPositionInStackUpArea(bIsCommandFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight);
        if (NewBreacher == InDoorUser && SharedData->NumInTeam > 1)
            NewBreacher = nullptr;
    }
    
    // Give available breach item to the new breacher
    if (NewBreacher)
    {
        if (const ACyberneticCharacter* OldUser = GetCharacterWithItem(DoorBreachItemClass))
        {
            #if !UE_BUILD_SHIPPING
            if (GIsAutomationTesting && CHECK_DEBUG_SUBSYSTEM)
            {
                DEBUG_SUBSYSTEM->bInfiniteSWATItems = false;
            }
            #endif
            ABaseItem* BaseItem = OldUser->GetInventoryComponent()->GetInventoryItemOfClass(DoorBreachItemClass, false);
            OldUser->GetInventoryComponent()->RemoveInventoryItem(BaseItem);
            NewBreacher->GetInventoryComponent()->AddInventoryItem(BaseItem);
            #if !UE_BUILD_SHIPPING
            if (GIsAutomationTesting && CHECK_DEBUG_SUBSYSTEM)
            {
                DEBUG_SUBSYSTEM->bInfiniteSWATItems = true;
            }
            #endif
        }
        
        return NewBreacher;
    }
    
    return GetCharacterWithItem(DoorBreachItemClass);
}

ACyberneticCharacter* UTeamBreachAndClearActivity::FindUser() const
{
    if (bIsLeaderBreach)
        return nullptr;
    
	const bool bIsCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
    
	TArray<UTeamStackUpActivity*> TotalSwatInStackUpArea_Right = GetTotalSwatInStackUpArea(bIsCommandFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackRight);
	TArray<UTeamStackUpActivity*> TotalSwatInStackUpArea_Left = GetTotalSwatInStackUpArea(bIsCommandFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackLeft);
    
    if (!DoorUseItemClass)
    {
        if (TotalSwatInStackUpArea_Right.Num() > 0 && TotalSwatInStackUpArea_Left.Num() > 0)
        {
            if (TotalSwatInStackUpArea_Right[0]->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ABallisticsShield::StaticClass()))
                return TotalSwatInStackUpArea_Left[0]->GetCharacter();
            
            if (TotalSwatInStackUpArea_Left[0]->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ABallisticsShield::StaticClass()))
                return TotalSwatInStackUpArea_Right[0]->GetCharacter();
            
            const float A = (StackUpDoor->GetActorLocation() - TotalSwatInStackUpArea_Right[0]->GetCharacter()->GetNavAgentLocation()).SizeSquared2D();
            const float B = (StackUpDoor->GetActorLocation() - TotalSwatInStackUpArea_Left[0]->GetCharacter()->GetNavAgentLocation()).SizeSquared2D();
            if (A < B)
                return TotalSwatInStackUpArea_Right[0]->GetCharacter();

            return TotalSwatInStackUpArea_Left[0]->GetCharacter();
        }
        
        if (TotalSwatInStackUpArea_Right.Num() > 0)
        {
            return TotalSwatInStackUpArea_Right[0]->GetCharacter();
        }
        
        if (TotalSwatInStackUpArea_Left.Num() > 0)
        {
            return TotalSwatInStackUpArea_Left[0]->GetCharacter();
        }
        
        return nullptr;
    }

    /*
    for (const UTeamStackUpActivity* Activity : TotalSwatInStackUpArea_Right)
    {
        ULog::Info(Activity->GetCharacter()->GetName());
    }
    
    for (const UTeamStackUpActivity* Activity : TotalSwatInStackUpArea_Left)
    {
        ULog::Info(Activity->GetCharacter()->GetName());
    }
    */

    ACyberneticCharacter* NewUser = nullptr;
    for (const UTeamStackUpActivity* Activity : TotalSwatInStackUpArea_Right)
    {
        if (Activity->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(DoorUseItemClass, false))
        {
            NewUser = Activity->GetCharacter();
            break;
        }
    }

    if (!NewUser)
    {
        for (const UTeamStackUpActivity* Activity : TotalSwatInStackUpArea_Left)
        {
            if (Activity->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(DoorUseItemClass, false))
            {
                NewUser = Activity->GetCharacter();
                break;
            }
        }
    }

    return NewUser;
}

bool UTeamBreachAndClearActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    const bool bIsShieldUser = ShieldUser && GetCharacter() == ShieldUser;
    if (bHasEverPassedThreshold && OwningController->GetTrackedTarget() && !bIsShieldUser && OverrideSquadPosition != ESquadPosition::SP_Alpha)
    {
        return false;
    }
    
    const bool bIsDestinationPointOnOtherSideOfDoor = bHasEverPassedThreshold || bHasLeaderEverPassedThreshold || StackUpDoor->IsPointsOnOppositeSideOfDoor(OwningController->GetRONPathFollowingComp()->GetPathDestination(), SharedData->CommandLocation);
    
    if (bIsDestinationPointOnOtherSideOfDoor && GetActiveStateID() >= 4 && GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Num() > 0) // Scan, Clear
    {
        if (CornerFocalPoint != FVector::ZeroVector)
        {
            FocalPoint = CornerFocalPoint;
            return true;
        }
        
        if (HasLeaderPassedThreshold() || (OverrideSquadPosition == ESquadPosition::SP_Alpha && bHasEverPassedThreshold))
        {
            if (CurrentClearPoint)
            {
                float DistToNextPoint = 0.0f;
                FVector NextPathPoint = FVector::ZeroVector;
                if (const UReadyOrNotPathFollowingComp* PathFollowingComp = OwningController->GetRONPathFollowingComp())
                {
                    if (PathFollowingComp->GetPath().IsValid())
                    {
                        const TArray<FNavPathPoint>& PathPoints = PathFollowingComp->GetPath()->GetPathPoints();
                        
                        DistToNextPoint = PathFollowingComp->GetPath()->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), PathFollowingComp->GetNextPathIndex());

                        if (PathPoints.IsValidIndex(PathFollowingComp->GetNextPathIndex()))
                        {
                            NextPathPoint = PathPoints[PathFollowingComp->GetNextPathIndex()];
                        }
                        
                        if (FVector::Distance(GetCharacter()->GetNavAgentLocation(), NextPathPoint) < 100.0f)
                        {
                            if (PathPoints.IsValidIndex(PathFollowingComp->GetNextPathIndex()+1))
                            {
                                NextPathPoint = PathPoints[PathFollowingComp->GetNextPathIndex()+1];
                            }
                        }
                    }
                }
                
                const float MinDistance = CurrentClearPoint->Direction == EClearDirection::Forward ? 700.0f : 1250.0f;
                if (CurrentClearPoint->Direction == EClearDirection::Right && CurrentClearStage > 2)
                {
                    if (DistToNextPoint > MinDistance)
                    {
                        FVector OutDir;	
                        float OutLength;
                        GetCharacter()->GetVelocity().ToDirectionAndLength(OutDir, OutLength);

                        FocalPoint = GetCharacter()->GetActorLocation() + OutDir * 500.0f;
                        return true;
                    }
                }

                if (ClearingLeader && ClearingLeader->GetCyberneticsController())
                {
                    if (ClearingLeader->GetCyberneticsController()->GetCurrentActivity<USearchLandmarkActivity>())
                    {
                        if (FVector::Distance(GetCharacter()->GetNavAgentLocation(), ClearingLeader->GetActorLocation()) < 300.0f)
                        {
                            return false;
                        }
                    }
                }
                
                if (!AnyUnclearedLandmarksForClearPoint(CurrentClearPoint))
                {
                    if (CurrentClearPoint->Direction == EClearDirection::Forward && CurrentClearStage > StageLimit - 1 && bHasLOSToClearPoint)
                    {
                        return false;
                    }
                }

                if (DistToNextPoint > MinDistance)
                {
                    FocalPoint = NextPathPoint;
                    FocalPoint.Z = GetCharacter()->GetActorLocation().Z+50.0f;
                    return true;
                }

                if (bHasEverPassedThreshold && CurrentClearStage > 4)
                {
                    if (CurrentClearPoint->Direction == EClearDirection::Right)
                    {
                        const bool bLeaderAlreadyFocusingOnIt = ClearingLeader && FIntVector(ClearingLeader->Rep_FocalPoint) == FIntVector(CurrentClearPoint->Location);
                        if (bLeaderAlreadyFocusingOnIt)
                        {
                            FocalPoint = FVector::ZeroVector;
                            return false;
                        }
                    }
                    
                    if (!ClearingLeader && /*CurrentClearStage > 3 &&*/ bHasLOSToClearPoint && CurrentClearPoint->Direction != EClearDirection::Right)
                    {
                        FocalPoint = FVector::ZeroVector;
                        return false;
                    }
                    
                    if (bNextStageIsOccupied)
                    {
                        FocalPoint = FVector::ZeroVector;
                        return false;
                    }
                }

                if (AIBlockingClearingPath)
                {
                    FocalPoint = FVector::ZeroVector;
                    return false;
                }
                
                FocalPoint = CurrentClearPoint->Location;
                return true;
            }
        }

        if (CurrentClearPoint)
        {
            FocalPoint = CurrentClearPoint->Location;
            return true;
        }
    }
    else
    {
        if (GetActiveStateID() == 5) // clear
        {
            if (!bHasEverPassedThreshold)
            {
                FocalPoint = GetDoorFocalPoint();
                return true;
            }
        }
    }

    if (GetActiveStateID() < 5) // clear
    {
        return Super::OverrideFocalPoint(FocalPoint);
    }

    return false;
}

bool UTeamBreachAndClearActivity::ShouldForceStrafe() const
{
    if (bIsSwapping && GetActiveStateID() < 5) // clear
        return false;
    
    if (OwningController->GetTrackedTarget())
        return true;
    
    if (bHasEverPassedThreshold)
        return true;

    if (GetActiveStateID() < 5)
    {
        const bool bIsDoorBeingUsed = DoorUseActivity || (DoorUser && (DoorUser->IsOpeningDoor(StackUpDoor) || StackUpDoor->IsOpenBy_Angle(20.0f)));
        
        if (bIsDoorBeingUsed && OverrideSquadPosition == ESquadPosition::SP_Alpha)
            return false;
    }
    
    if (GetActiveStateID() >= 5) // clear
    {
        return HasLeaderPassedThreshold();
    }
    
    if (GetActiveStateID() >= 2 && GetActiveStateID() < 5 && HasReachedLocation(100.0f))
    {
        return OverrideSquadPosition == ESquadPosition::SP_Alpha || IsFurthestOccupiedStackUpInArea();
    }
    
    return true;
}

bool UTeamBreachAndClearActivity::ShouldForceNoStrafe() const
{
    if (bIsSwapping && GetActiveStateID() < 5) // clear
        return true;
    
    if (bHasEverPassedThreshold)
        return false;
    
    if (GetActiveStateID() >= 5) // clear
    {
        return !HasLeaderPassedThreshold();
    }
    
    if (OwningController->GetTrackedTarget()) // this is below the other checks so we can stay true to how real life swat behave
        return false;
    
    if (GetActiveStateID() < 5)
    {
        const ACyberneticCharacter* DoorBeingUsedBy = nullptr;
        if (DoorUseActivity)
        {
            DoorBeingUsedBy = DoorUseActivity->GetCharacter();
        }
        else
        {
            if (DoorUser && (DoorUser->IsOpeningDoor(StackUpDoor) || StackUpDoor->IsOpenBy_Angle(20.0f)))
            {
                DoorBeingUsedBy = DoorUser;
            }
        }

        if (DoorBeingUsedBy)
        {
            if (DoorBeingUsedBy != GetCharacter() && OverrideSquadPosition == ESquadPosition::SP_Alpha)
                return true;
        }
    }

    return false;
}

bool UTeamBreachAndClearActivity::GetLeanOverride(float& LeanOverride) const
{
    if (!GetCharacter()->bIsStrafing)
        return false;
    
    if (!bHasEverPassedThreshold)
    {
        return Super::GetLeanOverride(LeanOverride);
    }

    if (GetActiveStateID() >= 5) // Clear
    {
        if (CurrentClearPoint)
        {
            TArray<FNavPathPoint>* PathPoints = nullptr;
            int32 NextPathIndex = -1;
            
            if (const UReadyOrNotPathFollowingComp* PathFollowingComp = OwningController->GetRONPathFollowingComp())
            {
                if (PathFollowingComp->GetPath().IsValid())
                {
                    PathPoints = &PathFollowingComp->GetPath()->GetPathPoints();
                    NextPathIndex = PathFollowingComp->GetNextPathIndex();
                    
                    if (PathPoints->IsValidIndex(PathFollowingComp->GetCurrentPathIndex()))
                    {
                        const float DistToNextPoint = PathFollowingComp->GetPath()->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), NextPathIndex);
                        const float MinDistance = CurrentClearPoint->Direction == EClearDirection::Forward ? 700.0f : 1250.0f;
                        if (DistToNextPoint > MinDistance)
                            return false;
                    }
                }
            }
            
            if (HasReachedLocation(500.0f) && !bHasLOSToClearPoint)
            {
                const bool bLeftPath = ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints || ChosenClearPoints == &StackUpDoor->BackLeftClearPoints;

                if (StackUpDoor->IsActorInFrontOfDoorway(GetCharacter()))
                    LeanOverride = bLeftPath ? -1.0f : 1.0f;
                else
                    LeanOverride = bLeftPath ? 1.0f : -1.0f;

                const FVector ForwardDirection = IsCommandFrontOfDoor() ? -StackUpDoor->GetActorForwardVector() : StackUpDoor->GetActorForwardVector();
                const FVector DirectionToMe = (GetCharacter()->GetActorLocation() - FVector(CurrentClearPoint->Location)).GetSafeNormal2D();
                const bool bIsInFrontOfClearPoint = FVector::DotProduct(ForwardDirection, DirectionToMe) > 0.0f && CurrentClearStage != StageLimit;
                if (!bIsInFrontOfClearPoint)
                {
                    LeanOverride = -LeanOverride;
                    
                    FHitResult Hit;
                    GetWorld()->LineTraceSingleByChannel(Hit, CurrentClearPoint->Location, GetCharacter()->GetActorLocation(), ECC_Visibility, GetCharacter()->GetCollisionQueryParameters());
                    //DrawDebugLine(GetWorld(), CurrentClearPoint->Location, GetCharacter()->GetActorLocation(), FColor::Red, false, 0.1f);

                    if (Hit.bBlockingHit)
                    {
                        const FVector RightDirection = bLeftPath ? -StackUpDoor->GetActorRightVector() : StackUpDoor->GetActorRightVector();
                        
                        const float Dot = FVector::DotProduct(Hit.Normal, RightDirection);
                        if (FMath::Abs(Dot) > 0.9f)
                            LeanOverride = Dot;
                            
                        //const float DotHit = FVector::DotProduct(Hit.Normal, RightDirection);
                        //LOG_NUMBER(Dot);
                    }
                }
                else
                {
                    if (PathPoints->IsValidIndex(NextPathIndex))
                    {
                        const FVector End = (*PathPoints)[NextPathIndex].Location;
                        const FVector DoorLocation = StackUpDoor->GetDoorMidLocation();
                        const FVector Direction = (End - DoorLocation).GetSafeNormal2D();
                        const FVector RightDirection = bLeftPath ? -StackUpDoor->GetActorRightVector() : StackUpDoor->GetActorRightVector();
                        
                        const float Dot = FVector::DotProduct(Direction, RightDirection);

                        if (StackUpDoor->IsActorInFrontOfDoorway(GetCharacter()))
                        {
                            if (bLeftPath)
                                LeanOverride = Dot < 0.0f ? -1.0f : 1.0f;
                            else
                                LeanOverride = Dot > 0.0f ? -1.0f : 1.0f;
                        }
                        else
                        {
                            if (bLeftPath)
                                LeanOverride = Dot < 0.0f ? 1.0f : -1.0f;
                            else
                                LeanOverride = Dot > 0.0f ? 1.0f : -1.0f;
                        }
                    }
                }
                
                return true;
            }
        }
    }
    
    return false;
}

bool UTeamBreachAndClearActivity::GetLowReadyOverride(bool& bLowReady) const
{
    if (GetActiveStateID() >= 2)
    {
        bLowReady = false;
        return true;
    }
    
    return false;
}

////////// Stack up state events //////////
void UTeamBreachAndClearActivity::EnterStackupStage()
{
	if (bNewStackUpDoor)
	{
		FindStackUpPath();
	}
    else
    {
        if (OccupiedStackUpActor)
        {
            OverrideSquadPosition = OccupiedStackUpActor->GetSquadPosition();
        }

        if (AllStackUpPathsReady())
        {
            if (ShouldCheckDoorBeforeBreach())
            {
                if (!DoorChecker && !StackUpDoor->IsDoorwayOnly())
                {
                    DoorChecker = FindChecker();

                    #if WITH_EDITOR
                    ensureAlways(GetSquadPositionForCharacter(DoorChecker) == ESquadPosition::SP_Alpha);
                    #endif
                }
            }
        }
    }
}

////////// Check state events //////////
void UTeamBreachAndClearActivity::EnterStackedStage()
{
    Super::EnterStackedStage();

    if (AllStacked())
    {
        SetupDoorUsers();
    }
}

void UTeamBreachAndClearActivity::OnSwatSorted(const TArray<ASWATCharacter*>& InSortedSwat, bool bReversed)
{
    if (!bNewStackUpDoor && OccupiedStackUpActor)
    {
        OverrideSquadPosition = OccupiedStackUpActor->GetSquadPosition();
        return;
    }

    Super::OnSwatSorted(InSortedSwat, bReversed);
}

void UTeamBreachAndClearActivity::PerformCheckStage(float DeltaTime, float Uptime)
{
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("PerformCheckStage | No valid stack up door");
        return;
    }

    ProgressState = FText::FromStringTable("SwatCommandTable", "Checking");

    if (bHasCheckedDoor)
    {
        if (DoorCheckResult != EDoorCheckResult::None && DoorCheckResult != EDoorCheckResult::Unlocked)
        {
            if (DoorCheckResult == EDoorCheckResult::Locked && StackUpDoor->TeamKnowsDoorLockState(GetCharacter()->IsSuspect()))
            {
                ProgressState = FText::FromStringTable("SwatCommandTable", "DoorUnlocked");
            }
            else
            {
                ProgressState = DoorCheckResultToText(DoorCheckResult);
            }
        }
        
        return;
    }
    
    Super::PerformCheckStage(DeltaTime, Uptime);
}

////////// Breach stage events //////////
bool UTeamBreachAndClearActivity::CanPerformBreach() const
{
    if (!StackUpDoor)
        return false;
    
	if (!OccupiedStackUpActor)
		return false;
    
    if (GetSharedData<FSharedBreachData>()->StackUpSortedSwat.Num() == 0)
        return false;

    if (bIsCollapsing)
        return false;

    if (DoorBreachType == EDoorBreachType::Open && DoorCheckResult != EDoorCheckResult::Unlocked)
        return false;

    if (StackUpDoor->IsTrapLive() && StackUpDoor->IsLocationSameSideAsTrap(SharedData->CommandLocation))
        return false;

    if (UActivityManager::AnyAIHasActivity<UDoorInteractionActivity>([&](const UDoorInteractionActivity* Activity)
    {
        return Activity->Door == StackUpDoor && Activity->GetActiveStateID() > 0 && !Activity->IsActivityComplete();
    }))
    {
        return false;
    }

    if (DoorUser && DoorUser->GetCyberneticsController())
    {
        if (const UTeamBreachAndClearActivity* Activity = DoorUser->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
        {
            if (Activity->GetActiveStateID() <= 2)
            {
                if (!IsLeaderOccupyingSquadPosition(ESquadPosition::SP_Alpha, Activity->ChosenStackUpArea))
                {
                    if (GetSquadPositionForCharacter(DoorUser) != ESquadPosition::SP_Alpha)
                    {
                        return false;
                    }
                }
            }
        }
    }

    if (!(IsDoorUser() && IsDoorBreacher()))
    {
        if (GetCharacterAtSquadPosition(ESquadPosition::SP_Beta))
        {
            const bool bIsGrenadeLauncher = DoorBreachItemClass && DoorBreachItemClass->GetDefaultObject()->IsA(AGrenadeLauncher::StaticClass());
            
            if (DoorBreacher && DoorBreacher->GetCyberneticsController() && bIsGrenadeLauncher)
            {
                if (const UTeamBreachAndClearActivity* Activity = DoorBreacher->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
                {
                    if (Activity->GetActiveStateID() <= 2)
                    {
                        if (!IsLeaderOccupyingSquadPosition(ESquadPosition::SP_Beta, Activity->ChosenStackUpArea))
                        {
                            if (GetSquadPositionForCharacter(DoorBreacher) != ESquadPosition::SP_Beta)
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (IsAnyoneSwapping())
        return false;

    if (DoorBreachType == EDoorBreachType::Move && GetSharedData<FSharedBreachData>()->NumInTeam > 2 && !DoorBreachItemClass && !ShieldUser)
    {
        bool bAlphaStacked = false, bBetaStacked = false;
        uint8 NumAlphaStack = 0;
        bool bAnyoneFar = false;
        UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
        {
            if (!Activity->HasReachedLocation(300.0f))
            {
                bAnyoneFar = true;
                return false;
            }
            
            if (Activity->OverrideSquadPosition == ESquadPosition::SP_Alpha)
            {
                bAlphaStacked = Activity->GetActiveStateID() >= 2; // stacked
                if (bAlphaStacked)
                    NumAlphaStack++;
            }
            else if (Activity->OverrideSquadPosition == ESquadPosition::SP_Beta)
            {
                bBetaStacked = Activity->GetActiveStateID() >= 2; // stacked
            }

            if (bAlphaStacked && bBetaStacked)
                return false;
            
            return true; 
        });

        if (bAnyoneFar)
            return false;
        
        if (NumAlphaStack >= 2 || (bAlphaStacked && bBetaStacked))
            return true;
    }
    
    if (AllStacked())
    {
        if (ShouldCheckDoorBeforeBreach())
        {
            return bHasCheckedDoor;
        }
        
        return true;
    }
    
    return false;
}

void UTeamBreachAndClearActivity::EnterBreachStage()
{
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("EnterBreachStage | No valid stack up door");
        return;
    }
    
    if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
    {
        BreachingTime = 0.0f;
    }

    SetupDoorUsers();
    
    //bPauseIfTrackingEnemy = true;
}

void UTeamBreachAndClearActivity::PerformBreachStage(const float DeltaTime, const float Uptime)
{
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("PerformBreachStage | No valid stack up door");
        return;
    }
    
    if (GetSharedData<FSharedStackUpData>()->StackUpSortedSwat.Num() == 0)
    {
        ACTIVITY_FAILED("PerformBreachStage | Stack up sorted swat array is empty");
        return;
    }

    ProgressState = FText::FromStringTable("SwatCommandTable", "Breaching");

    if (IsDoorUser())
    {
        bHasUserBreached = StackUpDoor->IsOpenBeyondCloseThreshold();
    }

    //const bool bUserFinished = !DoorUser || (DoorUser && IsDoorUser() && DoorUseActivity == nullptr);
    const bool bBreacherFinished = !IsDoorBreacher() || (IsDoorBreacher() && bHasBreacherBreached && DoorBreachActivity == nullptr);
    const bool bUserSameSide = DoorUser && DoorUser->GetCyberneticsController<ASWATController>() && DoorUser->GetCyberneticsController<ASWATController>()->GetStackUpActivity()->GetStackUpArea() == ChosenStackUpArea;

    // Shield user case handling
    if (ShieldUser)
    {
        // try transfer shield user to the other shield guy if we're the door user and also have a shield
        if (GetCharacter() == ShieldUser && (IsDoorUser() || IsDoorBreacher()) && !bHasUserBreached)
        {
            UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
            {
                if (Activity->SharedData->ActivityId == SharedData->ActivityId && Activity != this)
                {
                    if (Activity->GetCharacter()->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Shield))
                    {
                        ShieldUser = Activity->GetCharacter();
                        return false;
                    }
                }

                return true;
            });
        }

        if (GetCharacter() == ShieldUser)
        {
            if ((bHasUserBreached /*&& bUserFinished*/) ||
                (!IsDoorUser() && !IsDoorBreacher() && bBreacherFinished && !bUserSameSide))
            {
                if (GetSquadPositionForCharacter(ShieldUser) != ESquadPosition::SP_Alpha)
                {
                    if (UTeamStackUpActivity* ShieldGuyActivity = ShieldUser->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
                    {
                        // dont swap if alpha is a door user
                        if (GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, ChosenStackUpArea) != DoorUser || bHasUserBreached)
                            ShieldGuyActivity->SwapSquadPositionWith(ESquadPosition::SP_Alpha);
                    }
                }
                
                if (!ShieldUser->GetInventoryComponent()->IsItemEquipped(EItemCategory::IC_Shield))
                {
                    ShieldUser->GetInventoryComponent()->EquipItemOfClass(ABallisticsShield::StaticClass());
                }
            }
        }

        // try swap with beta if alpha is a door user (for single side stack)
        if (bUserSameSide)
        {
            if (GetCharacter() == ShieldUser && !IsDoorUser() && !IsDoorBreacher() && bBreacherFinished && !bHasUserBreached)
            {
                if (GetSquadPositionForCharacter(ShieldUser) != ESquadPosition::SP_Beta)
                {
                    if (UTeamStackUpActivity* ShieldGuyActivity = ShieldUser->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
                    {
                        ShieldGuyActivity->SwapSquadPositionWith(ESquadPosition::SP_Beta);
                    }
                }
                
                if (!ShieldUser->GetInventoryComponent()->IsItemEquipped(EItemCategory::IC_Shield))
                {
                    ShieldUser->GetInventoryComponent()->EquipItemOfClass(ABallisticsShield::StaticClass());
                }
            }
        }
    }
    
    if (IsDoorUser()) // if we are the user and also the one who has the launcher, switch with beta if possible
    {
        // grenade launcher case
        if (const ACyberneticCharacter* BetaMale = GetCharacterAtSquadPosition(ESquadPosition::SP_Beta))
        {
            const bool bIsGrenadeLauncher = DoorBreachItemClass && DoorBreachItemClass->GetDefaultObject()->IsA(AGrenadeLauncher::StaticClass());
            if (IsDoorBreacher() && bIsGrenadeLauncher)
            {
                if (BetaMale != DoorBreacher)
                {
                    SwapSquadPositionWith(ESquadPosition::SP_Beta);
                }
            }
        }
    }

    if (bHasBreacherBreached)
        return;
    
    if (bIsCollapsing)
        return;
    
    Location = FVector::ZeroVector;

    if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
    {
        BreachingTime = Uptime;
    }

    // failsafe
    {
        if (DoorBreachItemClass && !DoorBreacher ||
            DoorUseItemClass && !DoorUser)
        {
            SetupDoorUsers();
        }

        if (GetSharedData<FSharedBreachData>()->NumInTeam == 1)
        {
            DoorUser = GetCharacter();
        }

        if (IsDoorUser() && !bHasUserBreached)
        {
            if (GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, ChosenStackUpArea) != DoorUser)
            {
                SwapSquadPositionWith(ESquadPosition::SP_Alpha);
            }
        }
    }

    // Try breach door
    {
        if (DoorBreacher || DoorBreachItemClass)
        {
            const bool bIsDoorBreacherAndUser = IsDoorBreacher() && IsDoorUser();
            if (bIsDoorBreacherAndUser)
                bBreacherReady = true;
                
            if ((IsDoorBreacher() && !bIsDoorBreacherAndUser) || (bIsDoorBreacherAndUser && bHasUserBreached))
            {
                if (DoorBreachActivity != OwningController->GetCurrentActivity() && !bHasBreacherBreached)
                {
                    if (ABaseItem* BreachItem = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(DoorBreachItemClass, false))
                    {
                        if (const AGrenadeLauncher* Launcher = Cast<AGrenadeLauncher>(BreachItem))
                        {
                            if (Launcher->HasAmmo())
                            {
                                if (ULaunchGrenadeThroughDoorActivity* LaunchGrenadeThroughDoorActivity = Cast<ASWATController>(OwningController)->GetLaunchGrenadeThroughDoorActivity())
                                {
                                    if (const ABaseItem* ItemCDO = DoorBreachItemClass->GetDefaultObject<ABaseItem>())
                                    {
                                        LaunchGrenadeThroughDoorActivity->ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "DeployItemName"), ItemCDO->ItemName);
                                    }
                                    
                                    LaunchGrenadeThroughDoorActivity->OnLauncherReady.RemoveAll(this);
                                    LaunchGrenadeThroughDoorActivity->OnLauncherReady.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorBreacherReady);
                                    
                                    if (GiveDoorBreachActivity(LaunchGrenadeThroughDoorActivity, BreachItem))
                                    {
                                        DoorBreachActivity = LaunchGrenadeThroughDoorActivity;
                                    }
                                }
                            }
                            else
                            {
                                DoorBreachActivity = nullptr;
                                bBreacherReady = true;
                                bHasBreacherBreached = true;
                            }
                        }
                        else
                        {
                            if (UThrowGrenadeThroughDoorActivity* ThrowGrenadeThroughDoorActivity = Cast<ASWATController>(OwningController)->GetThrowGrenadeThroughDoorActivity())
                            {
                                if (const ABaseItem* ItemCDO = DoorBreachItemClass->GetDefaultObject<ABaseItem>())
                                {
                                    ThrowGrenadeThroughDoorActivity->ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "DeployItemName"), ItemCDO->ItemName);
                                }
                                
                                ThrowGrenadeThroughDoorActivity->ThrowItemClass = DoorBreachItemClass;
                                ThrowGrenadeThroughDoorActivity->bWaitBeforeThrow = DoorBreachType != EDoorBreachType::Move && !bIsDoorBreacherAndUser;
                                
                                ThrowGrenadeThroughDoorActivity->OnThrowReady.RemoveAll(this);
                                ThrowGrenadeThroughDoorActivity->OnThrowReady.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorBreacherReady);
                                
                                ThrowGrenadeThroughDoorActivity->OnThrowingItem.RemoveAll(this);
                                ThrowGrenadeThroughDoorActivity->OnThrowingItem.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorBreacherBreaching);

                                if (GiveDoorBreachActivity(ThrowGrenadeThroughDoorActivity, BreachItem))
                                {
                                    DoorBreachActivity = ThrowGrenadeThroughDoorActivity;
                                }
                            }
                        }
                    }
                }

                const bool bBreacherComplete = DoorBreachActivity == nullptr;
                
                if (StackUpDoor->IsDoorwayOnly())
                {
                    bHasBreacherBreached = bBreacherComplete;
                }
                else
                {
                    bHasBreacherBreached = bBreacherComplete && (StackUpDoor->IsOpenBeyond(0.5f) || bHasUserBreached);
                }
            }
        }
        else
        {
            if (bIsLeaderThrow)
            {
                // A "throw" in the leader's sense means any one of the following..
                //  1. A grenade is thrown and detonated or
                //  2. A grenade launcher grenade is launched through the door or
                //  3. The leader themselves is "thrown" through the door, by moving into the room

                if (const AReadyOrNotCharacter* SquadLeader = GetSquadLeader())
                {
                    const bool bCommandInFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
                    const bool bLeaderInFront = StackUpDoor->IsPointInFrontOfDoorway(SquadLeader->GetActorLocation());
                    
                    if (bCommandInFront != bLeaderInFront)
                        bHasLeaderBreached = true;
                }
                
                bHasBreacherBreached = StackUpDoor->IsOpenBeyond(0.5f) && bHasLeaderBreached;
            }
            else
            {
                if (bIsLeaderBreach)
                    bHasBreacherBreached = StackUpDoor->IsOpenBeyond(0.5f);
                else
                    bHasBreacherBreached = StackUpDoor->IsOpenBeyond(0.5f) || bHasUserBreached;
            }
        }

        if (IsDoorUser())
        {
            // Use the door only when the breacher is ready
            // Note: if door user is also the breacher, just start using the door immediately
            if (CanUseDoor() && HasReachedLocation(GetDestinationTolerance()))
            {
                UDoorBreachActivity* ChosenActivity = nullptr;
                ABaseItem* ChosenBreachItem = nullptr;
                
                switch (DoorBreachType)
                {
                    case EDoorBreachType::Kick:
                    {
                        if (UKickDoorActivity* KickDoorActivity = Cast<ASWATController>(OwningController)->GetKickDoorActivity())
                        {
                            ChosenActivity = KickDoorActivity;
                            ChosenBreachItem = nullptr;
                        }
                    }
                    break;
                    
                    case EDoorBreachType::Shotgun:
                    {
                        ABaseItem* BreachItem = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_BreachingShotgun);
                        if (!BreachItem)
                        {
                            // Try opening the door instead, if there is no breaching shotgun available on the door user
                            DoorBreachType = EDoorBreachType::Open;
                            break;
                        }
                        
                        if (UShotgunDoorActivity* ShotgunDoorActivity = Cast<ASWATController>(OwningController)->GetShotgunDoorActivity())
                        {
                            ChosenActivity = ShotgunDoorActivity;
                            ChosenBreachItem = BreachItem;
                        }
                    }
                    break;
                    
                    case EDoorBreachType::Ram:
                    {
                        ABaseItem* BreachItem = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_BatteringRam);
                        if (!BreachItem)
                        {
                            // Try opening the door instead, if there is no battering ram available on the door user
                            DoorBreachType = EDoorBreachType::Open;
                            break;
                        }

                        if (URamDoorActivity* RamDoorActivity = Cast<ASWATController>(OwningController)->GetRamDoorActivity())
                        {
                            ChosenActivity = RamDoorActivity;
                            ChosenBreachItem = BreachItem;
                        }
                    }
                    break;
                    
                    case EDoorBreachType::C2:
                    {
                        // Try opening the door instead, if there are no C2 explosives available on the door user
                        AC2Explosive* C2Explosive = Cast<AC2Explosive>(GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(AC2Explosive::StaticClass(), false));
                        if (!C2Explosive)
                        {
                            DoorBreachType = EDoorBreachType::Open;
                            break;
                        }
                        
                        if (UC2DoorActivity* C2DoorActivity = Cast<ASWATController>(OwningController)->GetC2DoorActivity())
                        {
                            ChosenActivity = C2DoorActivity;
                            ChosenBreachItem = C2Explosive;
                        }
                    }
                    break;
                    
                    case EDoorBreachType::Custom:   break;
                    default:                        break;
                }

                if (DoorBreachType == EDoorBreachType::Open)
                {
                    GetCharacter()->ToggleDoor(StackUpDoor, true);
                    return;
                }

                if (ChosenActivity)
                {
                    if (GiveDoorBreachActivity(ChosenActivity, ChosenBreachItem))
                    {
                        DoorUseActivity = ChosenActivity;
                    }
                }
            }
        }
    }
}

////////// Clear stage events //////////
bool UTeamBreachAndClearActivity::CanPerformClear() const
{
    if (!StackUpDoor)
        return false;

    if (GetSharedData<FSharedBreachData>()->StackUpSortedSwat.Num() == 0)
        return false;

    // is one sided stack?
    if (!AnySwatOnOtherSide() || ShieldUser ||
        (GetSharedData<FSharedBreachData>()->Assessment == EThresholdAssessment::None && DoorBreachType != EDoorBreachType::Move))
    {
        if (DoorUseActivity || DoorBreachActivity)
            return false;
    }
    
    if (ShieldUser)
    {
        const bool bDoesLeaderOccupyAlpha = IsLeaderOccupyingSquadPosition(ESquadPosition::SP_Alpha, ChosenStackUpArea);
        
        if (!bDoesLeaderOccupyAlpha && GetSquadPositionForCharacter(ShieldUser) != ESquadPosition::SP_Alpha)
        {
            return false;
        }
        
        if (const UTeamStackUpActivity* ShieldUserActivity = ShieldUser->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
        {
            if (ShieldUserActivity->bIsSwapping)
            {
                return false;
            }
        }
    }

    if (IsAnyoneSwapping())
        return false;

    if (DoorUser)
    {
        // If the door user is somehow killed, start clearing immediately (if the door has been opened)
        if (DoorUser->IsDeadOrUnconscious() || DoorUser->IsIncapacitated())
        {
            if (StackUpDoor->IsHalfwayOpen())
            {
                return true;
            }

            return false;
        }
    }

    if (DoorBreacher)
    {
        // If the door breacher is somehow killed, start clearing immediately (if the door has been opened)
        if (DoorBreacher->IsDeadOrUnconscious() || DoorBreacher->IsIncapacitated())
        {
            if (StackUpDoor->IsHalfwayOpen())
            {
                return true;
            }

            return false;
        }
    }

    if (DoorScanner && !DoorScanner->IsDeadOrUnconscious() && !DoorScanner->IsIncapacitated())
    {
        if (DoorScanner->bHasEverShot && DoorScanner->TimeSinceLastShot < 1.0f)
        {
            return false;
        }
    }

    return bHasBreacherBreached && GetSharedData<FSharedBreachData>()->Assessment == EThresholdAssessment::None;
}

bool UTeamBreachAndClearActivity::ShouldScan() const
{
    if (GetSharedData<FSharedBreachData>()->StackUpSortedSwat.Num() == 0)
        return false;

    return bHasBreacherBreached && GetSharedData<FSharedBreachData>()->Assessment != EThresholdAssessment::None;
}

bool UTeamBreachAndClearActivity::IsCheckFinished() const
{
    if (ShouldCheckDoorBeforeBreach())
    {
        return false;
    }
    
    return Super::IsCheckFinished();
}

void UTeamBreachAndClearActivity::EnterClearStage()
{
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("EnterClearStage | No valid stack up door");
        return;
    }

    Location = FVector::ZeroVector;
    
	GetCharacter()->MoveIgnoreActorAdd(StackUpDoor);

    //GetCharacter()->SetCanAffectNavigationGeneration(false, true);
    
    OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(IsCommandFrontOfDoor() ? StackUpDoor->GetActorForwardVector() : -StackUpDoor->GetActorForwardVector());

    // we're already stacked up and breached, so don't abort at this point,
    // because the door may have just been opened and the swat cannot
    // reach the location immediately when this is first called
    bAbortActivityIfCannotReachLocation = false;
    
    SortSwatForClearing();

    #if WITH_EDITOR
    ensureAlways(ChosenClearPoints != nullptr);
    #endif

    if (!ChosenClearPoints)
    {
        return;
    }

    if (ChosenClearPoints->Num() <= 1)
    {
        bHasEverPassedThreshold = true;
        
        #if !UE_BUILD_SHIPPING
        // For automation testing, don't finish the activity this frame,
        // just wait until next frame until the test recognizes it as a "success"
        if (!GIsAutomationTesting)
        #endif
        {
            OwningController->FinishActivity(this, true, true);
        }
        
        return;
    }
    
    CurrentClearStage = 1;
    StageLimit = ChosenClearPoints->Num()-1;

    for (FClearPoint& Point : *ChosenClearPoints)
    {
        Point.bCleared = false;
    }

    TryActivateDoorBlocker(GetSharedData<FSharedBreachData>()->Room);
    
    // this is to prevent swat from taking the long route
    TryActivateDoorBlocker(GetSharedData<FSharedBreachData>()->CurrentRoom);
    
    StackUpDoor->DeactivateDoorBlocker();

    if (HasTeamReachedPosition(200.0f))
    {
        TryActivateBreachBlockers();
    }
    
    if (bIsLeaderBreach || bIsLeaderThrow)
    {
        if (bLeaderUsedItem)
            UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTeamBreachAndClearActivity::LeaderCallBreachDone, 0.3f);
        else
            LeaderCallBreachDone();
    }
    else
    {
        if (SharedData->NumInTeam > 1)
        {
            if (!GetSharedData<FSharedBreachData>()->BreachCaller)
            {
                // find the last guy in clearing order who is not speaking
                for (ASWATCharacter* Swat : GetSharedData<FSharedBreachData>()->ClearingSortedSwat)
                {
                    if (!Swat->VoiceSoundSource || !Swat->VoiceSoundSource->bIsActive)
                    {
                        GetSharedData<FSharedBreachData>()->BreachCaller = Swat;
                        break;
                    }
                }
            }
            
            if (GetSharedData<FSharedBreachData>()->BreachCaller)
                GetSharedData<FSharedBreachData>()->BreachCaller->PlayRawVO(VO_SWAT_GENERAL::CALL_BREACH_DONE);
        }
    }

    TArray<ADoor*> AllowedTrackingDoors;
    for (ADoor* Door : GetSharedData<FSharedBreachData>()->Room->AdditionalRootDoors)
    {
        if (Door && Door != StackUpDoor)
        {
            AllowedTrackingDoors.AddUnique(Door);
        }
    }
    
    OwningController->GetTargetingComp()->SetAllowedTrackingDoors(AllowedTrackingDoors);
}

void UTeamBreachAndClearActivity::PerformClearStage(const float DeltaTime, float Uptime)
{
    bPauseIfTrackingEnemy = false;
    
    if (!StackUpDoor)
    {
        ACTIVITY_FAILED("PerformClearStage | No valid stack up door");
        return;
    }

    if (!ChosenClearPoints)
    {
        // failsafe, try again one more time.

        // temp debug code, will delete once found and fixed bug
        
        if (GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Num() == 0)
            ULog::Error("Clearing sorted swat array was empty");
        else
            ULog::Error("Clearing sorted swat array was not empty");
        
        ULog::Error("No clear points chosen. Trying again...");
        
        GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Empty();
        SortSwatForClearing();
        
        if (!ChosenClearPoints)
        {
            #if !UE_BUILD_SHIPPING
            if (GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Num() == 0)
                ULog::Error("Clearing sorted swat array was empty");
            else
                ULog::Error("Clearing sorted swat array was not empty");
            
            if (GetSharedData<FSharedBreachData>()->StackUpSortedSwat.Num() == 0)
                ULog::Error("Stackup sorted swat array was empty");
            else
                ULog::Error("Stackup sorted swat array was not empty");
            #endif
            
            ACTIVITY_FAILED("PerformClearStage | No clear points chosen");
            return;
        }
    }
    
    if (ChosenClearPoints->Num() <= 1)
    {
        bHasEverPassedThreshold = true;
        
        #if !UE_BUILD_SHIPPING
        // For automation testing, don't finish the activity this frame, just wait until next frame until the test recognizes it as a "success"
        if (!GIsAutomationTesting)
        #endif
        {
            OwningController->FinishActivity(this, true, true);
        }
        
        return;
    }
    
    const TArray<ASWATCharacter*>& SortedSwat = GetSharedData<FSharedBreachData>()->ClearingSortedSwat;

    if (SortedSwat.Num() == 0)
    {
        ACTIVITY_FAILED("PerformClearStage | Swat team was not sorted for clearing");
        return;
    }

    TimeSinceLastClearingTick += DeltaTime;
    if (TimeSinceLastClearingTick < 0.1f)
    {
        return;
    }

    ProgressState = FText::FromStringTable("SwatCommandTable", "Clearing");

    LocalClearingTime += TimeSinceLastClearingTick;
    
    TimeSinceLastClearingTick = 0.0f;
    
    if (GetCharacter() == SortedSwat[0])
    {
        bHasLeaderEverPassedThreshold = true;
    }

    if (OwningController->GetTrackedTarget())
    {
        bHasTrackedAnyoneWhileClearing = true;
    }
    
    StackUpDoor->DeactivateDoorBlocker();

    // failsafe
    bool bIsDestinationPointOnOtherSideOfDoor = StackUpDoor->IsPointsOnOppositeSideOfDoor(OwningController->GetRONPathFollowingComp()->GetPathDestination(), SharedData->CommandLocation);

    // close door if opened towards us
    if (!bHasEverPassedThreshold && !bIsDestinationPointOnOtherSideOfDoor && !bIsCollapsing && bHasLeaderEverPassedThreshold)
    {
        if (!StackUpDoor->IsOpenBeyond(0.5f))
        {
            StackUpDoor->UnlockDoor();
            if (GetCharacter() == SortedSwat[0])
            {
                GetCharacter()->ToggleDoor(StackUpDoor, true);
                return;
            }
        }
        
        if (GetCharacter()->GetVelocity().Size() <= 0.0f)
        {
            // todo: make function
            if (OverrideSquadPosition == ESquadPosition::SP_Alpha &&
                OccupiedStackUpActor &&
                !StackUpDoor->IsAnyAIOpening() && !StackUpDoor->IsAnyAIClosing())
            {
                if (Location != FVector::ZeroVector && HasReachedLocation(200.0f))
                {
                    const bool bCommandFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);

                    bool bOpenTowardsUs = bCommandFront ? StackUpDoor->IsOpen_Backward() : StackUpDoor->IsOpen_Forward();
                    if (bOpenTowardsUs && !StackUpDoor->IsClosing())
                    {
                        if (!GetCharacter()->IsAny3PMontageActive() && GetCharacter()->QueuedDoorToClose != StackUpDoor)
                        {
                            GetCharacter()->ToggleDoor(StackUpDoor, false);
                        }
                    }

                    if (ADoor* SubDoor = StackUpDoor->GetSubDoor())
                    {
                        const bool bCommandFront_SubDoor = SubDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);

                        bOpenTowardsUs = bCommandFront_SubDoor ? SubDoor->IsOpen_Backward() : SubDoor->IsOpen_Forward();
                        
                        if (bOpenTowardsUs && !SubDoor->IsClosing())
                        {
                            if (!GetCharacter()->IsAny3PMontageActive() && GetCharacter()->QueuedDoorToClose != SubDoor)
                            {
                                GetCharacter()->ToggleDoor(SubDoor, false);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!StackUpDoor->IsHalfwayOpen())
        return;
    
    // is the room too small for us to enter and thus become overcrowded?
    // if so, don't enter and hold position
    {
        if (FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
        {
            if (Room->Threats.Num() <= 4)
            {
                int32 Index = SortedSwat.Find(GetCharacter<ASWATCharacter>())+1;
                if (Index > Room->Threats.Num()-1)
                {
                    if (LocalClearingTime > 2.0f)
                    {
                        // consider this as a success
                        // since we deem it not necessary for everybody to enter a room
                        // especially if it's a small room where it can get overcrowed quickly with 4 swat guys
                        bHasEverPassedThreshold = true;

                        OwningController->FinishActivity(this, true, true);
                    }
                    
                    return;
                }
            }
        }
    }

    // try equip shield if we have one
    if (!bHasEverEquippedShieldWhileClearing)
    {
        if (GetCharacter()->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Shield) &&
            !GetCharacter()->GetEquippedItem<ABallisticsShield>())
        {
            EquipItem(EItemCategory::IC_Shield);
            bHasEverEquippedShieldWhileClearing = true;
        }
    }

    if (!bHasLeaderEverPassedThreshold)
    {
        const int32 Index = SortedSwat.Find(GetCharacter<ASWATCharacter>());
        if (SortedSwat.IsValidIndex(Index - 1))
        {
            if (const ASWATCharacter* Leader = SortedSwat[Index - 1])
            {
                if (Leader->IsActive())
                {
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

                    if (!ClosestDoor->IsOpening() || ClosestDoor->IsDoorwayOnly())
                    {
                        FVector ClosestPoint = StackUpDoor->CalculateClosestPoint(Leader->GetActorLocation());
                        FVector ClosestPoint2 = StackUpDoor->GetSubDoor() ? StackUpDoor->GetSubDoor()->CalculateClosestPoint(Leader->GetActorLocation()) : FVector::ZeroVector;
                        float Dist = FVector::Distance(ClosestPoint, Leader->GetActorLocation());
                        float Dist2 = ClosestPoint2 != FVector::ZeroVector ? FVector::Distance(ClosestPoint2, Leader->GetActorLocation()) : FLT_MAX;

                        float Threshold = 70.0f;
                        
                        bool bIsCommandFront = ClosestDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
                        bool bOpenTowardsUs = bIsCommandFront ? ClosestDoor->IsOpen_Backward() : ClosestDoor->IsOpen_Forward();

                        if (ClosestDoor->GetSubDoor())
                        {
                            Threshold = 40.0f;
                        }
                        
                        if (bOpenTowardsUs && !ClosestDoor->IsDoorwayOnly() && !ClosestDoor->IsDoorBroken())
                        {
                            Threshold = 140.0f;
                            
                            if (ClosestDoor->GetSubDoor())
                                Threshold = 200.0f;
                        }
                        
                        //ULog::Number(Dist, "Dist to Leader " + Leader->GetName() + ": ");
                        if (Dist <= Threshold ||
                            Dist2 <= Threshold ||
                            StackUpDoor->IsPointsOnOppositeSideOfDoor(Leader->GetActorLocation(), SharedData->CommandLocation) ||
                            !Leader->GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>()) // this somehow happens...
                        {
                            bHasLeaderEverPassedThreshold = true;
                            //ULog::Info(Leader->GetName() + " passed threshold");
                        }
                    }
                }
                else
                {
                    bHasLeaderEverPassedThreshold = true;
                }
            }
            else
            {
                bHasLeaderEverPassedThreshold = true;
            }
        }
        else
        {
            bHasLeaderEverPassedThreshold = true;
        }
    }
    
    //DrawDebugBox(GetWorld(), CornerFocalPoint, FVector(10.0f), FColor::Black, false, 10.0f, 0, 2.0f);
    //DrawDebugLine(GetWorld(), CornerFocalPoint, CornerFocalPoint + FVector(0.0f, 0.0f, 1000000.0f), FColor::Black, false, 10.0f, 0, 2.0f);

    // only clear corners if we're the leader
    if (!ClearingLeader)
    {
        if (bHasEverPassedThreshold || !bIsDestinationPointOnOtherSideOfDoor)
        {
            if (CornerFocalPoint != FVector::ZeroVector)
            {
                const FVector StartTrace = GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
                const FVector EndTrace = CornerFocalPoint;

                FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
                CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
                CollisionQueryParams.bTraceComplex = true;
                
                if (LocalClearingTime > 2.0f || !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, CollisionQueryParams))
                {
                    CornerFocalPoint = FVector::ZeroVector;
                }
            }
        }
        else
        {
            bool bIsHallway = GetSharedData<FSharedBreachData>()->BreachingRoomPosition == EDoorRoomPosition::Hallway ||
                              GetSharedData<FSharedBreachData>()->BreachingRoomPosition == EDoorRoomPosition::HallwayRight ||
                              GetSharedData<FSharedBreachData>()->BreachingRoomPosition == EDoorRoomPosition::HallwayLeft;
            
            bool bIsCornerFedRoom = GetSharedData<FSharedBreachData>()->BreachingRoomPosition != EDoorRoomPosition::Center ||
                                    GetSharedData<FSharedBreachData>()->StackingRoomPosition != EDoorRoomPosition::Center;

            if (!bIsCornerFedRoom && !bIsHallway)
            {
                constexpr float ForwardOffset = 25.0f;

                bool bLeftPath = ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints || ChosenClearPoints == &StackUpDoor->BackLeftClearPoints;
                
                FVector CornerCheckPoint = FVector::ZeroVector;
                if (bLeftPath)
                {
                    CornerCheckPoint = StackUpDoor->GetDoorMidLocation() - StackUpDoor->GetActorRightVector() * (StackUpDoor->GetDoorSize().Y+25.0f);
                }
                else
                {
                    CornerCheckPoint = StackUpDoor->GetDoorMidLocation() + StackUpDoor->GetActorRightVector() * (StackUpDoor->GetDoorSize().Y+25.0f);
                }
                
                CornerCheckPoint += StackUpDoor->GetActorForwardVector() * (IsCommandFrontOfDoor() ? -ForwardOffset : ForwardOffset);

                bool bSuccess = UReadyOrNotAISystem::ProjectPointToNav(CornerCheckPoint, CornerCheckPoint, FVector(50.0f, 50.0f, 150.0f));
                CornerCheckPoint.Z = GetCharacter()->GetActorLocation().Z;

                /*
                FVector ClosestPointOnDoor = StackUpDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation());

                float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(200.0f, 100.0f), FVector2D(0.0f, 1.0f), FVector::Distance(GetCharacter()->GetActorLocation(), ClosestPointOnDoor));
                if (bHasEverPassedThreshold)
                    Alpha = 1.0f;

                //LOG_NUMBER(Alpha);
                ClosestPointOnDoor = StackUpDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation()) + StackUpDoor->GetActorForwardVector() * (IsCommandFrontOfDoor() ? -300.0f : 300.0f);
                FVector CornerPoint = FMath::Lerp(ClosestPointOnDoor, CornerCheckPoint, Alpha);
                */

                if (bSuccess)
                {
                    CornerFocalPoint = CornerCheckPoint;
                }
                else
                {
                    CornerFocalPoint = FVector::ZeroVector;
                }
            }
            else
            {
                CornerFocalPoint = FVector::ZeroVector;
            }
        }
    }
    else
    {
        CornerFocalPoint = FVector::ZeroVector;
    }
    
    if (!bHasLeaderEverPassedThreshold)
        return;

    OwningController->GetTargetingComp()->ClearCustomFocusPoints();
    
    // update the clearing leader
    {
        TArray<ASWATCharacter*> SwatOnClearPath;
        for (ASWATCharacter* Swat : SortedSwat)
        {
            if (Swat->GetCyberneticsController<ASWATController>())
            {
                if (UTeamBreachAndClearActivity* Activity = Swat->GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
                {
                    if (Activity->ChosenClearPoints == ChosenClearPoints)
                    {
                        SwatOnClearPath.Add(Swat);
                    }
                }
            }
        }
        
        for (uint8 i = 1; i < SwatOnClearPath.Num(); i++)
        {
            ASWATCharacter* Swat = SwatOnClearPath[i];
            if (Swat->GetCyberneticsController<ASWATController>())
            {
                if (UTeamBreachAndClearActivity* BreachAndClearActivity = Swat->GetCyberneticsController<ASWATController>()->GetBreachAndClearActivity())
                {
                    if (SwatOnClearPath.IsValidIndex(i-1))
                    {
                        BreachAndClearActivity->ClearingLeader = SwatOnClearPath[i-1];
                    }
                }
            }
        }
    }

    if (OwningController->IsMovingForRequest(LastRequestedMoveId) && bIsDestinationPointOnOtherSideOfDoor) // wait for nav mesh to update before collapsing
    {
        if (OccupiedStackUpActor)
        {
            OccupiedStackUpActor->OccupiedBy = nullptr;
            OccupiedStackUpActor = nullptr;
            bHasLetGoOfStackUp = true;
            bIsCollapsing = false;
        }
    }

    if (!OccupiedStackUpActor)
        OverrideSquadPosition = (ESquadPosition)SortedSwat.Find(GetCharacter<ASWATCharacter>());
    
    if (!bHasEverPassedThreshold && StackUpDoor->IsPointsOnOppositeSideOfDoor(GetCharacter()->GetActorLocation(), SharedData->CommandLocation))
    {
        bHasEverPassedThreshold = true;
        
        OwningController->GetTargetingComp()->CustomFocusActor = nullptr;
        OwningController->GetTargetingComp()->CustomFocusLocation = FVector::ZeroVector;

        // all passed threshold? disable room blockers and breach blockers
        bool bAllPassedThreshold = true;
        UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
        {
            if (Activity->SharedData->ActivityId == SharedData->ActivityId && !Activity->bHasEverPassedThreshold)
            {
                bAllPassedThreshold = false;
                return false;
            }

            return true;
        });
        
        if (bAllPassedThreshold)
        {
            for (ADoor* Door : GetSharedData<FSharedBreachData>()->CurrentRoom->AdditionalRootDoors)
            {
                if (Door && !GetSharedData<FSharedBreachData>()->Room->AdditionalRootDoors.Contains(Door)) // part of the breaching room, dont disable it yet
                {
                    // make sure no other breach activity is using this door
                    bool bAnyStackUpForThisDoor = false;
                    UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
                    {
                        if (Activity->SharedData->ActivityId != SharedData->ActivityId)
                        {
                            if (Activity->GetSharedData<FSharedBreachData>()->CurrentRoom->AdditionalRootDoors.Contains(Door))
                            {
                                bAnyStackUpForThisDoor = true;
                                return false;
                            }
                        }
                        
                        return true;
                    });
                    
                    if (bAnyStackUpForThisDoor)
                        continue;
                    
                    Door->DeactivateDoorBlocker();
                }
            }
            
            StackUpDoor->DeactivateBreachBlockers();
        }
    }
    
    if (bHasEverPassedThreshold)
    {
        // done here because we dont have a gurantee that EnterClearedStage() would ever be run
        // (could be due to the player cancelling or swat dying or any other unforseen factor)
        if (FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
        {
            Room->bClearedBySwat = true;
            if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
            {
                GS->RoomData->ClearedRooms.AddUnique(Room);
            }
        }
        
        // test if we're in front of forward points. if so, immediately move up stages
        // if there are uncleared landmarks as we are moving up stages, stop moving up and clear that first
        {
            for (uint8 i = CurrentClearStage; i < ChosenClearPoints->Num(); i++)
            {
                const FClearPoint& Point = (*ChosenClearPoints)[i];
                
                if (Point.Direction == EClearDirection::Right) // idk if i like this
                {
                    const FVector StartTrace = GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
                    const FVector EndTrace = FVector(Point.Location);

                    const bool bIsAboveClearPoint = FMath::Abs(GetCharacter()->GetActorLocation().Z - Point.Location.Z) > 50.0f;
                    
                    bool bHasLOS = bIsAboveClearPoint || !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, QueryParameters);
                    
                    if (!Point.bHasLineOfSightToDoor && !bHasLOS) // we must check it before moving up
                        break;
                }

                bool bPreviousStageWasForward = i > 0 && (*ChosenClearPoints)[i-1].Direction == EClearDirection::Forward;
                if (Point.Direction == EClearDirection::Forward && bPreviousStageWasForward)// && FVector::Distance(FVector(Point.Location), GetCharacter()->GetActorLocation()) < 1000.0f) // must be somewhat near to auto-clear it
                {
                    FVector ForwardDirection = IsCommandFrontOfDoor() ? -StackUpDoor->GetActorForwardVector() : StackUpDoor->GetActorForwardVector();
                    FVector DirectionToMe = (GetCharacter()->GetActorLocation() - FVector(Point.Location)).GetSafeNormal2D();
                    if (FVector::DotProduct(ForwardDirection, DirectionToMe) > 0.0f)
                    {
                        // cap the stage to this if there are landmarks that have not been cleared yet
                        if (AnyUnclearedLandmarksForClearPoint(&Point))
                        {
                            CurrentClearPoint = nullptr;
                            CurrentClearStage = Point.Stage;
                            break;
                        }
                        
                        CurrentClearPoint = nullptr;
                        CurrentClearStage = Point.Stage;
                    }
                }
            }
        }

        if (ClearingLeader || GetSharedData<FSharedBreachData>()->NumInTeam == 2)
        {
            if (ChosenClearPoints->Num() >= 30) // min 30 points for auto chemlight deploying to work for large rooms
            {
                // is halfway?
                if (CurrentClearStage >= ChosenClearPoints->Num()/2.0f && !bDroppedChemlightHalfway)
                {
                    bDroppedChemlightHalfway = true;
                    TryDropChemlight();
                }
            }
        }
    }

    // pause movement if leader is searching landmark and is nearby them
    if (bHasEverPassedThreshold && CurrentClearStage > 2)
    {
        if (ClearingLeader && ClearingLeader->GetCyberneticsController())
        {
            if (USearchLandmarkActivity* Activity = ClearingLeader->GetCyberneticsController()->GetCurrentActivity<USearchLandmarkActivity>())
            {
                if (FVector::Distance(GetCharacter()->GetNavAgentLocation(), ClearingLeader->GetActorLocation()) < 150.0f)
                {
                    OwningCharacter->ReasonsToStandStill.AddUnique("Leader searching landmark");
                    OwningController->PauseMove(OwningController->GetRONPathFollowingComp()->GetCurrentRequestId());
                    return;
                }
            }
        }
        
        if (OwningCharacter->ReasonsToStandStill.Contains("Leader searching landmark"))
        {
            OwningCharacter->ReasonsToStandStill.Remove("Leader searching landmark");
            OwningController->ResumeMove(OwningController->GetRONPathFollowingComp()->GetCurrentRequestId());
        }
    }
    
    if (!CurrentClearPoint && !NearestThreat && CurrentClearStage <= StageLimit)
    {
        TryMoveToCurrentStage();
    }
    
    if (CurrentClearPoint && !NearestThreat)
    {
        if (AIBlockingClearingPath)
        {
            FVector Direction = (GetCharacter()->GetNavAgentLocation() - AIBlockingClearingPath->GetNavAgentLocation()).GetSafeNormal2D();
            SetLocation(AIBlockingClearingPath->GetActorLocation() + Direction * 150.0f, false);
        }
        else
        {
            SetLocation(CurrentClearPoint->Location, false);
        }

        // move up stages if we cant project on the navmesh
        // (could be due to the door blocking off a corner of a room where the current clear point is)
        if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation NewLocationProjected(FVector(CurrentClearPoint->Location));
            if (!NavSys->ProjectPointToNavigation(FVector(CurrentClearPoint->Location), NewLocationProjected, FVector(75.0f, 75.0f, 150.0f)))
            {
                CurrentClearStage = FMath::Clamp<uint8>(CurrentClearStage + 1, 1, StageLimit);
                CurrentClearPoint = nullptr;

                // Also, switch chosen path if we can't project all points on a nav mesh (due to the above reason)
                if (StageLimit < 3) // only if small amount of clear points
                {
                    if (CurrentClearStage >= StageLimit)
                    {
                        if (ChosenClearPoints == &StackUpDoor->BackLeftClearPoints)
                            ChosenClearPoints = &StackUpDoor->BackRightClearPoints;
                        else if (ChosenClearPoints == &StackUpDoor->BackRightClearPoints)
                            ChosenClearPoints = &StackUpDoor->BackLeftClearPoints;
                        else if (ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints)
                            ChosenClearPoints = &StackUpDoor->FrontRightClearPoints;
                        else if (ChosenClearPoints == &StackUpDoor->FrontRightClearPoints)
                            ChosenClearPoints = &StackUpDoor->FrontLeftClearPoints;
                        
                        CurrentClearStage = 1;
                        StageLimit = ChosenClearPoints->Num()-1;
                        
                        TryMoveToCurrentStage();
                    }
                }
            }
        }
    }

    if (StageLimit == 255)
        StageLimit = ChosenClearPoints->Num()-1;
    
    if (ClearingLeader && ClearingLeader->IsActive() && !NearestThreat && bHasEverPassedThreshold)
    {
        if (const UTeamBreachAndClearActivity* Activity = ClearingLeader->GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>())
        {
            if (Activity->CurrentClearStage >= Activity->StageLimit || Activity->NearestThreat)
            {
                StageLimit = Activity->StageLimit - 3;

                // recalculate
                if (CurrentClearStage > StageLimit)
                {
                    CurrentClearStage = StageLimit;
                    CurrentClearPoint = nullptr;
                    AbortMove(true);

                    if (!HasReachedLocation(200.0f))
                    {
                        TryMoveToCurrentStage();
                    }
                }
            }
        }
    }
    else
    {
        if (!ClearingLeader)
            StageLimit = ChosenClearPoints->Num()-1;
    }
    
    // have we cleared the current point?
    if (CurrentClearPoint)
    {
        const FVector StartTrace = GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
        const FVector EndTrace = FVector(CurrentClearPoint->Location);

        const bool bIsAboveClearPoint = FMath::Abs(GetCharacter()->GetActorLocation().Z - CurrentClearPoint->Location.Z) > 50.0f;
        
        bHasLOSToClearPoint = bIsAboveClearPoint || !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, QueryParameters);
        
        const float ClearAcceptanceRadius_WithLeader = FMath::GetMappedRangeValueClamped(FVector2D(6, 30), FVector2D(100.0f, 400.0f), ChosenClearPoints->Num());
        const float ClearAcceptanceRadius_NoLeader = FMath::GetMappedRangeValueClamped(FVector2D(6, 30), FVector2D(30.0f, 200.0f), ChosenClearPoints->Num());
        const float Tolerance = CurrentClearStage > StageLimit - 3 && !ClearingLeader ? ClearAcceptanceRadius_NoLeader : ClearAcceptanceRadius_WithLeader;
        const bool bIsForwardClearPointReached = CurrentClearPoint->Direction == EClearDirection::Forward && bHasLOSToClearPoint && HasReachedLocation(Tolerance);
        const bool bIsRightClearPointVisible = CurrentClearPoint->Direction == EClearDirection::Right && bHasLOSToClearPoint && HasReachedLocation(500.0f);

        bool bIsInFrontOfForwardClearPoint = false;
        if (CurrentClearPoint->Direction == EClearDirection::Forward)
        {
            FVector ForwardDirection = IsCommandFrontOfDoor() ? -StackUpDoor->GetActorForwardVector() : StackUpDoor->GetActorForwardVector();
            FVector DirectionToMe = (GetCharacter()->GetActorLocation() - FVector(CurrentClearPoint->Location)).GetSafeNormal2D();
            bIsInFrontOfForwardClearPoint = FVector::DotProduct(ForwardDirection, DirectionToMe) > 0.0f && CurrentClearStage != StageLimit;
        }

        uint8 NumForwardClearPoints = 0;
        for (uint8 i = 0; i < ChosenClearPoints->Num(); i++)
        {
            const FClearPoint* ClearPoint = &(*ChosenClearPoints)[i];

            if (ClearPoint->Direction == EClearDirection::Forward)
            {
                NumForwardClearPoints++;
            }
        }
        
        // look back 3 stages to see if the current forward clear point is close to a right clear point
        bool bIsForwardClearPointCloseToRightClearPoint = false;
        if (NumForwardClearPoints > 3 && CurrentClearStage > 3 && CurrentClearPoint->Direction == EClearDirection::Forward)
        {
            for (uint8 i = 0; i < 3; i++)
            {
                uint8 PreviousStage = CurrentClearStage - (i+1);
                if (ChosenClearPoints->IsValidIndex(PreviousStage))
                {
                    const FClearPoint* PreviousClearPoint = &(*ChosenClearPoints)[PreviousStage];
                    if (PreviousClearPoint->Direction == EClearDirection::Right)
                    {
                        if (FVector::Distance(FVector(PreviousClearPoint->Location), FVector(CurrentClearPoint->Location)) < 100.0f)
                        {
                            bIsForwardClearPointCloseToRightClearPoint = true;
                            break;
                        }
                    }
                }
            }
        }

        AIBlockingClearingPath = nullptr;
        
        if (!ClearingLeader && bHasEverPassedThreshold && CurrentClearStage > 1) // must at least be in the room to consider stopping
        {
            if (const FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
            {
                // get all characters within room
                TArray<ACyberneticCharacter*, TFixedAllocator<64>> AIInRoom;
                for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
                {
                    if (AI && AI->IsActive() && !AI->IsInRagdoll() && !AI->IsOnSWATTeam())
                    {
                        if (const AThreatAwarenessActor* TAA = AI->GetCyberneticsController()->GetTargetingComp()->GetNearestThreat())
                        {
                            if (TAA->OwningRoom == Room->Name)
                            {
                                AIInRoom.AddUnique(AI);
                            }
                        }
                    }
                }
                
                for (ACyberneticCharacter* AI : AIInRoom)
                {
                    // is ai on the same side as the clearing path
                    if (ChosenClearPoints == &StackUpDoor->BackLeftClearPoints)
                    {
                        const bool bIsLeft = !StackUpDoor->IsActorInFrontOfDoorway(AI) && !StackUpDoor->IsActorRightOfDoorway(AI);
                        if (!bIsLeft)
                        {
                            continue;
                        }
                    }
                    else if (ChosenClearPoints == &StackUpDoor->BackRightClearPoints)
                    {
                        const bool bIsRight = !StackUpDoor->IsActorInFrontOfDoorway(AI) && StackUpDoor->IsActorRightOfDoorway(AI);
                        if (!bIsRight)
                        {
                            continue;
                        }
                    }
                    else if (ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints)
                    {
                        const bool bIsLeft = StackUpDoor->IsActorInFrontOfDoorway(AI) && !StackUpDoor->IsActorRightOfDoorway(AI);
                        if (!bIsLeft)
                        {
                            continue;
                        }
                    }
                    else if (ChosenClearPoints == &StackUpDoor->FrontRightClearPoints)
                    {
                        const bool bIsRight = StackUpDoor->IsActorInFrontOfDoorway(AI) && StackUpDoor->IsActorRightOfDoorway(AI);
                        if (!bIsRight)
                        {
                            continue;
                        }
                    }

                    const FClearPoint* ClosestClearPoint = nullptr;
                    float ClosestDistance = 500.0f;
                    for (FClearPoint& P : *ChosenClearPoints)
                    {
                        const float D = FVector::Distance(AI->GetActorLocation(), P.Location);
                        if (D < ClosestDistance)
                        {
                            ClosestDistance = D;
                            ClosestClearPoint = &P;
                        }
                    }

                    if (ClosestClearPoint)
                    {
                        //DrawDebugLine(GetWorld(), AI->GetActorLocation(), ClosestClearPoint->Location, FColor::Yellow, false, 0.1f);
                        
                        if (ClosestClearPoint->Stage == 0)
                        {
                            AIBlockingClearingPath = AI;
                            break;
                        }
                        
                        if (CurrentClearPoint && CurrentClearPoint->Stage >= ClosestClearPoint->Stage-2)
                        {
                            AIBlockingClearingPath = AI;
                            break;
                        }
                    }
                }
            }
        }
        
        if (!AIBlockingClearingPath &&
            (bIsForwardClearPointReached ||
            bIsRightClearPointVisible ||
            bIsInFrontOfForwardClearPoint ||
            bIsForwardClearPointCloseToRightClearPoint ||
            CurrentClearPoint->Stage == 0))
        {
            ACoverLandmark* UnclearedLandmark = nullptr;
            for (ACoverLandmark* Landmark : CurrentClearPoint->CoverLandmarks)
            {
                if (Landmark && Landmark->bEnabled && !Landmark->bClearedBySwat)
                {
                    UnclearedLandmark = Landmark;
                    break;
                }
            }

            // detour from the current clear path and search cover landmarks
            if (UnclearedLandmark && !GetCharacter()->GetEquippedItem<ABallisticsShield>())
            {
                if (UnclearedLandmark->Type == ECoverLandmarkType::Custom)
                {
                    if (UnclearedLandmark->CustomSearchActivityClass)
                    {
                        if (USearchLandmarkActivity* SearchLandmarkActivity = UActivityManager::CreateActivity<USearchLandmarkActivity>(this, UnclearedLandmark->CustomSearchActivityClass))
                        {
                            SearchLandmarkActivity->CoverLandmark = UnclearedLandmark;
                            
                            UActivityManager::GiveActivityTo(SearchLandmarkActivity, GetCharacter(), true, false);
                        }
                    }
                    else
                    {
                        UnclearedLandmark->bClearedBySwat = true; // failsafe incase no custom search activity was specified
                    }
                }
                else
                {
                    if (USearchLandmarkActivity* SearchLandmarkActivity = GetOwningController<ASWATController>()->GetSearchLandmarkActivity())
                    {
                        SearchLandmarkActivity->CoverLandmark = UnclearedLandmark;	
                        
                        UActivityManager::GiveActivityTo(SearchLandmarkActivity, GetCharacter(), true, false);
                    }
                }
                
                return;
            }

            CurrentClearStage = FMath::Clamp<uint8>(CurrentClearStage + 1, 1, StageLimit+1);
            CurrentClearPoint->bCleared = true;
            CurrentClearPoint = nullptr;

            if (bHasEverPassedThreshold)
            {
                if (CurrentClearStage > StageLimit - 1)
                {
                    OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
                }
                else
                {
                    if (CurrentClearStage > 1)
                    {
                        //FVector DoorLocation = StackUpDoor->GetDoorMidLocation() + (IsCommandFrontOfDoor() ? StackUpDoor->GetActorForwardVector() * 100.0f : StackUpDoor->GetActorForwardVector() * -100.0f);
                        
                        //OwningController->GetTargetingComp()->SetIgnoredTrackingDirection();

                        FVector Direction = (StackUpDoor->GetDoorMidLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();
                        if (CurrentClearStage < 4)
                        {
                            bool bLeftPath = ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints || ChosenClearPoints == &StackUpDoor->BackLeftClearPoints;
                            Direction = bLeftPath ? StackUpDoor->GetActorRightVector() : -StackUpDoor->GetActorRightVector();
                        }
                        
                        OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(Direction);
                    }
                }
            }
        }
    }

    if (bNextStageIsOccupied)
    {
        OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
    }
    
    if (!CurrentClearPoint && bHasEverPassedThreshold)
    {
        if (CurrentClearStage > StageLimit)
        {
            GetCharacter()->MoveIgnoreActorRemove(StackUpDoor);
            
            if (!bHasTrackedAnyoneWhileClearing)
            {
                if (!bCalledOutAreaClear)
                {
                    bCalledOutAreaClear = USWATManager::Get(this)->PlaySpeechWithSharedCooldown(VO_SWAT_GENERAL::CALL_COVER_AREA, GetCharacter(), 0.25f);
                }
            }
            
            OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
            
            // Immediately finish if auto clearing (just so we can reduce the slow down time) between auto clearing doors
            if (GetSharedData<FSharedBreachData>()->bIsAuto && !GetSharedData<FSharedBreachData>()->bLastForAutoClear)
            {
                TryDropChemlight();
                EnterClearedStage();
                return;
            }
            
            EquipWeapon();
            
            // Occupy the nearest threat
            if (!NearestThreat)
            {
                if (UThreatAwarenessSubsystem* System = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
                {
                    TArray<AThreatAwarenessActor*> ExcludedThreats;
                    UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
                    {
                        if (Activity != this && Activity->NearestThreat)
                        {
                            ExcludedThreats.AddUnique(Activity->NearestThreat);
                        }

                        return true;
                    });

                    // In a small room, include extreme threats so we can find somewhere to position ourselves without clipping with each other
                    TArray<EThreatLevel> ExcludedThreatLevels;
                    FName RoomName = NAME_None;
                    if (const FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
                    {
                        RoomName = Room->Name;
                        
                        uint8 NumNonExtremeThreats = 0;
                        for (AThreatAwarenessActor* Threat : Room->Threats)
                        {
                            if (Threat->GetThreatLevel() != EThreatLevel::TL_Extreme)
                            {
                                NumNonExtremeThreats++;
                            }
                        }
                        
                        if (NumNonExtremeThreats > 4)
                        {
                            ExcludedThreatLevels.Add(EThreatLevel::TL_Extreme);
                        }
                    }
                    
                    // first try to find a non-extreme threats
                    if (AThreatAwarenessActor* Threat = System->GetNearestThreatForLocation(GetCharacter()->GetActorLocation(), 1000.0f, 150.0f, false, {EThreatLevel::TL_Extreme}, ExcludedThreats, RoomName))
                    {
                        NearestThreat = Threat;
                        SetLocation(Threat->GetActorLocation(), true);
                    }
                    else if (AThreatAwarenessActor* Threat2 = System->GetNearestThreatForLocation(GetCharacter()->GetActorLocation(), 1000.0f, 150.0f, false, ExcludedThreatLevels, ExcludedThreats, RoomName))
                    {
                        NearestThreat = Threat2;
                        SetLocation(Threat2->GetActorLocation(), true);
                    }
                }

                // try drop another chem at the end (if it's a big room)
                if (ChosenClearPoints->Num() >= 15)
                {
                    if (!ClearingLeader) // is at the head of clearing? (i.e not following behind anyone?)
                    {
                        TryDropChemlight();
                    }
                }
                else
                {
                    if (OverrideSquadPosition == ESquadPosition::SP_Alpha || (ClearingLeader && ChosenClearPoints->Num() > 10))
                        TryDropChemlight();
                }
            }
            else
            {
                FVector Threat = NearestThreat->GetActorLocation();
                Threat.Z = GetCharacter()->GetNavAgentLocation().Z;
                
                if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                {
                    FNavLocation NewLocationProjected(Threat);
                    if (!NavSys->ProjectPointToNavigation(Threat, NewLocationProjected, FVector(75.0f, 75.0f, 150.0f)))
                    {
                        Location = GetCharacter()->GetNavAgentLocation();
                    }
                    else
                    {
                        Location = NewLocationProjected.Location;
                    }
                }

                RequestMoveAsync();
            }
        }
        else
        {
            TryMoveToCurrentStage();
        }
    }

    if (bHasEverPassedThreshold && LocalClearingTime > 1.0f)
    {
        // try call out border patrol VO
        if (!GetSharedData<FSharedBreachData>()->bCalledOutBorderPatrol)
        {
            bool bHasLandmarksOnClearingPath = false;
            for (FClearPoint& P : *ChosenClearPoints)
            {
                if (P.CoverLandmarks.Num() > 0)
                {
                    bHasLandmarksOnClearingPath = true;
                    break;
                }
            }
            
            if (bHasLandmarksOnClearingPath)
            {
                if (USWATManager::Get(this)->PlaySpeechWithSharedCooldown(VO_SWAT_GENERAL::CALL_TRAIT_BORDER_PATROL, GetCharacter()))
                {
                    GetSharedData<FSharedBreachData>()->bCalledOutBorderPatrol = true;
                }
            }
        }
        
        if (CurrentClearStage > 1 && CurrentClearStage < StageLimit)
        {
            // try call out openings
            FVector First = FVector((*ChosenClearPoints)[0].Location);

            if (FRoom* Room = GetSharedData<FSharedBreachData>()->Room)
            {
                uint8 NumOpenDoors = 0;
                bool bAllHallways = false;
                for (ADoor* Door : Room->AdditionalRootDoors)
                {
                    if (Door && Door != StackUpDoor && Door != StackUpDoor->GetSubDoor())
                    {
                        if (Door->IsDoorwayOnly() || Door->IsOpenBeyondIncrementThreshold())
                        {
                            if (StackUpDoor->IsPointsOnOppositeSideOfDoor(SharedData->CommandLocation, Door->GetDoorMidLocation()))
                            {
                                NumOpenDoors++;
                                
                                EDoorRoomPosition RoomPosition;
                                if (Door->IsPointInFrontOfDoorway(First))
                                {
                                    RoomPosition = Door->FrontRoomPosition;
                                }
                                else
                                {
                                    RoomPosition = Door->BackRoomPosition;
                                }

                                if (RoomPosition != EDoorRoomPosition::Hallway && RoomPosition != EDoorRoomPosition::HallwayRight && RoomPosition != EDoorRoomPosition::HallwayLeft)
                                {
                                    bAllHallways = false;
                                }
                            }
                        }
                    }
                }

                if (NumOpenDoors > 1 && SharedData->NumInTeam == 1)
                {
                    if (!GetSharedData<FSharedBreachData>()->bCalledOutFrontOpening)
                    {
                        if (GetCharacter()->PlayRawVO(bAllHallways ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_MULTIPLE : VO_SWAT_GENERAL::CALL_OPENING_MULTIPLE))
                        {
                            GetSharedData<FSharedBreachData>()->bCalledOutFrontOpening = true;
                            GetSharedData<FSharedBreachData>()->bCalledOutLeftOpening = true;
                            GetSharedData<FSharedBreachData>()->bCalledOutRightOpening = true;
                        }
                    }
                }
                else
                {
                    for (ADoor* Door : Room->AdditionalRootDoors)
                    {
                        if (Door && Door != StackUpDoor && Door != StackUpDoor->GetSubDoor())
                        {
                            if (Door->IsDoorwayOnly() || Door->IsOpenBeyondIncrementThreshold())
                            {
                                //if (GetCharacter()->HasLineOfSightTo(Door->GetDoorMidLocation()))
                                if (StackUpDoor->IsPointsOnOppositeSideOfDoor(SharedData->CommandLocation, Door->GetDoorMidLocation()))
                                {
                                    FVector BaseDirection;
                                    if (IsCommandFrontOfDoor())
                                        BaseDirection = StackUpDoor->GetActorForwardVector();
                                    else
                                        BaseDirection = -StackUpDoor->GetActorForwardVector();

                                    FVector OppositeDirection;
                                    if (Door->IsPointInFrontOfDoorway(First))
                                        OppositeDirection = -Door->GetActorForwardVector();
                                    else
                                        OppositeDirection = Door->GetActorForwardVector();
                                    
                                    FVector TestPoint = Door->GetDoorMidLocation() + OppositeDirection * 50.0f; // bias it a little
                                    
                                    //DrawDebugBox(GetWorld(), TestPoint, FVector(10.0f), FColor::Cyan, false, 2.0f);
                                    //DrawDebugBox(GetWorld(), StackUpDoor->GetDoorMidLocation(), FVector(10.0f), FColor::Orange, false, 2.0f);

                                    //ULog::Vector(TestPoint, false, "Test Point: ");
                                    //ULog::Vector(StackUpDoor->GetDoorMidLocation(), false, "Door Point: ");
                                    
                                    const FVector DirectionToDoor = (TestPoint - StackUpDoor->GetDoorMidLocation()).GetSafeNormal2D();
                                    const float RightDot = FVector::DotProduct(BaseDirection, DirectionToDoor);

                                    bool bDoorsAreParallel = FMath::Abs(FVector::DotProduct(BaseDirection, OppositeDirection)) >= 0.99f;
                                    
                                    bool bLeftPath = ChosenClearPoints == &StackUpDoor->FrontLeftClearPoints || ChosenClearPoints == &StackUpDoor->BackLeftClearPoints;
                                    
                                    //LOG_NUMBER(RightDot);

                                    if (FMath::Abs(RightDot) > 0.95f) // directly in front
                                    {
                                        bool* bCallOutPtr = nullptr;
                                        EDoorRoomPosition RoomPosition, RoomPosition_Opposite;
                                        if (Door->IsPointInFrontOfDoorway(First))
                                        {
                                            RoomPosition = Door->FrontRoomPosition;
                                            RoomPosition_Opposite = Door->BackRoomPosition;
                                        }
                                        else
                                        {
                                            RoomPosition = Door->BackRoomPosition;
                                            RoomPosition_Opposite = Door->FrontRoomPosition;
                                        }

                                        bool bInsideHallway = RoomPosition_Opposite == EDoorRoomPosition::Hallway || RoomPosition_Opposite == EDoorRoomPosition::HallwayLeft || RoomPosition_Opposite == EDoorRoomPosition::HallwayRight;
                                        bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;
                                        bool bMovesLeft = false;
                                        bool bMovesRight = false;

                                        FString VO = VO_SWAT_GENERAL::CALL_OPENING_FRONT;
                                        if (bInsideHallway)
                                        {
                                            bMovesLeft = RoomPosition == EDoorRoomPosition::CornerRight;
                                            bMovesRight = RoomPosition == EDoorRoomPosition::CornerLeft;
                                            
                                            if (bMovesLeft)
                                            {
                                                VO = VO_SWAT_GENERAL::CALL_OPENING_LEFT;
                                            }
                                            else if (bMovesRight)
                                            {
                                                VO = VO_SWAT_GENERAL::CALL_OPENING_RIGHT;
                                            }
                                        }
                                        else if (bIsHallway)
                                        {
                                            VO = VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_FRONT;
                                        }
                                        
                                        if (bMovesLeft)
                                            bCallOutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutLeftOpening;
                                        else if (bMovesRight)
                                            bCallOutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutRightOpening;
                                        else
                                            bCallOutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutFrontOpening;

                                        if (!(*bCallOutPtr))
                                        {
                                            if (GetCharacter()->PlayRawVO(VO))
                                            {
                                                GetSharedData<FSharedBreachData>()->bCalledOutFrontOpening = true;
                                                *bCallOutPtr = true;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (StackUpDoor->IsPointRightOfDoorway(TestPoint))
                                        {
                                            if (!bLeftPath)
                                            {
                                                if (RightDot < 0.0f)
                                                {
                                                    if (bDoorsAreParallel)
                                                        TryCalloutLeftOpening(Door, First);
                                                    else
                                                        TryCalloutRightOpening(Door, First);
                                                }
                                                else
                                                {
                                                    if (bDoorsAreParallel)
                                                        TryCalloutRightOpening(Door, First);
                                                    else
                                                        TryCalloutLeftOpening(Door, First);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (bLeftPath)
                                            {
                                                if (RightDot < 0.0f)
                                                {
                                                    if (bDoorsAreParallel)
                                                        TryCalloutRightOpening(Door, First);
                                                    else
                                                        TryCalloutLeftOpening(Door, First);
                                                }
                                                else
                                                {
                                                    if (bDoorsAreParallel)
                                                        TryCalloutLeftOpening(Door, First);
                                                    else
                                                        TryCalloutRightOpening(Door, First);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void UTeamBreachAndClearActivity::TryMoveToCurrentStage()
{
    if (ChosenClearPoints->Num() <= 1)
        return;

    if (ChosenClearPoints->IsValidIndex(CurrentClearStage))
    {
        FClearPoint& Point = (*ChosenClearPoints)[CurrentClearStage];
        
        bool bIsPointOccupied = false;
        if (CurrentClearStage > 3 && ClearingLeader) // ignore the first 3 clear points, dont be too stict on those as they're near the door
        {
            UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
            {
                if (Activity != this && !Activity->IsActivityComplete() && Activity->CurrentClearPoint)
                {
                    if ((&Point == Activity->CurrentClearPoint) || (Activity->AIBlockingClearingPath && Activity->AIBlockingClearingPath == AIBlockingClearingPath))
                    {
                        bIsPointOccupied = true;
                        return false;
                    }
                }
                
                return true;
            });
        }

        bNextStageIsOccupied = bIsPointOccupied;
        
        if (!bIsPointOccupied)
        {
            CurrentClearPoint = &Point;
            
            if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
            {
                FNavLocation NewLocationProjected(FVector(Point.Location));
                if (!NavSys->ProjectPointToNavigation(FVector(Point.Location), NewLocationProjected, FVector(75.0f, 75.0f, 150.0f)))
                {
                    CurrentClearStage++;
                }
                else
                {
                    Location = NewLocationProjected.Location;
                    RequestMoveAsync();
                }
            }
        }
    }
}

void UTeamBreachAndClearActivity::PerformScanStage(float DeltaTime, float Uptime)
{
    if (DoorScanner)
    {
        if (DoorScanner->IsDeadOrUnconscious() || DoorScanner->IsIncapacitated() || !DoorScanner->IsActive())
        {
            DoorScanner = nullptr;
            OnDoorScanFinished_Internal();
        }
    }
}

////////// Cleared state events //////////
void UTeamBreachAndClearActivity::EnterClearedStage()
{
	GetCharacter()->MoveIgnoreActorRemove(StackUpDoor);
    
    OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
    
    CurrentClearPoint = nullptr;

    EquipWeapon();

    OnCleared.Broadcast(this, GetSharedData<FSharedBreachData>()->bIsAuto);
    
    OwningController->FinishActivity(this, true, true);
}

void UTeamBreachAndClearActivity::EnterScanStage()
{
    if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
    {
        return;
    }

    if (GetSharedData<FSharedBreachData>()->bHasChosenScanner)
    {
        return;
    }

    if (AnySwatOnOtherSide())
    {
        if (IsDoorUser())
            return;
    }

    StackUpDoor->DeactivateDoorBlocker();
    
    GetSharedData<FSharedBreachData>()->bHasChosenScanner = true;
    
    uint8 HighestDepth = 0;
    for (const AStackUpActor* StackUpActor : StackUpDoor->GetStackupsForArea(ChosenStackUpArea))
    {
        if (StackUpActor)
        {
            if (StackUpActor->Depth > HighestDepth)
            {
                HighestDepth = StackUpActor->Depth;
            }
        }
    }

    bool bAnySwatOnOtherSide = false;
    EStackupGenArea OppositeStackUpArea = ChosenStackUpArea;
    ADoor::FlipStackUpArea(OppositeStackUpArea, true, false);
    const TArray<AStackUpActor*> OppositeStackUps = StackUpDoor->GetStackupsForArea(OppositeStackUpArea);
    for (const AStackUpActor* StackUpActor : OppositeStackUps)
    {
        if (StackUpActor && StackUpActor->OccupiedBy)
        {
            bAnySwatOnOtherSide = true;
            break;
        }
    }

    const bool bCommandInFront = StackUpDoor->IsPointInFrontOfDoorway(SharedData->CommandLocation);
    const EDoorRoomPosition RoomPosition = bCommandInFront ? StackUpDoor->FrontRoomPosition : StackUpDoor->BackRoomPosition;
    const bool bIsCenterFedOnly = RoomPosition == EDoorRoomPosition::Center;

    bool bCanProject = false;
    if (OppositeStackUps.Num() > 0)
    {
        FVector a;
        bCanProject = UReadyOrNotAISystem::ProjectPointToNav(OppositeStackUps[0]->GetActorLocation(), a, FVector(50.0f, 50.0f, 150.0f));
    }

    const bool bIsDoorOpenOnCommandSide = !StackUpDoor->IsDoorwayOnly() && ((StackUpDoor->IsOpen_Backward() && bCommandInFront) || (StackUpDoor->IsOpen_Forward() && !bCommandInFront));
    const bool bAnyStackUpsOnOtherSide = OppositeStackUps.Num() > 0;
    const bool bCanPie = bIsCenterFedOnly &&
                        GetSharedData<FSharedBreachData>()->StackUpStyle != EStackUpStyle::Split &&
                        bCanProject &&
                        bAnyStackUpsOnOtherSide &&
                        !bAnySwatOnOtherSide &&
                        !bIsDoorOpenOnCommandSide;
                        
    const bool bEnoughRoomForAssessment = HighestDepth >= 1;

    if (bEnoughRoomForAssessment)
    {
        EThresholdAssessment ChosenAssessment = EThresholdAssessment::None;
        
        switch (GetSharedData<FSharedBreachData>()->Assessment)
        {
            case EThresholdAssessment::Pie:                 if (bCanPie) ChosenAssessment = EThresholdAssessment::Pie; else ChosenAssessment = EThresholdAssessment::CenterCheck;  break;
            case EThresholdAssessment::CenterCheck:         ChosenAssessment = EThresholdAssessment::CenterCheck;       break;
            default:                                        break;
        }

        EDoorScanMethod ScanMethod = EDoorScanMethod::None;
        if (ChosenAssessment == EThresholdAssessment::Pie)
        {
            ChosenStackUpArea = OppositeStackUpArea;
            OccupiedStackUpActor->OccupiedBy = nullptr;
            OccupiedStackUpActor = OppositeStackUps[0];
            OccupiedStackUpActor->OccupiedBy = OwningController;
            
            for (const AStackUpActor* StackUpActor : OppositeStackUps)
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
            
            ScanMethod = EDoorScanMethod::Slice;
        }
        else if (ChosenAssessment == EThresholdAssessment::CenterCheck)
        {
            ScanMethod = EDoorScanMethod::CenterCheck;
        }
        
        GetSharedData<FSharedBreachData>()->Assessment = ChosenAssessment;

        if (ScanMethod != EDoorScanMethod::None)
        {
            bool bPlayedVO = false;
            if (GetSquadLeader() && !Cast<ASWATCharacter>(GetSquadLeader()) && !GetSharedData<FSharedBreachData>()->bIsAuto &&
                FVector::Distance(GetSquadLeader()->GetActorLocation(), GetCharacter()->GetActorLocation()) < 1000.0f)
            {
                if (ChosenAssessment == EThresholdAssessment::Pie)
                    bPlayedVO = GetSquadLeader()->PlayRawVO(VO_SWAT_COMMAND::CALL_SCAN_DOOR_PIE);
            }

            if (bPlayedVO)
            {
                if (UScanDoorActivity* ScanDoorActivity = GetOwningController<ASWATController>()->GetScanDoorActivity())
                {
                    DoorScanActivity = ScanDoorActivity;
                    DoorScanner = GetCharacter();
                }
            
                UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &UTeamBreachAndClearActivity::GiveScanActivity, ChosenAssessment), 0.5f);
            }
            else
            {
                GiveScanActivity(ChosenAssessment);
            }
        }
    }
}

void UTeamBreachAndClearActivity::GiveScanActivity(EThresholdAssessment Assessment)
{
    if (UScanDoorActivity* ScanDoorActivity = GetOwningController<ASWATController>()->GetScanDoorActivity())
    {
        ScanDoorActivity->Door = StackUpDoor;
        ScanDoorActivity->CommandLocation = SharedData->CommandLocation;
        if (Assessment == EThresholdAssessment::Pie)
            ScanDoorActivity->ScanMethod = EDoorScanMethod::Slice;
        else if (Assessment == EThresholdAssessment::CenterCheck)
            ScanDoorActivity->ScanMethod = EDoorScanMethod::CenterCheck;
        ScanDoorActivity->OppositeStackUpLocation = FIntVector(OccupiedStackUpActor->GetActorLocation());
        ScanDoorActivity->OnFinishActivity.RemoveAll(this);
        ScanDoorActivity->OnFinishActivity.AddDynamic(this, &UTeamBreachAndClearActivity::OnDoorScanFinished);

        DoorScanActivity = nullptr;
        DoorScanner = nullptr;
        
        if (UActivityManager::GiveActivityTo(ScanDoorActivity, GetCharacter()))
        {
            DoorScanActivity = ScanDoorActivity;
            DoorScanner = GetCharacter();

            if (Assessment == EThresholdAssessment::CenterCheck)
            {
                OccupiedStackUpActor->OccupiedBy = nullptr;
                OccupiedStackUpActor = nullptr;
                
                SortSwatForClearing();
            }
        }
    }
}

bool UTeamBreachAndClearActivity::IsScanFinished() const
{
    if (ShieldUser)
    {
        if (GetSquadPositionForCharacter(ShieldUser) != ESquadPosition::SP_Alpha)
        {
            return false;
        }
        
        if (const UTeamStackUpActivity* ShieldUserActivity = ShieldUser->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
        {
            if (ShieldUserActivity->bIsSwapping)
            {
                return false;
            }
        }
    }

    if (DoorUseActivity || DoorBreachActivity)
        return false;
    
    if (DoorScanner && !DoorScanner->IsDeadOrUnconscious() && !DoorScanner->IsIncapacitated())
    {
        if (DoorScanner->bHasEverShot && DoorScanner->TimeSinceLastShot < 1.0f)
        {
            return false;
        }
    }
    
    return GetSharedData<FSharedBreachData>()->bHasChosenScanner && DoorScanActivity == nullptr;
}

void UTeamBreachAndClearActivity::OnDoorScanFinished(UBaseActivity* Activity, ACyberneticController* Controller)
{
    Activity->OnFinishActivity.RemoveAll(this);

    OnDoorScanFinished_Internal();
}

void UTeamBreachAndClearActivity::OnDoorScanFinished_Internal()
{
    DoorScanActivity = nullptr;

    SortSwatForClearing();

    if (ChosenClearPoints && ChosenClearPoints->Num() > 1 && GetSharedData<FSharedBreachData>()->ClearingSortedSwat.Num() > 0)
    {
        CurrentClearStage = 1;

        if (GetSharedData<FSharedBreachData>()->ClearingSortedSwat[0] == GetCharacter()) // only if we're the first man in, immediately request a move
        {
            SetLocation(FVector((*ChosenClearPoints)[1].Location), true);
        }
    }
}

void UTeamBreachAndClearActivity::SetupDoorUsers()
{
    DoorChecker = FindChecker();
    
    if (StackUpDoor->IsDoorwayOnly() || StackUpDoor->IsOpenBeyondCloseThreshold())
    {
        DoorChecker = nullptr;
        DoorUser = nullptr;
    }
    else
    {
        if (!StackUpDoor->IsClosed())
        {
            DoorChecker = nullptr;
        }
        
        DoorUser = FindUser();
    }

    if (DoorUser)
    {
        if (GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Alpha, ChosenStackUpArea) != DoorUser)
        {
            if (UTeamStackUpActivity* DoorUserActivity = DoorUser->GetCyberneticsController()->GetActivity<UTeamStackUpActivity>())
            {
                DoorUserActivity->SwapSquadPositionWith(ESquadPosition::SP_Alpha);
            }
        }
        
        bBreacherReady = GetSharedData<FSharedBreachData>()->NumInTeam == 1;
    }
    else
    {
        if (!bIsLeaderBreach)
            bHasUserBreached = true;
    }
    
    if (ACyberneticCharacter* ShieldGuy = GetCharacterWithItem(ABallisticsShield::StaticClass()))
    {
        ShieldUser = ShieldGuy;
    }
    
    DoorBreacher = FindBreacher(DoorUser);
    
    // Sanity checks
    #if WITH_EDITOR
    if (StackUpDoor->IsDoorwayOnly() || StackUpDoor->IsOpenBeyondCloseThreshold())
    {
        ensureAlways(DoorChecker == nullptr);
        ensureAlways(DoorUser == nullptr);

        if (DoorBreacher && DoorBreachItemClass)
        {
            ensureAlways(DoorBreacher->GetInventoryComponent()->GetInventoryItemOfClass(DoorBreachItemClass, false) != nullptr);
        }
    }
    else
    {
        if (StackUpDoor->IsClosed() && !DoorBreacher)
            ensureAlways(DoorChecker != nullptr);

        if (bIsLeaderBreach)
        {
            ensureAlways(DoorUser == nullptr);
            ensureAlways(DoorUseItemClass == nullptr);
        }
        else
        {
            ensureAlways(DoorUser != nullptr);
            
            if (DoorUser && DoorUseItemClass)
            {
                ensureAlways(DoorUser->GetInventoryComponent()->GetInventoryItemOfClass(DoorUseItemClass, false) != nullptr);
            }
        }

        const bool bOnlyLeaderThrow = bIsLeaderThrow && !bIsLeaderBreach;
        
        if (bOnlyLeaderThrow)
        {
            ensureAlways(DoorBreacher == nullptr);
            ensureAlways(DoorBreachItemClass == nullptr);
        }
        else
        {
            if (DoorBreacher && DoorBreachItemClass)
            {
                ensureAlways(DoorBreacher->GetInventoryComponent()->GetInventoryItemOfClass(DoorBreachItemClass, false) != nullptr);
            }
        }
    }
    
    if (DoorBreachItemClass)
        ensureAlways(DoorBreacher != nullptr);
    else
        ensureAlways(DoorBreacher == nullptr);
    #endif
}

void UTeamBreachAndClearActivity::TryActivateDoorBlocker(FRoom* Room)
{
    if (!Room)
        return;
    
    for (ADoor* Door : Room->AdditionalRootDoors)
    {
        if (!Door)
            continue;
        
        // make sure no other breach activity is using this door
        bool bAnyStackUpForThisDoor = false;
        bool bAnyOneStackingInCurrentRoom = false;
        UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
        {
            if (Activity != this && Activity->GetSharedData<FSharedStackUpData>())
            {
                if (Activity->StackUpDoor == Door)
                {
                    bAnyStackUpForThisDoor = true;
                    return false;
                }

                if (Activity->SharedData->ActivityId != SharedData->ActivityId)
                {
                    if (Activity->GetSharedData<FSharedStackUpData>()->CurrentRoom == GetSharedData<FSharedStackUpData>()->CurrentRoom)
                    {
                        bAnyOneStackingInCurrentRoom = true;
                        return false;
                    }
                }
            }
            
            return true;
        });
        
        if (bAnyStackUpForThisDoor || bAnyOneStackingInCurrentRoom)
            continue;
        
        const bool bSubDoorCheck = !StackUpDoor->GetSubDoor() || (StackUpDoor->GetSubDoor() && StackUpDoor->GetSubDoor() != Door);
        if (Door != StackUpDoor && bSubDoorCheck)
        {
            Door->ActivateDoorBlocker();
            Door->CurrentStackUpActivities.AddUnique(this);
        }
    }

    StackUpDoor->DeactivateDoorBlocker(); // just in case
}

void UTeamBreachAndClearActivity::TryDeactivateDoorBlocker(FRoom* Room)
{
    if (!Room)
        return;
    
    for (ADoor* Door : Room->AdditionalRootDoors)
    {
        if (Door)
        {
            Door->CurrentStackUpActivities.Remove(this);
            
            // make sure no other breach activity is using this door
            bool bAnyStackUpForThisDoor = false;
            UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
            {
                if (Activity->SharedData->ActivityId != SharedData->ActivityId)
                {
                    if (Activity->StackUpDoor == Door)
                    {
                        bAnyStackUpForThisDoor = true;
                        return false;
                    }
                }
                
                return true;
            });
            
            if (bAnyStackUpForThisDoor)
                continue;
            
            Door->DeactivateDoorBlocker();
        }
    }
}

void UTeamBreachAndClearActivity::TryActivateBreachBlockers()
{
    // Causing more trouble than it's worth, may turn this back on if problems persist
    return;
    
    // make sure no other team is stack up within the same room to prevent their breach blockers from blocking the way of our door
    // the reason we have breach blockers is because swat for whatever reason will take the long way route sometimes.
    // a better fix would be to look at why this actually happens but for now this hack fix seems to do the job fine
    
    for (ADoor* Door : GetSharedData<FSharedBreachData>()->CurrentRoom->AdditionalRootDoors)
    {
        if (!Door)
            continue;

        if (Door == StackUpDoor)
            continue;
        
        // make sure no other breach activity is using this door
        bool bAnyStackUpForThisDoor = false;
        UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
        {
            if (Activity->SharedData->ActivityId != SharedData->ActivityId)
            {
                if (Activity->StackUpDoor == Door)
                {
                    bAnyStackUpForThisDoor = true;
                    return false;
                }
            }
            
            return true;
        });
        
        if (bAnyStackUpForThisDoor)
            return;
    }

    StackUpDoor->ActivateBreachBlockers(IsCommandFrontOfDoor());
}

void UTeamBreachAndClearActivity::TryDropChemlight()
{
    // not when in combat
    if (UReadyOrNotAISystem::WasRecentlyInCombat(5.0f))
        return;
    
    // ballistic shields cannot drop chems
    if (GetCharacter()->GetEquippedItem<ABallisticsShield>())
        return;
    
    bool bAnyAIDroppingChemlight = false;
    UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
    {
        if (Activity->SharedData->ActivityId == SharedData->ActivityId)
        {
            if (Activity->GetCharacter()->IsTableMontagePlaying("tp_swt_dropchem"))
            {
                bAnyAIDroppingChemlight = true;
                return false;
            }
        }

        return true;
    });

    if (bAnyAIDroppingChemlight)
        return;
    
    // Don't try to deploy a chemlight if there is already one nearby
    bool bFoundChemlight = false;
    for (const AThrownItem* ThrownItem : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllThrownItems)
    {
        if (const FRoom* ItemInRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(ThrownItem->ThrownInRoom))
        {
            if (const FRoom* CurrentRoom = GetSharedData<FSharedBreachData>()->Room)
            {
                if (ItemInRoom->Name == CurrentRoom->Name)
                {
                    if (FVector::Distance(ThrownItem->GetActorLocation(), GetCharacter()->GetActorLocation()) < 1000.0f)
                    {
                        bFoundChemlight = true;
                    }
                }
            }
        }
    }

    if (!bFoundChemlight)
    {
        GetCharacter()->PlayMontageFromTable("tp_swt_dropchem");
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_CHEMLIGHT);
    }
}

bool UTeamBreachAndClearActivity::AnyUnclearedLandmarksForClearPoint(const FClearPoint* ClearPoint) const
{
    for (const ACoverLandmark* Landmark : ClearPoint->CoverLandmarks)
    {
        if (!Landmark->bClearedBySwat)
        {
            return true;
        }
    }

    return false;
}

float UTeamBreachAndClearActivity::GetPathDistanceToNextPoint() const
{
    if (const UReadyOrNotPathFollowingComp* PathFollowingComp = OwningController->GetRONPathFollowingComp())
    {
        if (PathFollowingComp->GetPath().IsValid())
        {
            return PathFollowingComp->GetPath()->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), PathFollowingComp->GetNextPathIndex());
        }
    }

    return 0.0f;
}

void UTeamBreachAndClearActivity::LeaderCallBreachDone()
{
    if (AReadyOrNotCharacter* Leader = Cast<APlayerCharacter>(GetSquadLeader()))
    {
        Leader->PlayRawVO(VO_SWAT_GENERAL::CALL_BREACH_DONE, "", false);
    }
}

void UTeamBreachAndClearActivity::TryCalloutLeftOpening(ADoor* TargetDoor, FVector Point)
{
    bool* bCalloutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutLeftOpening;
    
    EDoorRoomPosition RoomPosition;
    if (TargetDoor->IsPointInFrontOfDoorway(Point))
        RoomPosition = TargetDoor->FrontRoomPosition;
    else
        RoomPosition = TargetDoor->BackRoomPosition;

    bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;
    
    FString VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_LEFT : VO_SWAT_GENERAL::CALL_OPENING_LEFT;
    
    bool bInvert = !StackUpDoor->IsPointInFrontOfDoorway(Point);
    if (bInvert)
    {
        VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_RIGHT : VO_SWAT_GENERAL::CALL_OPENING_RIGHT;
        bCalloutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutRightOpening;
    }

    if (!(*bCalloutPtr))
    {
        if (GetCharacter()->PlayRawVO(VO, "", false))
        {
            *bCalloutPtr = true;
            
            //DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), TargetDoor->GetDoorMidLocation(), FColor::Magenta, false, 2.0f, 0, 2.0f);
            //DrawDebugBox(GetWorld(), TargetDoor->GetDoorMidLocation(), FVector(20.0f), FColor::Black, false, 2.0f, 0, 2.0f);
            //DrawDebugString(GetWorld(), TargetDoor->GetDoorMidLocation(), "Left Opening", nullptr, FColor::White, 2.0f, true);
        }
    }
}

void UTeamBreachAndClearActivity::TryCalloutRightOpening(ADoor* TargetDoor, FVector Point)
{
    bool* bCalloutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutRightOpening;
    
    EDoorRoomPosition RoomPosition;
    if (TargetDoor->IsPointInFrontOfDoorway(Point))
        RoomPosition = TargetDoor->FrontRoomPosition;
    else
        RoomPosition = TargetDoor->BackRoomPosition;

    bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;

    FString VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_RIGHT : VO_SWAT_GENERAL::CALL_OPENING_RIGHT;

    bool bInvert = !StackUpDoor->IsPointInFrontOfDoorway(Point);
    if (bInvert)
    {
        VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_LEFT : VO_SWAT_GENERAL::CALL_OPENING_LEFT;
        bCalloutPtr = &GetSharedData<FSharedBreachData>()->bCalledOutLeftOpening;
    }
    
    if (!(*bCalloutPtr))
    {
        if (GetCharacter()->PlayRawVO(VO, "", true))
        {
            *bCalloutPtr = true;

            //DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), TargetDoor->GetDoorMidLocation(), FColor::Magenta, false, 2.0f, 0, 2.0f);
            //DrawDebugBox(GetWorld(), TargetDoor->GetDoorMidLocation(), FVector(20.0f), FColor::Black, false, 2.0f, 0, 2.0f);
            //DrawDebugString(GetWorld(), TargetDoor->GetDoorMidLocation(), "Right Opening", nullptr, FColor::White, 2.0f, true);
        }
    }
}

bool UTeamBreachAndClearActivity::ShouldCheckDoorBeforeBreach() const
{
    if (bHasCheckedDoor)
        return false;
    
    if (StackUpDoor->IsDoorwayOnly())
        return false;
    
    if (DoorBreachType >= EDoorBreachType::Move)
        return false;
    
    if (DoorBreachType == EDoorBreachType::Open && StackUpDoor->IsLocked())
        return true;
    
    if (DoorBreachType == EDoorBreachType::Open)
        return false;
    
    return true;
}

bool UTeamBreachAndClearActivity::IsDoorBreacher() const
{
    return DoorBreacher && GetCharacter() == DoorBreacher;
}

bool UTeamBreachAndClearActivity::IsDoorUser() const
{
    return DoorUser && GetCharacter() == DoorUser;
}

bool UTeamBreachAndClearActivity::IsDoorScanner() const
{
    return DoorScanner && GetCharacter() == DoorScanner;
}

bool UTeamBreachAndClearActivity::HasLeaderPassedThreshold() const
{
    const TArray<ASWATCharacter*>& SortedSwat = GetSharedData<FSharedBreachData>()->ClearingSortedSwat;
    if (SortedSwat.Num() <= 1)
        return true;

    if (GetSharedData<FSharedBreachData>()->NumInTeam == 1)
        return true;
    
    bool bLeaderHasPassedThreshold = SortedSwat[0] == GetCharacter();
    if (!bLeaderHasPassedThreshold)
    {
        const int32 Index = SortedSwat.Find(GetCharacter<ASWATCharacter>());
        if (SortedSwat.IsValidIndex(Index - 1))
        {
            if (const ASWATCharacter* Leader = SortedSwat[Index - 1])
            {
                if (!Leader->IsActive())
                    return true;
                
                bLeaderHasPassedThreshold = StackUpDoor->IsPointsOnOppositeSideOfDoor(Leader->GetActorLocation(), SharedData->CommandLocation);
            }
        }
    }

    return bLeaderHasPassedThreshold;
}

bool UTeamBreachAndClearActivity::AnySwatOnOtherSide(uint8* Num) const
{
    EStackupGenArea OppositeStackUpArea = ChosenStackUpArea;
    ADoor::FlipStackUpArea(OppositeStackUpArea, true, false);
    const TArray<AStackUpActor*> OppositeStackUps = StackUpDoor->GetStackupsForArea(OppositeStackUpArea);
    
    if (Num)
    {
        *Num = 0;
        for (const AStackUpActor* StackUpActor : OppositeStackUps)
        {
            if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
            {
                *Num++;
            }
        }
    }
    
    for (const AStackUpActor* StackUpActor : OppositeStackUps)
    {
        if (StackUpActor && Cast<ACyberneticController>(StackUpActor->OccupiedBy))
        {
            return true;
        }
    }

    return false;
}

void UTeamBreachAndClearActivity::RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath)
{
    Super::RequestMoveFromPath(InLocation, NavPath);
}

bool UTeamBreachAndClearActivity::IsFinishedClearing() const
{
    if (LocalClearingTime < 2.5f)
        return false;

    if (!bHasEverPassedThreshold)
        return false;
        
    if (!ChosenClearPoints)
        return true;
    
    if (ChosenClearPoints->Num() <= 1)
        return true;

    if (OwningController->GetActivity<USearchLandmarkActivity>())
        return false;

    return HasReachedLocation(50.0f) && CurrentClearStage >= StageLimit;
}

bool UTeamBreachAndClearActivity::CanCollapse() const
{
    if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
        return false;

    if (bHasLetGoOfStackUp)
        return false;
    
    if (CurrentClearStage > 1 || CurrentClearPoint)
        return false;
    
    if (bHasEverPassedThreshold)
        return false;
    
    if (IsDoorScanner())
        return false;
    
    if (GetActiveStateID() >= 3 && GetActiveStateID() < 5 && DoorScanner) // during and after breach but not clear
    {
        if (DoorScanActivity)
        {
            if (!DoorScanActivity->HasStartedActivity())
                return false;

            return DoorScanActivity->GetActiveStateID() >= 1 && DoorScanActivity->GetSliceStage() > 1 && DoorScanActivity->GetActiveStateUptime() > 0.15f;
        }

        return false;
    }
    
    return GetActiveStateID() >= 2 && GetActiveStateID() <= 5;
}

bool UTeamBreachAndClearActivity::ShouldCloseDoorWhenStackingUp()
{
    return false;
    
    if (DoorBreachType == EDoorBreachType::Move)
    {        
        return false;
    }
    
    return Super::ShouldCloseDoorWhenStackingUp();
}

#undef StackUpDoor
#undef DoorChecker
#undef bNewStackUpDoor
#undef bStackOppositeSide
#undef bHasCheckedDoor
#undef DoorCheckResult
#undef bHasStartedCheckingLock
#undef DoorBreachType
#undef DoorUser
#undef DoorBreacher
#undef DoorScanner
#undef ShieldUser
#undef DoorUseItemClass
#undef DoorBreachItemClass
#undef bIsLeaderThrow
#undef bIsLeaderBreach
#undef bHasBreacherBreached
#undef bHasUserBreached
#undef bHasLeaderBreached
#undef bLeaderUsedItem
#undef bBreacherReady
#undef bIsBreaching
#undef DoorUseActivity
#undef DoorBreachActivity
#undef DoorScanActivity
#undef BreachingTime
#undef ClearingTime
