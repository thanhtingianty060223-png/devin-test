// Void Interactive, 2020

#pragma once

#include "NotifyingRecastNavMesh.h"
#include "ReadyOrNotRecastNavMesh.generated.h"

UCLASS()
class READYORNOT_API AReadyOrNotRecastNavMesh final : public ANotifyingRecastNavMesh
{
	GENERATED_BODY()

public:
	AReadyOrNotRecastNavMesh();

	TArray<FVector> GenerateStairPoints();
	
protected:
	virtual void BeginPlay() override;
	
	virtual FRecastNavMeshGenerator* CreateGeneratorInstance() override;

	TArray<FVector> GenerateStairPoints_Internal(const FVector& Vertex1, const FVector& Vertex2);
	
	UPROPERTY(EditInstanceOnly)
	bool bDrawStairPoints = false;

	#if WITH_EDITOR
	virtual void EditorTick(float DeltaSeconds) override;
	TArray<FBatchedLine> StairDebugLines;
	#endif
};
