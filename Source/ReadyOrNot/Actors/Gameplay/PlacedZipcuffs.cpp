#include "PlacedZipcuffs.h"
#include "ReadyOrNot.h"

APlacedZipcuffs::APlacedZipcuffs()
{
	ZipcuffMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = ZipcuffMesh;
	bReplicates = true;
}

void APlacedZipcuffs::Reset()
{
	Super::Reset();

	Destroy();
}
