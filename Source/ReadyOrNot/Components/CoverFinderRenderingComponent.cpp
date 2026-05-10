// Void Interactive, 2020

#include "Components/CoverFinderRenderingComponent.h"

#include "NavigationSystem.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

FCoverFinderSceneProxy::FCoverFinderSceneProxy(UPrimitiveComponent* InComponent, const FString& InViewFlagName, CoverFinder::FDebugRenderElements* InDebugRenderElements)
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

		QueryDataSource = Cast<ICoverQueryResultInterface>(ActorOwner);
		
		if (!QueryDataSource)
		{
			QueryDataSource = Cast<ICoverQueryResultInterface>(InComponent);
		}

		if (UCoverFinderRenderingComponent* RenderComponent = Cast<UCoverFinderRenderingComponent>(InComponent))
			bDrawOnlyWhenSelected = RenderComponent->bDrawOnlyWhenSelected;

		if (InDebugRenderElements)
		{
			InDebugRenderElements->Reset();
			
			if (QueryDataSource)
			{
				CollectCoverData(InComponent, QueryDataSource, *InDebugRenderElements);
			}
			
			Lines = InDebugRenderElements->Lines;
			Spheres = InDebugRenderElements->Spheres;
			Texts = InDebugRenderElements->Texts;
		}
	}
}

void FCoverFinderSceneProxy::CollectCoverData(UPrimitiveComponent* InComponent, ICoverQueryResultInterface* InQueryDataSource, CoverFinder::FDebugRenderElements& DebugElements)
{
	#if !UE_BUILD_SHIPPING
	AActor* ActorOwner = InComponent ? InComponent->GetOwner() : nullptr;
	
	if (!InQueryDataSource)
	{
		InQueryDataSource = Cast<ICoverQueryResultInterface>(ActorOwner);
		if (!InQueryDataSource)
		{
			InQueryDataSource = Cast<ICoverQueryResultInterface>(InComponent);
			if (!InQueryDataSource)
			{
				return;
			}
		}
	}
	
	const FCoverData* Winner = InQueryDataSource->GetCoverData();
	const FFindCoverQuery* QueryData = InQueryDataSource->GetCoverQuery();

	const bool bDrawPass = InQueryDataSource->ShouldDrawPass();
	const bool bDrawFail = InQueryDataSource->ShouldDrawFail();
	const bool bDrawFailReason = InQueryDataSource->ShouldDrawFailReason();
	const bool bDrawScore = InQueryDataSource->ShouldDrawScore();

	float Radius = 15.0f;

	FVector InstigatorLoc = QueryData->InstigatorTransform.GetLocation();
	FVector AILoc = QueryData->OurTransform.GetLocation();

	const TArray<FCoverQueryItem>& QueryItems = QueryData->QueryItems;

	if (QueryItems.Num() == 0)
		return;

	const FCoverQueryItem* WinnerQueryItem = nullptr;
	
	for (const FCoverQueryItem& QueryItem : QueryItems)
	{
		if (*QueryItem.CoverPoint == *Winner)
		{
			WinnerQueryItem = &QueryItem;
			break;
		}
	}
	
	DebugElements = QueryData->DebugRenderElements;

	float MinScore = 0.0f;
	float MaxScore = 0.0f;
	
	for (const FCoverQueryItem& QueryItem : QueryItems)
	{
		MinScore = FMath::Min(MinScore, QueryItem.Score);
		MaxScore = FMath::Max(MaxScore, QueryItem.Score);
	}
	
	for (const FCoverQueryItem& QueryItem : QueryItems)
	{
		if (&QueryItem == WinnerQueryItem)
			continue;
		
		const FCoverData* CoverPoint = QueryItem.CoverPoint;
		const FNavPathSharedPtr& Path = QueryItem.CoverPath;

		if (Path.IsValid())
		{
			if (Path->GetPathPoints().Num() > 1)
			{
				for (int32 i = 1; i < Path->GetPathPoints().Num(); i++)
				{
					DebugElements.Lines.Add({Path->GetPathPoints()[i-1].Location + FVector(0.0f, 0.0f, 30.0f), Path->GetPathPoints()[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Red, 2.0f});
				}
			}
		}
		
		if (QueryItem.bDiscarded)
		{
			if (bDrawFail)
			{
				DebugElements.Spheres.Add({15.0f, CoverPoint->CoverLocation, FColor::Red});

				if (!QueryItem.DiscardReason.IsEmpty())
					DebugElements.Texts.Add({bDrawFailReason ? FString::Printf(TEXT("Fail (%s)"), *QueryItem.DiscardReason) : "Fail", CoverPoint->CoverLocation, FColor::White});
			}
		}
		else
		{
			if (bDrawPass)
			{
				DebugElements.Spheres.Add({15.0f, CoverPoint->CoverLocation, FColor::MakeRedToGreenColorFromScalar(FMath::GetMappedRangeValueClamped(FVector2D(MinScore, MaxScore), FVector2D(0.0f, 1.0f), QueryItem.Score))});
				DebugElements.Texts.Add({bDrawScore ? FString::Printf(TEXT("Pass (%.2f)"), QueryItem.Score) : "Pass", CoverPoint->CoverLocation, FColor::White});		
			}
		}
	}
	
	DebugElements.Spheres.Add(FSphere(Radius, InstigatorLoc, FColor::Purple));
	DebugElements.Spheres.Add(FSphere(Radius, AILoc, FColor::Purple));
	DebugElements.Texts.Add(FText3d(FString::Printf(TEXT("Instigator %s"), *InstigatorLoc.ToString()), InstigatorLoc, FColor::White));
	DebugElements.Texts.Add(FText3d(FString::Printf(TEXT("AI %s"), *AILoc.ToString()), AILoc, FColor::White));

	if (!WinnerQueryItem)
		return;
	
	DebugElements.Spheres.Add(FSphere(Radius, Winner->CoverLocation, FColor::Green));
	DebugElements.Texts.Add(FText3d(bDrawScore ? FString::Printf(TEXT("Winner (%.2f)"), WinnerQueryItem->Score) : "Winner", Winner->CoverLocation, FLinearColor::White));

	if (bDrawPass)
	{
		const FNavPathSharedPtr& WinnerPath = WinnerQueryItem->CoverPath;
		if (WinnerPath.IsValid())
		{
			const TArray<FNavPathPoint>& PathPoints = WinnerPath->GetPathPoints();

			for (int32 i = 1; i < PathPoints.Num(); i++)
			{
				DebugElements.Lines.Add({WinnerPath->GetPathPoints()[i-1].Location + FVector(0.0f, 0.0f, 35.0f), WinnerPath->GetPathPoints()[i].Location + FVector(0.0f, 0.0f, 35.0f), FColor::Green, 5.0f});
			}
		}
	}

	DebugElements.Lines.Add({Winner->CoverLocation + FVector(0.0f, 0.0f, 10.0f), Winner->CoverLocation + FVector::UpVector * 10000.0f, FColor::Cyan, 10.0f});

	// Draw entry points
	{
		constexpr float DistanceFromCover = 130.0f;
		constexpr float Angle = 50.0f;

		const FVector& DirectionToUs = (QueryData->OurTransform.GetLocation() - Winner->CoverLocation).GetSafeNormal2D();
		const float RightDotProduct = FVector::DotProduct(DirectionToUs, FVector::CrossProduct(Winner->CoverNormal, FVector::UpVector));
		const bool bIsRightSideOfCoverPoint = RightDotProduct > 0.0f;
		const FVector EntryPoint1 = Winner->CoverLocation + Winner->CoverNormal.RotateAngleAxis(-Angle, FVector::UpVector) * DistanceFromCover;
		const FVector EntryPoint2 = Winner->CoverLocation + Winner->CoverNormal.RotateAngleAxis(Angle, FVector::UpVector) * DistanceFromCover;
		
		DebugElements.Spheres.Add(FSphere(5.0f, bIsRightSideOfCoverPoint ? EntryPoint1 : EntryPoint2, FColor::Purple));
	}
	#endif
}

FPrimitiveViewRelevance FCoverFinderSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	
	Result.bDrawRelevance = IsShown(View) && (!bDrawOnlyWhenSelected || (bDrawOnlyWhenSelected && SafeIsActorSelected()));
	Result.bDynamicRelevance = true;
	
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	Result.bSeparateTranslucency = Result.bNormalTranslucency = IsShown(View);

	return Result;
}

bool FCoverFinderSceneProxy::SafeIsActorSelected() const
{
	if (ActorOwner)
	{
		return ActorOwner->IsSelected();
	}

	return false;
}

SIZE_T FCoverFinderSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FCoverFinderRenderingDebugDrawDelegateHelper::DrawDebugLabels(UCanvas* Canvas, APlayerController* PlayerController)
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

	if (!QueryDataSource)
		return;
		
	if (!QueryDataSource->ShouldDrawDebugLabels())
		return;

	FDebugDrawDelegateHelper::DrawDebugLabels(Canvas, PlayerController);
}

UCoverFinderRenderingComponent::UCoverFinderRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	DrawFlagName("GameplayDebug"),
	bDrawOnlyWhenSelected(true)
{
}

FPrimitiveSceneProxy* UCoverFinderRenderingComponent::CreateSceneProxy()
{
	FCoverFinderSceneProxy* NewSceneProxy = new FCoverFinderSceneProxy(this, DrawFlagName, &DebugData);
	
	if (NewSceneProxy)
	{
		if (IsInGameThread())
		{
			CoverRenderingDebugDrawDelegateHelper.InitDelegateHelper(NewSceneProxy);
			CoverRenderingDebugDrawDelegateHelper.ReregisterDebugDrawDelgate();
		}
		else
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("Couldn't register delegate helper on non-game thread"));
		}
	}

	return NewSceneProxy;
}

FBoxSphereBounds UCoverFinderRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	ICoverQueryResultInterface* QueryDataSource = Cast<ICoverQueryResultInterface>(GetOwner());
	if (!QueryDataSource)
	{
		QueryDataSource = Cast<ICoverQueryResultInterface>(const_cast<UCoverFinderRenderingComponent*>(this));
	}

	CoverFinder::FDebugRenderElements DebugRenderElements;

	if (QueryDataSource != nullptr)
	{
		FCoverFinderSceneProxy::CollectCoverData(const_cast<UCoverFinderRenderingComponent*>(this), QueryDataSource, DebugRenderElements);	
	}

	if (DebugRenderElements.Spheres.Num() > 0)
	{
		FBoxSphereBounds DebugBounds(FSphere(DebugRenderElements.Spheres[0].Location, DebugRenderElements.Spheres[0].Radius));
		for (int32 Index = 1; Index < DebugRenderElements.Spheres.Num(); ++Index)
		{
			DebugBounds = DebugBounds + FSphere(DebugRenderElements.Spheres[Index].Location, DebugRenderElements.Spheres[Index].Radius);
		}

		return DebugBounds;
	}
	
	static FSphere BoundingSphere(FVector::ZeroVector, 0.f);
	return FBoxSphereBounds(BoundingSphere).TransformBy(LocalToWorld);
}

void UCoverFinderRenderingComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	CoverRenderingDebugDrawDelegateHelper.RegisterDebugDrawDelgate();
}

void UCoverFinderRenderingComponent::DestroyRenderState_Concurrent()
{
	CoverRenderingDebugDrawDelegateHelper.UnregisterDebugDrawDelgate();

	Super::DestroyRenderState_Concurrent();
}
