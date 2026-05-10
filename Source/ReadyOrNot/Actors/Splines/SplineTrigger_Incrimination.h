// Copyright Void Interactive, 2021

#pragma once

#include "SplineTrigger.h"
#include "SplineTrigger_Incrimination.generated.h"

UCLASS(HideCategories=("HLOD", "Mobile", "Asset User Data", "Actor", "Rendering", "Physics", "Input", "Cooking", "LOD", "Collision"))
class READYORNOT_API ASplineTrigger_Incrimination : public ASplineTrigger
{
	GENERATED_BODY()
	
public:
	ASplineTrigger_Incrimination();

protected:
	void BeginPlay() override;
};
