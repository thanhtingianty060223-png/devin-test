// Void Interactive, 2020

#pragma once

#include "UObject/Interface.h"
#include "Perception/AIPerceptionTypes.h"
#include "ReceiveAISenseUpdates.generated.h"

UINTERFACE()
class UReceiveAISenseUpdates : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IReceiveAISenseUpdates
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interface|Receive AI Sense Updates")
	void OnAIPerceptionSense(class ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interface|Receive AI Sense Updates")
	void OnAIHearingSense(class ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interface|Receive AI Sense Updates")
	void OnAIDamageSense(class ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor);
};
