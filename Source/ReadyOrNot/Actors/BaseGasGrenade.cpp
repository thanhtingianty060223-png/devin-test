// Copyright Void Interactive, 2023


#include "BaseGasGrenade.h"
#include "NavigationSystem.h"
#include "ReadyOrNotAIConfig.h"
#include "Components/MoraleComponent.h"
#include "Perception/AISense_Hearing.h"
#include "ReadyOrNotDebugSubsystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Info/CSGasManager.h"
#include "NavAreas/NavArea_Null.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "Navigation/ReadyOrNotNavAreas.h"
#include "Navigation/ReadyOrNotNavQueries.h"
#include "NavMesh/RecastNavMesh.h"

ABaseGasGrenade::ABaseGasGrenade()
{
}

void ABaseGasGrenade::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseGasGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ABaseGasGrenade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ABaseGasGrenade::Detonate()
{	
	CurrentDetonations++;
	V_LOGM(LogReadyOrNot, "Detonations: %i", CurrentDetonations);
	
	if (CurrentDetonations <= RedotonateCount)
	{
		StartDetonationTimer();
	}
	else if (CurrentDetonations > RedotonateCount)
	{
		UCSGasManager* CSGasManager = UCSGasManager::Get(GetWorld());
		if (IsValid(CSGasManager))
			CSGasManager->RemoveGasSource(this);
	}
	else if (CurrentDetonations > 1)
	{
		return;
	}

	if (CurrentDetonations == 1)
	{
		UCSGasManager* CSGasManager = UCSGasManager::Get(GetWorld());
		if (IsValid(CSGasManager))
			CSGasManager->AddGasSource(this);

		GetWorldTimerManager().ClearTimer(TH_RecordLocation);
		
		const float Damage = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.Damage");
		const float InnerRadius = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.DamageInnerRadius");
		const float OuterRadius = AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale.DamageOuterRadius");
		const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("GrenadeDetonateMorale.DamageFalloffCurve"));

		UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, ItemMesh->GetComponentLocation(), Damage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT}, Curve);
		
		//UMoraleComponent::ChangeMoraleInArea(ItemMesh->GetComponentLocation(), AI_CONFIG_GET_FLOAT("GrenadeDetonateMorale"), 1100.0f, false, {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT});
	}

	if ((CurrentDetonations >= 1 && bTriggerSFXOnRedetonate) || CurrentDetonations == 1 || (bPlayDetonationEffectsExactlyOnce && !bDetonationEffectsPlayed))
	{
		//UMoraleComponent::ChangeMoraleInArea(ItemMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 100.0f), -0.25f, 1100.0f, true, {ETeamType::TT_SUSPECT, ETeamType::TT_CIVILIAN});
		FVector CalculatedForce;

		CalculatedForce.X = FMath::FRandRange(-MaxRandomizedForceOnDetonation.X, MaxRandomizedForceOnDetonation.X);
		CalculatedForce.Y = FMath::FRandRange(-MaxRandomizedForceOnDetonation.Y, MaxRandomizedForceOnDetonation.Y);
		CalculatedForce.Z = FMath::FRandRange(-MaxRandomizedForceOnDetonation.Z, MaxRandomizedForceOnDetonation.Z);

		Multicast_DetonationEffects(CalculatedForce);

		DetonationLight->SetIntensity(DetonationFlashIntensitiy);

		bDetonationEffectsPlayed = true;
	}
	
	// Apply screen shake to all actors in radius --eez
	if (bUseScreenShake)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionScreenShake, ItemMesh->GetComponentLocation(), 0.0f, CameraShakeRadius);
	}
	
	for (int32 i = 0; i < DetonationDamage.Num(); i++)
	{
        if (DetonationRadialForce->ImpulseStrength > 0.0f)
        {
			DetonationRadialForce->FireImpulse();
        }
	}

	if (bDetonationTriggersStimuli)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetItemLocation(), 2.0f, this, DetonationSoundMaxRange, DetonationTag);
	}
	
	// detonation removes the hearing event
	DetonationStimuliComp->UnregisterFromSense(UAISense_Hearing::StaticClass());

	OnGrenadeDetonated.Broadcast(this);
}

void ABaseGasGrenade::OnItemUseComplete()
{
	Super::OnItemUseComplete();
}

bool ABaseGasGrenade::IsReleasingGas_Implementation() const
{
	return true;
}

float ABaseGasGrenade::GetGasRadius_Implementation() const
{
	return DetonationDamage[0].DamageOuterRadius;
}

int32 ABaseGasGrenade::GetMaximumGasPoints_Implementation() const
{
	return MaxGasPoints;
}

bool ABaseGasGrenade::GetGasReleaseLocation_Implementation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys == nullptr)
	{
		return false;
	}

	FVector ProjectionExtents = FVector(70, 70,250);

	FNavLocation ProjLoc;
	bool Result;
	Result = NavSys->ProjectPointToNavigation(GetActorLocation(), ProjLoc, ProjectionExtents);

	if (Result)
	{
		OutLocation = ProjLoc.Location;
		return true;
	}
	
	// for (int i = PastLocations.Num() - 1; i >= 0; i--)
	// {
	// 	DrawDebugPoint(GetWorld(), PastLocations[i], 50, FColor::Purple, false, 20);
	// }

	FHitResult Hit;
	for (int i = PastLocations.Num() - 1; i >= 0; i--)
	{
		FVector TestLocation = PastLocations[i];

		// Don't bother with a bounce an unreasonable distance away from its current location
		float DistanceFromCurrentLocation = FVector::Distance(TestLocation, ItemMesh->GetComponentLocation());
		if (DistanceFromCurrentLocation > 500)
			continue;
 
		FCollisionObjectQueryParams QueryParams(ECollisionChannel::ECC_WorldStatic);
		GetWorld()->LineTraceSingleByObjectType(Hit, TestLocation, TestLocation - FVector(0.f, 0.f, 500.f), QueryParams);
		
		// DrawDebugLine(GetWorld(), TestLocation, Hit.bBlockingHit ? Hit.Location : TestLocation - FVector(0.f, 0.f, 500.f), FColor::Red, false, 20, 0, 5);
		
		TestLocation = Hit.bBlockingHit ? Hit.Location : TestLocation;
		
		Result = NavSys->ProjectPointToNavigation(TestLocation, ProjLoc, ProjectionExtents);
		if (Result)
		{
			// We don't want to take this point if it's gone decently vertically above as that could be going through floors
			if (ProjLoc.Location.Z - TestLocation.Z > 50)
				continue;
			
			OutLocation = ProjLoc.Location;
			return true;
		}
	}

	return false;
}

void ABaseGasGrenade::Throw(bool bLocalOnly, bool bOverarmThrow, const FVector& ThrowDirection, const FVector& ThrowStart)
{
	Super::Throw(bLocalOnly, bOverarmThrow, ThrowDirection, ThrowStart);
	
	GetWorldTimerManager().SetTimer(TH_RecordLocation, this, &ABaseGasGrenade::RecordLocation, LocationRecordingRate, true);
	PastLocations.Reset();
	PastLocations.Reserve(DetonationTime * LocationRecordingRate);

	// Add the throwers location as the first recorded location as a backup that'll almost certainly be on a navmesh
	PastLocations.Emplace(GetOwner()->GetActorLocation());
}

void ABaseGasGrenade::RecordLocation()
{
	if (!PastLocations.Num() || (FVector::DistSquared(ItemMesh->GetComponentLocation(), PastLocations.Last()) > FMath::Square(RecordDistanceThreshold)))
		PastLocations.Emplace(ItemMesh->GetComponentLocation());
}

void ABaseGasGrenade::OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnPhysicsImpact(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	if (ItemMesh->GetComponentVelocity().Size() > 300.0f)
	{
		BounceLocations.Emplace(Hit.Location);
		//DrawDebugPoint(GetWorld(), Hit.Location, 80, FColor::Red, false, 30);
	}
}