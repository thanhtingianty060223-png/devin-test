// Copyright Void Interactive, 2023


#include "BaseTriggerable.h"

ABaseTriggerable::ABaseTriggerable()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(FName("Scene Component"));
	SetRootComponent(SceneComponent);
}
