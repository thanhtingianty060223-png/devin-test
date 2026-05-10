// Copyright Void Interactive, 2021

#include "SplineTrigger.h"

#include "Components/SplineComponent.h"

DECLARE_CYCLE_STAT(TEXT("Spline Trigger ~ Tick"), STAT_SplineTriggerTick, STATGROUP_SplineTrigger);

ASplineTrigger::ASplineTrigger()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.1);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline Component"));
	SplineComponent->SetClosedLoop(true);
	
#if WITH_EDITOR
	SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 0.023497f, 0.203665f, 1.0f);
	SplineComponent->bShouldVisualizeScale = true;
	SplineComponent->ScaleVisualizationWidth = 50.0f;
#endif

	bFindCameraComponentWhenViewTarget = false;

	bEnabled = true;

	SetCanBeDamaged(false);
}

bool ASplineTrigger::IsActorInsideSplineEnclosure(AActor* InActor) const
{
	return !IsActorOutsideSplineEnclosure(InActor);
}

bool ASplineTrigger::IsActorOutsideSplineEnclosure(AActor* InActor) const
{
	if (!InActor)
		return false;

	if (SplineComponent->GetNumberOfSplinePoints() < 3)
		return false;

	const FVector ActorLocation = InActor->GetActorLocation();

	FVector ClosestPointOnSpline = SplineComponent->FindLocationClosestToWorldLocation(ActorLocation, ESplineCoordinateSpace::World);
	FVector SplinePointRightVector = SplineComponent->FindRightVectorClosestToWorldLocation(ActorLocation, ESplineCoordinateSpace::World);

	DrawDebugDirectionalArrow(GetWorld(), ClosestPointOnSpline, ClosestPointOnSpline + (SplinePointRightVector * 100), 30, FColor::Blue, false, 5, 0, 10);
	return FVector::DotProduct((ClosestPointOnSpline - ActorLocation).GetSafeNormal2D(), SplinePointRightVector) < 0.f;
}

void ASplineTrigger::ToggleDrawDebug()
{
	bDrawDebugElements = !bDrawDebugElements;
}

void ASplineTrigger::EnableTrigger()
{
	bEnabled = true;
}

void ASplineTrigger::DisableTrigger()
{
	bEnabled = false;
}

void ASplineTrigger::BeginPlay()
{
	Super::BeginPlay();

	// Need at least 3 points to create an area
	if (SplineComponent->GetNumberOfSplinePoints() < 2)
	{
		ULog::Info("Spline Enclosure Needs At Least 3 Vertices. Current Vertex Count: " + FString::FromInt(SplineComponent->GetNumberOfSplinePoints()) + ". Destroying Spline Enclosure");
		Destroy();
	}

	// This works fine and should always work in theory, but how can it be guaranteed
	// Safer to just expose an invert bounds button, less frustrating for LDs in case some weird edge case gets the drection wrong
	// bInvertBounds = ShouldInvertBoundsChecks();
}

void ASplineTrigger::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_SplineTriggerTick);

	if (bEnabled)
	{
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (PlayerCharacter && SplineComponent->GetNumberOfSplinePoints() > 2)
		{
			const FVector PlayerLocation = PlayerCharacter->GetActorLocation();

			FVector ClosestPointOnSpline = SplineComponent->FindLocationClosestToWorldLocation(PlayerLocation, ESplineCoordinateSpace::World);
			FVector SplinePointRightVector = SplineComponent->FindRightVectorClosestToWorldLocation(PlayerLocation, ESplineCoordinateSpace::World);
			
			bIsOutsideSplineEnclosure = FVector::DotProduct((ClosestPointOnSpline - PlayerLocation).GetSafeNormal2D(), SplinePointRightVector) < 0.f;
			if (bInvertBounds)
				bIsOutsideSplineEnclosure = !bIsOutsideSplineEnclosure;
			
			if (!bIsOutsideSplineEnclosure && !bSplineEnclosureEnteredEventBroadcasted)
			{
				TryBroadcast_SplineEnclosureEntered(PlayerCharacter);
			}
			else if (bIsOutsideSplineEnclosure && !bSplineEnclosureExitedEventBroadcasted)
			{
				TryBroadcast_SplineEnclosureExited(PlayerCharacter);
			}

			// TODO: Run OOB checks server-side rather than rely on clients
			// All of this is done client side (Everything in blueprints too). This requires the clients to inform server when they've stayed out of bounds too long and teleport back.
			// If we ever wanna improve network security for this, have the server be running these checks
			if (!bIsOutsideSplineEnclosure)
			{
				// Add some offset back into the zone more for when player respawns
				FVector AdditionalPositionOffset = (PlayerLocation - ClosestPointOnSpline).GetSafeNormal2D() * 200.0f;
				LastValidPlayerLocation = PlayerLocation + AdditionalPositionOffset;
			}

			#if WITH_EDITOR
			if (bDrawDebugElements)
			{
				const FColor LineColor = bIsOutsideSplineEnclosure ? FColor::Red : FColor::Green; 
				DrawDebugDirectionalArrow(GetWorld(), ClosestPointOnSpline, ClosestPointOnSpline + (SplinePointRightVector * 100), 30, FColor::Blue, false, 0.1, 0, 10);
				DrawDebugDirectionalArrow(GetWorld(), FVector(PlayerLocation.X, PlayerLocation.Y, GetActorLocation().Z), ClosestPointOnSpline, 30, FColor::Red, false, 0.1, 0, 10);
			}
			#endif
		}
	}
}

void ASplineTrigger::TryBroadcast_SplineEnclosureEntered(class APlayerCharacter* PlayerCharacter)
{
	if (bEnabled)
	{
		OnSplineEnclosureEntered(PlayerCharacter);
		Delegate_OnSplineEnclosureEntered.Broadcast(PlayerCharacter);

		bSplineEnclosureEnteredEventBroadcasted = !bSplineEnclosureEnteredEventBroadcasted;

		bSplineEnclosureExitedEventBroadcasted = false;
	}
}

void ASplineTrigger::TryBroadcast_SplineEnclosureExited(class APlayerCharacter* PlayerCharacter)
{
	if (bEnabled)
	{
		OnSplineEnclosureExited(PlayerCharacter);
		Delegate_OnSplineEnclosureExited.Broadcast(PlayerCharacter);

		bSplineEnclosureExitedEventBroadcasted = !bSplineEnclosureExitedEventBroadcasted;

		bSplineEnclosureEnteredEventBroadcasted = false;
	}
}

void ASplineTrigger::OnSplineEnclosureEntered_Implementation(class APlayerCharacter* PlayerCharacter)
{
	#if WITH_EDITOR
	if (PlayerCharacter)
		ULog::Info(PlayerCharacter->GetName() + " Entered Spline Enclosure: " + GetName());	
	else
		ULog::Info("Entered Spline Enclosure: " + GetName());	
	#endif
}

void ASplineTrigger::OnSplineEnclosureExited_Implementation(class APlayerCharacter* PlayerCharacter)
{
	#if WITH_EDITOR
	if (PlayerCharacter)
		ULog::Info(PlayerCharacter->GetName() + " Exited Spline Enclosure: " + GetName());
	else
		ULog::Info("Exited Spline Enclosure: " + GetName());	
	#endif
}

bool ASplineTrigger::ShouldInvertBoundsChecks()
{
	// For default setup, need the spline to be in a specific direction. The Right vector of the direction at any given point on the spline should be pointing out of bounds.
	// If the spline is setup inverted, with the direction right vector pointing inwards, we need to invert the checks done on tick to determine if actor is out of bounds.

	// To determine whether the spline is setup to work with default implementation or if it needs to be inverted,
	// we take the average location of the spline points and treat that as the centre. We then take the direction from the calculated centre to each spline point,
	// and dot that with the right vector of the direction at that point. Taking the average of these dot products, we should expect that if it is setup correctly,
	// the average dot product will be > 0. If not, invert
	AverageSplinePointLocation = FVector::ZeroVector;
	for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		AverageSplinePointLocation += SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
	}

	AverageSplinePointLocation /= SplineComponent->GetNumberOfSplinePoints();

	float AverageDotProduct = 0;
	
	for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		AverageDotProduct += FVector::DotProduct(SplineComponent->GetRightVectorAtSplinePoint(i, ESplineCoordinateSpace::World), (SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World) - AverageSplinePointLocation).GetSafeNormal2D());  
	}

	AverageDotProduct /= SplineComponent->GetNumberOfSplinePoints();

	return AverageDotProduct < 0.f;

}