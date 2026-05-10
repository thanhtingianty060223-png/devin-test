// Void Interactive, 2020

#include "FlankingAvoidanceVolume.h"

#include "NavModifierComponent.h"
#include "Navigation/ReadyOrNotNavAreas.h"

AFlankingAvoidanceVolume::AFlankingAvoidanceVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.0f;
	
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->AreaClass = UNavArea_FlankingAvoidanceArea::StaticClass();
	Bounds->bDynamicObstacle = true;
	Bounds->SetBoxExtent(FVector(1000.0f, 1000.0f, 250.0f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetCanEverAffectNavigation(false);
	
	NavModifierComponent = CreateDefaultSubobject<UNavModifierComponent>(TEXT("Nav Modifier"));
	NavModifierComponent->AreaClass = UNavArea_FlankingAvoidanceArea::StaticClass();

	SetActorEnableCollision(false);
}
