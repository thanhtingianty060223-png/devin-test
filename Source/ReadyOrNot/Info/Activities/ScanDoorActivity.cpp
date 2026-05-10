// Copyright Void Interactive, 2023

#include "Info/Activities/ScanDoorActivity.h"

#include "Actors/Door.h"
#include "Actors/StackUpActor.h"
#include "Actors/Attachments/LaserAttachment.h"
#include "Actors/Attachments/LightAttachment.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "Info/SWATManager.h"
#include "Team/TeamStackUpActivity.h"

extern TAutoConsoleVariable<int32> CVarRonDisplaySwatDebug;

UScanDoorActivity::UScanDoorActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "Scan");
	bFinishActivityWhenOverriden = true;
	bIsProgressActivity = true;
	bAbortIfTrackingEnemy = false;

	ActivityStateMachine->AddState("Move To")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::EnterMoveToStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::TickMoveToStage))
						.CreateTransition("Scan", MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::CanStartScanning));
	
	ActivityStateMachine->AddState("Scan")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::EnterScanStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::TickScanStage))
						.CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::IsScanComplete));
	
	ActivityStateMachine->AddState("Complete")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::EnterCompleteStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UScanDoorActivity::TickCompleteStage));
}

void UScanDoorActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (ScanMethod == EDoorScanMethod::None)
	{
		ACTIVITY_FAILED("Scan method is None");
		return;
	}

	switch (ScanMethod)
	{
		case EDoorScanMethod::Slide:			ActivityName = FText::FromStringTable("SwatCommandTable", "SlideBy"); break;
		case EDoorScanMethod::Slice:			ActivityName = FText::FromStringTable("SwatCommandTable", "PIE"); break;
		case EDoorScanMethod::Snap:				ActivityName = FText::FromStringTable("SwatCommandTable", "Peek"); break;
		case EDoorScanMethod::CenterCheck:		ActivityName = FText::FromStringTable("SwatCommandTable", "CenterCheck"); break;
		default:								ActivityName = FText::FromStringTable("SwatCommandTable", "Scan"); break;
	}

	// TODO: make it run in an event, when any activity has started > stop gesture
	GetCharacter<ASWATCharacter>()->StopGestureAnimation();
}

bool UScanDoorActivity::ShouldForceStrafe() const
{
	return FIntVector(Location) != OriginalLocation;
}

bool UScanDoorActivity::ShouldForceNoStrafe() const
{
	return false;
}

void UScanDoorActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

}

#if !UE_BUILD_SHIPPING
void UScanDoorActivity::PerformActivity_Debug(float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (CVarRonDisplaySwatDebug.GetValueOnAnyThread() == 0)
		return;
	
	const bool bIsCommandInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);

	FVector TraceStart = Door->GetDoorMidLocation();
	TraceStart += bIsCommandInFrontOfDoor ? Door->GetActorForwardVector() * -75.0f : Door->GetActorForwardVector() * 75.0f;
	TraceStart.Z += 50.0f;
	
    for (const ACyberneticCharacter* AI : SpottedCharacters)
    {
        if (AI)
        {
			DrawDebugLine(GetWorld(), TraceStart, AI->GetMesh()->GetBoneLocation("head"), FColor::Green, false, DeltaTime, 0, 0.5f);
        }
    }

	FVector ScanPoint;
	OverrideFocalPoint(ScanPoint);
	DrawDebugPoint(GetWorld(), ScanPoint, 10.0f, FColor::Cyan, false, DeltaTime);

	FString DebugMessage;
	DebugMessage += AddDebugString("Lean", FString::SanitizeFloat(GetCharacter()->QuickLeanAmount));
	DebugMessage += AddDebugString("Scan Method", RON_ENUM_TO_STRING(EDoorScanMethod, ScanMethod));
	DebugMessage += AddDebugString("Spotted", FString::FromInt(SpottedCharacters.Num()));
	if (ScanMethod == EDoorScanMethod::Slice)
		DebugMessage += AddDebugString("Slice Stage", FString::FromInt(SliceStage));

	FVector DebugLocation = Door->GetDoorMidLocation();
	if (Door->GetSubDoor())
		DebugLocation = Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
	
	DrawDebugString(GetWorld(), DebugLocation + FVector::UpVector * 50.0f, DebugMessage, nullptr, FColor::White, DeltaTime, true);
}
#endif

void UScanDoorActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	if (UTeamStackUpActivity* StackUpActivity = OwningController->GetActivity<UTeamStackUpActivity>())
	{
		StackUpActivity->CalculateStackUpPosition();
	}
}

bool UScanDoorActivity::CanFinishActivity() const
{
	return false;
}

bool UScanDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (GetActiveStateID() == 2 || StartFocalPoint == FVector::ZeroVector)
	{
		FocalPoint = Door->GetDoorMidLocation();
		return true;
	}
	
	switch (ScanMethod)
	{
		case EDoorScanMethod::Slide:		FocalPoint = SlideFocalPoint(); break;
		case EDoorScanMethod::Slice:		FocalPoint = SliceFocalPoint(); break;
		case EDoorScanMethod::Snap:			FocalPoint = SnapFocalPoint(); break;
		case EDoorScanMethod::CenterCheck:	FocalPoint = SliceFocalPoint(); break;
		default:							FocalPoint = SlideFocalPoint(); break;
	}
	
	return true;
}

bool UScanDoorActivity::GetOverrideMovementSpeed(float& OutMovementSpeed) const
{
	if (GetActiveStateID() == 0)
	{
		if (Location != FVector::ZeroVector)
		{
			OutMovementSpeed = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 600.0f), FVector2D(100.0f, 240.0f), FVector::Distance(Location, GetCharacter()->GetNavAgentLocation()));
			return true;
		}

		return false;
	}
	
	if (GetActiveStateID() > 0)
	{
		if (ScanMethod == EDoorScanMethod::Slide || ScanMethod == EDoorScanMethod::CenterCheck)
		{
			OutMovementSpeed = 150.0f;
			return true;
		}
		
		OutMovementSpeed = 120.0f;
		return true;
	}

	return false;
}

void UScanDoorActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);
	
}

void UScanDoorActivity::ResumeActivity()
{
	Super::ResumeActivity();
	
	if (!HasReachedLocation() || Location == FVector::ZeroVector)
	{
		Location = LastRequestedLocation;
		RequestMoveAsync();
	}
}

void UScanDoorActivity::ResetData()
{
	Super::ResetData();
	
	Door = nullptr;
	CommandLocation = FVector::ZeroVector;
	ScanMethod = EDoorScanMethod::Slide;
	SpottedCharacters.Empty();

	OriginalLocation = FIntVector::ZeroValue;
	OppositeStackUpLocation = FIntVector::ZeroValue;
	RightDoorVector = FVector::ZeroVector;
	SliceStage = 0;

	bPeekedDoor = false;

	SpottedTrap = nullptr;
}

bool UScanDoorActivity::CanBePushed() const
{
	return GetActiveStateID() == 0;
}

bool UScanDoorActivity::GetLeanOverride(float& LeanOverride) const
{
	if (ScanMethod == EDoorScanMethod::Snap)
	{
		if (Door->IsPointRightOfDoorway(FVector(OriginalLocation)))
		{
			LeanOverride = -1.0f;
			return true;
		}
		
		if (Door->IsOpenBy(0.5f))
		{
			LeanOverride = -1.0f;
			return true;
		}
		
		LeanOverride = 1.0f;
		return true;
	}
	
	if (ScanMethod == EDoorScanMethod::Slice || ScanMethod == EDoorScanMethod::CenterCheck)
	{
		if (Door->IsPointInFrontOfDoorway(CommandLocation))
		{
			if (Door->IsPointRightOfDoorway(FVector(OriginalLocation)))
			{
				LeanOverride = 1.0f;
				return true;
			}
			
			LeanOverride = -1.0f;
			return true;
		}
		
		if (Door->IsPointRightOfDoorway(FVector(OriginalLocation)))
		{
			LeanOverride = -1.0f;
			return true;
		}
		
		LeanOverride = 1.0f;
		return true;
	}

	return false;
}

void UScanDoorActivity::EnterMoveToStage()
{
	const bool bHasStackUpActivity = OwningController->GetActivity<UTeamStackUpActivity>() != nullptr;
	const bool bIsOnFrontSideOfDoor = Door->IsPointInFrontOfDoorway(GetCharacter()->GetActorLocation());
	
	// closed, or almost closed
	if (!Door->IsDoorwayOnly() && !Door->IsOpenBeyondIncrementThreshold() && !bHasStackUpActivity && ScanMethod != EDoorScanMethod::Snap)
	{
		EStackupGenArea StackUpArea;
		if (bIsOnFrontSideOfDoor)
		{
			StackUpArea = EStackupGenArea::SGA_FrontRight;
		}
		else
		{
			StackUpArea = EStackupGenArea::SGA_BackRight;
		}
		
		TArray<AStackUpActor*> StackUpActors = Door->GetStackupsForArea(StackUpArea);
		if (StackUpActors.Num() == 0)
		{
			return;	
		}

		SetLocation(StackUpActors[0]->GetActorLocation(), true);
		return;
	}
	
	const bool bCommandFront = Door->IsPointInFrontOfDoorway(CommandLocation);
	const FVector DirectionToDoor = (GetCharacter()->GetActorLocation() - Door->GetDoorway()->GetComponentLocation()).GetSafeNormal2D();
	const float RightDotProduct = FVector::DotProduct(DirectionToDoor, Door->GetActorRightVector());
	const bool bIsOnRightSideOfDoor = RightDotProduct > 0.0f;

	// Find out which area (quadrant) we are in 
	EStackupGenArea StackUpArea;
	if (bIsOnFrontSideOfDoor)
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
	}
	else
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
	}

	if (bCommandFront != bIsOnFrontSideOfDoor)
		ADoor::FlipStackUpArea(StackUpArea, false, true);

	TArray<AStackUpActor*> StackUpActors = Door->GetStackupsForArea(StackUpArea);
	if (StackUpActors.Num() == 0)
	{
		ACTIVITY_FAILED("No stackup actors found", true);
		return;	
	}

	bool bShouldRequestMove = true;
	if (OwningController->GetActivity<UTeamStackUpActivity>())
	{
		bShouldRequestMove = false;
	}

	if (bShouldRequestMove)
	{
		SetLocation(StackUpActors[0]->GetActorLocation(), true);
	}
}

void UScanDoorActivity::TickMoveToStage(float DeltaTime, float Uptime)
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "MovingToPosition");
}

bool UScanDoorActivity::CanStartScanning() const
{
	return HasReachedLocation(20.0f);
}

void UScanDoorActivity::EnterScanStage()
{
	const bool bIsOnFrontSideOfDoor = Door->IsPointInFrontOfDoorway(GetCharacter()->GetActorLocation());
	const FVector DirectionToDoor = (GetCharacter()->GetActorLocation() - Door->GetDoorway()->GetComponentLocation()).GetSafeNormal2D();
	const float RightDotProduct = FVector::DotProduct(DirectionToDoor, Door->GetActorRightVector());
	const bool bIsOnRightSideOfDoor = RightDotProduct > 0.0f;

	// Find out which area (quadrant) we are in 
	EStackupGenArea StackUpArea;
	if (bIsOnFrontSideOfDoor)
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
	}
	else
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
	}
	
	TArray<AStackUpActor*> StackUpActors = Door->GetStackupsForArea(StackUpArea);
	if (StackUpActors.Num() == 0)
	{
		ACTIVITY_FAILED("No stackup actors found", true);
		return;	
	}

	float RightOffset = Door->GetDoorSize().Y;
	
	if (Door->GetSubDoor())
	{
		RightOffset *= 2;
	}
	
	StartFocalPoint = Door->GetActorLocation() + Door->GetActorRightVector() * RightOffset;
	StartFocalPoint.Z = Door->GetDoorMidLocation().Z;
	
	OriginalLocation = FIntVector(StackUpActors[0]->GetActorLocation());
	
	ADoor::FlipStackUpArea(StackUpArea, true, false);
	
	StackUpActors = Door->GetStackupsForArea(StackUpArea);

	if (StackUpActors.Num() == 0)
	{
		ACTIVITY_FAILED("No stackup actors found", true);
		return;	
	}

	// is door closed? peek it!
	if (!Door->IsDoorwayOnly() && !Door->IsOpenBeyondIncrementThreshold() && !Door->IsDoorBroken())
	{
		Door->PeekDoor(GetCharacter(), Door->GetIncrementAngle());
		GetCharacter()->GetEquippedItem()->PlayDoorPushAnimation();
		
		if (Door->IsLocked() || Door->IsJammed())
		{
			if (Door->IsLocked())
				GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_LOCKED);
			else if (Door->IsJammed())
				GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_JAMMED);
			
			ACTIVITY_FAILED("Door Locked", true);
			return;
		}
		
		bPeekedDoor = true;
	}

	if (OppositeStackUpLocation == FIntVector::ZeroValue)
	{
		OppositeStackUpLocation = FIntVector(StackUpActors[0]->GetActorLocation());
	}

	if (LOCAL_PLAYER)
	{
		if (FVector::Distance(LocalPlayer->GetNavAgentLocation(), FVector(OppositeStackUpLocation)) < 100.0f)
		{
			ScanMethod = EDoorScanMethod::CenterCheck;
		}
	}

	if (ScanMethod == EDoorScanMethod::Snap)
	{
		FVector Back = -Door->GetActorForwardVector();
		if (Door->IsPointInFrontOfDoorway(CommandLocation))
			Back = Door->GetActorForwardVector();
		
		{
			if (Door->IsActorRightOfDoorway(GetCharacter()))
			{
				OppositeStackUpLocation = FIntVector(Door->GetDoorMidLocation() + Back * 50.0f + Door->GetActorRightVector() * (Door->GetDoorSize().Y+10.0f));
				//OppositeStackupLocation = Door->GetActorLocation() + Back * 50.0f + Door->GetActorRightVector() * 130.0f;
				SetLocation(FVector(OppositeStackUpLocation), true);
				OppositeStackUpLocation = FIntVector(Location);
			}
			else
			{
				OppositeStackUpLocation = FIntVector(Door->GetDoorMidLocation() + Back * 50.0f + Door->GetActorRightVector() * (-(Door->GetDoorSize().Y)));
				//OppositeStackupLocation = Door->GetActorLocation() + Back * 50.0f + Door->GetActorRightVector() * -20.0f;
				OppositeStackUpLocation.Z = Door->GetActorLocation().Z;
				SetLocation(FVector(OppositeStackUpLocation), true);
				OppositeStackUpLocation = FIntVector(Location);
			}
		}
	}
	else if (ScanMethod == EDoorScanMethod::Slice || ScanMethod == EDoorScanMethod::CenterCheck)
	{
		if (ScanMethod == EDoorScanMethod::CenterCheck)
		{
			bAbortIfTrackingEnemy = true;
		}
		
		RightDoorVector = -Door->GetActorRightVector();
		if (Door->IsActorRightOfDoorway(GetCharacter()))
			RightDoorVector = Door->GetActorRightVector();

		SliceStage++;
	}
	else
	{
		SetLocation(FVector(OppositeStackUpLocation), true);
		OppositeStackUpLocation = FIntVector(Location);
	}
}

void UScanDoorActivity::TickScanStage(float DeltaTime, float Uptime)
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "Scanning");

	LOCAL_PLAYER;

	if (Location != FVector::ZeroVector &&
		ScanMethod != EDoorScanMethod::CenterCheck &&
		LocalPlayer && FVector::Distance(LocalPlayer->GetNavAgentLocation(), FVector(OppositeStackUpLocation)) < 100.0f)
	{
		ScanMethod = EDoorScanMethod::CenterCheck;
		Location = FVector::ZeroVector;
	}

	if (ScanMethod == EDoorScanMethod::Snap)
	{
		if (Uptime > 2.0f)
		{
			//if (LocationBeforeReturn == FVector::ZeroVector)
				//LocationBeforeReturn = GetCharacter()->GetNavAgentLocation();
			
			//SetLocation(OriginalLocation, true);
		}
	}
	else if (ScanMethod == EDoorScanMethod::Slice || ScanMethod == EDoorScanMethod::CenterCheck)
	{
		if (Door->IsOpenAtOrBeyond(0.5f) || Door->IsDoorwayOnly())
		{
			if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
			{
				if (Weapon->GetLightAttachment())
				{
					Weapon->GetLightAttachment()->ToggleLight(true);
				}
				
				if (Weapon->GetLaserAttachment())
				{
					Weapon->GetLaserAttachment()->ToggleLaser(true);
				}
			}
		}

		if (ScanMethod == EDoorScanMethod::CenterCheck)
		{
			if (SliceStage > 2)
			{
				if (HasReachedLocation())
				{
					return;
				}
			}
		}
		
		if (SliceStage >= 3)
		{
			if (HasReachedLocation())
			{
				SetLocation(FVector(OppositeStackUpLocation), true);
				OppositeStackUpLocation = FIntVector(Location);
			}
			
			return;
		}
		
		if (HasReachedLocation())
		{
			FVector BaseLocation = Door->GetDoorMidLocation();
			
			if (Door->GetSubDoor())
			{
				BaseLocation = Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
			}
			
			BaseLocation.Z = Door->GetActorLocation().Z;

			float Depth = 20.0f;
			if (Door->IsPointInFrontOfDoorway(CommandLocation))
			{
				BaseLocation += Door->GetActorForwardVector() * Depth;
			}
			else
			{
				BaseLocation += Door->GetActorForwardVector() * -Depth;
			}

			bool bNegate;
			
			if (Door->IsPointInFrontOfDoorway(CommandLocation))
			{
				bNegate = false;
				if (Door->IsActorRightOfDoorway(GetCharacter()))
					bNegate = true;
			}
			else
			{
				bNegate = true;
				if (Door->IsActorRightOfDoorway(GetCharacter()))
					bNegate = false;
			}

			float Distance = FVector::Distance(BaseLocation, FVector(OppositeStackUpLocation));// ScanMethod == EDoorScanMethod::Slice ? 120.0f : 85.0f;
			if (ScanMethod == EDoorScanMethod::CenterCheck)
				Distance /= 1.5f;
			FVector MoveLocation = BaseLocation + RightDoorVector.RotateAngleAxis(SliceStage * (bNegate ? -45.0f : 45.0f), FVector::UpVector) * Distance;

			#if !UE_BUILD_SHIPPING
			DrawDebugDirectionalArrow(GetWorld(), BaseLocation, MoveLocation, 10.0f, FColor::Orange, false, DeltaTime);
			#endif
		
			SetLocation(MoveLocation, true);
			SliceStage++;
		}
	}

	if (CanScanForThreats())
	{
		if (Door->IsTrapLive())
			SpottedTrap = Door->GetAttachedTrap();
		
		const bool bIsCommandInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);

		FVector TraceStart = Door->GetDoorMidLocation();
		TraceStart += bIsCommandInFrontOfDoor ? Door->GetActorForwardVector() * -75.0f : Door->GetActorForwardVector() * 75.0f;
		TraceStart.Z += 50.0f;

		// Find all characters in sight
		for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
		{
			if (IsValid(AI) && !AI->IsOnSWATTeam())
			{
				if (!SpottedCharacters.Contains(AI) && !Door->IsPointsOnOppositeSideOfDoor(TraceStart, AI->GetActorLocation()))
				{
					FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(AI, GetCharacter());
					
					if (!GetWorld()->LineTraceTestByChannel(TraceStart, AI->GetMesh()->GetBoneLocation("head"), ECC_Visibility, CollisionQueryParams))
					{
						SpottedCharacters.AddUnique(AI);
					}
					else if (!GetWorld()->LineTraceTestByChannel(TraceStart, AI->GetActorLocation(), ECC_Visibility, CollisionQueryParams))
					{
						SpottedCharacters.AddUnique(AI);
					}
				}
			}
		}
	}
}

bool UScanDoorActivity::IsScanComplete() const
{
	if (ScanMethod == EDoorScanMethod::Snap)
	{
		return GetActiveStateUptime() > 2.0f;
	}

	if (ScanMethod == EDoorScanMethod::CenterCheck)
	{
		return SliceStage >= 3 && HasReachedLocation(50.0f);
	}
	
	if (FIntVector(Location.X, Location.Y, OppositeStackUpLocation.Z) == OppositeStackUpLocation || FIntVector(Location) == OriginalLocation)
	{
		return HasReachedLocation(50.0f);
	}

	return false;
}

void UScanDoorActivity::EnterCompleteStage()
{
	if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
	{
		if (Weapon->GetLightAttachment())
		{
			Weapon->GetLightAttachment()->ToggleLight(false);
		}
		
		if (Weapon->GetLaserAttachment())
		{
			Weapon->GetLaserAttachment()->ToggleLaser(false);
		}
	}

	if (bPeekedDoor)
	{
		// was door peeked open? close it!
		if (!Door->IsDoorwayOnly() && Door->IsOpen() && !Door->IsDoorBroken())
		{
			Door->PeekDoor(GetCharacter(), Door->GetIncrementAngle());
			GetCharacter()->GetEquippedItem()->PlayDoorPushAnimation();
		}
	}

	bPeekedDoor = false;
	
	if (ScanMethod == EDoorScanMethod::Snap)
	{
		if (LocationBeforeReturn == FIntVector::ZeroValue)
			LocationBeforeReturn = FIntVector(GetCharacter()->GetNavAgentLocation());
		
		SetLocation(FVector(OriginalLocation), true);
	}

	if (!CanScanForThreats())
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}
	
    int32 SpottedEnemyCount = 0;
    int32 SpottedCivilianCount = 0;

    for (const ACyberneticCharacter* AI : SpottedCharacters)
    {
        if (AI)
        {
            if (AI->IsSuspect())
            {
                SpottedEnemyCount++;
            }
            else if (AI->IsCivilian())
            {
                SpottedCivilianCount++;
            }
        }
    }
	
    // Call out the results
    if (SpottedEnemyCount == 0 && SpottedCivilianCount == 0 && !SpottedTrap)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_NONE, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "NoTargetsVisible");
    }
    else if (SpottedEnemyCount == 1 && SpottedCivilianCount == 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN_AND_SUSPECT, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneSuspectAndOneCivilianSpotted");
    }
    else if (SpottedEnemyCount == 1 && SpottedCivilianCount == 0)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_SUSPECT, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneSuspectSpotted");
    }
    else if (SpottedEnemyCount == 0 && SpottedCivilianCount == 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneCivilianSpotted");
    }
    else if (SpottedEnemyCount > 1 && SpottedCivilianCount == 0)
    {    
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_SUSPECTS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleSuspectsSpotted");
    }
    else if (SpottedEnemyCount == 0 && SpottedCivilianCount > 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleCiviliansSpotted");
    }
    else
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS_AND_SUSPECTS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleSuspectsAndCiviliansSpotted");
    }

	if (SpottedTrap)
	{
		GetCharacter()->PlayRawVO(FMath::RandBool() ? VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_EXPLOSIVE : VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_NO_MIRRORGUN, "", false);
		ProgressState = FText::FromStringTable("SwatCommandTable", "TrapSpotted");
	}

	if (ScanMethod == EDoorScanMethod::CenterCheck)
		OwningController->FinishActivity(this, true, true);
}

void UScanDoorActivity::TickCompleteStage(float DeltaTime, float Uptime)
{
	if (HasReachedLocation() && Uptime > 0.25f)
	{
		OwningController->FinishActivity(this, true, true);
	}
}

bool UScanDoorActivity::CanScanForThreats() const
{
	return ScanMethod != EDoorScanMethod::CenterCheck;
}

FVector UScanDoorActivity::SliceFocalPoint() const
{
	float Offset = 15.0f;
	if (ScanMethod == EDoorScanMethod::CenterCheck) Offset = Door->GetDoorSize().Y;
	
	const float Width = Door->GetDoorSize().Y-Offset;
	
	const bool bRightOfDoor = Door->IsActorRightOfDoorway(GetCharacter());
	FVector Start = bRightOfDoor ? Door->GetDoorMidLocation() - Door->GetActorRightVector() * Width : Door->GetDoorMidLocation() + Door->GetActorRightVector() * Width;
	FVector End = bRightOfDoor ? Door->GetDoorMidLocation() + Door->GetActorRightVector() * Width : Door->GetDoorMidLocation() - Door->GetActorRightVector() * Width;
	
	if (Door->GetSubDoor())
		End += Door->GetActorRightVector() * Width;

	constexpr float Depth = 50.0f;
	if (Door->IsPointInFrontOfDoorway(CommandLocation))
	{
		Start += Door->GetActorForwardVector() * -Depth;
		End += Door->GetActorForwardVector() * -Depth;
	}
	else
	{
		Start += Door->GetActorForwardVector() * Depth;
		End += Door->GetActorForwardVector() * Depth;
	}

	const FVector P = GetCharacter()->GetActorLocation();
	const FVector A = End;
	const FVector B = Start;
	const FVector AB = B - A;
	const FVector AP = P - A;
	
	const float PathSegmentProgress = FMath::Clamp(FVector::DotProduct(AP, AB)/FVector::DotProduct(AB, AB), -1.0f, 1.0f);

	return FMath::Lerp(Start, End, PathSegmentProgress);
}

FVector UScanDoorActivity::SlideFocalPoint() const
{
	if (Location != FVector::ZeroVector && OriginalLocation != FIntVector::ZeroValue)
	{
		FVector Start = Door->GetDoorMidLocation();
		FVector End = Start;

		constexpr float Depth = 100.0f;
		if (Door->IsPointInFrontOfDoorway(CommandLocation))
		{
			End += Door->GetActorForwardVector() * -Depth;
		}
		else
		{
			End += Door->GetActorForwardVector() * Depth;
		}

		const FVector P = GetCharacter()->GetNavAgentLocation();
		const FVector A = FVector(OriginalLocation);
		const FVector B = Location;
		const FVector AB = B - A;
		const FVector AP = P - A;
		
		float PathSegmentProgress = FVector::DotProduct(AP, AB)/FVector::DotProduct(AB, AB);
		PathSegmentProgress = FMath::Clamp(PathSegmentProgress, 0.0f, 1.0f);

		return FMath::Lerp(Start, End, PathSegmentProgress);
	}
	
	return Door->GetDoorMidLocation();
}

FVector UScanDoorActivity::SnapFocalPoint() const
{
	//if (OppositeStackUpLocation != FIntVector::ZeroValue && FIntVector(Location) != OppositeStackUpLocation && FIntVector(Location) != OriginalLocation)
	{
		FVector Start = Door->GetDoorMidLocation();
		FVector End = Start;
		
		constexpr float Depth = 300.0f;
		if (Door->IsPointInFrontOfDoorway(CommandLocation))
		{
			End += Door->GetActorForwardVector() * -Depth;
		}
		else
		{
			End += Door->GetActorForwardVector() * Depth;
		}

		if (Door->IsOpenBy(0.5f))
		{
			if (!Door->IsActorRightOfDoorway(GetCharacter()))
			{
				return Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
			}
		}
		
		return End;
	}

	return Door->GetDoorMidLocation();
}
