// Void Interactive, 2020

#include "Actors/CoverLandmarkProxy.h"

#include "CoverLandmark.h"

ACoverLandmarkProxy::ACoverLandmarkProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 1.0f;

	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	bRelevantForLevelBounds = false;
	AutoReceiveInput = EAutoReceiveInput::Disabled;
	bEnableAutoLODGeneration = false;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bReplicates = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	SceneComponent->SetMobility(EComponentMobility::Static);
	SceneComponent->SetCanEverAffectNavigation(false);

	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Billboard_Icon(TEXT("Texture2D'/Engine/EditorResources/Waypoint.Waypoint'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetWorldScale3D(FVector(0.5f));
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetCanEverAffectNavigation(false);
	BillboardComponent->bEnableAutoLODGeneration = false;
	BillboardComponent->bReceiveMobileCSMShadows = false;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->ScreenSize = 0.0035f;
	BillboardComponent->SetSprite(Billboard_Icon.Object);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	ArrowComponent->SetupAttachment(GetRootComponent());
	ArrowComponent->SetRelativeLocation(FVector::ZeroVector);
	#endif
}

void ACoverLandmarkProxy::BeginPlay()
{
	Super::BeginPlay();

	if (!LandmarkOwner)
	{
		Destroy();
	}
}
