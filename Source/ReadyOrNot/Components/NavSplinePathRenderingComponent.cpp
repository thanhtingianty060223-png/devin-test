// Void Interactive, 2020

#include "Components/NavSplinePathRenderingComponent.h"

#include "NavigationSystem.h"
#include "Actors/NavigationSplinePathPreview.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

FNavSplinePathSceneProxy::FNavSplinePathSceneProxy(UPrimitiveComponent* InComponent, const FString& InViewFlagName, NavSplinePath::FDebugRenderElements* InDebugRenderElements)
	: FDebugRenderSceneProxy(InComponent)
{
	DrawType = SolidAndWireMeshes;
	TextWithoutShadowDistance = 1500;
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;
	bWantsSelectionOutline = false;
	bDrawOnlyWhenSelected = true;

	if (InComponent)
	{
		ActorOwner = InComponent->GetOwner();

		if (UNavSplinePathRenderingComponent* RenderComponent = Cast<UNavSplinePathRenderingComponent>(InComponent))
			bDrawOnlyWhenSelected = RenderComponent->bDrawOnlyWhenSelected;

		if (InDebugRenderElements)
		{
			InDebugRenderElements->Reset();
			
			DrawPathData(InComponent, *InDebugRenderElements);
			
			Lines = InDebugRenderElements->Lines;
			Spheres = InDebugRenderElements->Spheres;
			Texts = InDebugRenderElements->Texts;
		}
	}
}

void FNavSplinePathSceneProxy::DrawPathData(UPrimitiveComponent* InComponent, NavSplinePath::FDebugRenderElements& DebugElements)
{
	#if !UE_BUILD_SHIPPING
	const ANavigationSplinePathPreview* ActorOwner = InComponent ? Cast<ANavigationSplinePathPreview>(InComponent->GetOwner()) : nullptr;
	if (!ActorOwner)
		return;
	
	const FSplineCurves& SplinePath = ActorOwner->GetSplinePath();

	for (int32 i = 0; i < SplinePath.Position.Points.Num(); i++)
	{
		const int32 Index = i == 0 ? i+1 : i == SplinePath.Position.Points.Num()-1 ? i-1 : i;

		// Calculate max velocity of a path segement
		const FVector Y1 = SplinePath.Position.EvalDerivative(Index, FVector(1.0f, 0.0f, 0.0f));
		const FVector Y2 = SplinePath.Position.EvalSecondDerivative(Index, FVector(0.0f, 1.0f, 0.0f));
		
		constexpr float MaxPerpAcceleration = 500.0f;
		constexpr float MaxAngularVelocity = 200.0f;
		const float K = (Y1 * Y2).Size() / FMath::Pow(Y1.Size(), 3.0f);

		// Calculate turn rate limit of a path segement
		const FVector Kv = (Y1 * Y2) / FMath::Pow(Y1.Size(), 3.0f);
		const FVector W = Kv*453.0f; // Current velocity

		//const float KYaw = FVector::DotProduct(Kv, FVector::UpVector) * 600.0f;
		
		const float MaxVelocity = FMath::Min(FMath::Sqrt(MaxPerpAcceleration/K), MaxAngularVelocity/K);
	
		if (i-1 > -1)
		{
			FVector PathPoint = SplinePath.Position.Points[i-1].OutVal;
			DebugElements.Texts.Add(FText3d(FString::Printf(TEXT("Max Vel: %.2f | Ang Vel: %.2f\nTurn Rate: %f"), MaxVelocity, W.Size(), Kv.Size()), PathPoint, FColor::White));

			constexpr float ArrowLength = 75.0f;
			DebugElements.Lines.Add(FDebugLine(PathPoint, PathPoint + Kv.GetSafeNormal() * ArrowLength, FColor::Orange, 2.0f));
			DebugElements.Lines.Add(FDebugLine(PathPoint + Kv.GetSafeNormal() * ArrowLength, PathPoint + Kv.GetSafeNormal() * ArrowLength + (-Kv.GetSafeNormal()).RotateAngleAxis(30.0f, FVector::UpVector) * (ArrowLength/4.0f), FColor::Orange, 2.0f));
			DebugElements.Lines.Add(FDebugLine(PathPoint + Kv.GetSafeNormal() * ArrowLength, PathPoint + Kv.GetSafeNormal() * ArrowLength + (-Kv.GetSafeNormal()).RotateAngleAxis(-30.0f, FVector::UpVector) * (ArrowLength/4.0f), FColor::Orange, 2.0f));
		}
	}
	#endif
}

FPrimitiveViewRelevance FNavSplinePathSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	
	Result.bDrawRelevance = IsShown(View) && (!bDrawOnlyWhenSelected || (bDrawOnlyWhenSelected && SafeIsActorSelected()));
	Result.bDynamicRelevance = true;
	
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	Result.bSeparateTranslucency = Result.bNormalTranslucency = IsShown(View);

	return Result;
}

bool FNavSplinePathSceneProxy::SafeIsActorSelected() const
{
	if (ActorOwner)
	{
		return ActorOwner->IsSelected();
	}

	return false;
}

SIZE_T FNavSplinePathSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FNavSplinePathRenderingDebugDrawDelegateHelper::DrawDebugLabels(UCanvas* Canvas, APlayerController* PlayerController)
{
	// little hacky test but it's the only way to remove text rendering from bad worlds, when using UDebugDrawService for it
	if (Canvas && Canvas->SceneView && Canvas->SceneView->Family && Canvas->SceneView->Family->Scene && Canvas->SceneView->Family->Scene->GetWorld() != ActorOwner->GetWorld())
	{
		return;
	}

	if (ActorOwner)
	{
		if (bDrawOnlyWhenSelected && !ActorOwner->IsSelected())
			return;
		
		const bool bInEditor = ActorOwner->GetWorld()->WorldType == EWorldType::Editor || ActorOwner->GetWorld()->WorldType == EWorldType::EditorPreview;

#if WITH_EDITOR
		if (bInEditor)
		{
			ISGAMEVIEWRETURN()
		}
#endif
	}

	FDebugDrawDelegateHelper::DrawDebugLabels(Canvas, PlayerController);
}

UNavSplinePathRenderingComponent::UNavSplinePathRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	DrawFlagName("GameplayDebug"),
	bDrawOnlyWhenSelected(true)
{
}

FPrimitiveSceneProxy* UNavSplinePathRenderingComponent::CreateSceneProxy()
{
	FNavSplinePathSceneProxy* NewSceneProxy = new FNavSplinePathSceneProxy(this, DrawFlagName, &DebugData);
	
	if (NewSceneProxy)
	{
		if (IsInGameThread())
		{
			RenderingDebugDrawDelegateHelper.InitDelegateHelper(NewSceneProxy);
			RenderingDebugDrawDelegateHelper.ReregisterDebugDrawDelgate();
		}
		else
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("Couldn't register delegate helper on non-game thread"));
		}
	}

	return NewSceneProxy;
}

FBoxSphereBounds UNavSplinePathRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	NavSplinePath::FDebugRenderElements DebugRenderElements;

	FNavSplinePathSceneProxy::DrawPathData(const_cast<UNavSplinePathRenderingComponent*>(this), DebugRenderElements);	

	if (DebugRenderElements.Lines.Num() > 0)
	{
		FBoxSphereBounds DebugBounds(FBox(DebugRenderElements.Lines[0].Start, DebugRenderElements.Lines[0].End));
		for (int32 Index = 1; Index < DebugRenderElements.Lines.Num(); ++Index)
		{
			DebugBounds = DebugBounds + FBox(DebugRenderElements.Lines[Index].Start, DebugRenderElements.Lines[Index].End);
		}

		return DebugBounds;
	}
	
	static FSphere BoundingSphere(FVector::ZeroVector, 0.f);
	return FBoxSphereBounds(BoundingSphere).TransformBy(LocalToWorld);
}

void UNavSplinePathRenderingComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	RenderingDebugDrawDelegateHelper.RegisterDebugDrawDelgate();
}

void UNavSplinePathRenderingComponent::DestroyRenderState_Concurrent()
{
	RenderingDebugDrawDelegateHelper.UnregisterDebugDrawDelgate();

	Super::DestroyRenderState_Concurrent();
}
