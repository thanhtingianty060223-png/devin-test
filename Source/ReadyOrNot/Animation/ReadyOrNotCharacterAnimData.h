// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReadyOrNotCharacterAnimData.generated.h"

USTRUCT()
struct FCharacterTPAnim
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* TP_Anim;

	bool operator==(const FCharacterTPAnim& OtherItem) const
	{
		return OtherItem.TP_Anim == TP_Anim;
	}

	FCharacterTPAnim()
	{
		TP_Anim = nullptr;
	}
};

USTRUCT()
struct FCharacterFPAnim
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* FP_Anim;

	bool operator==(const FCharacterFPAnim& OtherItem) const
	{
		return OtherItem.FP_Anim == FP_Anim;
	}

	FCharacterFPAnim()
	{
		FP_Anim = nullptr;
	}
};

USTRUCT()
struct FCharacterSharedAnim
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* TP_Anim;

	UPROPERTY(EditAnywhere, Category = Animation)
	UAnimMontage* FP_Anim;

	bool operator==(const FCharacterSharedAnim& OtherItem) const
	{
		return OtherItem.TP_Anim == TP_Anim && OtherItem.FP_Anim == FP_Anim;
	}

	FCharacterSharedAnim()
	{
		TP_Anim = FP_Anim = nullptr;
	}
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotCharacterAnimData : public UDataAsset
{
	GENERATED_BODY()

public:

	void UpdateAllAnimsList();

	static FCharacterTPAnim GetRandomCharacterAnimation(TArray<FCharacterTPAnim> AnimationArray);
	

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Surrender;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> FakeSurrender;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Spooked_Front;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Spooked_Right;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Spooked_Left;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Spooked_Back;

	UPROPERTY(EditAnywhere, Category = Surrender)
		TArray<FCharacterTPAnim> Arrested;

	UPROPERTY(EditAnywhere, Category = Decision)
		TArray<FCharacterTPAnim> Decision;

	UPROPERTY(EditAnywhere, Category = Idle)
		TArray<FCharacterTPAnim> StandRelaxedFidget;

	UPROPERTY(EditAnywhere, Category = Idle)
		TArray<FCharacterTPAnim> StandAlertFidget;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_Head;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_UpperBody;


	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_LowerBody;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_LeftArm;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_RightArm;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_LeftLeg;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_RightLeg;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_LeftFoot;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_RightFoot;

	UPROPERTY(EditAnywhere, Category = HitReaction)
		TArray<FCharacterTPAnim> HitReaction_DropWeapon;

	UPROPERTY(EditAnywhere, Category = Weapons)
		FCharacterTPAnim FireWeapon;

	UPROPERTY(EditAnywhere, Category = Weapons)
		FCharacterTPAnim DrawWeapon;

	UPROPERTY(EditAnywhere, Category = Weapons)
		FCharacterTPAnim HolsterWeapon;

	UPROPERTY(EditAnywhere, Category = Weapons)
		FCharacterTPAnim ReloadWeapon;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Head_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Head_Back;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Arm_Left_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Arm_Left_Back;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Arm_Right_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Arm_Right_Back;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Leg_Left_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Leg_Left_Back;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Leg_Right_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Leg_Right_Back;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Front;

	UPROPERTY(EditAnywhere, Category = Death)
		TArray<FCharacterTPAnim> Death_Back;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Head;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Chest;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Stomach;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Left_Arm;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Right_Arm;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Left_Leg;

	UPROPERTY(EditAnywhere, Category = "Death | Bleedout")
		TArray<FCharacterTPAnim> Death_Bleedout_Right_Leg;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Grenade")
		TArray<FCharacterTPAnim> Flashbanged;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Grenade")
		TArray<FCharacterTPAnim> Stingballed;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Grenade")
		TArray<FCharacterTPAnim> Gassed;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Less Lethal")
		TArray<FCharacterTPAnim> Sprayed;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Less Lethal")
		TArray<FCharacterTPAnim> Tasered;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Less Lethal")
		TArray<FCharacterTPAnim> Meleed;

	UPROPERTY(EditAnywhere, Category = "Doors")
		FCharacterTPAnim OpenDoor;

	UPROPERTY(EditAnywhere, Category = "Doors")
		FCharacterTPAnim CloseDoor;

	UPROPERTY(EditAnywhere, Category = "HitReaction | Flinches")
		FCharacterTPAnim Flinches;


	/* TURN TRANSITION ASSETS, gets used by animinstance turn in place code */
	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Calm_Turn90Left;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Calm_Turn90Right;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Calm_Turn180Left;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Calm_Turn180Right;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Alert_Turn90Left;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Alert_Turn90Right;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Alert_Turn180Left;

	UPROPERTY(EditAnywhere, Category = TurnTransitions)
	UAnimSequence* Alert_Turn180Right;


	UPROPERTY(VisibleAnywhere, Category = Data)
	TArray<FCharacterTPAnim> AllAnimsList;
};
