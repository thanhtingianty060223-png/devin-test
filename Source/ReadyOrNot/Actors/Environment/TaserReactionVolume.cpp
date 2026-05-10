#include "TaserReactionVolume.h"
#include "ReadyOrNot.h"

ATaserReactionVolume::ATaserReactionVolume()
{
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;

	Bounds->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel6);
}