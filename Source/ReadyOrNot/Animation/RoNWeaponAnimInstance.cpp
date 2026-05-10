// Copyright Void Interactive, 2017

#include "RoNWeaponAnimInstance.h"
#include "ReadyOrNot.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/PickupMagazineActor.h"
#include "Characters/PlayerCharacter.h"


URoNWeaponAnimInstance::URoNWeaponAnimInstance()
{

}

void URoNWeaponAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
}

void URoNWeaponAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	//GEngine->AddOnScreenDebugMessage(-1, DeltaSeconds, FColor::White, "Tick: " + GetName() + " Owner: " + GetOwningActor()->GetName());
	ABaseMagazineWeapon* weapon = Cast<ABaseMagazineWeapon>(GetOwningActor());
	if (weapon)
	{
		AmmoRemaining = weapon->GetAmmo();
		bIsEquipped = weapon->IsEquipped();
	}
}

void URoNWeaponAnimInstance::NativeLastTick(float DeltaSeconds)
{

}

void URoNWeaponAnimInstance::OnSpeedReloadMagazineEjected()
{
	ABaseMagazineWeapon* weapon = Cast<ABaseMagazineWeapon>(GetOwningActor());
	if (weapon)
	{
		
		int32 boneIdx = -1;
		FTransform SpawnTransform;
		FVector downVector = FVector::ZeroVector;
		FVector forwardVector = FVector::ForwardVector;
		FVector rightVector = FVector::ZeroVector;
		APlayerCharacter* pc = Cast<APlayerCharacter>(weapon->GetOwner());
		if (pc)
		{
			FName Mag01Socket, Mag02Socket;
			weapon->GetMagazineAttachmentSockets(Mag01Socket, Mag02Socket);
			downVector = pc->GetActorUpVector() * -1;
			forwardVector = pc->GetActorForwardVector();
			rightVector = pc->GetActorRightVector();
			if (pc->IsLocallyControlled())
			{
				SpawnTransform = weapon->ItemMesh->GetSocketTransform(Mag01Socket);
			}
			else
			{
				SpawnTransform = weapon->ItemMesh->GetSocketTransform(Mag01Socket);
			}

		}

		if (SpawnTransform.GetLocation() != FVector::ZeroVector)
		{
			// spawn the magazine to eject
			pc->Server_SpawnEjectedMagazine(SpawnTransform, weapon);
		}
	}
}

void URoNWeaponAnimInstance::OnDisassembleMagazineEjected()
{
	ABaseMagazineWeapon* const Weapon = Cast<ABaseMagazineWeapon>(GetOwningActor());

	if (!Weapon)
	{
		return;
	}

	APlayerCharacter* const pc = Cast<APlayerCharacter>(Weapon->GetOwner());

	if (!pc)
	{
		return;
	}

	int32 BoneIndex = INDEX_NONE;
	FTransform BoneTransform = FTransform::Identity;

	if (pc->IsLocallyControlled())
	{
		BoneIndex = Weapon->ItemMesh->GetBoneIndex(Weapon->Mag_01_Socket);
		BoneTransform = Weapon->ItemMesh->GetBoneTransform(BoneIndex);
	}
	else
	{
		BoneIndex = Weapon->ItemMesh->GetBoneIndex(Weapon->Mag_01_Socket);
		BoneTransform = Weapon->ItemMesh->GetBoneTransform(BoneIndex);
	}

	if (BoneIndex != INDEX_NONE)
	{
		const FQuat Rotation  =  BoneTransform.GetRotation();
		const FVector Forward = -Rotation.GetForwardVector();
		const FVector Right   =  Rotation.GetRightVector();
		const FVector Down    = -Rotation.GetUpVector();

		pc->Server_SpawnEjectedMagazine(BoneTransform, Weapon);
	}
}
