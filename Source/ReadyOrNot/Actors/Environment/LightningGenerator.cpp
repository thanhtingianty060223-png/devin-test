#include "LightningGenerator.h"
#include "ReadyOrNot.h"

ALightningGenerator::ALightningGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = SceneRoot;

	Thunder = CreateDefaultSubobject<UAudioComponent>(TEXT("Thunder"));
	Thunder->SetupAttachment(SceneRoot);

	Lightning = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Lightning"));
	Lightning->SetupAttachment(SceneRoot);

	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));
	ParticleComponent->SetupAttachment(SceneRoot);
	ParticleComponent->bAutoActivate = false;
}

void ALightningGenerator::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(LightningHandle, this, &ALightningGenerator::PlayLightning,
		FMath::RandRange(LightningDelayMin, LightningDelayMax));
}

void ALightningGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Lightning->Intensity > 0.0f)
	{
		// Decay the intensity
		float NewIntensity = Lightning->Intensity;
		NewIntensity -= LightningIntensityDecay * DeltaSeconds;

		// Jitter the intensity
		LightningJitterTimeRemaining -= DeltaSeconds;
		if (LightningJitterTimeRemaining <= 0.0f)
		{
			NewIntensity += FMath::RandRange(LightningIntensityJitterMin, LightningIntensityJitterMax);
			LightningJitterTimeRemaining = FMath::RandRange(LightningIntensityJitterTimeMin, LightningIntensityJitterTimeMax);
		}

		// Cap the intensity
		if (NewIntensity < 0.0f)
		{
			NewIntensity = 0.0f;
		}

		Lightning->SetIntensity(NewIntensity);
	}
}

void ALightningGenerator::PlayLightning_Implementation()
{
	// Randomize intensity, color, and then do the lighting thing
	FLinearColor Color;
	Color.A = 1.0f;
	Color.R = FMath::RandRange(LightningColorMin.R, LightningColorMax.R);
	Color.G = FMath::RandRange(LightningColorMin.G, LightningColorMax.G);
	Color.B = FMath::RandRange(LightningColorMin.B, LightningColorMax.B);
	Lightning->SetLightColor(Color);
	Lightning->Intensity = FMath::RandRange(LightningIntensityMin, LightningIntensityMax);

	// Play the particle effect, if we can
	if (FMath::FRand() < ParticleSpawnChance && ParticleTemplates.Num() > 0)
	{
		ParticleComponent->SetTemplate(ParticleTemplates[FMath::RandRange(0, ParticleTemplates.Num() - 1)]);
		ParticleComponent->Activate();
	}

	// Start the thunder timer
	GetWorld()->GetTimerManager().SetTimer(ThunderHandle, this, &ALightningGenerator::PlayThunder,
		FMath::RandRange(ThunderDelayMin, ThunderDelayMax));
}


void ALightningGenerator::PlayThunder_Implementation()
{
	// Pick a sound and play it
	if (ThunderSounds.Num() != 0)
	{
		int32 Num = FMath::RandRange(0, ThunderSounds.Num() - 1);
		Thunder->SetSound(ThunderSounds[Num]);
		Thunder->Play();
	}

	// Start the lightning timer
	GetWorld()->GetTimerManager().SetTimer(LightningHandle, this, &ALightningGenerator::PlayLightning,
		FMath::RandRange(LightningDelayMin, LightningDelayMax));
}
