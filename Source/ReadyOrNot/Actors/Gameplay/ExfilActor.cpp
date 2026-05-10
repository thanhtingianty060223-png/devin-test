// Copyright Void Interactive, 2023


#include "Actors/Gameplay/ExfilActor.h"

#include "Actors/Environment/ExfilPortal.h"

// Sets default values
AExfilActor::AExfilActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bIsExfilEnabled = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base Mesh"));
	RootComponent = BaseMesh;
	RootComponent->Mobility = EComponentMobility::Static;
	Tags.Add("Exfil");
	
}

// Called when the game starts or when spawned
void AExfilActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AExfilActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AExfilActor::CanIssueCommand_Implementation() const
{
	return true;
}

AActor* AExfilActor::GetCommandActor_Implementation() const
{
	return const_cast<AExfilActor*>(this);
}


