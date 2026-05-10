// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/Sound/GraphNode.h"
#include "Actors/Sound/PortalVolume.h"
#include "Actors/Sound/RoomVolume.h"
#include "Actors/Sound/SoundSource.h"

#include "SoundManager.generated.h"

#define MAX_BVH_DEPTH 7

class SoundGraphQueue
{
public:
	TArray<TSharedPtr<FGraphNode>> NodeArray;
	int32 Length;
	
	SoundGraphQueue()
	{
		NodeArray.Add(nullptr);
		Length = 0;
	}
	
	int GetParentIndex(int i) { return i/2; }

	int GetLeftChildIndex(int i) { return i*2; }

	int GetRightChildIndex(int i) { return i*2 + 1; }
	
	bool CompareNodes(TSharedPtr<FGraphNode> a, TSharedPtr<FGraphNode> b)
	{
		if (a->f < b->f)
		{
			return true;
		}
		
		return false;
	}
	
	void ReHeapUp(int32 i)
	{
		if (i < 2)
			return;

		int parentIndex = GetParentIndex(i);
		// The child has higher priority, so we need to raise it.
		
		if (CompareNodes(NodeArray[i], NodeArray[parentIndex]))
		{
			// Flip values
			TSharedPtr<FGraphNode> p = NodeArray[parentIndex];
			NodeArray[parentIndex] = NodeArray[i];
			NodeArray[i] = p;
			// Recurse
			ReHeapUp(parentIndex);
		}
	}

	void ReHeadDown(int i){
		int leftIndex = GetLeftChildIndex(i);
		int rightIndex = GetRightChildIndex(i);

		// We're out of bounds, so we need to break.
		if(leftIndex > Length)
			return;

		int currentChild = leftIndex;
		// Ensure we are not out of bounds and check if the right has a higher priority than the left
		if(rightIndex <= Length && CompareNodes(NodeArray[rightIndex], NodeArray[leftIndex])){
			currentChild = rightIndex;
		}

		// Switch if the current child's priority is higher than the parent's.
		if(CompareNodes(NodeArray[currentChild], NodeArray[i])){
			TSharedPtr<FGraphNode> p = NodeArray[currentChild];
			NodeArray[currentChild] = NodeArray[i];
			NodeArray[i] = p;
			ReHeadDown(currentChild);
		}
	}

	TSharedPtr<FGraphNode> Pop()
	{
		TSharedPtr<FGraphNode> Node = NodeArray[1];
		NodeArray[1] = NodeArray[Length];
		Length--;
		ReHeadDown(1);
		return Node;
	}

	void Push(TSharedPtr<FGraphNode> Node)
	{
		Length++;
		NodeArray.Add(Node);
		ReHeapUp(NodeArray.Num()-1);
	}

	
	void ChangePriority(TSharedPtr<FGraphNode> GraphNode, int newPriority){
		// Attempt to find a matching patient.
		int foundIndex = -1;
		for(int i = 1; i < NodeArray.Num(); i++){
			if(NodeArray[i]->ID == GraphNode->ID){
				foundIndex = i;
				break;
			}
		}

		// Ensure we have found a patient
		if(foundIndex == -1)
			return;

		// If we need to reheap up.
		if(newPriority < NodeArray[foundIndex]->f){
			NodeArray[foundIndex]->f = newPriority;
			ReHeapUp(foundIndex);
		}
		// If we need to reheap down.
		else{
			NodeArray[foundIndex]->f = newPriority;
			ReHeadDown(foundIndex);
		}
	}
};

USTRUCT()
struct FNodePair
{
	GENERATED_BODY()
	
	uint32 FirstID;
	uint32 SecondID;

	bool operator==(const FNodePair& Other) const
	{
		return (FirstID == Other.FirstID && SecondID == Other.SecondID);
	}
};

USTRUCT()
struct FNodePath
{
	GENERATED_BODY()

	TArray<TSharedPtr<FGraphNode>> Path;
};

class READYORNOT_API UBVHNode;

/**
 * 
 */
UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API USoundManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	USoundManager();
	
public:
	UFUNCTION(BlueprintPure)
	// Gets the instance of the SoundManager given the UWorld.
	static USoundManager* Get(UWorld* World);

	// Connects a PortalVolume to the SoundManager.
	void ConnectPortal();

	// Connects a RoomVolume to the SoundManager.
	void ConnectRoom();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual TStatId GetStatId() const override;

	// Check if the SoundManager has been initialized, and will initialize if it can.
	void CheckInitialization();

	// Check if the graph can be finalized, and will finalize if it can.
	void CheckFinishGraph();
	
	// Gets all the portal volumes on the level.
	void FindPortalVolumes();
	// Gets all the room volumes on the level.
	void FindRoomVolumes();

public:

	// If the level has not been setup for advanced sound and needs to be defaulted to a legacy version.
	UPROPERTY()
	bool bForceLegacySound = false;
	
	// If the setup of the object has completed, which allows for use of the advanced sound systems. 
	UPROPERTY()
	bool bHasFinishedSetup = false;

	// All PortalVolumes
	UPROPERTY()
	TArray<APortalVolume*> PortalVolumes;

	// All RoomVolumes.
	UPROPERTY()
	TArray<ARoomVolume*> RoomVolumes;

	// All SoundSources that are currently in use.
	UPROPERTY()
	TArray<USoundSource*> ActiveSoundSources;

	// All inactive and available SoundSources.
	UPROPERTY()
	TArray<USoundSource*> InactiveSoundSources;

	// The root node of the BVH tree.
	UPROPERTY()
	UBVHNode* RootBVHNode;
	
	// List of all static GraphNodes
	TArray<TSharedPtr<FGraphNode>> GraphNodes;

	// List of all outside graph nodes
	TArray<TSharedPtr<FGraphNode>> OuterGraphNodes;

	// Gets the next available SoundSource or creates a new one if not found.
	USoundSource* GetAvailableSoundSource();

	// Adds an external GraphNode to the sound graph.
	ARoomVolume* AddExternalNode(TSharedPtr<FGraphNode> Node);

	// Removes an external GraphNode from the graph.
	void RemoveExternalNode(TSharedPtr<FGraphNode> Node);

	// Gets the path between two graph nodes with optional parameters.
	TArray<TSharedPtr<FGraphNode>> GetUnObstructedPath(TSharedPtr<FGraphNode> A, TSharedPtr<FGraphNode> B, ARoomVolume* RoomVolumeA, ARoomVolume* RoomVolumeB, bool bAvoidClosedPortals, TArray<TSharedPtr<FGraphNode>> NodesToAvoid);
	
	void GenerateGraph();


private:
	bool bHasInitialized = false;
	int CurrentPortalCount = 0;
	int CurrentRoomCount = 0;

	// Generates the SoundSource pool.
	void GenerateSoundSourcePool();

	// Gets the distance to the closest point on a collision body.
	float GetSquaredDistanceToBody(const FBodyInstance* InInstance, const FVector& InPoint);

	// Check if a portal volume is obstructed or unobstructed.
	bool IsPortalClosed(APortalVolume* PortalVolume);

	// Gets the RoomVolume dependent on the given location.
	ARoomVolume* GetLocationVolume(FVector Location);
};

UCLASS()
class READYORNOT_API UBVHNode : public UObject
{
	GENERATED_BODY()
public:
	int Depth;
	FBoxSphereBounds Bounds;
	
	UPROPERTY()
	TArray<UBVHNode*> Children;

	UPROPERTY()
	TArray<ARoomVolume*> Rooms;

	UPROPERTY()
	UWorld* World;

	UBVHNode(){}

	void Initialize(FVector Location, FVector Extent, int InDepth, UBVHNode* Parent, UWorld* InWorld)
	{
		World = InWorld;
		Bounds.Origin = Location;
		Bounds.BoxExtent = Extent;
		Depth = InDepth;
		
		GenerateRooms(Parent);
		// Delete this room, it's useless.
		if(Parent && Rooms.Num() == 0)
		{
			for(int i = 0; i < Parent->Children.Num(); i++)
			{
				if(Parent->Children[i]->GetUniqueID() == this->GetUniqueID())
				{
					Parent->Children[i] = nullptr;
				}
			}
			return;
		}
		
		if(Depth < MAX_BVH_DEPTH)
		{
			CreateChildren();
		}
	}
	
	void CreateChildren()
	{
		float SizeX = Bounds.BoxExtent.X;
		float SizeY = Bounds.BoxExtent.Y;
		float SizeZ = Bounds.BoxExtent.Z;

		//Top
		float OriginHeight = Bounds.Origin.Z - SizeZ/2;
		
		UBVHNode* Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X-SizeX/2,Bounds.Origin.Y-SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X+SizeX/2,Bounds.Origin.Y-SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X-SizeX/2,Bounds.Origin.Y+SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X+SizeX/2,Bounds.Origin.Y+SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		//Bottom
		OriginHeight = Bounds.Origin.Z + SizeZ/2;
		
		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X-SizeX/2,Bounds.Origin.Y-SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X+SizeX/2,Bounds.Origin.Y-SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X-SizeX/2,Bounds.Origin.Y+SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);

		Node = NewObject<UBVHNode>();
		Node->Initialize(FVector(Bounds.Origin.X+SizeX/2,Bounds.Origin.Y+SizeY/2,OriginHeight), FVector(SizeX/2, SizeY/2, SizeZ/2), Depth + 1, this, World);
		Children.Add(Node);
	}

	void GenerateRooms(UBVHNode* Parent)
	{
		TArray<ARoomVolume*> RoomsToCheck;
		if(Parent)
		{
			RoomsToCheck = Parent->Rooms;
		}
		else
		{
			RoomsToCheck = World->GetSubsystem<USoundManager>()->RoomVolumes;
		}
		
		for(ARoomVolume* RoomVolume : RoomsToCheck)
		{
			if(Bounds.GetBox().Intersect(RoomVolume->GetBounds().GetBox()))
			{
				Rooms.Add(RoomVolume);
			}
		}
	}
};