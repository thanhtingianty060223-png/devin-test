// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GraphNode.generated.h"

UENUM(BlueprintType)
enum class EGraphNodeType : uint8
{
	Portal,
	Listener,
	SoundSource
};

USTRUCT()
struct FGraphNode
{
	GENERATED_BODY()
	
	EGraphNodeType NodeType = EGraphNodeType::SoundSource;
	FVector Location = FVector::ZeroVector;
	FGuid UniqueId = FGuid();
	int32 ID = -1;
	float f = 0.0f;
	
	UPROPERTY()
	UObject* Object = nullptr;

	TSharedPtr<FGraphNode> Parent = nullptr;
	
	TArray<TSharedPtr<FGraphNode>> NeighborNodes;

	FGraphNode(){}

	void Initialize(EGraphNodeType InNodeType, UObject* InObject, FVector InLocation, int InID)
	{
		UniqueId = FGuid::NewGuid();
		NodeType = InNodeType;
		Object = InObject;
		Location = InLocation;
		ID = InID;
		Parent = nullptr;
	}

	void AddNeighbor(TSharedPtr<FGraphNode> Node)
	{
		NeighborNodes.Add(Node);
	}

	void RemoveNeighbor(TSharedPtr<FGraphNode> Node)
	{
		NeighborNodes.Remove(Node);
	}

	FORCEINLINE FGuid GetUniqueID() const { return UniqueId; }
};