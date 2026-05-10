// Copyright Void Interactive, 2023

#include "PredictionPFComponent.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "AISystem.h"
#include "Navigation/MetaNavMeshPath.h"

TAutoConsoleVariable<int32> CVar_PredictionPFComponent_EnableDebug(TEXT("a.PredictionPFComponent.EnableDebug"), 0, TEXT("Toggle Debug Information for Prediction PF Component."));

UPredictionPFComponent::UPredictionPFComponent(const FObjectInitializer& ObjectInitializer)
{
	PathStopLocation = FVector::ZeroVector;
	CustomAcceptanceRadius = 10.0f;
	bFixPathToAlwaysHaveBraking = true; // default on
	bFixPathRemoveClosePoints = true;
	PathPointRemoveDistanceThreshold = 100.0f;
	bAppliedAcceptanceRadiusFix = false;
}

void UPredictionPFComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	#if !UE_BUILD_SHIPPING
	// update cvar status for debugging output
	bShowDebug = (CVar_PredictionPFComponent_EnableDebug.GetValueOnAnyThread() == 1);

	if (bShowDebug)
	{
		DebugDrawPath(DeltaTime);
	}
	#endif

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

/* copy of original code with added prediction logic */
void UPredictionPFComponent::FollowPathSegment(float DeltaTime)
{
	if (!Path.IsValid() || MovementComp == nullptr)
	{
		return;
	}

	const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
	const FVector CurrentTarget = GetCurrentTargetLocation();

	// set to false by default, we will set set this back to true if appropriate
	bIsDecelerating = false;

	const bool bAccelerationBased = MovementComp->UseAccelerationForPathFollowing();
	if (bAccelerationBased)
	{
		CurrentMoveInput = (CurrentTarget - CurrentLocation).GetSafeNormal();

		if (MoveSegmentStartIndex >= DecelerationSegmentIndex)
		{
			const FVector PathEnd = Path->GetEndLocation();
			const float DistToEndSq = FVector::DistSquared(CurrentLocation, PathEnd);
			const bool bShouldDecelerate = DistToEndSq < FMath::Square(CachedBrakingDistance);
			if (bShouldDecelerate)
			{
				bIsDecelerating = true;

				const float SpeedPct = FMath::Clamp(FMath::Sqrt(DistToEndSq) / CachedBrakingDistance, 0.0f, 1.0f);
				CurrentMoveInput *= SpeedPct;

				/* what we distance match to for AI */
				PathStopLocation = GetLastPathPointWithRadius(GetAcceptanceRadius());
				
				#if !UE_BUILD_SHIPPING
				if (bShowDebug)
				{
					DrawDebugSphere(GetWorld(), PathStopLocation, 8.0f, 8.0f, FColor::Red, false, -1.0f, 0, 1.0f);
					DrawDebugSphere(GetWorld(), CurrentLocation, 8.0f, 8.0f, FColor::Blue, false, -1.0f, 0, 1.0f);
					DrawDebugLine(GetWorld(), CurrentLocation, PathStopLocation, FColor::Red, false, -1.0f, 0, 1.0f);
					const FVector DebugDistanceToStop = PathStopLocation - CurrentLocation;
					const float Dist2D = DebugDistanceToStop.Size();
					FString DebugDistanceString = FString::Printf(TEXT("DISTANCE TO STOP: %f"), Dist2D);
					GEngine->AddOnScreenDebugMessage(0, 10.0f, FColor::White, DebugDistanceString);
				}
				#endif
			}
		}

		#if !UE_BUILD_SHIPPING
		/* draw debugging in regards to radius handling */
		if (bShowDebug)
		{
			DrawDebugSphere(GetWorld(), CurrentTarget, 8.0f, 32.0f, FColor::Magenta, false, -1.0f, 0, 0.35f);

			float AgentRadiusMultiplier = 0.05f;
			float GoalRadius = 0.0f;
			// get cylinder of moving agent
			float AgentRadius = 0.0f;
			float AgentHalfHeight = 0.0f;
			AActor* MovingAgent = MovementComp->GetOwner();
			MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

			const float UseRadius = CurrentAcceptanceRadius + GoalRadius + (AgentRadius * AgentRadiusMultiplier);
			DrawDebugSphere(GetWorld(), CurrentTarget, UseRadius, 32.0f, FColor::White, false, -1.0f, 0, 0.35f);
		}
		#endif

		PostProcessMove.ExecuteIfBound(this, CurrentMoveInput);
		MovementComp->RequestPathMove(CurrentMoveInput);
	}
	else
	{
		FVector MoveVelocity = (CurrentTarget - CurrentLocation) / DeltaTime;

		const int32 LastSegmentStartIndex = Path->GetPathPoints().Num() - 2;
		const bool bNotFollowingLastSegment = (MoveSegmentStartIndex < LastSegmentStartIndex);

		PostProcessMove.ExecuteIfBound(this, MoveVelocity);
		MovementComp->RequestDirectMove(MoveVelocity, bNotFollowingLastSegment);
	}

	//Super::FollowPathSegment(DeltaTime);
}

bool UPredictionPFComponent::HasReachedCurrentTarget(const FVector& CurrentLocation) const
{
	return Super::HasReachedCurrentTarget(CurrentLocation);
}

void UPredictionPFComponent::OnUnableToMove(const UObject& Instigator)
{
	Super::OnUnableToMove(Instigator);
}

TArray<FNavPathPoint> FilterPathPoints(const TArray<FNavPathPoint>& InPathPoints, const float DistanceThreshold)
{
	TArray<FNavPathPoint> FilteredPathPoints;

	// Filter out small path points
	if (InPathPoints.Num() > 2)
	{
		for (int32 k = 1; k < InPathPoints.Num(); k += 2)
		{
			if (InPathPoints.IsValidIndex(k + 1))
			{
				if (FVector::Distance(InPathPoints[k - 1].Location, InPathPoints[k].Location) < FMath::Abs(DistanceThreshold) ||
					FVector::Distance(InPathPoints[k].Location, InPathPoints[k + 1].Location) < FMath::Abs(DistanceThreshold))
				{
					FilteredPathPoints.AddUnique(InPathPoints[k - 1]);
					FilteredPathPoints.AddUnique(InPathPoints[k + 1]);
					continue;
				}

				FilteredPathPoints.AddUnique(InPathPoints[k - 1]);
				FilteredPathPoints.AddUnique(InPathPoints[k]);
				FilteredPathPoints.AddUnique(InPathPoints[k + 1]);
			}
			else
			{
				FilteredPathPoints.AddUnique(InPathPoints[k]);
			}
		}
	}
	else
	{
		FilteredPathPoints = InPathPoints;
	}

	return FilteredPathPoints;
}

void UPredictionPFComponent::OnPathUpdated()
{
	if (Path.IsValid())
	{
		if (Path->GetPathPoints().Num() == 0)
			return;

		if (bFixPathRemoveClosePoints)
		{
			Path->GetPathPoints() = FilterPathPoints(Path->GetPathPoints(), PathPointRemoveDistanceThreshold);

			#if !UE_BUILD_SHIPPING
			if (bShowDebug)
				UE_LOG(LogTemp, Warning, TEXT("UPredictionPFComponent::OnPathUpdated: Optimized Path and removed close Path Points!"));
			#endif
		}

		if (bFixPathToAlwaysHaveBraking)
		{
			/* reset so we know we still need to apply the braking fix */
			bAppliedAcceptanceRadiusFix = false;
		}
	}

	Super::OnPathUpdated();
}

void UPredictionPFComponent::OnPathFinished(const FPathFollowingResult& Result)
{
	Super::OnPathFinished(Result);
}

void UPredictionPFComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	const float PathPointAcceptanceRadius = GET_AI_CONFIG_VAR(PathfollowingRegularPathPointAcceptanceRadius);
	const float NavLinkAcceptanceRadius = GET_AI_CONFIG_VAR(PathfollowingNavLinkAcceptanceRadius);

	int32 EndSegmentIndex = SegmentStartIndex + 1;
	const FNavigationPath* PathInstance = Path.Get();
	if (PathInstance != nullptr && PathInstance->GetPathPoints().IsValidIndex(SegmentStartIndex) && PathInstance->GetPathPoints().IsValidIndex(EndSegmentIndex))
	{
		EndSegmentIndex = DetermineCurrentTargetPathPoint(SegmentStartIndex);

		MoveSegmentStartIndex = SegmentStartIndex;
		MoveSegmentEndIndex = EndSegmentIndex;
		const FNavPathPoint& PathPt0 = PathInstance->GetPathPoints()[MoveSegmentStartIndex];
		const FNavPathPoint& PathPt1 = PathInstance->GetPathPoints()[MoveSegmentEndIndex];

		MoveSegmentStartRef = PathPt0.NodeRef;
		MoveSegmentEndRef = PathPt1.NodeRef;

		CurrentDestination = PathInstance->GetPathPointLocation(MoveSegmentEndIndex);
		const FVector SegmentStart = *PathInstance->GetPathPointLocation(MoveSegmentStartIndex);
		FVector SegmentEnd = *CurrentDestination;

		// make sure we have a non-zero direction if still following a valid path
		if (SegmentStart.Equals(SegmentEnd) && PathInstance->GetPathPoints().IsValidIndex(MoveSegmentEndIndex + 1))
		{
			MoveSegmentEndIndex++;

			CurrentDestination = PathInstance->GetPathPointLocation(MoveSegmentEndIndex);
			SegmentEnd = *CurrentDestination;
		}

		// Alex HACK: implement path fixing for braking here
		if (bFixPathToAlwaysHaveBraking)
		{
			/* only bother applying if the acceptance radius is large enough */
			if (GetAcceptanceRadius() >= 30.0f)
			{
				#if !UE_BUILD_SHIPPING
				if (bShowDebug)
					UE_LOG(LogTemp, Warning, TEXT("UPredictionPFComponent::CustomGetFinalAcceptanceRadius: Applying Acceptance Radius Fixup"));
				#endif

				OriginalAcceptanceRadius = GetAcceptanceRadius();

				/* HACK: i had to move this here because having this in on path updated is before the radius is actually set, FML */
				ModifyLastPathPointToIncludeRadius();

				SetAcceptanceRadius(CustomAcceptanceRadius);
			}
		}

		CurrentAcceptanceRadius = (PathInstance->GetPathPoints().Num() == (MoveSegmentEndIndex + 1))
			? GetFinalAcceptanceRadius(*PathInstance, OriginalMoveRequestGoalLocation)
			// pick appropriate value base on whether we're going to nav link or not
			: (FNavMeshNodeFlags(PathPt1.Flags).IsNavLink() == false ? PathPointAcceptanceRadius : NavLinkAcceptanceRadius);

		MoveSegmentDirection = (SegmentEnd - SegmentStart).GetSafeNormal();
		bWalkingNavLinkStart = FNavMeshNodeFlags(PathPt0.Flags).IsNavLink();

		// handle moving through custom nav links
		if (PathPt0.CustomLinkId)
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			INavLinkCustomInterface* CustomNavLink = NavSys->GetCustomLink(PathPt0.CustomLinkId);
			StartUsingCustomLink(CustomNavLink, SegmentEnd);
		}

		// update move focus in owning AI
		UpdateMoveFocus();
	}

	//Super::SetMoveSegment(SegmentStartIndex);
}

void UPredictionPFComponent::DebugDrawPath(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	int32 EndSegmentIndex = MoveSegmentStartIndex + 1;
	const FNavigationPath* PathInstance = Path.Get();
	if (PathInstance != nullptr && PathInstance->GetPathPoints().IsValidIndex(MoveSegmentStartIndex) && PathInstance->GetPathPoints().IsValidIndex(EndSegmentIndex))
	{
		constexpr float ZOffset = 1.0f;
		const FVector SegmentStart = *PathInstance->GetPathPointLocation(MoveSegmentStartIndex);
		FVector SegmentEnd = *CurrentDestination;

		//FVector PathDirection = (SegmentEnd - SegmentStart).GetSafeNormal();
		//FVector Desired = PathDirection;
		//FVector Velocity = MovementComp->Velocity;
		//FVector Steering = (Velocity-Desired);

		//DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + Desired * 50.0f, 20.0f, FColor::Cyan, false, 0.0167f, 0, 2.0f);
		DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + MovementComp->Velocity.GetClampedToMaxSize(50.0f), 20.0f, FColor::Blue, false, 0.0167f, 0, 2.0f);
		//DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + Steering.GetSafeNormal() * 50.0f, 20.0f, FColor::Green, false, 0.0167f, 0, 2.0f);

		for (int32 i = 0; i < Path->GetPathPoints().Num(); i++)
		{
			DrawDebugSphere(GetWorld(), Path->GetPathPoints()[i].Location, 4.0f, 8.0f, FColor::Black, false, -1.0f, 0, 1.0f);

			if (i != 0)
			{
				FVector Start = Path->GetPathPoints()[i].Location + FVector(0.0f, 0.0f, ZOffset);
				FVector End = Path->GetPathPoints()[i - 1].Location + FVector(0.0f, 0.0f, ZOffset);
				DrawDebugLine(GetWorld(), Start, End, FColor::Black, false, DeltaTime + 0.001f, 0, 1.25f);
			}
		}
	}

#endif
}

FVector UPredictionPFComponent::GetLastPathPointWithRadius(float PointRadius)
{
	if (!Path.IsValid() || MovementComp == nullptr)
		return FVector::ZeroVector;

	const FVector PreviousPathPointLocation = Path->GetPathPoints()[Path->GetPathPoints().Num() - 2].Location;
	const FVector CurrentTarget = Path->GetEndLocation();
	FVector direction = (CurrentTarget - PreviousPathPointLocation).GetSafeNormal2D();  // 2D normalization.
	FVector adjustedTargetLocation = CurrentTarget - (direction * PointRadius);
	return adjustedTargetLocation;
}

void UPredictionPFComponent::ModifyLastPathPointToIncludeRadius()
{
	if (!bAppliedAcceptanceRadiusFix)
	{
		if (bFixPathToAlwaysHaveBraking)
		{
			/* make sure we have at least two path points or more */
			if (Path->GetPathPoints().Num() > 1)
			{
				/* to apply this fix make sure we have enough space between the last and previous path point otherwise ignore it */
				FVector LastPathPointLocation = Path->GetEndLocation();
				FVector PreviousPathPointLocation = Path->GetPathPoints()[Path->GetPathPoints().Num() - 2].Location;

				/* measure distance between prev and last path point */
				float DistanceBetweenLastAndPrev = (LastPathPointLocation - PreviousPathPointLocation).Size();
				/*
				FString DebugDistanceString = FString::Printf(TEXT("ORIGINAL RADIUS IS: %f"), OriginalAcceptanceRadius);
				GEngine->AddOnScreenDebugMessage(1, 10.0f, FColor::White, DebugDistanceString);
				*/

				/* the distance between the last and prev points needs to twice as large as the radius otherwise we cannot shift the point properly! */
				if (DistanceBetweenLastAndPrev >= (OriginalAcceptanceRadius * 1.3))
				{
					/* calculate the new final end position with radious in mind */
					FVector LastPathPointWithRadius = GetLastPathPointWithRadius(OriginalAcceptanceRadius);

					#if !UE_BUILD_SHIPPING
					if (bShowDebug)
					{
						/* draw the final fixed point */
						DrawDebugSphere(GetWorld(), PreviousPathPointLocation, 8.0f, 8.0f, FColor::Yellow, false, -1, 0, 1.0f);
						DrawDebugLine(GetWorld(), PreviousPathPointLocation, LastPathPointLocation, FColor::Yellow, false, -1, 0, 1.25f);

						DrawDebugSphere(GetWorld(), LastPathPointWithRadius, 8.0f, 8.0f, FColor::Green, false, -1, 0, 1.0f);
						DrawDebugSphere(GetWorld(), LastPathPointLocation, 8.0f, 8.0f, FColor::Red, false, -1, 0, 1.0f);
						DrawDebugLine(GetWorld(), LastPathPointWithRadius, LastPathPointLocation, FColor::Green, false, -1, 0, 1.25f);
						UE_LOG(LogTemp, Warning, TEXT("UPredictionPFComponent::OnPathUpdated: Adjusted path to have braking with given acceptance radius!"));
					}
					#endif

					/* modify the last path point */
					Path->GetPathPoints().Last() = LastPathPointWithRadius;
				}
			}
		}

		bAppliedAcceptanceRadiusFix = true;
	}
}