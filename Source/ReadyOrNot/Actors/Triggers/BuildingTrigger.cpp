// Void Interactive, 2020

#include "BuildingTrigger.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"

ABuildingTrigger::ABuildingTrigger()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	OnActorBeginOverlap.AddDynamic(this, &ABuildingTrigger::OnBuildingEnter);
	OnActorEndOverlap.AddDynamic(this, &ABuildingTrigger::OnBuildingExit);

	bAuto = true;
	bUniformFloorSpacing = true;
	
	#if WITH_EDITORONLY_DATA
	bVisualizeFloors = true;
	bVisualizeFloorMidPoints = true;
	bVisualizeMinMaxExtents = false;
	#endif
}

void ABuildingTrigger::BeginPlay()
{
	Super::BeginPlay();

	GenerateFloors();

	#if WITH_EDITORONLY_DATA
	bVisualizeFloors = false;
	bVisualizeFloorMidPoints = false;
	bVisualizeMinMaxExtents = false;
	#endif
}

void ABuildingTrigger::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITORONLY_DATA
	if (bVisualizeFloors)
	{
		for (const FBuildingFloor& Floor : GeneratedFloors)
		{
			DrawDebugBox(GetWorld(), Floor.Location, Floor.Extent, FColor::Red, false, -1.0f, 0, 5.0f);
		}
	}

	if (bVisualizeFloorMidPoints)
	{
		for (const FBuildingFloor& Floor : GeneratedFloors)
		{
			DrawDebugSphere(GetWorld(), Floor.Location, 20.0f, 4, FColor::Blue, false, -1.0f, 0, 2.0f);
		}
	}

	if (bVisualizeMinMaxExtents)
	{
		const FVector BoxExtent = GetComponentsBoundingBox().GetExtent();
		const FVector HighestExtent = GetActorLocation() + BoxExtent;
		const FVector LowestExtent = GetActorLocation() - BoxExtent;
		DrawDebugSphere(GetWorld(), HighestExtent, 20.0f, 16, FColor::Green, false, -1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), LowestExtent, 20.0f, 16, FColor::Green, false, -1.0f, 0, 2.0f);
	}
	#endif
}

#if WITH_EDITOR
void ABuildingTrigger::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		GenerateFloors();
	}
}

bool ABuildingTrigger::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void ABuildingTrigger::OnBuildingEnter_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (OtherActor && OtherActor != this)
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
		{
			PlayerCharacter->InsideCurrentBuilding = this;
			PlayerCharacter->LastBuildingEntered = this;
			
			if (PlayerCharacter->HumanCharacterWidget_V2)
			{
				PlayerCharacter->HumanCharacterWidget_V2->UpdateMapFloors(GeneratedFloors);
			}
		}
	}
}

void ABuildingTrigger::OnBuildingExit_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (OtherActor && OtherActor != this)
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
		{
			PlayerCharacter->InsideCurrentBuilding = nullptr;

			if (PlayerCharacter->HumanCharacterWidget_V2)
			{
				PlayerCharacter->HumanCharacterWidget_V2->UpdateMapFloors(TArray<FBuildingFloor>());
			}
		}
	}
}

void ABuildingTrigger::GenerateFloors()
{
	const FVector BoxExtent = GetComponentsBoundingBox().GetExtent();
	const FVector LowestExtent = GetActorLocation() - BoxExtent;
	
	FVector LastLocation = LowestExtent;
	
	GeneratedFloors.Empty(NumberOfFloors);
	for (int32 i = 0; i < NumberOfFloors; i++)
	{
		FBuildingFloor Floor;
		Floor.Number = i+1;
		Floor.Height = bAuto ? BoxExtent.Z / NumberOfFloors : (bUniformFloorSpacing ? SpacingBetweenFloors : SpacingPerFloor.IsValidIndex(i) ? SpacingPerFloor[i] : 0.0f);
		Floor.Location = FVector(GetActorLocation().X, GetActorLocation().Y, LastLocation.Z + Floor.Height);
		Floor.Extent = FVector(BoxExtent.X, BoxExtent.Y, Floor.Height);

		if (FString* FoundFloorName = FloorNumberToFloorName.Find(Floor.Number))
		{
			Floor.Name = FText::FromString(*FoundFloorName);
		}
		else
		{
			Floor.Name = FText::FromString("Unknown");
		}
		
		GeneratedFloors.Add(Floor);

		LastLocation = Floor.Location + Floor.Height * GetActorUpVector();
	}
}

bool ABuildingTrigger::IsActorOnFloor(AActor* Actor, const int32 FloorNumber) const
{
	if (!Actor)
		return false;
	
	if (GeneratedFloors.IsValidIndex(FloorNumber-1))
	{
		const FBuildingFloor& Floor = GeneratedFloors[FloorNumber-1];

		if (UKismetMathLibrary::IsPointInBox(Actor->GetActorLocation(), Floor.Location, Floor.Extent))
			return true;

		if (Actor->GetActorLocation().Z >= Floor.Location.Z - Floor.Extent.Z && Actor->GetActorLocation().Z <= Floor.Location.Z + Floor.Extent.Z)
			return true;

		return false;
	}
	
	return false;
}

FVector ABuildingTrigger::GetFloorLocation(const int32 FloorNumber) const
{
	return GeneratedFloors.IsValidIndex(FloorNumber-1) ? GeneratedFloors[FloorNumber-1].Location : FVector::ZeroVector;
}

int32 ABuildingTrigger::GetFloorNumberFromActorLocation(AActor* Actor) const
{
	if (!Actor)
		return 0;

	int32 FloorNumber = 0;
	for (int32 i = 0; i < GeneratedFloors.Num(); i++)
	{
		if (IsActorOnFloor(Actor, GeneratedFloors[i].Number))
		{
			FloorNumber = GeneratedFloors[i].Number;
			break;
		}
	}

	return FloorNumber;
}
