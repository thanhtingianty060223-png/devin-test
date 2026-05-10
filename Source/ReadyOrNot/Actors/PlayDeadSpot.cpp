// Void Interactive, 2020


#include "Actors/PlayDeadSpot.h"

// Sets default values
APlayDeadSpot::APlayDeadSpot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlayDeadSpot::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayDeadSpot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

