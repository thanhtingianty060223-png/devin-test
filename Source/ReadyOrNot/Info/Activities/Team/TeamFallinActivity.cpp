// Void Interactive, 2020

#include "TeamFallinActivity.h"

#include "ArrestTargetActivity.h"
#include "CollectEvidenceActivity.h"
#include "PhysicsCoreTypes.h"
#include "Actors/ThreatAwarenessActor.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"

#include "Info/SWATManager.h"

#include "Info/Activities/ActivityManagerTemplates.h"

#include "Navigation/ReadyOrNotNavQueries.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Fall In Activity ~ Perform Activity"), STAT_FallInPerformActivity, STATGROUP_FallInActivity);

TAutoConsoleVariable<int32> CVarSwatFallinPattern(TEXT("Swat.FallInPattern"), 0, TEXT("0 = Snake | 1 = Half Snake | 2 = Diamond | 3 = Flock"));

// todo: not moving, allow focus for alpha, then clear it when moving

UTeamFallinActivity::UTeamFallinActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "FallIn");
	bIsProgressActivity = true;
	bFinishActivityWhenOverriden = false;
	bAbortIfNotMovingForAWhile = false;
	bAllowPartialMove = true;
	bAlwaysRequestMove = true;
}

void UTeamFallinActivity::PerformActivity(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_FallInPerformActivity);

	if (!USWATManager::Get(this)->GetSquadLeader())
	{
		ACTIVITY_FAILED("No squad leader available");
		return;
	}
	
	// Alex: disable for consistency to predict stopping distance
	// MoveAcceptanceRadius = GetDestinationTolerance();

	if (!IsAnyoneSwapping() && !bIsSwapping)
	{
		Location = CalculateFallInPosition(GetSharedData<FSharedFallInData>()->Pattern);
		bFirstCalculation = false;
		if (Location == FVector::ZeroVector && LastRequestedLocation != FVector::ZeroVector)
		{
			LastRequestedLocation = FVector::ZeroVector;
			AbortMove();
		}
	}
	
	switch (GetSharedData<FSharedFallInData>()->Pattern)
	{
		case EFallInPattern::Snake:			ProgressState = FText::FromStringTable("SwatCommandTable", "SingleFile"); break;
		case EFallInPattern::HalfSnake:		ProgressState = FText::FromStringTable("SwatCommandTable", "DoubleFile"); break;
		case EFallInPattern::Diamond:		ProgressState = FText::FromStringTable("SwatCommandTable", "Diamond"); break;
		case EFallInPattern::Flock:			ProgressState = FText::FromStringTable("SwatCommandTable", "Wedge"); break;
		default: break;
	}

	if (USWATManager::Get(this)->bForceSnakeFallIn)
	{
		ProgressState = FText::FromStringTable("SwatCommandTable", "ForcingSingleFile");
	}

	const FVector DirectionToSquadLeader = (GetSquadLeader()->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();

	// dont set ignored direction if we're on stairs

	bool bOnStairs = false;
	const AThreatAwarenessActor* TAA = Cast<AThreatAwarenessActor>(OwningController->GetFocusActor());
	if (!TAA)
	{
		TAA = OwningController->GetTargetingComp()->GetNearestExtremeThreat();
		if (TAA)
		{
			if (FVector::Distance(TAA->GetActorLocation(), GetCharacter()->GetActorLocation()+FVector(0.0f, 0.0f, 70.0f)) > 500.0f)
			{
				TAA = nullptr;
			}
		}
	}
	
	if (TAA && TAA->GetThreatLevel() == EThreatLevel::TL_Stairs)
		bOnStairs = true;

	if (bOnStairs)
	{
		OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
	}
	else
	{
		if (SharedData->NumInTeam > 2)
		{
			if (USWATManager::Get(this)->bForceSnakeFallIn ||
				GetSharedData<FSharedFallInData>()->Pattern == EFallInPattern::Snake ||
				GetSharedData<FSharedFallInData>()->Pattern == EFallInPattern::Diamond)
			{
				if (OverrideSquadPosition == ESquadPosition::SP_Alpha || OverrideSquadPosition == ESquadPosition::SP_Beta)
				{
					OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(-DirectionToSquadLeader);
				}
				else if (OverrideSquadPosition == ESquadPosition::SP_Charlie)
				{
					// charlie can look wherever
					OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
				}
				else
				{
					OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(DirectionToSquadLeader);
				}
			}
			else if (GetSharedData<FSharedFallInData>()->Pattern == EFallInPattern::HalfSnake)
			{
				if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
				{
					OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(-DirectionToSquadLeader);
				}
				else
				{
					OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(DirectionToSquadLeader);
				}
			}
			else
			{
				OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
			}
		}
		else
		{
			if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			{
				OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(-DirectionToSquadLeader);
			}
			else
			{
				// beta can look wherever
				OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
			}
		}
	}
	
	Super::PerformActivity(DeltaTime);
}

#if !UE_BUILD_SHIPPING
void UTeamFallinActivity::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		return;
	
	/*
	FString FileDebugInfo = "File: ";
	
	TArray<ASWATCharacter*> Swat_FileA = USWATManager::Get(this)->FallInSwat_FileA;
	TArray<ASWATCharacter*> Swat_FileB = USWATManager::Get(this)->FallInSwat_FileB;

	Swat_FileA.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	Swat_FileB.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});

	if (Swat_FileA.Num() == 0 || Swat_FileB.Num() == 0)
	{
		FileDebugInfo += "None";
	}
	else
	{
		const int32 FileAIndex = Swat_FileA.Find(Cast<ASWATCharacter>(GetCharacter()));
		
		bool bIsInFileA = FileAIndex != INDEX_NONE ? true : false;

		FileDebugInfo += (bIsInFileA ? "A" : "B");
	}
	
	DrawDebugString(GetWorld(), GetCharacter()->GetActorLocation(), FileDebugInfo, nullptr, FColor::White, DeltaTime, true);
	*/

	FVector SwatAverageLocation = USWATManager::Get(this)->GetAverageSwatLocation();

	if (MyLeader)
	{
		FVector Direction = (MyLeader->GetActorLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal();
		DrawDebugDirectionalArrow(GetWorld(), GetCharacter()->GetActorLocation(), GetCharacter()->GetActorLocation() + Direction * 100.0f, 5.0f, FColor::Yellow, false, DeltaTime);
		//DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation(), FColor::Yellow, false, DeltaTime);
	}

	//if (OverrideSquadPosition == ESquadPosition::SP_Alpha && GetSquadLeader())
		//DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), GetSquadLeader()->GetActorLocation(), FColor::Yellow, false, DeltaTime);
	
	const AReadyOrNotCharacter* const SquadLeader = USWATManager::Get(this)->GetSquadLeader();
	
	FVector AverageDirection = FVector::ZeroVector;
	for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
	{
		AverageDirection += (SwatAverageLocation - swat->GetActorLocation()).GetSafeNormal2D();
	}
	
	AverageDirection.Normalize();
	FVector PerpDirection = FVector::CrossProduct(AverageDirection, FVector::UpVector);

	TArray<ASWATCharacter*> FrontSwat, BackSwat;
	for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
	{
		if (FVector::DotProduct((swat->GetActorLocation() - SwatAverageLocation).GetSafeNormal2D(), PerpDirection) < 0.0f)
		{
			BackSwat.Add(swat);
		}
		else
		{
			FrontSwat.Add(swat);
		}
	}

	/*
	FString Side = BackSwat.Contains(Cast<ASWATCharacter>(GetCharacter())) ? "Back" : "Front";
	FString DebugMessage = FString::Printf(TEXT("%i\nSpeed: %.2f\nPath Dist: %.2f\nLeader Dist: %.2f\n%s"), Index, MoveSpeed, DistanceToFinalDestination, LeaderDist, *Side);
	
	DrawDebugString(GetWorld(), GetCharacter()->GetActorLocation(), DebugMessage, nullptr, FColor::White, DeltaTime, true);
	*/

	/*
	bool bLeaderAtBack = FVector::DotProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal2D(), PerpDirection) < 0.0f;
	FString ForceSnakeString = (USWATManager::Get(this)->bForceSnakeFallIn ? "Forcing Single File Fall In" : "");
	FString LeaderSideString = (bLeaderAtBack ? "Back" : "Front");
	FString SortedSwat;
	for (ASWATCharacter* Swat : USWATManager::Get(this)->FallInSwat)
	{
		SortedSwat += Swat->GetName();
		SortedSwat += LINE_TERMINATOR;
	}
	FString CenterDebugMessage = FString::Printf(TEXT("%s\n%s\n%s"), *ForceSnakeString, *LeaderSideString, *SortedSwat);
	
	DrawDebugString(GetWorld(), SwatAverageLocation, CenterDebugMessage, nullptr, FColor::White, DeltaTime, true);
	*/
}
#endif

bool UTeamFallinActivity::CanFinishActivity() const
{
	return false;
}

void UTeamFallinActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	MyLeader = nullptr;
}

void UTeamFallinActivity::ResumeActivity()
{
	Super::ResumeActivity();

	Location = CalculateFallInPosition(GetSharedData<FSharedFallInData>()->Pattern);
}

void UTeamFallinActivity::ResetData()
{
	Super::ResetData();

	MyLeader = nullptr;

	bFirstCalculation = true;
}

bool UTeamFallinActivity::ShouldForceStrafe() const
{
	if (bIsSwapping)
		return false;
	
	return UReadyOrNotAISystem::WasRecentlyInCombat(5.0f, false);
}

void UTeamFallinActivity::GatherDebugString(FString& OutString)
{
	#if !UE_BUILD_SHIPPING
	Super::GatherDebugString(OutString);

	FVector SwatAverageLocation = USWATManager::Get(this)->GetAverageSwatLocation();

	TArray<ASWATCharacter*> SwatTeam = USWATManager::Get(this)->FallInSwat;

	int32 Index = SwatTeam.Find(Cast<ASWATCharacter>(GetCharacter()));

	float MoveSpeed = 0.0f;
	GetOverrideMovementSpeed(MoveSpeed);
	
	float DistanceToFinalDestination = 0.0f;

	bool bHasPath = false;
	if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
	{
		if (Path.IsValid())
		{
			bHasPath = true;
			DistanceToFinalDestination = Path->GetLength();
		}
	}

	float LeaderDist = 0.0f;
	if (MyLeader)
	{
		LeaderDist = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation());
	}
	
	FVector AverageDirection = FVector::ZeroVector;
	for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
	{
		AverageDirection += (SwatAverageLocation - swat->GetActorLocation()).GetSafeNormal2D();
	}
	
	AverageDirection.Normalize();
	FVector PerpDirection = FVector::CrossProduct(AverageDirection, FVector::UpVector);

	TArray<ASWATCharacter*> FrontSwat, BackSwat;
	for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
	{
		if (FVector::DotProduct((swat->GetActorLocation() - SwatAverageLocation).GetSafeNormal2D(), PerpDirection) < 0.0f)
		{
			BackSwat.Add(swat);
		}
		else
		{
			FrontSwat.Add(swat);
		}
	}

	FString Side = BackSwat.Contains(Cast<ASWATCharacter>(GetCharacter())) ? "Back" : "Front";
	//FString DebugMessage = FString::Printf(TEXT("%i\nSpeed: %.2f\nPath Dist: %.2f\nLeader Dist: %.2f\n%s"), Index, MoveSpeed, DistanceToFinalDestination, LeaderDist, *Side);

	OutString += AddDebugString("Squad Position", RON_ENUM_TO_STRING(ESquadPosition, OverrideSquadPosition));
	OutString += AddDebugString("", FString::FromInt(Index));
	OutString += AddDebugString("Speed", FString::Printf(TEXT("%.2f"), MoveSpeed));
	if (bHasPath)
		OutString += AddDebugString("Path Dist", FString::Printf(TEXT("%.2f"), DistanceToFinalDestination));
	if (MyLeader)
		OutString += AddDebugString("Leader Dist", FString::Printf(TEXT("%.2f"), LeaderDist));
	OutString += AddDebugString("", Side);
	#endif
}

FVector UTeamFallinActivity::CalculateFallInPosition(EFallInPattern Pattern)
{
	AReadyOrNotCharacter* const SquadLeader = USWATManager::Get(this)->GetSquadLeader();
	if (!SquadLeader)
	{
		return FVector::ZeroVector;
	}
	
	TArray<ASWATCharacter*> SortedSWAT = USWATManager::Get(this)->FallInSwat;

	SortedSWAT.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	if (SortedSWAT.Num() == 0)
		return FVector::ZeroVector;

	//if (USWATManager::Get(this)->bWaitingOnPathTest)
		//return FVector::ZeroVector;
	if (USWATManager::Get(this)->bForceSnakeFallIn)
		return SnakeFallInPositionV2(SquadLeader, SortedSWAT);
	
	switch (Pattern)
	{
		case EFallInPattern::Snake:		return SnakeFallInPositionV2(SquadLeader, SortedSWAT, 100.0f);
		case EFallInPattern::HalfSnake:	return HalfSnakeFallInPosition(SquadLeader);
		case EFallInPattern::Diamond:	return DiamondFallInPosition(SquadLeader);
		case EFallInPattern::Flock:		return FlockFallInPosition(SquadLeader);
		default: break;
	}
	
	return FVector::ZeroVector;
}

FVector UTeamFallinActivity::SnakeFallInPosition(AReadyOrNotCharacter* MasterLeader, const TArray<ASWATCharacter*>& SortedSwat, float Spacing)
{
	AReadyOrNotCharacter* SquadLeader = MasterLeader;
	AReadyOrNotCharacter* Leader = MasterLeader;
	
	if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
	{
		Leader = GetCharacterAtSquadPosition((ESquadPosition)((uint8)OverrideSquadPosition-1));
	}
	
	if (Leader)
	{
		if (Leader != SquadLeader)
		{
			float LeaderDist = FVector::Distance(GetCharacter()->GetActorLocation(), Leader->GetActorLocation());
			if (LeaderDist > 500.0f)
			{
				return Leader->GetNavAgentLocation();
			}
		}
		
		FCollisionQueryParams QueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), Leader);
		QueryParams.AddIgnoredActor(SquadLeader);
		QueryParams.AddIgnoredActors((TArray<AActor*>)SortedSwat);

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

		// move into LOS of our leader if obstructed by a wall
		if (GetWorld()->LineTraceTestByObjectType(Leader->GetActorLocation(), GetCharacter()->GetActorLocation(), ObjectQueryParams, QueryParams))
		{
			DrawDebugBox(GetWorld(), Leader->GetNavAgentLocation(), FVector(15.0f), FColor::Cyan, false);
			//ULog::Info(GetCharacter()->GetName() + " | Out of LOS of leader");
			//return FVector::ZeroVector;
			return Leader->GetNavAgentLocation();
		}

		if (FIntVector(LastRequestedLocation) == FIntVector(Leader->GetNavAgentLocation()))
		{
			AbortMove();
			Location = FVector::ZeroVector;
		}
	}
	
	int32 Index = SortedSwat.Find(Cast<ASWATCharacter>(GetCharacter()));
	if (Index == INDEX_NONE)
	{
		//MyLeader = nullptr;
		//OverrideSquadPosition = ESquadPosition::SP_NONE;
		return FVector::ZeroVector;
	}
	
	if (OverrideSquadPosition != (ESquadPosition)Index)
	{
		MyLeader = nullptr;
	}
	
	OverrideSquadPosition = (ESquadPosition)Index;

	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return FVector::ZeroVector;

	bool bAlphaLeaderMoved = FVector::Distance(SquadLeader->GetActorLocation(), SortedSwat[0]->GetActorLocation()) > 300.0f;
	/*
	if (float* DistPtr = USWATManager::Get(this)->FallInSwat_PathFound.Find(SortedSWAT[0]))
	{
		bAlphaLeaderMoved = (*DistPtr > 300.0f);
	}
	*/
	
	bool bMyLeaderMoved = false;
	
	if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
	{
		int32 LeaderIndex = FMath::Clamp(Index-1, 0, 4);
		if (SortedSwat.IsValidIndex(LeaderIndex))
			bMyLeaderMoved = FVector::Distance(SortedSwat[LeaderIndex]->GetActorLocation(), GetCharacter()->GetActorLocation()) > Spacing + 50.0f;
			//bMyLeaderMoved = SortedSwat[LeaderIndex]->GetVelocity().Size() > 50.0f;
	}
	
	if (MyLeader)
	{
		bMyLeaderMoved = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation()) > Spacing;
		//bMyLeaderMoved = MyLeader->GetVelocity().Size() > 50.0f;
	}
	
	if (bAlphaLeaderMoved || bMyLeaderMoved)
	{
		if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		{
			//Leader = GetCharacterAtSquadPosition((ESquadPosition)((uint8)OverrideSquadPosition-Missing-1));
			int32 LeaderIndex = FMath::Clamp((uint8)OverrideSquadPosition-1, 0, 4);
			if (SortedSwat.IsValidIndex(LeaderIndex))
			{
				Leader = SortedSwat[LeaderIndex];
				MyLeader = Cast<ASWATCharacter>(Leader);
			}
		}
		
		if (!Leader)
			return FVector::ZeroVector;
		
		const FVector LeaderLocation = Leader->GetNavAgentLocation();
		const FVector DirectionToLeader = (Leader->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();

		//float SpacingClamped = (FMath::Clamp(Spacing-50.0f, 125.0f, 1000.0f));
		float DistanceToTarget = DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 100.0f : 150.0f;
		if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 100.0f : 150.0f;

		if (Spacing <= 20.0f)
		{
			DistanceToTarget = 0.0f;
		}

		//bool bLeaderHasMoved = FVector::Distance(LeaderLocation, GetCharacter()->GetNavAgentLocation()) > Spacing;
		
		//if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			//bLeaderHasMoved = bAlphaLeaderMoved;

		//if (bLeaderHasMoved)
		{
			if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
				ULog::Info(GetCharacter()->GetName() + " | Leader " + SquadLeader->GetName() + " has moved");
			else
				ULog::Info(GetCharacter()->GetName() + " " + RON_ENUM_TO_STRING(ESquadPosition, (ESquadPosition)((uint8)OverrideSquadPosition)) + " | Leader " + RON_ENUM_TO_STRING(ESquadPosition, (ESquadPosition)((uint8)OverrideSquadPosition-1)) + " has moved");
			
			//return FVector::ZeroVector;
			return LeaderLocation + DirectionToLeader * -DistanceToTarget;
		}
	}

	return FVector::ZeroVector;
}

FVector UTeamFallinActivity::SnakeFallInPositionV2(AReadyOrNotCharacter* MasterLeader, const TArray<ASWATCharacter*>& SortedSwat, float Spacing)
{
	AReadyOrNotCharacter* Leader = nullptr;
	float Dist = FVector::Distance(MasterLeader->GetNavAgentLocation(), GetCharacter()->GetNavAgentLocation());
	
	if (SortedSwat[0] == GetCharacter())
	{
		OverrideSquadPosition = ESquadPosition::SP_Alpha;

		if (const float* PathDist = USWATManager::Get(this)->FallInSwat_PathFound.Find(SortedSwat[0]))
		{
			Dist = *PathDist - Dist; // minus the distance that was originally added in swat manager
		}
		
		//LOG_NUMBER(Dist);
		constexpr float Tolerance = 300.0f;
		if (Dist < Tolerance && !bFirstCalculation)
			return FVector::ZeroVector;
		
		Leader = MasterLeader;
	}
	
	const int32 Index = SortedSwat.Find(GetCharacter<ASWATCharacter>());
	
	OverrideSquadPosition = (ESquadPosition)Index;
	
	if (SortedSwat.IsValidIndex(Index-1))
	{
		Leader = SortedSwat[Index-1];

		if (const float* PathDist = USWATManager::Get(this)->FallInSwat_PathFound.Find(GetCharacter<ASWATCharacter>()))
		{
			Dist = *PathDist - Dist; // minus the distance that was originally added in swat manager
		}
		
		//LOG_NUMBER(Dist);
		constexpr float Tolerance = 500.0f;
		if (Dist < Tolerance && ElapsedActivityTime > 0.25f)
			return FVector::ZeroVector;
	}
	
	MyLeader = Cast<ASWATCharacter>(Leader);
	
	if (Leader)
	{
		const FVector LeaderLocation = Leader->GetNavAgentLocation();
		const FVector DirectionToLeader = (LeaderLocation - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();
		float DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 100.0f : 150.0f;
		
		//float DistanceToFinalDestination = FVector::Distance(LeaderLocation, GetCharacter()->GetActorLocation());
		if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
		{
			if (Path.IsValid() && Path->GetPathPoints().Num() > 2)
			{
				FVector LastPoint = Path->GetPathPoints().Last();
				FVector SecondLastPoint = Path->GetPathPoints()[Path->GetPathPoints().Num()-2];

				FVector Direction = (LastPoint - SecondLastPoint).GetSafeNormal();
				return LeaderLocation + Direction * -DistanceToTarget;
			}
		}
		
		return LeaderLocation + DirectionToLeader * -DistanceToTarget;
		

		FVector a;
		//DrawDebugPoint(GetWorld(), LeaderLocation + DirectionToLeader * -DistanceToTarget, 15.0f, FColor::Magenta, false, 1.0f);
		
		if (!UReadyOrNotAISystem::ProjectPointToNav(LeaderLocation + DirectionToLeader * -DistanceToTarget, a, FVector(30.0f, 30.0f, 50.0f)))
		{
			DistanceToTarget = 200.0f;
			
			if (!UReadyOrNotAISystem::ProjectPointToNav(LeaderLocation + DirectionToLeader * -DistanceToTarget, a, FVector(30.0f, 30.0f, 50.0f)))
			{
				DistanceToTarget = 50.0f;
			}
		}

		//LOG_NUMBER(DistanceToTarget);
		
		//const float ZHeight = FMath::Abs(LeaderLocation.Z - GetCharacter()->GetActorLocation().Z);
		//LOG_NUMBER(ZHeight);
		//float DistanceToFinalDestination = FVector::Distance(LeaderLocation, GetCharacter()->GetActorLocation());
		//if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
		/*
		{
			if (LastNavPath.IsValid())
			{
				DistanceToFinalDestination = LastNavPath->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), LastNavPath->GetPathPoints().Num()-1);
			}
		}
		*/
		//DrawDebugPoint(GetWorld(), LeaderLocation, 15.0f, FColor::Magenta, false, 1.0f);
		//DrawDebugPoint(GetWorld(), GetCharacter()->GetActorLocation(), 15.0f, FColor::White, false, 1.0f);
		//if (ZHeight > 75.0f && FVector::Distance(LeaderLocation, GetCharacter()->GetNavAgentLocation()) > 200.0f)
		//	DistanceToTarget = 0.0f;
		//LOG_NUMBER(DistanceToFinalDestination);
		//if (DistanceToFinalDestination > 500.0f)
			//DistanceToTarget = 0.0f;

		//LOG_NUMBER(DistanceToTarget);
		return LeaderLocation + DirectionToLeader * -DistanceToTarget;
	}

	return FVector::ZeroVector;
}

FVector UTeamFallinActivity::HalfSnakeFallInPosition(AReadyOrNotCharacter* MasterLeader)
{
	AReadyOrNotCharacter* SquadLeader = MasterLeader;
	AReadyOrNotCharacter* Leader = SquadLeader;
	TArray<ASWATCharacter*> Swat_FileA = USWATManager::Get(this)->FallInSwat_FileA;
	TArray<ASWATCharacter*> Swat_FileB = USWATManager::Get(this)->FallInSwat_FileB;

	Swat_FileA.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	Swat_FileB.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});

	if (USWATManager::Get(this)->SwatAI.Num() < 2)
	{
		return SnakeFallInPositionV2(SquadLeader, USWATManager::Get(this)->FallInSwat);
	}
	
	if (Swat_FileA.Num() == 0 || Swat_FileB.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	const int32 FileAIndex = Swat_FileA.Find(Cast<ASWATCharacter>(GetCharacter()));
	const int32 FileBIndex = Swat_FileB.Find(Cast<ASWATCharacter>(GetCharacter()));
	
	if (FileAIndex == INDEX_NONE && FileBIndex == INDEX_NONE)
		return FVector::ZeroVector;
	
	int32 Index = FileAIndex != INDEX_NONE ? FileAIndex : FileBIndex;
	if (Index == INDEX_NONE)
		return FVector::ZeroVector;
	
	const TArray<ASWATCharacter*>& CurrentFile = FileAIndex != INDEX_NONE ? Swat_FileA : Swat_FileB;
	
	if (OverrideSquadPosition != (ESquadPosition)Index)
	{
		MyLeader = nullptr;
	}
	
	OverrideSquadPosition = (ESquadPosition)Index;

	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return FVector::ZeroVector;
	
	ASWATCharacter* ClosestSwatToSquadLeader = USWATManager::Get(this)->GetClosestSWATToLocation(SquadLeader->GetActorLocation());
	bool bExceededDistanceThreshold = FVector::Distance(ClosestSwatToSquadLeader->GetActorLocation(), SquadLeader->GetActorLocation()) > 250.0f;
	if (!bExceededDistanceThreshold && !bFirstCalculation)
	{
		if (MyLeader)
		{
			bExceededDistanceThreshold = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation()) > 250.0f;
			if (!bExceededDistanceThreshold)
				return FVector::ZeroVector;
		}
		else
		{
			return FVector::ZeroVector;
		}
	}

	const bool bAlphaLeaderMoved = FVector::Distance(SquadLeader->GetActorLocation(), GetCharacter()->GetActorLocation()) > 250.0f;
	
	bool bMyLeaderMoved = false;
	if (MyLeader)
	{
		bMyLeaderMoved = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation()) > 250.0f;
	}

	if (bAlphaLeaderMoved || bMyLeaderMoved || bFirstCalculation)
	{
		if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		{
			Leader = CurrentFile[0];
			MyLeader = Cast<ASWATCharacter>(Leader);
		}

		if (!Leader)
			return FVector::ZeroVector;
		
		const FVector LeaderLocation = Leader->GetNavAgentLocation();
		const FVector DirectionToLeader = (Leader->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();

		float DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 125.0f : 200.0f;
		if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 120.0f : 150.0f;

		constexpr float DistanceThreshold = 250.0f;
		const bool bLeaderHasMoved = (LeaderLocation - GetCharacter()->GetNavAgentLocation()).Size() > DistanceThreshold;

		if (bLeaderHasMoved || bFirstCalculation)
		{
			const FVector SwatAverageLocation = USWATManager::Get(this)->GetAverageSwatLocation();
			const FVector PerpDirection = FVector::CrossProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), FVector::UpVector);

			const bool bOnLeftSide = CurrentFile == Swat_FileB;
			
			if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			{
				if (bOnLeftSide)
				{
					return (LeaderLocation + DirectionToLeader * -DistanceToTarget) - PerpDirection * 45.0f;
				}
				
				return (LeaderLocation + DirectionToLeader * -DistanceToTarget) + PerpDirection * 45.0f;
			}

			return LeaderLocation + DirectionToLeader * -DistanceToTarget;
		}
	}

	return FVector::ZeroVector;
}

FVector UTeamFallinActivity::DiamondFallInPosition(AReadyOrNotCharacter* MasterLeader)
{
	AReadyOrNotCharacter* SquadLeader = MasterLeader;
	AReadyOrNotCharacter* Leader = SquadLeader;
	if (!SquadLeader)
	{
		return FVector::ZeroVector;
	}
	
	bool bExceededDistanceThreshold = false;
	
	if (ASWATCharacter* ClosestSwatToLeader = USWATManager::Get(this)->GetClosestSWATToActor(SquadLeader))
	{
		bExceededDistanceThreshold = FVector::Distance(ClosestSwatToLeader->GetActorLocation(), SquadLeader->GetActorLocation()) > 250.0f;
	}

	if (!bExceededDistanceThreshold && !bFirstCalculation)
		return FVector::ZeroVector;
	
	TArray<ASWATCharacter*> SwatTeam = USWATManager::Get(this)->FallInSwat_Diamond;

	SwatTeam.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	if (SwatTeam.Num() < 3)
	{
		return SnakeFallInPositionV2(MasterLeader, USWATManager::Get(this)->FallInSwat);
	}

	const int32 Index = SwatTeam.Find(Cast<ASWATCharacter>(GetCharacter()));
	if (Index == INDEX_NONE)
		return FVector::ZeroVector;
	
	if (OverrideSquadPosition != (ESquadPosition)Index)
	{
		MyLeader = nullptr;
	}
	
	OverrideSquadPosition = (ESquadPosition)Index;

	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return FVector::ZeroVector;

	const bool bAlphaLeaderMoved = FVector::Distance(SquadLeader->GetActorLocation(), GetCharacter()->GetActorLocation()) > 250.0f;
	
	bool bMyLeaderMoved = false;
	if (MyLeader)
	{
		bMyLeaderMoved = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation()) > 250.0f;
	}

	//if (bAlphaLeaderMoved || bMyLeaderMoved)
	{
		if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		{
			Leader = SwatTeam[0];
			MyLeader = Cast<ASWATCharacter>(Leader);
		}

		if (!Leader)
			return FVector::ZeroVector;
		
		const FVector LeaderLocation = Leader->GetNavAgentLocation();
		const FVector DirectionToLeader = (Leader->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();

		float DistanceToTarget = DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 100.0f : 200.0f;
		if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 100.0f : 200.0f;

		float DistanceThreshold = 250.0f;
		if (OverrideSquadPosition == ESquadPosition::SP_Delta)
			DistanceThreshold = 450.0f;
		const bool bLeaderHasMoved = (LeaderLocation - GetCharacter()->GetNavAgentLocation()).Size() > DistanceThreshold;
		
		//if (bLeaderHasMoved)
		{
			const FVector SwatAverageLocation = USWATManager::Get(this)->GetAverageSwatLocation();
			const FVector PerpDirection = FVector::CrossProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), FVector::UpVector);

			bool bOnLeftSide = FVector::DotProduct((GetCharacter()->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), PerpDirection) < 0.0f;

			switch (OverrideSquadPosition)
			{
				case ESquadPosition::SP_Alpha:			return LeaderLocation + DirectionToLeader * -DistanceToTarget;
				case ESquadPosition::SP_Beta:
				case ESquadPosition::SP_Charlie:
				{
					// switch sides to prevent overlapping/clipping
					{
						const ASWATCharacter* Beta = SwatTeam[1];
						const ASWATCharacter* Charlie = SwatTeam[2];

						const bool bBetaOnLeftSide = FVector::DotProduct((Beta->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), PerpDirection) < 0.0f;
						const bool bCharlieOnLeftSide = FVector::DotProduct((Charlie->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), PerpDirection) < 0.0f;

						const bool bBetaAndCharlieSameSide = bBetaOnLeftSide == bCharlieOnLeftSide;
						if (bBetaAndCharlieSameSide)
						{
							if (GetCharacter() == Beta)
								bOnLeftSide = !bBetaOnLeftSide;
						}
					}
						
					if (bOnLeftSide)
						return LeaderLocation + (DirectionToLeader * -DistanceToTarget) - PerpDirection * 125.0f;
						
					return LeaderLocation + (DirectionToLeader * -DistanceToTarget) + PerpDirection * 125.0f;
				}
				case ESquadPosition::SP_Delta:			return LeaderLocation + DirectionToLeader * -DistanceToTarget*2.0f;
				default:								return FVector::ZeroVector;
			}
		}
	}

	return FVector::ZeroVector;
}

FVector UTeamFallinActivity::FlockFallInPosition(AReadyOrNotCharacter* MasterLeader)
{
	AReadyOrNotCharacter* SquadLeader = MasterLeader;
	AReadyOrNotCharacter* Leader = SquadLeader;
	if (!SquadLeader)
	{
		return FVector::ZeroVector;
	}
	
	TArray<ASWATCharacter*> Swat_FileA = USWATManager::Get(this)->FallInSwat_FileA;
	TArray<ASWATCharacter*> Swat_FileB = USWATManager::Get(this)->FallInSwat_FileB;

	Swat_FileA.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	Swat_FileB.RemoveAll([&](ASWATCharacter* swat)
	{
		if (USWATManager::Get(this)->IsSWATValid(swat))
			return swat->GetCyberneticsController()->GetCurrentActivity<UTeamFallinActivity>() == nullptr;

		return true;
	});
	
	if (USWATManager::Get(this)->SwatAI.Num() < 2)
	{
		return SnakeFallInPositionV2(MasterLeader, USWATManager::Get(this)->FallInSwat);
	}
	
	if (Swat_FileA.Num() == 0 || Swat_FileB.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	const int32 FileAIndex = Swat_FileA.Find(Cast<ASWATCharacter>(GetCharacter()));
	const int32 FileBIndex = Swat_FileB.Find(Cast<ASWATCharacter>(GetCharacter()));
	
	if (FileAIndex == INDEX_NONE && FileBIndex == INDEX_NONE)
		return FVector::ZeroVector;
	
	int32 Index = FileAIndex != INDEX_NONE ? FileAIndex : FileBIndex;
	if (Index == INDEX_NONE)
		return FVector::ZeroVector;
	
	const TArray<ASWATCharacter*>& CurrentFile = FileAIndex != INDEX_NONE ? Swat_FileA : Swat_FileB;
	
	if (OverrideSquadPosition != (ESquadPosition)Index)
	{
		MyLeader = nullptr;
	}
	
	OverrideSquadPosition = (ESquadPosition)Index;

	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return FVector::ZeroVector;

	const bool bAlphaLeaderMoved = FVector::Distance(SquadLeader->GetActorLocation(), GetCharacter()->GetActorLocation()) > 250.0f;
	
	bool bMyLeaderMoved = false;
	if (MyLeader)
	{
		bMyLeaderMoved = FVector::Distance(GetCharacter()->GetActorLocation(), MyLeader->GetActorLocation()) > 250.0f;
	}

	if (bAlphaLeaderMoved || bMyLeaderMoved || bFirstCalculation)
	{
		if (OverrideSquadPosition != ESquadPosition::SP_Alpha)
		{
			if (CurrentFile.IsValidIndex(Index - 1))
				Leader = CurrentFile[Index - 1];
			
			MyLeader = Cast<ASWATCharacter>(Leader);
		}

		if (!Leader)
			return FVector::ZeroVector;
		
		const FVector LeaderLocation = Leader->GetNavAgentLocation();
		const FVector DirectionToLeader = (Leader->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();

		float DistanceToTarget = DistanceToTarget = Leader->GetVelocity().Size() > 100.0f ? 125.0f : 200.0f;
		if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			DistanceToTarget = 100;//Leader->GetVelocity().Size() > 100.0f ? 100.0f : 200.0f;

		constexpr float DistanceThreshold = 250.0f;
		const bool bLeaderHasMoved = (LeaderLocation - GetCharacter()->GetNavAgentLocation()).Size() > DistanceThreshold;

		if (bLeaderHasMoved || bFirstCalculation)
		{
			const FVector SwatAverageLocation = USWATManager::Get(this)->GetAverageSwatLocation();
			const FVector PerpDirection = FVector::CrossProduct((SquadLeader->GetActorLocation() - SwatAverageLocation).GetSafeNormal(), FVector::UpVector);

			const bool bOnLeftSide = CurrentFile == Swat_FileB;
			
			if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			{
				if (bOnLeftSide)
				{
					return LeaderLocation - PerpDirection * 100.0f + DirectionToLeader * -DistanceToTarget;
				}
				
				return LeaderLocation + PerpDirection * 100.0f + DirectionToLeader * -DistanceToTarget;
			}

			if (bOnLeftSide)
			{
				return LeaderLocation - PerpDirection * ((int32)OverrideSquadPosition+2)*50.0f + DirectionToLeader * -DistanceToTarget;
			}
			
			return LeaderLocation + PerpDirection * ((int32)OverrideSquadPosition+2)*50.0f + DirectionToLeader * -DistanceToTarget;
		}
	}

	return FVector::ZeroVector;
}

void UTeamFallinActivity::SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated)
{
	if (OverrideSquadPosition == ESquadPosition::SP_NONE)
		return;
	
	// can't swap with yourself :p
	if (SquadPosition == OverrideSquadPosition)
		return;
	
	TArray<ASWATCharacter*> Swat_FileA = USWATManager::Get(this)->FallInSwat_FileA;
	TArray<ASWATCharacter*> Swat_FileB = USWATManager::Get(this)->FallInSwat_FileB;

	UActivityManager::IterateAllActivitiesOfType<UTeamFallinActivity>([&](UTeamFallinActivity* Activity)
	{
		if (Activity != this && Activity->SharedData->ActivityId == SharedData->ActivityId)
		{
			if (Activity->OverrideSquadPosition == SquadPosition)
			{
				const EFallInPattern Pattern = GetSharedData<FSharedFallInData>()->Pattern;
				
				if (Pattern == EFallInPattern::HalfSnake) // special case with half snake, to ensure proper swapping
				{
					const int32 FileAIndex = Swat_FileA.Find(Cast<ASWATCharacter>(Activity->GetCharacter()));
					const int32 FileBIndex = Swat_FileB.Find(Cast<ASWATCharacter>(Activity->GetCharacter()));
					
					if (FileAIndex == INDEX_NONE && FileBIndex == INDEX_NONE)
						return true;
					
					int32 Index = FileAIndex != INDEX_NONE ? FileAIndex : FileBIndex;
					if (Index == INDEX_NONE)
						return true;
					
					const TArray<ASWATCharacter*>& CurrentFile = FileAIndex != INDEX_NONE ? Swat_FileA : Swat_FileB;
					
					//ULog::Array_Object((TArray<UObject*>)Swat_FileA, "A: ");
					//ULog::Array_Object((TArray<UObject*>)Swat_FileB, "B: ");

					const bool bInSameFile = CurrentFile.Find(GetCharacter<ASWATCharacter>()) != INDEX_NONE;
					if (!bInSameFile)
					{
						return true;
					}
				}
				
				Swap(OverrideSquadPosition, Activity->OverrideSquadPosition);
				SetLocation(Activity->GetCharacter()->GetNavAgentLocation(), true);
				Activity->SetLocation(GetCharacter()->GetNavAgentLocation(), true);
				bIsSwapping = true;
				Activity->bIsSwapping = true;
				return false;
			}
		}
		
		return true;
	});
}

void UTeamFallinActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	OwningController->GetTargetingComp()->SetIgnoredTrackingDirection(FVector::ZeroVector);
}

float UTeamFallinActivity::GetDestinationTolerance() const
{
	/*
	if (GetSharedData<FSharedFallInData>()->Pattern == EFallInPattern::Snake)
	{
		if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
			return 150.0f;
		
		return 200.0f;
	}
	*/

	return 30.0f;
}

bool UTeamFallinActivity::GetOverrideMovementSpeed(float& OutMovementSpeed) const
{
	if (bIsSwapping)
	{
		return false;
	}
	
	const AReadyOrNotCharacter* const SquadLeader = USWATManager::Get(this)->GetSquadLeader();
	const AReadyOrNotCharacter* Leader = SquadLeader;

	if (!Leader)
		return false;

	if (MyLeader)
		Leader = MyLeader;
	
	constexpr float DistanceThreshold = 100.0f;
	const float DistanceToLeader = FVector::Distance(Leader->GetNavAgentLocation(), GetCharacter()->GetNavAgentLocation());
	float DistanceToFinalDestination = 0.0f;
	
	if (const FNavPathSharedPtr Path = OwningController->GetRONPathFollowingComp()->GetPath())
	{
		if (Path.IsValid())
		{
			DistanceToFinalDestination = Path->GetLengthFromPosition(GetCharacter()->GetNavAgentLocation(), Path->GetPathPoints().Num()-1);
		}
	}
	
	const bool bLeaderHasMoved = DistanceToLeader > DistanceThreshold;

	if (bLeaderHasMoved)
	{
		float MaxSpeed = Leader->GetCharacterMovement()->GetMaxSpeed() * 0.98f;
		if (SquadLeader->GetVelocity().Size() < 50.0f && DistanceToFinalDestination > 300.0f)
			MaxSpeed = 240.0f;
		
		OutMovementSpeed = FMath::GetMappedRangeValueClamped(FVector2D(80.0f, 250.0f), FVector2D(100.0f, MaxSpeed), DistanceToFinalDestination);
		return true;
	}

	OutMovementSpeed = 100.0f;
	return true;
}

TSubclassOf<UNavigationQueryFilter> UTeamFallinActivity::GetNavigationQueryOverride()
{
	return UNavQuery_SwatFallIn::StaticClass();
}

bool UTeamFallinActivity::OverrideAvoidanceLocation() const
{
	return true;
}

FVector UTeamFallinActivity::GetBestAvoidanceLocation(ACyberneticCharacter* OverlappingAI) const
{
	return FVector::ZeroVector;
	if (OverrideSquadPosition == ESquadPosition::SP_Alpha)
	{
		return FVector::ZeroVector;
	}

	const AReadyOrNotCharacter* Leader = GetCharacterAtSquadPosition((ESquadPosition)((uint8)OverrideSquadPosition-1));

	if (!Leader)
		return FVector::ZeroVector;
	
	const FVector LeaderLocation = Leader->GetNavAgentLocation();
	const FVector DirectionToLeader = (Leader->GetNavAgentLocation() - GetCharacter()->GetNavAgentLocation()).GetSafeNormal();

	constexpr float DistanceToTarget = 200.0f;

	return LeaderLocation + DirectionToLeader * -DistanceToTarget;
}
