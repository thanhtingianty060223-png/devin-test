// Copyright Void Interactive, 2021

#include "DoorInteractionActivity.h"

#include "TeamBaseActivity.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/Door.h"

UDoorInteractionActivity::UDoorInteractionActivity()
{
    ActivityName = 	FText::FromStringTable("SwatCommandTable", "DoorInteraction");
    bIsProgressActivity = true;
    bCanInteractWithDoorway = false;
    MaxActivityTime = 60.0f;
    bFinishActivityWhenOverriden = true;

    MoveAcceptanceRadius = 1.0f;
    
    bDisablePlayerDoorInteraction = true;

    ActivityStateMachine->AddState("Get In Position")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::EnterGetInPositionStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::PerformGetInPositionStage))
                        .BindEventExit(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::ExitGetInPositionStage))
                        .CreateTransition("Interact", MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::CanInteract));
    
    ActivityStateMachine->AddState("Interact")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::EnterInteractStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::PerformInteractStage))
                        .BindEventExit(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::ExitInteractStage))
                        .CreateTransition("Return", MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::CanReturn));
    
	ActivityStateMachine->AddState("Return")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::EnterReturnStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::TickReturnStage))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UDoorInteractionActivity::ExitReturnStage));
}

void UDoorInteractionActivity::StartActivity(AAIController* Owner)
{
    Super::StartActivity(Owner);

    if (CheckEdgeCases())
    {
        BindEvents();
        DisableDoorInteractable();
    }
}

void UDoorInteractionActivity::ResumeActivity()
{
    Super::ResumeActivity();

    if (CheckEdgeCases())
    {
        BindEvents();
        DisableDoorInteractable();
    }
}

void UDoorInteractionActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
    Super::ActivityOverriden(OverridingActivity);

    StopInteractionAnim();
    
    UnbindEvents();

    EnableDoorInteractable();
}

void UDoorInteractionActivity::FinishedActivity(const bool bSuccess)
{
    Super::FinishedActivity(bSuccess);

    if (!bSuccess)
        StopInteractionAnim();
    
    UnbindEvents();

    EnableDoorInteractable();

    // Fixup anything we may have been using mid anim
    GetCharacter()->GetInventoryComponent()->ReattachAllGear();
    
    if (Door)
    {
        Door->OperatingStates.Remove("AI");
    }
}

void UDoorInteractionActivity::FinishedActivity_NoOwner(bool Success)
{
    Super::FinishedActivity_NoOwner(Success);

    UnbindEvents_Internal();
    
    EnableDoorInteractable();
    
    if (Door)
    {
        Door->OperatingStates.Remove("AI");
    }
}

FVector UDoorInteractionActivity::GetInteractionLocation() const
{
	if (Door)
	{
		if (Door->GetSubDoor())
		{
			return Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
		}

		return Door->GetDoorMidLocation();
	}

    return FVector::ZeroVector;
}

float UDoorInteractionActivity::GetInteractionDistance() const
{
    return 100.0f;
}

FString UDoorInteractionActivity::GetInteractionAnimation() const
{
    return "";
}

bool UDoorInteractionActivity::IsLeftSideInteraction() const
{
    if (!Door)
        return false;
	
    const bool bIsInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
    const bool bIsOnTacticalSideOfDoor = Door->IsActorRightOfDoorway(GetCharacter());

    return (bIsInFrontOfDoor && bIsOnTacticalSideOfDoor) || (!bIsInFrontOfDoor && !bIsOnTacticalSideOfDoor);
}

void UDoorInteractionActivity::EnterGetInPositionStage()
{
}

void UDoorInteractionActivity::PerformGetInPositionStage(float DeltaTime, float Uptime)
{
    if (CheckEdgeCases())
    {
        RequestMoveToInteractLocation();
    }
}

void UDoorInteractionActivity::ExitGetInPositionStage()
{
}

bool UDoorInteractionActivity::ShouldGetInPosition() const
{
    return true;
}

bool UDoorInteractionActivity::CanInteract() const
{
    /*
	FVector FocalPoint = Door->GetDoorMidLocation() + Door->GetActorForwardVector() * (Door->IsPointInFrontOfDoorway(CommandLocation) ? GetInteractionDistance() : -GetInteractionDistance());
    
    const float DotProduct = FVector::DotProduct((FocalPoint - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
    const bool bIsFacingCover = DotProduct > (0.98f - (ActivityStateMachine->GetState(0).Uptime * 0.05f));
    //LOG_NUMBER(DotProduct);
    */
    
	const bool bIncreaseTolerance = TimeSinceLastAsyncMove > 1.0f && !OwningController->IsMoving();
    return Door /*&& bIsFacingCover*/ && (HasReachedLocation(GetDestinationTolerance()) || (bIncreaseTolerance && HasReachedLocation(ActivityStateMachine->GetState(0).Uptime * 25)));
}

void UDoorInteractionActivity::EnterInteractStage()
{
    if (CheckEdgeCases())
    {
        Door->OperatingStates.AddUnique("AI");
        FVector ActivityFocalPoint;
        OverrideFocalPoint(ActivityFocalPoint);
        PlayInteractionAnim(ActivityFocalPoint);
    }
}

void UDoorInteractionActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
}

void UDoorInteractionActivity::ExitInteractStage()
{
    if (Door)
    {
        Door->OperatingStates.Remove("AI");
    }
}

void UDoorInteractionActivity::OnInteractionBegin()
{
}

void UDoorInteractionActivity::OnInteractionEnd()
{
    if (Door)
    {
        Door->OperatingStates.Remove("AI");
    }
    
    UnbindEvents();

    if (!GetCharacter())
        return;
    
    float TimeRemaining;
    if (GetCharacter()->IsMontagePlayingWithTimeRemaining(InteractionAnimMontage, TimeRemaining))
    {
        UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_InteractionAnim, this, &UDoorInteractionActivity::OnInteractionAnimFinished, TimeRemaining - InteractionAnimMontage->GetDefaultBlendOutTime() - 0.1f);
    }

    #if WITH_EDITOR
    ensureAlways(TH_InteractionAnim.IsValid());
    #endif
    
    if (!TH_InteractionAnim.IsValid())
    {
        OnInteractionAnimFinished();
    }
}

void UDoorInteractionActivity::EnterReturnStage()
{
    SetLocation(OriginalLocation, true);
}

void UDoorInteractionActivity::ExitReturnStage()
{
}

void UDoorInteractionActivity::TickReturnStage(float DeltaTime, float Uptime)
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "Returning");
	
	if (HasReachedLocation(GetDestinationTolerance()))
	{
		OwningController->FinishActivity(this, true, true);
	}
}

bool UDoorInteractionActivity::CanReturn() const
{
    return bReturnToPositionAfterInteraction && bInteractionFinished;
}

void UDoorInteractionActivity::OnInteractionAnimFinished()
{
    EnableDoorInteractable();
    
    bInteractionFinished = true;
    
	if (!bReturnToPositionAfterInteraction)
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}
}

bool UDoorInteractionActivity::CheckEdgeCases()
{
    if (!IsValid(Door))
    {
        ACTIVITY_FAILED("No valid door specified");
        return false;
    }

    if (Door->IsDoorwayOnly() && !bCanInteractWithDoorway)
    {
        ACTIVITY_FAILED("Cannot interact with a doorway", true);
        return false;
    }

    return true;
}

void UDoorInteractionActivity::EnableDoorInteractable()
{
    if (Door)
    {
        Door->bCanPlayerInteract = true;
    }
}

void UDoorInteractionActivity::DisableDoorInteractable()
{
    if (!bDisablePlayerDoorInteraction)
        return;
    
    if (Door)
    {
        Door->bCanPlayerInteract = false;
    }
}

void UDoorInteractionActivity::BindEvents()
{
    UnbindEvents();

    BindEvents_Internal();
}

void UDoorInteractionActivity::UnbindEvents()
{
    UnbindEvents_Internal();
}

void UDoorInteractionActivity::OnDoorOpened()
{
    ACTIVITY_FAILED("Door has been opened whilst performing activity");
}

void UDoorInteractionActivity::OnDoorClosed()
{
}

void UDoorInteractionActivity::OnDoorBroken()
{
    ACTIVITY_FAILED("Door has been broken whilst performing activity");
}

void UDoorInteractionActivity::OnDoorMovementBlocked()
{
}

void UDoorInteractionActivity::PlayInteractionAnim(const FVector& InFocalPoint)
{
    if (GetInteractionAnimation().IsEmpty())
    {
        ACTIVITY_FAILED("No interaction animation specified");
        return;
    }

    if (GetCharacter()->GetInventoryComponent()->IsEquippingItem())
        return;
    
    if (GetCharacter()->Is3PMontagePlaying(InteractionAnimMontage))
        return;
    
    Door->OperatingStates.AddUnique("AI");
    InteractionAnimMontage = GetCharacter()->PlayMontageFromTableWithFocalPoint(GetInteractionAnimation(), InFocalPoint);
    Location = FVector::ZeroVector;
    AbortMove(true);
}

void UDoorInteractionActivity::StopInteractionAnim(const float BlendOutTime)
{
    if (Door)
    {
        Door->OperatingStates.Remove("AI");
    }
    
    UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_InteractionAnim);
    if (GetCharacter())
    {
        GetCharacter()->StopTPMontage(InteractionAnimMontage);
        GetCharacter()->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Primary, true);
    }
}

void UDoorInteractionActivity::RequestMoveToInteractLocation()
{
    if (ShouldGetInPosition())
    {
        const FVector InteractionLocation = GetInteractionLocation();
        if (InteractionLocation != FVector::ZeroVector)
        {
            SetLocation(InteractionLocation + (Door->IsPointInFrontOfDoorway(CommandLocation) ? Door->GetActorForwardVector() * FMath::Abs(GetInteractionDistance()) : Door->GetActorForwardVector() * -FMath::Abs(GetInteractionDistance())));
            DrawDebugBox(GetWorld(), Location, FVector(15.0f), FColor::Magenta, false, 0.5f, 0, 1.0f);
        }
        else
        {
            Location = FVector::ZeroVector;
        }

        ProgressState = FText::FromStringTable("SwatCommandTable", "MovingToPosition");
    }
}

void UDoorInteractionActivity::BindEvents_Internal()
{
    UnbindEvents_Internal();

    if (Door)
    {
        Door->OnDoorOpened.AddDynamic(this, &UDoorInteractionActivity::OnDoorOpened);
        Door->OnDoorClosed.AddDynamic(this, &UDoorInteractionActivity::OnDoorClosed);
        Door->OnDoorBroken.AddDynamic(this, &UDoorInteractionActivity::OnDoorBroken);
        Door->OnDoorMovementBlocked.AddDynamic(this, &UDoorInteractionActivity::OnDoorMovementBlocked);
    }
}

void UDoorInteractionActivity::UnbindEvents_Internal()
{
    if (Door)
    {
        Door->OnDoorOpened.RemoveAll(this);
        Door->OnDoorClosed.RemoveAll(this);
        Door->OnDoorBroken.RemoveAll(this);
        Door->OnDoorMovementBlocked.RemoveAll(this);
    }
}

float UDoorInteractionActivity::GetDestinationTolerance() const
{
    return 20.0f;
}

bool UDoorInteractionActivity::CanBePushed() const
{
    return GetActiveStateID() > 0;
}

bool UDoorInteractionActivity::GetOverrideMovementSpeed(float& OutMovementSpeed) const
{
    //OutMovementSpeed = 150.0f;
    return false;
}

bool UDoorInteractionActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    if (Door)
    {
        FocalPoint = Door->GetDoorMidLocation();
        return true;
    }
    
    return false;
}

bool UDoorInteractionActivity::ShouldForceStrafe() const
{
    return true;
}

bool UDoorInteractionActivity::CanFinishActivity() const
{
    // Must be force finished
    return false;
}

bool UDoorInteractionActivity::CanOverrideActivity() const
{
    return true;
}

bool UDoorInteractionActivity::CanBeOverridenBy(UBaseActivity* InOverridingActivity)
{
    if (GetActiveStateID() == 0)
        return true;
    
    if (Cast<UTeamBaseActivity>(InOverridingActivity))
        return true;

    return false;
}
