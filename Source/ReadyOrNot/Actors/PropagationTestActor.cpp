// Copyright Void Interactive, 2022


#include "PropagationTestactor.h"

#include "Components/FMODAudioPropagationComponent.h"
#include "Info/SoundManager.h"


// Sets default values
APropagationTestactor::APropagationTestactor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = TickInterval;

	static ConstructorHelpers::FObjectFinder<UTexture2D> SoundIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD/T_HUD_Sound_3_BO'"));
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Open Billboard Component (Front)"));
	Billboard->SetSprite(SoundIcon.Object);
	Billboard->bIsScreenSizeScaled = true;
	Billboard->bIsEditorOnly = true;

	OcclusionType = EOcclusionType::OT_Angular;
	PropagationType = EPropagationType::PT_Portal;

	SetRootComponent(Billboard);
}

// Called when the game starts or when spawned
void APropagationTestactor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APropagationTestactor::Tick(float DeltaTime)
{
	PrimaryActorTick.TickInterval = TickInterval;
	Super::Tick(DeltaTime);

	if(Event)
	{
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), Event, FTransform(FRotator(0,0,0), GetActorLocation(), FVector(1,1,1)), {}, OcclusionType, PropagationType, bDebugMode);
		SoundSource->Play();
	}
}

