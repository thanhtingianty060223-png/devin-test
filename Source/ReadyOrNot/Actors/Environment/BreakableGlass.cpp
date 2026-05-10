// Copyright Void Interactive, 2022

#include "Actors/Environment/BreakableGlass.h"

ABreakableGlass::ABreakableGlass()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ABreakableGlass::BeginPlay()
{
	Super::BeginPlay();
}

void ABreakableGlass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABreakableGlass::ConvertHitAndExecute(FHitResult Hit, float Damage)
{
	if (!Hit.GetComponent())
		return;

	if (Hit.GetComponent()->ComponentTags.Num() > 2)
	{
		Damage *= 0.75f;
		int32 LastValidHitNum = FCString::Atoi(*Hit.GetComponent()->ComponentTags[0].ToString()) - 1;
		int32 LastTextureY = FCString::Atoi(*Hit.GetComponent()->ComponentTags[1].ToString());
		int32 LastTextureX = FCString::Atoi(*Hit.GetComponent()->ComponentTags[2].ToString());
		FVector Direction = UKismetMathLibrary::FindLookAtRotation(Hit.TraceStart, Hit.TraceEnd).Vector();
		FVector HitPosition = Hit.ImpactPoint;

		Multicast_ConvertHitAndExecute(LastValidHitNum, LastTextureX, LastTextureY, HitPosition, Direction, Damage);
	}
	else
	{
		Multicast_DestructibleHit(Hit.Location);
	}
}

// NOTE(killo): we avoid sending the whole hit result since its a whopping 144 bytes, also glass BP makes new component so can't use RPC to send the hit component
void ABreakableGlass::Multicast_ConvertHitAndExecute_Implementation(int32 FirstPositionBox, int32 TextureX, int32 TextureY, FVector_NetQuantize HitPosition, FVector_NetQuantize Direction, float Damage)
{
	FirstHitPositionObject(FirstPositionBox, TextureY, TextureX, HitPosition, Direction, Damage, true, 10000.0f);
}

void ABreakableGlass::Multicast_DestructibleHit_Implementation(FVector_NetQuantize Location)
{
	DestructibleHit(Location);
}