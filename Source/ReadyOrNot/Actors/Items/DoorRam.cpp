// Copyright Void Interactive, 2021

#include "DoorRam.h"

#include "Actors/Door.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "Perception/AISense_Hearing.h"
#include "Subsystems/AchievementSubsystem.h"

void ADoorRam::OnItemPrimaryUse()
{
	StartRamming();
}

void ADoorRam::StartRamming()
{
	if (!AnimationData)
	{
		return;
	}

	if (IsBlockingAnimationPlaying())
	{
		return;
	}

	if (APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner()))
	{
		// Play the montage. It will drive the rest of the action through notifies.
		pc->Play1PMontage(AnimationData->FireSingle[0].Body_FP);
		pc->Play3PMontage(AnimationData->FireSingle[0].Body_TP);
	}

	Super::OnItemPrimaryUse();
}

void ADoorRam::OnBatteringRamHit()
{
	LastGoodHit = TryGetHitPosition();

	if (ADoor* Door = Cast<ADoor>(LastGoodHit.GetActor()))
	{
		Server_StrikeDoor(Door);

		// Count number of rams
		// Achievement HERES_JOHNNY
		if (GetOwnerCharacter() && GetOwnerCharacter()->IsLocalPlayer())
		{
			UAchievementStatics::IncreaseAchievementStat(GetWorld(), EAchievementStats::PROGRESS_BATTERING_RAM, 1);
		}
	}
	else if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(LastGoodHit.GetActor()))
	{
		Server_StrikePlayer(PlayerCharacter);
	}
}

void ADoorRam::Server_StrikePlayer_Implementation(APlayerCharacter* TargetCharacter)
{
	APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter();
	if (!OwnerCharacter)
		return;
	
	if (!TargetCharacter)
		return;

	LastGoodHit = TryGetHitPosition();
	
	TargetCharacter->LaunchCharacter(LastGoodHit.ImpactNormal * -500.0f, false, false);
	
	UGameplayStatics::ApplyDamage(TargetCharacter, StrikePlayerDamage, OwnerCharacter->Controller, this, RamDamageTypePlayer);
}

void ADoorRam::Server_StrikeDoor_Implementation(ADoor* TargetDoor)
{
	APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter();
	if (!OwnerCharacter)
		return;

	if (!TargetDoor)
		return;

	LastGoodHit = TryGetHitPosition();
	
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, OwnerCharacter, 1200.0f, "RammedDoor");
	
	UGameplayStatics::ApplyPointDamage(TargetDoor, StrikePlayerDamage, LastGoodHit.Location, LastGoodHit, OwnerCharacter->Controller, this, RamDamageTypeDefault);
}

FHitResult ADoorRam::TryGetHitPosition() const
{
	APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter();
	if (!OwnerCharacter)
		return FHitResult();

	const FVector StartTrace = (GetItemMesh()->GetComponentLocation() + GetItemMesh()->GetRightVector() * -50.0f);
	const FVector EndTrace = StartTrace + (OwnerCharacter->GetControlRotation().Vector().GetSafeNormal().RotateAngleAxis(-20.0f, FVector::UpVector) + OwnerCharacter->GetActorForwardVector()) * MaxHitDistance;

	FCollisionObjectQueryParams CollisionObjectQuery;
	CollisionObjectQuery.AddObjectTypesToQuery(ECC_DOOR);
	CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	CollisionObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	CollisionQueryParams.AddIgnoredActors(OwnerCharacter->GetCollisionIgnoredActors());
	CollisionQueryParams.bTraceComplex = true;
	CollisionQueryParams.bReturnPhysicalMaterial = false;

	TArray<FHitResult> HitResults;
	GetWorld()->LineTraceMultiByObjectType(HitResults, StartTrace, EndTrace, CollisionObjectQuery, CollisionQueryParams);

	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bDrawDebugTraces)
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Red, false, 1.0f, 0, 2.0f);
	#endif

	for (const FHitResult& HitResult : HitResults)
	{
		if (CanHitActor(HitResult))
		{
			return HitResult;
		}
	}

	return FHitResult();
}

bool ADoorRam::CanHitActor(const FHitResult& TestHit) const
{
	if (TestHit.GetActor())
	{
		for (int32 i = 0; i < AcceptableHitWhitelist.Num(); i++)
		{
			if (TestHit.GetActor()->IsA(AcceptableHitWhitelist[i]) && TestHit.GetActor() != GetOwner())
			{
				return true;
			}
		}
	}
	
	return false;
}

bool ADoorRam::IsBlockingAnimationPlaying(const TArray<EBlockingAnimationExclusion> Exclusions) const
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc && AnimationData)
	{
		if (pc->Is1PMontagePlaying(AnimationData->FireSingle[0].Body_FP))
		{
			return true;
		}
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}
