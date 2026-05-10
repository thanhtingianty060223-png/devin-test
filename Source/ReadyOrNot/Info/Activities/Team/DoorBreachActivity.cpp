// Void Interactive, 2020

#include "DoorBreachActivity.h"

#include "Characters/CyberneticController.h"

#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/ReloadSafelyActivity.h"
#include "Info/Activities/ActivityManagerTemplates.h"

#include "Actors/Door.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Actors/Items/C2Explosive.h"
#include "Actors/Items/Detonator.h"
#include "Actors/Items/BreachingShotgun.h"

#include "Components/DestructibleDoorChunkComponent.h"

#include "ReadyOrNotAIConfig.h"
#include "Actors/BaseGrenade.h"
#include "Actors/StackUpActor.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/GrenadeLauncher.h"
#include "Components/InteractableComponent.h"

UDoorBreachActivity::UDoorBreachActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "BreachingDoor");
	
	bIsProgressActivity = true;

	MaxActivityTime = 60.0f;

	MoveAcceptanceRadius = 1.0f;
	
	ActivityStateMachine->GetState("Get In Position")
						.CreateTransition("Breached", MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::IsBreachFinished), 99);

	ActivityStateMachine->GetState("Interact")
						.CreateTransition("Breached", MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::IsBreachFinished), 99)
						.RemoveTransitionByName("Return");
	
	ActivityStateMachine->AddState("Breached")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::EnterBreachedStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::TickBreachedStage))
						.BindEventExit(MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::ExitBreachedStage))
						.CreateTransition("Return", MAKE_DELEGATE_BINDING(this, &UDoorBreachActivity::CanReturn), 1);
}

void UDoorBreachActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (!Door)
	{
		ACTIVITY_FAILED("No valid door specified");
	}
}

bool UDoorBreachActivity::CanFinishActivity() const
{
	// Must be force finished
	return false;
}

bool UDoorBreachActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (!Door)
	{
		return false;
	}

	FocalPoint = Door->GetDoorway()->GetComponentLocation();
	return true;
}

bool UDoorBreachActivity::ShouldForceStrafe() const
{
	return true;
}

void UDoorBreachActivity::ResetData()
{
	Super::ResetData();

	BreachItem = nullptr;
	OnBreachFinished.Clear();
	bBreachFinished = false;
}

void UDoorBreachActivity::OnDoorOpened()
{
	// Don't fail activity if opened
}

void UDoorBreachActivity::OnDoorBroken()
{
	// Don't fail activity if broken
}

void UDoorBreachActivity::OnDoorClosed()
{
}

void UDoorBreachActivity::EnterBreachedStage()
{
	OnBreachFinished.Broadcast(this, OwningController);
	if (GetCharacter()->GetEquippedItem() == BreachItem)
	{
		EquipWeapon();
	}
	if (!bReturnToPositionAfterInteraction)
	{
		//OwningController->FinishActivity(this, true, true);
		return;
	}
	
}

void UDoorBreachActivity::ExitBreachedStage()
{
}

void UDoorBreachActivity::TickBreachedStage(float DeltaTime, float Uptime)
{
	if (Door->IsOpenBeyondIncrementThreshold() && !GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
	{
		OwningController->FinishActivity(this, true, true);
	}
}

bool UDoorBreachActivity::IsBreachFinished() const
{
	return bBreachFinished;
}

bool UDoorBreachActivity::CanReturn() const
{
	return bReturnToPositionAfterInteraction && bBreachFinished;
}

void UDoorBreachActivity::OnBreacherKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	OwningController->FinishActivity(this, true, true);
}

void UDoorBreachActivity::FinishDoorBreach()
{
	bBreachFinished = true;
}

void UDoorBreachActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	GetCharacter()->OnCharacterKilled.RemoveAll(this);
	GetCharacter()->OnCharacterKilled.AddDynamic(this, &UDoorBreachActivity::OnBreacherKilled);

	EquipWeapon();

	GetCharacter()->bDisableTurnInPlace = true;
}

void UDoorBreachActivity::FinishedActivity(bool Success)
{
	Super::FinishedActivity(Success);
	
    if (GetCharacter()->GetEquippedItem() == BreachItem)
    {
		EquipWeapon();
    }

	GetCharacter()->OnCharacterKilled.RemoveAll(this);
	
	GetCharacter()->bDisableTurnInPlace = true;
}

/////////// Kick Door Activity ///////////
UKickDoorActivity::UKickDoorActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "KickingDoor");
	
	bCanInteractWithDoorway = false;
	bReturnToPositionAfterInteraction = false;
}

bool UKickDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	//return Super::OverrideFocalPoint(FocalPoint);
	const float ForwardOffset = (Door->IsPointInFrontOfDoorway(CommandLocation) ? GetInteractionDistance() + 20.0f : -GetInteractionDistance() - 20.0f);
	const float RightOffset = Door->IsPointRightOfDoorway(OriginalLocation) ? -60.0f : 60.0f;
	FocalPoint = Door->GetDoorMidLocation() + Door->GetActorForwardVector() * ForwardOffset + Door->GetActorRightVector() * RightOffset;
	return true;
}

void UKickDoorActivity::PerformGetInPositionStage(const float DeltaTime, const float Uptime)
{
	Super::PerformGetInPositionStage(DeltaTime, Uptime);

	if (CheckEdgeCases())
	{
		if (Door->IsOpenBeyondIncrementThreshold())
		{
			OwningController->FinishActivity(this, true, true);
			return;
		}
	}
	
	ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForKick");
}

void UKickDoorActivity::EnterInteractStage()
{
	Super::EnterInteractStage();

	GetCharacter()->OnDoorKickBreach_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnDoorKickBreach_FromAnimNotify.AddDynamic(this, &UKickDoorActivity::OnDoorKicked);
	
	AbortMove();

	if (!bHasPlayedKickResponseLine)
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_BREACH_KICK);
		bHasPlayedKickResponseLine = true;
	}
}

void UKickDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	if (CheckEdgeCases())
	{
		if (Door->IsOpenBeyondIncrementThreshold() && !GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
		{
			OwningController->FinishActivity(this, true, true);
			return;
		}
		
		if (!Door->IsOpen())
		{
			if (!GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
			{
				GetCharacter()->SetCanBeDamaged(false); // to gurantee the animation doesnt get interrupted if a suspect shoots at the door damaging us
				PlayInteractionAnim(Door->GetDoorway()->GetComponentLocation());
			}
		}

		ProgressState = FText::FromStringTable("SwatCommandTable", "PerformingKick");
	}
}

void UKickDoorActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->SetCanBeDamaged(true);
}

bool UKickDoorActivity::CanInteract() const
{
	if (!Door)
		return false;
    
	if (Door->IsOpenBeyondCloseThreshold())
		return false;
	
	return Super::CanInteract() && !GetCharacter()->IsActionsLocked();
}

void UKickDoorActivity::ResetData()
{
	Super::ResetData();
	
	bHasPlayedKickResponseLine = false;
}

bool UKickDoorActivity::CanFinishActivity() const
{
	return false;
}

float UKickDoorActivity::GetDestinationTolerance() const
{
	return 20.0f;
}

FString UKickDoorActivity::GetInteractionAnimation() const
{
	const bool bInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
	const bool bRightOfDoor = Door->IsPointRightOfDoorway(OriginalLocation);
	const bool bShouldKickLeft = (bInFrontOfDoor && bRightOfDoor) || (!bInFrontOfDoor && !bRightOfDoor);
	
	return bShouldKickLeft ? "tp_swt_kickdoor_l" : "tp_swt_kickdoor_r";
}

float UKickDoorActivity::GetInteractionDistance() const
{
	return 60.0f;
}

FVector UKickDoorActivity::GetInteractionLocation() const
{
	if (Door)
	{
		float RightOffset = Door->GetDoorSize().Y;
		RightOffset += 60.0f; // additional offset
		
		if (Door->GetSubDoor())
		{
			RightOffset *= 1.5f;
		}
		
		const bool bIsInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
		if (bIsInFrontOfDoor)
		{
			if (IsLeftSideInteraction())
			{
				return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
			}
			
			return Door->GetActorLocation();
		}

		if (IsLeftSideInteraction())
		{
			return Door->GetActorLocation();
		}
		
		return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
	}
	
	return Super::GetInteractionLocation();
}

bool UKickDoorActivity::CheckEdgeCases()
{
	if (!Super::CheckEdgeCases())
		return false;

	/*
	if (Door->IsJammed()) // TODO: just kick jammed door anyway?
	{
		return false;
	}
	*/
	
	if (Door->IsMiddleChunkBroken())
	{
		ACTIVITY_FAILED("Cannot kick door");
		return false;
	}

	return true;
}

void UKickDoorActivity::OnDoorOpened()
{
	FinishDoorBreach();
}

void UKickDoorActivity::OnDoorKicked()
{
	if (CheckEdgeCases())
	{
		Door->KickDoor(GetCharacter(), true);
	}
}

/////////// C2 Door Activity ///////////
UC2DoorActivity::UC2DoorActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "C2Door");
	
	bCanInteractWithDoorway = false;
	bReturnToPositionAfterInteraction = false;

    ActivityStateMachine->GetState("Get In Position")
						.CreateTransition("Detonate C2", MAKE_DELEGATE_BINDING(this, &UC2DoorActivity::CanDetonateC2), 99);
	
	ActivityStateMachine->GetState("Interact")
						.CreateTransition("Detonate C2", MAKE_DELEGATE_BINDING(this, &UC2DoorActivity::CanDetonateC2));

	ActivityStateMachine->AddState("Detonate C2")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UC2DoorActivity::EnterDetonateC2Stage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UC2DoorActivity::PerformDetonateC2Stage))
						.CreateTransition("Breached", MAKE_DELEGATE_BINDING(this, &UC2DoorActivity::IsBreachFinished));
}

bool UC2DoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation() + Door->GetLockpickHighlight()->GetRightVector() * (Door->IsPointInFrontOfDoorway(CommandLocation) ? 25.0f : -25.0f);
	//FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation();

	/*
	if (GetActiveStateID() == 1) // Interact
	{
		FocalPoint = (GetInteractionLocation() + Door->GetActorForwardVector() * (Door->IsPointInFrontOfDoorway(CommandLocation) ?  -1000.0f : 1000.0f));
	}
	else
	{
		FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation();
	}
	*/
	
	return true;
}

bool UC2DoorActivity::ShouldGetInPosition() const
{
	if (Door->IsC2Placed())
		return false;

	return true;
}

void UC2DoorActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->StopTPMontageFromTable(GetInteractionAnimation());
	GetCharacter()->StopTPMontageFromTable("tp_swt_c2_detonate");

	GetCharacter()->OnC2Placed_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnC2Detonate_FromAnimNotify.RemoveAll(this);
	
	if (Door->GetPlacedC2())
		Door->GetPlacedC2()->C2InteractableComponent->EnableInteractable();
}

bool UC2DoorActivity::CanFinishActivity() const
{
	return false;
	//return !Door || Door->IsDoorBroken() || Door->IsHalfwayOpen() || bHasEverDetonatedC2;
}

void UC2DoorActivity::EnterInteractStage()
{
	if (CheckEdgeCases())
	{
		ProgressState = FText::FromStringTable("SwatCommandTable", "RiggingC2");

		GetCharacter()->OnC2Placed_FromAnimNotify.RemoveAll(this);
		GetCharacter()->OnC2Placed_FromAnimNotify.AddDynamic(this, &UC2DoorActivity::OnC2Placed);
	            
		GetCharacter()->OnC2Detonate_FromAnimNotify.RemoveAll(this);
		GetCharacter()->OnC2Detonate_FromAnimNotify.AddDynamic(this, &UC2DoorActivity::OnC2Detonate);
	}
}

void UC2DoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	if (CheckEdgeCases())
	{
		if (!Door->IsC2Placed() && !bHasEverDetonatedC2)
		{
			if (!GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
			{
				// failsafe
				{
					GetCharacter()->OnC2Placed_FromAnimNotify.RemoveAll(this);
					GetCharacter()->OnC2Placed_FromAnimNotify.AddDynamic(this, &UC2DoorActivity::OnC2Placed);
							
					GetCharacter()->OnC2Detonate_FromAnimNotify.RemoveAll(this);
					GetCharacter()->OnC2Detonate_FromAnimNotify.AddDynamic(this, &UC2DoorActivity::OnC2Detonate);
				}
				
				FVector FocalPoint;
				OverrideFocalPoint(FocalPoint);
				PlayInteractionAnim(FocalPoint);
			}
		}
	}
}

void UC2DoorActivity::ExitInteractStage()
{
	SetLocation(OriginalLocation, true);
}

FString UC2DoorActivity::GetInteractionAnimation() const
{
	return "tp_swt_c2_place";
}

float UC2DoorActivity::GetInteractionDistance() const
{
	return 100.0f;
}

FVector UC2DoorActivity::GetInteractionLocation() const
{
	if (Door && !Door->GetSubDoor())
	{
		return Door->GetLockpickHighlight()->GetComponentLocation() + Door->GetLockpickHighlight()->GetRightVector() * -30.0f;
	}
	
	return Super::GetInteractionLocation();
}

bool UC2DoorActivity::IsBreachFinished() const
{
	return Super::IsBreachFinished();
}

bool UC2DoorActivity::CheckEdgeCases()
{
	if (!Super::CheckEdgeCases())
		return false;

	if (GetActiveStateID() <= 1)
	{
		if (Door->IsDoorBroken())
		{
			ACTIVITY_FAILED("Door is broken", true);
			return false;
		}

		if (Door->IsHalfwayOpen())
		{
			ACTIVITY_FAILED("Door is more than half-way open", true);
			return false;
		}

		if (bHasEverDetonatedC2)
		{
			ACTIVITY_FAILED("C2 has been detonated", true);
			return false;
		}
	}
	
	return true;
}

void UC2DoorActivity::EnterDetonateC2Stage()
{
	SetLocation(OriginalLocation, true);

	ProgressState = FText::FromStringTable("SwatCommandTable", "DetonatingC2");
}

void UC2DoorActivity::PerformDetonateC2Stage(float DeltaTime, float Uptime)
{
	if (CheckEdgeCases())
	{
		if (Door->IsC2Placed() && !bHasEverDetonatedC2)
		{
			if (Location == OriginalLocation)
			{
				if (HasReachedLocation(30.0f))
				{
					if (!GetCharacter()->OnC2Detonate_FromAnimNotify.IsAlreadyBound(this, &UC2DoorActivity::OnC2Detonate))
						GetCharacter()->OnC2Detonate_FromAnimNotify.AddDynamic(this, &UC2DoorActivity::OnC2Detonate);

					Door->GetPlacedC2()->C2InteractableComponent->DisableInteractable();
					
					// blow it open broskiies
					GetCharacter()->PlayMontageFromTable("tp_swt_c2_detonate");

					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UC2DoorActivity::PlayC2BreachResponse, 0.5f);
				}
			}
			else
			{
				Location = OriginalLocation;
			}
		}
	}
}

bool UC2DoorActivity::CanInteract() const
{
	if (!Door)
		return false;

	if (bHasEverDetonatedC2)
		return false;

	if (Door->IsC2Placed())
		return false;

	return Super::CanInteract() && !Door->IsC2Placed() && !GetCharacter()->IsTableMontagePlaying("tp_swt_c2_place");
}

void UC2DoorActivity::ResetData()
{
	Super::ResetData();
	
	bHasEverDetonatedC2 = false;

	GetWorld()->GetTimerManager().ClearTimer(DelayFinishDoorUseTimer);
}

bool UC2DoorActivity::CanDetonateC2() const
{
	if (!Door)
		return false;

	if (bHasEverDetonatedC2)
		return false;
	
	return HasReachedLocation(20.0f) && Door->IsC2Placed() && !bHasEverDetonatedC2 && !GetCharacter()->IsTableMontagePlaying("tp_swt_c2_place");
}

void UC2DoorActivity::OnC2Placed()
{
	if (!GetCharacter())
		return;
	
    AC2Explosive* C2Explosive = Cast<AC2Explosive>(GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(AC2Explosive::StaticClass(), false));

	#if WITH_EDITOR
	ensure(C2Explosive != nullptr);
	#endif
	
    if (!C2Explosive)
    {
    	ACTIVITY_FAILED("No C2 explosives available on " + GetCharacter()->GetName());
        return;
    }

	if (!Door)
	{
		return;
	}
    
    if (!Door->IsC2Placed())
    {
        // Just fake the trace, we already know what door we want to place this on
        FHitResult TraceAttempt;
        TraceAttempt.TraceStart = CommandLocation;
        TraceAttempt.TraceEnd = Door->GetDoorway()->GetComponentLocation();
        TraceAttempt.Location = TraceAttempt.TraceEnd;
		// ##UE5UPGRADE## Compatibility
        TraceAttempt.HitObjectHandle = Door;
        TraceAttempt.Component = Door->GetDoorMesh();
        
        C2Explosive->LastGoodPlacement = TraceAttempt;
        C2Explosive->bIsValidPlacement = true;
        C2Explosive->CurrentActorPlacement = Door;
        C2Explosive->Server_FinishC2Placement_Implementation();

        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_C2_PLACED);
    	
		if (Door->GetPlacedC2())
			Door->GetPlacedC2()->C2InteractableComponent->DisableInteractable();
    	
		GetCharacter()->OnC2Placed_FromAnimNotify.RemoveAll(this);
    }

	float TimeRemaining;
	GetCharacter()->IsTableMontagePlayingWithTimeRemaining(GetInteractionAnimation(), TimeRemaining);

	FTimerDelegate Delegate;
	Delegate.BindWeakLambda(this, [&]
	{
		Location = OriginalLocation;
	});
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, Delegate, TimeRemaining, false);
}

void UC2DoorActivity::OnC2Detonate()
{
	bHasEverDetonatedC2 = true;

	UReadyOrNotFunctionLibrary::StartTimerForCallback(DelayFinishDoorUseTimer, this, &UC2DoorActivity::FinishDoorBreach, 1.5f);

	if (!GetCharacter())
		return;

	if (ADetonator* Detonator = Cast<ADetonator>(GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ADetonator::StaticClass(), false)))
	{
		Detonator->PlacedCharges.AddUnique(Door->GetPlacedC2());
		Detonator->Server_DetonateC2_Implementation();

		GetCharacter()->OnC2Detonate_FromAnimNotify.RemoveAll(this);
		
		// Make everyone react to the C2 detonation
		UActivityManager::IterateAllActivitiesOfType<UTeamBreachAndClearActivity>([&](UTeamBreachAndClearActivity* Activity)
		{
			if (Activity->GetStackUpDoor() == Door)
			{
				if (Activity->GetCharacter() != GetCharacter())
				{
					//BreachAndClearActivity->GetCharacter()->Multicast_Stop3PMontage_Implementation(nullptr, 0.0f);
					if (!Activity->GetCharacter()->GetEquippedItem<ABallisticsShield>())
					{
						Activity->GetCharacter()->PlayMontageFromTable("tp_swt_c2_react");
					}
				}
			}

			return true;
		});
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UC2DoorActivity::FinishDoorBreach, 0.5f);
}

void UC2DoorActivity::PlayC2BreachResponse()
{
	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_BREACH_C2);
}

/////////// Shotgun Door Activity ///////////
UShotgunDoorActivity::UShotgunDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "BreachingDoor");
	
	bCanInteractWithDoorway = false;
	bReturnToPositionAfterInteraction = true;
}

void UShotgunDoorActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->OnDoorShotgunBreach_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnDoorKickBreach_FromAnimNotify.RemoveAll(this);
}

void UShotgunDoorActivity::DestroyChunk(const int32 ChunkIndex, const float ImpulseForce, const bool bCheckSupports)
{
	if (!Door)
	{
		FinishDoorBreach();
		return;
	}

	if (Door->IsDestructible())
	{
		// Just push the door if passed in an invalid chunk index
		if (!Door->GetChunkComponents().IsValidIndex(ChunkIndex))
		{
			return;
		}

		if (UDestructibleDoorChunkComponent* Chunk = Door->GetChunkComponents()[ChunkIndex])
		{
			Chunk->OnChunkDestroyed();
                    
			if (Chunk->IsSimulatingPhysics())
			{
				const bool bInFrontOfDoor = Door->IsPointInFrontOfDoor(GetCharacter()->GetActorLocation());
				Chunk->AddImpulse(Door->GetActorForwardVector() * (bInFrontOfDoor ? -ImpulseForce : ImpulseForce));
			}
			
			if (bCheckSupports)
			{
				Door->DestroyAllChunks(Door->GetActorForwardVector(), ImpulseForce);
			}
			
			Door->BreakDoor(false, GetCharacter());
		}
	}

	if (Door->GetSubDoor())
	{
		if (Door->GetSubDoor()->IsDestructible())
		{
			// Just push the door if passed in an invalid chunk index
			if (!Door->GetSubDoor()->GetChunkComponents().IsValidIndex(ChunkIndex))
			{
				return;
			}

			if (UDestructibleDoorChunkComponent* Chunk = Door->GetSubDoor()->GetChunkComponents()[ChunkIndex])
			{
				Chunk->OnChunkDestroyed();
				
				if (Chunk->IsSimulatingPhysics())
				{
					const bool bInFrontOfDoor = Door->GetSubDoor()->IsPointInFrontOfDoor(GetCharacter()->GetActorLocation());
					Chunk->AddImpulse(Door->GetSubDoor()->GetActorForwardVector() * (bInFrontOfDoor ? -ImpulseForce : ImpulseForce));
				}
				
				if (bCheckSupports)
				{
					Door->GetSubDoor()->DestroyAllChunks(Door->GetSubDoor()->GetActorForwardVector(), ImpulseForce);
				}
				
				Door->GetSubDoor()->BreakDoor(false, GetCharacter());
			}
		}
	}
}

bool UShotgunDoorActivity::CanShotgunHinges() const
{
	if (const ABaseItem* EquippedItem = GetCharacter()->GetEquippedItem())
	{
		return EquippedItem->ContainsItemCategory(EItemCategory::IC_BreachingShotgun);
	}

	return false;
}

void UShotgunDoorActivity::ResetData()
{
	Super::ResetData();
	
	bBreachedDoor = false;
	bKickedDoor = false;
}

void UShotgunDoorActivity::EnterInteractStage()
{
	if (CheckEdgeCases())
	{
		GetCharacter()->OnDoorShotgunBreach_FromAnimNotify.RemoveAll(this);
		GetCharacter()->OnDoorShotgunBreach_FromAnimNotify.AddDynamic(this, &UShotgunDoorActivity::OnDoorShotgunned);

		GetCharacter()->OnDoorKickBreach_FromAnimNotify.RemoveAll(this);
		GetCharacter()->OnDoorKickBreach_FromAnimNotify.AddDynamic(this, &UShotgunDoorActivity::OnDoorKicked);
	}
}

void UShotgunDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	if (CheckEdgeCases())
	{
		if (!Door->IsOpen() && !bBreachedDoor)
		{
			if (!GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
			{
				FVector FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation() - Door->GetLockpickHighlight()->GetRightVector() * 20.0f;
				
				const bool bIsInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
				const bool bIsOnTacticalSideOfDoor = Door->IsPointRightOfDoorway(OriginalLocation);
				if (bIsOnTacticalSideOfDoor)
				{
					if (bIsInFrontOfDoor)
					{
						FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation() - Door->GetLockpickHighlight()->GetRightVector() * -10.0f + Door->GetActorForwardVector() * -200.0f;
					}
					else
					{
						FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation() - Door->GetLockpickHighlight()->GetRightVector() * 50.0f + Door->GetActorForwardVector() * 200.0f;
					}
				}
				
				PlayInteractionAnim(FocalPoint);
			}
		}
		
		const bool bIsShotgunBreachFinished = bKickedDoor || (Door && Door->IsHalfwayOpen());
		if (bIsShotgunBreachFinished)
			FinishDoorBreach();
	}
}

FString UShotgunDoorActivity::GetInteractionAnimation() const
{
	const bool bIsInFrontOfDoor = Door->IsActorInFrontOfDoorway(GetCharacter());
	if (bIsInFrontOfDoor)
	{
		return IsLeftSideInteraction() ? "tp_swt_shotgundoor_l" : "tp_swt_shotgundoor_r";
	}
	
	return IsLeftSideInteraction() ? "tp_swt_shotgundoor_r" : "tp_swt_shotgundoor_l";
}

float UShotgunDoorActivity::GetInteractionDistance() const
{
	return 70.0f;
}

FVector UShotgunDoorActivity::GetInteractionLocation() const
{
	if (Door)
	{
		float RightOffset = Door->GetDoorSize().Y;
		RightOffset += 40.0f; // additional offset
		
		if (Door->GetSubDoor())
		{
			RightOffset *= 1.5f;
		}
		
		if (Door->IsPointInFrontOfDoorway(CommandLocation))
		{
			if (IsLeftSideInteraction())
			{
				return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
			}
			
			return Door->GetActorLocation();
		}

		if (IsLeftSideInteraction())
		{
			return Door->GetActorLocation();
		}
		
		return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
	}
	
	return Super::GetInteractionLocation();
}

void UShotgunDoorActivity::OnDoorShotgunned()
{
	if (ABreachingShotgun* ShotgunEquipped = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass_Native<ABreachingShotgun>(ABreachingShotgun::StaticClass()))
	{
		ShotgunEquipped->bInfiniteAmmo = true;
		ShotgunEquipped->OnFireAtBulletSpawn();
		ShotgunEquipped->bInfiniteAmmo = false;
	}

	if (!Door)
	{
		FinishDoorBreach();
		return;
	}

	bBreachedDoor = true;

	if (Door->IsDestructible())
	{
		DestroyChunk(5, 2000.0f, false);
	}

	// Breaching from a specified point because the door might close back on the AI if shotgunning a second time, the door thinks the AI is behind it. Use a fixed location instead of the AI's location
	Door->BreachDoorFromPoint(GetCharacter(), CommandLocation, 10.0f);
	if (Door->GetSubDoor())
		Door->GetSubDoor()->BreachDoorFromPoint(GetCharacter(), CommandLocation, 10.0f);
}

void UShotgunDoorActivity::OnDoorKicked()
{
	if (Door)
	{
		Door->KickDoor(GetCharacter(), true, true);
	}
	
	bKickedDoor = true;
}

void UShotgunDoorActivity::OnDoorOpened()
{
}

bool UShotgunDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (Door)
	{
		// Finished shotgunning, now kicking..
		if (bBreachedDoor)
		{
			FocalPoint = Door->GetActorLocation();
		}
		else
		{
			if (Door->IsActorRightOfDoorway(GetCharacter()))
				FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation();
			else
				FocalPoint = Door->GetLockpickHighlight()->GetComponentLocation() + Door->GetActorRightVector() * 400.0f;
		}

		return true;
	}

	return false;
}

/////////// Ram Door Activity ///////////
URamDoorActivity::URamDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "RammingDoor");
	
	bCanInteractWithDoorway = false;
	bReturnToPositionAfterInteraction = true;
}

void URamDoorActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->OnDoorRamBreach_FromAnimNotify.RemoveAll(this);
}

bool URamDoorActivity::CheckEdgeCases()
{
	if (!Super::CheckEdgeCases())
		return false;

	if (!IsValid(BreachItem))
		return false;

	return true;
}

void URamDoorActivity::EnterInteractStage()
{
	if (CheckEdgeCases())
	{
		GetCharacter()->OnDoorRamBreach_FromAnimNotify.RemoveAll(this);
		GetCharacter()->OnDoorRamBreach_FromAnimNotify.AddDynamic(this, &URamDoorActivity::OnDoorRammed);
		
		AbortMove();

		GetCharacter()->GetInventoryComponent()->PutItemInHands(BreachItem, false, false);

		if (!bHasPlayedRamResponseLine)
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_BREACH_KICK);
			bHasPlayedRamResponseLine = true;
		}
	}
}

void URamDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	if (CheckEdgeCases())
	{
		/*
		if (Door->IsOpenBeyond(0.75f))
		{
			FinishDoorBreach();
			return;
		}
		*/
		
		if (!Door->IsOpen() && GetCharacter()->GetEquippedItem() == BreachItem && !BreachItem->IsPlayingDraw())
		{
			if (!GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
			{
				PlayInteractionAnim(Door->GetDoorway()->GetComponentLocation());
			}
		}

		ProgressState = FText::FromStringTable("SwatCommandTable", "Ramming");
	}
}

void URamDoorActivity::ResetData()
{
	Super::ResetData();
	
	bHasPlayedRamResponseLine = false;
}

FString URamDoorActivity::GetInteractionAnimation() const
{
	return "tp_swt_ramdoor";
}

float URamDoorActivity::GetInteractionDistance() const
{
	return 60.0f;
}

FVector URamDoorActivity::GetInteractionLocation() const
{
	if (Door)
	{
		float RightOffset = Door->GetDoorSize().Y;
		RightOffset += 50.0f; // additional offset
		
		if (Door->GetSubDoor())
		{
			RightOffset *= 2;
		}

		if (Door->IsPointInFrontOfDoorway(CommandLocation))
		{
			if (IsLeftSideInteraction())
			{
				return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
			}
			
			return Door->GetActorLocation();
		}

		if (IsLeftSideInteraction())
		{
			return Door->GetActorLocation();
		}
		
		return Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
	}

	return Super::GetInteractionLocation();
}

void URamDoorActivity::OnDoorRammed()
{
	if (CheckEdgeCases())
	{
		Door->RamDoor(GetCharacter());
		if (Door->GetSubDoor())
			Door->GetSubDoor()->RamDoor(GetCharacter());

		if (UTeamBreachAndClearActivity* Activity = OwningController->GetActivity<UTeamBreachAndClearActivity>())
		{
			const ACyberneticCharacter* Breacher = Activity->GetSharedData<FSharedBreachData>()->DoorBreacher;
			const ACyberneticCharacter* BetaMale = Activity->GetCharacterAtSquadPositionInStackUpArea(ESquadPosition::SP_Beta, Activity->GetStackUpArea());
			
			if (Breacher != BetaMale)
			{
				Activity->SwapSquadPositionWith(ESquadPosition::SP_Beta);
			}
		}
		
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UDoorBreachActivity::FinishDoorBreach, 0.25f);
	}
}

ULaunchGrenadeThroughDoorActivity::ULaunchGrenadeThroughDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "Launch40mmThroughDoor");
	bCanInteractWithDoorway = true;
	bReturnToPositionAfterInteraction = false;
	bDisablePlayerDoorInteraction = false;
}

void ULaunchGrenadeThroughDoorActivity::EnterGetInPositionStage()
{
	Super::EnterGetInPositionStage();
	
	EquipItem(EItemCategory::IC_Launcher);
}

FVector ULaunchGrenadeThroughDoorActivity::GetInteractionLocation() const
{
	// if we have a breach and clear and we're alpha, just stay put and fire from the current position
	if (const UTeamBreachAndClearActivity* Activity = OwningController->GetActivity<UTeamBreachAndClearActivity>())
	{
		if (Activity->OccupiedStackUpActor && Activity->OverrideSquadPosition == ESquadPosition::SP_Alpha)
		{
			return Activity->OccupiedStackUpActor->GetActorLocation();
		}
	}
	
	const ADoor* ClosestDoor = Door;
	if (Door->GetSubDoor())
	{
		const float A = FVector::Distance(Door->GetActorLocation(), GetCharacter()->GetNavAgentLocation());
		const float B = FVector::Distance(Door->GetSubDoor()->GetActorLocation(), GetCharacter()->GetNavAgentLocation());

		if (B < A)
		{
			ClosestDoor = Door->GetSubDoor();
		}
	}

	/*
	FTransform Transform;
	Transform.SetLocation(ClosestDoor->GetDoorMidLocation());
	Transform.SetRotation(ClosestDoor->GetActorRotation().Quaternion());

	const float DoorWidth = ClosestDoor->GetDoorSize().Y + 5.0f;
	
	const FVector Extent = FVector(300.0f, DoorWidth, 120.0f);
	
	const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->GetActorLocation(), Transform, Extent);

	// dont reposition if already inside door threshold
	if (bInsideDoorThreshold)
	{
		DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 1.0f);
		return FVector::ZeroVector;
	}
	*/
	
	constexpr float RightOffset = 30.0f;

	const FVector BaseLocation = ClosestDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation());
	
	return BaseLocation + ClosestDoor->GetActorRightVector() * (ClosestDoor->IsActorRightOfDoorway(GetCharacter()) ? RightOffset : -RightOffset);
}

float ULaunchGrenadeThroughDoorActivity::GetInteractionDistance() const
{
	// if we have a breach and clear and we're alpha, just stay put and fire from the current position
	if (const UTeamBreachAndClearActivity* Activity = OwningController->GetActivity<UTeamBreachAndClearActivity>())
	{
		if (Activity->OccupiedStackUpActor && Activity->OverrideSquadPosition == ESquadPosition::SP_Alpha)
		{
			return 0.0f;
		}
	}
	
	return 180.0f;
}

bool ULaunchGrenadeThroughDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	const ADoor* ClosestDoor = Door;
	if (Door->GetSubDoor())
	{
		const float A = FVector::Distance(Door->GetActorLocation(), GetCharacter()->GetNavAgentLocation());
		const float B = FVector::Distance(Door->GetSubDoor()->GetActorLocation(), GetCharacter()->GetNavAgentLocation());

		if (B < A)
		{
			ClosestDoor = Door->GetSubDoor();
		}
	}
	
	FocalPoint = ClosestDoor->CalculateClosestPoint(GetCharacter()->GetActorLocation());

	if (ClosestDoor->IsActorRightOfDoorway(GetCharacter()))
	{
		FocalPoint += ClosestDoor->GetActorRightVector() * -50.0f;
	}
	else
	{
		FocalPoint += ClosestDoor->GetActorRightVector() * 50.0f;
	}

	if (ClosestDoor->IsPointInFrontOfDoorway(CommandLocation))
	{
		FocalPoint += ClosestDoor->GetActorForwardVector() * -200.0f;
	}
	else
	{
		FocalPoint += ClosestDoor->GetActorForwardVector() * 200.0f;
	}

	FocalPoint.Z += 50.0f;
	
	return true;
}

void ULaunchGrenadeThroughDoorActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	EquipWeapon();
}

void ULaunchGrenadeThroughDoorActivity::EnterInteractStage()
{
	EquipItem(EItemCategory::IC_Launcher);
	OnLauncherReady.Broadcast();
}

void ULaunchGrenadeThroughDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	Super::PerformInteractStage(DeltaTime, Uptime);

	if (Door->IsOpenBeyond(0.4f) && !bFired && HasReachedLocation(GetDestinationTolerance()) && !GetCharacter()->IsDrawingWeapon())
	{
		TimeDoorOpen += DeltaTime;

		if (TimeDoorOpen > 0.45f && !GetCharacter()->IsAny3PMontageActive())
		{
			if (AGrenadeLauncher* Launcher = GetCharacter()->GetEquippedItem<AGrenadeLauncher>())
			{
				const FVector& FireDirection = Launcher->GetBulletSpawn()->GetForwardVector();
				FVector FocalPoint = FVector::ZeroVector;
				OverrideFocalPoint(FocalPoint);
				if (FocalPoint != FVector::ZeroVector)
				{
					const FVector& DirectionToFocalPoint = (FocalPoint - Launcher->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
					const float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);
					
					const bool bIsFocusedOnFocalPoint = TargetRotationDelta > 0.9f;

					if (bIsFocusedOnFocalPoint)
					{
						Launcher->bAIFireAtBulletSpawn = true;
						Launcher->OnFireAtBulletSpawn();
						bFired = true;
						EquipWeapon();
						Launcher->bAIFireAtBulletSpawn = false;
					}
				}
			}
		}
	}

	if (bFired)
	{
		if (FMath::IsNearlyZero(GetCharacter()->QuickLeanAmount, 1.0f))
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ULaunchGrenadeThroughDoorActivity::FinishDoorBreach, 0.25f);
		}
	}
}

bool ULaunchGrenadeThroughDoorActivity::GetLeanOverride(float& LeanOverride) const
{
	if (bFired)
	{
		return false;
	}

	if (Door->IsPointInFrontOfDoorway(CommandLocation))
	{
		LeanOverride = Door->IsActorRightOfDoorway(GetCharacter()) ? 1.0f : -1.0f;
	}
	else
	{
		LeanOverride = Door->IsActorRightOfDoorway(GetCharacter()) ? -1.0f : 1.0f;
	}
	
	return true;
}

bool ULaunchGrenadeThroughDoorActivity::GetLowReadyOverride(bool& bLowReady) const
{
	bLowReady = false;
	return true;
}

void ULaunchGrenadeThroughDoorActivity::ResetData()
{
	Super::ResetData();
	
	TimeDoorOpen = 0.0f;
	bFired = false;
}

/////////// Throw Item Through Door Activity ///////////
UThrowItemThroughDoorActivity::UThrowItemThroughDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "ThrowItemThroughDoor");
	bCanInteractWithDoorway = true;
	bReturnToPositionAfterInteraction = false;
	bDisablePlayerDoorInteraction = false;
}

void UThrowItemThroughDoorActivity::CalculateThrowTransform(const FTransform& InSocketTransform, FVector& ThrowLocation, FVector& ThrowDirection)
{
	if (!Door)
		return;

	const FVector DoorForwardDirection_Relative = Door->IsPointInFrontOfDoorway(GetCharacter()->GetActorLocation()) ? -Door->GetActorForwardVector() : Door->GetActorForwardVector();

	if (Door->IsDoorwayOnly())
	{
		//DrawDebugSphere(GetWorld(), ClosestPoint, 10.0f, 4, FColor::Yellow, false, 5.0f, 0, 2.0f);

		//ThrowLocation = ClosestPoint + DoorForwardDirection_Relative * 150.0f;
		
		EDoorRoomPosition RoomPosition;
		if (Door->IsActorInFrontOfDoorway(GetCharacter()))
		{
			RoomPosition = Door->FrontRoomPosition;
		}
		else
		{
			RoomPosition = Door->BackRoomPosition;
		}

		if (RoomPosition == EDoorRoomPosition::CornerRight)
		{
			if (Door->IsActorInFrontOfDoorway(GetCharacter()))
				ThrowDirection = Door->GetActorRightVector();
			else
				ThrowDirection = -Door->GetActorRightVector();
		}
		else if (RoomPosition == EDoorRoomPosition::CornerLeft)
		{
			if (Door->IsActorInFrontOfDoorway(GetCharacter()))
				ThrowDirection = -Door->GetActorRightVector();
			else
				ThrowDirection = Door->GetActorRightVector();
		}

		float Offset = 40.0f;
		if (Door->GetDoorSize().Y > 80.0f)
			Offset = Door->GetDoorSize().Y;
		
		ThrowLocation = Door->GetDoorMidLocation() + ThrowDirection * Offset + DoorForwardDirection_Relative * 100.0f;
		ThrowLocation.Z = Door->GetDoorMidLocation().Z;
		return;
	}
	
	float Angle = Door->GetOpenAmount();
	bool bOpenTowardsUs = false;
	if (Door->IsActorInFrontOfDoorway(GetCharacter()))
	{
		if (Door->IsOpen_Backward())
			bOpenTowardsUs = true;
	}
	else
	{
		if (Door->IsOpen_Forward())
			bOpenTowardsUs = true;
	}
	
	if (bOpenTowardsUs)
		Angle = -Angle;
	
	const FVector ModifiedDoorForwardDirection = Door->IsDoorBroken() ? DoorForwardDirection_Relative : Door->GetActorRightVector().RotateAngleAxis(Angle, FVector::UpVector);
	
	//DrawDebugLine(MeshComp->GetWorld(), Door->GetDoorway()->GetComponentLocation(), Door->GetDoorway()->GetComponentLocation() + ModifiedDoorForwardDirection * 100.0f, FColor::Red, false, 10.0f, 0, 1.5f);

	// Find where the throwing hand is relative to the door, and use that point instead of the doorway
	const FVector ClosestPoint = Door->CalculateClosestPoint(InSocketTransform.GetLocation());
	
	//DrawDebugSphere(GetWorld(), ClosestPoint, 10.0f, 4, FColor::Yellow, false, 5.0f, 0, 2.0f);

	ThrowLocation = ClosestPoint + DoorForwardDirection_Relative * 100.0f;
	
	ThrowDirection = (Door->GetDoorMidLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();
	ThrowDirection += ModifiedDoorForwardDirection;
	ThrowLocation.Z = Door->GetDoorMidLocation().Z;
	ThrowDirection.Normalize();
}

void UThrowItemThroughDoorActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->PendingThrownItem = nullptr;
}

bool UThrowItemThroughDoorActivity::ShouldGetInPosition() const
{
	return false;
}

void UThrowItemThroughDoorActivity::TickBreachedStage(float DeltaTime, float Uptime)
{
	if (!GetCharacter()->IsTableMontagePlaying(GetInteractionAnimation()))
	{
		OwningController->FinishActivity(this, true, true);
	}
}

void UThrowItemThroughDoorActivity::TryThrowItem()
{
	if (!ThrownItem)
	{
		ACTIVITY_FAILED("No thrown item present");
		return;
	}
	
	if (!GetCharacter()->Is3PMontagePlaying(InteractionAnimMontage))
	{
		PlayInteractionAnim(Door->GetDoorway()->GetComponentLocation());
	}
	else
	{
		if (InteractionAnimMontage)
		{
			if (bWaitBeforeThrow)
			{
				if (GetCharacter()->GetMesh()->GetAnimInstance()->Montage_GetCurrentSection() != "Throw")
				{
					GetCharacter()->GetMesh()->GetAnimInstance()->Montage_JumpToSection("Throw", InteractionAnimMontage);
				}
			}
		}
	}

	bItemThrown = GetCharacter()->GetMesh()->GetAnimInstance()->Montage_GetCurrentSection() == "Throw";

	if (bItemThrown)
	{
		if (ThrownItem->ItemCategories.Contains(EItemCategory::IC_CSGas))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_CS_GAS);
		}
		else if (ThrownItem->ItemCategories.Contains(EItemCategory::IC_Stingball))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_STINGER);
		}
		else if (ThrownItem->ItemCategories.Contains(EItemCategory::IC_Flashbang))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_FLASHBANG);
		}

		ProgressState = FText::FromStringTable("SwatCommandTable", "Throwing");
	}
}

bool UThrowItemThroughDoorActivity::CanThrowNow() const
{
	return Door->IsHalfwayOpen() &&
			!GetCharacter()->IsTableMontagePlaying("tp_swt_c2_react") &&
			!GetCharacter()->IsTableMontagePlaying("tp_swt_c2_detonate") &&	
			!bItemThrown;
}

void UThrowItemThroughDoorActivity::ResetData()
{
	Super::ResetData();
	
	ThrowItemClass = nullptr;

	OnThrowReady.Clear();
	OnThrowingItem.Clear();

	bWaitBeforeThrow = false;
	
	ThrownItem = nullptr;

	bItemThrown = false;
}

void UThrowItemThroughDoorActivity::PerformGetInPositionStage(float DeltaTime, const float Uptime)
{
	if (CheckEdgeCases())
	{
		float TimeRemaining;
		if (!GetCharacter()->IsMontagePlayingWithTimeRemaining(InteractionAnimMontage, TimeRemaining))
		{
			PlayInteractionAnim(Door->GetDoorway()->GetComponentLocation());
		}
		else
		{
			if (InteractionAnimMontage)
			{
				//ULog::Info(GetCharacter()->GetMesh()->GetAnimInstance()->Montage_GetCurrentSection().ToString());

				float LoopPosition = 1.0f;
				const int32 SectionID = InteractionAnimMontage->GetSectionIndex("Loop");

				if (InteractionAnimMontage->IsValidSectionIndex(SectionID))
				{
					const FCompositeSection& CurSection = InteractionAnimMontage->GetAnimCompositeSection(SectionID);
					LoopPosition = CurSection.GetTime();
				}

				if (Uptime > LoopPosition * 0.3f)
				{
					OnReady();
				}
			}

			// Failsafe
			if (TimeRemaining <= 0.0f)
			{
				OnReady();
			}
		}
	}
}

void UThrowItemThroughDoorActivity::OnReady()
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "Ready");

	OnThrowReady.Broadcast();
}

void UThrowItemThroughDoorActivity::EnterInteractStage()
{
	if (CheckEdgeCases())
	{
		ThrownItem = GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ThrowItemClass, false);

		#if WITH_EDITOR
		ensureAlways(ThrownItem != nullptr);
		#endif

		if (!ThrownItem)
		{
			ACTIVITY_FAILED("No thrown item available");
			return;
		}
		
		GetCharacter()->PendingThrownItem = ThrownItem;

		if (CanThrowNow())
		{
			TryThrowItem();
		}
		
		OnThrowingItem.Broadcast();
	}
}

void UThrowItemThroughDoorActivity::PerformInteractStage(float DeltaTime, const float Uptime)
{
	if (CheckEdgeCases())
	{
		if (CanThrowNow())
		{
			TryThrowItem();
		}
	}
}

bool UThrowItemThroughDoorActivity::CanInteract() const
{
	return Door->IsHalfwayOpen() &&
			!GetCharacter()->IsTableMontagePlaying("tp_swt_c2_react") &&
			!GetCharacter()->IsTableMontagePlaying("tp_swt_c2_detonate");
}

/////////// Throw Grenade Through Door Activity ///////////
UThrowGrenadeThroughDoorActivity::UThrowGrenadeThroughDoorActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "ThrowGrenade");
	bCanInteractWithDoorway = true;
	bReturnToPositionAfterInteraction = false;
	bDisablePlayerDoorInteraction = false;
}

void UThrowGrenadeThroughDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
	Super::PerformInteractStage(DeltaTime, Uptime);

	if (const ABaseGrenade* Grenade = Cast<ABaseGrenade>(ThrownItem))
	{
		if (Grenade->bHasEverDetonated)
		{
			FinishDoorBreach();
		}
	}
}

void UThrowGrenadeThroughDoorActivity::FinishedActivity(bool Success)
{
	Super::FinishedActivity(Success);

	GetCharacter()->StopTPAnimMontage(GetCharacter()->GetMontageFromTable("tp_swt_throwgrenade_l_throw"));
	GetCharacter()->StopTPAnimMontage(GetCharacter()->GetMontageFromTable("tp_swt_throwgrenade_r_throw"));
}

FString UThrowGrenadeThroughDoorActivity::GetInteractionAnimation() const
{
	if (IsLeftSideInteraction())
	{
		if (bWaitBeforeThrow)
			return "tp_swt_throwgrenade_r_throw";

		return "tp_swt_throwgrenade_r_throw_noloop";
	}
	
	if (bWaitBeforeThrow)
		return "tp_swt_throwgrenade_l_throw";

	return "tp_swt_throwgrenade_l_throw_noloop";
}

/////////// Custom Door Activity ///////////
UCustomDoorBreachActivity::UCustomDoorBreachActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "CustomDoorBreach");
	bReturnToPositionAfterInteraction = true;
}

void UCustomDoorBreachActivity::PerformActivity(const float DeltaTime)
{
	TickBreachDoor(DeltaTime);
}
