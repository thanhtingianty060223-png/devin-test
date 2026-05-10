// Copyright Void Interactive, 2023


#include "ReadyOrNotNavMeshGenerator.h"

FReadyOrNotNavMeshGenerator::FReadyOrNotNavMeshGenerator(ARecastNavMesh& InDestNavMesh) : FRecastNavMeshGenerator(InDestNavMesh)
{
	
}

void FReadyOrNotNavMeshGenerator::SetSeedDistanceForTiles(TArray<FIntPoint> Tiles, float SeedDistance)
{
	for (FIntPoint Tile : Tiles)
	{
		FPendingTileElement* PendingTileElement = PendingDirtyTiles.FindByPredicate([&](const FPendingTileElement& Element){return Element == Tile;});
		if (!PendingTileElement)
		{
			FPendingTileElement NewElement;
			NewElement.Coord = Tile;
			NewElement.SeedDistance = SeedDistance;
			PendingDirtyTiles.Add(NewElement);
		}
		else
		{
			PendingTileElement->SeedDistance = SeedDistance;
		}
			
	}

	PendingDirtyTiles.Sort();
}
	