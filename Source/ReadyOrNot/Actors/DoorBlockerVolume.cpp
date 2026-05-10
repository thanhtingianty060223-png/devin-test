// Copyright Void Interactive, 2023

#include "Actors/DoorBlockerVolume.h"

#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"

ADoorBlockerVolume::ADoorBlockerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.0f;
	
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->AreaClass = UNavArea_Null::StaticClass();
	Bounds->bDynamicObstacle = true;
	Bounds->SetBoxExtent(FVector(50.0f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	NavModifierComponent = CreateDefaultSubobject<UNavModifierComponent>(TEXT("Nav Modifier"));
	NavModifierComponent->AreaClass = UNavArea_Null::StaticClass();

	SetActorEnableCollision(false);
}
