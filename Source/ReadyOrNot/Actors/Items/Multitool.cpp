// Void Interactive, 2020

#include "Multitool.h"

AMultitool::AMultitool()
{
	ItemCategories.Add(EItemCategory::IC_Multitool);

	GetItemMesh()->bEnableUpdateRateOptimizations = false;

	MultitoolAnimData.Add(EMultitoolFunctions::MF_None, nullptr);
	MultitoolAnimData.Add(EMultitoolFunctions::MF_Knife, nullptr);
	MultitoolAnimData.Add(EMultitoolFunctions::MF_Lockpick, nullptr);
	MultitoolAnimData.Add(EMultitoolFunctions::MF_Wirecutter, nullptr);
}

void AMultitool::ChangeToolkit(const EMultitoolFunctions MultitoolFunction, const bool bPlayAnimation)
{
	if (bOperating)
		return;

	// Don't try to change again
	if (CurrentToolKit == MultitoolFunction)
		return;

	Client_ChangeToolkit(MultitoolFunction, bPlayAnimation);

	CurrentToolKit = MultitoolFunction;
}

void AMultitool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultitool, CurrentToolKit);
}

bool AMultitool::PlayDraw(const bool bDrawFirst)
{
	if (PendingFreeCharacter)
	{
		Server_StartUsingTool(PendingFreeCharacter);
		return false;
	}

	return Super::PlayDraw(bDrawFirst);
}

void AMultitool::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentToolKit == EMultitoolFunctions::MF_None)
		return;

	if (UReadyOrNotWeaponAnimData** AnimData = MultitoolAnimData.Find(CurrentToolKit))
	{
		AnimationData = *AnimData;
	}

	if (bOperating && TargetActor)
	{
		if (GetMultitoolOperatingTimeFromActiveToolkit() <= 0.0f)
		{	
			MaxOperatingTime = ICanUseMultitoolOn::Execute_GetMultitoolUseTime(TargetActor);
		}
		else
		{
			MaxOperatingTime = bAudioBasedProgress ? GetMultitoolOperatingTimeFromActiveToolkit() : ICanUseMultitoolOn::Execute_GetMultitoolUseTime(TargetActor);
		}
	}
}

void AMultitool::Server_StartUsingTool_Implementation(AActor* Target)
{
	if (!Target)
		return;

	if (!Target->Implements<UCanUseMultitoolOn>())
		return;
	
	if (!AnimationData)
		return;
	
	if (bOperating)
		return;
	
	if (APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter())
	{	
		bOperating = true;
		CurrentOperatingTime = 0.0f;
		TargetActor = Target;
		
		Client_PlayMultitoolAudio();
		Client_PlayItemAnimation(AnimationData->Multitool_Use);

		const EMultitoolFunctions NewKit = ICanUseMultitoolOn::Execute_GetMultitoolUseType(Target);
		if (NewKit == EMultitoolFunctions::MF_Knife)
		{
			if (PvPFreeInteraction)
			{
				if (APlayerCharacter* TargetPlayer = Cast<APlayerCharacter>(Target))
				{
					PvPFreeInteraction->PlayPairedInteraction(GetOwner(), TargetPlayer, this);
					OwnerCharacter->Server_PlayPVPSpeech("ReleasingPlayerFromArrestedState", OwnerCharacter->GetTeam());
				}
			}
		}
		
		OwnerCharacter->LockAllActions();
	}
}

void AMultitool::Server_StopUsingTool_Implementation(AActor* Target)
{
	if (GetOwnerCharacter())
		GetOwnerCharacter()->UnlockAllActions();

	if (!bOperating)
		return;
	
	bOperating = false;
	CurrentOperatingTime = 0.0f;
	
	Client_StopMultitoolAudio();
	Client_StopToolAnimation();
}

void AMultitool::Server_ToolComplete_Implementation()
{
	if (APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter())
	{
		OwnerCharacter->UnlockAllActions();
		OwnerCharacter->GetInventoryComponent()->EquipLastEquippedItem();

		if (bOperating)
		{
			bOperating = false;
			CurrentOperatingTime = 0.0f;

			Client_StopMultitoolAudio();
			Client_PlayItemAnimation(AnimationData->Multitool_Use_End);
			Client_FinishedToolUse(TargetActor, OwnerCharacter);
		}
	}
	
	if (TargetActor)
	{
		ICanUseMultitoolOn::Execute_Server_FinishedUsingMultitool(TargetActor, GetOwnerCharacter());
	}
}

void AMultitool::Client_FinishedToolUse_Implementation(AActor* Target, APlayerCharacter* pc)
{
	if (!Target)
		return;

	if (Target->Implements<UCanUseMultitoolOn>())
	{
		ICanUseMultitoolOn::Execute_Client_FinishedUsingMultitool(Target, pc);
		OnItemUseCompleted.Broadcast(this);
	}
}

float AMultitool::GetMultitoolOperatingTimeFromAudioLength(UFMODEvent* Event) const
{
	if (!Event)
		return 0.0f;

	FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(Event, EFMODSystemContext::Auditioning);

	if (EventDesc)
	{
		int AudioLength;
		EventDesc->getLength(&AudioLength);

		return ((float)AudioLength)/1000.0f;
	}

	return 0.0f;
}

float AMultitool::GetMultitoolOperatingTimeFromToolkit(const EMultitoolFunctions MultitoolFunction) const
{
	switch (MultitoolFunction)
	{
		case EMultitoolFunctions::MF_None:
		return 0.0f;

		case EMultitoolFunctions::MF_Lockpick:
		return GetMultitoolOperatingTimeFromAudioLength(FMODLockpickingSound);

		case EMultitoolFunctions::MF_Knife:
		return GetMultitoolOperatingTimeFromAudioLength(FMODKnifeSound);

		case EMultitoolFunctions::MF_Wirecutter:
		return GetMultitoolOperatingTimeFromAudioLength(FMODWirecutterSound);

		default:
		return 0.0f;
	}
}

float AMultitool::GetMultitoolOperatingTimeFromActiveToolkit() const
{
	return GetMultitoolOperatingTimeFromToolkit(CurrentToolKit);
}

bool AMultitool::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (!AnimationData)
	{
		return false;
	}

	if (IsItemAnimationPlaying(AnimationData->Multitool_Use, true, true) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Use_End, true, true) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Cutters_To_Knife) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Cutters_To_Lockpick) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Lockpick_To_Cutters) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Lockpick_To_Knife) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Knife_To_Cutters) ||
		IsItemAnimationPlaying(AnimationData->Multitool_Knife_To_Lockpick))
	{
		return true;
	}
	
	if (PvPFreeInteraction && PvPFreeInteraction->IsPairedInteractionPlayingOn(GetOwnerPlayerCharacter()))
	{
		return true;
	}
	
	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void AMultitool::Client_PlayMultitoolAudio_Implementation()
{
	UFMODEvent* Sound = nullptr;
	
	switch (CurrentToolKit)
	{
		case EMultitoolFunctions::MF_None:			break;
		case EMultitoolFunctions::MF_Lockpick:		Sound = FMODLockpickingSound; break;
		case EMultitoolFunctions::MF_Knife:			Sound = FMODKnifeSound;	break;
		case EMultitoolFunctions::MF_Wirecutter:	Sound = FMODWirecutterSound; break;
		default: break;
	}

	FMODAudioComp->SetEvent(Sound);
	FMODAudioComp->Activate(true);
	FMODAudioComp->Play();
}

void AMultitool::Client_ChangeToolkit_Implementation(const EMultitoolFunctions MultitoolFunction, const bool bPlayAnimation)
{
	if (!AnimationData)
		return;
	
	if (CurrentToolKit == MultitoolFunction)
		return;
	
	if (bPlayAnimation)
	{
		switch (CurrentToolKit)
		{
			case EMultitoolFunctions::MF_Lockpick:
				if (MultitoolFunction == EMultitoolFunctions::MF_Knife)
					Client_PlayItemAnimation(AnimationData->Multitool_Lockpick_To_Knife);
				else if (MultitoolFunction == EMultitoolFunctions::MF_Wirecutter)
					Client_PlayItemAnimation(AnimationData->Multitool_Lockpick_To_Cutters);
			break;
			
			case EMultitoolFunctions::MF_Knife:
				if (MultitoolFunction == EMultitoolFunctions::MF_Lockpick)
					Client_PlayItemAnimation(AnimationData->Multitool_Knife_To_Lockpick);
				else if (MultitoolFunction == EMultitoolFunctions::MF_Wirecutter)
					Client_PlayItemAnimation(AnimationData->Multitool_Knife_To_Cutters);
			break;
			
			case EMultitoolFunctions::MF_Wirecutter:
				if (MultitoolFunction == EMultitoolFunctions::MF_Lockpick)
					Client_PlayItemAnimation(AnimationData->Multitool_Cutters_To_Lockpick);
				else if (MultitoolFunction == EMultitoolFunctions::MF_Knife)
					Client_PlayItemAnimation(AnimationData->Multitool_Cutters_To_Knife);
			break;

			default:
			break;
		}
	}

	CurrentToolKit = MultitoolFunction;
}

void AMultitool::Client_StopMultitoolAudio_Implementation()
{
	if (FMODAudioComp->StudioInstance)
	{
		FMODAudioComp->StudioInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
	}
}

void AMultitool::Client_StopToolAnimation_Implementation()
{
	StopItemAnimation(AnimationData->Multitool_Use);
}

void AMultitool::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	StopUsingTool(nullptr);
	bMarkAsEvidenceWhenNoOwner = false;
	
	Super::OnThrownFromInventory(Thrower, bMarkAsEvidence);
}
