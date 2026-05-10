// Copyright Void Interactive, 2022

#include "SWATCharacter.h"

#include "ReadyOrNotAISystem.h"
#include "GameModes/CoopGM.h"

#include "Actors/Attachments/LaserAttachment.h"
#include "Actors/Attachments/LightAttachment.h"
#include "Actors/BaseItem.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "Characters/CyberneticController.h"
#include "Commander/RosterManager.h"

#include "Components/ScoringComponent.h"
#include "Components/ObjectiveMarkerComponent.h"
#include "Components/ReadyOrNotCharMovementComp.h"
#include "Components/CharacterHealthComponent.h"

#include "DamageTypes/BulletDamageType.h"
#include "DamageTypes/LessLethal/BeanbagDamageType.h"
#include "DamageTypes/LessLethal/CSGasDamageType.h"
#include "DamageTypes/LessLethal/PepperballDamageType.h"
#include "Engine/DemoNetDriver.h"

#include "Info/ScoringManager.h"
#include "Info/SWATManager.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/Activities/SwatCombatActivity.h"

#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/DoorBreachActivity.h"

#include "Info/Activities/MoveToActivity.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Character Tick"), STAT_SwatCharacterTick, STATGROUP_SwatCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat View Block Test"), STAT_LeanTest, STATGROUP_SwatCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Swat Lean Test"), STAT_ViewBlockTest, STATGROUP_SwatCharacter);

TAutoConsoleVariable<int32> CVarSwatRosterDebug(TEXT("a.SwatRosterDebug"), 0, TEXT("View character details such as stress while in-game"));

ASWATCharacter::ASWATCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	IFMODStudioModule::Get();

	static ConstructorHelpers::FObjectFinder<UTexture2D> T_DiamondTilted(TEXT("Texture2D'/Game/ReadyOrNot/Assets/Particles/Command/T_Diamond.T_Diamond'"));
	FSlateBrush MarkerBrush;
	MarkerBrush.SetResourceObject(T_DiamondTilted.Object);

	PlayerMarkerComponent->bEnabled = true;
	PlayerMarkerComponent->InitMarkerSettings(MarkerBrush);
	PlayerMarkerComponent->bHideDirectionalArrow = true;
	PlayerMarkerComponent->IconBrush.SetResourceObject(nullptr);

	MoveStyle->TargetInterpSpeed = 10.0f;

	QuickLeanIntensity = 0.85f;
	QuickLeanInterpSpeed = 3.0f;
	
	FocalPointInterpSpeed = 2.0f;
	FocusTurnSpeed = 6.0f;
	TurnDegreesPerSecond = 60.0f;
	FocalPointInterpCurve = EAlphaBlendOption::HermiteCubic;
	ActorRotationInterpStandingSpeed = 5.0f;
	ActorRotationInterpMovingSpeed = 6.0f;
	AimOffsetInterpSpeed = 4.0f;
}

void ASWATCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	PlayerMarkerComponent->bDisplayMarkerText = true;
	
	bool bShowPlayerNames = false;
	UBpGameplayHelperLib::LoadShowPlayerNamesSetting(bShowPlayerNames);
	
	if (GetInventoryComponent()->GetSpawnedGear().Character && bShowPlayerNames)
		PlayerMarkerComponent->SetMarkerText(GetInventoryComponent()->GetSpawnedGear().Character->CharacterName);
	
	switch (GetTeam())
	{
		case ETeamType::TT_NONE:
			PlayerMarkerComponent->SetIconColor(FColor::White);
		break;

		case ETeamType::TT_SERT_RED:
			PlayerMarkerComponent->SetIconColor(FLinearColor(1.0f, 0.099356f, 0.039631, 0.8f));
		break;

		case ETeamType::TT_SERT_BLUE:
			PlayerMarkerComponent->SetIconColor(FLinearColor(0.25f, 0.48f, 1.0f, 0.8f));
		break;

		case ETeamType::TT_SUSPECT:
			PlayerMarkerComponent->SetIconColor(FLinearColor(1.0f, 0.099356f, 0.039631, 0.8f));
		break;

		case ETeamType::TT_CIVILIAN:
			PlayerMarkerComponent->SetIconColor(FLinearColor(0.25, 0.48, 1.0f, 0.8f));
		break;

		case ETeamType::TT_SQUAD:
			PlayerMarkerComponent->SetIconColor(FColor::Yellow);
		break;

		default: ;
	}
}

bool ASWATCharacter::IsAffectedByDamageType(UDamageType* DamageType) const
{
	if (/*UCSGasDamageType* CSGasDamage = */Cast<UCSGasDamageType>(DamageType))
		return false;
	
	return Super::IsAffectedByDamageType(DamageType);
}

void ASWATCharacter::Multicast_OnKilled_Implementation(FName LastBone, AActor* DamageCauser)
{
	Super::Multicast_OnKilled_Implementation(LastBone, DamageCauser);

	ScoringComponent->TakeScore(FText::FromStringTable("ScoringTable", "Alive"));

	UReadyOrNotGameInstance* GameInstance = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	if(GameInstance)
	{
		if (GetWorld()->GetDemoNetDriver())
		{
			V_LOGM(LogReadyOrNot, "Adding SwatKilled Replay Event.")
			GameInstance->AddReplayEvent(SwatKilled, GetActorLocation(), GetWorld()->GetDemoNetDriver()->GetDemoCurrentTime(), "");
		}
	}
}

void ASWATCharacter::Server_ReportTarget_Implementation(AActor* Character)
{
	const bool bHasBreachActivity = GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>() != nullptr;
	
	const bool bIsBreaching = (bHasBreachActivity && GetCyberneticsController()->GetActivity<UTeamBreachAndClearActivity>()->IsBreaching()) || GetCyberneticsController()->GetCurrentActivity<UDoorBreachActivity>();
	
	const bool bCanReportNow = !bIsBreaching;
	if (bCanReportNow)
	{
		Super::Server_ReportTarget_Implementation(Character);
	}
}

void ASWATCharacter::StartStun(EStunType StunType, AActor* StunCauser)
{
	// Swat don't get stunned
}

void ASWATCharacter::UpdateDefaultMoveStyle()
{
	if (!GetCyberneticsController())
		return;

	const bool bIsActiveForMovement = !IsDeadOrUnconscious() && !IsIncapacitated();
	if (!bIsActiveForMovement)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		GetCharacterMovement()->MaxWalkSpeedCrouched = 0.0f;
		GetCharacterMovement()->MaxAcceleration = 0.0f;
		MoveStyle->SetComponentTickEnabled(false);
		return;
	}
	
	MoveStyle->SetComponentTickEnabled(true);
	
	if (IsArrested())
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.ArrestedMoveStyle);
		return;
	}

	if (IsSurrenderedFor(1.5f) || IsSurrenderComplete())
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.SurrenderedMoveStyle);
		return;
	}

	if (IsStunned())
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.StunnedMoveStyle);
		return;
	}

	static const FName RifleLoweredInjuredMoveStyle = "male01_swat_rifle_injured";
	static const FName RifleStrafeInjuredMoveStyle = "male01_swat_rifle_strafe_injured";
	static const FName PistolStrafeInjuredMoveStyle = "male01_swat_pistol_strafe_injured";
	static const FName PistolLoweredInjuredMoveStyle = "male01_swat_pistol_injured";

	const bool IsInjured = IsHalfHealth() || bIsArterialBleeding;
	
	// weapon is lowered
	if (IsLowReady())
	{
		if (GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Pistol)
			MoveStyle->SetMovementStyleByName(IsInjured ? PistolLoweredInjuredMoveStyle : CurMoveDataBlock.PistolMovementStyle);
		else if (GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Rifle)
			MoveStyle->SetMovementStyleByName(IsInjured ? RifleLoweredInjuredMoveStyle : MovementStyleData.LoweredTwoHandedMoveStyle);
	}
	else
	{
		if (GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Pistol)
			MoveStyle->SetMovementStyleByName(IsInjured ? PistolStrafeInjuredMoveStyle : CurMoveDataBlock.PistolStrafeMovementStyle);
		else
			MoveStyle->SetMovementStyleByName(IsInjured ? RifleStrafeInjuredMoveStyle : MovementStyleData.RaisedTwoHandedMoveStyle);
	}
}

void ASWATCharacter::Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal)
{
	Super::Multicast_InflictSuppression_Implementation(SuppressionData, CameraShake, bLessLethal);

	ReturnSuppressiveFire(SuppressionData, bLessLethal);
}

void ASWATCharacter::Multicast_InflictSuppression_NoLineOfSight_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal)
{
	ReturnSuppressiveFire(SuppressionData, bLessLethal);
}

void ASWATCharacter::ReturnSuppressiveFire(const FSuppressionData& SuppressionData, bool bIsUsingLessLethal)
{
	if (GetCyberneticsController() && !GetCyberneticsController()->GetTrackedTarget())
	{
		const FVector DirectionToHitScanStart = (GetActorLocation() - SuppressionData.Origin).GetSafeNormal2D();
		const float DotProduct = FVector::DotProduct(SuppressionData.Direction, DirectionToHitScanStart);

		if (DotProduct > 0.95f && !bIsUsingLessLethal)
		{
			if (FVector::Distance(SuppressionData.Origin, GetActorLocation()) < 2000.0f)
			{
				GetCyberneticsController()->GetCombatActivity()->ScriptedFireAtActor(SuppressionData.Instigator, 0.1f, true, 2.0f);
			}
		}
	}
}

bool ASWATCharacter::IsSecured_Implementation() const
{
	return true;
}

bool ASWATCharacter::CanBeSecured_Implementation() const
{
	return false;
}

bool ASWATCharacter::CanBeSecuredByTrailers_Implementation() const
{
	return IsDeadOrUnconscious() && !IsHidden();
}

void ASWATCharacter::Secure_Implementation(AReadyOrNotCharacter* InInstigator)
{
}

void ASWATCharacter::Surrender()
{
	// swat cant surrender...
}

void ASWATCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	MovementStyleData.LoweredTwoHandedMoveStyle = "male01_swat_rifle";
	MovementStyleData.RaisedTwoHandedMoveStyle = "male01_swat_rifle_strafe";

	CurrentGestureInterval = FMath::FRandRange(16.0f, 29.0f);

	// don't optimize swat as they may need to perform actions etc
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
}

void ASWATCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	SCOPE_CYCLE_COUNTER(STAT_SwatCharacterTick);

	if (bDeactivated)
		return;

	if (TimeSinceFriendlyShotAtMe < 86400.0f) // 24 hrs
	{
		TimeSinceFriendlyShotAtMe += DeltaSeconds;
	}

	#if !UE_BUILD_SHIPPING
	if (RosterCharacter && CVarSwatRosterDebug.GetValueOnAnyThread() != 0)
	{
		FString RosterDebugText = FString::Printf(TEXT("%s %s (%.0f%%)"), *RosterCharacter->FirstName.ToString(), *RosterCharacter->LastName.ToString(), RosterCharacter->StressLevel * 100.0f);
		DrawDebugString(GetWorld(), GetActorLocation() + FVector::UpVector * 90.0f, RosterDebugText, nullptr, FColor::Green, 0.0f, true);
	}
	#endif
	
	// Don't do anything if not possessed by a controller
	if (!GetCyberneticsController())
	{
		if (PlayerMarkerComponent)
			PlayerMarkerComponent->HideObjectiveMarker();
		if (bHasBeenReported)
			SetActorTickEnabled(false);
		
		return;
	}

	CurrentGestureInterval = FMath::Clamp(CurrentGestureInterval - DeltaSeconds, 0.0f, CurrentGestureInterval);
	
	if (CurrentGestureInterval <= 0.0f)
	{
		PlayGestureAnimation();
		CurrentGestureInterval = FMath::FRandRange(16.0f, 29.0f);
	}

	if (GetCyberneticsController()->GetCurrentActivity<UDoorInteractionActivity>() == nullptr)
	{
		SCOPE_CYCLE_COUNTER(STAT_ViewBlockTest);
		
		const FVector StartTrace = GetActorLocation();
		const FVector EndTrace = GetActorLocation() + GetActorForwardVector() * 3000.0f;
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

		if (GetWorld()->IsTraceHandleValid(ViewBlockTraceHandle, false))
		{
			bViewBlockedByOtherSwat = false;
				
			FTraceDatum OutTraceData;
			if (GetWorld()->QueryTraceData(ViewBlockTraceHandle, OutTraceData))
			{
				if (OutTraceData.OutHits.Num() > 0)
				{
					FHitResult& Hit = OutTraceData.OutHits[0];
					if (AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(Hit.GetActor()))
					{
						if (Character->IsOnSWATTeam() && Character->IsActive() && !USWATManager::Get(this)->IsCharacterKnownEnemy(Character))
						{
							bViewBlockedByOtherSwat = true;
						}
					}
				}
			}
		}
		else
		{
			ViewBlockTraceHandle = GetWorld()->AsyncLineTraceByObjectType(EAsyncTraceType::Single, StartTrace, EndTrace, ObjectQueryParams, GetCollisionQueryParameters());
		}
		
		//ULog::Bool(bViewBlockedByOtherSwat, GetName() + ": ");
		
		if (!bViewBlockedByOtherSwat)
		{
			LOCAL_PLAYER;
			if (LocalPlayer && !USWATManager::Get(this)->IsCharacterKnownEnemy(LocalPlayer))
			{
				if (FVector::DotProduct(GetActorForwardVector(), (LocalPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal()) > 0.5f)
				{
					FVector Point = FMath::ClosestPointOnLine(GetActorLocation(), Rep_FocalPoint, LocalPlayer->GetActorLocation());
					float Distance = FVector::Distance(Point, LocalPlayer->GetActorLocation());
					
					if (Distance < 80.0f)
					{
						bViewBlockedByOtherSwat = true;
					}
				}
			}
		}

		if (!bViewBlockedByOtherSwat)
		{
			float ClosestDistance = FLT_MAX;
			for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
			{
				if (swat == this)
					continue;

				float Dot = FVector::DotProduct(GetActorForwardVector(), (swat->GetActorLocation() - GetActorLocation()).GetSafeNormal());
				if (Dot > 0.75f)
				{
					FVector Point = FMath::ClosestPointOnLine(GetActorLocation(), Rep_FocalPoint, swat->GetActorLocation());
					float Distance = FVector::Distance(Point, swat->GetActorLocation());
					if (Distance < ClosestDistance)
					{
						ClosestDistance = Distance;
						//bIsRight = FVector::DotProduct(GetActorRightVector(), (Point - swat->GetActorLocation()).GetSafeNormal()) > 0;
					}
				}
			}

			if (ClosestDistance < 70.0f)
			{
				bViewBlockedByOtherSwat = true;
			}
		}
	}

	if (bViewBlockedByOtherSwat)
	{
		if (IsTableMontagePlaying("tp_swat_gestures"))
		{
			StopTPMontageFromTable("tp_swat_gestures", 0.25f);
		}
	}

	bool bOverridingLean = false;
	if (!IsLowReady() && bIsStrafing)
	{
		if (UBaseActivity* Activity = GetCyberneticsController()->GetCurrentActivity())
		{
			float LeanOverride = 0.0f;
			if (Activity->GetLeanOverride(LeanOverride))
			{
				Lean(LeanOverride);
				bOverridingLean = true;
			}
		}
	}
	else
	{
		QuickLeanAmount = 0.0f;
		bIsLeaning = false;
		bFreeLeaning = false;
	}

	if (!bOverridingLean)
	{
		if (Rep_FocalPoint != FVector::ZeroVector && !IsLowReady())
		{
			SCOPE_CYCLE_COUNTER(STAT_LeanTest);
			
			const FVector StartTrace = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
			const FVector EndTrace = Rep_FocalPoint;
			FCollisionQueryParams CollisionQueryParams = GetCollisionQueryParameters();
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
			CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllEvidenceActors);

			// ignore only closed doors
			TArray<AActor*> ClosedDoors;
			ClosedDoors.Reserve(30);
			for (ADoor* Door : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors)
			{
				if (Door->IsClosed())
				{
					ClosedDoors.Add(Door);
				}
			}
			
			CollisionQueryParams.AddIgnoredActors(ClosedDoors);
			
			if (GetWorld()->IsTraceHandleValid(LeanTraceHandle, false))
			{
				FTraceDatum OutTraceData;
				if (GetWorld()->QueryTraceData(LeanTraceHandle, OutTraceData))
				{
					if (OutTraceData.OutHits.Num() == 0)
					{
						QuickLeanAmount = 0.0f;
						bFreeLeaning = false;
					}
					else
					{
						FHitResult& Hit = OutTraceData.OutHits[0];
						
						if (Hit.Distance < 1000.0f &&
							FVector::DotProduct(GetActorForwardVector(), (Rep_FocalPoint - GetActorLocation()).GetSafeNormal2D()) < 0.9f)
						{
							bool bLeanRight = FVector::DotProduct(GetActorRightVector(), (Rep_FocalPoint - GetActorLocation()).GetSafeNormal2D()) > 0.0f;
							// is something to the right?
							if (bLeanRight)
							{
								if (GetWorld()->LineTraceTestByChannel(StartTrace, StartTrace + GetActorRightVector() * 100.0f, ECC_Visibility, CollisionQueryParams))
									bLeanRight = false;
							}
							else
							{
								if (GetWorld()->LineTraceTestByChannel(StartTrace, StartTrace + GetActorRightVector() * -100.0f, ECC_Visibility, CollisionQueryParams))
									bLeanRight = true;
							}
							
							bFreeLeaning = false;
							Lean(bLeanRight ? -1.0f : 1.0f);
						}
						else
						{
							Lean(0.0f);
						}
					}
				}
			}
			else
			{
				LeanTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, StartTrace, EndTrace, ECC_Visibility, CollisionQueryParams);
			}

			/*
			if (GetWorld()->LineTraceSingleByChannel(OutHit, StartTrace, EndTrace, ECC_Visibility, CollisionQueryParams))
			{
				if (OutHit.Distance < 1000.0f &&
					FVector::DotProduct(GetActorForwardVector(), (Rep_FocalPoint - GetActorLocation()).GetSafeNormal2D()) < 0.9f)
				{
					bool bLeanRight = FVector::DotProduct(GetActorRightVector(), (Rep_FocalPoint - GetActorLocation()).GetSafeNormal2D()) > 0.0f;
					// is something to the right?
					if (bLeanRight)
					{
						if (GetWorld()->LineTraceTestByChannel(StartTrace, StartTrace + GetActorRightVector() * 100.0f, ECC_Visibility, CollisionQueryParams))
							bLeanRight = false;
					}
					else
					{
						if (GetWorld()->LineTraceTestByChannel(StartTrace, StartTrace + GetActorRightVector() * -100.0f, ECC_Visibility, CollisionQueryParams))
							bLeanRight = true;
					}
					
					bFreeLeaning = false;
					Lean(bLeanRight ? -1.0f : 1.0f);
				}
				else
				{
					Lean(0.0f);
				}
			}
			else
			{
				QuickLeanAmount = 0.0f;
				bFreeLeaning = false;
			}
			*/
		}
		else
		{
			QuickLeanAmount = 0.0f;
			bFreeLeaning = false;
		}
	}
	
	if (const AReadyOrNotCharacter* SquadLeader = USWATManager::Get(this)->SquadLeader)
	{
		// match the flashlight state of the squad leader
		TimeUntilNextFlashlightCheck -= DeltaSeconds;
		if (TimeUntilNextFlashlightCheck <= 0.0f)
		{
			TimeUntilNextFlashlightCheck = FMath::RandRange(1, 5);
			
			ABaseMagazineWeapon* Leadbmw = SquadLeader->GetEquippedWeapon();
			ABaseMagazineWeapon* Mybmw = GetEquippedWeapon();
			if (Leadbmw && Mybmw)
			{
				if (Leadbmw->GetLightAttachment() && Mybmw->GetLightAttachment())
				{
					if (Mybmw->GetLightAttachment()->IsLightOn() != Leadbmw->GetLightAttachment()->IsLightOn())
					{
						Mybmw->GetLightAttachment()->ToggleLight(Leadbmw->GetLightAttachment()->IsLightOn());
					}
				}
				
				if (Leadbmw->GetLaserAttachment() && Mybmw->GetLaserAttachment())
				{
					if (Mybmw->GetLaserAttachment()->IsLaserOn() != Leadbmw->GetLaserAttachment()->IsLaserOn())
					{
						Mybmw->GetLaserAttachment()->ToggleLaser(Leadbmw->GetLaserAttachment()->IsLaserOn());
					}
				}
			}
		}
	}

	// nav mesh stuck detection
	if (AReadyOrNotCharacter* SquadLeader = USWATManager::Get(this)->GetSquadLeader())
	{
		FVector Location = GetActorLocation();
		if (!UReadyOrNotAISystem::ProjectPointToNav(GetActorLocation(), Location, FVector(75.0f, 75.0f, 150.0f)))
		{
			TimeStuck += DeltaSeconds;
			if (TimeStuck > 2.0f)
			{
				SetActorLocation(SquadLeader->GetActorLocation() + SquadLeader->GetActorForwardVector() * -100.0f, true, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		else
		{
			TimeStuck = 0.0f;
		}
	}
	
	float OverrideSpeed = 240.0f;

	bool bOverriddenMoveSpeed = false;
	if (const UBaseActivity* Activity = GetCyberneticsController()->GetCurrentActivity())
	{
		if (Activity->GetOverrideMovementSpeed(OverrideSpeed))
		{
			bOverriddenMoveSpeed = true;
			MoveStyle->SetCharacterSpeed(OverrideSpeed);
		}
	}

	if (!bOverriddenMoveSpeed)
	{
		if (UReadyOrNotPathFollowingComp* PathFollowingComp = GetCyberneticsController()->GetRONPathFollowingComp())
		{
			if (PathFollowingComp->GetPath().IsValid())
			{
				TArray<FNavPathPoint> PathPoints = PathFollowingComp->GetPath()->GetPathPoints();

				float PathLength = PathFollowingComp->GetPath()->GetLengthFromPosition(GetNavAgentLocation(), PathPoints.Num()-1);

				OverrideSpeed = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 300.0f), FVector2D(100.0f, 240.0f), PathLength);
				//LOG_NUMBER(OverrideSpeed);
			}
		}
	}
	
	const ASWATCharacter* Character = USWATManager::Get(this)->GetClosestSWATToActor(this);

	// slow down when moving on a sloped floor
	if (!GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
	{
		AReadyOrNotCharacter* Lead = nullptr;
		float Dist = 0.0f;
		float ShortestDistance = 0.0f;
		if (USWATManager* Manager = USWATManager::Get(this))
		{
			Lead = Manager->SquadLeader;

			if (Manager->FallInSwat_PathFound.Num() > 0)
			{
				ShortestDistance = Manager->FallInSwat_PathFound.begin().Value(); // first one is always the shortest as it's sorted already
			}
			
			if (const float* PathDist = Manager->FallInSwat_PathFound.Find(this)) // this is the path to the squad lead
			{
				Dist = *PathDist;
			}
		}

		bool bIsLeadFarAway = ShortestDistance > 1000.0f || Dist > 1500.0f;
		if (!Lead || !bIsLeadFarAway)
		{
			const FVector SlopeNormal = GetCharacterMovement()->CurrentFloor.HitResult.Normal;
			const float Dot = FMath::Abs(FVector::DotProduct(FVector::UpVector, SlopeNormal));
			//const bool bIsOnSlope = SlopeNormal != FVector::ZeroVector && Dot < 0.975f;
			const float SlowDownMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 0.8f), FVector2D(1.0f, 0.5f), Dot);
			OverrideSpeed *= SlowDownMultiplier;
			OverrideSpeed = FMath::Clamp(OverrideSpeed, 100.0f, 240.0f);
		}
	}
	
	MoveStyle->SetCharacterSpeed(OverrideSpeed);
	
	bool bNearAnotherSwat = false;
	if (Character)
		bNearAnotherSwat = FVector::Distance(Character->GetActorLocation(), GetActorLocation()) < 125.0f;

	const bool bAvoiding = AvoidingCharacter ||
							(GetCyberneticsController()->GetCurrentActivity<UMoveToActivity>() == GetCyberneticsController()->GetPushMoveToActivity()) ||
							bNearAnotherSwat;
	if (bAvoiding)
	{
		MoveStyle->SetCharacterSpeed(OverrideSpeed * 0.75f);
	}
}

void ASWATCharacter::Knockout(float Duration, bool bPlayVO)
{
	// policey bois can't be knocked out
}

FString ASWATCharacter::MutateVoiceline(const FString& VO)
{
	if (UReadyOrNotAISystem::WasRecentlyInCombat(30.0f))
		return VO;

	return VO + "S"; // stealth
}

bool ASWATCharacter::CanPushDoor(ADoor* Door) const
{
	if (GetCyberneticsController()->GetCurrentActivity<UDoorInteractionActivity>() ||
		GetCyberneticsController()->GetCurrentActivity<UTeamStackUpActivity>())
	{
		return false;
	}

	return Super::CanPushDoor(Door);
}

void ASWATCharacter::PlayOnShotDialogue(const bool bIsFriendly)
{
	if (IsDeadOrUnconscious())
		return;
	
	PlayRawVO(bIsFriendly ? VO_SWAT_GENERAL::CALL_FRIENDLY_FIRE : VO_SWAT_GENERAL::CALL_SHOT_AT_BY_SUSPECT);
}

bool ASWATCharacter::OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::OnTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	
	const UBulletDamageType* BulletDamage = Cast<UBulletDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
	
	UStunDamage* StunDamage = Cast<UStunDamage>(DamageEvent.DamageTypeClass->GetDefaultObject());

	if (EventInstigator)
	{
		// Voice Line reactions to being shot
		if (!IsDeadOrUnconscious())
		{
			APlayerCharacter* InstigatorCharacter = Cast<APlayerCharacter>(EventInstigator->GetPawn());		
			if (InstigatorCharacter && DamageEvent.DamageTypeClass != nullptr)
			{
				if (InstigatorCharacter->GetTeam() == ETeamType::TT_SQUAD || InstigatorCharacter->GetTeam() == ETeamType::TT_SERT_BLUE || InstigatorCharacter->GetTeam() == ETeamType::TT_SERT_RED)
				{
					if (BulletDamage || (StunDamage && StunDamage->StunType == EStunType::ST_Tased) || Cast<UPepperballDamageType>(StunDamage) || Cast<UBeanbagDamageType>(StunDamage))
					{
						OnShotResponse_Delegate.Unbind();
						OnShotResponse_Delegate.BindUFunction(this, FName("PlayOnShotDialogue"), true);

						GetWorld()->GetTimerManager().ClearTimer(OnShotResponse_Handle);
						GetWorld()->GetTimerManager().SetTimer(OnShotResponse_Handle, OnShotResponse_Delegate, OnShotResponseDelay, false);	
					}
				}
				else if (InstigatorCharacter->GetTeam() == ETeamType::TT_SUSPECT)
				{ 
					if (GetWorld()->GetTimerManager().IsTimerActive(OnShotResponse_Handle))
					{
						GetWorld()->GetTimerManager().ClearTimer(OnShotResponse_Handle);
					}
				}
			}
			else
			{
				PlayRawVO(VO_SWAT_GENERAL::CALL_PAIN_GRUNT);
			}
		}
		else
		{
			OnShotResponse_Delegate.Unbind();
			GetWorld()->GetTimerManager().ClearTimer(OnShotResponse_Handle);
		}
		
		if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(EventInstigator->GetPawn()))
		{
			AICharacter->bHasDamagedSWATTeam = true;
			AICharacter->ScoringComponent->RevokeAllPenalties();

			//#if WITH_EDITOR
			//ULog::Info(AICharacter->GetName() + " has damaged swat");
			//#endif

			// if (GetCyberneticsController())
			// {
			// 	if (UBaseCombatActivity* CombatActivity = GetCyberneticsController()->GetCombatActivity())
			// 	{
			// 		const float AccuracyPenalty = HasLineOfSightToCharacter(AICharacter) ? 1.0f : 5.0f;
			// 		CombatActivity->ScriptedFireAtActor(AICharacter, 1.0f, true, AccuracyPenalty);
			// 	}
			// }
		}
	}

	if (GetHealthComponent()->GetHealthStatus() >= EPlayerHealthStatus::HS_Healthy)
	{
		if (EventInstigator)
		{
			if (LOCAL_PLAYER)
			{
				if (EventInstigator->GetPawn() == LocalPlayer)
				{
					const FString Suffix = LocalPlayer->GetPlayerState() ? " (" + LocalPlayer->GetPlayerState()->GetPlayerName() + ")" : "";
					FText ScoreText = FText::Format(FText::FromString("{0}{1}"), AScoringManager::PENALTY_FRIENDLY_FIRE, FText::FromString(Suffix));
					ScoringComponent->GivePenalty(AScoringManager::PENALTY_FRIENDLY_FIRE, false, ScoreText);

					TimeSinceFriendlyShotAtMe = 0.0f;

					if (GetCyberneticsController())
					{
						const bool bIsStackUpOnDoor = GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>() ||
													GetCyberneticsController()->GetCurrentActivity<UDoorInteractionActivity>() ||
													GetCyberneticsController()->GetCurrentActivity<UScanDoorActivity>();
						
						if (!bIsStackUpOnDoor && !IsFullBodyMontagePlaying())
						{
							if (UBaseCombatActivity* CombatActivity = GetCyberneticsController()->GetCombatActivity())
							{
								if (!UReadyOrNotAISystem::WasRecentlyInCombat(5.0f) && TimeSinceLastYell > 5.0f)
								{
									CombatActivity->ScriptedLookAtActor(LocalPlayer, 2.0f);
								}
							}
						}
					}
				}
			}
		}
	}

	return true;
}

void ASWATCharacter::OnGameEnded_Implementation()
{
	if (IsDeadOrUnconscious())
	{
		ScoringComponent->TakeScore(FText::FromStringTable("ScoringTable", "Alive"));

		if (!HasBeenReported())
		{
			ScoringComponent->GivePenalty(AScoringManager::PENALTY_FAILED_TO_REPORT_DOWNED_OFFICER, false);
		}
	}
}

void ASWATCharacter::PlayGestureAnimation()
{
	if (IsDeadOrUnconscious())
		return;

	// do not attempt at higher velocity
	if (GetVelocity().Size() > 50.0f || bViewBlockedByOtherSwat)
	{
		StopTPMontageFromTable("tp_swat_gestures", 0.25f);
		return;
	}

	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController());
	if (CyberneticController)
	{
		if (CyberneticController->GetCurrentActivity<UDoorInteractionActivity>() ||
			CyberneticController->GetCurrentActivity<UTeamStackUpActivity>() ||
			CyberneticController->GetCurrentActivity<UScanDoorActivity>())
		{
			StopTPMontageFromTable("tp_swat_gestures", 0.25f);
			return;
		}
		
		if (CyberneticController->GetTrackedTarget() || CyberneticController->HasBeenExposedToAggressiveNoise(5.0f))
        {
			StopTPMontageFromTable("tp_swat_gestures", 0.25f);
			return;
        }
		
		// do not play gestures if we have a active tracked enemy
		if ((!CyberneticController->GetTrackedTarget() || !CyberneticController->HasBeenExposedToAggressiveNoise(10.0f)) && !IsAny3PMontageActive())
		{
			if (!IsTableMontagePlaying("tp_swat_gestures"))
			{
				PlayMontageFromTable("tp_swat_gestures");
			}
		}
	}
}

void ASWATCharacter::SetRosterCharacter(URosterCharacter* InRosterCharacter)
{
	if (!ensure(InRosterCharacter))
		return;

	RosterCharacter = InRosterCharacter;
	// if (!RosterCharacter->Character)
	// 	return;
	//
	// USkeletalMesh* RosterFaceMesh = RosterCharacter->Character->FaceMesh.LoadSynchronous();
	// if (RosterFaceMesh)
	// 	Rep_FaceMesh = RosterFaceMesh;
	//
	// USkeletalMesh* RosterBodyMesh = RosterCharacter->Character->BodyMesh.LoadSynchronous();
	// if (RosterBodyMesh)
	// 	Rep_BodyMesh = InRosterCharacter->Character->BodyMesh.LoadSynchronous();
	//
	// CharacterLookOverride = FCharacterLookOverride();
	// SpeechCharacterName = InRosterCharacter->Voice.ToString();
	// OnRep_MeshReplicated();
}

FText ASWATCharacter::GetSwatCharacterName() const
{
	if (RosterCharacter)
		return RosterCharacter->LastName;

	UCustomizationCharacter* CustomizationCharacter = Cast<UCustomizationCharacter>(Customization.Character);
	if (CustomizationCharacter)
		return CustomizationCharacter->Name;
	
	return FText::FromStringTable("SwatCommandTable", "Unknown");
}

void ASWATCharacter::StopGestureAnimation()
{
	StopTPMontageFromTable("tp_swat_gestures", 0.25f);
}
