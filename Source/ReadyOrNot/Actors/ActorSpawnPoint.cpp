// Void Interactive, 2020

#include "ActorSpawnPoint.h"

AActorSpawnPoint::AActorSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bFindCameraComponentWhenViewTarget = false;

	bReplicates = true;
	SetCanBeDamaged(false);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(FName("Scene Component"));
	SetRootComponent(SceneComponent);

	#if WITH_EDITOR
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(FName("Billboard"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> S_WindDirectional(TEXT("Texture2D'/Engine/EditorResources/EmptyActor.EmptyActor'"));
	BillboardComponent->SetSprite(S_WindDirectional.Object);
	BillboardComponent->SetupAttachment(RootComponent);
	#endif

	bHasVisited = false;
}
