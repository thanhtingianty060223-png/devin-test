// Void Interactive, 2020

#include "ReadyOrNotRecastNavMesh.h"

#include "ReadyOrNotNavigationSystem.h"
#include "Actors/ThreatAwarenessActor.h"
#include "Navigation/ReadyOrNotNavMeshGenerator.h"

AReadyOrNotRecastNavMesh::AReadyOrNotRecastNavMesh()
{
	bFixedTilePoolSize = true;
	TileSizeUU = 2044.0f;
	CellSize = 14.0f;
	TilePoolSize = 10000;
}

TArray<FVector> AReadyOrNotRecastNavMesh::GenerateStairPoints()
{
	FRecastDebugGeometry NavGeometry;
	NavGeometry.bGatherNavMeshEdges = true;

	BeginBatchQuery();
	GetDebugGeometry(NavGeometry, -1);
	FinishBatchQuery();

	// Process the navmesh vertices
	const TArray<FVector>& Vertices = NavGeometry.NavMeshEdges;
	const int32 NumVertices = Vertices.Num();

	TArray<FVector> StairPoints;
	StairPoints.Reserve(Vertices.Num());

	if (NumVertices > 1)
	{
		for (int32 i = 0; i < NumVertices; i += 2)
		{
			StairPoints += GenerateStairPoints_Internal(Vertices[i], Vertices[i+1]);
		}
	}

	int32 Index = 0;
	for (const FVector& Point : StairPoints)
	{
		if (AThreatAwarenessActor* StairThreat = GetWorld()->SpawnActor<AThreatAwarenessActor>(Point, FRotator::ZeroRotator))
		{
			StairThreat->CheckIsOutside();
			StairThreat->Tags.AddUnique("generated");
			StairThreat->DoorThreat = nullptr;
			StairThreat->SetThreatLevel(EThreatLevel::TL_Stairs);
			
			#if WITH_EDITOR
			StairThreat->SetFolderPath("GeneratedStairThreats");
			StairThreat->SetActorLabel("TAA_Stairs_" + FString::FromInt(Index));
			#endif

			Index++;
		}
	}

	return StairPoints;
}

void AReadyOrNotRecastNavMesh::BeginPlay()
{
	Super::BeginPlay();
}

FRecastNavMeshGenerator* AReadyOrNotRecastNavMesh::CreateGeneratorInstance()
{
	return new FReadyOrNotNavMeshGenerator(*this);
}

#if WITH_EDITOR
void AReadyOrNotRecastNavMesh::EditorTick(float DeltaSeconds)
{
	Super::EditorTick(DeltaSeconds);

	// stair detection debug rendering
	{
		StairDebugLines.Empty(100);
		
		FRecastDebugGeometry NavGeometry;
		NavGeometry.bGatherNavMeshEdges = true;

		BeginBatchQuery();
		GetDebugGeometry(NavGeometry, -1);
		FinishBatchQuery();

		// Process the navmesh vertices
		const TArray<FVector>& Vertices = NavGeometry.NavMeshEdges;
		const int32 NumVertices = Vertices.Num();

		if (NumVertices > 1)
		{
			for (int32 i = 0; i < NumVertices; i += 2)
			{
				GenerateStairPoints_Internal(Vertices[i], Vertices[i+1]);
			}
			
			if (StairDebugLines.Num() > 0)
			{
				GetWorld()->ForegroundLineBatcher->DrawLines(StairDebugLines);
				StairDebugLines.Empty(100);
			}
		}
	}
}
#endif

TArray<FVector> AReadyOrNotRecastNavMesh::GenerateStairPoints_Internal(const FVector& Vertex1, const FVector& Vertex2)
{
	const FVector Direction = (Vertex2 - Vertex1).GetSafeNormal();

	if (FMath::Abs(FVector::DotProduct(FVector::UpVector, Direction)) < 0.15f)
		return {};

	if (FVector::Distance(Vertex1, Vertex2) < 200.0f)
		return {};
	
	TArray<FVector> GeneratedPoints;
	
	const float EdgeLength = (Vertex2 - Vertex1).Size2D();
	const int32 AmountOfPoints = EdgeLength/MinDistanceBetweenCoverPoints;
	
	if (AmountOfPoints <= 0)
		return GeneratedPoints;
	
	const float IncrementAmount = AmountOfPoints == 1 ? 0.5f : 1.0f/AmountOfPoints;

	for (float j = 0.0f; j < 1.0f; j += IncrementAmount)
	{
		const FVector EdgeStart_NoOffset = FMath::Lerp(Vertex1, Vertex2, FMath::Clamp(j, 0.0f, 1.0f));

		FVector FinalLocation = EdgeStart_NoOffset + FVector::UpVector * 150.0f;
		GeneratedPoints.Add(FinalLocation);

		#if WITH_EDITOR
		if (bDrawStairPoints)
		{
			StairDebugLines.Add({Vertex1 + FVector::UpVector * 20.0f, Vertex2 + FVector::UpVector * 20.0f, FColor::Purple, 1.0f, 2.0f, 0});
			StairDebugLines.Add({FinalLocation, FinalLocation, FColor::Orange, 1.0f, 10.0f, 0});
		}
		#endif
	}

	return GeneratedPoints;
}
