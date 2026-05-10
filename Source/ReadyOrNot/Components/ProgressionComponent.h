// Copyright Void Interactive, 2021

#pragma once

#include "Components/ActorComponent.h"
#include "ProgressionComponent.generated.h"

UCLASS(ClassGroup=(Progression), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UProgressionComponent();

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void AddExperience(float XP);
	
protected:
	void AddLevel(float OverflowXP);
	
	float StatLevel = 0.0f;
	float StatExperience = 0.0f;
	float XPRequiredToLevel = 0.0f;
};
