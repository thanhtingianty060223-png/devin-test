// Copyright Void Interactive, 2021

#include "KillHostageChanceAnimNotify.h"
#include "ReadyOrNot.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Info/Activities/TakeHostageActivity.h"

void UKillHostageChanceAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ACyberneticCharacter* cc = Cast<ACyberneticCharacter>(MeshComp->GetOwner());
	if (cc)
	{
		ACyberneticController* cb = Cast<ACyberneticController>(cc->GetController());
		if (cb)
		{
			UTakeHostageActivity* HostageTakeActivity = Cast<UTakeHostageActivity>(cb->GetCurrentActivity());
			if (HostageTakeActivity)
			{
				/*
				if (HostageTakeActivity->bWillKillHostage)
				{
					HostageTakeActivity->KillHostage();
				}
				*/
			}
		}
	}
}
