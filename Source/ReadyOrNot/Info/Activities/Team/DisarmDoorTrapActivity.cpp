// Copyright Void Interactive, 2021

#include "DisarmDoorTrapActivity.h"

#include "LockPickDoorActivity.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/Door.h"
#include "Actors/Items/Multitool.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"

#include "Info/SWATManager.h"

#include "ReadyOrNotAIConfig.h"
#include "Components/InteractableComponent.h"

UDisarmDoorTrapActivity::UDisarmDoorTrapActivity()
{
    ActivityName = FText::FromStringTable("SwatCommandTable", "DisarmTrap");

    bReturnToPositionAfterInteraction = true;
}

bool UDisarmDoorTrapActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    if (Door && Door->GetAttachedTrap())
    {
        if (Door->IsPointInFrontOfDoorway(CommandLocation))
        {
            FocalPoint = Door->GetAttachedTrap()->GetActorLocation() + Door->GetAttachedTrap()->GetActorForwardVector() * 25.0f;
        }
        else
        {
            FocalPoint = Door->GetAttachedTrap()->GetActorLocation() - Door->GetAttachedTrap()->GetActorForwardVector() * 25.0f;
        }
        
        return true;
    }
    
    return Super::OverrideFocalPoint(FocalPoint);
}

void UDisarmDoorTrapActivity::FinishedActivity(const bool bSuccess)
{
    Super::FinishedActivity(bSuccess);
}

void UDisarmDoorTrapActivity::OnInteractionBegin()
{
    Super::OnInteractionBegin();

    ProgressState = FText::FromStringTable("SwatCommandTable", "CuttingWire");
}

void UDisarmDoorTrapActivity::OnInteractionEnd()
{
    Super::OnInteractionEnd();
    
    if (CheckEdgeCases())
    {
        Door->GetAttachedTrap()->OnTrapDisarmed(GetCharacter());

        Door->SetDoorTrapKnowledge(GetCharacter()->IsSuspect(), true);
        
        if (Door->IsOpen())
        {
            Door->CloseDoor(nullptr);
        }

        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_TRAP_DISARMED);

        ProgressState = FText::FromStringTable("ScoringTable", "TrapDisarmed");
    }
}

float UDisarmDoorTrapActivity::GetInteractionDistance() const
{
    return 83.5f;
}

FString UDisarmDoorTrapActivity::GetInteractionAnimation() const
{
    return "tp_swt_disarmtrap";
}

FVector UDisarmDoorTrapActivity::GetInteractionLocation() const
{
    return Door ? Door->GetAttachedTrap()->GetActorLocation() + Door->GetAttachedTrap()->GetActorForwardVector() * 30.0f : Super::GetInteractionLocation();
}

bool UDisarmDoorTrapActivity::IsDoorTrapLive() const
{
    return Door && Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live;
}

bool UDisarmDoorTrapActivity::CheckEdgeCases()
{
    if (!Super::CheckEdgeCases())
        return false;
    
    if (Door && !Door->GetAttachedTrap())
    {
        ACTIVITY_FAILED("No valid trap actor or trap is not live", true);
        return false;
    }

    return true;
}

void UDisarmDoorTrapActivity::EnterGetInPositionStage()
{
    Super::EnterGetInPositionStage();
    
    ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForDisarm");
}

bool UDisarmDoorTrapActivity::CanInteract() const
{
    return Super::CanInteract() && IsDoorTrapLive();
}

bool UDisarmDoorTrapActivity::CanOverrideActivity() const
{
    return true;
}

void UDisarmDoorTrapActivity::EnterInteractStage()
{
    if (CheckEdgeCases())
    {
        const bool bTrapInFront = Door->IsPointInFrontOfDoorway(Door->GetAttachedTrap()->GetActorLocation());
        const bool bCommandLocInFront = Door->IsPointInFrontOfDoorway(CommandLocation);
        
        if (!Door->AnyBottomDoorChunksBroken() && ((bCommandLocInFront && !bTrapInFront) || (!bCommandLocInFront && bTrapInFront)))
        {
            if (Door->IsLocked())
            {
                GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCKED);
                USWATManager* SwatManager = USWATManager::Get(this);
                if (SwatManager)
                {
                    //SwatManager->GivePickLockCommand(Door, GetCharacter()->GetTeam(), CommandLocation);
                    ULockPickDoorActivity* DoorActivity = UActivityManager::CreateActivity<ULockPickDoorActivity>(this, ULockPickDoorActivity::StaticClass(), ActivityName, 0.0f);
                    DoorActivity->Door = Door;
                    DoorActivity->CommandLocation = CommandLocation;
                    UActivityManager::GiveActivityTo(DoorActivity, GetCharacter(), true, false);
                    ActivityStateMachine->Reset();
                    return;
                }
                
                ACTIVITY_FAILED("Door is locked");
                return;
            }
            
            if (!Door->IsOpen())
            {
                Door->PeekDoor(GetCharacter(), 15.0f);
            }
        }

        Super::EnterInteractStage();
    }
}

void UDisarmDoorTrapActivity::OnDoorOpened()
{
    // Don't fail activity, allow it if opening door to cut the trap wire
    if (ActivityStateMachine->GetActiveState()->ID != 1) // Interact
        Super::OnDoorOpened();
}

void UDisarmDoorTrapActivity::OnTrapTriggered(ATrapActor* Trap, AReadyOrNotCharacter* TriggeredBy)
{
    if (!Door)
        return;
    
    if (Trap == Door->GetAttachedTrap())
    {
        if (TriggeredBy)
            ACTIVITY_FAILED("Trap triggered by " + TriggeredBy->GetName(), true);
        else
            ACTIVITY_FAILED("Trap triggered", true);
    }
}

void UDisarmDoorTrapActivity::BindEvents()
{
    Super::BindEvents();

    GetCharacter()->OnTrapDisarmBegin_FromAnimNotify.AddDynamic(this, &UDisarmDoorTrapActivity::OnInteractionBegin);
    GetCharacter()->OnTrapDisarmEnd_FromAnimNotify.AddDynamic(this, &UDisarmDoorTrapActivity::OnInteractionEnd);

    if (Door && Door->GetAttachedTrap())
    {
        Door->GetAttachedTrap()->TrapTriggered.AddDynamic(this, &UDisarmDoorTrapActivity::OnTrapTriggered);

        Door->GetAttachedTrap()->InteractableComponent->ResetToOriginalLocation();

        // Don't want to interact with the trap if this AI is disarming it
        for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
        {
            UBpGameplayHelperLib::DisableInteractionForController(Door->GetAttachedTrap(), *It);
        }
    }
}

void UDisarmDoorTrapActivity::UnbindEvents()
{
    Super::UnbindEvents();
    
    GetCharacter()->OnTrapDisarmBegin_FromAnimNotify.RemoveAll(this);
    GetCharacter()->OnTrapDisarmEnd_FromAnimNotify.RemoveAll(this);

    if (Door && Door->GetAttachedTrap())
    {
        Door->GetAttachedTrap()->TrapTriggered.RemoveAll(this);

        Door->GetAttachedTrap()->InteractableComponent->ResetToOriginalLocation();

        // Disable player's from interacting with the trap if this AI is disarming it
        for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
        {
            UBpGameplayHelperLib::EnableInteractionForController(Door->GetAttachedTrap(), *It);
        }
    }
}
