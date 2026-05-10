// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "NavigationSplinePathPreview.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ANavigationSplinePathPreview : public AActor
{
	GENERATED_BODY()
	
public:
	ANavigationSplinePathPreview();

	FORCEINLINE FSplineCurves GetSplinePath() const { return SplineCurves; }

protected:
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* StartBillboardComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* EndBillboardComponent = nullptr;
#endif
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = 0.0f))
	float PathPointDistanceThreshold = 50.0f;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = 1, ClampMax = 1000))
	int32 PathPointSubStep = 20;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawRawPath : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawRawPathPoints : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawSmoothedPath : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawSmoothedPathPoints : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawSmoothedPathPointsDetail : 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	float PathLineThickness = 5.0f;
	
	FSplineCurves SplineCurves = {};

/*private:
	void FilterPathPoints(const TArray<FNavPathPoint>& InPathPoints, TArray<FNavPathPoint>& OutFilteredPathPoints, const float MinDistanceBetweenPoints);
*/
	
private:
	#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UNavSplinePathRenderingComponent* DebugRenderingComponent = nullptr;
	#endif
};
