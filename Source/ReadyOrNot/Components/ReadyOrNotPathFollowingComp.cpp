// Void Interactive, 2021

#include "ReadyOrNotPathFollowingComp.h"

#include "Actors/Door.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "NavLinkCustomComponent.h"

#include "AIConfig.h"
#include "NavigationSystem.h"
#include "ReadyOrNotNavigationSystem.h"
#include "Actors/WallHoleTraversal.h"
#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/TraverseHoleActivity.h"
#include "Navigation/NavMeshSplinePath.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Path Following ~ Tick"), STAT_PathFollowingTick, STATGROUP_ReadyOrNotPathFollowing);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Path Following ~ Path Related Tick Logic"), STAT_PathRelatedTickLogic, STATGROUP_ReadyOrNotPathFollowing);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Path Following ~ Swat ~ Player Path Block Detection"), STAT_PathBlockDetection, STATGROUP_ReadyOrNotPathFollowing);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Path Following ~ Door Nav Link"), STAT_DoorLink, STATGROUP_ReadyOrNotPathFollowing);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Path Following ~ Debug Draw Path"), STAT_DebugDrawPath, STATGROUP_ReadyOrNotPathFollowing);

TAutoConsoleVariable<int32> CVarDrawAIPath(TEXT("AI.DrawPath"), 1, TEXT("Draw AI pathing and movement direction"));

/*
TArray<FNavPathPoint> FilterPathPoints(const TArray<FNavPathPoint>& InPathPoints, const float DistanceThreshold)
{
	TArray<FNavPathPoint> FilteredPathPoints;
		
	// Filter out small path points
	if (InPathPoints.Num() > 2)
	{
		for (int32 k = 1; k < InPathPoints.Num(); k+=2)
		{
			if (InPathPoints.IsValidIndex(k+1))
			{
				if (FVector::Distance(InPathPoints[k-1].Location, InPathPoints[k].Location) < FMath::Abs(DistanceThreshold) ||
					FVector::Distance(InPathPoints[k].Location, InPathPoints[k+1].Location) < FMath::Abs(DistanceThreshold))
				{
					FilteredPathPoints.AddUnique(InPathPoints[k-1]);
					FilteredPathPoints.AddUnique(InPathPoints[k+1]);
					continue;
				}
					
				FilteredPathPoints.AddUnique(InPathPoints[k-1]);
				FilteredPathPoints.AddUnique(InPathPoints[k]);
				FilteredPathPoints.AddUnique(InPathPoints[k+1]);
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
*/

float UReadyOrNotPathFollowingComp::GetCurrentPathProgress() const
{
	if (Path.IsValid())
	{
		const float PathProgress = (float)GetCurrentPathIndex()/(float)Path->GetPathPoints().Num();
		return PathProgress;
	}

	return 0.0f;
}

uint32 UReadyOrNotPathFollowingComp::GetClosestPathPointFromLocation(const FVector& Location) const
{
	if (Path.IsValid())
	{
		float ClosestDist = FLT_MAX;
		uint32 Index = 0;
		uint32 i = 0;
		for (FNavPathPoint& PathPoint : Path->GetPathPoints())
		{
			float Distance = FVector::Distance(Location, PathPoint.Location);
			if (Distance < ClosestDist)
			{
				ClosestDist = Distance;
				Index = i;
			}
			
			i++;
		}

		return Index;
	}

	return 0;
}

/*
bool UReadyOrNotPathFollowingComp::GetCurrentSplinePathMovementData(FNavMeshSplinePath::FMovementData& OutMovementData, const float DistanceOffset) const
{
	if (Path.IsValid())
	{
		if (bUsingSplinePath)
		{
			const FSplineCurves* Spline = &SplineCurvePath;
			
			if (FMath::IsNearlyZero(DistanceOffset, 0.0001f))
			{
				const float PathProgress = (float)GetCurrentPathIndex()/(float)Path->GetPathPoints().Num();
				const int32 Index = PathProgress * Spline->Position.Points.Num();
				OutMovementData = FNavMeshSplinePath::CalculateMovementDataForPathIndex(Spline, Index, MovementComp->GetMaxSpeed());
				return true;
			}

			const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), MovementComp->GetActorFeetLocation());
			const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
			const FVector Location = FNavMeshSplinePath::GetLocationAtDistanceAlongSpline(Spline, Distance + DistanceOffset);

			const float OffsetInputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), Location);
			OutMovementData = FNavMeshSplinePath::CalculateMovementDataForPathIndex(Spline, (int32)OffsetInputKey, MovementComp->GetMaxSpeed());
			return true;
		}
	}

	return false;
}
*/

void UReadyOrNotPathFollowingComp::BeginPlay() 
{
	Super::BeginPlay();

	// Alex: not using this at all so disable here too
	//SetCrowdSimulationState(ECrowdSimulationState::Disabled);
	//SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
	//SetCrowdSlowdownAtGoal(false);
	//SetCrowdSeparation(true);
}

void UReadyOrNotPathFollowingComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_PathFollowingTick);
	
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return;

	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CyberneticController->GetCharacter());
	if (!CyberneticCharacter)
		return;

	#if WITH_EDITOR
	bUsingSplinePath = false;
	
	if (CVarUseRawPath.GetValueOnAnyThread() == 0)
		bUsingSplinePath = true;
	#else
	bUsingSplinePath = false;
	#endif

	if (CyberneticCharacter->IsOnSWATTeam())
		bUsingSplinePath = false;

	/*
	for (auto& k : TimeSinceLastOpenedDoor)
	{
		k.Value += DeltaTime;
	}

	for (auto& k : TimeSinceLastClosedDoor)
	{
		k.Value += DeltaTime;
	}
	*/

	TimeSinceLastValidNavLinkDestination += DeltaTime;

	UNavLinkCustomComponent* CustomNavLinkOb = (UNavLinkCustomComponent*)CurrentCustomLinkOb.Get();
	bWalkingNavLinkStart = CustomNavLinkOb != nullptr;
	
	HoleTraversalCooldown = FMath::Max(HoleTraversalCooldown - DeltaTime, 0.0f);
	if (HoleTraversalCooldown <= 0.0f)
	{
		LastUsedWallHole = nullptr;
	}
	
	if (Status != EPathFollowingStatus::Paused)
	{
		if (UBaseActivity* Activity = CyberneticController->GetCurrentActivity())
		{
			if (Activity->IsActivityPaused())
				Activity->OnPathMoveResumed(CyberneticController);
		}
	}

	if (Path.IsValid())
	{
		SCOPE_CYCLE_COUNTER(STAT_PathRelatedTickLogic);

		const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
		int32 CurrentPathIndex = GetCurrentPathIndex();
		int32 NextPathIndex = GetNextPathIndex();
		
		if (PathPoints.IsValidIndex(CurrentPathIndex) && PathPoints.IsValidIndex(NextPathIndex))
		{
			// one door per frame
			const TArray<ADoor*>& AllDoors = GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors;
			for (ADoor* Door : AllDoors)
			{
				TSubclassOf<UNavigationQueryFilter> DefaultFilter = UNavigationQueryFilter::StaticClass();
				bool bBypassLock = (Door->bSuspectAlwaysUnlocks && CyberneticCharacter->IsSuspect()) || (Path->GetQueryData().QueryFilter == UNavigationQueryFilter::GetQueryFilter(*Path->GetNavigationDataUsed(), DefaultFilter));
				bool bCanPathThrough = !((Door->IsLocked() && !bBypassLock) || Door->IsJammed() || Door->GetAttachedTrap());
				bool bIsOpen = Door->IsOpenBeyondCloseThreshold() || Door->IsDoorBroken() || Door->IsDoorwayOnly();
				if (!bIsOpen && bCanPathThrough && !Door->IsAnyAIOpening())
				{
					if (FVector::Distance(Door->CalculateClosestPoint(CyberneticCharacter->GetActorLocation()), CyberneticCharacter->GetActorLocation()) < 150.0f)
					{
						FVector DoorLocation = Door->GetDoorMidLocation();
						DoorLocation.Z = Door->GetActorLocation().Z;
					
						FVector Start = PathPoints[CurrentPathIndex].Location;
						Start.Z = CyberneticCharacter->GetNavAgentLocation().Z;
					
						FVector End = PathPoints[NextPathIndex].Location;
						End.Z = Start.Z;
					
						if (Door->IsPointsOnOppositeSideOfDoor(CyberneticCharacter->GetNavAgentLocation(), End))
						{
							if (FMath::LineSphereIntersection<double>(Start, MoveSegmentDirection, FVector::Distance(Start, End), DoorLocation, 32.0f))
							{
								CyberneticCharacter->ToggleDoor(Door, true, bBypassLock);
								break;
							}
						}
					}
				}
			}
		}
		
		if (LastUsedDoorLink && CyberneticCharacter->IsSuspect())
		{
			SCOPE_CYCLE_COUNTER(STAT_DoorLink);
			
			if (CyberneticCharacter->IsStunned() ||
				CyberneticController->GetAwarenessState() == EAIAwarenessState::Alerted)
			{
				LastUsedDoorLink = nullptr;
				LastUsedDoorLinkComp = nullptr;
			}
			else
			{
				if (LastUsedDoorLink->IsClosed())
				{
					CyberneticCharacter->ToggleDoor(LastUsedDoorLink, true);
				}
				
				float Dist = (CyberneticCharacter->GetActorLocation() - LastUsedDoorLink->GetDoorway()->GetComponentLocation()).Size();
				//LOG_NUMBER(Dist);
				if (Dist > 200.0f)
				{
					FVector v1 = CyberneticController->GetControlRotation().Vector();
					FVector DirectionToDoor = (LastUsedDoorLink->GetActorLocation() - CyberneticCharacter->GetActorLocation()).GetSafeNormal2D();

					if (FVector::DotProduct(v1, DirectionToDoor) < 0.0f)
					{
						OnDoorLinkFinished(LastUsedDoorLinkComp, LastUsedDoorLink);
					}
				}
				
				LastUsedDoorLink = nullptr;
				LastUsedDoorLinkComp = nullptr;
			}
		}
		
		// General non navlink movement blocking check for player (to stop gunking)
		// not really needed for non swat team
		if (CyberneticCharacter->IsOnSWATTeam())
		{
			SCOPE_CYCLE_COUNTER(STAT_PathBlockDetection);
			
			FHitResult Hit;
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActor(CyberneticCharacter);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters);

			GetWorld()->LineTraceSingleByObjectType(Hit, CyberneticCharacter->GetActorLocation(), CyberneticCharacter->GetActorLocation() + MoveSegmentDirection.GetSafeNormal() * 50.0f, FCollisionObjectQueryParams(ECC_Pawn), CollisionQueryParams);

			APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Hit.GetActor());
			if (PlayerCharacter && Hit.Distance < 50.0f && PlayerCharacter->GetVelocity().Size2D() < 150.0f)
			{
				TimeBlocked += DeltaTime;

				if (TimeBlocked > 0.5f)
				{
					CyberneticCharacter->PlayRawVOWithCooldown(VO_SWAT_GENERAL::CALL_BLOCKED_BY_PLAYER, 5.0f);
				}
				
				PauseMove(GetCurrentRequestId(), EPathFollowingVelocityMode::Reset);
			}
			else
			{
				if (GetStatus() == EPathFollowingStatus::Paused)
				{
					ResumeMove(GetCurrentRequestId());
				}
				
				TimeBlocked = 0.0f;
			}
		}
		
		//DrawDebugPoint(GetWorld(), MovementComp->GetActorFeetLocation(), 5.0f, FColor::White, true);

		//FHitResult Hit;
		//bLOSToDestination = !GetWorld()->LineTraceSingleByChannel(Hit, CyberneticCharacter->GetMesh()->GetBoneLocation("head_end"), Path->GetDestinationLocation(), ECC_Visibility, CyberneticCharacter->GetCollisionQueryParameters());

		/*
		if (bUsingSplinePath)
		{
			if (GetCurrentSplinePathMovementData(SplinePathMovementData, 200.0f))
			{
				if (SplinePathMovementData.MaxVelocity > 0.0f && !FMath::IsNaN(SplinePathMovementData.MaxVelocity))
				{
					//Acceleration = FMath::FInterpTo(Acceleration, SplinePathMovementData.MaxVelocity, DeltaTime, 1.0f);
					Acceleration = 1000.0f;

					#if WITH_EDITOR
					const FSplineCurves* Spline = &SplineCurvePath;
					const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), MovementComp->GetActorFeetLocation());
					const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
					const FVector Location = FNavMeshSplinePath::GetLocationAtDistanceAlongSpline(Spline, Distance + 200.0f);
					DrawDebugSphere(GetWorld(), Location + FVector(0.0f, 0.0f, 30.0f), 10.0f, 12, FColor::Purple, false, GetWorld()->GetDeltaSeconds() - 0.005f, 0, 0.5f);
					#endif
				}

					float TargetAcceleration = 0.0f;

					{
						const FSplineCurves* Spline = &SplineCurvePath;
						const float PathProgress = (float)GetCurrentPathIndex()/(float)Path->GetPathPoints().Num();
						const int32 Index = FMath::Clamp((int32)(PathProgress * (Spline->Position.Points.Num()-1)), 0, Spline->Position.Points.Num()-1);
						const int32 Index2 = FMath::Min(Index+1, Spline->Position.Points.Num()-1);
					
						if (Spline->Position.Points.IsValidIndex(Index) && Spline->Position.Points.IsValidIndex(Index2))
						{
							float DistanceToNextPathPoint = FVector::Distance(Spline->Position.Points[Index].OutVal, Spline->Position.Points[Index2].OutVal);
							TargetAcceleration = FMath::GetMappedRangeValueClamped({200.0f, 500.0f}, {200.0f, 600.0f}, DistanceToNextPathPoint);
						}
						else
						{
							// Fall back on a default speed
							TargetAcceleration = 200.0f;
						}
					}

					Acceleration = FMath::FInterpTo(Acceleration, TargetAcceleration, DeltaTime, 1.0f);
					//Acceleration = 1000.0f;
			}
		}
		*/

		#if !UE_BUILD_SHIPPING
		if (CVarDrawAIPath.GetValueOnAnyThread() > 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_DebugDrawPath);
			
			int32 EndSegmentIndex = MoveSegmentStartIndex + 1;
			const FNavigationPath* PathInstance = Path.Get();
			if (PathInstance != nullptr && PathInstance->GetPathPoints().IsValidIndex(MoveSegmentStartIndex) && PathInstance->GetPathPoints().IsValidIndex(EndSegmentIndex))
			{
				constexpr float ZOffset = 30.0f;
				const FVector SegmentStart = *PathInstance->GetPathPointLocation(MoveSegmentStartIndex);
				FVector SegmentEnd = *CurrentDestination;
				//FVector PathDirection = (SegmentEnd - SegmentStart).GetSafeNormal();
				//FVector Desired = PathDirection;
				//FVector Velocity = MovementComp->Velocity;
				//FVector Steering = (Velocity-Desired);

				//DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + Desired * 50.0f, 20.0f, FColor::Cyan, false, 0.0167f, 0, 2.0f);
				DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + MovementComp->Velocity.GetClampedToMaxSize(50.0f), 20.0f, FColor::Purple, false, 0.0167f, 0, 2.0f);
				//DrawDebugDirectionalArrow(GetWorld(), MovementComp->GetActorFeetLocation(), MovementComp->GetActorFeetLocation() + Steering.GetSafeNormal() * 50.0f, 20.0f, FColor::Green, false, 0.0167f, 0, 2.0f);
				
				FColor DebugColor = FColor::Red;
				if (bUsingSplinePath)
					DebugColor = FColor::Cyan;
				
				for (uint32 i = Path->GetPathPoints().Num()-1; i > GetCurrentPathIndex()+1; i--)
				{
					FVector Start = Path->GetPathPoints()[i].Location + FVector(0.0f, 0.0f, ZOffset);
					FVector End = Path->GetPathPoints()[i-1].Location + FVector(0.0f, 0.0f, ZOffset);
					
					DrawDebugLine(GetWorld(), Start, End, DebugColor, false, DeltaTime + 0.001f, 0, 1.25f);
				}
				
				const FVector P = MovementComp->GetActorFeetLocation();
				const FVector A = SegmentStart;
				const FVector B = SegmentEnd;
				const FVector AB = B - A;
				const FVector AP = P - A;
				
				const float PathSegmentProgress = FVector::DotProduct(AP, AB)/FVector::DotProduct(AB, AB);
				const FVector ClosestPoint = A + (FMath::Clamp(PathSegmentProgress, 0.0f, 1.0f) * AB);
				
				DrawDebugLine(GetWorld(), ClosestPoint + FVector(0.0f, 0.0f, ZOffset), SegmentEnd + FVector(0.0f, 0.0f, ZOffset), DebugColor, false, DeltaTime + 0.001f, 0, 1.25f);

				//DrawDebugString(GetWorld(), SegmentStart, "Is NavLink: " + FString(IsCurrentSegmentNavigationLink() ? "true" : "false"), nullptr, FColor::White, DeltaTime + 0.001f, true);
			}
		}
		#endif
	}
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UReadyOrNotPathFollowingComp::SetMoveSegment(int32 SegmentStartIndex)
{
	Super::SetMoveSegment(SegmentStartIndex);

	// Is future path point a nav link?
	// Fixes AI stuck trying to navigate through navlinks if current path point is not a navlink
	/*
	if (Path.IsValid())
	{
		const FNavPathPoint& PathPt0 = Path->GetPathPoints()[SegmentStartIndex];
		if (PathPt0.CustomLinkId)
			return;
		
		const FSplineCurves* Spline = &SplineCurvePath;
		const float PathProgress = (float)GetCurrentPathIndex()/(float)Path->GetPathPoints().Num();
		const int32 Index = PathProgress * (Spline->Position.Points.Num()-1);
		const int32 Index2 = FMath::Min(Index+1, Spline->Position.Points.Num()-1);
		const float SplinePathProgress = (float)Index2 / (float)(Spline->Position.Points.Num()-1);
		const int32 NextIndex = (SplinePathProgress * Path->GetPathPoints().Num())+1;

		if (Path->GetPathPoints().IsValidIndex(NextIndex))
		{
			const FNavPathPoint& NextPathPoint = Path->GetPathPoints()[NextIndex];

			// handle moving through custom nav links
			if (NextPathPoint.CustomLinkId)
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
				INavLinkCustomInterface* CustomNavLink = NavSys->GetCustomLink(NextPathPoint.CustomLinkId);
				StartUsingCustomLink(CustomNavLink, NextPathPoint.Location);
			}
		}
	}
	*/
}

bool UReadyOrNotPathFollowingComp::HasReachedCurrentTarget(const FVector& CurrentLocation) const
{
	return Super::HasReachedCurrentTarget(CurrentLocation);
}

void UReadyOrNotPathFollowingComp::OnHoleTraversalFinished(UBaseActivity* Activity, ACyberneticController* Controller)
{
	if (Controller->GetCombatActivity())
	{
		LastUsedWallHole = Controller->GetCombatActivity()->TraverseHoleActivity->WallHoleTraversalActor;
	}
	
	HoleTraversalCooldown = 2.0f;
}

void UReadyOrNotPathFollowingComp::OnPathUpdated()
{
	Super::OnPathUpdated();

	/*
	if (Path.IsValid())
	{
		RawPath = Path->GetPathPoints();

		if (RawPath.Num() == 0)
			return;

		if (bUsingSplinePath)
		{
			constexpr uint8 NumSubSteps = 20;
			constexpr float PathPointDistanceThreshold = 50.0f;
			
			RawPath = FilterPathPoints(Path->GetPathPoints(), PathPointDistanceThreshold);
			
			FSplineCurves SplineCurves;
			constexpr EInterpCurveMode CurveMode = CIM_CurveAutoClamped;
			for (int32 i = 0; i < RawPath.Num(); i++)
			{
				SplineCurves.Position.AddPoint((float)i, RawPath[i].Location);
				SplineCurves.Position.Points[i].InterpMode = CurveMode;
				SplineCurves.Rotation.AddPoint((float)i, FQuat::Identity);
				SplineCurves.Rotation.Points[i].InterpMode = CurveMode;
				SplineCurves.Scale.AddPoint((float)i, FVector::OneVector);
				SplineCurves.Scale.Points[i].InterpMode = CurveMode;
			}
			
			SplineCurves.UpdateSpline(false, false);

			SplineCurvePath = SplineCurves;
			
			Path->GetPathPoints().Empty(RawPath.Num()*NumSubSteps);
			
			for (int32 i = 0; i < RawPath.Num(); i++)
			{
				for (uint8 StepIdx = 1; StepIdx <= NumSubSteps; StepIdx++)
				{
					const float Alpha = (float)StepIdx / (float)NumSubSteps;
					const float Key = (i-1) + Alpha;
					const FVector NewPos = SplineCurvePath.Position.Eval(Key);

					FNavPathPoint NewPathPoint = RawPath[i];
					NewPathPoint.Location = NewPos;
					
					Path->GetPathPoints().AddUnique(NewPathPoint);
				}
			}
		}
	}
	*/
}

void UReadyOrNotPathFollowingComp::OnPathFinished(const FPathFollowingResult& Result)
{
	Super::OnPathFinished(Result);

	//RawPath.Empty();
	//SplineCurvePath = {};
}

void UReadyOrNotPathFollowingComp::StartUsingCustomLink(INavLinkCustomInterface* CustomNavLink, const FVector& DestPoint)
{
	Super::StartUsingCustomLink(CustomNavLink, DestPoint);
	
	NavLinkDestination = DestPoint;
	TimeSinceLastValidNavLinkDestination = 0.0f;

	DrawDebugPoint(GetWorld(), NavLinkDestination, 15.0f, FColor::Red, false, 0.1f, 0);

	if (CustomNavLink)
	{
		if (const UNavLinkCustomComponent* NavLinkCustomComponent = Cast<UNavLinkCustomComponent>(CustomNavLink))
		{
			if (NavLinkCustomComponent->GetOwner())
			{
				if (AWallHoleTraversal* WallHoleTraversal = Cast<AWallHoleTraversal>(NavLinkCustomComponent->GetOwner()->GetOwner()))
				{
					OnWallHoleLinkStart(nullptr, WallHoleTraversal);
					return;
				}

				/*
				if (ADoor* Door = Cast<ADoor>(NavLinkCustomComponent->GetOwner()->GetOwner()))
				{
					OnDoorLinkStart(nullptr, Door);
					return;
				}
				*/
			}
		}
	}
}

void UReadyOrNotPathFollowingComp::FinishUsingCustomLink(INavLinkCustomInterface* CustomNavLink)
{
	Super::FinishUsingCustomLink(CustomNavLink);
	
	if (LastUsedDoorLink)
	{
		OnDoorLinkFinished(LastUsedDoorLinkComp, LastUsedDoorLink);
	}
	
	NavLinkDestination = FVector::ZeroVector;
}

FVector UReadyOrNotPathFollowingComp::GetMoveFocus(bool bAllowStrafe) const
{
	const FVector CurrentMoveDirection = GetCurrentDirection();
	const FVector MoveFocus = *CurrentDestination + (CurrentMoveDirection * FAIConfig::Navigation::FocalPointDistance);

	return MoveFocus;
}

/*
void UReadyOrNotPathFollowingComp::OnDoorLinkStart(UNavLinkCustomComponent* Link, ADoor* Door)
{
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return;
	
	if (!CyberneticController->bCanOpenDoorThroughNavLink)
	{
		AbortMove(*this, EPathFollowingResult::Aborted);
		return;
	}

	if (!CyberneticController->IsSWAT() && Door->OperatingStates.Num() > 0)
	{
		AbortMove(*this, EPathFollowingResult::Aborted);
		return;
	}

	LastUsedDoorLink = Door;
	LastUsedDoorLinkComp = Link;
	
	if (ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CyberneticController->GetPawn()))
	{
		if (Door->IsOpenBeyondCloseThreshold() && CyberneticCharacter->IsOnSWATTeam())
		{
			return;
		}
		
		if (Door->IsDoorBroken())
		{
			return;
		}

		if (Door->IsLocked() || Door->IsJammed() || Door->GetAttachedTrap())
		{
			LastUsedDoorLink = nullptr;
			AbortMove(*this, EPathFollowingResult::Aborted);
			return;
		}

		if (const float* TimePtr = TimeSinceLastOpenedDoor.Find(Door))
		{
			if (*TimePtr < 1.0f)
			{
				LastUsedDoorLink = nullptr;
				AbortMove(*this, EPathFollowingResult::Aborted);
				return;
			}
		}

		//TimeSinceLastOpenedDoor.Add(Door, 0.0f);
	}
}
*/

void UReadyOrNotPathFollowingComp::OnDoorLinkFinished(UNavLinkCustomComponent* Link, ADoor* Door)
{
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return;

	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CyberneticController->GetCharacter());
	if (!CyberneticCharacter)
		return;

	if (Door->IsAnyAIClosing() || Door->IsDoorBroken() || !Door->IsOpen())
	{
		LastUsedDoorLink = nullptr;
		LastUsedDoorLinkComp = nullptr;
		return;
	}

	/*
	if (TimeSinceLastClosedDoor.Find(Door))
	{
		if (TimeSinceLastClosedDoor[Door] < 10.0f)
		{
			LastUsedDoorLink = nullptr;
			LastUsedDoorLinkComp = nullptr;
			return;
		}
	}
	*/
	 
	if (!CyberneticController->IsSWAT())
	{
		//TimeSinceLastClosedDoor.Add(Door, 0.0f);
		CyberneticCharacter->ToggleDoor(Door, false);
		LastUsedDoorLink = nullptr;
		LastUsedDoorLinkComp = nullptr;
	}
}

void UReadyOrNotPathFollowingComp::OnWallHoleLinkStart(UNavLinkCustomComponent* Link, AWallHoleTraversal* WallHole)
{
	const ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return;

	if (CyberneticController->IsSWAT())
		return;

	if (LastUsedWallHole == WallHole)
		return;
		
	if (!CyberneticController->GetActivity<UTraverseHoleActivity>())
	{
		if (CyberneticController->GetCombatActivity())
		{
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->ResetData();
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->WallHoleTraversalActor = WallHole;
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->bIgnoreCooldown = true;
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->bFromNavLink = true;
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->OnFinishActivity.RemoveAll(this);
			CyberneticController->GetCombatActivity()->TraverseHoleActivity->OnFinishActivity.AddDynamic(this, &UReadyOrNotPathFollowingComp::OnHoleTraversalFinished);
			
			UActivityManager::GiveActivityTo(CyberneticController->GetCombatActivity()->TraverseHoleActivity, CyberneticController->GetCharacter(), true);
		}
	}
}

void UReadyOrNotPathFollowingComp::PauseMove(FAIRequestID RequestID, EPathFollowingVelocityMode VelocityMode)
{
	Super::PauseMove(RequestID, VelocityMode);

	if (Status == EPathFollowingStatus::Paused)
	{
		if (ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner()))
		{
			if (UBaseActivity* Activity = CyberneticController->GetCurrentActivity())
			{
				if (!Activity->IsActivityComplete() && CyberneticController->LastRequestID == RequestID)
					Activity->OnPathMovePaused(CyberneticController, RequestID);
			}
		}
	}
}

void UReadyOrNotPathFollowingComp::ResumeMove(FAIRequestID RequestID)
{
	Super::ResumeMove(RequestID);
	
	if (ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner()))
	{
		if (UBaseActivity* Activity = CyberneticController->GetCurrentActivity())
		{
			if (!Activity->IsActivityComplete())
				Activity->OnPathMoveResumed(CyberneticController);
		}
	}
}

/*
FVector UReadyOrNotPathFollowingComp::Seek(const FVector& Target)
{
	FVector Desired = Target - (MovementComp->GetActorFeetLocation() * FVector(1.0f, 1.0f, 0.0f));
	Desired *= MovementComp->GetMaxSpeed() / Desired.Size();

	const FVector Force = Desired - MovementComp->GetOwner()->GetActorForwardVector();
	return Force * (0.01f/MovementComp->GetMaxSpeed());
}
*/

bool UReadyOrNotPathFollowingComp::IsUsingCustomLink() const
{
	return CurrentCustomLinkOb.Get() != nullptr; 
}

bool UReadyOrNotPathFollowingComp::IsUsingThisDoorLink(ADoor* Door) const
{
	return LastUsedDoorLink == Door && CurrentCustomLinkOb.Get(); 
}

FVector UReadyOrNotPathFollowingComp::GetNavLinkDestination() const
{
	if (CurrentCustomLinkOb.Get() != nullptr)
		return NavLinkDestination;

	return FVector::ZeroVector;
}

bool UReadyOrNotPathFollowingComp::DoesCurrentPathGoThroughDoor(ADoor* Door) const
{
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return false;

	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CyberneticController->GetCharacter());
	if (!CyberneticCharacter)
		return false;

	if (!Path.IsValid())
	{
		return false;
	}
	
	int32 CurrentPathIndex = GetCurrentPathIndex();
	int32 NextPathIndex = GetNextPathIndex();

	const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();

	if (PathPoints.IsValidIndex(CurrentPathIndex) && PathPoints.IsValidIndex(NextPathIndex))
	{
		if (FVector::Distance(Door->CalculateClosestPoint(CyberneticCharacter->GetActorLocation()), CyberneticCharacter->GetActorLocation()) < 150.0f)
		{
			FVector DoorLocation = Door->GetDoorMidLocation();
			DoorLocation.Z = Door->GetActorLocation().Z;
		
			FVector Start = PathPoints[CurrentPathIndex].Location;
			Start.Z = CyberneticCharacter->GetNavAgentLocation().Z;
		
			FVector End = PathPoints[NextPathIndex].Location;
			End.Z = Start.Z;
		
			if (Door->IsPointsOnOppositeSideOfDoor(CyberneticCharacter->GetNavAgentLocation(), End))
			{
				// within the sphere?
				if (FVector::Distance(Start, DoorLocation) <= 32.0f ||
					FVector::Distance(End, DoorLocation) <= 32.0f)
				{
					return true;
				}
				
				if (FMath::LineSphereIntersection<double>(Start, MoveSegmentDirection, FVector::Distance(Start, End), DoorLocation, 32.0f))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UReadyOrNotPathFollowingComp::DoesPathGoThroughDoor(FNavPathSharedPtr NavPath, ADoor* Door) const
{
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetOwner());
	if (!CyberneticController)
		return false;

	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(CyberneticController->GetCharacter());
	if (!CyberneticCharacter)
		return false;

	if (!NavPath.IsValid())
	{
		return false;
	}
	
	const TArray<FNavPathPoint>& PathPoints = NavPath->GetPathPoints();

	for (uint16 i = 0; i < PathPoints.Num(); i++)
	{
		if (PathPoints.IsValidIndex(i) && PathPoints.IsValidIndex(i+1))
		{
			if (FVector::Distance(Door->CalculateClosestPoint(CyberneticCharacter->GetActorLocation()), CyberneticCharacter->GetActorLocation()) < 150.0f)
			{
				FVector DoorLocation = Door->GetDoorMidLocation();
				DoorLocation.Z = Door->GetActorLocation().Z;
			
				FVector Start = PathPoints[i].Location;
				Start.Z = CyberneticCharacter->GetNavAgentLocation().Z;
			
				FVector End = PathPoints[i+1].Location;
				End.Z = Start.Z;
			
				if (Door->IsPointsOnOppositeSideOfDoor(CyberneticCharacter->GetNavAgentLocation(), End))
				{
					// within the sphere?
					if (FVector::Distance(Start, DoorLocation) <= 32.0f ||
						FVector::Distance(End, DoorLocation) <= 32.0f)
					{
						return true;
					}

					// line goes through the sphere?
					if (FMath::LineSphereIntersection<double>(Start, (End-Start).GetSafeNormal(), FVector::Distance(Start, End), DoorLocation, 32.0f))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

