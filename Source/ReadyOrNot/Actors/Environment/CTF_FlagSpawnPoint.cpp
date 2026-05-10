// Void Interactive, 2020

#include "CTF_FlagSpawnPoint.h"

ACTF_FlagSpawnPoint::ACTF_FlagSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bFindCameraComponentWhenViewTarget = false;

	SetCanBeDamaged(false);

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> S_WindDirectional(TEXT("Texture2D'/Engine/EditorResources/S_WindDirectional.S_WindDirectional'"));
	BillboardComponent->SetSprite(S_WindDirectional.Object);
	
	RootComponent = BillboardComponent;

	bHasVisited = false;
}
