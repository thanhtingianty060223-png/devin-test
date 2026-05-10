// Copyright Void Interactive, 2021

#include "PickupActor.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/InteractableComponent.h"

APickupActor::APickupActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	SetCanBeDamaged(false);
	AActor::SetReplicateMovement(true);
	
	bReplicates = true;
	bAlwaysRelevant = true;

	bFindCameraComponentWhenViewTarget = false;
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupSkeletalMesh"));
	SkeletalMesh->SetCollisionProfileName("Item");
	SkeletalMesh->SetSimulatePhysics(true);
	SetRootComponent(SkeletalMesh);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupStaticMesh"));
	StaticMesh->SetCollisionProfileName("Item");
	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetRelativeLocation(FVector::ZeroVector);
	StaticMesh->SetupAttachment(SkeletalMesh);

	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component"));
	ObjectiveMarkerComponent->SetRelativeLocation(FVector::ZeroVector);
	ObjectiveMarkerComponent->SetIsReplicated(true);
	ObjectiveMarkerComponent->bEnabled = false;
	ObjectiveMarkerComponent->SetRelativeLocation(FVector::ZeroVector);
	ObjectiveMarkerComponent->SetupAttachment(SkeletalMesh);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->ShowPromptAtDistance = 200.0f;
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Pickup " + PickupName.ToString()));
	InteractableComponent->SetupAttachment(SkeletalMesh);
}

void APickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickupActor, PickupInstigator);
}

void APickupActor::ActorPickedUp(AActor* InPickupInstigator)
{
	PickupInstigator = InPickupInstigator;
	
	OnActorPickedUp.Broadcast(this);
	OnActorPickedUp_NoParam.Broadcast();
}

void APickupActor::ActorDropped(AActor* InDroppedInstigator)
{
	OnActorDropped.Broadcast(this);
}

void APickupActor::ToggleObjectiveMarker()
{
	if (ObjectiveMarkerComponent)
	{
		ObjectiveMarkerComponent->ToggleObjectiveMarkerVisibility();
	}
}

void APickupActor::ShowObjectiveMarker()
{
	if (ObjectiveMarkerComponent)
	{
		ObjectiveMarkerComponent->ShowObjectiveMarker();
	}
}

void APickupActor::HideObjectiveMarker()
{
	if (ObjectiveMarkerComponent)
	{
		ObjectiveMarkerComponent->HideObjectiveMarker();
	}
}

void APickupActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent)
{
	ActorPickedUp(InteractInstigator);
}
