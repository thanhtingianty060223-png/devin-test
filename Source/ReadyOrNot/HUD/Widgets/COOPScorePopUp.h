// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "COOPScorePopUp.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCOOPScorePopUp : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* Reward;
	
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayRewardSound();
};
