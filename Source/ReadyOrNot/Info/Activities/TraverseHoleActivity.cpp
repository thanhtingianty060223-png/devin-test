// Copyright Void Interactive, 2022

#include "Info/Activities/TraverseHoleActivity.h"

#include "Characters/CyberneticController.h"

#include "Actors/WallHoleTraversal.h"

#include "Info/ReadyOrNotSignificanceManager.h"

#include "NavigationSystem.h"

TAutoConsoleVariable<int32> CVarHoleTraversalInstantEntry(TEXT("HoleTraversal.InstantEntry"), 0, TEXT("0 = Use entry animation. 1 = Instantly enter"));
TAutoConsoleVariable<int32> CVarHoleTraversalInstantExit(TEXT("HoleTraversal.InstantExit"), 0, TEXT("0 = Use exit animation. 1 = Instantly exit"));

UTraverseHoleActivity::UTraverseHoleActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "TraverseHole");
	bIsProgressActivity = false;
	bAbortMoveWhenActivityFinished = false;
	bAbortMoveWhenActivityOverriden = false;

	ActivityStateMachine->AddState("Move To Hole")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Enter_MoveToHole_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Tick_MoveToHole_State))
						.CreateTransition("Enter Hole", MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::CanEnterHole));

	ActivityStateMachine->AddState("Enter Hole")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Enter_EnterHole_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Tick_EnterHole_State))
						.CreateTransition("Move Through Hole", MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::ShouldMove));

	ActivityStateMachine->AddState("Move Through Hole")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Enter_Move_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Tick_Move_State))
						.CreateTransition("Exit Hole", MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::CanExitHole));

	ActivityStateMachine->AddState("Exit Hole")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Enter_ExitHole_State))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UTraverseHoleActivity::Tick_ExitHole_State));
	
	bIsInverseEntry = false;
}

void UTraverseHoleActivity::ResetData()
{
	Super::ResetData();
	
	WallHoleTraversalActor = nullptr;
	bIgnoreCooldown = false;
	bFromNavLink = false;
	bIsInverseEntry = false;

	EntryAnim = nullptr;
	LoopAnim = nullptr;
	ExitAnim = nullptr;

	TimeEnteringHole = 0.0f;
	TimeExitingHole = 0.0f;
	EntryAnimTime = 0.0f;
	ExitAnimTime = 0.0f;

	TraversalProgress = 0.0f;
}

void UTraverseHoleActivity::StartActivity(AAIController* Owner)
{
	if (!WallHoleTraversalActor)
	{
		ACTIVITY_FAILED("Wall hole traversal actor not specified");
		return;
	}

	if (!WallHoleTraversalActor->bEnabled)
	{
		ACTIVITY_FAILED(WallHoleTraversalActor->GetName() + " is disabled", true);
		return;
	}
	
	if (WallHoleTraversalActor->OccupiedByController && WallHoleTraversalActor->OccupiedByController != Owner)
	{
		ACTIVITY_FAILED(WallHoleTraversalActor->GetName() + " is occupied by " + Owner->GetName(), true);
		return;
	}

	if (WallHoleTraversalActor->IsCooldownActiveFor(Owner))
	{
		ACTIVITY_FAILED("Can't use hole traversal. " + WallHoleTraversalActor->GetName() + " has a cooldown active for " + Owner->GetName(), true);
		return;
	}

	if (OwningController->IsSWAT())
	{
		ACTIVITY_FAILED("Swat can't perform hole traversal", true);
		return;
	}
	
	EntryAnim = WallHoleTraversalActor->EntryAnim;
	LoopAnim = WallHoleTraversalActor->LoopAnim;
	ExitAnim = WallHoleTraversalActor->ExitAnim;
	
	if (!EntryAnim || !LoopAnim || !ExitAnim)
	{
		ACTIVITY_FAILED("Missing animations from " + WallHoleTraversalActor->GetName());
		return;
	}
	
	WallHoleTraversalActor->OccupiedByController = Owner;
	
	GetCharacter()->Rep_HoleTraversalAnimState = {};
	GetCharacter()->CurrentWallHoleTraversalInUse = WallHoleTraversalActor;

	EntryAnimTime = EntryAnim->GetPlayLength() - (EntryAnim->GetDefaultBlendOutTime() + 0.05f);
	ExitAnimTime = ExitAnim->GetPlayLength() - (ExitAnim->GetDefaultBlendOutTime() + 0.05f);

	TraversalProgress = 0.0f;
	
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(GetCharacter());
	
	Super::StartActivity(Owner);
}

void UTraverseHoleActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
}

#if !UE_BUILD_SHIPPING
void UTraverseHoleActivity::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);
	
	if (WallHoleTraversalActor)
	{
		WallHoleTraversalActor->DrawDebug();
	}
}
#endif

void UTraverseHoleActivity::FinishedActivity(const bool bSuccess)
{
	OwningController->bStopDecisionMaking = false;
	
	ULog::Info(GetName() + " finished");
	
	if (WallHoleTraversalActor)
	{
		if (!bIgnoreCooldown)
			WallHoleTraversalActor->AddCooldownFor(OwningController, WallHoleTraversalActor->CooldownAfterUse);
		
		WallHoleTraversalActor->OccupiedByController = nullptr;
	}

	RemoveIgnoredActors();
	
	GetCharacter()->CurrentWallHoleTraversalInUse = nullptr;
	GetCharacter()->LastWallHoleTraversalUsed = WallHoleTraversalActor;
	GetCharacter()->Rep_HoleTraversalAnimState = {};
	
	UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(GetCharacter());
	
	Super::FinishedActivity(bSuccess);
}

void UTraverseHoleActivity::FinishedActivity_NoOwner(const bool bSuccess)
{
	if (WallHoleTraversalActor)
	{
		if (!bIgnoreCooldown)
			WallHoleTraversalActor->AddCooldownFor(OwningController, WallHoleTraversalActor->CooldownAfterUse);
		
		WallHoleTraversalActor->OccupiedByController = nullptr;
	}
	
	Super::FinishedActivity_NoOwner(bSuccess);
}

bool UTraverseHoleActivity::CanFinishActivity() const
{
	return GetActiveStateID() == 3 && !GetCharacter()->Rep_HoleTraversalAnimState.bIsTraversing;
}

bool UTraverseHoleActivity::CanShoot() const
{
	return false;
}

bool UTraverseHoleActivity::ShouldForceStrafe() const
{
	return true;
}

bool UTraverseHoleActivity::ShouldForceNoStrafe() const
{
	return false;
}

bool UTraverseHoleActivity::CanOverrideActivity() const
{
	return false;
}

bool UTraverseHoleActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (GetActiveStateID() == 0) // Move To Hole state
	{
		if (HasReachedEntryLocation(200.0f))
		{
			//FocalPoint = ChosenEntryTransform.GetLocation();
			if (bIsInverseEntry)
			{
				FocalPoint = ChosenEntryTransform.GetLocation() + ChosenEntryTransform.GetRotation().Vector() * -500.0f;
				FocalPoint.Z = GetCharacter()->GetActorLocation().Z;
				return true;
			}
			
			FocalPoint = ChosenEntryTransform.GetLocation() + ChosenEntryTransform.GetRotation().Vector() * 500.0f;
			FocalPoint.Z = GetCharacter()->GetActorLocation().Z;
			return true;
		}

		/*
		const float DistanceToLandmark = FVector::Distance(GetCharacter()->GetActorLocation(), ChosenEntryTransform.GetLocation()); 

		if (DistanceToLandmark < 350.0f || HasReachedLocation(150.0f))
		{
			FHitResult HitResult;
			const FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
			
			FVector TraceStart = GetCharacter()->GetMesh()->GetSocketLocation("head_end");
			FVector TraceEnd = ChosenEntryTransform.GetLocation();
			TraceEnd.Z = TraceStart.Z;

			bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams);

			if (bHasLOS)
			{
				FocalPoint = ChosenEntryTransform.GetLocation();
				FocalPoint.Z = TraceStart.Z;
			}

			return true;
		}
		*/

		/*
		if (OwningController->GetTrackedTarget())
		{
			FocalPoint = OwningController->GetTrackedTarget()->GetActorLocation();
			return true;
		}
		*/
		
		FocalPoint = FVector::ZeroVector;
		return false;
	}
	
	FocalPoint = FVector::ZeroVector;
	return true;
}

float UTraverseHoleActivity::GetDestinationTolerance() const
{
	return 50.0f;
}

bool UTraverseHoleActivity::HasReachedEntryLocation(const float Tolerance) const
{
	if (!OwningController || !GetCharacter())
		return false;

	const float ZHeightDifference = FMath::Abs(ChosenEntryTransform.GetLocation().Z - GetCharacter()->GetActorLocation().Z);
	if (ZHeightDifference > 150.0f)
		return false;

	const float Dist = FVector::Distance(ChosenEntryTransform.GetLocation(), FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, ChosenEntryTransform.GetLocation().Z));
	//LOG_NUMBER(Dist);
	return Dist < Tolerance;
}

void UTraverseHoleActivity::AddIgnoredActors()
{
	if (WallHoleTraversalActor)
	{
		for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : WallHoleTraversalActor->IgnoredMeshActors)
		{
			GetCharacter()->MoveIgnoreActorAdd(MeshActor.LoadSynchronous());
		}
		
		// Ignore all blocking volumes too
		for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
		{
			GetCharacter()->MoveIgnoreActorAdd(V);
		}
	}
}

void UTraverseHoleActivity::RemoveIgnoredActors()
{
	if (WallHoleTraversalActor)
	{
		for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : WallHoleTraversalActor->IgnoredMeshActors)
		{
			GetCharacter()->MoveIgnoreActorRemove(MeshActor.LoadSynchronous());
		}
		
		for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
		{
			GetCharacter()->MoveIgnoreActorRemove(V);
		}
	}
}

void UTraverseHoleActivity::Enter_MoveToHole_State()
{
	// Shortest path = Entry point
	// Longest path = Exit point
	{
		const FPathFindingResult PathA = FindPath(WallHoleTraversalActor->GetEntryTransform().GetLocation());
		const FPathFindingResult PathB = FindPath(WallHoleTraversalActor->GetExitTransform().GetLocation());

		float PathA_Length = FLT_MAX;
		float PathB_Length = FLT_MAX;

		const bool bAnyPathValid = (PathA.IsSuccessful() && PathA.Path.IsValid() && !PathA.IsPartial()) ||
									(PathB.IsSuccessful() && PathB.Path.IsValid() && !PathB.IsPartial());

		if (!bAnyPathValid)
		{
			ACTIVITY_FAILED("No valid paths to entry points for " + GetNameSafe(WallHoleTraversalActor), true);
			return;
		}
		
		if (PathA.IsSuccessful() && !PathA.IsPartial())
		{
			if (PathA.Path.IsValid())
			{
				PathA_Length = PathA.Path->GetLength();
			}
		}
		
		if (PathB.IsSuccessful() && !PathB.IsPartial())
		{
			if (PathB.Path.IsValid())
			{
				PathB_Length = PathB.Path->GetLength();
			}
		}
		
		if (PathA_Length < PathB_Length)
		{
			ChosenEntryTransform = WallHoleTraversalActor->GetEntryTransform();
			ChosenExitTransform = WallHoleTraversalActor->GetExitTransform();
			bIsInverseEntry = false;
		}
		else
		{
			ChosenEntryTransform = WallHoleTraversalActor->GetExitTransform();
			ChosenExitTransform = WallHoleTraversalActor->GetEntryTransform();
			bIsInverseEntry = true;
		}
	}

	// Entry offset
	{
		if (const UAnimSequence* AnimSequence = Cast<const UAnimSequence>(EntryAnim->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference))
		{
			const FTransform& PoseTransform = AnimSequence->ExtractRootTrackTransform(EntryAnim->GetPlayLength(), nullptr);

			const float TravelDistance = FVector::Distance(FVector::ZeroVector, PoseTransform.GetLocation());
			//LOG_NUMBER(TravelDistance);
			if (bIsInverseEntry)
				ChosenEntryTransform.SetLocation(ChosenEntryTransform.GetLocation() + ChosenEntryTransform.GetRotation().Vector() * TravelDistance);
			else
				ChosenEntryTransform.SetLocation(ChosenEntryTransform.GetLocation() + -ChosenEntryTransform.GetRotation().Vector() * TravelDistance);
		}
	}

	// Move to entry point of hole traversal
	SetLocation(ChosenEntryTransform.GetLocation(), true);
}

void UTraverseHoleActivity::Tick_MoveToHole_State(float DeltaTime, float Uptime)
{
	GetCharacter()->Rep_HoleTraversalAnimState.bLooping = false;

	if (!bFromNavLink)
	{
		if (!HasReachedEntryLocation(GetDestinationTolerance()))
		{
			SetLocation(ChosenEntryTransform.GetLocation(), true);
		}
	}
	else
	{
		if (HasReachedEntryLocation(100.0f))
		{
			if (!HasReachedEntryLocation(GetDestinationTolerance()))
				GetCharacter()->AddMovementInput((ChosenEntryTransform.GetLocation() - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), 1.0f, true);
		}
		else
		{
			if (Uptime > 5.0f)
				ACTIVITY_FAILED("Failed to move to entry point");
		}
	}
}

bool UTraverseHoleActivity::CanEnterHole() const
{
	//return HasReachedEntryLocation(GetDestinationTolerance());
	FVector FocalPoint = ChosenEntryTransform.GetLocation() + ChosenEntryTransform.GetRotation().Vector() * (bIsInverseEntry ? -500.0f : 500.0f);
	FocalPoint.Z = GetCharacter()->GetMesh()->GetSocketLocation("head").Z;

	const float DotProduct = FVector::DotProduct((FocalPoint - GetCharacter()->GetActorLocation()).GetSafeNormal2D(), GetCharacter()->GetActorForwardVector());
	const bool bIsFacingEntry = DotProduct > 0.9f;

	return HasReachedEntryLocation(GetDestinationTolerance()) && bIsFacingEntry;
}

void UTraverseHoleActivity::Enter_EnterHole_State()
{
	OwningController->bStopDecisionMaking = true;
	
	WallHoleTraversalActor->OccupiedByController = OwningController;

	TraversalProgress = 0.0f;

	AddIgnoredActors();
	
	// Don't try to move back
	SetLocation(FVector::ZeroVector);

	#if WITH_EDITOR
	ensureAlways(EntryAnim != nullptr);
	#endif
	
	GetCharacter()->Play3PMontage(EntryAnim);

	GetCharacter()->Rep_HoleTraversalAnimState.bIsTraversing = true;

	#if !UE_BUILD_SHIPPING
	if (CVarHoleTraversalInstantEntry.GetValueOnAnyThread() > 0)
	{
		GetCharacter()->Multicast_Stop3PMontage(EntryAnim, 0.0f);
		GetCharacter()->SetActorLocation(FVector(ChosenEntryTransform.GetLocation().X, ChosenEntryTransform.GetLocation().Y, GetCharacter()->GetActorLocation().Z));
    	GetCharacter()->SetActorRotation(ChosenEntryTransform.GetRotation().Rotator() - FRotator(0.0f, 90.0f, 0.0f));
	}
	#endif
}

void UTraverseHoleActivity::Tick_EnterHole_State(const float DeltaTime, const float Uptime)
{
	SetLocation(FVector::ZeroVector);

	GetCharacter()->Rep_HoleTraversalAnimState.bLooping = true;

	TimeEnteringHole = Uptime;
	
	FRotator NewRotation;
	if (bIsInverseEntry)
		NewRotation = ChosenEntryTransform.GetRotation().Rotator() + FRotator(0.0f, 180.0f, 0.0f);
	else
		NewRotation = ChosenEntryTransform.GetRotation().Rotator();

	const FRotator SmoothedRotation = FMath::RInterpTo(GetCharacter()->GetActorRotation(), NewRotation, DeltaTime, 5.0f);
	GetCharacter()->SetActorRotation(SmoothedRotation);
}

bool UTraverseHoleActivity::ShouldMove() const
{
	return TimeEnteringHole > EntryAnimTime;
}

void UTraverseHoleActivity::Enter_Move_State()
{
	GetCharacter()->Rep_HoleTraversalAnimState.bLooping = true;
	GetCharacter()->Play3PMontage(LoopAnim);
}

void UTraverseHoleActivity::Tick_Move_State(const float DeltaTime, float Uptime)
{
	SetLocation(FVector::ZeroVector);

	TimeEnteringHole = 0.0f;

	GetCharacter()->Rep_HoleTraversalAnimState.bLooping = true;
	
	// Play loop anim over and over
	if (!GetCharacter()->Is3PMontagePlaying(LoopAnim))
	{
		GetCharacter()->Play3PMontage(LoopAnim);
	}

	// Magnetize to the closest point on traversal line, so as to not stray from traversal path and get stuck somewhere
	{
		const FVector P = GetCharacter()->GetActorLocation();
		const FVector A = ChosenEntryTransform.GetLocation();
		const FVector B = ChosenExitTransform.GetLocation();
		const FVector AB = B - A;
		const FVector AP = P - A;
		
		TraversalProgress = FVector::DotProduct(AP, AB)/FVector::DotProduct(AB, AB);
		
		const FVector ClosestPoint = A + (FMath::Clamp(TraversalProgress, 0.0f, 1.0f) * AB);
		const FVector NewLocation = FVector(ClosestPoint.X, ClosestPoint.Y, GetCharacter()->GetActorLocation().Z);
		const FVector SmoothedLocation = FMath::VInterpTo(GetCharacter()->GetActorLocation(), NewLocation, DeltaTime, 1.0f);

		FRotator NewRotation;
		if (bIsInverseEntry)
			NewRotation = ChosenEntryTransform.GetRotation().Rotator() + FRotator(0.0f, 180.0f, 0.0f);
		else
			NewRotation = ChosenEntryTransform.GetRotation().Rotator();

		const FRotator SmoothedRotation = FMath::RInterpTo(GetCharacter()->GetActorRotation(), NewRotation, DeltaTime, 5.0f);

		GetCharacter()->SetActorLocationAndRotation(SmoothedLocation, SmoothedRotation);

		#if WITH_EDITOR
		DrawDebugSphere(GetWorld(), ClosestPoint, 5.0f, 16, FColor::Green, false, DeltaTime + 0.015f, 0, 0.0f);
		#endif
	}
}

bool UTraverseHoleActivity::CanExitHole() const
{
	const FVector TestLocation = FVector(GetCharacter()->GetActorLocation().X, GetCharacter()->GetActorLocation().Y, ChosenExitTransform.GetLocation().Z);
	if (UKismetMathLibrary::IsPointInBoxWithTransform(TestLocation, ChosenExitTransform, WallHoleTraversalActor->ExitTriggerBoxExtent))
		return true;

	// Failsafe
	if (TraversalProgress >= 1.0f || FMath::IsNearlyEqual(TraversalProgress, 1.0f, 0.0001f))
		return true;

	return false;
}

void UTraverseHoleActivity::Enter_ExitHole_State()
{
	AddIgnoredActors();
	
	GetCharacter()->Rep_HoleTraversalAnimState.bLooping = false;

	GetCharacter()->Play3PMontage(ExitAnim);

	if (WallHoleTraversalActor)
	{
		if (!bIgnoreCooldown)
			WallHoleTraversalActor->AddCooldownFor(OwningController, WallHoleTraversalActor->CooldownAfterUse);
	}
	
	#if !UE_BUILD_SHIPPING
	if (CVarHoleTraversalInstantExit.GetValueOnAnyThread() > 0)
	{
		GetCharacter()->Multicast_Stop3PMontage(nullptr, 0.0f);
		GetCharacter()->SetActorLocation(FVector(ChosenExitTransform.GetLocation().X, ChosenExitTransform.GetLocation().Y, GetCharacter()->GetActorLocation().Z));
		GetCharacter()->SetActorRotation(ChosenExitTransform.GetRotation().Rotator());
	}
	#endif
}

void UTraverseHoleActivity::Tick_ExitHole_State(const float DeltaTime, const float Uptime)
{
	if (Uptime > ExitAnimTime)
	{
		if (!GetCharacter()->Is3PMontagePlaying(ExitAnim))
		{
			GetCharacter()->Rep_HoleTraversalAnimState.bIsTraversing = false;
		}
	}
}

void UTraverseHoleActivity::OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::OnIncapacitated(IncapacitatedCharacter, InstigatorCharacter);

	if (GetCharacter())
		GetCharacter()->bDiedWhilstTraversingHole = true;
	
	TeleportWeapon();
}

void UTraverseHoleActivity::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::OnKilled(InstigatorCharacter, KilledCharacter);

	if (GetCharacter())
		GetCharacter()->bDiedWhilstTraversingHole = true;

	TeleportWeapon();
}

void UTraverseHoleActivity::TeleportWeapon()
{
	const ABaseItem* EquippedItem = GetCharacter()->GetEquippedItem();
	if (!EquippedItem)
		return;
	
	GetCharacter()->ThrowEquippedItem();
	
	const float DistanceToEntry = FVector::Distance(ChosenEntryTransform.GetLocation(), GetCharacter()->GetActorLocation());
	const float DistanceToExit = FVector::Distance(ChosenExitTransform.GetLocation(), GetCharacter()->GetActorLocation());

	if (DistanceToEntry < DistanceToExit)
	{
		EquippedItem->GetItemMesh()->SetWorldLocation(ChosenEntryTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		EquippedItem->GetItemMesh()->SetWorldLocation(ChosenExitTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
}
