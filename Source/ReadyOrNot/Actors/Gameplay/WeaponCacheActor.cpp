// Void Interactive, 2020

#include "WeaponCacheActor.h"

AWeaponCacheActor::AWeaponCacheActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;

	DefaultScene = CreateDefaultSubobject<USceneComponent>("DefaultScene");
	SetRootComponent(DefaultScene);
	
#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoverTexture(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/bsg.bsg'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	BillboardComponent->SetSprite(CoverTexture.Object);
	BillboardComponent->ScreenSize = 0.004f;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Movable); // To get rid of PIE warnings, if mobility was set to Static
	BillboardComponent->SetRelativeScale3D(FVector(0.85f));
	BillboardComponent->SetupAttachment(RootComponent);
#endif

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
}

TSubclassOf<ABaseMagazineWeapon> AWeaponCacheActor::GetRandomAvailableWeapon() const
{
	return AvailableWeapons.Num() > 0 ? AvailableWeapons[FMath::RandRange(0, AvailableWeapons.Num() - 1)] : nullptr;
}
