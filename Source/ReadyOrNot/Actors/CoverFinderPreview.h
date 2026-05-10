// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/CoverQueryResultInterface.h"
#include "CoverFinderPreview.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ACoverFinderPreview final : public AActor, public ICoverQueryResultInterface
{
	GENERATED_BODY()
	
public:	
	ACoverFinderPreview();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration")
	float SearchExtent = 1250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration")
	float SearchDangerZone = 700.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration")
	ECoverSearchMode SearchMode = ECoverSearchMode::NonWallOnly;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration")
	ECoverStance CoverStance = ECoverStance::Both;
	
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest SearchModeTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest HeightDifferenceTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest LineOfSightTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest CoverBehindInstigatorTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest SufficientCoverTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest DistanceToInstigatorTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest DirectionMatchTest;
	UPROPERTY(EditAnywhere, Category = "Tests")
	FCoverQueryTest RoomTest;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	int32 NumCoverPointsFound = 0;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	float CoverSearchTimeMs = 0.0f;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	TWeakObjectPtr<class ACoverPoint> BestCover = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawLabels : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug", meta = (EditCondition = "bDrawLabels", EditConditionHides))
	uint8 bDrawScore : 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawPass : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	uint8 bDrawFail : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug", meta = (EditCondition = "bDrawLabels && bDrawFail"))
	uint8 bDrawFailReason : 1;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* InstigatorActorBillboard = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* AIActorBillboard = nullptr;
	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	//USphereComponent* NavObstacleSphereComponent = nullptr;
	#endif

	TArray<FCoverData> GetAllCoverPointsInBox(const FTransform& InBoxTransform, float SearchRadius) const;
	class ACoverPoint* FindCoverPointActorInBox(const FVector& InLocation, const FTransform& InBoxTransform, float SearchRadius) const;

	void OnNewCoverFoundAsync(uint32 NumCoverFound, float TimeMs);
	void OnNoCoverFoundAsync(uint32 NumCoverFound, float TimeMs);

	FCoverData BestFoundCoverData;
	FFindCoverQuery CoverSearchQuery;

	// ICoverQueryResultInterface
	virtual const FCoverData* GetCoverData() override { return &BestFoundCoverData; }
	virtual const FFindCoverQuery* GetCoverQuery() override { return &CoverSearchQuery; }
	virtual bool ShouldDrawDebugLabels() const override { return bDrawLabels; }
	virtual bool ShouldDrawScore() const override { return bDrawScore; }
	virtual bool ShouldDrawFailReason() const override { return bDrawFailReason; }
	virtual bool ShouldDrawPass() const override { return bDrawPass; }
	virtual bool ShouldDrawFail() const override { return bDrawFail; }
	/////////////////////////////
	
private:
	#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UCoverFinderRenderingComponent* DebugRenderingComponent = nullptr;
	#endif
};
