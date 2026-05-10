// Void Interactive, 2020

#include "Actors/PolicePresenceTestActor.h"

#include "Actors/ThreatAwarenessActor.h"
#include "NavigationSystem.h"
#include "WorldDataGenerator.h"
#include "Navigation/ReadyOrNotNavQueries.h"

// Sets default values
APolicePresenceTestActor::APolicePresenceTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	Police = CreateDefaultSubobject<UBillboardComponent>(TEXT("Police"));

	Suspect = CreateDefaultSubobject<UBillboardComponent>(TEXT("Suspect"));

}

// Called when the game starts or when spawned
void APolicePresenceTestActor::BeginPlay()
{
	Super::BeginPlay();
	Destroy();
	
}

// Called every frame
void APolicePresenceTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AWorldDataGenerator* WorldDataGenerator = *TActorIterator<AWorldDataGenerator>(GetWorld());
	WorldDataGenerator->UnblockAllDoorways();

	for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
	{
		It->EnableNavLink();
	}

	
	AThreatAwarenessActor* SuspectThreat = WorldDataGenerator->GetNearestThreat(Suspect->GetComponentLocation(), false, true);
	if (SuspectThreat)
	{
		SuspectThreat->GenerateUniqueExits();
		int32 z = 0;
		
		TArray<ADoor*> Doors;
		SuspectThreat->GetUniqueExtis(Doors);
		for (ADoor* D : Doors)
		{
			TArray<AThreatAwarenessActor*> OutExitPoints;
			z++;
			FVector SuspectNoZ = Suspect->GetComponentLocation();
			SuspectNoZ.Z = 0.0f;
			FVector DoorNoZ = D->GetActorLocation();
			DoorNoZ.Z = 0.0f; 
			FVector PoliceNoZ = Police->GetComponentLocation();
			PoliceNoZ.Z = 0.0f;
			const FVector V1 = UKismetMathLibrary::FindLookAtRotation(SuspectNoZ, PoliceNoZ).Vector();;
			const FVector V2 = UKismetMathLibrary::FindLookAtRotation(SuspectNoZ,  DoorNoZ).Vector();

			float DotProduct = FVector::DotProduct(V1, V2);
			GEngine->AddOnScreenDebugMessage(9999999+z, 1.0f, FColor::White, D->GetName() + " " + FString::SanitizeFloat(DotProduct));
			if (DotProduct < -0.5f)
			{
				if (!D->FrontThreatAwarenessPoints.Contains(SuspectThreat))
				{
					//Back
					OutExitPoints.Append(D->FrontThreatAwarenessPoints);
				}
				if (!D->BackThreatAwarenessPoints.Contains(SuspectThreat))
				{
					// Front
					OutExitPoints.Append(D->BackThreatAwarenessPoints);
				}
				OutExitPoints.Remove(nullptr);
				for (int32 i = 0; i < (OutExitPoints.Num() > 10 ? 10 : OutExitPoints.Num() - 1); i++)
				{
					if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						FPathFindingQuery PathFindingQuery;
						PathFindingQuery.StartLocation = Suspect->GetComponentLocation();
						PathFindingQuery.EndLocation = OutExitPoints[i]->GetActorLocation();
						PathFindingQuery.SetAllowPartialPaths(true);
						const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_NoiseCheck::StaticClass();
						const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
						PathFindingQuery.QueryFilter = QueryFilter;
						const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
						if (PathFindingResult.Result == ENavigationQueryResult::Success)
						{					
							for (int32 y = 1; y < PathFindingResult.Path->GetPathPoints().Num(); y++)
							{
								DrawDebugLine(GetWorld(),  PathFindingResult.Path->GetPathPoints()[y-1], PathFindingResult.Path->GetPathPoints()[y], FColor::White, false, DeltaTime + 0.001f, 0, 1);
            				
							}
						}
					}
				}
			}
			
		}
		

		
	}
}

