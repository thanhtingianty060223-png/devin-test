// Void Interactive, 2020

#pragma once

#include "UObject/ObjectMacros.h"
#include "PrimitiveViewRelevance.h"
#include "DebugRenderSceneProxy.h"
#include "Components/PrimitiveComponent.h"
#include "Interfaces/CoverQueryResultInterface.h"
#include "CoverFinderRenderingComponent.generated.h"

class READYORNOT_API FCoverFinderSceneProxy final : public FDebugRenderSceneProxy
{
	friend class FCoverFinderRenderingDebugDrawDelegateHelper;
	
public:
	virtual SIZE_T GetTypeHash() const override;

	explicit FCoverFinderSceneProxy(UPrimitiveComponent* InComponent, const FString& InViewFlagName = TEXT("DebugAI"), CoverFinder::FDebugRenderElements* InDebugRenderElements = nullptr);

	static void CollectCoverData(UPrimitiveComponent* InComponent, ICoverQueryResultInterface* InQueryDataSource, CoverFinder::FDebugRenderElements& DebugElements);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

private:
	AActor* ActorOwner = nullptr;
	ICoverQueryResultInterface* QueryDataSource = nullptr;
	uint8 bDrawOnlyWhenSelected : 1;

	bool SafeIsActorSelected() const;
};

class FCoverFinderRenderingDebugDrawDelegateHelper : public FDebugDrawDelegateHelper
{
	typedef FDebugDrawDelegateHelper Super;

public:
	FCoverFinderRenderingDebugDrawDelegateHelper()
		: bDrawOnlyWhenSelected(true)
	{
	}

	// ##UE5UPGRADE## Compatibility InitDelegateHelper removed 
	void InitDelegateHelper(const FCoverFinderSceneProxy* InSceneProxy)
	{
		Super::InitDelegateHelper(InSceneProxy);

		ActorOwner = InSceneProxy->ActorOwner;
		QueryDataSource = InSceneProxy->QueryDataSource;
		bDrawOnlyWhenSelected = InSceneProxy->bDrawOnlyWhenSelected;
	}

	void Reset()
	{
		ResetTexts();
	}

protected:
	READYORNOT_API virtual void DrawDebugLabels(UCanvas* Canvas, APlayerController* PlayerController) override;

private:
	AActor* ActorOwner = nullptr;
	ICoverQueryResultInterface* QueryDataSource = nullptr;
	uint8 bDrawOnlyWhenSelected : 1;
};

UCLASS(hidecategories=Object)
class READYORNOT_API UCoverFinderRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	explicit UCoverFinderRenderingComponent(const FObjectInitializer& ObjectInitializer);

	FString DrawFlagName;
	uint8 bDrawOnlyWhenSelected : 1;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;

	FCoverFinderRenderingDebugDrawDelegateHelper CoverRenderingDebugDrawDelegateHelper;

protected:
	CoverFinder::FDebugRenderElements DebugData;
};
