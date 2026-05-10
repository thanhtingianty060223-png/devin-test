// Copyright Void Interactive, 2017

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#include "GlassActor.h"

// Sets default values
AGlassActor::AGlassActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// DestructibleWindow = CreateDefaultSubobject<UDestructibleComponent>(TEXT("DestructibleWindow"));
	// RootComponent = DestructibleWindow;
}

// Called when the game starts or when spawned
void AGlassActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGlassActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UMaterialInterface* AGlassActor::GetRandomGlassMaterial()
{
	if (RandomGlassMaterial.Num() > 0)
	{
		return RandomGlassMaterial[FMath::RandRange(0, RandomGlassMaterial.Num() - 1)];
	}
	return nullptr;
	// return DestructibleWindow->GetMaterial(0);
}

UMaterialInterface* AGlassActor::GetRandomShatteredGlassMaterial()
{
	if (RandomShatteredGlassMaterial.Num() > 0)
	{
		return RandomShatteredGlassMaterial[FMath::RandRange(0, RandomShatteredGlassMaterial.Num() - 1)];
	}
	return nullptr;
	// return DestructibleWindow->GetMaterial(0);
}

float AGlassActor::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{

	if (GetLocalRole() >= ROLE_Authority)
	{
		UDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
			Multicast_ApplyDamageToWindow(DamageAmount, PointDamageEvent->HitInfo.ImpactPoint, PointDamageEvent->ShotDirection, DamageTypeCDO->DestructibleImpulse);
		}
// 		else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
// 		{
// 			FRadialDamageEvent const* const RadialDamageEvent = (FRadialDamageEvent*)(&DamageEvent);
// 			Multicast_ApplyDamageToWindow(DamageAmount, RadialDamageEvent->Origin, RadialDamageEvent->Params.OuterRadius, DamageTypeCDO->DestructibleImpulse, false);
// 		}
	}
	return 0;//Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AGlassActor::Multicast_TakeDamage_Implementation(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
}

void AGlassActor::Multicast_ApplyDamageToWindow_Implementation(float DamageAmount, FVector HitLocation, FVector ImpulseDir, float ImpulseStrength)
{
	OnApplyDamageToWindow.Broadcast(DamageAmount, HitLocation, ImpulseDir, ImpulseStrength);
//	DestructibleWindow->ApplyDamage(DamageAmount, HitLocation, ImpulseDir, ImpulseStrength);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
