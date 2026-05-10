// Void Interactive, 2020

#include "ThreatAwarenessOctree.h"

bool FThreatAwarenessDataOctreeSemantics::AreElementsEqual(const FThreatAwarenessDataOctreeElement& A, const FThreatAwarenessDataOctreeElement& B)
{
	return A.Data->ThreatAwarenessActor == B.Data->ThreatAwarenessActor;
}

void FThreatAwarenessDataOctreeSemantics::SetElementId(FOctree& OctreeOwner, const FThreatAwarenessDataOctreeElement& Element, const FOctreeElementId2 Id)
{
	static_cast<FThreatAwarenessOctree&>(OctreeOwner).SetElementIdImpl(Element, Id);
}

FThreatAwarenessOctree::FThreatAwarenessOctree()
	: TOctree2<FThreatAwarenessDataOctreeElement, FThreatAwarenessDataOctreeSemantics>()
{
}

FThreatAwarenessOctree::FThreatAwarenessOctree(const FVector& Origin, const float Radius)
	: TOctree2<FThreatAwarenessDataOctreeElement, FThreatAwarenessDataOctreeSemantics>(Origin, Radius)
{
}

FThreatAwarenessOctree::~FThreatAwarenessOctree()
{
}

void FThreatAwarenessOctree::SetElementIdImpl(const FThreatAwarenessDataOctreeElement Element, const FOctreeElementId2 Id)
{
	FRWScopeLock ScopeLock{Mutex, SLT_Write};

	ThreatActorToOctreeId.Add(Element.Data->ThreatAwarenessActor, Id);
}

void FThreatAwarenessOctree::AddThreatPoint(const FThreatAwarenessData& InThreatPoint)
{
	AddElement(FThreatAwarenessDataOctreeElement(InThreatPoint));
}

void FThreatAwarenessOctree::RemoveThreatPoint(class AThreatAwarenessActor* InThreatPoint)
{
	RemoveThreatPoint(GetOctreeElementID(InThreatPoint));
}

void FThreatAwarenessOctree::RemoveThreatPoint(const FOctreeElementId2& Id)
{
	RemoveElement(Id);
}

void FThreatAwarenessOctree::RemoveAllThreatPoints()
{
	ThreatActorToOctreeId.Empty();
	Destroy();
}

bool FThreatAwarenessOctree::FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FSphere& InQuerySphere) const
{
	OutThreatPoints.Reserve(ThreatActorToOctreeId.Num());
	
	const FBoxCenterAndExtent& BoxFromSphere = FBoxCenterAndExtent(InQuerySphere.Center, FVector(InQuerySphere.W));
	FindElementsWithBoundsTest(BoxFromSphere, [&](const FThreatAwarenessDataOctreeElement& ThreatPoint)
	{
		// check if threat point is inside the supplied sphere's radius
		if (InQuerySphere.Intersects(ThreatPoint.Bounds.GetSphere()))
			OutThreatPoints.Add(ThreatPoint);
	});
	
	return OutThreatPoints.Num() > 0;
}

bool FThreatAwarenessOctree::FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FBox& InQueryBox) const
{
	OutThreatPoints.Reserve(ThreatActorToOctreeId.Num());
	
	FindElementsWithBoundsTest(InQueryBox, [&](const FThreatAwarenessDataOctreeElement& ThreatPoint)
	{
		OutThreatPoints.Add(ThreatPoint);
	});

	return OutThreatPoints.Num() > 0;
}

FOctreeElementId2 FThreatAwarenessOctree::GetOctreeElementID(class AThreatAwarenessActor* InThreatPoint) const
{
	if (ThreatActorToOctreeId.Find(InThreatPoint))
	{
		return ThreatActorToOctreeId[InThreatPoint];
	}

	return InvalidId;
}

FThreatAwarenessPointOctreeData* FThreatAwarenessOctree::GetDataFromOctreeID(const FOctreeElementId2& Id)
{
	if (!Id.IsValidId())
	{
		return nullptr;
	}

	const FThreatAwarenessDataOctreeElement& OctreeElement = GetElementById(Id);

	return &*OctreeElement.Data;
}

const FThreatAwarenessPointOctreeData* FThreatAwarenessOctree::GetDataFromOctreeID(const FOctreeElementId2& Id) const
{
	if (!Id.IsValidId())
	{
		return nullptr;
	}

	const FThreatAwarenessDataOctreeElement& OctreeElement = GetElementById(Id);

	return &*OctreeElement.Data;
}
