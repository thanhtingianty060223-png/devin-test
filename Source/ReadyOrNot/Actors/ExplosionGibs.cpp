// Copyright Void Interactive, 2022

#include "ExplosionGibs.h"
#include "ReadyOrNot.h"

void AExplosionGibs::SpawnBloodDecal(const FHitResult& Hit)
{
	UMaterialInterface* Material = BloodData->Splatters[FMath::RandRange(0, BloodData->Splatters.Num() - 1)];
	
	FVector Size = BloodData->BigSplatterDecalSize;
	
	FRotator Rotation = Hit.ImpactNormal.Rotation();
	Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

	UDecalComponent* SpawnedDecal = UGameplayStatics::SpawnDecalAttached(Material, Size, Hit.GetComponent(), Hit.BoneName, Hit.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition);
}
