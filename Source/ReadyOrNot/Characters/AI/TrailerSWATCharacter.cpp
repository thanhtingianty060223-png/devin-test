// Copyright Void Interactive, 2023

#include "Characters/AI/TrailerSWATCharacter.h"

ATrailerSWATCharacter::ATrailerSWATCharacter()
{
	ScoringComponent->bEnabled = false;

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	GetMesh()->bNoSkeletonUpdate = true;
	GetMesh()->bEnableUpdateRateOptimizations = true;
	GetFaceMesh()->bEnableUpdateRateOptimizations = true;
}

void ATrailerSWATCharacter::Tick(float DeltaSeconds)
{
	if (bDeactivated)
	{
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		GetMesh()->bNoSkeletonUpdate = true;
		GetMesh()->bEnableUpdateRateOptimizations = true;
		GetFaceMesh()->bEnableUpdateRateOptimizations = true;
		
		return;
	}
	
	Super::Tick(DeltaSeconds);
}

void ATrailerSWATCharacter::Tick_Authority(float DeltaSeconds)
{
	if (bDeactivated)
		return;
	
	Super::Tick_Authority(DeltaSeconds);
}

bool ATrailerSWATCharacter::CanBeSecuredByTrailers_Implementation() const
{
	return false;
}

bool ATrailerSWATCharacter::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation,
	int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible,
	int32* UserData) const
{
	return false;
}

void ATrailerSWATCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
}
