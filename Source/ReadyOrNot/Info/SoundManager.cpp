// Copyright Void Interactive, 2023

#include "SoundManager.h"

// #include <PxGeometryQuery.h>

#include "PhysXPublicCore.h"
#include "Actors/Door.h"

#define TOTAL_SOUND_SOURCES 255

USoundManager::USoundManager()
{
}

USoundManager* USoundManager::Get(UWorld* World)
{
	return World->GetSubsystem<USoundManager>();
}

void USoundManager::GenerateSoundSourcePool()
{
	InactiveSoundSources.Empty();
	ActiveSoundSources.Empty();
	for (uint32 i = 0; i < TOTAL_SOUND_SOURCES; i++)
	{
		InactiveSoundSources.Add(NewObject<USoundSource>());
	}
}

USoundSource* USoundManager::GetAvailableSoundSource()
{
	if (InactiveSoundSources.Num() > 0)
	{
		USoundSource* SoundSource = InactiveSoundSources.Pop(false);
		ActiveSoundSources.Add(SoundSource);
		return SoundSource;
	}
	
	USoundSource* SoundSource = NewObject<USoundSource>();
	ActiveSoundSources.Add(SoundSource);
	return SoundSource;
}

void USoundManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(!bHasInitialized)
	{
		CheckInitialization();
		return;
	}
	if(bHasInitialized && !bHasFinishedSetup)
	{
		CheckFinishGraph();
	}
}

TStatId USoundManager::GetStatId() const
{
	return GetStatID();
}

void USoundManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	V_LOGM(LogReadyOrNot, "Generating SoundManager SoundSource pool.");
	GenerateSoundSourcePool();
}

void USoundManager::CheckInitialization()
{
	if(!bHasInitialized)
	{
		FindRoomVolumes();
		FindPortalVolumes();
		if( !bForceLegacySound && (RoomVolumes.Num() == 0 || PortalVolumes.Num() == 0))
		{
			bForceLegacySound = true;
			V_LOG(LogReadyOrNot, "SoundManager forcing legacy sound.");
		}
		else
		{
			// Creates all static graph nodes
			int32 ID = 0;
			for(APortalVolume* PortalVolume : PortalVolumes)
			{
				TSharedPtr<FGraphNode> Node(new FGraphNode());
				Node->Initialize(EGraphNodeType::Portal, PortalVolume, PortalVolume->GetActorLocation(), ID);
				GraphNodes.Add(Node);

				if(PortalVolume->bIsOutside)
				{
					OuterGraphNodes.Add(Node);
				}
				ID++;
			}
		}
		
		bHasInitialized = true;
	}
}

void USoundManager::ConnectRoom()
{
	CheckInitialization();
	CurrentRoomCount++;
	CheckFinishGraph();
}

void USoundManager::ConnectPortal()
{
	CheckInitialization();
	CurrentPortalCount++;
	CheckFinishGraph();
}

void USoundManager::GenerateGraph()	
{

	// Get overlapping actors for each room volumes. Needs to be done here, and after all Room and Portal volumes are streamed in to be able to properly network with the graph.
	for(ARoomVolume* RoomVolume : RoomVolumes)
	{
		RoomVolume->GetOverlappingActors(RoomVolume->OverlappingActors);
	}
	
	// Create room groups and connect all nodes
	TArray<ARoomVolume*> Visited;
	for(ARoomVolume* RoomVolume : RoomVolumes)
	{
		// This is already in a room group.
		if(Visited.Contains(RoomVolume))
		{
			continue;
		}

		// Get all connected rooms
		TArray<ARoomVolume*> ConnectedRooms;
		if(RoomVolume->RoomGroupID == -1)
		{
			ConnectedRooms = {RoomVolume};
		}
		else
		{
			for(ARoomVolume* OtherRoom : RoomVolumes)
			{
				if(RoomVolume->RoomGroupID == OtherRoom->RoomGroupID)
				{
					ConnectedRooms.Add(OtherRoom);
				}
			}
		}

		// Get all connected graph nodes.
		TArray<TSharedPtr<FGraphNode>> ConnectedGraphNodes;
		for(TSharedPtr<FGraphNode> GraphNode : GraphNodes)
		{
			for(ARoomVolume* Room : ConnectedRooms)
			{
				TArray<AActor*> OverlappingActors = Room->OverlappingActors;

				if(GraphNode && GraphNode->Object && OverlappingActors.Contains(Cast<APortalVolume>(GraphNode->Object)))
				{
					ConnectedGraphNodes.Add(GraphNode);
				}
			}
		}

		// Connect each graph node to the other graph nodes
		for(TSharedPtr<FGraphNode> NodeA : ConnectedGraphNodes)
		{
			for(TSharedPtr<FGraphNode> NodeB : ConnectedGraphNodes)
			{
				if(NodeA != NodeB)
				{
					NodeA->NeighborNodes.Add(NodeB);
				}
			}
		}

		// Add all to visited, and add room to room group map
		for(ARoomVolume* RoomGroupRoom : ConnectedRooms)
		{
			Visited.Add(RoomGroupRoom);
			RoomGroupRoom->ConnectedNodes = ConnectedGraphNodes;
		}
	}
	
	// Connect all outdoor nodes to each other
	for(TSharedPtr<FGraphNode> NodeA : OuterGraphNodes)
	{
		for(TSharedPtr<FGraphNode> NodeB : OuterGraphNodes)
		{
			if(NodeA != NodeB)
			{
				NodeA->NeighborNodes.Add(NodeB);
			}
		}
	}
}

void USoundManager::CheckFinishGraph()
{
	// Check for finish conditions.
	if(!bForceLegacySound && CurrentPortalCount == PortalVolumes.Num() && CurrentRoomCount == RoomVolumes.Num())
	{
		// Increase size of graph node Neighbor list to prevent reallocating every tick
		for(TSharedPtr<FGraphNode> GraphNode : GraphNodes)
		{
			GraphNode->NeighborNodes.Reserve(GraphNode->NeighborNodes.Num()+2);
		}

		GenerateGraph();

		// Generate the BVH for fast room lookup
		RootBVHNode = NewObject<UBVHNode>();
		RootBVHNode->Initialize(FVector(0,0,0), FVector(30000, 30000, 30000), 0, nullptr, GetWorld());


		// Disable collision for rooms and portals
		for(ARoomVolume* RoomVolume : RoomVolumes)
		{
			RoomVolume->SetActorEnableCollision(false);
		}
		for(APortalVolume* PortalVolume : PortalVolumes)
		{
			PortalVolume->SetActorEnableCollision(false);
		}
		bHasFinishedSetup = true;
	}
	else if(bForceLegacySound){
		// Disable collision for rooms and portals
		for(ARoomVolume* RoomVolume : RoomVolumes)
		{
			RoomVolume->SetActorEnableCollision(false);
		}
		for(APortalVolume* PortalVolume : PortalVolumes)
		{
			PortalVolume->SetActorEnableCollision(false);
		}
		bHasFinishedSetup = true;
	}
}

bool USoundManager::IsPortalClosed(APortalVolume* PortalVolume)
{
	if (PortalVolume->AttachedObjects.Num() > 0)
	{
		bool bAllClosed = true;
		for (ADoor* Door : PortalVolume->Doors)
		{
			if (Door && Door->IsOpen())
			{
				bAllClosed = false;
				break;
			}
		}
		
		if (bAllClosed)
		{
			return true;
		}
	}
	return false;
}

TArray<TSharedPtr<FGraphNode>> USoundManager::GetUnObstructedPath(TSharedPtr<FGraphNode> A, TSharedPtr<FGraphNode> B, ARoomVolume* RoomVolumeA, ARoomVolume* RoomVolumeB, bool bAvoidClosedPortals, TArray<TSharedPtr<FGraphNode>> NodesToAvoid)
{
	// Outside
	if(!RoomVolumeA && !RoomVolumeB)
	{
		return {{A, B}};
	}

	// If both are inside
	if(RoomVolumeA && RoomVolumeB)
	{

		// If within the exact same room.
		if(RoomVolumeA->GetUniqueID() == RoomVolumeB->GetUniqueID())
		{
			return {{A, B}};
		}
		
		// If within the same room group
		if(RoomVolumeA->RoomGroupID != -1 && RoomVolumeA->RoomGroupID == RoomVolumeB->RoomGroupID)
		{
			return {{A, B}};
		}
	}
	
	SoundGraphQueue* Queue = new SoundGraphQueue();
	Queue->NodeArray.Reserve(GraphNodes.Num()+2);
	A->f = 0;
	B->f = TNumericLimits<float>::Max();
	
	// Infinite Distance
	Queue->Push(A);
	for (TSharedPtr<FGraphNode> GraphNode : GraphNodes)
	{
		if (GraphNode->GetUniqueID() != A->GetUniqueID())
		{
			if (bAvoidClosedPortals && IsPortalClosed(Cast<APortalVolume>(GraphNode->Object)))
			{
				continue;
			}

			// Prevent sound from passing through breakable windows.
			if (APortalVolume* PortalVolume = Cast<APortalVolume>(GraphNode->Object))
			{
				if (PortalVolume->BreakableGlasses.Num() > 0)
				{
					// We only want to continue if the all the attached glasses are intact.
					bool bGlassIntact = true;
					for (ABreakableGlass* Glass : PortalVolume->BreakableGlasses)
					{
						if (!IsValid(Glass))
							continue;
						
						if (Glass->bCanSoundPass)
						{
							bGlassIntact = false;
							break;
						}
					}
					if (bGlassIntact)
					{
						continue;
					}
				}
			}
			
			// Ensure node is not in the ignore list.
			bool bAvoid = false;
			for (TSharedPtr<FGraphNode> AvoidNode : NodesToAvoid)
			{
				if(AvoidNode->GetUniqueID() == GraphNode->GetUniqueID())
				{
					bAvoid = true;
					break;
				}
			}
			if (bAvoid)
				continue;
			
			GraphNode->f = TNumericLimits<float>::Max();
			Queue->Push(GraphNode);
		}
	}
	Queue->Push(B);
	
	while (Queue->Length > 0)
	{
		TSharedPtr<FGraphNode> Min = Queue->Pop();
		for (TSharedPtr<FGraphNode> Adj : Min->NeighborNodes)
		{
			if(!Adj)
			{
				continue;
			}

			// Prevent queue already added node.
			if(!Queue->NodeArray.Contains(Adj))
			{
				continue;
			}

			// Make sure to avoid closed portal if opted into.
			if(bAvoidClosedPortals && Adj->Object && IsPortalClosed(Cast<APortalVolume>(Adj->Object)))
			{
				continue;
			}

			// Prevent sound from passing through breakable windows.
			if(APortalVolume* PortalVolume = Cast<APortalVolume>(Adj->Object))
			{
				// We only want to continue if the all the attached glasses are intact.
				if(PortalVolume->BreakableGlasses.Num() > 0)
				{
					bool bGlassIntact = true;
					for(ABreakableGlass* Glass : PortalVolume->BreakableGlasses)
					{
						if(Glass->bCanSoundPass)
						{
							bGlassIntact = false;
						}
					}
					if(bGlassIntact)
					{
						continue;
					}
				}
			}

			// Ensure node is not in the ignore list.
			bool bAvoid = false;
			for(TSharedPtr<FGraphNode> AvoidNode : NodesToAvoid)
			{
				if(AvoidNode->GetUniqueID() == Adj->GetUniqueID())
				{
					bAvoid = true;
					break;
				}
			}
			if(bAvoid)
				continue;

			// Calculate new cost for node.
			float Alt = Min->f + FVector::Distance(Min->Location, Adj->Location);
			if (Adj->f > Alt)
			{
				Queue->ChangePriority(Adj, Alt);
				Adj->Parent = Min;
			}
		}
	}
	
	// Get the shortest path.
	A->Parent = nullptr;
	TArray<TSharedPtr<FGraphNode>> Path;
	TSharedPtr<FGraphNode> CurrentNode = B;
	while(CurrentNode)
	{
		Path.Add(CurrentNode);
		CurrentNode = CurrentNode->Parent;
	}

	if(Path.Num() == 1)
	{
		Path = {};
	}
	
	return Path;
}

ARoomVolume* USoundManager::AddExternalNode(TSharedPtr<FGraphNode> Node)
{
	ARoomVolume* RoomVolume = GetLocationVolume(Node->Location);
	
	// If the node is indoors
	if(RoomVolume)
	{
		for(TSharedPtr<FGraphNode> ConnectedNode : RoomVolume->ConnectedNodes)
		{
			Node->NeighborNodes.Add(ConnectedNode);
			ConnectedNode->NeighborNodes.Add(Node);
		}	
	}
	// If the node is outdoors
	else
	{
		for(TSharedPtr<FGraphNode> OutdoorNode : OuterGraphNodes)
		{
			Node->NeighborNodes.Add(OutdoorNode);
			OutdoorNode->NeighborNodes.Add(Node);
		}
	}
	
	return RoomVolume;
}

void USoundManager::RemoveExternalNode(TSharedPtr<FGraphNode> Node)
{
	for(TSharedPtr<FGraphNode> ConnectedNode : Node->NeighborNodes)
	{
		if(!ConnectedNode)
		{
			continue;
		}
		ConnectedNode->NeighborNodes.Remove(Node);
	}
}

ARoomVolume* USoundManager::GetLocationVolume(FVector Location)
{
	UBVHNode* CurrentNode = RootBVHNode;
	while (CurrentNode)
	{
		bool bHasFoundChild = false;
		for(UBVHNode* Child : CurrentNode->Children)
		{
			if(Child && UKismetMathLibrary::IsPointInBox(Location, Child->Bounds.Origin, Child->Bounds.BoxExtent))
			{
				CurrentNode = Child;
				bHasFoundChild = true;
			}
		}

		if(!bHasFoundChild)
		{
			for(ARoomVolume* RoomVolume : CurrentNode->Rooms)
			{
				if(GetSquaredDistanceToBody(RoomVolume->GetBrushComponent()->GetBodyInstance(), Location) <= 0)
				{
					return RoomVolume;
				}
			}
			return nullptr;
		}
	}
	return nullptr;
}

float USoundManager::GetSquaredDistanceToBody(const FBodyInstance* InInstance, const FVector& InPoint)
{
	
	float MinDistanceSqr = BIG_NUMBER;
	// ##UE5UPGRADE## Needs Fix       
	//FPhysicsCommand::ExecuteRead(InInstance->ActorHandle, [&](const FPhysicsActorHandle& Actor)
	//{

	//	// Get all the shapes from the actor
	//	PhysicsInterfaceTypes::FInlineShapeArray PShapes;
	//	const int32 NumTotalShapes = FillInlineShapeArray_AssumesLocked(PShapes, Actor);

	//	const PxVec3 PPoint = U2PVector(InPoint);

	//	// Iterate over each shape
	//	for(int32 ShapeIdx = 0; ShapeIdx < NumTotalShapes; ShapeIdx++)
	//	{
	//		FPhysicsShapeHandle_PhysX& ShapeRef = PShapes[ShapeIdx];
	//		PxShape* PShape = ShapeRef.Shape;
	//		check(PShape);

	//		FPhysicsGeometryCollection GeoCollection = FPhysicsInterface::GetGeometryCollection(ShapeRef);
	//		
	//		PxTransform PGlobalPose = U2PTransform(FPhysicsInterface::GetTransform(ShapeRef));

	//		ECollisionShapeType GeomType = FPhysicsInterface::GetShapeType(ShapeRef);

	//		if(GeomType == ECollisionShapeType::Trimesh)
	//		{
	//			// Type unsupported for this function, but some other shapes will probably work. 
	//			continue;
	//		}
	//		
	//		float SqrDistance = PxGeometryQuery::pointDistance(PPoint, GeoCollection.GetGeometry(), PGlobalPose);
	//		// distance has valid data and smaller than mindistance
	//		if(SqrDistance > 0.f && MinDistanceSqr > SqrDistance)
	//		{
	//			MinDistanceSqr = SqrDistance;
	//		}
	//		else if(SqrDistance == 0.f)
	//		{
	//			MinDistanceSqr = 0.f;
	//			break;
	//		}
	//	}
	//});
	//
	return MinDistanceSqr;
}

void USoundManager::FindPortalVolumes()
{
	PortalVolumes.Empty();
	for (TActorIterator<APortalVolume>It(GetWorld()); It; ++It)
	{
		APortalVolume* PortalVolume = *It;
		PortalVolumes.Add(PortalVolume);
	}
}

void USoundManager::FindRoomVolumes()
{
	RoomVolumes.Empty();
	for (TActorIterator<ARoomVolume>It(GetWorld()); It; ++It)
	{
		ARoomVolume* RoomVolume = *It;
		RoomVolumes.Add(RoomVolume);
	}
}
