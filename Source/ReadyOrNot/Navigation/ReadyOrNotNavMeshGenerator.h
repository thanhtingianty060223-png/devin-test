// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMeshGenerator.h"

/**
 * 
 */
class READYORNOT_API FReadyOrNotNavMeshGenerator : public FRecastNavMeshGenerator
{
public:
	FReadyOrNotNavMeshGenerator(ARecastNavMesh& InDestNavMesh);
	
	/** Adds tile to PendingDirtyTiles if not already in, SeedDistance is set to SeedDistance and Pending Tiles is sorted,*/
	void SetSeedDistanceForTiles(TArray<FIntPoint> Tiles, float SeedDistance);
};
