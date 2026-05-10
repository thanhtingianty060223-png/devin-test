// © Void Interactive, 2022

#pragma once

#include "Components/ArrowComponent.h"
#include "MirrorPortalComponent.generated.h"

// Mirror Portal Components are where the camera on the Optiwand goes to when a Mirror Zone is activated
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable)
class READYORNOT_API UMirrorPortalComponent : public UArrowComponent
{
	GENERATED_BODY()

};