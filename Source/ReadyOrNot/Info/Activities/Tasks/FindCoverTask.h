// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "CoverData.h"
#include "FindCoverTask.generated.h"

struct FCoverQueryItem
{
	FCoverQueryItem() {}

	FCoverQueryItem(FCoverData* InCoverPoint)
		: CoverPoint(InCoverPoint) {}
		
	FCoverQueryItem(FCoverData* InCoverPoint, const float InScore)
		: CoverPoint(InCoverPoint), Score(InScore) {}
		
	FCoverData* CoverPoint = nullptr;
	float Score = 0.0f;
	FNavPathSharedPtr CoverPath = nullptr;
	bool bDiscarded = false;

	#if !UE_BUILD_SHIPPING
	FString DiscardReason = "";
	#endif
};

struct FFindCoverQuery
{
	FFindCoverQuery() {}

	TWeakObjectPtr<UWorld> World = nullptr;
	TSubclassOf<UNavigationQueryFilter> NavQueryFilter = nullptr;

	FTransform OurTransform;
	FTransform InstigatorTransform;

	ECoverSearchMode SearchMode = ECoverSearchMode::NonWallOnly;
	ECoverStance CoverStance = ECoverStance::Both;
	
	FCollisionQueryParams CollisionQueryParams;

	TArray<FCoverQueryItem> QueryItems;

	float SearchExtent = 1250.0f;
	float SearchDangerRadiusFromInstigator = 700.0f;

	#if !UE_BUILD_SHIPPING
	bool bDrawDebug = false;
	float DrawTime = 0.0f;
	CoverFinder::FDebugRenderElements DebugRenderElements;
	#endif
};

UENUM()
enum class ECoverQueryTestPurpose : uint8
{
	FilterOnly,
	ScoreOnly,
	FilterAndScore
};

#if UE_BUILD_SHIPPING
typedef TFunction<bool(FFindCoverQuery&, FCoverData* const, float&)> CoverQueryTestPredicate;
#define COVER_QUERY_TEST_LAMBDA_SIG [&](FFindCoverQuery& InQuery, FCoverData* const CoverPoint, float& OutScore)
#else
typedef TFunction<bool(FFindCoverQuery&, FCoverData* const, float&, FString&)> CoverQueryTestPredicate;
#define COVER_QUERY_TEST_LAMBDA_SIG [&](FFindCoverQuery& InQuery, FCoverData* const CoverPoint, float& OutScore, FString& FailureReason)
#endif

USTRUCT()
struct FCoverQueryTest
{
	GENERATED_BODY()
	
	FCoverQueryTest() {}

	FCoverQueryTest(const ECoverQueryTestPurpose& InTestPurpose, const float InScoringFactor = 1.0f)
	{
		TestPurpose = InTestPurpose;
		ScoringFactor = InScoringFactor;
	}
	
	FCoverQueryTest(const CoverQueryTestPredicate& InTestFunction, const ECoverQueryTestPurpose& InTestPurpose, const float InScoringFactor = 1.0f)
	{
		TestPurpose = InTestPurpose;
		ScoringFactor = InScoringFactor;
		TestFunction = InTestFunction;
	}

	UPROPERTY(EditAnywhere)
	ECoverQueryTestPurpose TestPurpose = ECoverQueryTestPurpose::FilterOnly;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "TestPurpose == ECoverQueryTestPurpose::ScoreOnly || TestPurpose == ECoverQueryTestPurpose::FilterAndScore", EditConditionHides))
	float ScoringFactor = 1.0f;

	CoverQueryTestPredicate TestFunction;
};

struct FFindCoverResult
{
	FFindCoverResult() {}

	const FCoverData* BestCover = nullptr;
	FNavPathSharedPtr Path = nullptr;
	uint32 NumCoverFound = 0;
	float TimeMs = 0.0f;
};

static bool IsCoverQueryValid(const FFindCoverQuery& InFindCoverQuery)
{
	return InFindCoverQuery.World.IsValid() && InFindCoverQuery.SearchExtent > 0.0f;
}

static bool IsCoverResultValid(const FFindCoverResult& InFindCoverResult)
{
	return InFindCoverResult.BestCover != nullptr;
}

DECLARE_DELEGATE_TwoParams(FFindCoverDelegate, uint32 /* NumCoverFound */, float /* TimeMs */)

namespace CoverQueryTests
{
	extern FCoverQueryTest SearchMode;
	extern FCoverQueryTest HeightDifference;
	extern FCoverQueryTest LineOfSight;
	extern FCoverQueryTest CoverBehindInstigator;
	extern FCoverQueryTest Distance;
	extern FCoverQueryTest DirectionMatch;
	extern FCoverQueryTest SufficientCover;
	extern FCoverQueryTest Room;

	extern void UpdateTests();
};

/**
 * Uses the cover system to find the best cover location
 */
class READYORNOT_API FFindCoverTask final : public FNonAbandonableTask
{
public:
	FFindCoverTask(TSharedPtr<FCoverData>& OutBestCover, const FFindCoverQuery& InFindCoverQuery, const FFindCoverDelegate& InCoverFoundDelegate = {}, const FFindCoverDelegate& InNoCoverFoundDelegate = {});

	TStatId GetStatId() const;

	void DoWork();

	static FFindCoverResult FindBestCover(TArray<FCoverData>& InCoverPoints, FFindCoverQuery& InQuery, const TArray<FCoverQueryTest>& InTests, bool bAsync = true);

protected:
	FFindCoverQuery CoverSearchQuery;

	TWeakPtr<FCoverData> BestCover = nullptr;
	
	FFindCoverDelegate NewCoverFoundDelegate;
	FFindCoverDelegate NoCoverFoundDelegate;
};
