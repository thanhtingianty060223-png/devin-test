// Void Interactive, 2020

#pragma once

#include "UObject/ObjectMacros.h"
#include "PrimitiveViewRelevance.h"
#include "DebugRenderSceneProxy.h"
#include "Components/PrimitiveComponent.h"
#include "NavSplinePathRenderingComponent.generated.h"

namespace NavSplinePath
{
	struct FDebugRenderElements
	{
		TArray<FDebugRenderSceneProxy::FDebugLine> Lines;
		TArray<FDebugRenderSceneProxy::FSphere> Spheres;
		TArray<FDebugRenderSceneProxy::FText3d> Texts;

		void Reset()
		{
			Lines.Reset();
			Spheres.Reset();
			Texts.Reset();
		}
	};
}

class READYORNOT_API FNavSplinePathSceneProxy final : public FDebugRenderSceneProxy
{
	friend class FNavSplinePathRenderingDebugDrawDelegateHelper;
	
public:
	virtual SIZE_T GetTypeHash() const override;

	explicit FNavSplinePathSceneProxy(UPrimitiveComponent* InComponent, const FString& InViewFlagName = TEXT("DebugAI"), NavSplinePath::FDebugRenderElements* InDebugRenderElements = nullptr);

	static void DrawPathData(UPrimitiveComponent* InComponent, NavSplinePath::FDebugRenderElements& DebugElements);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

private:
	AActor* ActorOwner = nullptr;
	uint8 bDrawOnlyWhenSelected : 1;

	bool SafeIsActorSelected() const;
};

class FNavSplinePathRenderingDebugDrawDelegateHelper : public FDebugDrawDelegateHelper
{
	typedef FDebugDrawDelegateHelper Super;

public:
	FNavSplinePathRenderingDebugDrawDelegateHelper()
		: bDrawOnlyWhenSelected(true)
	{
	}

	void InitDelegateHelper(const FNavSplinePathSceneProxy* InSceneProxy)
	{
		Super::InitDelegateHelper(InSceneProxy);

		ActorOwner = InSceneProxy->ActorOwner;
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
	uint8 bDrawOnlyWhenSelected : 1;
};

UCLASS(hidecategories=Object)
class READYORNOT_API UNavSplinePathRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	explicit UNavSplinePathRenderingComponent(const FObjectInitializer& ObjectInitializer);

	FString DrawFlagName;
	uint8 bDrawOnlyWhenSelected : 1;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;

	FNavSplinePathRenderingDebugDrawDelegateHelper RenderingDebugDrawDelegateHelper;

protected:
	NavSplinePath::FDebugRenderElements DebugData;
};
