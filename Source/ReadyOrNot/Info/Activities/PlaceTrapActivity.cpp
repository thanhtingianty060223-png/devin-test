// Void Interactive, 2022

#include "PlaceTrapActivity.h"

#include "Actors/Door.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

// TODO: Do not make this activity a world building activity
// TODO(killo): Trap placement should play door rotation jiggle curve
// GetCharacter()->GetMesh()->GetAnimInstance()->GetCurveValue("DoorYawRot");

TAutoConsoleVariable<int32> CVarPlaceTrapActivityDebug(TEXT("PlaceTrapActivity.DrawDebug"), 0, TEXT("0 = Dont draw debug info. 1 = Draw debug info"));

UPlaceTrapActivity::UPlaceTrapActivity()
{
	bOneShotAnimationDataTable = true;
	TableMontageName = "tp_spct_place_trap";
	bShouldHolsterWeapon = true;
	bShouldSurrenderFromActivity = true;
	MaxActivityTime = 20.0f;
}

void UPlaceTrapActivity::StartActivity(AAIController* Controller)
{
	Super::StartActivity(Controller);
	ACTIVITY_FAILED("PlaceTrapActivity disabled");
	return;
	
	if (!IsValid(Door))
	{
		ACTIVITY_FAILED("No door to place a trap on");
		return;
	}
	
	if (Door->GetAttachedTrap())
	{
		ACTIVITY_FAILED(Door->GetName() + " already has a trap attached", true);
		return;
	}

	// note(killo): Might solve problem where move orders that occur before trap is placed ignore door having trap
	Door->SetDoorTrapKnowledge(true, true);

	UnbindEvents();
	BindEvents();
}

void UPlaceTrapActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	return;
	
	if (!IsValid(Door))
	{
		ACTIVITY_FAILED("No door to place a trap on");
		return;
	}
	
	if (Door->IsOpen())
	{
		ACTIVITY_FAILED(Door->GetName() + " is open. Cannot place trap", true);
		return;
	}

	// Wait until we have reached the door
	if (!HasReachedLocation(GetDestinationTolerance()))
	{
		ElapsedActivityTime = 0.0f;

		return;
	}

	// Wait until we holster our weapon
	if (TryHolsterWeapon())
	{
		ElapsedActivityTime = 0.0f;

		return;
	}

	// Wait until we start playing the trap placement animation
	if (!GetCharacter()->IsTableMontagePlaying(TableMontageName))
	{
		ElapsedActivityTime = 0.0f;

		return;
	}

	// Magnetize to a good spot away from the door, so as to not clip our body into the door and to perfectly line up the animation with the door
	constexpr float DistanceFromDoor = 100.0f; // 1m
	FVector TargetLocation = Door->GetDoorMidLocation();
	TargetLocation += Door->IsActorInFrontOfDoorway(GetCharacter()) ? Door->GetDoorway()->GetForwardVector() * DistanceFromDoor : Door->GetDoorway()->GetForwardVector() * -DistanceFromDoor;
	TargetLocation.Z = GetCharacter()->GetActorLocation().Z;

	GetCharacter()->SetActorLocation(FMath::VInterpTo(GetCharacter()->GetActorLocation(), TargetLocation, DeltaTime, 2.0f));

	#if !UE_BUILD_SHIPPING
	if (CVarPlaceTrapActivityDebug.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), TargetLocation, TargetLocation + FVector::UpVector * 10000.0f, FColor::Yellow, false, DeltaTime + 0.05f, 0, 10.0f);
		DrawDebugBox(GetWorld(), TargetLocation, FVector(15.0f), FColor::Yellow);
		DrawDebugBox(GetWorld(), Door->GetDoorMidLocation(), FVector(10.0f), FColor::Green);
		DrawDebugLine(GetWorld(), GetCharacter()->GetActorLocation(), Door->GetDoorMidLocation(), FColor::White, false, DeltaTime + 0.05f, 0, 1.5f);
	}
	#endif
	
	if (TableMontageAnim)
	{
		MaxActivityTime = TableMontageAnim->GetPlayLength();
	}
	
	if (ElapsedActivityTime > MaxActivityTime)
	{
		Door->SetTypeOfTrapRowName(TrapType);
		Door->SetupTrap();
		Door->SetDoorTrapKnowledge(true, true);
		
		OwningController->FinishActivity(this, true, true);
	}
}

void UPlaceTrapActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);
	return;
	
	GetCharacter()->StopTPMontageFromTable(TableMontageName);
	GetCharacter()->StopTPAnimMontage(TableMontageAnim); // Just in case if table montage could not be stopped, use the hard reference

	UnbindEvents();
}

bool UPlaceTrapActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (HasReachedLocation())
	{
		FVector TargetLocation = Door->GetDoorMidLocation();
		TargetLocation.Z = GetCharacter()->GetActorLocation().Z;

		FocalPoint = TargetLocation;
		return true;
	}
	
	return false;
}

void UPlaceTrapActivity::FinishedActivity_NoOwner(bool Success)
{
	Super::FinishedActivity_NoOwner(Success);
	return;
	
	UnbindEvents();
}

void UPlaceTrapActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	return;
	
	GetCharacter()->StopTPMontageFromTable(TableMontageName);
	GetCharacter()->StopTPAnimMontage(TableMontageAnim); // Just in case if table montage could not be stopped, use the hard reference
	
	UnbindEvents();
}

bool UPlaceTrapActivity::ShouldForceStrafe() const
{
	return false;
}

bool UPlaceTrapActivity::ShouldForceNoStrafe() const
{
	return true;
}

void UPlaceTrapActivity::OnTrapPlacementInterrupted()
{
	ACTIVITY_FAILED("Trap placement interrupted on " + Door->GetName(), true);
}

void UPlaceTrapActivity::BindEvents()
{
	if (Door)
	{
		Door->OnDoorOpened.AddDynamic(this, &UPlaceTrapActivity::OnTrapPlacementInterrupted);
		Door->OnDoorClosed.AddDynamic(this, &UPlaceTrapActivity::OnTrapPlacementInterrupted);
		Door->OnDoorBroken.AddDynamic(this, &UPlaceTrapActivity::OnTrapPlacementInterrupted);
	}
}

void UPlaceTrapActivity::UnbindEvents()
{
	if (Door)
	{
		Door->OnDoorOpened.RemoveAll(this);
		Door->OnDoorClosed.RemoveAll(this);
		Door->OnDoorBroken.RemoveAll(this);
	}
}

bool UPlaceTrapActivity::CanShoot() const
{
	return false;
}

float UPlaceTrapActivity::GetDestinationTolerance() const
{
	return 100.0f;
}
