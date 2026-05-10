// Void Interactive, 2020


#include "RandomizedShippingContainerActor.h"

// Sets default values
ARandomizedShippingContainerActor::ARandomizedShippingContainerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ContainerMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("ContainerMesh");
	SetRootComponent(ContainerMeshComp);

	ContainerDecalsComp = CreateDefaultSubobject<UStaticMeshComponent>("ContainerDecals");
	ContainerDecalsComp->SetupAttachment(ContainerMeshComp);

	LeftDoorDecalsComp = CreateDefaultSubobject<UStaticMeshComponent>("LeftDoorDecals");
	LeftDoorDecalsComp->SetupAttachment(ContainerMeshComp);

	RightDoorDecalsComp = CreateDefaultSubobject<UStaticMeshComponent>("RightDoorDecals");
	RightDoorDecalsComp->SetupAttachment(ContainerMeshComp);

}

// Called when the game starts or when spawned
void ARandomizedShippingContainerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARandomizedShippingContainerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARandomizedShippingContainerActor::RandomizeContainer()
{
	if (ContainerMaterials.Num() > 0)
	{
		ContainerMeshComp->SetMaterialByName("Container", ContainerMaterials[FMath::RandRange(0, ContainerMaterials.Num() - 1)]);
	}

	if (ContainerDecals.Num() > 0)
	{
		ContainerDecalsComp->SetStaticMesh(ContainerDecals[FMath::RandRange(0, ContainerDecals.Num() - 1)]);
	}

	if (LeftDoorDecals.Num() > 0)
	{
		LeftDoorDecalsComp->SetStaticMesh(LeftDoorDecals[FMath::RandRange(0, LeftDoorDecals.Num() - 1)]);
	}

	if (RightDoorDecals.Num() > 0)
	{
		RightDoorDecalsComp->SetStaticMesh(RightDoorDecals[FMath::RandRange(0, RightDoorDecals.Num() - 1)]);
	}
	
}

