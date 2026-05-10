// Void Interactive, 2020

#include "Actors/Items/Tool.h"

void ATool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATool, bOperating);
	DOREPLIFETIME(ATool, CurrentOperatingTime);
	DOREPLIFETIME(ATool, MaxOperatingTime);
	DOREPLIFETIME(ATool, TargetActor);
}

void ATool::StartUsingTool(AActor* Target)
{
	if (!AnimationData)
		return;
	
	if (bOperating)
		return;

	if (IsBlockingAnimationPlaying({}))
		return;
	
	Server_StartUsingTool(Target);
}

void ATool::StopUsingTool(AActor* Target)
{
	Server_StopUsingTool(Target);
}

void ATool::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	
	if (bOperating)
	{
		CurrentOperatingTime += DeltaSeconds;

		if (CurrentOperatingTime >= MaxOperatingTime)
		{
			Server_ToolComplete();
		}
	}
	else
	{
		CurrentOperatingTime = 0.0f;
	}

	if (HasAuthority())
	{
		// Failsafe
		if (!bOperating)
		{
			if (IsItemAnimationPlaying(AnimationData->Multitool_Use))
			{
				Client_StopItemAnimation(AnimationData->Multitool_Use);
			}
		}
	}
	
}

void ATool::Server_StartUsingTool_Implementation(AActor* Target)
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
		OwnerCharacter->LockAllActions();
		
		bOperating = true;
		CurrentOperatingTime = 0.0f;
		TargetActor = Target;
		
		//Client_PlayMultitoolAudio(); // todo
		Client_PlayItemAnimation(AnimationData->Multitool_Use);
	}
}

void ATool::Server_StopUsingTool_Implementation(AActor* Target)
{
	if (GetOwnerCharacter())
		GetOwnerCharacter()->UnlockAllActions();

	if (!bOperating)
		return;
	
	bOperating = false;
	CurrentOperatingTime = 0.0f;
	
	Client_StopToolAnimation();
}

void ATool::Client_StopToolAnimation_Implementation()
{
	StopItemAnimation(AnimationData->Multitool_Use);
}

void ATool::Server_ToolComplete_Implementation()
{
	if (APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter())
	{
		OwnerCharacter->UnlockAllActions();
		OwnerCharacter->GetInventoryComponent()->EquipLastEquippedItem();

		if (bOperating)
		{
			bOperating = false;
			CurrentOperatingTime = 0.0f;

			Client_PlayItemAnimation(AnimationData->Multitool_Use_End);
			Client_FinishedToolUse(TargetActor, OwnerCharacter);
		}
	}
	
	if (TargetActor)
	{
		ICanUseMultitoolOn::Execute_Server_FinishedUsingMultitool(TargetActor, GetOwnerCharacter());
	}
}

void ATool::Client_FinishedToolUse_Implementation(AActor* Target, APlayerCharacter* pc)
{
	if (!Target)
		return;

	if (Target->Implements<UCanUseMultitoolOn>())
	{
		ICanUseMultitoolOn::Execute_Client_FinishedUsingMultitool(Target, pc);
		OnItemUseCompleted.Broadcast(this);
	}
}
