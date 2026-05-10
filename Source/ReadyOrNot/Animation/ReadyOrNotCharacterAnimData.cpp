// Copyright Void Interactive, 2017

#include "ReadyOrNotCharacterAnimData.h"
#include "ReadyOrNot.h"


void UReadyOrNotCharacterAnimData::UpdateAllAnimsList()
{
	AllAnimsList.Empty();

#define IterateAnimsList(x) for(FCharacterTPAnim anim : x) AllAnimsList.AddUnique(anim);

	IterateAnimsList(Surrender);
	IterateAnimsList(Spooked_Front);
	IterateAnimsList(Spooked_Back);
	IterateAnimsList(Spooked_Left);
	IterateAnimsList(Spooked_Right);
	IterateAnimsList(Arrested);
	IterateAnimsList(StandRelaxedFidget);
	IterateAnimsList(StandAlertFidget);
	IterateAnimsList(HitReaction_Head);
	IterateAnimsList(HitReaction_UpperBody);
	IterateAnimsList(HitReaction_LowerBody);
	IterateAnimsList(HitReaction_LeftArm);
	IterateAnimsList(HitReaction_RightArm);
	IterateAnimsList(HitReaction_LeftLeg);
	IterateAnimsList(HitReaction_RightLeg);
	IterateAnimsList(HitReaction_LeftFoot);
	IterateAnimsList(HitReaction_RightFoot);
	IterateAnimsList(HitReaction_DropWeapon);
	AllAnimsList.AddUnique(DrawWeapon);
	AllAnimsList.AddUnique(HolsterWeapon);
	IterateAnimsList(Death_Head_Front);
	IterateAnimsList(Death_Head_Back);
	IterateAnimsList(Death_Arm_Left_Back);
	IterateAnimsList(Death_Arm_Left_Front);
	IterateAnimsList(Death_Arm_Right_Back);
	IterateAnimsList(Death_Arm_Right_Front);
	IterateAnimsList(Death_Leg_Left_Back);
	IterateAnimsList(Death_Leg_Left_Front);
	IterateAnimsList(Death_Leg_Right_Back);
	IterateAnimsList(Death_Leg_Right_Front);
	IterateAnimsList(Death_Front);
	IterateAnimsList(Death_Back);
	IterateAnimsList(Death_Bleedout_Head);
	IterateAnimsList(Death_Bleedout_Chest);
	IterateAnimsList(Death_Bleedout_Stomach);
	IterateAnimsList(Death_Bleedout_Left_Leg);
	IterateAnimsList(Death_Bleedout_Left_Arm);
	IterateAnimsList(Death_Bleedout_Right_Leg);
	IterateAnimsList(Death_Bleedout_Right_Arm);
	IterateAnimsList(Flashbanged);
	IterateAnimsList(Stingballed);
	IterateAnimsList(Sprayed);
	IterateAnimsList(Tasered);
	IterateAnimsList(Decision);
	IterateAnimsList(FakeSurrender);
}

FCharacterTPAnim UReadyOrNotCharacterAnimData::GetRandomCharacterAnimation(TArray<FCharacterTPAnim> AnimationArray)
{
	if (AnimationArray.Num() > 0)
	{
		return AnimationArray[FMath::RandRange(0, AnimationArray.Num() - 1)];
	}
	return FCharacterTPAnim();
}
