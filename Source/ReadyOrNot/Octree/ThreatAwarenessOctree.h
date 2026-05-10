// Void Interactive, 2020

#pragma once

#include "Math/GenericOctree.h"
#include "ThreatAwarenessData.h"

struct FThreatAwarenessPointOctreeData : TSharedFromThis<FThreatAwarenessPointOctreeData, ESPMode::ThreadSafe>
{
	FThreatAwarenessPointOctreeData() {}

	explicit FThreatAwarenessPointOctreeData(const FThreatAwarenessData& InThreatAwarenessData)
		: ThreatAwarenessActor(InThreatAwarenessData.ThreatAwarenessActor), Location(InThreatAwarenessData.ThreatLocation), ThreatLevel(InThreatAwarenessData.ThreatLevel)
	{
	}
	
	const TWeakObjectPtr<class AThreatAwarenessActor> ThreatAwarenessActor = nullptr;
	const FVector Location = FVector::ZeroVector;
	const EThreatLevel ThreatLevel = EThreatLevel::TL_None;
};

struct FThreatAwarenessDataOctreeElement
{
	FBoxSphereBounds Bounds;
	TSharedRef<FThreatAwarenessPointOctreeData, ESPMode::ThreadSafe> Data;

	FThreatAwarenessDataOctreeElement()
		: Data(new FThreatAwarenessPointOctreeData())
	{}

	explicit FThreatAwarenessDataOctreeElement(const FThreatAwarenessData& FThreatAwarenessData)
		: Bounds(FSphere(FThreatAwarenessData.ThreatLocation, 1.0f)), Data(new FThreatAwarenessPointOctreeData(FThreatAwarenessData))
	{}

	FORCEINLINE bool IsEmpty() const
	{
		const FBox BoundingBox = Bounds.GetBox();
		return BoundingBox.IsValid == false || BoundingBox.GetSize().IsNearlyZero();
	}

	FORCEINLINE UObject* GetOwner() const
	{
		return Data->ThreatAwarenessActor.Get();
	}
};

struct FThreatAwarenessDataOctreeSemantics
{
	typedef TOctree2<FThreatAwarenessDataOctreeElement, FThreatAwarenessDataOctreeSemantics> FOctree;

	enum { MaxElementsPerLeaf = 32 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FThreatAwarenessDataOctreeElement& Element)
	{
		return Element.Bounds;
	}

	static bool AreElementsEqual(const FThreatAwarenessDataOctreeElement& A, const FThreatAwarenessDataOctreeElement& B);

	static void SetElementId(FOctree& OctreeOwner, const FThreatAwarenessDataOctreeElement& Element, FOctreeElementId2 Id);

	FORCEINLINE static void ApplyOffset(FThreatAwarenessDataOctreeElement& Element, FVector Offset)
	{
	}
};

typedef TFunctionRef<bool(const FThreatAwarenessDataOctreeElement&)> ThreatSearchPredicate;

/**
 * Octree for storing threat awareness actors. Use UThreatAwarenessSubsystem for manipulation.
 */
class READYORNOT_API FThreatAwarenessOctree : public TOctree2<FThreatAwarenessDataOctreeElement, FThreatAwarenessDataOctreeSemantics>, public TSharedFromThis<FThreatAwarenessOctree, ESPMode::ThreadSafe>
{
public:
  	FThreatAwarenessOctree();
  	FThreatAwarenessOctree(const FVector& Origin, float Radius);
  	virtual ~FThreatAwarenessOctree();

	void AddThreatPoint(const FThreatAwarenessData& InThreatPoint);
	void RemoveThreatPoint(class AThreatAwarenessActor* InThreatPoint);
	void RemoveThreatPoint(const FOctreeElementId2& Id);
	void RemoveAllThreatPoints();

	bool FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FSphere& InQuerySphere) const;
	bool FindThreatPoints(TArray<FThreatAwarenessDataOctreeElement>& OutThreatPoints, const FBox& InQueryBox) const;

	FOctreeElementId2 GetOctreeElementID(class AThreatAwarenessActor* InThreatPoint) const;
	FThreatAwarenessPointOctreeData* GetDataFromOctreeID(const FOctreeElementId2& Id);
	const FThreatAwarenessPointOctreeData* GetDataFromOctreeID(const FOctreeElementId2& Id) const;

protected:
	friend struct FThreatAwarenessDataOctreeSemantics;
	void SetElementIdImpl(FThreatAwarenessDataOctreeElement Element, FOctreeElementId2 Id);

	TMap<TWeakObjectPtr<class AThreatAwarenessActor>, FOctreeElementId2> ThreatActorToOctreeId;

	FOctreeElementId2 InvalidId;
	mutable FRWLock Mutex;
};
