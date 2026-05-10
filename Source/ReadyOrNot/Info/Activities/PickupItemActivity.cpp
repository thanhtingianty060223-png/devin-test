// Copyright Void Interactive, 2021

#include "PickupItemActivity.h"

#include "Actors/Gameplay/WeaponCacheActor.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Info/ReadyOrNotSignificanceManager.h"

UPickupItemActivity::UPickupItemActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "PickupItem");

	bAbortActivityIfCannotReachLocation = true;
}

void UPickupItemActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	bHasStartedPickup = false;
	//bAnnouncedFindingGun = false;
	//bHasPlayedPickupItemAnimation = false;
	bPickupItemAnimationComplete = false;
	bHasEverSeenWeaponCacheActor = false;
	
	// Dont bother finding one if we have a weapon equipped already
	if (GetCharacter()->GetEquippedItem())
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}

	if (GetCharacter()->IsCivilian())
	{
		ACTIVITY_FAILED("Is Civilian", true);
		return;
	}
	
	if (!PickupItem)
	{
		WeaponCacheActor = GetWeaponCacheActor();

		if (WeaponCacheActor)
		{
			Location = WeaponCacheActor->GetActorLocation();
			return;
		}
		
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Unable to find weapon cache for %s. Finding weapons nearby...", *OwningController->GetName());
		#endif

		PickupItem = FindItemToPickup();

		if (!PickupItem)
		{
			if (GetCharacter()->GetInventoryComponent()->GetHolsteredItem())
			{
				GetCharacter()->GetInventoryComponent()->EquipHolsteredItem();
			}
			
			ACTIVITY_FAILED("No pickup weapons available nearby", true);
			return;
		}
	}
	
	if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(PickupItem))
	{
		if (!Weapon->HasAnyAmmo())
		{
			ACTIVITY_FAILED("Pickup item " + Weapon->GetName() + " has no mags left", true);
			return;
		}
	}

	if (PickupItem)
	{
		if (PickupItem->bInInventory || PickupItem->GetOwnerCharacter())
		{
			ACTIVITY_FAILED("Pickup item " + PickupItem->GetName() + " is in someone's inventory", true);
			return;
		}
		
		UReadyOrNotSignificanceManager::ForceActorRelevant(PickupItem);
		Location = PickupItem->GetItemLocation();
	}
}

void UPickupItemActivity::FinishedActivity(bool bSuccess)
{
	GetCharacter()->OnPickupItem_FromAnimNotify.RemoveAll(this);

	PickupItem = nullptr;
}

void UPickupItemActivity::PerformActivity(const float DeltaTime)
{
	if (!OwningController || !GetCharacter())
		return;

	if (!IsValid(PickupItem) && !IsValid(WeaponCacheActor))
	{
		ACTIVITY_FAILED("No valid pick up item found", true);
		return;
	}

	if (PickupItem)
	{
		if (PickupItem->IsActorBeingDestroyed())
		{
			ACTIVITY_FAILED("Pickup item found is being destroyed", true);
			return;
		}
		
		if (PickupItem->GetOwner() && PickupItem->GetOwner() != GetCharacter())
		{
			ACTIVITY_FAILED("Pickup item found has an owner: " + GetNameSafe(PickupItem->GetOwner()), true);
			return;
		}
	}
	
	if ((bPickupItemAnimationComplete &&
		(PickupItem && PickupItem == GetCharacter()->GetEquippedItem())) || (GetCharacter()->GetEquippedItem() != nullptr))
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}
	
	if (GetCharacter()->IsStunned())
	{
		ACTIVITY_FAILED("Is Stunned", true);
		return;
	}

	if (PickupItem)
		Location = PickupItem->GetItemLocation();
	
	if (HasReachedLocation(75.0f) && (PickupItem || WeaponCacheActor))
	{
		FString PickupType = "_item";

		if (PickupItem)
		{
			if (PickupItem->ItemClass == EItemClass::IC_AssaultRifle ||
				PickupItem->ItemClass == EItemClass::IC_SMG ||
				PickupItem->ItemClass == EItemClass::IC_LMG ||
				PickupItem->ItemClass == EItemClass::IC_Shotgun)
			{
				PickupType = "_rifle";
			}
			else if (PickupItem->ItemClass == EItemClass::IC_Pistol)
			{
				PickupType = "_pistol";
			}
		}

		const FString Animation = "tp_pickup" + PickupType;
		
		if (GetCharacter()->IsTableMontagePlaying(Animation))
		{
			bHasStartedPickup = true;

			/*
			if (!bAnnouncedFindingGun && PickupItem)
			{
				USuspectsAndCivilianManager::Get(GetWorld())->PlayBarkOrStartConversation("AnnounceFoundGun", GetCharacter()->GetActorLocation());
				bAnnouncedFindingGun = true;
			}
			*/
			
			return;
		}
		
		if (!UReadyOrNotSignificanceManager::IsActorRelevant(GetCharacter()))
		{
			OnPickupItemComplete();
		}
		else
		{
			FVector FocalPoint = FVector::ZeroVector;
			if (WeaponCacheActor)
				FocalPoint = WeaponCacheActor->GetActorLocation();

			if (PickupItem)
				FocalPoint = PickupItem->GetItemLocation();
			
			GetCharacter()->PlayMontageFromTableWithFocalPoint(Animation, FocalPoint);
			bHasStartedPickup = false;
			
			GetCharacter()->OnPickupItem_FromAnimNotify.RemoveAll(this);
			GetCharacter()->OnPickupItem_FromAnimNotify.AddDynamic(this, &UPickupItemActivity::OnPickupItemComplete);
		}
	}

	Super::PerformActivity(DeltaTime);
}

AWeaponCacheActor* UPickupItemActivity::GetWeaponCacheActor()
{
	return UReadyOrNotFunctionLibrary::FindClosestActor<AWeaponCacheActor>(GetWorld(), GetCharacter()->GetActorLocation(), 3000.0f);
}

void UPickupItemActivity::OnPickupItemComplete()
{
	bPickupItemAnimationComplete = true;
	
	if (WeaponCacheActor && !PickupItem)
	{
		PickupItem = GetWorld()->SpawnActor<ABaseItem>(WeaponCacheActor->GetRandomAvailableWeapon(), FVector(0.0f, 0.0f, 50000.0f), FRotator::ZeroRotator);
	}

	UReadyOrNotSignificanceManager::ForceActorRelevant(PickupItem);
	
	if (OwningController->IsSWAT())
	{
		GetCharacter()->Server_CollectEvidence(PickupItem);
	}
	else
	{
		PickupItem->MarkAsEvidence(false);
		PickupItem->GetItemMesh()->SetSimulatePhysics(false);
		
		OwningController->GetCharacter()->GetInventoryComponent()->AddInventoryItem(PickupItem);
		OwningController->GetCharacter()->GetInventoryComponent()->PutItemInHands(PickupItem, true, true);
		
		OwningController->FinishActivity(this, true, true);
	}
}

bool UPickupItemActivity::CanFinishActivity() const
{
	return false; // force finished
}

bool UPickupItemActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (WeaponCacheActor)
	{
		if (!bHasEverSeenWeaponCacheActor)
		{
			FHitResult Hit;
			GetWorld()->LineTraceSingleByObjectType(Hit, GetCharacter()->GetActorLocation(), WeaponCacheActor->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic));
			if (!Hit.bBlockingHit)
			{
				bHasEverSeenWeaponCacheActor = true;
			}
		}
		else
		{
			FocalPoint = WeaponCacheActor->GetActorLocation() + WeaponCacheActor->GetArrowComponent()->GetForwardVector() * 1000.0f;
			return true;
		}
	}
	
	return false;
}

float UPickupItemActivity::GetDestinationTolerance() const
{
	return 75.0f;
}

#if !UE_BUILD_SHIPPING
void UPickupItemActivity::GatherDebugString(FString& OutString)
{
	OutString += AddDebugString("Pickup Item", GetNameSafe(PickupItem));
	OutString += AddDebugString("Using Weapon Cache", WeaponCacheActor ? "true" : "false");
	OutString += AddDebugString("Search Radius", FString::SanitizeFloat(SearchRadius));
	OutString += AddDebugString("Started Pickup", bHasStartedPickup ? "true" : "false");
	OutString += AddDebugString("Pickup Complete", bPickupItemAnimationComplete ? "true" : "false");
}
#endif

ABaseItem* UPickupItemActivity::FindItemToPickup() const
{
	TArray<ABaseItem*> PossiblePickupWeapons = GetCharacter()->GetInventoryComponent()->GetRemovedInventoryItems();

	PossiblePickupWeapons.RemoveAll([](ABaseItem* Item)
	{
		if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Item))
		{
			return !Weapon->HasAnyAmmo();
		}

		return false;
	});

	/*if (PossiblePickupWeapons.Num() == 0)
	{
		// Find a weapon lying around within 10m, if possible
		const FVector StartLocation = GetCharacter()->GetActorLocation();
			
		TArray<FHitResult> Hits;

		// TODO: Async trace?
		//UKismetSystemLibrary::BoxTraceMultiForObjects(this, StartLocation, StartLocation, , FRotator::ZeroRotator, {UEngineTypes::ConvertToObjectType(ECC_ITEM)}, false, {}, EDrawDebugTrace::ForDuration, Hits, true);
		GetWorld()->SweepMultiByProfile(Hits, StartLocation, StartLocation, FQuat::Identity, "Item", FCollisionShape::MakeBox(FVector(1000.0f, 1000.0f, 150.0f)), GetCharacter()->GetCollisionQueryParameters());

		for (FHitResult& Hit : Hits)
		{
			if (ABaseWeapon* Weapon = Cast<ABaseWeapon>(Hit.GetActor()))
			{
				if (!Weapon->GetOwner())
				{
					PossiblePickupWeapons.Add(Weapon);
				}
			}
		}
	}*/

	return UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(GetCharacter()->GetActorLocation(), PossiblePickupWeapons);
}
