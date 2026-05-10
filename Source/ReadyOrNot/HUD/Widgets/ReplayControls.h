// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReplayControls.generated.h"


/**
 * 
 */
UCLASS()
class READYORNOT_API UReplayControls : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetMinimumReplayBarTime(float Percent);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateMountedSocketSelections();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PauseReplay();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SkipReplayForward();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SkipReplayBackward();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void NextActor();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PreviousActor();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ToggleHUD();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CustomTick();
};
