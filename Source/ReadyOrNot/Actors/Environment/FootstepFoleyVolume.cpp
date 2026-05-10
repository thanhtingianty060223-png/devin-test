// Copyright Void Interactive, 2022

#include "FootstepFoleyVolume.h"
#include "ReadyOrNot.h"

AFootstepFoleyVolume::AFootstepFoleyVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	if (GetBrushComponent())
	{
		GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
		GetBrushComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		GetBrushComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> AudioIcon(TEXT("/Engine/EditorResources/AudioIcons/S_AudioComponent.S_AudioComponent"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetMobility(EComponentMobility::Movable);
	BillboardComponent->SetSprite(AudioIcon.Object);
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->bUseInEditorScaling = true;
	BillboardComponent->EditorScale = 0.75f;
#endif
}

void AFootstepFoleyVolume::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.RemoveAll(this);
	OnActorBeginOverlap.AddDynamic(this, &AFootstepFoleyVolume::OnOverlapBegin);

	OnActorEndOverlap.RemoveAll(this);
	OnActorEndOverlap.AddDynamic(this, &AFootstepFoleyVolume::OnOverlapEnd);
}

void AFootstepFoleyVolume::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(OtherActor);
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (bNPCsTriggerFootstepFoley ? Character : PlayerCharacter)
	{
		UFMODEvent* FirstPersonEvent = FootstepFoleyEventFirstPerson ? FootstepFoleyEventFirstPerson : FootstepFoleyEvent;
		UFMODEvent* ThirdPersonEvent = FootstepFoleyEvent;
		Character->StartFoley(true, FirstPersonEvent, ThirdPersonEvent);
	}
}

void AFootstepFoleyVolume::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(OtherActor);
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (bNPCsTriggerFootstepFoley ? Character : PlayerCharacter)
	{
		Character->StopFoley();
	}
}
