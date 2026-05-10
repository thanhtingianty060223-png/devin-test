#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class FGameplayDebuggerCategory_ReadyOrNot : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_ReadyOrNot();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	TArray<FDebugData> DebugData;
	
};

#endif // WITH_GAMEPLAY_DEBUGGER
