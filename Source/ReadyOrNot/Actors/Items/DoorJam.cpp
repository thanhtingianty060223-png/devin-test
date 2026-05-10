// Copyright Void Interactive, 2021

#include "DoorJam.h"

#include "Actors/Door.h"
#include "Actors/Items/Multitool.h"

#include "Components/InteractableComponent.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

ADoorJam::ADoorJam()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	InteractableComponent->ShowPromptAtDistance = 200.0f;
	InteractableComponent->ActionSlot2.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipMultitool"));
	InteractableComponent->ActionSlot2.bCheckForDisallowedItems = false;
	InteractableComponent->ActionSlot3.Init("Fire", IE_Repeat, FText::FromStringTable("ActionPromptTable", "RemoveWedge"));
	InteractableComponent->ActionSlot4.bUseCustomActionText = true;
	InteractableComponent->ActionSlot4.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "RemovingWedge");
	InteractableComponent->ActionSlot4.bAnimate = true;
	InteractableComponent->ActionSlot4.bLoopAnimation = true;
	InteractableComponent->SetupAttachment(ItemMesh);
}

void ADoorJam::BeginPlay()
{
	Super::BeginPlay();
	ItemClass = EItemClass::IC_TacticalDevice;
	ItemCategories.AddUnique(EItemCategory::IC_TacticalDevice);
}

void ADoorJam::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorJam, bSet);
	DOREPLIFETIME(ADoorJam, PendingPlacement);
	DOREPLIFETIME(ADoorJam, JammedDoor);
	DOREPLIFETIME(ADoorJam, PlacedBy);
}

void ADoorJam::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InteractableComponent)
	{
		InteractableComponent->bEnabled = bSet || bIsEvidence;
		InteractableComponent->bShowIconWhenActionsLocked = true;
		InteractableComponent->UseActor = this;
		InteractableComponent->bImprintIconOnHUDUponInteraction = !bSet;
		InteractableComponent->bHideUponInteraction = !bSet;
		
		if (APlayerCharacter* PC = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			SetActorTickInterval(0.0167f); // In BaseItem, this is set to 1.0f because this does not have an owner character. We need to tick every frame for the interactable component

			// Removing wedge progress
			if (const AMultitool* Multitool = Cast<AMultitool>(PC->GetEquippedItem()))
			{
				InteractableComponent->SetAnimatedIconName(Multitool->GetCurrentOperatingTime() > 0.0f ? "Empty" : "Remove Wedge");
				InteractableComponent->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Multitool->GetMaxOperatingTime()), FVector2D(0.0f, 1.0f), Multitool->GetCurrentOperatingTime());
				InteractableComponent->ActionSlot4.bCondition = InteractableComponent->CurrentProgress > 0.0f;
			}
			else
			{
				InteractableComponent->SetAnimatedIconName(bSet ? "Remove Wedge" : "Empty");
				InteractableComponent->CurrentProgress = 0.0f;
				InteractableComponent->ActionSlot4.bCondition = false;
			}
			
			InteractableComponent->ActionSlot1.bCondition = CanShowActionSlot1_Implementation(PC);
			InteractableComponent->ActionSlot2.bUseCustomActionText = false;
			InteractableComponent->ActionSlot2.bCondition = CanEquipMultitool(PC);
			InteractableComponent->ActionSlot3.bCondition = CanRemoveWedge(PC) && InteractableComponent->CurrentProgress <= 0.0f;
		}
	}
}

void ADoorJam::JamDoor(ADoor* Door)
{
	if (!Door)
		return;
	
	if (Door->IsJammed() || bSet || bDeploying)
		return;

	bDeploying = true;
	
	Server_StartDoorjamPlacement(Door);
	Super::OnItemPrimaryUse();
}

void ADoorJam::OnRep_DoorjamSet()
{
	AReadyOrNotCharacter* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar)
		return;
	
	if (GetOwnerPlayerCharacter() && GetOwnerPlayerCharacter()->IsLocallyControlled())
	{
		// we update the PiP visibility here so wedges don't go invisible
		GetOwnerPlayerCharacter()->UpdatePictureInPictureVisibility();
	}

	if (PendingPlacement)
	{
		DisableWeaponFovShader();

		bDeploying = false;
		bInInventory = false;

		AttachToComponent(PendingPlacement->GetWedgeInteractableComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			
		ItemMesh->SetSkeletalMesh(PlacedMesh);
	
		SetActorLocation(FVector(PendingPlacement->GetWedgeLocation().X, PendingPlacement->GetWedgeLocation().Y, PendingPlacement->GetWedgeLocation().Z - 5.0f));

		if (PlacedBy && PlacedBy->IsPlayerControlled())
		{
			PlacedBy->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Primary, true);
		}
		
		SetActorTickEnabled(true);

		SetItemVisibility(true);
	}
}

bool ADoorJam::CanEquip(AReadyOrNotCharacter* ToCharacter) const
{
	if (ToCharacter == PlacedBy || PlacedBy == nullptr)
		return true;

	return false;
}

void ADoorJam::Reset()
{
	Super::Reset();

	if (APlayerCharacter* PC = Cast<APlayerCharacter>(GetOwner()))
	{
		Execute_Server_FinishedUsingMultitool(this, PC);
		Server_FinishedUsingMultitool_Implementation(PC);
	}

	Destroy();
}

void ADoorJam::Server_StartDoorjamPlacement_Implementation(class ADoor* TargetDoor)
{
	APlayerCharacter* pc = GetOwnerPlayerCharacter();
	if (!pc)
		return;
	
	if (TargetDoor)
	{
		bDeploying = true;
		PendingPlacement = TargetDoor;
		
		Multicast_StartPlacement();

		pc->LockAllActions();
	}
}

void ADoorJam::Server_FinishDoorjamPlacement_Implementation(class ADoor* TargetDoor)
{
	AReadyOrNotCharacter* pc = GetOwnerCharacter();
	if (!pc)
		return;
	
	if (TargetDoor && !bPlacementCanceled)
	{
		bDeploying = false;
		PlacedBy = pc;
		JammedDoor = TargetDoor;
		bSet = true;
		bInInventory = false;
		PendingPlacement = TargetDoor;
		JammedDoor->OperatingStates.Remove("Player");

		TargetDoor->AttachWedge(this);
		TargetDoor->bDoorJammed = true;
		
		if (TargetDoor->GetSubDoor())
			TargetDoor->GetSubDoor()->bDoorJammed = true;

		// order hre is important
		pc->GetInventoryComponent()->RemoveInventoryItem(this, false);
		OnRep_DoorjamSet();
		pc->UnlockAllActions();
	}
}

void ADoorJam::Multicast_StartPlacement_Implementation()
{
	if (AnimationData)
	{
		PlayItemAnimation(AnimationData->DryFire);
	}
}

void ADoorJam::StunnedWhileEquipped_Implementation()
{
	if (!bDeploying)
	{
		// act normally while not deploying
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}

	if (HasAuthority())
	{
		pc->UnlockAllActions(); // unrestrict the movement
		bPlacementCanceled = true;
	}
}

// ICanUseMultitoolOn implementation
//////////////////////////////////////
EMultitoolFunctions ADoorJam::GetMultitoolUseType_Implementation()
{
	return EMultitoolFunctions::MF_Knife;
}

float ADoorJam::GetMultitoolUseTime_Implementation()
{
	return WedgeRemovalTime;
}

void ADoorJam::Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner)
{
	if (JammedDoor)
	{
		JammedDoor->bDoorJammed = false;
		JammedDoor->OperatingStates.Remove("Player");

		if (JammedDoor->GetSubDoor())
		{
			JammedDoor->GetSubDoor()->bDoorJammed = false;
		}
	}

	if (ToolOwner)
		ToolOwner->GetInventoryComponent()->AddInventoryItem(this);

	ResetWedgeState();
}

//////////////////////////////////////
// IUseability implementation
//////////////////////////////////////
void ADoorJam::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (bSet)
		{
			InteractCharacter->Server_EquipMultitool_Implementation(EMultitoolFunctions::MF_Knife);
			return;
		}
	}

	Super::Interact_Implementation(InteractInstigator, InInteractableComponent);
}

void ADoorJam::Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->StartUsingMultitool(this);
	}
}

void ADoorJam::EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->StopUsingMultitool(this);
	}
}

void ADoorJam::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	EndFire_Implementation(InteractInstigator, InInteractableComponent);
}

FName ADoorJam::DetermineAnimatedIcon_Implementation() const
{	
	if (bSet)
	{
		if (APlayerCharacter* PC = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			if (AMultitool* Multitool = Cast<AMultitool>(PC->GetEquippedItem()))
			{
				return Multitool->GetCurrentOperatingTime() > 0.0f ? "Empty" : CanEquipMultitool(PC) ? "EquipMultitool" : CanRemoveWedge(PC) ? "RemoveWedge" : "";
			}
		}

		return "RemoveWedge";
	}
	
	return Super::DetermineAnimatedIcon_Implementation();
}

bool ADoorJam::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (Hit.GetActor())
	{
		return !Hit.GetActor()->IsA(APawn::StaticClass());
	}
	
	return true;
}

//////////////////////////////////////

bool ADoorJam::CanEquipMultitool(APlayerCharacter* PlayerCharacter) const
{
	return !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) && bSet;
}

bool ADoorJam::CanRemoveWedge(APlayerCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) && bSet;
}

bool ADoorJam::CanShowActionSlot1_Implementation(AReadyOrNotCharacter* PC)
{
	return Super::CanShowActionSlot1_Implementation(PC) && !bSet;
}

void ADoorJam::ResetWedgeState()
{
	bDeploying = false;
	PlacedBy = nullptr;
	bSet = false;
	bInInventory = true;
	PendingPlacement = nullptr;
	bPlacementCanceled = false;

	if (IsValid(JammedDoor))
	{
		JammedDoor->OperatingStates.Remove("Player");
		JammedDoor->AttachedWedge = nullptr;
	}
	
	JammedDoor = nullptr;	
}
