// Void Interactive, 2020

#include "FlankingCombatMove.h"

#include "FleeingCombatMove.h"
#include "ReadyOrNotAIConfig.h"

#include "Actors/Environment/FlankingAvoidanceVolume.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/BaseCombatActivity.h"

#include "Navigation/ReadyOrNotNavQueries.h"

UFlankingCombatMove::UFlankingCombatMove()
{
	bAllowPartialMove = false;
	bAllowRePathOnInvalidation = false;
	bRequireMagazineWeapon = true;
}

void UFlankingCombatMove::StartActivity(AAIController* Owner)
{
	bGlobalCooldownRandomRange = true;
	GlobalCooldownRange = AI_CONFIG_GET_VECTOR2D("SuspectGlobalFlankCooldown", FVector2D(3.0f, 10.0f));
	
	Super::StartActivity(Owner);

	ResetData();
	
	if (!IsValid(FlankingAvoidanceVolume))
	{
		FlankingAvoidanceVolume = GetWorld()->SpawnActor<AFlankingAvoidanceVolume>(AFlankingAvoidanceVolume::StaticClass(), FTransform());
		FlankingAvoidanceVolume->Bounds->SetBoxExtent(FVector::ZeroVector);
		FlankingAvoidanceVolume->Bounds->SetCanEverAffectNavigation(false);
	}
	else
	{
		FlankingAvoidanceVolume->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		FlankingAvoidanceVolume->Bounds->SetBoxExtent(FVector::ZeroVector);
		FlankingAvoidanceVolume->Bounds->SetCanEverAffectNavigation(false);
	}

	RequiredRegainedLOSTime = (OwningController->GetReactionTime(EActorSenseType::Sight) + 0.1f);

	/*
	FlankingAgainstCharacter = OwningController->GetTrackedTarget();
	if (!FlankingAgainstCharacter)
	{
		ULog::Warning(GetName() + " | " + GetCharacter()->GetName() + " | Flanking Failed. Reason: Flanking against nobody");
		FinishCombatMove(false);
		return;
	}
	*/

	FlankAgainstLocation = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
	
	if (OwningController->GetTrackedTarget())
	{
		FlankAgainstLocation = OwningController->GetTrackedTarget()->GetActorLocation();
		FlankingAgainstCharacter = OwningController->GetTrackedTarget();
	}
	else
	{
		if (OwningController->GetLastTrackedEnemy())
		{
			FlankAgainstLocation = OwningController->GetLastTrackedEnemy()->GetActorLocation();
			FlankingAgainstCharacter = OwningController->GetLastTrackedEnemy();
		}
	}

	if (FlankAgainstLocation == FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "Flanking against nobody";
		ULog::Warning(GetName() + " | " + GetCharacter()->GetName() + " | Flanking Failed. Reason: Flanking against nobody");
		#endif
		
		FinishCombatMove(false);
		return;
	}

	if (!GetCharacter()->GetEquippedWeapon())
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No equipped weapon";
		ULog::Warning(GetName() + " | " + GetCharacter()->GetName() + " | Flanking Failed. Reason: No equipped weapon");
		#endif
		
		FinishCombatMove(false);
		return;
	}

	GetCharacter()->ReasonsToSprint.AddUnique("flanking");
}

void UFlankingCombatMove::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->ReasonsToSprint.Remove("flanking");

	if (FlankingAvoidanceVolume)
	{
		FlankingAvoidanceVolume->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		FlankingAvoidanceVolume->Bounds->SetBoxExtent(FVector::ZeroVector);
		FlankingAvoidanceVolume->Bounds->SetCanEverAffectNavigation(false);
		
		FNavigationSystem::OnComponentTransformChanged(*FlankingAvoidanceVolume->GetRootComponent());
		FNavigationSystem::OnActorBoundsChanged(*FlankingAvoidanceVolume);
		FNavigationSystem::OnActorRegistered(*FlankingAvoidanceVolume);
		
		FNavigationSystem::UpdateActorAndComponentData(*FlankingAvoidanceVolume);
		FNavigationSystem::UpdateActorData(*FlankingAvoidanceVolume);
	}
}

void UFlankingCombatMove::FinishedActivity_NoOwner(const bool bSuccess)
{
	Super::FinishedActivity_NoOwner(bSuccess);

	if (FlankingAvoidanceVolume)
	{
		FlankingAvoidanceVolume->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		FlankingAvoidanceVolume->Bounds->SetBoxExtent(FVector::ZeroVector);
		FlankingAvoidanceVolume->Bounds->SetCanEverAffectNavigation(false);
		
		FNavigationSystem::OnComponentTransformChanged(*FlankingAvoidanceVolume->GetRootComponent());
		FNavigationSystem::OnActorBoundsChanged(*FlankingAvoidanceVolume);
		FNavigationSystem::OnActorRegistered(*FlankingAvoidanceVolume);
		
		FNavigationSystem::UpdateActorAndComponentData(*FlankingAvoidanceVolume);
		FNavigationSystem::UpdateActorData(*FlankingAvoidanceVolume);
	}
}

float UFlankingCombatMove::GetDestinationTolerance() const
{
	return 150.0f;
}

void UFlankingCombatMove::RequestCombatMove(const float DeltaTime)
{
	const FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
	if (LastKnownEnemyPosition == FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No last known enemy position";
		#endif
		
		Location = FVector::ZeroVector;
		FinishCombatMove(false);
		return;
	}

	const bool bCanSeeEnemy = OwningController->GetTargetingComp()->CanSeeTrackedTarget() && OwningController->GetTargetingComp()->HasLineOfSightToTrackedTarget();
	//const bool bHasLOS = HasLOSToEnemy();
	const bool bHasLOS = bCanSeeEnemy;

	if (!bHasLOS)
	{
		RegainedLOSTime = 0.0f;
		bHasBrokenLOS = true;
	}

	const float FlankPathProgress = OwningController->GetRONPathFollowingComp()->GetCurrentPathProgress();
	
	if (!bHasStartedFlanking)
	{
		bHasStartedFlanking = true;
		FlankLocation = LastKnownEnemyPosition;

		UpdateFlankingVolume(FMath::Lerp(FlankLocation, GetCharacter()->GetActorLocation(), 0.5f));
		
		FlankLocation += UKismetMathLibrary::FindLookAtRotation(GetCharacter()->GetActorLocation(), LastKnownEnemyPosition).Vector() * 50.0f;
		
		SetLocation(FlankLocation);
		FlankLocation = Location;
	}
	else
	{
		Location = FVector::ZeroVector;
		DistanceRemainingOnFlankPath = 0.0f;
		float DistanceRemainingOnPath_Enemy = 0.0f;

		if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
		{
			if (Path.IsValid())
			{
				const float Distance = Path->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), OwningController->GetRONPathFollowingComp()->GetNextPathIndex());
				
				DistanceRemainingOnFlankPath = Path->GetLength() - Distance;
				
				const uint32 ClosestPointIndex_Enemy = OwningController->GetRONPathFollowingComp()->GetClosestPathPointFromLocation(LastKnownEnemyPosition);
				const float Distance_Enemy = Path->GetLengthFromPosition(LastKnownEnemyPosition, ClosestPointIndex_Enemy);
				DistanceRemainingOnPath_Enemy = Path->GetLength() - Distance_Enemy;
			}
		}

		/*
		const FSplineCurves* Spline = OwningController->GetRONPathFollowingComp()->GetSplineCurvePath();
		const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), GetCharacter()->GetNavAgentLocation());
		const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
		DistanceRemainingOnFlankPath = Spline->GetSplineLength() - Distance;
	
		const float InputKey_Enemy = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), LastKnownEnemyPosition);
		const float Distance_Enemy = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey_Enemy);
		const float DistanceRemainingOnPath_Enemy = Spline->GetSplineLength() - Distance_Enemy;
		*/

		const float DistanceDifference = FMath::Abs(DistanceRemainingOnPath_Enemy - DistanceRemainingOnFlankPath);
	
		const bool bCanRegainLOS = FlankPathProgress < 0.5f || DistanceRemainingOnFlankPath < 1000.0f || DistanceDifference < 1000.0f;
		//ULog::Bool(bCanRegainLOS, "Can Regain LOS: ");
	
		if (bHasBrokenLOS && bHasLOS && bCanRegainLOS)
		{
			// Don't immediately finish flanking when seeing enemy, wait some time before finishing
			RegainedLOSTime += DeltaTime;
			if (RegainedLOSTime > RequiredRegainedLOSTime)
			{
				bHasCompletedFlank = true;
				
				FlankingAvoidanceVolume->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				FlankingAvoidanceVolume->Bounds->SetBoxExtent(FVector::ZeroVector);
				
				FinishCombatMove();
				return;
			}
		}
		else if (bHasBrokenLOS || !OwningController->IsMoving())
		{
			FHitResult Hit;
			GetWorld()->LineTraceSingleByObjectType(Hit, GetCharacter()->GetActorLocation(), LastKnownEnemyPosition, FCollisionObjectQueryParams(ECC_WorldStatic));
			if (!Hit.bBlockingHit)
			{
				TimeSinceFinishedFlanking += DeltaTime;
				if (TimeSinceFinishedFlanking > 2.0f)
				{
					FinishCombatMove(false);
				}
			}
		}
		
		if (!OwningController->IsMoving())
		{
			AbortMove();
			
			FlankLocation = LastKnownEnemyPosition;

			UpdateFlankingVolume(FMath::Lerp(FlankLocation, GetCharacter()->GetActorLocation(), 0.5f));
			
			FlankLocation += UKismetMathLibrary::FindLookAtRotation(GetCharacter()->GetActorLocation(), LastKnownEnemyPosition).Vector() * 250.0f;
			FlankLocation.Z = GetCharacter()->GetActorLocation().Z - 140.0f;

			SetLocation(FlankLocation);
			FlankLocation = Location;
			
			RequestMoveAsync();
		}
	}
}

bool UFlankingCombatMove::OverrideFocalPoint(FVector& FocalPoint)
{
	if (OwningController->GetTargetingComp()->CanCharacterBeSeen(OwningController->GetTrackedTarget()))
	{
		//ULog::Info("Enemy can be seen");
		FocalPoint = OwningController->GetFocalPointOnActor(OwningController->GetTrackedTarget());
		return true;
	}
	
	if (DistanceRemainingOnFlankPath < 1500.0f)
	{
		FocalPoint = FVector(FlankAgainstLocation.X, FlankAgainstLocation.Y, GetCharacter()->GetMesh()->GetBoneLocation("head").Z);
		return true;
	}

	/*
	if (bHasBrokenLOS)
	{
		FocalPoint = FVector::ZeroVector;
		return false;
	}
	*/
	
	return false;
}

void UFlankingCombatMove::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	if (!OwningController)
		return;
	
	FlankPathExposure = 1.0f;
	
	if (ResultType == ENavigationQueryResult::Success)
	{
		const TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
		if (PathPoints.Num() > 2)
		{
			// Don't allow paths that go towards instigator within their danger zone, too risky to traverse
			{
				const FVector PathPoint1 = PathPoints[1].Location;
				const FVector LastKnownPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition(); 
				
				bool bAnyPathPointGoesTowardsInstigator = false;

				// Is this path point inside of the instigator's danger radius
				const FVector DirectionToPathPoint = (PathPoint1 - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
				const FVector DirectionToInstigator = (LastKnownPosition - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
				
				const float PathPointDotProduct = FVector::DotProduct(DirectionToPathPoint, DirectionToInstigator);
				if (PathPointDotProduct > 0.9f)
				{
					bAnyPathPointGoesTowardsInstigator = true;
				}

				const float Z = GetCharacter()->GetMesh()->GetBoneLocation(BONE_FP_CAMERA).Z;
				FVector TraceStart = PathPoint1;
				TraceStart.Z = Z;
				
				const FVector TraceEnd = LastKnownPosition;
				
				FHitResult Hit;
				bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, GetCharacter()->GetCollisionQueryParameters());

				if (bHasLOS && bAnyPathPointGoesTowardsInstigator)
				{
					#if !UE_BUILD_SHIPPING
					LastFlankPathFailReason = "Flank path goes towards enemy";
					#endif
					
					FlankPathExposure = 1.0f;
					OnFlankPathFail(NavPath);
					return;
				}
			}

			// Is flank path exposed too much to the tracked target?
			{
				AReadyOrNotCharacter* TrackingTarget = OwningController->GetTrackedTarget();
				if (!TrackingTarget)
					TrackingTarget = OwningController->GetLastTrackedEnemy();
				
				if (TrackingTarget)
				{
					struct PathExposureInfo
					{
						FHitResult Hit = FHitResult();
						bool bHasLineTraced = false;
					};
					
					const int32 NumTraces = PathPoints.Num()-2;
					TArray<PathExposureInfo> ExposureResults;
					ExposureResults.Init(PathExposureInfo(), NumTraces);

					int32 NumPathPointsConsidered = 0;
					bool bAnyPathPointBehind = false;
					
					// Ignore first and last path points
					for (int32 i = 1; i < PathPoints.Num()-1; ++i)
					{
						const FNavPathPoint& PathPoint = PathPoints[i];

						// Only care about path points that are behind of us
						FVector DirectionToPathPoint = (PathPoint.Location - TrackingTarget->GetActorLocation()).GetSafeNormal2D();
						//FVector DirectionToTarget = (TrackingTarget->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D();
						const float DotProduct = FVector::DotProduct(DirectionToPathPoint, TrackingTarget->GetActorForwardVector());
						if (DotProduct > -0.35f)
						{
							bAnyPathPointBehind = true;
							NumPathPointsConsidered++;
							
							const float Z = GetCharacter()->GetMesh()->GetBoneLocation(BONE_FP_CAMERA).Z;
							FVector TraceStart = PathPoint.Location;
							TraceStart.Z = Z;
							
							const FVector TraceEnd = TrackingTarget->GetMesh()->GetBoneLocation(BONE_FP_CAMERA);

							GetWorld()->LineTraceSingleByChannel(ExposureResults[i-1].Hit, TraceStart, TraceEnd, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), TrackingTarget));
							ExposureResults[i-1].bHasLineTraced = true;

							#if !UE_BUILD_SHIPPING
							//DrawDebugPoint(GetWorld(), TraceStart, 10.0f, FColor::Yellow, false, 0.2f);
							//DrawDebugLine(GetWorld(), TraceStart, TraceEnd, ExposureResults[i-1].Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 0.2f);
							#endif
						}
					}

					if (!bAnyPathPointBehind)
					{
						#if !UE_BUILD_SHIPPING
						LastFlankPathFailReason = "No unexposed flank path that is behind the target";
						#endif

						OnFlankPathFail(NavPath);
						return;
					}
					
					for (const PathExposureInfo& Result : ExposureResults)
					{
						if (Result.bHasLineTraced && Result.Hit.bBlockingHit && !Cast<AReadyOrNotCharacter>(Result.Hit.GetActor()))
						{
							FlankPathExposure -= 1.0f/(float)NumPathPointsConsidered;
						}
					}

					if (FlankPathExposure > 0.7f)
					{
						#if !UE_BUILD_SHIPPING
						LastFlankPathFailReason = "Flank path is exposed more than 70%";
						#endif

						OnFlankPathFail(NavPath);
						return;
					}
				}
			}
		}
		
		bHasStartedFlanking = true;
		GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::FLANKING);
	}
	else
	{
		FinishCombatMove(false);
		return;
	}

	#if !UE_BUILD_SHIPPING
	LastFlankPathFailReason = "";
	#endif
	
	Super::OnPathFound(PathId, ResultType, NavPath);
}

TSubclassOf<UNavigationQueryFilter> UFlankingCombatMove::GetNavigationQueryOverride()
{
	return UNavQuery_FlankingSuspect::StaticClass();
}

#if !UE_BUILD_SHIPPING
void UFlankingCombatMove::GatherDebugString(FString& OutString)
{
	Super::GatherDebugString(OutString);
	
	OutString += "\n";
	OutString += AddDebugString("Flank Volume Location", FlankingAvoidanceVolume->GetActorLocation().ToCompactString());
	OutString += AddDebugString("Flank Volume Bounds", FlankingAvoidanceVolume->GetActorScale3D().ToCompactString());
	
	OutString += "\n";
	OutString += AddDebugString("Started Flanking", bHasStartedFlanking ? "true" : "false");
	OutString += AddDebugString("Has Broken LOS", bHasBrokenLOS ? "true" : "false");
	OutString += AddDebugString("Flank Path Exposure", FString::Printf(TEXT("%.3f"), FlankPathExposure));
	
	OutString += "\n";
	OutString += AddDebugString("Flanking To", FlankLocation.ToCompactString());
	OutString += AddDebugString("Flank Path Progress", FString::Printf(TEXT("%.3f"), OwningController->GetRONPathFollowingComp()->GetCurrentPathProgress()));

	if (OwningController->GetRONPathFollowingComp()->HasValidPath())
	{
		if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
		{
			if (Path.IsValid())
			{
				const float Distance = Path->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), OwningController->GetRONPathFollowingComp()->GetNextPathIndex());
				
				const uint32 ClosestPointIndex_Enemy = OwningController->GetRONPathFollowingComp()->GetClosestPathPointFromLocation(FlankAgainstLocation);
				const float Distance_Enemy = Path->GetLengthFromPosition(FlankAgainstLocation, ClosestPointIndex_Enemy);
				const float DistanceRemainingOnPath_Enemy = Path->GetLength() - Distance_Enemy;
				
				//const FSplineCurves* Spline = OwningController->GetRONPathFollowingComp()->GetSplineCurvePath();
				//const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), GetCharacter()->GetMovementComponent()->GetActorFeetLocation());
				//const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
				
				OutString += AddDebugString("Flank Path Distance", FString::Printf(TEXT("%.3f"), Path->GetLength()));
				OutString += AddDebugString("Flank Path Distance Remaining", FString::Printf(TEXT("%.3f"), DistanceRemainingOnFlankPath));
				OutString += AddDebugString("Current Distance On Flank Path", FString::Printf(TEXT("%.3f"), Distance));
				
				//const float InputKey_Enemy = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), FlankAgainstLocation);
				//const float Distance_Enemy = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey_Enemy);
				//const float DistanceRemainingOnPath_Enemy = Spline->GetSplineLength() - Distance_Enemy;

				const float DistanceDifference = FMath::Abs(DistanceRemainingOnPath_Enemy - DistanceRemainingOnFlankPath);
				OutString += AddDebugString("Current Distance On Flank Path (Enemy)", FString::Printf(TEXT("%.3f"), Distance_Enemy));
				OutString += AddDebugString("Flank Path Distance Remaining (Enemy)", FString::Printf(TEXT("%.3f"), DistanceRemainingOnPath_Enemy));
				OutString += AddDebugString("Flank Path Distance Difference To Enemy", FString::Printf(TEXT("%.3f"), DistanceDifference));
			}
		}
	}
	
	if (bHasBrokenLOS && HasLOSToEnemy())
		OutString += AddDebugString("Regained LOS Time", FString::Printf(TEXT("%.3f"), RegainedLOSTime));

	OutString += "\n";
	OutString += AddDebugString("Flank Path Fail Count", FString::FromInt(FlankPathFailCount));
	if (!LastFlankPathFailReason.IsEmpty())
		OutString += AddDebugString("Last Flank Path Fail Reason", LastFlankPathFailReason);

}
#endif

void UFlankingCombatMove::UpdateFlankingVolume(const FVector& InFlankLocation)
{
	if (FlankingAvoidanceVolume)
	{
		const FVector LastKnownEnemyPosition = OwningController->GetTargetingComp()->GetLastKnownEnemyPosition();
		
		FRotator VolumeRotation = UKismetMathLibrary::FindLookAtRotation(LastKnownEnemyPosition, GetCharacter()->GetActorLocation());
		VolumeRotation.Pitch = 0.0f;
		VolumeRotation.Roll = 0.0f;
		
		FlankingAvoidanceVolume->SetActorLocationAndRotation(InFlankLocation, VolumeRotation);

		const float X = FMath::Max((LastKnownEnemyPosition - GetCharacter()->GetActorLocation()).Size2D() * 0.49f, 249.0f) - 10.0f;
		const FVector Extent = FVector(X, FlankVolumeWidth, 250.0f);

		FlankingAvoidanceVolume->Bounds->SetBoxExtent(Extent, false);
		//FlankingAvoidanceVolume->SetActorScale3D(Extent/100.0f);
		FlankingAvoidanceVolume->Bounds->SetCanEverAffectNavigation(true);
		
		FNavigationSystem::OnComponentTransformChanged(*FlankingAvoidanceVolume->GetRootComponent());
		FNavigationSystem::OnActorBoundsChanged(*FlankingAvoidanceVolume);
		FNavigationSystem::OnActorRegistered(*FlankingAvoidanceVolume);
		
		FNavigationSystem::UpdateActorAndComponentData(*FlankingAvoidanceVolume);
		FNavigationSystem::UpdateActorData(*FlankingAvoidanceVolume);

		#if WITH_EDITOR
		FlankingAvoidanceVolume->SetActorHiddenInGame(true);
		FlankingAvoidanceVolume->Bounds->SetVisibility(false);
		FlankingAvoidanceVolume->Bounds->SetHiddenInGame(true);
		#endif
	}
}

void UFlankingCombatMove::OnFlankPathFail(FNavPathSharedPtr NavPath)
{
	FlankPathFailCount++;

	#if !UE_BUILD_SHIPPING
	/*
	const TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
	for (int32 i = 1; i < PathPoints.Num(); i++)
	{
		DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 30.0f), FColor::Black, false, 1.0f, 0, 1.25f);
	}
	*/
	#endif

	if (FlankPathFailCount > 15)
	{
		FinishCombatMove(false);
		return;
	}

	FlankVolumeWidth = FMath::Min(FlankVolumeWidth + 100.0f, 2000.0f);
}

void UFlankingCombatMove::ResetData()
{
	Super::ResetData();
	
	FlankLocation = FVector::ZeroVector;
	
	TimeSinceFinishedFlanking = 0.0f;
	FlankPathExposure = 0.0f;
	FlankPathFailCount = 0;
	RegainedLOSTime = 0.0f;
	RequiredRegainedLOSTime = 0.25f;
	FlankVolumeWidth = 500.0f;

	//FlankingAgainstCharacter = nullptr;

	#if !UE_BUILD_SHIPPING
	LastFlankPathFailReason = "";
	#endif
	
	bHasStartedFlanking = false;
	bHasCompletedFlank = false;
	bHasBrokenLOS = false;
}
