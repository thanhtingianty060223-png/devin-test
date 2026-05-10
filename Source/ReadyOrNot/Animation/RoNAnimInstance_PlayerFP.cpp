// Copyright Void Interactive, 2017
// Author: Alexander Mijalkovski

// Anim Instance for Player First-Person

#include "Animation/RoNAnimInstance_PlayerFP.h"
#include "ReadyOrNot.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/Items/Detonator.h"

URoNAnimInstance_PlayerFP::URoNAnimInstance_PlayerFP(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//set any default values for your variables here
}

// main func for tick calculations
void URoNAnimInstance_PlayerFP::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APlayerCharacter* CurPlayerChar = Cast<APlayerCharacter>(TryGetPawnOwner());

	// fallback just incase
	if (CurPlayerChar == nullptr)
	{
		return;
	}

	if (CurPlayerChar->GetEquippedItem())
	{
		if (CurPlayerChar->GetEquippedItem()->AnimationData)
		{
			LastWeaponAnimData = CurPlayerChar->GetEquippedItem()->AnimationData;
		}
	}

	if (CurPlayerChar->bIsCrouched)
		bCrouching = true;
	else
		bCrouching = false;

	GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::White, bCrouching ? "True" : "False");

	LeanAngleY = CurPlayerChar->FreeLeanX;
	LeanAngleZ = CurPlayerChar->FreeLeanZ;
	

	// handles the mesh control rotation for a specatign character...
	if (CurPlayerChar->GetControlRotation() != FRotator::ZeroRotator)
	{
		MeshControlRotation = CurPlayerChar->GetControlRotation();
	}
	else
	{
		MeshControlRotation = UKismetMathLibrary::RInterpTo(MeshControlRotation, CurPlayerChar->ReplicatedControlRotation.GetNormalized(), DeltaSeconds, 10.0f);
	}
	
	MeshPostureLeanOffset = CurPlayerChar->MeshPostureLeanOffset;

	MeshWeaponOffset = CurPlayerChar->MeshWeaponOffset;
	MeshWeaponRotation = CurPlayerChar->MeshWeaponRotation;

	MeshWeaponFreeAimRotation = CurPlayerChar->MeshWeaponFreeAimRotation;

	MeshWeaponLeanOffset = CurPlayerChar->MeshWeaponLeanOffset;
	MeshWeaponLeanRotation = CurPlayerChar->MeshWeaponLeanRotation;

	TPMeshReference = CurPlayerChar->GetMesh();

	// exposes lazy spring to weapon data
	if (CurPlayerChar->GetEquippedItem())
	{
		
		if (CurPlayerChar->bAiming)
			LazySpringStrength = CurPlayerChar->GetEquippedItem()->GetLazySpringStrengthADS();
		else
			LazySpringStrength = CurPlayerChar->GetEquippedItem()->GetLazySpringStrength();

		ADS_Movement_Weight = CurPlayerChar->GetEquippedItem()->CUR_FPS_ADS_Weight;
	}
	else
	{
		LazySpringStrength = 1.0f;
		ADS_Movement_Weight = 1.0f;
	}
	//

	

	// alpha of slot?
	InteractionSlotAlpha = GetSlotMontageGlobalWeight("Interaction");
	DefaultSlotAlpha = GetSlotMontageGlobalWeight("DefaultSlot");

	// added extra variable to handle roll input because we don't want any while in lean
	if (LeanAngleY != 0)
	{
		RollMoveInput = FMath::FInterpTo(RollMoveInput, 0.0f, DeltaSeconds, 15.0f);
	}
	else
	{
		RollMoveInput = FMath::FInterpTo(RollMoveInput, CurPlayerChar->MoveRightInput, DeltaSeconds, 15.0f);
	}
}


// TODO: figure out a better fallback if no animation data can be retrieved
// right now if the system can't find any data it will force a nullptr resulting in no motion which looks messed up in the engine.
// Ideal would be forcing a default anim or something i guess.

UAnimSequenceBase * URoNAnimInstance_PlayerFP::GetPlayerAnimation_FP(EBaseAnimType_FP::Entries AnimName) const
{
	UAnimSequenceBase * AnimSequence = nullptr;

	// use editor preview data if no data found in item
	UReadyOrNotWeaponAnimData* WeaponAnimData = LastWeaponAnimData ? LastWeaponAnimData : EditorWeaponAnimData;

	// if we have valid anim data proceed to find the requested motion
	if (WeaponAnimData)
	{
		switch (AnimName)
		{
		case EBaseAnimType_FP::IdlePose_FP: AnimSequence = WeaponAnimData->IdlePose_FP;
			break;

		case EBaseAnimType_FP::Idle_FP: AnimSequence = WeaponAnimData->Idle_FP;
			break;

		case EBaseAnimType_FP::Run_FP: AnimSequence = WeaponAnimData->Run_FP;
			break;

		case EBaseAnimType_FP::Walk_FP: AnimSequence = WeaponAnimData->Walk_FP;
			break;

		case EBaseAnimType_FP::Run_Limp_FP: AnimSequence = WeaponAnimData->Run_Limp_FP;
			break;

		case EBaseAnimType_FP::Walk_Limp_FP: AnimSequence = WeaponAnimData->Walk_Limp_FP;
			break;

		case EBaseAnimType_FP::Lowered_Up_Pose_FP: AnimSequence = WeaponAnimData->Lowered_Up_Pose_FP;
			break;

		case EBaseAnimType_FP::Lowered_Down_Pose_FP: AnimSequence = WeaponAnimData->Lowered_Down_Pose_FP;
			break;

		case EBaseAnimType_FP::ADS_Run_FP: AnimSequence = WeaponAnimData->ADS_Run_FP;
			break;

		case EBaseAnimType_FP::ADS_Walk_FP: AnimSequence = WeaponAnimData->ADS_Walk_FP;
			break;

		case EBaseAnimType_FP::ADS_Run_Limp_FP: AnimSequence = WeaponAnimData->ADS_Run_Limp_FP;
			break;

		case EBaseAnimType_FP::ADS_Walk_Limp_FP: AnimSequence = WeaponAnimData->ADS_Walk_Limp_FP;
			break;

		case EBaseAnimType_FP::IdlePose_AFG_FP: AnimSequence = WeaponAnimData->IdlePose_AFG_FP;
			break;

		case EBaseAnimType_FP::IdlePose_VFG_FP: AnimSequence = WeaponAnimData->IdlePose_VFG_FP;
			break;

		case EBaseAnimType_FP::IdlePose_HSTOP_FP: AnimSequence = WeaponAnimData->IdlePose_HSTOP_FP;
			break;

		default:
			break;
		}
	}

	return AnimSequence;
}

// ##UE5UPGRADE
UBlendSpace * URoNAnimInstance_PlayerFP::GetPlayerBlendspace_FP(EBaseBlendspaces_FP::Entries BlendspaceName) const
{
	UBlendSpace * Blendspace = nullptr;

	UReadyOrNotWeaponAnimData* WeaponAnimData = LastWeaponAnimData ? LastWeaponAnimData : EditorWeaponAnimData;

	// if we have valid anim data proceed to find the requested motion
	if (WeaponAnimData)
	{
		switch (BlendspaceName)
		{
		case EBaseBlendspaces_FP::Look_BS_FP: Blendspace = WeaponAnimData->Look_BS_FP;
			break;

		default:
			break;
		}
	}

	return Blendspace;
}

void URoNAnimInstance_PlayerFP::OnHolsterComplete()
{

	// // cast character
	// APlayerCharacter* CurPlayer = Cast<APlayerCharacter>(GetOwningActor());
	// if (CurPlayer)
	// {
	// 	CurPlayer->GetInventoryComponent()->OnLocalHolsterComplete();
	// }
}

void URoNAnimInstance_PlayerFP::OnReloadComplete()
{
	// cast character
	APlayerCharacter* CurPlayer = Cast<APlayerCharacter>(GetOwningActor());
	if (CurPlayer)
	{
		if (CurPlayer->GetLocalRole() < ROLE_Authority)
		{
			// TEMP ADD BULLETS CLIENT SIDE SO THE GUNS NOT EMPTY.. wait for server to replicate actual bullets.
			// this resolves issues where reload has finished but slide goes forward then back then forward due to replication delay..
			// even if the client fires a bullet it won't count server side and they'd besides either have to have a really high ping (and wouldn't notice it) 
			// or barely get the opportunity to fire a bullet because the anims still playing.
			ABaseMagazineWeapon* bmw = Cast<ABaseMagazineWeapon>(CurPlayer->GetEquippedItem());
			if (bmw && bmw->bClientPredictReload)
			{
				bmw->RemoveAmmo(-bmw->AmmoMax);
			}
		}
		CurPlayer->Server_OnReloadComplete();
	}
}

void URoNAnimInstance_PlayerFP::OnC2Detonation()
{
	// cast character
	APlayerCharacter* CurPlayer = Cast<APlayerCharacter>(GetOwningActor());
	if (CurPlayer)
	{
		ADetonator* det = Cast<ADetonator>(CurPlayer->GetEquippedItem());
		if (det)
		{
			det->Server_DetonateC2();
		}
	}
}
