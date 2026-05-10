// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerCategory_ReadyOrNot.h"

#include "Characters/CyberneticCharacter.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Pawn.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

FGameplayDebuggerCategory_ReadyOrNot::FGameplayDebuggerCategory_ReadyOrNot()
{
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_ReadyOrNot::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_ReadyOrNot());
}

void FGameplayDebuggerCategory_ReadyOrNot::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (!DebugActor)
		return;

	if (OwnerPC->GetLocalRole() != ENetRole::ROLE_Authority)
		return;

	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(DebugActor);
	if (CyberneticCharacter)
	{
		DebugData.Empty();
		CyberneticCharacter->GatherDebugData_Implementation(DebugData);
	}
}

void FGameplayDebuggerCategory_ReadyOrNot::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	for (FDebugData d : DebugData)
	{
		CanvasContext.Printf( TEXT("{white}%s {Yellow}%s"), *d.Label.ToString(), *d.Value.ToString());
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER
