// Copyright Void Interactive, 2022

#include "SoundPropagator.h"

ASoundPropagator::ASoundPropagator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f;

	SetCanBeDamaged(false);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	
	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD/T_HUD_Sound_3_BO.T_HUD_Sound_3_BO'"));
	
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetSprite(Icon.Object);
	#endif

	PropagationSwitchEnter = CreateDefaultSubobject<UBoxComponent>(TEXT("Propagation Switch Enter"));
	PropagationSwitchEnter->SetupAttachment(GetRootComponent());
	PropagationSwitchEnter->SetRelativeLocation(FVector(220.0f, 0.0f, 0.0f));
	PropagationSwitchEnter->SetBoxExtent(FVector(200.0f));
	PropagationSwitchEnter->OnComponentBeginOverlap.AddDynamic(this, &ASoundPropagator::OnPropagationEnterOverlap);
	
	PropagationSwitchExit = CreateDefaultSubobject<UBoxComponent>(TEXT("Propagation Switch Exit"));
	PropagationSwitchExit->SetupAttachment(GetRootComponent());
	PropagationSwitchExit->SetRelativeLocation(FVector(-220.0f, 0.0f, 0.0f));
	PropagationSwitchExit->SetBoxExtent(FVector(200.0f));
	PropagationSwitchExit->OnComponentBeginOverlap.AddDynamic(this, &ASoundPropagator::OnPropagationExitOverlap);
}

void ASoundPropagator::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASoundPropagator::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	//GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, )
}

void ASoundPropagator::OnPropagationEnterOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(OtherActor))
	{
		float CurrentHealth = Character->GetCurrentHealth();
	}
}

void ASoundPropagator::OnPropagationExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}
