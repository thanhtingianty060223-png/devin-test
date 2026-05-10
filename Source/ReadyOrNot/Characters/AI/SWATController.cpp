// Copyright Void Interactive, 2022

#include "SWATController.h"

#include "Commander/RosterManager.h"

#include "ReadyOrNotAIConfig.h"

#include "SWATCharacter.h"
#include "Actors/StackUpActor.h"

#include "Actors/Gameplay/IncapacitatedHuman.h"
#include "Actors/Items/BallisticsShield.h"
#include "Info/SWATManager.h"

#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/DeployChemlightActivity.h"
#include "Info/Activities/DeployGrenadeAtLocationActivity.h"
#include "Info/Activities/EngageTargetLessLethalActivity.h"
#include "Info/Activities/PickUpCharacterActivity.h"
#include "Info/Activities/ScanDoorActivity.h"
#include "Info/Activities/SearchLandmarkActivity.h"
#include "Info/Activities/ToggleDoorActivity.h"
#include "Info/Activities/TrailerSearchAndSecureActivity.h"
#include "Info/Activities/Team/ArrestTargetActivity.h"
#include "Info/Activities/Team/CollectEvidenceActivity.h"
#include "Info/Activities/Team/DeployWedgeActivity.h"
#include "Info/Activities/Team/DisarmDoorTrapActivity.h"
#include "Info/Activities/Team/ReportTargetActivity.h"
#include "Info/Activities/Team/HoldActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/DisarmStandaloneTrapActivity.h"
#include "Info/Activities/Team/SearchAndSecureActivity.h"
#include "Info/Activities/Team/TeamFallinActivity.h"
#include "Info/Activities/Team/DoorBreachActivity.h"
#include "Info/Activities/Team/LockPickDoorActivity.h"
#include "Info/Activities/Team/MirrorUnderDoorActivity.h"
#include "Info/Activities/Team/TeamCoverAreaActivity.h"

#include "Senses/ReadyOrNotAISense_Sight.h"

TAutoConsoleVariable<int32> CVarRonDisplaySwatDebug(TEXT("SWAT.Debug"), 1, TEXT("Draw debug information of the Swat AI"));

float ASWATController::GetReactionTime(const EActorSenseType& SenseType) const
{
	switch (SenseType)
	{
		case EActorSenseType::Sight:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SwatSightStimulusReactionTime", 0.1f), 0.0f, 0.5f); // max half a sec reaction time so they're not too dumb
		case EActorSenseType::Sound:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SwatSoundStimulusReactionTime", 0.2f), 0.0f, 0.5f);
		case EActorSenseType::Damage:	return FMath::Clamp(AI_CONFIG_GET_FLOAT("SwatDamageStimulusReactionTime", 0.05f), 0.0f, 0.5f);
		default: return 0.1f;
	}
}

void ASWATController::BeginPlay()
{
	Super::BeginPlay();
	
	PrimaryActorTick.TickInterval = 0.025f; // slightly faster than the suspects
	
	BreachAndClearActivity = NewObject<UTeamBreachAndClearActivity>(this, UTeamBreachAndClearActivity::StaticClass());
	BreachAndClearActivity->bNoResetDataOnFinish = true;

	DeployChemlightActivity = NewObject<UDeployChemlightActivity>(this, UDeployChemlightActivity::StaticClass());

	ReportTargetActivity = NewObject<UReportTargetActivity>(this, UReportTargetActivity::StaticClass());

	CollectEvidenceActivity = NewObject<UCollectEvidenceActivity>(this, UCollectEvidenceActivity::StaticClass());
	
	DisarmStandaloneTrapActivity = NewObject<UDisarmStandaloneTrapActivity>(this, UDisarmStandaloneTrapActivity::StaticClass());
	
	DeployGrenadeAtLocationActivity = NewObject<UDeployGrenadeAtLocationActivity>(this, UDeployGrenadeAtLocationActivity::StaticClass());
	
	ArrestTargetActivity = NewObject<UArrestTargetActivity>(this, UArrestTargetActivity::StaticClass());
	
	StackUpActivity = NewObject<UTeamStackUpActivity>(this, UTeamStackUpActivity::StaticClass());
	FallinActivity = NewObject<UTeamFallinActivity>(this, UTeamFallinActivity::StaticClass());
	HoldActivity = NewObject<UHoldActivity>(this, UHoldActivity::StaticClass());
	
	KickDoorActivity = NewObject<UKickDoorActivity>(this, UKickDoorActivity::StaticClass());
	C2DoorActivity = NewObject<UC2DoorActivity>(this, UC2DoorActivity::StaticClass());
	ShotgunDoorActivity = NewObject<UShotgunDoorActivity>(this, UShotgunDoorActivity::StaticClass());
	RamDoorActivity = NewObject<URamDoorActivity>(this, URamDoorActivity::StaticClass());
	ThrowGrenadeThroughDoorActivity = NewObject<UThrowGrenadeThroughDoorActivity>(this, UThrowGrenadeThroughDoorActivity::StaticClass());
	
	SearchAndSecureActivity = NewObject<USearchAndSecureActivity>(this, USearchAndSecureActivity::StaticClass());
	
	MirrorUnderDoorActivity = NewObject<UMirrorUnderDoorActivity>(this, UMirrorUnderDoorActivity::StaticClass());
	DisarmDoorTrapActivity = NewObject<UDisarmDoorTrapActivity>(this, UDisarmDoorTrapActivity::StaticClass());
	DeployWedgeActivity = NewObject<UDeployWedgeActivity>(this, UDeployWedgeActivity::StaticClass());
	LockPickDoorActivity = NewObject<ULockPickDoorActivity>(this, ULockPickDoorActivity::StaticClass());
	
	ScanDoorActivity = NewObject<UScanDoorActivity>(this, UScanDoorActivity::StaticClass());
	
	LaunchGrenadeThroughDoorActivity = NewObject<ULaunchGrenadeThroughDoorActivity>(this, ULaunchGrenadeThroughDoorActivity::StaticClass());

	CoverAreaActivity = NewObject<UTeamCoverAreaActivity>(this, UTeamCoverAreaActivity::StaticClass());
	
	SearchLandmarkActivity = NewObject<USearchLandmarkActivity>(this, USearchLandmarkActivity::StaticClass());

	PickUpCharacterActivity = NewObject<UPickUpCharacterActivity>(this, UPickUpCharacterActivity::StaticClass());
	
	TrailerSearchAndSecureActivity = NewObject<UTrailerSearchAndSecureActivity>(this, UTrailerSearchAndSecureActivity::StaticClass());

	ToggleDoorActivity = NewObject<UToggleDoorActivity>(this, UToggleDoorActivity::StaticClass());

	EngageLessLethalActivity = NewObject<UEngageTargetLessLethalActivity>(this, UEngageTargetLessLethalActivity::StaticClass());
}

void ASWATController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsValid(GetCharacter()))
		return;

	// is standing in front of a door? if so, move out of way
	if (!CurrentActivity && !GetCharacter()->GetEquippedItem<ABallisticsShield>())
	{
		if (const AThreatAwarenessActor* NearestThreat = TargetingComponent->GetNearestThreat())
		{
			ADoor* D = NearestThreat->DoorThreat;
			if (!D)
			{
				if (TargetingComponent->GetTrackingType() == ETargetingCompTracking::TCT_TrackingThreatAwarenessActor)
				{
					if (const FLookAtPoint* P = TargetingComponent->CurrentThreatLookAtPoint)
					{
						D = P->LinkedDoor;
					}
				}
			}

			if (!D)
			{
				D = Cast<ADoor>(GetFocusActor());
			}

			TArray<ADoor*, TInlineAllocator<64>> Doors;
			
			if (const FRoom* CurrentRoom = CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(NearestThreat->OwningRoom))
			{
				Doors.Add(D);
				Doors.Append(CurrentRoom->AdditionalRootDoors);
			}
				
			for (const ADoor* Door : Doors)
			{
				if (Door && Door->bHasFrame)
				{
					FVector BestLocation = FVector::ZeroVector;
					
					FTransform Transform;
					Transform.SetLocation(Door->GetDoorMidLocation());
					Transform.SetRotation(Door->GetActorRotation().Quaternion());

					const float DoorWidth = Door->GetDoorSize().Y - 5.0f;
					
					FVector Extent = FVector(300.0f, DoorWidth, 120.0f);
					
					if (Door->IsDoorwayOnly())
					{
						Extent.X = 700.0f;
					}

					//DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 1.0f);

					const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->GetActorLocation(), Transform, Extent);

					if (bInsideDoorThreshold)
					{
						EDoorRoomPosition RoomPosition;

						const bool bFront = Door->IsActorInFrontOfDoorway(GetCharacter());
						if (bFront)
						{
							RoomPosition = Door->BackRoomPosition;
						}
						else
						{
							RoomPosition = Door->FrontRoomPosition;
						}

						if (RoomPosition != EDoorRoomPosition::Hallway && RoomPosition != EDoorRoomPosition::HallwayLeft && RoomPosition != EDoorRoomPosition::HallwayRight) // can't really move out of way of a hallway
						{
							const bool bIsOneSidedPosition = RoomPosition == EDoorRoomPosition::CornerLeft ||
															 RoomPosition == EDoorRoomPosition::CornerRight ||
															 RoomPosition == EDoorRoomPosition::HallwayLeft ||
															 RoomPosition == EDoorRoomPosition::HallwayRight;

							if (bIsOneSidedPosition)
							{
								const TArray<AStackUpActor*>* StackUpActors = nullptr;
								if (RoomPosition == EDoorRoomPosition::CornerLeft ||
									RoomPosition == EDoorRoomPosition::HallwayLeft)
								{
									StackUpActors = bFront ? &Door->FrontRightStackUpPoints : &Door->BackLeftStackUpPoints;
								}
								else if (RoomPosition == EDoorRoomPosition::CornerRight ||
										 RoomPosition == EDoorRoomPosition::HallwayRight)
								{
									StackUpActors = bFront ? &Door->FrontLeftStackUpPoints : &Door->BackRightStackUpPoints;
								}

								if (StackUpActors)
								{
									if (const AStackUpActor* ClosestStackUp = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation<AStackUpActor>(GetCharacter()->GetActorLocation(), *StackUpActors))
									{
										BestLocation = ClosestStackUp->GetActorLocation();
									}
								}
							}
							else
							{
								if (Door->IsActorRightOfDoorway(GetCharacter()))
									BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * 120.0f;
								else
									BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * -120.0f;
							}
							
							GiveMoveTo(BestLocation);
						}
					}
				}
			}
		}
	}
}

void ASWATController::ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor)
{
}

bool ASWATController::CanSpotCharacter(AReadyOrNotCharacter* SensedCharacter) const
{
	if (const ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(SensedCharacter))
	{
		if (CyberneticCharacter->IsSuspect() && CyberneticCharacter->IsPlayingDead())
		{
			return false;
		}
	}

	return Super::CanSpotCharacter(SensedCharacter);
}

void ASWATController::OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnSeenCharacter(SensedCharacter, Stimulus);
	
	if (SensedCharacter->IsSuspect())
	{
		if (!SensedCharacter->bHasBeenSpottedBySWAT)
		{
			NewEnemies++;
		}
	}
	else if (SensedCharacter->IsCivilian())
	{
		if (!SensedCharacter->bHasBeenSpottedBySWAT)
		{
			NewNeutrals++;
		}
	}

	for (AReadyOrNotCharacter* Friendly : TargetingComponent->KnownFriendlies)
	{
		if (Friendly->IsActive() && Cast<ACyberneticCharacter>(Friendly))
		{
			if (ACyberneticController* Controller = Cast<ACyberneticCharacter>(Friendly)->GetCyberneticsController())
			{
				if (IsCharacterEnemy_Implementation(SensedCharacter))
					Controller->GetTargetingComp()->AddKnownEnemy(SensedCharacter, true);
				
				for (AReadyOrNotCharacter* Enemy : TargetingComponent->KnownEnemies)
				{
					Controller->GetTargetingComp()->AddKnownEnemy(Enemy, true);
				}
			}
		}
	}

	if (!UInteractionsData::IsPairedInteractionPlayingOn(GetCharacter()) && !GetCharacter()->IsAnimationBlocking())
	{
		if (SensedCharacter->bArrestComplete || SensedCharacter->IsDeadOrUnconscious() || SensedCharacter->IsIncapacitated())
		{
			if (!SensedCharacter->bHasBeenReported && !USWATManager::Get(this)->ReportQueue.Contains(SensedCharacter))
			{
				GetCharacter()->Server_ReportTarget_Implementation(SensedCharacter);	
				//USWATManager::Get(this)->ReportQueue.Add(SensedCharacter, GetCharacter<ASWATCharacter>());
			}
		}
	}
	
	SensedCharacter->bHasBeenSpottedBySWAT = true;
}

void ASWATController::OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnHeardCharacter(SensedCharacter, Stimulus);
	
	AddExposedToStimulusTag(Stimulus.Tag, Stimulus.StimulusLocation, SensedCharacter->IsOnSWATTeam(), SensedCharacter);
	
	if (!LastHeardActorTime.Find(SensedCharacter) && SensedCharacter->IsSuspect())
	{
		// minimum of 1 LOS hearing check per second
		if (LineOfSightTo(SensedCharacter))
		{
			SpottedEnemy(SensedCharacter);
		}
		
		LastHeardActorTime.Add(SensedCharacter, 1.0f);
	}
}

void ASWATController::OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus)
{
	Super::OnDamagedByCharacter(SensedCharacter, Stimulus);
	
	if (SensedCharacter->IsSuspect()) 
	{
		if (GetCharacter()->HasLineOfSightToCharacter(SensedCharacter))
		{
			DamagedBy.Add(SensedCharacter, 0.0f);
			DamagedByLocation.Add(SensedCharacter, SensedCharacter->GetActorLocation());
			GetRONPerceptionComp()->RegisterStimulus(SensedCharacter, FAIStimulus(*UReadyOrNotAISense_Sight::StaticClass()->GetDefaultObject<UReadyOrNotAISense_Sight>(), 1.0f, SensedCharacter->GetActorLocation(), GetCharacter()->GetActorLocation()));
			SpottedEnemy(SensedCharacter);
		}
	}
}

void ASWATController::OnSeenIncapHuman(AIncapacitatedHuman* IncapHuman)
{
	if (IncapHuman->HasBeenReported() || USWATManager::Get(this)->ReportQueue.Contains(IncapHuman))
	{
		return;
	}

	//GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_INCAPACITATED_TARGET);

	USWATManager::Get(this)->ReportQueue.Add(IncapHuman, GetCharacter<ASWATCharacter>());
}

void ASWATController::OnTrackedTargetKilled(AReadyOrNotCharacter* Insitgator, AReadyOrNotCharacter* KilledCharacter)
{
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ASWATController::RespondToAIKilled, KilledCharacter), FMath::RandRange(0.5f, 2.0f));
}

void ASWATController::OnTrackedTargetIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter)
{
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ASWATController::RespondToAIIncapacitated, IncapacitatedCharacter), FMath::RandRange(0.5f, 2.0f));
}

void ASWATController::OnTrackedTargetExitedSurrender(ACyberneticCharacter* InCharacter, ESurrenderExitType ExitType)
{
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ASWATController::RespondToAISurrenderExit, InCharacter, ExitType), FMath::RandRange(0.1f, 0.5f));
}

void ASWATController::RespondToAIKilled(AReadyOrNotCharacter* AI)
{
	if (AI->IsSuspect())
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SUSPECT_KILLED);
	}
	else
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN);
	}
}

void ASWATController::RespondToAIIncapacitated(AReadyOrNotCharacter* AI)
{
	if (AI->IsSuspect())
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SUSPECT_KILLED);
	}
	else
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN);
	}
}

void ASWATController::RespondToAISurrenderExit(ACyberneticCharacter* AI, ESurrenderExitType ExitType)
{
	if (AI->IsDeadOrUnconscious() || AI->IsIncapacitated())
		return;
	
	if (ExitType == ESurrenderExitType::Gun)
	{
		if (USWATManager* Manager = USWATManager::Get(this))
		{
			Manager->PlaySpeechWithSharedCooldown(VO_SWAT_GENERAL::CALL_SUSPECT_SURRENDER_GUN_EXIT, GetCharacter());
		}
	}
	else if (ExitType == ESurrenderExitType::Knife)
	{
		if (USWATManager* Manager = USWATManager::Get(this))
		{
			Manager->PlaySpeechWithSharedCooldown(VO_SWAT_GENERAL::CALL_SUSPECT_SURRENDER_KNIFE_EXIT, GetCharacter());
		}
	}
}

bool ASWATController::IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	return InCharacter->IsCivilian();
}

bool ASWATController::IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	if (IsCharacterKnownEnemy(InCharacter))
		return true;
	
	return InCharacter->IsSuspect();
}

bool ASWATController::IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const
{
	if (IsCharacterKnownEnemy(InCharacter))
		return false;
	
	return InCharacter->IsOnSWATTeam();
}

#if !UE_BUILD_SHIPPING
void ASWATController::DisplayAIDebugInfo(float DeltaTime)
{
	if (CVarRonDisplaySwatDebug.GetValueOnAnyThread() == 0)
		return;
		
	FString DebugMessage = GetCharacter()->GetName() + " | " + FString::Printf(TEXT("Tick Rate: %.3f"), PrimaryActorTick.TickInterval);
	
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += RON_ENUM_TO_STRING(ESquadPosition, GetCharacter()->GetSquadPosition());
	//DebugMessage += " | " + FString("Tick Rate: ") + FString::SanitizeFloat(PrimaryActorTick.TickInterval);
	DebugMessage += LINE_TERMINATOR;
	
	if (TargetingComponent->GetTrackingType() == ETargetingCompTracking::TCT_TrackingVisibleTarget && GetTrackedTarget())
		DebugMessage += "Tracking: " + GetNameSafe(GetTrackedTarget());
	else
		DebugMessage += "Tracking: " + RON_ENUM_TO_STRING(ETargetingCompTracking, TargetingComponent->GetTrackingType());

	if (CurrentActivity)
	{
		if (TargetingComponent->GetTrackingType() == ETargetingCompTracking::TCT_TrackingActivity)
		{
			FVector FocalPoint;
			CurrentActivity->OverrideFocalPoint(FocalPoint);
			DebugMessage += "\n\t" + GetNameSafe(CurrentActivity) + ": " + FocalPoint.ToCompactString();
		}
	}
	
	//DebugMessage += LINE_TERMINATOR;
	//DebugMessage += "Move Style: " + GetCharacter()->MoveStyle->ActiveMoveStyle.Name.ToString();
	
	DebugMessage += "\nLow Ready: " + FString(GetCharacter()->IsLowReady() ? "true" : "false") + " | " + "Strafing: " + FString(GetCharacter()->IsStrafing() ? "true" : "false");;
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += "Nearest Threat: " + GetNameSafe(TargetingComponent->GetNearestThreat());
	
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += "Turn In Place: " + FString(GetCharacter()->bDisableTurnInPlace ? "disabled" : "enabled");
	
	DebugMessage += "\n--------------\n";
	DebugMessage += "Current Activity: " + GetNameSafe(CurrentActivity);
	if (CurrentActivity)
	{
		FString CombatMoveDebugInfo;
		CurrentActivity->GatherDebugString(CombatMoveDebugInfo);
		
		DebugMessage += CombatMoveDebugInfo;
	}
	
	if (CombatActivity)
	{
		FString CombatMoveDebugInfo;
		CombatActivity->GatherDebugString(CombatMoveDebugInfo);
		
		DebugMessage += "\n--------------";
		DebugMessage += CombatMoveDebugInfo;
	}
	
	if (UBaseCombatMoveActivity* CombatMoveActivity = CombatActivity->GetCombatMoveActivity())
	{
		FString CombatMoveDebugInfo;
		CombatMoveActivity->GatherDebugString(CombatMoveDebugInfo);
		
		DebugMessage += "\n--------------\n";
		DebugMessage += CombatMoveDebugInfo;
	}
	
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const float Distance = FVector::Distance(CameraManager->GetCameraLocation(), GetCharacter()->GetActorLocation());
		if (Distance < 3000.0f)
		{
			DrawDebugString(GetWorld(), GetCharacter()->GetMesh()->GetBoneLocation("spine_3"), DebugMessage, nullptr, FColor::White, 0.0f, true);
			
			if (URosterCharacter* RosterCharacter = GetCharacter<ASWATCharacter>()->GetRosterCharacter())
			{
				FString RosterDebugText = FString::Printf(TEXT("%s %s (%.0f%%)"), *RosterCharacter->FirstName.ToString(), *RosterCharacter->LastName.ToString(), RosterCharacter->StressLevel * 100.0f);
				DrawDebugString(GetWorld(), GetCharacter()->GetActorLocation() + FVector::UpVector * 90.0f, RosterDebugText, nullptr, FColor::Green, 0.0f, true);
			}
		}
	}
}
#endif
