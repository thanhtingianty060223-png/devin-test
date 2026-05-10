#include "ShotDetectionVolume.h"

AShotDetectionVolume::AShotDetectionVolume()
{
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;

	Bounds->SetCollisionObjectType(ECC_GameTraceChannel8);
	Bounds->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECR_Overlap);
}
