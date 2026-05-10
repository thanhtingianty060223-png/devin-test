// Void Interactive, 2020

#include "CoverFinderPreview.h"

#include "CoverSystem.h"
#include "Actors/CoverPoint.h"

#include "Components/BillboardComponent.h"
#include "Components/CoverFinderRenderingComponent.h"

#include "UObject/ConstructorHelpers.h"

#include "NavigationPath.h"

#include "Info/Activities/Tasks/FindCoverTask.h"

#include "Kismet/KismetMathLibrary.h"
#include "EngineUtils.h"

#include "DrawDebugHelpers.h"

ACoverFinderPreview::ACoverFinderPreview()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.TickInterval = 1.0f;
	#endif

	bIsEditorOnlyActor = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	SceneComponent->SetMobility(EComponentMobility::Static);
	SceneComponent->SetCanEverAffectNavigation(false);

	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> SusBillboard_Icon(TEXT("Texture2D'/Game/Blueprints/Widgets/Editor/MapIcons/Hotel.Hotel'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SwatBillboard_Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_ALPHA.STACKING_ALPHA'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Billboard_Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/FallbackIcon.FallbackIcon'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetCanEverAffectNavigation(false);
	BillboardComponent->bEnableAutoLODGeneration = false;
	BillboardComponent->bReceiveMobileCSMShadows = false;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->ScreenSize = 0.0035f;
	BillboardComponent->SetSprite(Billboard_Icon.Object);
	
	InstigatorActorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Instigator Actor Billboard"));
	InstigatorActorBillboard->SetupAttachment(GetRootComponent());
	InstigatorActorBillboard->SetRelativeLocation(FVector::ZeroVector);
	InstigatorActorBillboard->SetMobility(EComponentMobility::Static);
	InstigatorActorBillboard->SetCanEverAffectNavigation(false);
	InstigatorActorBillboard->bEnableAutoLODGeneration = false;
	InstigatorActorBillboard->bReceiveMobileCSMShadows = false;
	InstigatorActorBillboard->bIsScreenSizeScaled = true;
	InstigatorActorBillboard->ScreenSize = 0.0035f;
	InstigatorActorBillboard->SetSprite(SwatBillboard_Icon.Object);
	
	AIActorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("AI Actor Billboard"));
	AIActorBillboard->SetupAttachment(GetRootComponent());
	AIActorBillboard->SetRelativeLocation(FVector::ZeroVector);
	AIActorBillboard->SetWorldScale3D(FVector(0.5f));
	AIActorBillboard->SetMobility(EComponentMobility::Static);
	AIActorBillboard->SetCanEverAffectNavigation(false);
	AIActorBillboard->bEnableAutoLODGeneration = false;
	AIActorBillboard->bReceiveMobileCSMShadows = false;
	AIActorBillboard->bIsScreenSizeScaled = true;
	AIActorBillboard->ScreenSize = 0.0035f;
	AIActorBillboard->SetSprite(SusBillboard_Icon.Object);

	/*
	NavObstacleSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Nav Obstacle Sphere Component"));
	NavObstacleSphereComponent->SetupAttachment(InstigatorActorBillboard);
	NavObstacleSphereComponent->SetRelativeLocation(FVector::ZeroVector);
	NavObstacleSphereComponent->SetMobility(EComponentMobility::Static);
	NavObstacleSphereComponent->AreaClass = UNavArea_Obstacle::StaticClass();
	NavObstacleSphereComponent->SetSphereRadius(SearchDangerZone);
	NavObstacleSphereComponent->bDynamicObstacle = true;
	NavObstacleSphereComponent->SetEnableGravity(false);
	NavObstacleSphereComponent->bApplyImpulseOnDamage = false;
	NavObstacleSphereComponent->bReplicatePhysicsToAutonomousProxy = false;
	NavObstacleSphereComponent->SetGenerateOverlapEvents(false);
	NavObstacleSphereComponent->CanCharacterStepUpOn = ECB_No;
	NavObstacleSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NavObstacleSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	*/
	
	DebugRenderingComponent = CreateDefaultSubobject<UCoverFinderRenderingComponent>(TEXT("Debug Rendering Component"));
	DebugRenderingComponent->SetupAttachment(GetRootComponent());
	DebugRenderingComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	#endif

	bDrawLabels = true;
	bDrawScore = true;
	bDrawFailReason = true;
	bDrawPass = true;
	bDrawFail = true;

	SearchModeTest.TestPurpose = CoverQueryTests::SearchMode.TestPurpose;
	HeightDifferenceTest.TestPurpose = CoverQueryTests::HeightDifference.TestPurpose;
	LineOfSightTest.TestPurpose = CoverQueryTests::LineOfSight.TestPurpose;
	CoverBehindInstigatorTest.TestPurpose = CoverQueryTests::CoverBehindInstigator.TestPurpose;
	SufficientCoverTest.TestPurpose = CoverQueryTests::SufficientCover.TestPurpose;
	DistanceToInstigatorTest.TestPurpose = CoverQueryTests::Distance.TestPurpose;
	DirectionMatchTest.TestPurpose = CoverQueryTests::DirectionMatch.TestPurpose;
	RoomTest.TestPurpose = CoverQueryTests::Room.TestPurpose;
}

void ACoverFinderPreview::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->WorldType != EWorldType::Editor && GetWorld()->WorldType != EWorldType::EditorPreview)
	{
		Destroy();
	}
}

void ACoverFinderPreview::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITORONLY_DATA
	if (!IsSelected())
	{
		BestCover = nullptr;
		return;
	}

	if (GetWorld()->WorldType != EWorldType::Editor && GetWorld()->WorldType != EWorldType::EditorPreview)
	{
		//SetActorTickEnabled(false);
		SetActorTickInterval(1.0f);
		BestCover = nullptr;

		//DESTROY_COMPONENT(NavObstacleSphereComponent);
		return;
	}

	FFindCoverDelegate NewCoverFoundDelegate;
	NewCoverFoundDelegate.BindUObject(this, &ACoverFinderPreview::OnNewCoverFoundAsync);
	
	FFindCoverDelegate NoCoverFoundDelegate;
	NoCoverFoundDelegate.BindUObject(this, &ACoverFinderPreview::OnNoCoverFoundAsync);

	CoverSearchQuery = {};
	CoverSearchQuery.World = GetWorld();
	CoverSearchQuery.InstigatorTransform = InstigatorActorBillboard->GetComponentTransform();
	CoverSearchQuery.OurTransform = AIActorBillboard->GetComponentTransform();
	CoverSearchQuery.SearchMode = SearchMode;
	CoverSearchQuery.CoverStance = CoverStance;
	CoverSearchQuery.SearchExtent = SearchExtent;
	CoverSearchQuery.SearchDangerRadiusFromInstigator = SearchDangerZone;
	CoverSearchQuery.bDrawDebug = true;
	CoverSearchQuery.DrawTime = DeltaTime + 0.05f;

	CoverQueryTests::UpdateTests();

	SearchModeTest.TestFunction = CoverQueryTests::SearchMode.TestFunction;
	HeightDifferenceTest.TestFunction = CoverQueryTests::HeightDifference.TestFunction;
	LineOfSightTest.TestFunction = CoverQueryTests::LineOfSight.TestFunction;
	CoverBehindInstigatorTest.TestFunction = CoverQueryTests::CoverBehindInstigator.TestFunction;
	SufficientCoverTest.TestFunction = CoverQueryTests::SufficientCover.TestFunction;
	DistanceToInstigatorTest.TestFunction = CoverQueryTests::Distance.TestFunction;
	DirectionMatchTest.TestFunction = CoverQueryTests::DirectionMatch.TestFunction;
	RoomTest.TestFunction = CoverQueryTests::Room.TestFunction;
	
	TArray<FCoverQueryTest> CoverQueryTests;
	CoverQueryTests.Reserve(6);
	CoverQueryTests.Add(SearchModeTest);
	CoverQueryTests.Add(HeightDifferenceTest);
	CoverQueryTests.Add(LineOfSightTest);
	CoverQueryTests.Add(CoverBehindInstigatorTest);
	CoverQueryTests.Add(DistanceToInstigatorTest);
	CoverQueryTests.Add(DirectionMatchTest);
	CoverQueryTests.Add(SufficientCoverTest);
	CoverQueryTests.Add(RoomTest);

	const FVector Location_Offset = CoverSearchQuery.OurTransform.GetLocation() - FVector::UpVector * 100.0f;
	const FVector Origin = Location_Offset + (CoverSearchQuery.OurTransform.GetLocation() - CoverSearchQuery.InstigatorTransform.GetLocation()).GetSafeNormal2D() * SearchExtent/2;
	
	FTransform BoundsTransformTest;
	BoundsTransformTest.SetLocation(Origin);
	BoundsTransformTest.SetRotation(CoverSearchQuery.OurTransform.GetRotation());
	BoundsTransformTest.SetScale3D(FVector::OneVector);

	TArray<FCoverData> FoundCoverPoints = GetAllCoverPointsInBox(BoundsTransformTest, SearchExtent);
	
	const FFindCoverResult& Result = FFindCoverTask::FindBestCover(FoundCoverPoints, CoverSearchQuery, CoverQueryTests, false);
	if (IsCoverResultValid(Result))
	{
		BestFoundCoverData = *Result.BestCover;
	}
	else
	{
		BestFoundCoverData = {};
	}

	CoverSearchTimeMs = Result.TimeMs;
	NumCoverPointsFound = Result.NumCoverFound;

	BestCover = FindCoverPointActorInBox(BestFoundCoverData.CoverLocation, BoundsTransformTest, SearchExtent);
	
	// Debug visual
	if (CoverSearchQuery.bDrawDebug)
	{
		const FVector SearchBoxExtent = FVector(SearchExtent, SearchExtent, 205.0f);
		const FBox SearchBox = FBoxCenterAndExtent(BoundsTransformTest.GetLocation(), SearchBoxExtent).GetBox();

		DrawDebugBox(GetWorld(), SearchBox.GetCenter(), SearchBox.GetExtent(), AIActorBillboard->GetComponentQuat(), NumCoverPointsFound > 0 ? FColor::Green : FColor::Red, false, DeltaTime + 0.05f, 0, 10.0f);

		DrawDebugCircle(GetWorld(), InstigatorActorBillboard->GetComponentLocation(), SearchDangerZone, 64, FColor::Red, false, DeltaTime + 0.05f, 0, 5.0f, FVector::RightVector, FVector::ForwardVector);
		DrawDebugCircle(GetWorld(), InstigatorActorBillboard->GetComponentLocation(), SearchDangerZone*2, 64, FColor::Yellow, false, DeltaTime + 0.05f, 0, 5.0f, FVector::RightVector, FVector::ForwardVector);
		DrawDebugCircle(GetWorld(), InstigatorActorBillboard->GetComponentLocation(), SearchDangerZone*3, 64, FColor::Green, false, DeltaTime + 0.05f, 0, 5.0f, FVector::RightVector, FVector::ForwardVector);

		if (HasAnyFlags(RF_ClassDefaultObject))
			return;
		
		DebugRenderingComponent->MarkRenderStateDirty();
	}
	#endif
}

bool ACoverFinderPreview::ShouldTickIfViewportsOnly() const
{
	return true;
}

#if WITH_EDITORONLY_DATA
void ACoverFinderPreview::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "SearchDangerZone")
	{
		//NavObstacleSphereComponent->SetSphereRadius(SearchDangerZone);
		//NavObstacleSphereComponent->UpdateNavigationBounds();
	}
}
#endif

TArray<FCoverData> ACoverFinderPreview::GetAllCoverPointsInBox(const FTransform& InBoxTransform, const float SearchRadius) const
{
	TArray<FCoverData> CoverPoints;
	CoverPoints.Reserve(1000);
	
	// Create a search area to find cover points
	const FVector SearchBoxExtent = FVector(SearchRadius, SearchRadius, 205.0f);

	for (TActorIterator<ACoverPoint> It(GetWorld()); It; ++It)
	{
		const ACoverPoint* CoverPoint = *It;
		
		if (UKismetMathLibrary::IsPointInBoxWithTransform(CoverPoint->GetActorLocation(), InBoxTransform, SearchBoxExtent))
		{
			CoverPoints.Add(ACoverPoint::ToCoverData(CoverPoint));
		}
	}

	return CoverPoints;
}

ACoverPoint* ACoverFinderPreview::FindCoverPointActorInBox(const FVector& InLocation, const FTransform& InBoxTransform, float SearchRadius) const
{
	// Create a search area to find cover points
	const FVector SearchBoxExtent = FVector(SearchRadius, SearchRadius, 205.0f);

	for (TActorIterator<ACoverPoint> It(GetWorld()); It; ++It)
	{
		ACoverPoint* CoverPoint = *It;
		
		if (UKismetMathLibrary::IsPointInBoxWithTransform(CoverPoint->GetActorLocation(), InBoxTransform, SearchBoxExtent))
		{
			if (InLocation.Equals(CoverPoint->GetActorLocation(), 0.001f))
				return CoverPoint;
		}
	}

	return nullptr;
}

void ACoverFinderPreview::OnNewCoverFoundAsync(uint32 NumCoverFound, float TimeMs)
{
	NumCoverPointsFound = NumCoverFound;
	CoverSearchTimeMs = TimeMs;
}

void ACoverFinderPreview::OnNoCoverFoundAsync(uint32 NumCoverFound, float TimeMs)
{
	BestFoundCoverData = {};
	NumCoverPointsFound = NumCoverFound;
	CoverSearchTimeMs = TimeMs;
}
