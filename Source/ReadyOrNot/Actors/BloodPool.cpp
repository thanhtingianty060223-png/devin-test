#include "BloodPool.h"
#include "ReadyOrNot.h"

ABloodPool::ABloodPool() : Super()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	RootComponent = Decal;
}
