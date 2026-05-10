// Copyright Void Interactive, 2017

#include "C2Explosive.h"

#include "Actors/Door.h"

#include "Actors/Gameplay/PlacedC2Explosive.h"

#include "Actors/Items/Detonator.h"

#include "Components/InteractableComponent.h"
#include "Subsystems/AchievementSubsystem.h"

AC2Explosive::AC2Explosive()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	// Force initialization of FMod
	IFMODStudioModule::Get();
	
	// Removed static to prevent access to FMod before FMod is initialized on XBoxOne
	ConstructorHelpers::FClassFinder<APlacedC2Explosive> C2ExplosiveClassObj(TEXT("Blueprint'/Game/Blueprints/Items/WeaponsRevised/Device_C2_Placed.Device_C2_Placed_C'"));
	PlacedC2Class = C2ExplosiveClassObj.Class;
}

void AC2Explosive::BeginPlay()
{
	Super::BeginPlay();
	
	ItemClass = EItemClass::IC_TacticalDevice;
	ItemCategories.AddUnique(EItemCategory::IC_TacticalDevice);
}

void AC2Explosive::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AC2Explosive, LastPlacedC2Explosive);
	DOREPLIFETIME(AC2Explosive, CurrentActorPlacement);
}

void AC2Explosive::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetOwnerPlayerCharacter())
	{
		return;
	}
	
	if (!IsEquipped())
	{
		// DON'T tick if we aren't equipped, this will just wind up being expensive.
		return;
	}

	if (IsBlockingAnimationPlaying() || !AnimationData || CurrentActorPlacement)
	{
		return;
	}

	// Only tick this if we are the local client
	if (IsLocallyControlled() || HasAuthority())
	{
		LastGoodPlacement = GetOwnerPlayerCharacter()->GetHitFromCamera(110.0f, {ECC_DOOR, ECC_WorldStatic, ECC_WorldDynamic});
		
		bool bValidHit = false;
		if(LastGoodPlacement.GetActor() && LastGoodPlacement.GetActor()->Implements<UCanPlaceC2On>())
		{
			ADoor* Door = Cast<ADoor>(LastGoodPlacement.GetActor());
			// If we're a door, we need to check if the door has a handle, and already doesn't have a C2 on it.
			if(Door && !Door->IsC2Placed() && !Door->IsHandleBroken() && Door->GetC2InteractableComponent() == GetOwnerPlayerCharacter()->LastInteractableComponent)
			{
				bValidHit = true;
			}
			else
			{
				bValidHit = false;
			}
		}
		else if(GetOwnerPlayerCharacter()->LastInteractableComponent && GetOwnerPlayerCharacter()->LastInteractableComponent->UseActor->Implements<UCanPlaceC2On>())
		{
			ADoor* Door = Cast<ADoor>(GetOwnerPlayerCharacter()->LastInteractableComponent->UseActor);
			// If we're a door, we need to check if the door has a handle, and already doesn't have a C2 on it.
			if(Door && !Door->IsC2Placed() && !Door->IsHandleBroken() && Door->GetC2InteractableComponent() == GetOwnerPlayerCharacter()->LastInteractableComponent)
			{
				bValidHit = true;
			}
			else
			{
				bValidHit = false;
			}
		}
		
		
		if (bValidHit)
		{
			if (!bIsValidPlacement && GetOwnerPlayerCharacter()->IsLocallyControlled())
			{
				PlayItemAnimation(AnimationData->Charge_Valid_Plant_Start);
				bIsValidPlacement = true;
			}
		}
		else
		{
			if (bIsValidPlacement && GetOwnerPlayerCharacter()->IsLocallyControlled())
			{
				PlayItemAnimation(AnimationData->Charge_Valid_Plant_End);
				bIsValidPlacement = false;
			}
		}
	}
}

void AC2Explosive::Server_StartC2Placement_Implementation(AActor* Actor)
{
	if (!Actor)
		return;

	if (!Actor->Implements<UCanPlaceC2On>())
		return;

	CurrentActorPlacement = Actor;

	ICanPlaceC2On::Execute_C2StartPlacement(Actor, this);

	// Fire off the animation
	Multicast_StartPlaceC2Explosive();
	Multicast_StartPlaceC2Explosive_Implementation();

	// Root the player in place
	if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
	{
		pc->LockMovement();
		pc->Server_PlayPVPSpeech("PlacingC2", pc->GetTeam());
	}
}

void AC2Explosive::OnRep_LastPlacedC2Explosive()
{
	if (LastPlacedC2Explosive)
	{
		LastPlacedC2Explosive->AttachToComponent(LastGoodPlacement.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, LastGoodPlacement.BoneName);
	}
}

void AC2Explosive::Multicast_StartPlaceC2Explosive_Implementation()
{
	if (AnimationData)
	{
		PlayItemAnimation(AnimationData->PlantCharge);
	}
}

void AC2Explosive::EquipDetonator(const bool bFromExplosives) const
{
	if (const APlayerCharacter* PC = GetOwnerPlayerCharacter())
	{
		PC->GetInventoryComponent()->EquipItemOfType(EItemCategory::IC_Detonator, bFromExplosives);
	}
}

void AC2Explosive::Server_FinishC2Placement_Implementation()
{
	if (!GetOwnerCharacter())
		return;

	if (APlayerCharacter* pc = GetOwnerPlayerCharacter())
	{
		pc->UnlockAllActions();
		pc->GetInventoryComponent()->RemoveInventoryItem(this, false);
	}
	
	if (!CurrentActorPlacement)
		return;
	
	const FVector PlacementLocation = ICanPlaceC2On::Execute_GetPlacementLocation(CurrentActorPlacement, LastGoodPlacement);
	const FRotator PlacementRotation = ICanPlaceC2On::Execute_GetPlacementRotation(CurrentActorPlacement, LastGoodPlacement);

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(PlacementLocation);
	SpawnTransform.SetRotation(PlacementRotation.Quaternion());
	SpawnTransform.SetScale3D(FVector::OneVector);
	
	// Spawn the placed C2
	if (APlacedC2Explosive* PlacedExplosive = GetWorld()->SpawnActorDeferred<APlacedC2Explosive>(PlacedC2Class, SpawnTransform, GetOwner(), GetOwnerCharacter()))
	{
		PlacedExplosive->ConnectedC2Explosive = this;
		PlacedExplosive->PlacedByController = GetOwnerCharacter()->GetController();
		PlacedExplosive->TargetItem = CurrentActorPlacement;
		PlacedExplosive->PlacementHit = LastGoodPlacement;
		PlacedExplosive->ItemInventoryClass = GetClass();

		PlacedExplosive->FinishSpawning(SpawnTransform);
		PlacedExplosive->AttachToComponent(LastGoodPlacement.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, LastGoodPlacement.BoneName);

		LastPlacedC2Explosive = PlacedExplosive;

		// Hide the FP model
		Client_SetFPModelVisibility(false);

		if (APlayerCharacter* pc = GetOwnerPlayerCharacter())
		{
			// Add this item to the detonator's list of charges
			if (ADetonator* Detonator = Cast<ADetonator>(pc->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Detonator)))
			{
				Detonator->PlacedCharges.Add(PlacedExplosive);
				Detonator->PlacedChargesCount++;
			}
		}
		
		Client_C2PlacementFinished();

		ICanPlaceC2On::Execute_C2StopPlacement(CurrentActorPlacement, this);
	}
	
	// We're done with this remove it shortly but not instantly as we're still waiting for notifies to complete!
	if (!GIsAutomationTesting)
		SetLifeSpan(3.0f);
}

void AC2Explosive::Client_C2PlacementFinished_Implementation()
{
	EquipDetonator(true);
}

bool AC2Explosive::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (!GetOwnerPlayerCharacter() || !AnimationData)
	{
		return false;
	}

	// If planting, then we are blocking
	if (GetOwnerPlayerCharacter()->Is1PMontagePlaying(AnimationData->PlantCharge.Body_FP) || GetOwnerPlayerCharacter()->Is1PMontagePlaying(AnimationData->PlantCharge.Body_TP))
	{
		return true;
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}

bool AC2Explosive::CanEquip(AReadyOrNotCharacter* ToCharacter) const
{
	bool bIsPlaced = false;

	if (ADetonator* Detonator = Cast<ADetonator>(ToCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Detonator)))
	{
		Detonator->PlacedCharges.Remove(nullptr);
		bIsPlaced = Detonator->PlacedCharges.Contains(LastPlacedC2Explosive);
	}
	
	if (IsDepleted() || CurrentActorPlacement != nullptr || bIsPlaced)
		return false;
	
	return Super::CanEquip(ToCharacter);
}

void AC2Explosive::OnItemPrimaryUse()
{
	if (IsBlockingAnimationPlaying() || !bIsValidPlacement)
	{
		return;
	}
	
	if (GetOwnerPlayerCharacter())
	{
		if (GetOwnerPlayerCharacter()->LastInteractableComponent && GetOwnerPlayerCharacter()->LastInteractableComponent->IsFocused())
		{
			if (GetOwnerPlayerCharacter()->LastInteractableComponent->UseActor == LastGoodPlacement.GetActor())
			{

				if(ADoor* Door = Cast<ADoor>(CurrentActorPlacement))
				{
					if(Door->IsC2Placed() || Door->IsHandleBroken() && Door->GetC2InteractableComponent() == GetOwnerPlayerCharacter()->LastInteractableComponent)
					{
						return;
					}
				}
				
				CurrentActorPlacement = LastGoodPlacement.GetActor();
				Server_StartC2Placement(CurrentActorPlacement);
				Super::OnItemPrimaryUse();
			}
		}
	}
}

void AC2Explosive::OnItemSecondaryUsed()
{
	if (!IsBlockingAnimationPlaying())
	{
		EquipDetonator(false);
		Super::OnItemSecondaryUsed();
	}
}
