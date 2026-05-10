// Void Interactive, 2020

#include "CollectedEvidenceActor.h"

ACollectedEvidenceActor::ACollectedEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.033f;

	bReplicates = true;
	bAlwaysRelevant = true;
	
	CollectionBagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagMesh"));
	CollectionBagMesh->SetGenerateOverlapEvents(false);
	CollectionBagMesh->bIgnoreRadialForce = true;
	CollectionBagMesh->bIgnoreRadialImpulse = true;
	CollectionBagMesh->bApplyImpulseOnDamage = false;
	CollectionBagMesh->bApplyImpulseOnDamage = false;
	CollectionBagMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	CollectionBagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionBagMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollectionBagMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);	
	CollectionBagMesh->SetSimulatePhysics(true);
	CollectionBagMesh->SetCanEverAffectNavigation(false);
	CollectionBagMesh->bNavigationRelevant = false;
	
	SetRootComponent(CollectionBagMesh);
	
	AActor::SetReplicateMovement(true);
}

bool ACollectedEvidenceActor::IsSecured_Implementation() const
{
	return IsHidden();
}

bool ACollectedEvidenceActor::CanBeSecured_Implementation() const
{
	return false;
}

bool ACollectedEvidenceActor::CanBeSecuredByTrailers_Implementation() const
{
	return !IsHidden();
}

FVector ACollectedEvidenceActor::GetLocation_Implementation() const
{
	return CollectionBagMesh->GetComponentLocation() + FVector::UpVector * 20.0f;
}

void ACollectedEvidenceActor::BeginPlay()
{
	Super::BeginPlay();

	PlaySpawnSound();
}

void ACollectedEvidenceActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetGameTimeSinceCreation() > 1.0f && GetVelocity().IsZero())
	{
		CollectionBagMesh->SetSimulatePhysics(false);
		CollectionBagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollectionBagMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		CollectionBagMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);	

		SetActorTickEnabled(false);
	}
	
	CollectionBagMesh->SetUseCCD(true);
}

void ACollectedEvidenceActor::PlaySpawnSound()
{
	if (HasAuthority())
	{
		Multicast_PlaySpawnSound();
	}
	else
	{
		Server_PlaySpawnSound();
	}
}

void ACollectedEvidenceActor::Server_PlaySpawnSound_Implementation()
{
	Multicast_PlaySpawnSound();
}

void ACollectedEvidenceActor::Multicast_PlaySpawnSound_Implementation()
{
	if (Bag_Spawn_Sound)
	{
		UFMODBlueprintStatics::PlayEventAttached(Bag_Spawn_Sound, CollectionBagMesh, FName(), FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	}
}
