// Copyright Void Interactive, 2021

#include "Chemlight.h"

#include "Actors/Gameplay/ThrownChemlight.h"

#include "Components/AmmoComponent.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

AChemlight::AChemlight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	
	Ammo = CreateDefaultSubobject<UAmmoComponent>(TEXT("Chemlight Ammo"));
	
	ItemMesh->bReplicatePhysicsToAutonomousProxy = false;
}

void AChemlight::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetOwnerCharacter())
	{
		if (GetOwnerCharacter()->GetEquippedItem() != this)
		{
			SetItemVisibility(false);
		}
	}
}

void AChemlight::OnItemPrimaryUse()
{
	Super::OnItemPrimaryUse();
	NormalThrow();
}

bool AChemlight::PlayDraw(bool bDrawFirst)
{
	SetFPMeshHidden(false);
	SetItemVisibility(true);

	PlayChemlightThrowAnimation();
	return true;
}

void AChemlight::Server_SpawnThrownChemlight_Implementation()
{
	Multicast_SpawnThrownChemlight();
}

void AChemlight::Multicast_SpawnThrownChemlight_Implementation()
{
	SpawnThrownChemlight();
}

void AChemlight::SpawnThrownChemlight()
{
	AReadyOrNotCharacter* PCOwner = GetOwnerCharacter();
	if (!PCOwner)
		return;

	FTransform SpawnTransform = FTransform();
	FVector SpawnLocation = GetItemLocation();
	FRotator SpawnRotation = GetItemMesh()->GetComponentRotation();

	SetFPMeshHidden(true);
	

	SpawnTransform.SetLocation(SpawnLocation);
	SpawnTransform.SetRotation(SpawnRotation.Quaternion());

	SpawnThrownItemAtTransform(SpawnTransform, GetOwnerCharacter()->GetControlRotation().Vector());
}

void AChemlight::NormalThrow()
{
	if (!CanThrow())
		return;
	

}

void AChemlight::QuickThrow()
{
	if (!CanThrow())
		return;

	PlayChemlightThrowAnimation();
}

void AChemlight::CancelThrow()
{
	ClearTimers();
	
	if (APlayerCharacter* PCOwner = GetOwnerPlayerCharacter())
	{
		PCOwner->StopFPAnimMontage();
		PCOwner->bPendingChemlightThrow = false;
	}
}

void AChemlight::PlayChemlightThrowAnimation()
{
	if (!AnimationData)
		return;
	
	if (APlayerCharacter* PCOwner = Cast<APlayerCharacter>(GetOwner()))
	{
		SetFPMeshHidden(false);

		bIsBeingQuickThrown = true;


		if (PCOwner->IsLocalPlayer())
		{
			PCOwner->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_CHEMLIGHT);
		}
		PCOwner->bPendingChemlightThrow = true;

		// Play the appropriate chemlight throw animations
		FWeaponAnim& ThrowAnimData = PCOwner->bIsCrouched ? (AnimationData->Crouch_QuickThrow_Throw) : (AnimationData->QuickThrow_Throw);

		if (GetOwnerCharacter()->IsLocalPlayer())
		{
			PCOwner->PlayLocal1PMontage(ThrowAnimData.Body_FP);
			
		}
		PCOwner->PlayLocal3PMontage(ThrowAnimData.Body_TP, 0.5f);

		PlayLocalFPMontage(ThrowAnimData.Gun_FP);
		PlayTPMontage(ThrowAnimData.Gun_TP);

		if (ThrowAnimData.Gun_FP)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_ChemThrowCompleteExpiry, this, &AChemlight::OnChemlightThrownComplete, ThrowAnimData.Gun_FP->GetPlayLength()-0.15f);

			// Dummy timer used to artificially block the player from equipping whilst holstering this chemlight, this is to avoid the soft-lock issue when rapidly switching weapons
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_ChemQuickThrowAnimExpiry, this, &AChemlight::OnChemlightThrownComplete, ThrowAnimData.Gun_FP->GetPlayLength()*1.5f);
		}
		
		if (Cast<ACyberneticCharacter>(PCOwner))
		{
			if (ThrowAnimData.Body_TP)
				UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_AIDropChem, this, &AChemlight::OnChemlightThrown, ThrowAnimData.Body_TP->GetPlayLength() * 0.75f, false);
		}
	}
}

bool AChemlight::IsPlayingChemlightThrowAnimations() const
{
	if (!AnimationData)
		return false;
	
	if (APlayerCharacter* PCOwner = Cast<APlayerCharacter>(GetOwner()))
	{
		return PCOwner->Is1PMontagePlaying(AnimationData->Throw.Body_FP) || PCOwner->Is1PMontagePlaying(AnimationData->QuickThrow_Throw.Body_FP) || PCOwner->Is1PMontagePlaying(AnimationData->Crouch_Throw.Body_FP) || PCOwner->Is1PMontagePlaying(AnimationData->Crouch_QuickThrow_Throw.Body_FP);
	}

	return false;
}

int32 AChemlight::GetRemainingAmmo() const
{
	return (int32)Ammo->GetCurrentResource();
}

void AChemlight::OnChemlightThrown()
{
	SetFPMeshHidden(true);
	Server_SpawnThrownChemlight();
}

void AChemlight::OnChemlightThrownComplete()
{
	SetFPMeshHidden(false);
}

bool AChemlight::CanThrow() const
{
	if (!AnimationData || !GetOwnerCharacter())
		return false;
	
	if (Ammo->IsDepleted() || AnyTimersActive() || IsPlayingChemlightThrowAnimations())
		return false;

	return true;
}

bool AChemlight::AnyTimersActive() const
{
	return GetWorldTimerManager().IsTimerActive(TH_ChemThrowCompleteExpiry) || GetWorldTimerManager().IsTimerActive(TH_ChemQuickThrowAnimExpiry);
}

void AChemlight::ClearTimers()
{
	GetWorldTimerManager().ClearTimer(TH_ChemThrowCompleteExpiry);
	GetWorldTimerManager().ClearTimer(TH_ChemQuickThrowAnimExpiry);
}

void AChemlight::SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation)
{
	if (!GetOwnerCharacter())
		return;

	if (!ThrownItemClass)
		return;

	// Spawn thrown chemlight
	if (AThrownChemlight* Thrown = GetWorld()->SpawnActorDeferred<AThrownChemlight>(ThrownItemClass.Get(), Transform, GetOwner(), GetOwnerCharacter(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Thrown->FinishSpawning(Transform);
		
		Thrown->GetStaticMesh()->SetCollisionProfileName("Item");
		Thrown->GetStaticMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		
		Thrown->FullySimulateThrowPath(ThrowDirection, 0.0f, ThrowLocation);

		if (const FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Thrown->GetStaticMesh()->GetComponentLocation()))
		{
			Thrown->ThrownInRoom = Room->Name;
		}
		
		Ammo->DecreaseResource(1);

		if (GetOwnerPlayerCharacter())
		{
			GetOwnerPlayerCharacter()->bPendingChemlightThrow = false;
		}
	}
}

void AChemlight::SetFPMeshHidden(const bool bFPMeshHidden)
{
	ItemMesh->SetHiddenInGame(bFPMeshHidden);
}

bool AChemlight::IsDepleted() const
{
	return Ammo->IsDepleted();
}

bool AChemlight::IsBlockingAnimationPlaying(const TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (IsPlayingChemlightThrowAnimations() || AnyTimersActive())
		return true;
	
	return Super::IsBlockingAnimationPlaying(Exclusions);
}
