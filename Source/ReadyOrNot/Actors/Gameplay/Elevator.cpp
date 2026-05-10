// Copyright Void Interactive, 2017

#include "Elevator.h"
#include "ReadyOrNot.h"


// Sets default values
AElevator::AElevator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ElevatorPath = CreateDefaultSubobject<USplineComponent>(TEXT("ElevatorPath"));
	SetRootComponent(ElevatorPath);

	ElevatorMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Elevator");
	ElevatorMesh->SetupAttachment(RootComponent);

	DestinationReachedSound = CreateDefaultSubobject<UFMODAudioComponent>("DestinationReachedSound");
	DestinationReachedSound->SetupAttachment(ElevatorMesh);

	FloorReachedSound = CreateDefaultSubobject<UFMODAudioComponent>("FloorReachedSound");
	FloorReachedSound->SetupAttachment(ElevatorMesh);

	DoorCloseSoundFMOD = CreateDefaultSubobject<UFMODAudioComponent>("DoorCloseSound");
	DoorCloseSoundFMOD->SetupAttachment(ElevatorMesh);

	DoorOpenSoundFMOD = CreateDefaultSubobject<UFMODAudioComponent>("DoorOpenSound");
	DoorOpenSoundFMOD->SetupAttachment(ElevatorMesh);
}

void AElevator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AElevator, bMoveElevator);
	DOREPLIFETIME(AElevator, SelectedFloor);
	DOREPLIFETIME(AElevator, bCloseDoors);
}

// Called when the game starts or when spawned
void AElevator::BeginPlay()
{
	Super::BeginPlay();
	DefaultElevatorLoc = ElevatorMesh->GetComponentLocation();
	Floors = GetPath()->GetNumberOfSplinePoints();
}

// Called every frame
void AElevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector TargetElevatorLoc = ElevatorPath->GetWorldLocationAtSplinePoint(SelectedFloor);
	if (bMoveElevator)
	{
		ElevatorMesh->SetWorldLocation(FMath::VInterpConstantTo(ElevatorMesh->GetComponentLocation(), TargetElevatorLoc, DeltaTime, ElevatorSpeed));

		// Figure out whether we should play the floor change sound
		for (int32 i = 0; i < ElevatorPath->GetNumberOfSplinePoints(); i++)
		{
			FVector Location = ElevatorPath->GetWorldLocationAtSplinePoint(i);

			if (ElevatorMesh->GetComponentLocation().Equals(Location, 25.0f))
			{
				currentFloor = i;

				if (i == SelectedFloor)
				{
					bMoveElevator = false;
					bCloseDoors = false;
					Multicast_PlayDestinationReachedSound();
					Multicast_PlayDestinationReachedSound_Implementation();
					Multicast_PlayDoorOpenSound();
					Multicast_PlayDoorOpenSound_Implementation();
				}
				else
				{
					Multicast_PlayFloorReachedSound();
					Multicast_PlayFloorReachedSound_Implementation();
				}

				break;
			}
		}
	}
}

bool AElevator::ShouldTickIfViewportsOnly()  const
{
	return true;
}

void AElevator::StartMovingElevator()
{
	bMoveElevator = true;
}

void AElevator::SetSelectedFloor(int32 Floor)
{
	if (GetLocalRole() < ROLE_Authority)
		Server_SetSelectedFloor(Floor);
	else
	{
		int32 oldSelectedFloor = SelectedFloor;
		currentFloor = SelectedFloor;
		SelectedFloor = FMath::Clamp(Floor, 0, Floors - 1);

		if (oldSelectedFloor != SelectedFloor)
		{
			bCloseDoors = true;
			bMoveElevator = false;
			GetWorld()->GetTimerManager().SetTimer(MoveElevatorDelay_Handle, this, &AElevator::StartMovingElevator, MoveElevatorDelay, false);
		}
	}
}

void AElevator::Server_SetSelectedFloor_Implementation(int32 Floor)
{
	SetSelectedFloor(Floor);
	Multicast_PlayDoorCloseSound();
	Multicast_PlayDoorCloseSound_Implementation();
}

void AElevator::Multicast_PlayDoorOpenSound_Implementation()
{
	DoorOpenSoundFMOD->Play();
}

void AElevator::Multicast_PlayDoorCloseSound_Implementation()
{
	DoorCloseSoundFMOD->Play();
}

void AElevator::Multicast_PlayFloorReachedSound_Implementation()
{
	FloorReachedSound->Play();
}

void AElevator::Multicast_PlayDestinationReachedSound_Implementation()
{
	DestinationReachedSound->Play();
}

void AElevator::Server_OpenCloseDoors_Implementation(bool bShouldCloseDoors)
{
	if (bMoveElevator)
	{
		return; // can't open or close the doors while the elevator is moving!
	}

	if (bShouldCloseDoors && !bCloseDoors)
	{
		bCloseDoors = true;
		Multicast_PlayDoorCloseSound();
		Multicast_PlayDoorCloseSound_Implementation();
	}
	else if(!bShouldCloseDoors && bCloseDoors)
	{
		bCloseDoors = false;
		Multicast_PlayDoorOpenSound();
		Multicast_PlayDoorOpenSound_Implementation();
	}
}