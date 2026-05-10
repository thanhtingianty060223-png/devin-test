// Void Interactive, 2020


#include "Actors/PremissionStreetView.h"

// Sets default values
APremissionStreetView::APremissionStreetView()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Direction = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction"));
	SetRootComponent(Direction);
	LeftBuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftBuilding"));
	LeftBuildingMesh->SetupAttachment(Direction);
	RightBuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightBuilding"));
	RightBuildingMesh->SetupAttachment(Direction);
	LeftTrafficLight = CreateDefaultSubobject<UChildActorComponent>(TEXT("LeftTrafficLight"));
	LeftTrafficLight->SetupAttachment(Direction);
	RightTrafficLight = CreateDefaultSubobject<UChildActorComponent>(TEXT("RightTrafficLight"));
	RightTrafficLight->SetupAttachment(Direction);


}

// Called when the game starts or when spawned
void APremissionStreetView::BeginPlay()
{
	Super::BeginPlay();
	OriginalActorLocation = GetActorLocation();
	InterpConstantSpeed = 2000.0f;
	
}

// Called every frame
void APremissionStreetView::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	SetActorHiddenInGame(!GS || GS->TimeTillGameStartCountdown > 11.0f || GS->MatchState != EMatchState::MS_Warmup);
	if (TimeUntilReset <= 0.0f)
	{
		TimeUntilReset = FMath::RandRange(3.0f, 10.0f);
		InterpConstantSpeed = FMath::RandRange(2000.0f, 4000.0f);
		if (Buildings.Num() > 0)
		{
			LeftBuildingMesh->SetStaticMesh(Buildings[FMath::RandRange(0, Buildings.Num() - 1)]);
			RightBuildingMesh->SetStaticMesh(Buildings[FMath::RandRange(0, Buildings.Num() - 1)]);
		}

		if (TrafficLights.Num() > 0)
		{
			TSubclassOf<AActor> Light = TrafficLights[FMath::RandRange(0, TrafficLights.Num() - 1)];
			LeftTrafficLight->SetChildActorClass(Light);
			RightTrafficLight->SetChildActorClass(Light);
		}
	
		SetActorLocation(OriginalActorLocation);
	} else
	{
		TimeUntilReset -= DeltaTime; 
		SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), GetActorLocation() + Direction->GetComponentRotation().Vector() * InterpConstantSpeed * 2.0f, DeltaTime, InterpConstantSpeed));
	}

}

