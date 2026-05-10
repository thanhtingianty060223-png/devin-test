// Copyright Void Interactive, 2021

#include "OutOfBoundsVolume.h"

AOutOfBoundsVolume::AOutOfBoundsVolume()
{
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetEnableGravity(false);
	Bounds->bApplyImpulseOnDamage = false;
	Bounds->bReplicatePhysicsToAutonomousProxy = false;
	Bounds->SetCollisionProfileName("Trigger");
	SetRootComponent(Bounds);
}
