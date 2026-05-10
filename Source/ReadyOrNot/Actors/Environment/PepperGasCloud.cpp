// Copyright Void Interactive, 2023


#include "PepperGasCloud.h"

#include "NavigationSystem.h"
#include "Info/CSGasManager.h"


// Sets default values
APepperGasCloud::APepperGasCloud()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	GasRadius = 500;

	GasParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>("Particle System");
	SetRootComponent(GasParticleSystem);
}

// Called when the game starts or when spawned
void APepperGasCloud::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(10);

	GasParticleSystem->Activate(true);
}

void APepperGasCloud::Destroyed()
{
	UCSGasManager* GasManager = UCSGasManager::Get(GetWorld());
	if (!GasManager)
	{
		return;
	}

	GasManager->RemoveGasSource(this);
}


// Called every frame
void APepperGasCloud::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool APepperGasCloud::IsReleasingGas_Implementation() const
{
	return true;
}

float APepperGasCloud::GetGasRadius_Implementation() const
{
	return GasRadius;
}

int32 APepperGasCloud::GetMaximumGasPoints_Implementation() const
{
	return MaxGasPoints;
}

bool APepperGasCloud::GetGasReleaseLocation_Implementation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys == nullptr)
	{
		return false;
	}

	FVector ProjectionExtents = FVector(50, 50, 500);
	FNavLocation ProjLoc;
	bool Result = NavSys->ProjectPointToNavigation(GetActorLocation(), ProjLoc, ProjectionExtents);

	OutLocation = ProjLoc.Location;
	return Result;
}
