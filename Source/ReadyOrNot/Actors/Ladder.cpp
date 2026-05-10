// Copyright Void Interactive, 2017

#include "Ladder.h"
#include "ReadyOrNot.h"


// Sets default values
ALadder::ALadder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	RootComponent = BoxCollision;

	LadderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ladder"));
	LadderMesh->SetSimulatePhysics(false);
	LadderMesh->SetupAttachment(BoxCollision);
	


}

// Called when the game starts or when spawned
void ALadder::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALadder::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void ALadder::SetLadderLength(float NewLength)
{
	if (BoxCollision)
	{
		FVector b = BoxCollision->GetScaledBoxExtent();

		// Box extends both up and down from the origin so divide by 2
		b.Z = NewLength / 2;
		BoxCollision->SetBoxExtent(b);

		// Box extends both ways so end vector should land in the middle
		AddActorWorldOffset(FVector(0.0f, 0.0f, b.Z / 2.0f));
	}
}
