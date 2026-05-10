// Copyright Void Interactive, 2021


#include "BulletTracer.h"

// Sets default values
ABulletTracer::ABulletTracer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	RootComp = CreateDefaultSubobject<USceneComponent>("RootComp");
	SetRootComponent(RootComp);
	static ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleSystem(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXImpacts/P_BulletTracer.P_BulletTracer'"));
	TracerParticle = ParticleSystem.Object;
	
	static ConstructorHelpers::FObjectFinder<UParticleSystem> SmokeParticleSystem(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXDetonation/P_BulletTrail.P_BulletTrail'"));
	SmokeParticle = SmokeParticleSystem.Object;
}

// Called when the game starts or when spawned
void ABulletTracer::BeginPlay()
{
	Super::BeginPlay();

	SetActorHiddenInGame(false);
	
}

// Called every frame
void ABulletTracer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ParticleComponent)
	{
		if (bInterpLoc)
		{
			SetActorLocation(UKismetMathLibrary::VInterpTo_Constant(GetActorLocation(), EndLoc, DeltaTime, 11000.0f));
			if ((GetActorLocation() - EndLoc).Size() < 100.0f)
			{
				ParticleComponent->ReleaseToPool();
				ParticleComponent = nullptr;
			}
		}
	}
	
}

void ABulletTracer::StartParticle(FVector Start, FVector End)
{
	bIsSmoke = false;
	if (ParticleComponent)
	{
		ParticleComponent->ReleaseToPool();
	}
	ParticleComponent = UGameplayStatics::SpawnEmitterAttached(TracerParticle, GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, EPSCPoolMethod::ManualRelease);
	StartLoc = Start + UKismetMathLibrary::Normal(End - Start) * 170;
	EndLoc = End;
	SetActorLocation(StartLoc);
	SetLifeSpan(10.0f);
	bInterpLoc = true;
}

void ABulletTracer::StartSmoke(FVector Start, FVector End)
{
	SetActorHiddenInGame(false);
	bIsSmoke = true;
	if (ParticleComponent)
	{
		ParticleComponent->ReleaseToPool();
	}
	ParticleComponent = UGameplayStatics::SpawnEmitterAttached(SmokeParticle, GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, EPSCPoolMethod::ManualRelease);
	StartLoc = Start + UKismetMathLibrary::Normal(End - Start) * 170;
	EndLoc = End;
	SetActorLocation(StartLoc);
	SetLifeSpan(10.0f);
	bInterpLoc = true;
}

