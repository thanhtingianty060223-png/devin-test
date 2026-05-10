// Copyright Void Interactive, 2023

#pragma once

#include "GraphNode.h"
#include "RoomVolume.generated.h"

UCLASS()
class READYORNOT_API ARoomVolume : public AVolume
{
	GENERATED_BODY()

public:
	ARoomVolume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int RoomGroupID = -1;

	TArray<TSharedPtr<FGraphNode>> ConnectedNodes;

	UPROPERTY()
	TArray<AActor*> OverlappingActors;

	void BeginPlay() override;
};
