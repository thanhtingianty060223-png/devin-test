// Void Interactive, 2020

#include "TargetingComponent.h"

#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "Actors/ThreatAwarenessActor.h"
#include "Actors/Door.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/AI/SWATCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/SWATManager.h"

#include "NavigationSystem.h"
#include "Navigation/ReadyOrNotNavQueries.h"

#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotPathFollowingComp.h"
#include "Actors/BaseGrenade.h"
#include "Actors/InterestOverrideZone.h"
#include "Actors/Items/MeleeWeapon.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/TakeHostageActivity.h"
#include "Info/Activities/CombatMove/ChargeCombatMove.h"
#include "Info/Activities/CombatMove/FlankingCombatMove.h"
#include "Info/Activities/CombatMove/FleeingCombatMove.h"
#include "Info/Activities/Team/ArrestTargetActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/MoveToExitActivity.h"
#include "Perception/AISense_Hearing.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Targeting Component Tick"), STAT_TargetingCompTick, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Update Characters Seen Map"), STAT_TargetingComp_UpdateCharactersSeenMap, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Cache Threats"), STAT_TargetingComp_CacheThreats, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Update Preferred Target"), STAT_TargetingComp_UpdatePreferredTarget, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Find Focus Target"), STAT_TargetingComp_FindFocusTarget, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Points Of Interest"), STAT_TargetingComp_TrackPointsOfInterest, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Cache LOS Targets (Async)"), STAT_TargetingComp_CacheLOSTargets, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Smoke Grenade Detection"), STAT_TargetingComp_SmokeDetection, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Calculate Threat"), STAT_TargetingComp_CalculateThreat, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Calculate Exposure"), STAT_TargetingComp_CalculateExposure, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Bone Targeting"), STAT_TargetingComp_BoneTargeting, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Update Head Focal Point"), STAT_TargetingComp_UpdateHeadFocalPoint, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Known AI Array Filter"), STAT_TargetingComp_KnownAIArrayFilter, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Editor Stuff"), STAT_TargetingComp_EditorStuff, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Find Nearest Threat"), STAT_TargetingComp_FindNearestThreat, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Nearest Door"), STAT_TargetingComp_TrackNearestDoor, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Threat Actors"), STAT_TargetingComp_TrackThreatAwarenessActors, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Stair Threat Actors"), STAT_TargetingComp_TrackStairThreatAwarenessActors, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Find Awareness Path"), STAT_TargetingComp_FindAwarenessPath, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Visible Neutrals"), STAT_TargetingComp_TrackVisibleNeutrals, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Suspect Points Of Interest"), STAT_TargetingComp_TrackSuspectPointsOfInterest, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Civilian Points Of Interest"), STAT_TargetingComp_TrackCivilianPointsOfInterest, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Enemy Last Known Position"), STAT_TargetingComp_TrackEnemyLastKnownPosition, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Move Vector"), STAT_TargetingComp_TrackMoveVector, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Activity Related Points"), STAT_TargetingComp_TrackActivityRelatedPoints, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Noise Related Events"), STAT_TargetingComp_TrackNoiseRelatedEvents, STATGROUP_TargetingComponent);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Track Montage Position"), STAT_TargetingComp_TrackMontagePosition, STATGROUP_TargetingComponent);

TAutoConsoleVariable<int32> CVarRonDrawSwatQuadrantCoverage(TEXT("a.RonDrawSwatQuadrantCoverage"), 0, TEXT("0 = No draw quadrant coverage, 1 = Draw quadrant coverage"));
TAutoConsoleVariable<int32> CVarRonTrackDebugActor(TEXT("a.RonTrackDebugActor"), 0, TEXT("1 = track debug actor with tag (TestTrackingActor)"));
TAutoConsoleVariable<int32> CVarDrawBoneTargeting(TEXT("DrawBoneTargeting"), 0, TEXT("0 = Dont draw bone targeting. 1 = Draw bone targeting debug info"));
TAutoConsoleVariable<int32> CVarRonTrackDebugTurns(TEXT("a.RonTrackDebugTurns"), 0, TEXT(""));

UTargetingComponent::UTargetingComponent()
{
	// Tick will be called at the end of ACyberneticController::Tick, to ensure correct tick order
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 1.0f;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	PathAwarenessSearchTimeout.Add(EPathedAwareness::PA_Noise);
	PathAwarenessSearchTimeout.Add(EPathedAwareness::PA_LastKnownEnemyPosition);
	PathAwarenessSearchTimeout.Add(EPathedAwareness::PA_ActivityLocation);

	TimeSinceLastSeenTarget = FLT_MAX;
	
	BoneTargetZones.Empty();
	
	TArray<FString> BoneZones = AI_CONFIG_GET_STRING_ARRAY("BoneTargetZones");

	// Failsafe
	if (BoneZones.Num() == 0)
	{
		const TArray<FName> Zone1 = {"upperarm_RI", "upperarm_LE", "calf_LE", "calf_RI"};
		const TArray<FName> Zone2 = {"spine_2", "spine_1", "upperarm_RI", "upperarm_LE", "calf_LE", "calf_RI"};
		const TArray<FName> Zone3 = {"head", "spine_3", "spine_2", "spine_1", "upperarm_RI", "upperarm_LE", "calf_LE", "calf_RI"};
		
		BoneTargetZones.Add(Zone1);
		BoneTargetZones.Add(Zone2);
		BoneTargetZones.Add(Zone3);

		BonesToTarget = Zone1; // Default to zone 1
	}
	else
	{
		for (FString Line : BoneZones)
		{
			// Split them up
			TArray<FString> Bones;
			Line.ParseIntoArray(Bones, TEXT(","));

			// Convert them to FNames
			TArray<FName> BonesAsNames;
			for (FString& Bone : Bones)
			{
				Bone.RemoveSpacesInline();
				BonesAsNames.Add(*Bone);
			}

			BoneTargetZones.Add(BonesAsNames);
		}

		BonesToTarget = BoneTargetZones[0]; // Default to zone 1
	}
}

float UTargetingComponent::CalculateExposure(AReadyOrNotCharacter* Observer, AReadyOrNotCharacter* Target) const
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_CalculateExposure);
	
	if (!Observer || !Target)
		return 0.0f;

	constexpr uint8 TraceTargets = 6;
	
	FName Bones[TraceTargets];
	Bones[0] = "head";
	Bones[1] = "spine_2";
	Bones[2] = "hand_LE";
	Bones[3] = "hand_RI";
	Bones[4] = "foot_LE";
	Bones[5] = "foot_RI";
	
	bool HitResults[TraceTargets];
	for (uint8 i = 0; i < TraceTargets; ++i)
	{
		HitResults[i] = GetWorld()->LineTraceTestByChannel(Target->GetMesh()->GetBoneLocation(Bones[i]), Observer->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f), ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Observer, Target));
	}

	float Exposure = 0.0f;
	for (const bool bHit : HitResults)
	{
		if (!bHit)
		{
			Exposure += 1.0f/(float)TraceTargets;
		}
	}

	return Exposure;
}

void UTargetingComponent::SetMontageFocalPoint(UAnimMontage* Montage, const FVector FocalPoint)
{
	MontageFocalAnim = Montage;
	MontageFocalPoint = FocalPoint;
}

void UTargetingComponent::OnTrackedTargetKilled(AReadyOrNotCharacter* Instigator, AReadyOrNotCharacter* KilledCharacter)
{
	GetOwningController()->OnTrackedTargetKilled(Instigator, KilledCharacter);
}

void UTargetingComponent::OnTrackedTargetIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter)
{
	GetOwningController()->OnTrackedTargetIncapacitated(IncapacitatedCharacter);
}

void UTargetingComponent::OnTrackedTargetExitedSurrender(ACyberneticCharacter* Character, ESurrenderExitType ExitType)
{
	GetOwningController()->OnTrackedTargetExitedSurrender(Character, ExitType);
}

void UTargetingComponent::OnTrackedTargetStartedReloading(APlayerCharacter* Character)
{
	GetOwningController()->OnTrackedTargetStartedReloading(Character);
}

void UTargetingComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_TargetingCompTick);
	
	if (!GetOwningCharacter())
	{
		return;
	}
	
	if (GetOwningCharacter()->IsDeadOrUnconscious() || GetOwningCharacter()->IsIncapacitated() || GetOwningCharacter()->bDeactivated)
	{
		if (TrackedTarget)
		{
			TrackedTarget->AITrackingMe.Remove(GetOwningCharacter());
		}
		
		TrackedTarget = nullptr;
		TrackingType = ETargetingCompTracking::TCT_None;
		NearestThreat = nullptr;
		
		TimeTrackingTarget = 0.0f;
		
		bHasLOSToTarget = false;
		bCanSeeTarget = false;
		
		bHasLOSToLastTarget = false;
		bCanSeeLastTarget = false;

		bHasLOSToLastKnownTargetPosition = false;
		Threat = 0.0f;
		Tension = 0.0f;
		
		GetOwningCharacter()->Rep_FocalPoint = FVector::ZeroVector;
		
		GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
		GetOwningController()->ClearFocus(EAIFocusPriority::Move);
		GetOwningController()->ClearFocus(EAIFocusPriority::Montage);
		GetOwningController()->ClearFocus(EAIFocusPriority::Default);
		
		return;
	}

	const bool bRequireThreatSearch = GetOwningController()->IsMoving();

	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_KnownAIArrayFilter);
		
		KnownEnemies.RemoveAll([](const AReadyOrNotCharacter* KnownEnemy)
		{
			return !IsValid(KnownEnemy) || !KnownEnemy->GetController() || KnownEnemy->IsDeadOrUnconscious() || KnownEnemy->IsIncapacitated() || KnownEnemy->bArrestComplete || KnownEnemy->bIsBeingArrested;
		});

		KnownNeutrals.RemoveAll([](const AReadyOrNotCharacter* Neutral)
		{
			return !IsValid(Neutral) || !Neutral->GetController() || Neutral->IsDeadOrUnconscious() || Neutral->bSurrendered || Neutral->IsIncapacitated() || Neutral->bArrestComplete || Neutral->bIsBeingArrested;
		});

		KnownFriendlies.RemoveAll([](const AReadyOrNotCharacter* Friendly)
		{
			return !IsValid(Friendly) || !Friendly->GetController() || Friendly->IsDeadOrUnconscious() || Friendly->bSurrendered || Friendly->IsIncapacitated() || Friendly->bArrestComplete || Friendly->bIsBeingArrested;
		});
	}

	for (auto& It : PathAwarenessSearchTimeout)
	{
		It.Value -= DeltaTime;
	}

	for (TMap<EPathedAwareness, FPathAwarenessInfo>::TIterator It = LatestPathedAwareness.CreateIterator(); It; ++It)
	{
		It.Value().Age += DeltaTime;
		if (It.Value().Age > 30.0f || GetOwningController()->IsMoving())
		{
			It.RemoveCurrent();
			continue;
		}

		if (ADoor* Door = Cast<ADoor>(It.Value().Actor))
		{
			if (!Door->IsPointsOnOppositeSideOfDoor(It.Value().Location, It.Value().SensedFrom) || GetOwningController()->DoesPathGoThroughDoor(Door))
			{
				It.RemoveCurrent();
			}
		}
	}
	
	for (TMap<AReadyOrNotCharacter*, float>::TIterator It = CharactersSeen.CreateIterator(); It; ++It)
	{
		It.Value() += DeltaTime;
		if (It.Value() > 60.0f)
			It.RemoveCurrent();
	}
	
	TimeSinceLastHeardNoiseStimulus += DeltaTime;
	TimeSinceGotLastThreatAwarenessActor += DeltaTime;
	TimeUntilFinishedCheckingThreat = FMath::Max(TimeUntilFinishedCheckingThreat - DeltaTime, 0.0f);
	TimeSinceLastFriendlyDeath += DeltaTime;
	TimeSinceLastFriendlyTookDamage += DeltaTime;
	TimeSinceLastFriendlyStunned += DeltaTime;
	TimeSinceLastEnemyDeath += DeltaTime;
	TimeSinceLastEnemyTookDamage += DeltaTime;
	TimeSinceLastEnemyStunned += DeltaTime;
	
	AllKnownCharacters = KnownEnemies;
	AllKnownCharacters.Append(KnownFriendlies);
	AllKnownCharacters.Append(KnownNeutrals);
	
	// Can see characters in the known list?
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_UpdateCharactersSeenMap);
		
		for (AReadyOrNotCharacter* KnownCharacter : AllKnownCharacters)
		{
			bool bAnyValidStimulus = false;
			
			FActorPerceptionBlueprintInfo PerceptionOfActor;
			if (GetOwningController()->GetRONPerceptionComp()->GetActorsPerception(KnownCharacter, PerceptionOfActor))
			{
				for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
				{
					if (!Stimulus.IsValid())
						continue;
					
					if (Stimulus.IsExpired() || Stimulus.GetAge() > 1.0f)
						continue;

					if (Stimulus.Type.Name == RON_SENSE_SIGHT || Stimulus.Type.Name == RON_SENSE_DAMAGE)
					{
						bAnyValidStimulus = true;

						if (!CharactersSeen.Contains(KnownCharacter))
						{
							CharactersSeen.Add(KnownCharacter, 0.0f);
						}
						
						break;
					}
				}
			}

			if (!bAnyValidStimulus)
			{
				CharactersSeen.Remove(KnownCharacter);
			}
		}
	}

	// update head focal point (head bone rotation to look at a specific location)
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_UpdateHeadFocalPoint);

		const auto FindClosest = [&](const TArray<AReadyOrNotCharacter*>& InArray)
		{
			float ClosestDist = 500.0f;
			AReadyOrNotCharacter* ClosestCharacter = nullptr;
			for (AReadyOrNotCharacter* Target : InArray)
			{
				const float Dist = (Target->GetActorLocation() - GetOwningCharacter()->GetActorLocation()).SizeSquared();
				if (Dist < ClosestDist)
				{
					ClosestCharacter = Target;
					ClosestDist = Dist;
				}
			}

			return ClosestCharacter;
		};

		HeadTrackingLocation = FVector::ZeroVector;

		if (!GetOwningCharacter()->IsAny3PMontageActive())
		{
			if (TrackedTarget && bCanSeeTarget)
			{
				HeadTrackingLocation = TrackedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				//HeadTrackingLocation = TrackedTarget->GetMesh()->GetBoneLocation("head") + FVector(0.0f, 0.0f, 12.0f);
			}
			else if (TimeSinceLastSeenTarget < 5.0f)
			{
				HeadTrackingLocation = LastKnownTargetPosition + FVector(0.0f, 0.0f, 70.0f);
			}
			else
			{
				if (const AReadyOrNotCharacter* ClosestEnemy = FindClosest(KnownEnemies))
				{
					HeadTrackingLocation = ClosestEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
					//HeadTrackingLocation = ClosestEnemy->GetMesh()->GetBoneLocation("head") + FVector(0.0f, 0.0f, 12.0f);
				}
				else if (const AReadyOrNotCharacter* ClosestNeutral = FindClosest(KnownNeutrals))
				{
					HeadTrackingLocation = ClosestNeutral->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
					//HeadTrackingLocation = ClosestNeutral->GetMesh()->GetBoneLocation("head") + FVector(0.0f, 0.0f, 12.0f);
					//HeadTrackingLocation = ClosestNeutral->GetActorLocation();
				}
				else if (const AReadyOrNotCharacter* ClosestFriendly = FindClosest(KnownFriendlies))
				{
					HeadTrackingLocation = ClosestFriendly->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
					//HeadTrackingLocation = ClosestFriendly->GetMesh()->GetBoneLocation("head") + FVector(0.0f, 0.0f, 12.0f);
					//HeadTrackingLocation = ClosestFriendly->GetActorLocation();
				}
			}

			if (GetOwningCharacter()->IsTakingHostage())
			{
				HeadTrackingLocation = LastKnownTargetPosition;
			}

			if (GetOwningCharacter()->IsBeingTakenHostage())
			{
				HeadTrackingLocation = FVector::ZeroVector;
			}
		}
		
		GetOwningCharacter()->Rep_HeadFocalPoint = HeadTrackingLocation;
	}

	//DrawDebugSphere(GetWorld(), HeadTrackingLocation, 20.0f, 32, FColor::Yellow, false, 0.033f);

	if (GetOwningCharacter()->IsArrested() || GetOwningCharacter()->IsBeingArrested())
	{
		GetOwningController()->ClearFocus(EAIFocusPriority::Montage);
		GetOwningController()->ClearFocus(EAIFocusPriority::Move);
		GetOwningController()->ClearFocus(EAIFocusPriority::Default);
		GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
		
		return;
	}

	/*
	if (NearestExtremeThreat)
	{
		DrawDebugBox(GetWorld(), NearestExtremeThreat->GetActorLocation(), FVector(5.0f), FColor::White, false, 0.099f, 0, 1.0f);
		DrawDebugLine(GetWorld(), GetOwningCharacter()->GetActorLocation(), NearestExtremeThreat->GetActorLocation(), FColor::Orange, false, 0.099f, 0, 1.0f);
	}
	*/

	// find the nearest threat
	TimeSinceLastThreatSearch += DeltaTime;
	if ((bRequireThreatSearch || !NearestThreat) && TimeSinceLastThreatSearch > 1.0f)
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_FindNearestThreat);
		
		TimeSinceLastThreatSearch = 0.0f;
		NearestThreat = UThreatAwarenessSubsystem::Get(this)->GetNearestThreatForLocation(GetOwningCharacter()->GetActorLocation(), 700.0f, 200.0f, true);

		if (NearestThreat && !NearestThreat->DoorThreat)
			NearestExtremeThreat = UThreatAwarenessSubsystem::Get(this)->GetNearestThreatForLocation(GetOwningCharacter()->GetActorLocation(), 1500.0f, 200.0f, false, {EThreatLevel::TL_Low, EThreatLevel::TL_Medium, EThreatLevel::TL_High, EThreatLevel::TL_None});
		
		if (NearestExtremeThreat && NearestExtremeThreat->DoorThreat)
		{
			if (const UTeamStackUpActivity* StackUpActivity = GetOwningController()->GetCurrentActivity<UTeamStackUpActivity>())
			{
				if (StackUpActivity->GetActiveStateID() < 2 && StackUpActivity->HasReachedLocation(120.0f))
				{
					if (FVector::Distance(NearestExtremeThreat->DoorThreat->GetDoorMidLocation(), GetOwningCharacter()->GetActorLocation()) < 150.0f &&
						FVector::Distance(StackUpActivity->GetStackUpDoor()->GetDoorMidLocation(), NearestExtremeThreat->DoorThreat->GetDoorMidLocation()) < 300.0f)
					{
						if (StackUpActivity->GetStackUpDoor() != NearestExtremeThreat->DoorThreat)
						{
							if (NearestExtremeThreat->DoorThreat->IsActorInFrontOfDoorway(GetOwningCharacter()))
							{
								if (NearestExtremeThreat->DoorThreat->IsOpen_Backward())
								{
									GetOwningCharacter()->ToggleDoor(NearestExtremeThreat->DoorThreat, false);
								}
							}
							else
							{
								if (NearestExtremeThreat->DoorThreat->IsOpen_Forward())
								{
									GetOwningCharacter()->ToggleDoor(NearestExtremeThreat->DoorThreat, false);
								}
							}
						}
					}
				}
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_UpdatePreferredTarget);

		AReadyOrNotCharacter* PreferredTarget = nullptr;

		// Find the closest visible enemy (active first, then non-active, if they're stunned for example, we wanna prioritize ppl who are more dangerous to us in the moment)
		AReadyOrNotCharacter* ClosestVisibleEnemy = nullptr;
		float ClosestVisibleEnemyDist = FLT_MAX;

		// active only
		for (AReadyOrNotCharacter* Enemy : KnownEnemies)
		{
			if (Enemy->bSurrendered)
				continue;

			bool bIsPlayingDead = false;
			if (const ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(Enemy))
			{
				bIsPlayingDead = AI->IsPlayingDead();
				bool bActive = !Enemy->IsStunned() && !AI->IsHesitating() && !AI->IsPlayingStunAnimation();
				if (!bActive)
					continue;

				if (ACyberneticController* Controller = AI->GetCyberneticsController())
				{
					if (Controller->GetCurrentActivity<UMoveToExitActivity>())
						continue;
				}
			}
			
			const float Dist = (GetOwningCharacter()->GetActorLocation() - Enemy->GetActorLocation()).SizeSquared();
			if (Dist < ClosestVisibleEnemyDist)
			{
				if (!bIsPlayingDead && CanCharacterBeSeen(Enemy))
				{
					ClosestVisibleEnemyDist = Dist;
					ClosestVisibleEnemy = Enemy;
				}
			}
		}
		
		PreferredTarget = ClosestVisibleEnemy;

		if (!PreferredTarget)
		{
			// non-active only
			for (AReadyOrNotCharacter* Enemy : KnownEnemies)
			{
				if (Enemy->bSurrendered)
					continue;
				
				bool bIsPlayingDead = false;
				if (const ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(Enemy))
				{
					bIsPlayingDead = AI->IsPlayingDead();
					bool bActive = !Enemy->IsStunned() && !AI->IsHesitating() && !AI->IsPlayingStunAnimation();
					if (bActive)
						continue;
					
					if (ACyberneticController* Controller = AI->GetCyberneticsController())
					{
						if (Controller->GetCurrentActivity<UMoveToExitActivity>())
							continue;
					}
				}
				
				const float Dist = (GetOwningCharacter()->GetActorLocation() - Enemy->GetActorLocation()).SizeSquared();
				if (Dist < ClosestVisibleEnemyDist)
				{
					if (!bIsPlayingDead && CanCharacterBeSeen(Enemy))
					{
						ClosestVisibleEnemyDist = Dist;
						ClosestVisibleEnemy = Enemy;
					}
				}
			}
		}

		PreferredTarget = ClosestVisibleEnemy;

		if (!PreferredTarget)
		{
			if (!GetOwningCharacter()->IsSuspect())
			{
				AReadyOrNotCharacter* ClosestVisibleNeutral = nullptr;
				float ClosestVisibleNeutralDist = FLT_MAX;

				for (AReadyOrNotCharacter* Neutral : KnownNeutrals)
				{
					if (ACyberneticController* C = Neutral->GetController<ACyberneticController>())
					{
						if (C->GetCurrentActivity<UMoveToExitActivity>())
							continue;
					}
					
					bool bAnyKnownFriendlyIsTrackingTarget = false;
					AReadyOrNotCharacter* FriendlyWhoIsTracking = nullptr;
					for (AReadyOrNotCharacter* Friendly : KnownFriendlies)
					{
						if (Friendly->IsActive() && Cast<ACyberneticCharacter>(Friendly))
						{
							if (ACyberneticController* Controller = Cast<ACyberneticCharacter>(Friendly)->GetCyberneticsController())
							{
								if (Controller->GetCurrentActivity<UMoveToExitActivity>())
									continue;

								if (Controller->GetTargetingComp()->TrackedTarget == Neutral)
								{
									FriendlyWhoIsTracking = Friendly;
									bAnyKnownFriendlyIsTrackingTarget = true;
									break;
								}
							}
						}
					}
					
					if (bAnyKnownFriendlyIsTrackingTarget)
					{
						// am i closer? if so, allow it
						if (FVector::Distance(GetOwningCharacter()->GetActorLocation(), Neutral->GetActorLocation()) < FVector::Distance(FriendlyWhoIsTracking->GetActorLocation(), Neutral->GetActorLocation()))
						{
							
						}
						else
						{
							continue;
						}
					}
					
					const float Dist = (GetOwningCharacter()->GetActorLocation() - Neutral->GetActorLocation()).SizeSquared();
					if (Dist < ClosestVisibleNeutralDist)
					{
						bool bIsPlayingDead = false;
						if (const ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(Neutral))
						{
							bIsPlayingDead = AI->IsPlayingDead();
							
							if (ACyberneticController* Controller = AI->GetCyberneticsController())
							{
								if (Controller->GetCurrentActivity<UMoveToExitActivity>())
									continue;
							}
						}
							
						if (!bIsPlayingDead && CanCharacterBeSeen(Neutral))
						{
							ClosestVisibleNeutralDist = Dist;
							ClosestVisibleNeutral = Neutral;
						}
					}
				}
				
				PreferredTarget = ClosestVisibleNeutral;
			}
		}

		FString Montage = "";
		GetOwningCharacter()->IsAnyTableMontagePlaying(Montage);
		bool bSurrendering = Montage == "tp_surrender";
		
		if (!GetOwningCharacter()->IsActiveForThinking() || bSurrendering)
			PreferredTarget = nullptr;
		
		if (PreferredTarget)
		{
			const bool bIsNewTarget = !TrackedTarget || PreferredTarget != TrackedTarget;
			if (bIsNewTarget)
			{
				GetOwningController()->BulletsFiredTowardsAccuracyPenalty = 0;
				
				if (TrackedTarget)
				{
					TrackedTarget->AITrackingMe.Remove(GetOwningCharacter());
				}
				
				LastTrackedTarget = TrackedTarget;
				if (LastTrackedTarget)
				{
					LastTrackedTarget->OnCharacterKilled.RemoveAll(this);
					LastTrackedTarget->OnCharacterIncapacitated.RemoveAll(this);
				}
				
				TrackedTarget = PreferredTarget;
				TrackedTarget->OnCharacterKilled.RemoveAll(this);
				TrackedTarget->OnCharacterKilled.AddDynamic(this, &UTargetingComponent::OnTrackedTargetKilled);
				TrackedTarget->OnCharacterIncapacitated.RemoveAll(this);
				TrackedTarget->OnCharacterIncapacitated.AddDynamic(this, &UTargetingComponent::OnTrackedTargetIncapacitated);
				
				if (TrackedTarget)
				{
					TrackedTarget->AITrackingMe.AddUnique(GetOwningCharacter());
				}

				if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(TrackedTarget))
				{
					AI->OnExitedSurrender.RemoveAll(this);
					AI->OnExitedSurrender.AddDynamic(this, &UTargetingComponent::OnTrackedTargetExitedSurrender);
				}

				if (APlayerCharacter* Player = Cast<APlayerCharacter>(TrackedTarget))
				{
					Player->OnWeaponReload.RemoveAll(this);
					Player->OnWeaponReload.AddDynamic(this, &UTargetingComponent::OnTrackedTargetStartedReloading);
				}

				// notify the target that we're tracking them so they have a fair fight
				if (ACyberneticController* Controller = TrackedTarget->GetController<ACyberneticController>())
				{
					if (Controller->IsCharacterEnemy(GetOwningCharacter()))
					{
						Controller->GetTargetingComp()->AddKnownEnemy(GetOwningCharacter(), true);
						Controller->GetTargetingComp()->AddCharacterToSeenMap(GetOwningCharacter());
					}
				}
			}
			
			LastKnownTargetPosition = TrackedTarget->GetActorLocation();
			LastSeenKnownTargetFrom = GetOwningCharacter()->GetActorLocation();
			PreviousTimeNotSeenTarget = TimeSinceLastSeenTarget;
			PreviousTimeNotSeenEnemy = TimeSinceLastSeenEnemy;
			PreviousTimeNotSeenFriendly = TimeSinceLastSeenFriendly;
			PreviousTimeNotSeenNeutral = TimeSinceLastSeenNeutral;
			TimeSinceLastSeenTarget = 0.0f;

			if (!LastTrackedTarget)
			{
				LastTrackedTarget = TrackedTarget;
			}

			TimeTrackingTarget = FMath::Clamp(TimeTrackingTarget + DeltaTime, 0.0f, 86400.0f);
			
			if (GetOwningController()->IsCharacterEnemy(TrackedTarget))
			{
				TimeTrackingEnemy = FMath::Clamp(TimeTrackingEnemy + DeltaTime, 0.0f, 86400.0f);
				TimeTrackingNeutral = 0.0f;
				TimeTrackingFriendly = 0.0f;
				TimeSinceLastSeenEnemy = 0.0f;
			}
			else if (GetOwningController()->IsCharacterNeutral(TrackedTarget))
			{
				TimeTrackingNeutral = FMath::Clamp(TimeTrackingNeutral + DeltaTime, 0.0f, 86400.0f);
				TimeTrackingEnemy = 0.0f;
				TimeTrackingFriendly = 0.0f;
				TimeSinceLastSeenNeutral = 0.0f;
			}
			else if (GetOwningController()->IsCharacterFriendly(TrackedTarget))
			{
				TimeTrackingFriendly = FMath::Clamp(TimeTrackingFriendly + DeltaTime, 0.0f, 86400.0f);
				TimeTrackingEnemy = 0.0f;
				TimeTrackingNeutral = 0.0f;
				TimeSinceLastSeenFriendly = 0.0f;
			}
			
			TimeUntilNextLocationHistoryUpdate -= DeltaTime;
			if (TimeUntilNextLocationHistoryUpdate <= 0.0f)
			{
				TimeUntilNextLocationHistoryUpdate = 1.0f;
				TargetLocationHistory.Add(LastKnownTargetPosition);

				if (TargetLocationHistory.Num() > 2)
				{
					TargetLocationHistory.RemoveAt(0);
					TargetLocationHistory.RemoveAll([&](const FVector& Loc)
					{
						return !GetOwningCharacter()->HasLineOfSightTo(Loc);
					});
				}
			}
		}
		else
		{
			if (TrackedTarget)
			{
				TrackedTarget->AITrackingMe.Remove(GetOwningCharacter());
				
				TrackedTarget->OnCharacterKilled.RemoveAll(this);
				TrackedTarget->OnCharacterIncapacitated.RemoveAll(this);

				if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(TrackedTarget))
				{
					AI->OnExitedSurrender.RemoveAll(this);
				}

				if (APlayerCharacter* Player = Cast<APlayerCharacter>(TrackedTarget))
				{
					Player->OnWeaponReload.RemoveAll(this);
				}
			}
			
			TimeSinceLastSeenTarget = FMath::Clamp(TimeSinceLastSeenTarget + DeltaTime, 0.0f, 86400.0f);
			TimeTrackingEnemy = 0.0f;
			TimeTrackingNeutral = 0.0f;
			TimeTrackingFriendly = 0.0f;

			TimeTrackingTarget = 0.0f;
			TimeTrackingHead = 0.0f;
			TrackedTarget = nullptr;
			
			GetOwningCharacter()->SeenBone = NAME_None;

			if (!GetOwningCharacter()->IsActive())
			{
				if (GetOwningController()->GetFocusActorForPriority(EAIFocusPriority::Gameplay) == LastTrackedTarget)
				{
					GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
				}
			}

			if (TimeSinceLastSeenTarget < LastKnownTrackingTime)
			{
				FindAwarenessPath(LastKnownTargetPosition, EPathedAwareness::PA_LastKnownEnemyPosition);
			}
		}
		
		if (LastTrackedTarget)
		{
			if (GetOwningController()->IsCharacterEnemy(LastTrackedTarget))
			{
				TimeSinceLastSeenEnemy = FMath::Clamp(TimeSinceLastSeenEnemy + DeltaTime, 0.0f, 86400.0f);
				TimeSinceLastSeenFriendly = 0.0f;
				TimeSinceLastSeenNeutral = 0.0f;
			}
			else if (GetOwningController()->IsCharacterNeutral(LastTrackedTarget))
			{
				TimeSinceLastSeenNeutral = FMath::Clamp(TimeSinceLastSeenNeutral + DeltaTime, 0.0f, 86400.0f);
				TimeSinceLastSeenFriendly = 0.0f;
				TimeSinceLastSeenEnemy = 0.0f;
			}
			else if (GetOwningController()->IsCharacterFriendly(LastTrackedTarget))
			{
				TimeSinceLastSeenFriendly = FMath::Clamp(TimeSinceLastSeenFriendly + DeltaTime, 0.0f, 86400.0f);
				TimeSinceLastSeenNeutral = 0.0f;
				TimeSinceLastSeenEnemy = 0.0f;
			}
		}
		else
		{
			TimeSinceLastSeenFriendly = 86400.0f;
			TimeSinceLastSeenNeutral = 86400.0f;
			TimeSinceLastSeenEnemy = 86400.0f;
		}
	}

	if (GetOwningCharacter()->IsActive())
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_FindFocusTarget);

		if (TrackedTarget && ShouldTrackTarget())
		{
			CustomFocusActor = nullptr;
			CustomFocusLocation = FVector::ZeroVector;
			TrackingType = ETargetingCompTracking::TCT_TrackingVisibleTarget;
			SetFocusActor(TrackedTarget);

			const FVector FocusBoneLocation = TrackedTarget->GetMesh()->GetBoneLocation(TargetedBone);
			if (FocusBoneLocation != FVector::ZeroVector)
			{
				GetOwningController()->SetFocalPoint(FocusBoneLocation);
			}
		}
		else
		{
			if (!GetOwningCharacter()->IsArrestedOrSurrendered())
			{
				if (!TrackPointsOfInterest())
				{
					GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
				}
			}
		}

		if (!TrackMontagePosition())
		{
			GetOwningController()->ClearFocus(EAIFocusPriority::Montage);
		}
	}
	else
	{
		GetOwningController()->ClearFocus(EAIFocusPriority::Move);
		GetOwningController()->ClearFocus(EAIFocusPriority::Montage);
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_CacheLOSTargets);

		if (TrackedTarget)
		{
			if (GetWorld()->IsTraceHandleValid(TrackedTargetLOSTraceHandle, false))
			{
				FTraceDatum OutTraceData;
				if (GetWorld()->QueryTraceData(TrackedTargetLOSTraceHandle, OutTraceData))
				{
					bHasLOSToTarget = OutTraceData.OutHits.Num() == 0;
					
					if (bHasLOSToTarget)
					{
						LastKnownTargetPositionInLOS = TrackedTarget->GetActorLocation();
					}
				}
			}
			else
			{
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = TrackedTarget->GetActorLocation();
				TrackedTargetLOSTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetOwningCharacter(), TrackedTarget));
			}
			
			bCanSeeTarget = CanCharacterBeSeen(TrackedTarget);
		}
		else
		{
			bHasLOSToTarget = false;
		}

		if (LastTrackedTarget && LastTrackedTarget != TrackedTarget)
		{
			if (GetWorld()->IsTraceHandleValid(LastTrackedTargetLOSTraceHandle, false))
			{
				FTraceDatum OutTraceData;
				if (GetWorld()->QueryTraceData(LastTrackedTargetLOSTraceHandle, OutTraceData))
				{
					bHasLOSToLastTarget = OutTraceData.OutHits.Num() == 0;
				}
			}
			else
			{
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = LastTrackedTarget->GetActorLocation();
				LastTrackedTargetLOSTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetOwningCharacter(), LastTrackedTarget));
			}
			
			bCanSeeLastTarget = CanCharacterBeSeen(LastTrackedTarget);
		}
		else
		{
			if (LastTrackedTarget == TrackedTarget)
				bCanSeeLastTarget = bCanSeeTarget;
		}

		{
			if (GetWorld()->IsTraceHandleValid(LastKnownTargetLOSTraceHandle, false))
			{
				FTraceDatum OutTraceData;
				if (GetWorld()->QueryTraceData(LastKnownTargetLOSTraceHandle, OutTraceData))
				{
					bHasLOSToLastKnownTargetPosition = OutTraceData.OutHits.Num() == 0;
				}
			}
			else
			{
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = LastKnownTargetPosition;
				LastKnownTargetLOSTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, GetOwningCharacter()->GetCollisionQueryParameters());
			}
			
			//bHasLOSToLastKnownTargetPosition = GetOwningCharacter()->HasLineOfSightTo(LastKnownTargetPosition);
		}
	}

	if (GetOwningCharacter()->IsActive())
	{
		//for (FVector a : TargetLocationHistory)
			//DrawDebugSphere(GetWorld(), a, 20.0f, 32, FColor::Cyan, false, 1.0f);
		
		// Find smoke between us and the tracked target
		/*
		{
			SCOPE_CYCLE_COUNTER(STAT_TargetingComp_SmokeDetection);
			
			SmokeGrenadeBetweenTarget = nullptr;
			
			if (TrackedTarget)
			{
				FHitResult Hit;
				FCollisionObjectQueryParams ObjectQueryParams;
				ObjectQueryParams.AddObjectTypesToQuery(ECC_VOLUME);
				
				GetWorld()->LineTraceSingleByObjectType(Hit, GetOwningCharacter()->GetActorLocation(), TrackedTarget->GetActorLocation(), ObjectQueryParams);

				// todo: instead of doing a trace above, it would be much faster if we have the smoke grenade actor location and do simple math instead
				//FMath::LineSphereIntersection(GetOwningCharacter()->GetActorLocation(), (TrackedTarget->GetActorLocation() - GetOwningCharacter()->GetActorLocation()).GetSafeNormal(), FVector::Distance(GetOwningCharacter()->GetActorLocation(), TrackedTarget->GetActorLocation()), FVector::ZeroVector, 500.0f);

				if (ABaseGrenade* Grenade = Cast<ABaseGrenade>(Hit.GetActor()))
				{
					if (Grenade->Type == EGrenadeType::Smoke)
					{
						if (Grenade->bUsed && Grenade->TimeSinceUsed < 20.0f)
						{
							SmokeGrenadeBetweenTarget = Grenade;
						}
					}
				}
			}
		}
		*/
		
		if (GetOwningController()->IsSuspect())
		{
			// Calculate threat
			{
				SCOPE_CYCLE_COUNTER(STAT_TargetingComp_CalculateThreat);
				
				CalculatedThreat = 0.0f;

				for (AReadyOrNotCharacter* Enemy : KnownEnemies)
				{
					if (!CanCharacterBeSeen(Enemy))
						continue;
					
					// Add some amount of threat per seen enemy
					CalculatedThreat += 0.15f;

					// Add threat based on distance to each enemy
					const float DistanceToEnemy = FVector::Distance(Enemy->GetActorLocation(), GetOwningCharacter()->GetActorLocation());
					CalculatedThreat += FMath::GetMappedRangeValueClamped(FVector2D(300.0f, 1500.0f), FVector2D(0.4f, 0.0f), DistanceToEnemy);

					// Add threat based on 'toughness' of enemy
					// Ex. Health, Ammo, Armor, Stunned, etc.

					// Health
					CalculatedThreat -= FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Enemy->GetMaxHealth()), FVector2D(0.5f, 0.0f), Enemy->GetCurrentHealth());

					// Armor
					if (Enemy->GetInventoryComponent()->IsWearingArmour())
					{
						if (Enemy->GetInventoryComponent()->GetArmour()->HasRemainingProtection())
						{
							float Durability = Enemy->GetInventoryComponent()->GetArmour()->GetDurabilityPercentage();
							
							if (Enemy->GetInventoryComponent()->GetArmour()->bIsHeavy)
								Durability *= 2;
							
							CalculatedThreat += Durability/10.0f;
						}
					}

					// All Ammo
					{
						float AllAmmo = 0.0f;
						float AllCurrentAmmo = 0.0f;

						// Add up all magazine weapon ammo from inventory
						for (const ABaseMagazineWeapon* Weapon : Enemy->GetInventoryComponent()->GetInventoryItemsOfClass<ABaseMagazineWeapon>())
						{
							AllAmmo += Weapon->MagazineCountMax * Weapon->AmmoMax;

							for (const FMagazine& Mag : Weapon->Magazines)
							{
								AllCurrentAmmo += Mag.Ammo;
							}
						}

						// Add up all grenade ammo from inventory
						for (const ABaseGrenade* Grenade : Enemy->GetInventoryComponent()->GetInventoryItemsOfClass<ABaseGrenade>())
						{
							AllAmmo += 1.0f;
							
							if (!Grenade->bUsed && !Grenade->bHasEverDetonated)
							{
								AllCurrentAmmo += 1.0f;
							}
						}
						
						if (AllAmmo > 0.0f)
							CalculatedThreat -= FMath::GetMappedRangeValueClamped(FVector2D(0.0f, AllAmmo), FVector2D(0.25f, 0.0f), AllCurrentAmmo);
					}

					// Stun
					if (Enemy->IsStunned())
					{
						CalculatedThreat -= 0.25f;
					}
				}

				Threat = FMath::Lerp(Threat, CalculatedThreat, DeltaTime * 0.5f);

				Tension = 0.0f;
				
				if (!FMath::IsNearlyZero(Threat, 0.0001f))
				{
					float Armor = 0.0f;
					float Ammo = 0.0f;
					
					// All Ammo
					{
						float AllAmmo = 0.0f;
						float AllCurrentAmmo = 0.0f;

						// Add up all magazine weapon ammo from inventory
						for (const ABaseMagazineWeapon* Weapon : GetOwningCharacter()->GetInventoryComponent()->GetInventoryItemsOfClass<ABaseMagazineWeapon>())
						{
							AllAmmo += Weapon->MagazineCountMax * Weapon->AmmoMax;

							for (const FMagazine& Mag : Weapon->Magazines)
							{
								AllCurrentAmmo += Mag.Ammo;
							}
						}

						// Add up all grenade ammo from inventory
						for (const ABaseGrenade* Grenade : GetOwningCharacter()->GetInventoryComponent()->GetInventoryItemsOfClass<ABaseGrenade>())
						{
							AllAmmo += 1.0f;
							
							if (!Grenade->bUsed && !Grenade->bHasEverDetonated)
							{
								AllCurrentAmmo += 1.0f;
							}
						}

						if (AllAmmo > 0.0f)
							Ammo = AllCurrentAmmo/AllAmmo;
					}

					if (GetOwningCharacter()->GetInventoryComponent()->IsWearingArmour())
					{
						if (GetOwningCharacter()->GetInventoryComponent()->GetArmour()->HasRemainingProtection())
						{
							float Durability = GetOwningCharacter()->GetInventoryComponent()->GetArmour()->GetDurabilityPercentage();
							
							if (GetOwningCharacter()->GetInventoryComponent()->GetArmour()->bIsHeavy)
								Durability *= 2;

							Armor += Durability;
						}
					}

					const float Resources = Armor + Ammo + (KnownFriendlies.Num()/4);
					
					//ULog::Number(Resources, GetOwningCharacter()->GetName() + " | Resources: ");
					
					Tension = Threat-Resources;

					Tension += GetOwningCharacter()->GetHesitationTime()/10.0f;
				}
			}

			// calculate exposure levels
			{
				SCOPE_CYCLE_COUNTER(STAT_TargetingComp_CalculateExposure);
	
				AReadyOrNotCharacter* TrackingEnemy = TrackedTarget;
				if (!TrackingEnemy)
					TrackingEnemy = LastTrackedTarget;

				constexpr uint8 TraceTargets = 6;
				
				const FName Bones[TraceTargets] = { "head", "spine_2", "hand_LE", "hand_RI", "foot_LE", "foot_RI"};
				
				// Calculate exposure to tracked enemy
				// How much of our body is visible to the enemy
				
				//ExposureFromEnemy = CalculateExposure(TrackingEnemy, GetOwningCharacter());
				{
					bool HitResults[TraceTargets] = {true, true, true, true, true, true};

					for (uint8 i = 0; i < TraceTargets; i++)
					{
						FTraceHandle& H = ExposureTraceHandles_1[i];
						
						if (GetWorld()->IsTraceHandleValid(H, false))
						{
							FTraceDatum OutTraceData;
							if (GetWorld()->QueryTraceData(H, OutTraceData))
							{
								HitResults[i] = OutTraceData.OutHits.Num() > 0;
								ExposureFromEnemy = 0.0f;
							}
						}
						else
						{
							if (TrackingEnemy)
							{
								const FVector StartTrace = GetOwningCharacter()->GetMesh()->GetBoneLocation(Bones[i]);
								const FVector EndTrace = TrackingEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
								H = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetOwningCharacter(), TrackingEnemy));
							}
							else
							{
								ExposureFromEnemy = 0.0f;
							}
						}
					}
					
					for (const bool bHit : HitResults)
					{
						if (!bHit)
						{
							ExposureFromEnemy += 1.0f/(float)TraceTargets;
						}
					}
				}
				
				//EnemyExposureFromUs = CalculateExposure(GetOwningCharacter(), TrackingEnemy);
				{
					bool HitResults[TraceTargets] = {true, true, true, true, true, true};
					
					for (uint8 i = 0; i < TraceTargets; i++)
					{
						FTraceHandle& H = ExposureTraceHandles_1[i];
						
						if (GetWorld()->IsTraceHandleValid(H, false))
						{
							FTraceDatum OutTraceData;
							if (GetWorld()->QueryTraceData(H, OutTraceData))
							{
								HitResults[i] = OutTraceData.OutHits.Num() > 0;
								EnemyExposureFromUs = 0.0f;
							}
						}
						else
						{
							if (TrackingEnemy)
							{
								const FVector StartTrace = TrackingEnemy->GetMesh()->GetBoneLocation(Bones[i]);
								const FVector EndTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
								H = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetOwningCharacter(), TrackingEnemy));
							}
							else
							{
								EnemyExposureFromUs = 0.0f;
							}
						}
					}
					
					for (const bool bHit : HitResults)
					{
						if (!bHit)
						{
							EnemyExposureFromUs += 1.0f/(float)TraceTargets;
						}
					}
				}
			}

			if (GetOwningCharacter()->IsActiveForCombat() && TrackedTarget)
			{
				// Bone targeting
				{
					SCOPE_CYCLE_COUNTER(STAT_TargetingComp_BoneTargeting);
					
					if (GetOwningCharacter()->GetEquippedWeapon())
					{
						PreviousBoneTargetZoneIndex = CurrentBoneTargetZoneIndex;
						
						float TimeSpentEngagingOnTarget = GetOwningController()->GetCombatActivity()->TimeSpentEngagingOnTarget;
						
						// convert time to bone target zone
						CurrentBoneTargetZoneIndex = (uint8)FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EngagementTimeUntilReachedLastBoneZone), FVector2D(0, BoneTargetZones.Num() - 1), TimeSpentEngagingOnTarget);

						// Keep retargeting at a constant rate, no need to null TargetedBone
						if (TimeSinceLastBoneRetarget > BoneRetargetingRate)
						{
							// Switch to new bone zone, if different from previous
							{
								if (CurrentBoneTargetZoneIndex != PreviousBoneTargetZoneIndex ||
									BonesToTarget.Num() == 0)
								{
									if (BoneTargetZones.IsValidIndex(CurrentBoneTargetZoneIndex))
										BonesToTarget = BoneTargetZones[CurrentBoneTargetZoneIndex];
									else
										BonesToTarget = BoneTargetZones[0];
								}
							}

							// Retarget bone
							{
								if (BonesToTarget.Num() > 0)
								{
									const uint8 Index = FMath::RandRange(0, BonesToTarget.Num() - 1);
										
									TargetedBone = BonesToTarget[Index];
										
									BonesToTarget.RemoveAt(Index);
								}

								TimeSinceLastBoneRetarget = 0.0f;
							}
						}

						TimeSinceLastBoneRetarget += DeltaTime;
					}
					else
					{
						PreviousBoneTargetZoneIndex = 0;
						CurrentBoneTargetZoneIndex = 0;
						TimeSinceLastBoneRetarget = 0.0f;
					}
				}
			}
		}

		// calculate num of AI tracking us
		{
			AITrackingMe.Reset(5);
			for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
			{
				if (AI != GetOwningCharacter() && AI->IsActive())
				{
					if (AI->GetCyberneticsController()->GetTrackedTarget() == AI)
					{
						AITrackingMe.AddUnique(AI);
					}
				}
			}
		}
	}
}

#if !UE_BUILD_SHIPPING
void UTargetingComponent::TickComponent_Debug(float DeltaTime)
{
	#if WITH_EDITOR
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_EditorStuff);

		/*
		if (GetOwningCharacter()->IsSuspect())
			RequiredTimeTrackingTarget = FMath::Max(AI_CONFIG_GET_FLOAT("SuspectRequiredTimeSpentOnTarget"), 0.0f);
		else if (GetOwningCharacter()->IsOnSWATTeam())
			RequiredTimeTrackingTarget = FMath::Max(AI_CONFIG_GET_FLOAT("SWATRequiredTimeSpentOnTarget"), 0.0f);
		
		EngagementTimeUntilReachedLastBoneZone = FMath::Clamp(AI_CONFIG_GET_FLOAT("EngagementTimeUntilReachedLastBoneZone", 4.0f), 0.01f, 60.0f);
		BoneRetargetingRate = FMath::Clamp(AI_CONFIG_GET_FLOAT("BoneRetargetingRate", 1.0f), 0.1f, 5.0f);
		*/
		
		if (CVarRonTrackDebugActor.GetValueOnAnyThread() == 1)
		{
			for (TActorIterator<AActor>It(GetWorld()); It; ++It)
			{
				if (It->Tags.Contains("TestTrackingActor"))
				{
					GetOwningController()->SetFocus(*It);
					break;
				}
			}
		}
		
		if (GetOwningCharacter()->IsSuspect() && CVarRonTrackDebugTurns.GetValueOnAnyThread() > 0)
		{
			static float time = 0.0f;
			time += DeltaTime;
			if (time > 3.0f)
			{
				time = 0.0f;

				FVector FocalPoint = GetOwningCharacter()->GetActorLocation() + FVector::UpVector * 45.0f + GetOwningCharacter()->GetActorForwardVector() * -100.0f;

				LOCAL_PLAYER;
				UAISense_Hearing::ReportNoiseEvent(GetWorld(), FocalPoint, 1.0f, LocalPlayer);

				DrawDebugBox(GetWorld(), FocalPoint, FVector(5.0f), FColor::Cyan, false, 1.0f, 0, 0.5f);
			}
		}
	}
	#endif
	
	if (CVarDrawBoneTargeting.GetValueOnAnyThread() > 0)
	{
		if (TargetedBone != NAME_None)
		{
			const float TimeSpentEngagingOnTarget = GetOwningController()->GetCombatActivity()->TimeSpentEngagingOnTarget;
			const FColor& BoneZoneColor = FColor::MakeRedToGreenColorFromScalar(FMath::GetMappedRangeValueClamped(FVector2D(0, BoneTargetZones.Num() - 1), FVector2D(1.0f, 0.0f), CurrentBoneTargetZoneIndex));
			//const FColor& BoneZoneColor = FColor::MakeRedToGreenColorFromScalar(FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EngagementTimeUntilReachedLastBoneZone), FVector2D(1.0f, 0.0f), TimeSpentEngagingOnTarget));

			const FString& DebugMessage = "Targeted Bone: " + TargetedBone.ToString() + LINE_TERMINATOR +
											"Bone Zone Index: " + FString::FromInt(CurrentBoneTargetZoneIndex) + LINE_TERMINATOR +
											FString::Printf(TEXT("Target Engagement Time: %.2f"), TimeSpentEngagingOnTarget);
			
			DrawDebugString(GetWorld(), GetOwningCharacter()->GetMesh()->GetBoneLocation(TargetedBone), DebugMessage, nullptr, BoneZoneColor, DeltaTime + 0.01f, true);
		}
	}

	//if (GetLastKnownEnemyPosition() != FVector::ZeroVector && FAISystem::IsValidLocation(GetLastKnownEnemyPosition()))
	//{
	//	DrawDebugBox(GetWorld(), GetLastKnownEnemyPosition(), FVector(10.0f), FColor::Red, false, DeltaTime, 0, 1.5f);
	//	DrawDebugString(GetWorld(), GetLastKnownEnemyPosition(), "LastKnownEnemyPosition: " + GetLastKnownEnemyPosition().ToString(), nullptr, FColor::White, DeltaTime, true);
	//}
}
#endif

void UTargetingComponent::AddCharacterToSeenMap(AReadyOrNotCharacter* InCharacter)
{
	if (!CharactersSeen.Contains(InCharacter))
	{
		CharactersSeen.Add(InCharacter, GetOwningController()->GetReactionTime(EActorSenseType::Sight)+0.001f);
	}
}

ACyberneticCharacter* UTargetingComponent::GetOwningCharacter() const
{
	if (const ACyberneticController* Controller = Cast<ACyberneticController>(GetOwner()))
	{
		return Controller->GetCharacter();
	}

	return nullptr;
}

ACyberneticController* UTargetingComponent::GetOwningController() const
{
	return GetOwner<ACyberneticController>();
}

bool UTargetingComponent::TrackMontagePosition()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackMontagePosition);
	
	FVector MontageFocusLocation = LastKnownTargetPosition;
	
	if (GetTrackedTarget())
	{
		MontageFocusLocation = GetTrackedTarget()->GetActorLocation();
	}

	if (GetOwningCharacter()->IsTableMontagePlaying("tp_fake_surrender") ||
		GetOwningCharacter()->IsTableMontagePlaying("tp_spct_detonatevest") ||
		(GetOwningCharacter()->IsSurrendered() && GetOwningCharacter()->IsAny3PMontageActive()))
	{
		GetOwningController()->SetFocalPoint(MontageFocusLocation, EAIFocusPriority::Montage);
		return true;
	}

	if (!MontageFocalAnim)
	{
		return false;
	}
	
	if (const UAnimMontage* CurrentActiveMontage = GetOwningCharacter()->GetCurrentMontage())
	{
		if (CurrentActiveMontage == MontageFocalAnim)
		{
			GetOwningController()->SetFocalPoint(MontageFocalPoint, EAIFocusPriority::Montage);
			TrackingType = ETargetingCompTracking::TCT_TrackMontagePosition;
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::ShouldTrackTarget()
{
	if (!GetOwningCharacter()->IsActiveForCombat())
		return false;
	
	if (const UBaseCombatActivity* CombatActivity = GetOwningController()->GetCombatActivity())
	{
		if (!CombatActivity->ShouldTrackTarget())
		{
			return false;
		}
		
		FScriptedFireAt ScriptedFireAt;
		if (CombatActivity->IsTryingToFireAtScriptedActor(ScriptedFireAt))
		{
			if (ScriptedFireAt.bOverrideTargetedEnemy || TrackingType != ETargetingCompTracking::TCT_TrackingVisibleTarget)
			{
				return false;
			}
		}
		
		if (GetOwningCharacter()->IsOnSWATTeam())
			return true;
		
		if (GetOwningCharacter()->GetEquippedItem<AMeleeWeapon>())
		{
			if (TrackedTarget)
			{
				return FVector::Distance(TrackedTarget->GetActorLocation(), GetOwningCharacter()->GetActorLocation()) < 200.0f;
			}
		}

		if (GetOwningCharacter()->IsTableMontagePlaying("tp_fake_surrender"))
			return true;
		
		if (!GetOwningCharacter()->IsStrafing())
			return false;

		if (GetOwningCharacter()->IsStunned())
			return false;

		if (GetOwningCharacter()->IsTakingCover() ||
			GetOwningCharacter()->IsTakingCoverAtLandmark() ||
			GetOwningCharacter()->IsTraversingHole())
			return false;

		if (GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UFlankingCombatMove>() ||
			GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UFleeingCombatMove>() ||
			GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UHardCoverCombatMove>())
			return false;
		
		if (GetOwningController()->GetCurrentActivity<UTakeHostageActivity>() ||
			GetOwningController()->GetCurrentActivity<UArrestTargetActivity>())
			return false;
		
		if (UTeamStackUpActivity* StackUpActivity = GetOwningController()->GetCurrentActivity<UTeamStackUpActivity>())
		{
			if (StackUpActivity->GetActiveStateID() <= 4)
			{
				if (ADoor* Door = StackUpActivity->GetStackUpDoor())
				{
					const FVector DirectionToDoorA = (TrackedTarget->GetActorLocation() - Door->GetDoorMidLocation()).GetSafeNormal2D();
					const float DotA = FVector::DotProduct(DirectionToDoorA, Door->GetActorRightVector());
					
					const FVector DirectionToDoorB = (GetOwningCharacter()->GetActorLocation() - Door->GetDoorMidLocation()).GetSafeNormal2D();
					const float DotB = FVector::DotProduct(DirectionToDoorB, Door->GetActorRightVector());
					
					// if same side, ignore them
					if (DotA > -0.1f && DotB > -0.1f)
					{
						return false;
					}
				}
			}
		}
		
		return true;
	}
	
	return false;
}

void UTargetingComponent::ClearCustomFocusPoints()
{
	CustomFocusLocation = FVector::ZeroVector;
	CustomFocusActor = nullptr;
}

bool UTargetingComponent::CanCharacterBeSeen(AReadyOrNotCharacter* InCharacter) const
{
	if (!InCharacter)
		return false;
	
	if (const float* ValuePtr = CharactersSeen.Find(InCharacter))
	{
		return *ValuePtr >= GetOwningController()->GetReactionTime(EActorSenseType::Sight);
	}
	
	if (const float* ValuePtr = GetOwningController()->DamagedBy.Find(InCharacter))
	{
		return *ValuePtr < 0.5f;
	}
	
	return false;
}

bool UTargetingComponent::CanActorBeSeen(AActor* InActor) const
{
	FActorPerceptionBlueprintInfo PerceptionOfActor;
	if (GetOwningController()->GetRONPerceptionComp()->GetActorsPerception(InActor, PerceptionOfActor))
	{
		for (const FAIStimulus& Stimulus : PerceptionOfActor.LastSensedStimuli)
		{
			if (!Stimulus.IsValid())
				continue;
			
			if (Stimulus.IsExpired() || Stimulus.GetAge() > 1.0f)
				continue;

			if (Stimulus.Type.Name == RON_SENSE_SIGHT || Stimulus.Type.Name == RON_SENSE_DAMAGE)
			{
				return true;
			}
		}
	}

	return false;
}

bool UTargetingComponent::IsTrackingMontagePosition() const
{
	if (GetOwningCharacter())
	{
		if (GetOwningCharacter()->IsTableMontagePlaying("tp_fake_surrender"))
		{
			return true;
		}

		if (GetOwningCharacter()->IsTableMontagePlaying("tp_melee"))
		{
			return true;
		}

		if (!GetOwningCharacter()->IsSurrenderComplete() &&
			GetOwningCharacter()->bSurrenderingWithItem &&
			GetOwningCharacter()->IsAny3PMontageActive())
		{
			return true;
		}
		
		if (GetOwningCharacter()->IsTableMontagePlaying("tp_raise"))
		{
			return true;
		}
		
		if (GetOwningCharacter()->IsSuspect())
		{
			if (GetOwningCharacter()->IsTableMontagePlaying("tp_hesitate")) 
			{
				return true; 
			}

			if (GetOwningCharacter()->IsTableMontagePlaying("tp_search_fullbody"))
			{
				return true;				
			}
		}
		
		const UAnimMontage* CurrentActiveMontage = GetOwningCharacter()->GetCurrentMontage();
		if (CurrentActiveMontage && CurrentActiveMontage == MontageFocalAnim)
		{
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::HasSeenCharacterFor(AReadyOrNotCharacter* InCharacter, float Seconds) const
{
	if (!InCharacter)
		return false;

	if (const float* ValuePtr = CharactersSeen.Find(InCharacter))
	{
		return *ValuePtr > Seconds;
	}

	return false;
}

bool UTargetingComponent::TrackScriptedFireAtActor()
{
	if (const UBaseCombatActivity* CombatActivity = GetOwningController()->GetCombatActivity())
	{
		const FScriptedFireAt& ScriptedFireAt = CombatActivity->CurrentScriptedFireAt;
		
		if ((ScriptedFireAt.FireAtActor || ScriptedFireAt.FireAtLocation != FVector::ZeroVector) &&
			ScriptedFireAt.TimeRemaining > 0.0f)
		{
			if (ScriptedFireAt.FireAtActor)
			{
				/*
				#if !UE_BUILD_SHIPPING
				DrawDebugSphere(GetWorld(), ScriptedFireAt.FireAtActor->GetActorLocation(), 20.0f, 32, FColor::Yellow, false, GetWorld()->DeltaTimeSeconds + 0.005f);
				DrawDebugString(GetWorld(), ScriptedFireAt.FireAtActor->GetActorLocation(), "Scripted Firing at " + ScriptedFireAt.FireAtActor->GetName(), nullptr, FColor::White, GetWorld()->GetDeltaSeconds() - 0.01f, true);
				#endif
				*/
				
				GetOwningController()->SetFocus(ScriptedFireAt.FireAtActor);
				GetOwningController()->SetFocalPoint(GetOwningController()->GetFocalPointOnActor(ScriptedFireAt.FireAtActor));
				return true;
			}

			/*
			#if !UE_BUILD_SHIPPING
			DrawDebugSphere(GetWorld(), ScriptedFireAt.FireAtLocation, 20.0f, 32, FColor::Yellow, false, GetWorld()->DeltaTimeSeconds + 0.005f);
			DrawDebugString(GetWorld(), ScriptedFireAt.FireAtLocation, "Scripted Firing at " + ScriptedFireAt.FireAtLocation.ToCompactString(), nullptr, FColor::White, GetWorld()->GetDeltaSeconds() - 0.01f, true);
			#endif
			*/
			
			GetOwningController()->SetFocalPoint(ScriptedFireAt.FireAtLocation);
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackScriptedLookAtActor()
{
	if (const UBaseCombatActivity* CombatActivity = GetOwningController()->GetCombatActivity())
	{
		const FScriptedLookAt& ScriptedLookAt = CombatActivity->CurrentScriptedLookAt;
		
		if ((ScriptedLookAt.LookAtActor || ScriptedLookAt.LookAtLocation != FVector::ZeroVector) &&
			ScriptedLookAt.TimeRemaining > 0.0f)
		{
			if (ScriptedLookAt.LookAtActor)
			{
				/*
				#if !UE_BUILD_SHIPPING
				DrawDebugSphere(GetWorld(), ScriptedLookAt.LookAtActor->GetActorLocation(), 20.0f, 32, FColor::Yellow, false, GetWorld()->DeltaTimeSeconds + 0.005f);
				DrawDebugString(GetWorld(), ScriptedLookAt.LookAtActor->GetActorLocation(), "Scripted Looking at " + ScriptedLookAt.LookAtActor->GetName(), nullptr, FColor::White, GetWorld()->GetDeltaSeconds() - 0.01f, true);
				#endif
				*/
				
				GetOwningController()->SetFocus(ScriptedLookAt.LookAtActor);
				GetOwningController()->SetFocalPoint(GetOwningController()->GetFocalPointOnActor(ScriptedLookAt.LookAtActor));
				return true;
			}

			/*
			#if !UE_BUILD_SHIPPING
			DrawDebugSphere(GetWorld(), ScriptedLookAt.LookAtLocation, 20.0f, 32, FColor::Yellow, false, GetWorld()->DeltaTimeSeconds + 0.005f);
			DrawDebugString(GetWorld(), ScriptedLookAt.LookAtLocation, "Scripted Firing at " + ScriptedLookAt.LookAtLocation.ToCompactString(), nullptr, FColor::White, GetWorld()->GetDeltaSeconds() - 0.01f, true);
			#endif
			*/
			
			GetOwningController()->SetFocalPoint(ScriptedLookAt.LookAtLocation);
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackCombatMoveLocation()
{
	if (const UBaseCombatActivity* CombatActivity = GetOwningController()->GetCombatActivity())
	{
		if (UBaseCombatMoveActivity* CombatMoveActivity = CombatActivity->GetCombatMoveActivity())
		{
			FVector FocalPoint = FVector::ZeroVector;
			if (CombatMoveActivity->OverrideFocalPoint(FocalPoint))
			{
				GetOwningController()->SetFocalPoint(FocalPoint);
				return true;
			}
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackVisibleNeutrals()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackVisibleNeutrals);
	
	const bool bIsMoveStyleStrafe = GetOwningCharacter()->MoveStyle && GetOwningCharacter()->MoveStyle->bIsStrafing;
	if (!bIsMoveStyleStrafe && GetOwningController()->IsMoving())
		return false;
	
	for (AReadyOrNotCharacter* Neutral : KnownNeutrals)
	{
		if (!Neutral)
			continue;
		
		if (!Neutral->IsActive())
			continue;
		
		if ((Neutral->GetActorLocation() - GetOwningCharacter()->GetActorLocation()).Size() < 1000.0f &&
			CanCharacterBeSeen(Neutral))
		{
			if (!IsTrackedByKnownFriendly(Neutral))
			{
				GetOwningController()->SetFocus(Neutral);
				return true;
			}
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackPointsOfInterest()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackPointsOfInterest);
		
	TrackingType = ETargetingCompTracking::TCT_None;
	
	if (TrackMoveVector())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingMoveVector;
		return true;
	}
	
	if (TrackScriptedLookAtActor())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackScriptedFireAtActor;
		return true;
	}
	
	if (TrackScriptedFireAtActor())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackScriptedFireAtActor;
		return true;
	}
	
	if (TrackActivityRelatedPoints())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingActivity;
		return true;
	}
	
	if (TrackCombatMoveLocation())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingCombatMoveActivity;
		return true;
	}

	if (GetOwningCharacter()->IsOnSWATTeam() && !Cast<ATrailerSWATCharacter>(GetOwningCharacter()))
	{
		return TrackSwatPointsOfInterest();
	}

	if (GetOwningCharacter()->IsSuspect())
	{
		return TrackSuspectPointsOfInterest();
	}
	
	if (GetOwningCharacter()->IsCivilian())
	{
		return TrackCivilianPointsOfInterest();
	}
	
	if (CustomFocusLocation != FVector::ZeroVector)
	{
		TrackingType = ETargetingCompTracking::TCT_TrackCustomLocation;
		GetOwningController()->SetFocalPoint(CustomFocusLocation, EAIFocusPriority::Gameplay);
		return true;
	}

	GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
	return false;
}

bool UTargetingComponent::TrackSwatPointsOfInterest()
{
	if (TrackVisibleNeutrals())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingVisibleNeutrals;
		return true;
	}

	/*
	if (TimeSinceLastSeenTarget < LastKnownTrackingTime)
	{
		//DrawDebugSphere(GetWorld(), LastKnownTargetPosition, 10.0f, 32, FColor::Red, false, GetWorld()->GetDeltaSeconds() + 0.01f);
		GetOwningController()->SetFocalPoint(LastKnownTargetPosition);
		TrackingType = ETargetingCompTracking::TCT_TrackingEnemyLastKnownPosition;
		return true;
	}
	*/

	if (CustomFocusActor)
	{
		if (const ADoor* Door = Cast<ADoor>(CustomFocusActor))
		{
			FVector DoorLocation = Door->CalculateClosestPoint(GetOwningCharacter()->GetActorLocation());
			if (FMath::Abs(DoorLocation.Z - GetOwningCharacter()->GetActorLocation().Z) > 150.0f)
			{
				DoorLocation.Z = Door->GetDoorMidLocation().Z;
			}
			else
			{
				if (FMath::Abs(Door->GetDoorMidLocation().Z - GetOwningCharacter()->GetActorLocation().Z) < 150.0f)
					DoorLocation.Z = GetOwningCharacter()->GetActorLocation().Z + 50.0f;
			}

			FVector OwnerLocation = FVector(GetOwningCharacter()->GetActorLocation().X, GetOwningCharacter()->GetActorLocation().Y, GetOwningCharacter()->GetActorLocation().Z+50.0f);
			if (FVector::Distance(DoorLocation, OwnerLocation) < 100.0f)
			{
				DoorLocation += (DoorLocation-OwnerLocation).GetSafeNormal() * 100.0f;
			}

			if (GetOwningCharacter()->Rep_FocalPoint == DoorLocation)
			{
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = DoorLocation;
				
				FCollisionQueryParams QueryParams = GetOwningCharacter()->GetCollisionQueryParameters();
				QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
				QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
				QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
				
				const bool bNoLOS = GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, QueryParams);
				
				if (bNoLOS)
				{
					CustomFocusLocation = FVector::ZeroVector;
					CustomFocusActor = nullptr;
					return false;
				}
			}

			TrackingType = ETargetingCompTracking::TCT_TrackCustomLocation;
			GetOwningController()->SetFocalPoint(DoorLocation, EAIFocusPriority::Gameplay);
			return true;
		}
		
		GetOwningController()->SetFocus(CustomFocusActor, EAIFocusPriority::Gameplay);
		return true;
	}
	
	if (CustomFocusLocation != FVector::ZeroVector)
	{
		if (GetOwningCharacter()->Rep_FocalPoint == CustomFocusLocation)
		{
			const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
			const FVector EndTrace = CustomFocusLocation;
			
			FCollisionQueryParams QueryParams = GetOwningCharacter()->GetCollisionQueryParameters();
			QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
			QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
			QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
			
			const bool bNoLOS = GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, QueryParams);
			
			if (bNoLOS)
			{
				CustomFocusLocation = FVector::ZeroVector;
				CustomFocusActor = nullptr;
				return false;
			}
		}
		
		TrackingType = ETargetingCompTracking::TCT_TrackCustomLocation;
		GetOwningController()->SetFocalPoint(CustomFocusLocation, EAIFocusPriority::Gameplay);
		return true;
	}
	
	if (TrackStairThreatAwarenessActors())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingStairThreatAwarenessActor;
		return true;
	}
	
	if (TrackNoiseRelatedEvents())
	{
		ClearCustomFocusPoints();
		
		TrackingType = ETargetingCompTracking::TCT_TrackingNoiseStimulus;
		return true;
	}
	
	if (TrackThreatAwarenessActors())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingThreatAwarenessActor;
		return true;
	}

	return false;
}

bool UTargetingComponent::TrackSuspectPointsOfInterest()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackSuspectPointsOfInterest);

	//#if WITH_EDITOR	// initialized in cyber controller
	//LastKnownTrackingTime = AI_CONFIG_GET_FLOAT("SuspectTrackLastKnownPositionTime");
	//#endif
	
	if (TimeSinceLastSeenTarget < LastKnownTrackingTime)
	{
		SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackEnemyLastKnownPosition);
		
		CustomFocusLocation = FVector::ZeroVector;
		TrackingType = ETargetingCompTracking::TCT_TrackingEnemyLastKnownPosition;
		
		if (const FPathAwarenessInfo* Info = LatestPathedAwareness.Find(EPathedAwareness::PA_LastKnownEnemyPosition))
		{
			GetOwningController()->SetFocalPoint(Info->Location);
			return true;
		}
		
		//DrawDebugSphere(GetWorld(), LastKnownTargetPosition, 10.0f, 32, FColor::Red, false, GetWorld()->GetDeltaSeconds() + 0.01f);
		GetOwningController()->SetFocalPoint(LastKnownTargetPosition);
		return true;
	}

	/*
	ADoor* Door = nullptr;
	if (GetOwningController()->DoesPathGoThroughDoor(Door))
	{
		if (FVector::Distance(Door->GetDoorMidLocation(), GetOwningCharacter()->GetActorLocation()) > 250.0f)
		{
			Door = nullptr;
		}
		
		if (Door)
		{
			TrackingType = ETargetingCompTracking::TCT_TrackNearestDoor;
			GetOwningController()->SetFocalPoint(Door->GetDoorMidLocation(), EAIFocusPriority::Gameplay);
			return true;
		}
	}
	*/

	if (TrackNoiseRelatedEvents())
	{
		CustomFocusLocation = FVector::ZeroVector;
		TrackingType = ETargetingCompTracking::TCT_TrackingNoiseStimulus;
		return true;
	}

	if (TrackOverrideInterests())
	{
		CustomFocusLocation = FVector::ZeroVector;
		TrackingType = ETargetingCompTracking::TCT_TrackingOverrideInterests;
		return true;
	}
	
	if (TrackNearestDoor())
	{
		CustomFocusLocation = FVector::ZeroVector;
		TrackingType = ETargetingCompTracking::TCT_TrackNearestDoor;
		return true;
	}

	return false;
}

bool UTargetingComponent::TrackCivilianPointsOfInterest()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackCivilianPointsOfInterest);
	
	if (TrackVisibleNeutrals())
	{
		TrackingType = ETargetingCompTracking::TCT_TrackingVisibleNeutrals;
		return true;
	}
	
	if (TrackNoiseRelatedEvents())
	{
		CustomFocusLocation = FVector::ZeroVector;
		TrackingType = ETargetingCompTracking::TCT_TrackingNoiseStimulus;
		return true;
	}

	return false;
}

bool UTargetingComponent::TrackMoveVector()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackMoveVector);
	
	if (!GetOwningCharacter()->IsActiveForMovement())
		return false;
		
	const FVector Velocity = GetOwningCharacter()->GetVelocity();
	const float Length = Velocity.SizeSquared();
	
	if (GetOwningController()->IsMoving() && Length > 625.0f) // 25.0f (when Sqrt'd)
	{
		// Check if we're stunned with gas, as we still need movement focus when fleeing
		if (GetOwningCharacter()->IsAnimationBlocking() && !GetOwningCharacter()->IsOnlyStunnedWithGas())
		{
			GetOwningController()->ClearFocus(EAIFocusPriority::Move);
			return false;
		}

		if (GetOwningController()->GetActivity<UTeamStackUpActivity>() ||
			GetOwningController()->GetActivity<UTeamBreachAndClearActivity>() ||
			GetOwningController()->GetActivity<UDoorInteractionActivity>() ||
			GetOwningController()->GetActivity<UScanDoorActivity>())
		{
			return false;
		}

		bool bForceTrackMovement = false;
		
		const bool bIsMoveStyleStrafe = GetOwningCharacter()->MoveStyle && GetOwningCharacter()->MoveStyle->bIsStrafing;

		if (!bIsMoveStyleStrafe || (bIsMoveStyleStrafe && GetOwningController()->bCanTrackMoveVectorWhileStrafe))
			bForceTrackMovement = true;

		if (GetOwningCharacter()->IsCarrying() || GetOwningCharacter()->IsStunned())
			bForceTrackMovement = true;

		if (Cast<UChargeCombatMove>(GetOwningController()->GetCombatActivity()->GetCombatMoveActivity()))
			bForceTrackMovement = true;
		
		if (GetOwningCharacter()->GetEquippedItem<AMeleeWeapon>())
			bForceTrackMovement = true;

		if (bForceTrackMovement)
		{
			LastMoveVectorFocalPoint = GetOwningCharacter()->GetActorLocation() + Velocity.GetSafeNormal() * 500.0f;

			GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
			GetOwningController()->SetFocalPoint(LastMoveVectorFocalPoint, EAIFocusPriority::Move);
				
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackActivityRelatedPoints()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackActivityRelatedPoints);
	
	if (UBaseActivity* BaseActivity = GetOwningController()->GetCurrentActivity())
	{
		FVector OutFocalPoint = FVector::ZeroVector;
		if (BaseActivity->OverrideFocalPoint(OutFocalPoint))
		{
			GetOwningController()->ClearFocus(EAIFocusPriority::Gameplay);
			
			if (FAISystem::IsValidLocation(OutFocalPoint) && OutFocalPoint != FVector::ZeroVector)
			{
				/*
				if (const FVector* PathLocation = LatestPathedAwareness.Find(EPathedAwareness::PA_ActivityLocation))
				{
					GetOwningController()->SetFocalPoint(*PathLocation);
					return true;
				}
				*/
				
				GetOwningController()->SetFocalPoint(OutFocalPoint, EAIFocusPriority::Gameplay);
			}
			
			TrackingType = ETargetingCompTracking::TCT_TrackingActivity;
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackStairThreatAwarenessActors()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackStairThreatAwarenessActors);

	FVector Destination = GetOwningController()->GetRONPathFollowingComp()->GetPathDestination();
	
	if (Destination == FVector::ZeroVector)
	{
		if (const UBaseActivity* Activity = GetOwningController()->GetCurrentActivity())
		{
			Destination = Activity->GetLocation();
		}
	}

	if (Destination == FVector::ZeroVector)
	{
		return false;
	}
	
	const FVector DirectionToDestination = (Destination - GetOwningCharacter()->GetNavAgentLocation()).GetSafeNormal();

	const float Dot = FVector::DotProduct(FVector::UpVector, DirectionToDestination);
	if (FMath::Abs(Dot) < 0.1f)
		return false;
	
	const bool bLookUp = Dot > 0.0f;
		
	const AThreatAwarenessActor* StairThreat = nullptr;
	
	constexpr float MaxDistance = 1000.0f;
	constexpr float MaxZHeight = 1000.0f;
	
	const float ClampedXY = FMath::Clamp(MaxDistance, 1.0f, 10000.0f);
	const float ClampedZ = FMath::Clamp(MaxZHeight, 1.0f, 10000.0f);
	
	TArray<FThreatAwarenessDataOctreeElement> ThreatPoints;
	UThreatAwarenessSubsystem::Get(this)->FindThreatPoints(ThreatPoints, FBox::BuildAABB(GetOwningCharacter()->GetActorLocation(), FVector(ClampedXY, ClampedXY, ClampedZ)));

	if (ThreatPoints.Num() > 0)
	{
		if (bLookUp)
		{
			ThreatPoints.Sort([&](const FThreatAwarenessDataOctreeElement& Lhs, const FThreatAwarenessDataOctreeElement& Rhs)
			{
				return Lhs.Data->Location.Z > Rhs.Data->Location.Z;
			});
		}
		else
		{
			ThreatPoints.Sort([&](const FThreatAwarenessDataOctreeElement& Lhs, const FThreatAwarenessDataOctreeElement& Rhs)
			{
				return Lhs.Data->Location.Z < Rhs.Data->Location.Z;
			});
		}
		
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
		//CollisionQueryParams.AddIgnoredActor(GetOwningCharacter()->GetCharacterMovement()->CurrentFloor.HitResult.GetActor());
		
		for (const FThreatAwarenessDataOctreeElement& Element : ThreatPoints)
		{
			if (Element.Data->ThreatLevel < EThreatLevel::TL_Stairs)
				continue;
			
			const FVector ThreatLocation = Element.Data->Location;

			FVector DirectionToLookAtPoint = (ThreatLocation - GetOwningCharacter()->GetNavAgentLocation()).GetSafeNormal();

			if (ThreatTrackingIgnoredDirection != FVector::ZeroVector)
			{
				if (FVector::DotProduct(ThreatTrackingIgnoredDirection, DirectionToLookAtPoint) > 0.0f)
					continue;
			}
			
			if (bLookUp)
			{
				// ignore everything below us
				if (GetOwningCharacter()->GetActorLocation().Z > ThreatLocation.Z)
				{
					continue;
				}
				
				if (FMath::Abs(ThreatLocation.Z - (GetOwningCharacter()->GetActorLocation().Z)) < 200.0f)
				{
					continue;
				}
			}
			else
			{
				// ignore everything above us
				if (GetOwningCharacter()->GetActorLocation().Z < ThreatLocation.Z)
				{
					continue;
				}
				
				if (FMath::Abs(ThreatLocation.Z - GetOwningCharacter()->GetActorLocation().Z) < 150.0f)
				{
					continue;
				}
			}

			FVector Offset = FVector(0.0f, 0.0f, 180.0f);
			if (!bLookUp)
				Offset = FVector(0.0f, 0.0f, 130.0f);
			
			if (GetWorld()->LineTraceTestByChannel(Element.Data->Location, GetOwningCharacter()->GetActorLocation() + Offset, ECC_Visibility, CollisionQueryParams))
			{
				continue;
			}

			StairThreat = Element.Data->ThreatAwarenessActor.Get();
			break;
		}
	}

	if (StairThreat)
	{
		GetOwningController()->SetFocalPoint(StairThreat->GetActorLocation());
		
		#if !UE_BUILD_SHIPPING
		DrawDebugBox(GetWorld(), GetOwningController()->GetFocalPoint(), FVector(15.0f), FColor::Red, false, 0.1f, 0, 2.0f);
		#endif
		
		return true;
	}

	return false;
}

bool UTargetingComponent::TrackThreatAwarenessActors()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackThreatAwarenessActors);

	USWATManager* SwatManager = USWATManager::Get(this);

	if (SwatManager)
	{
		if (ASWATCharacter* Swat = SwatManager->IsLookAtPointOccupied(LastLookAtPoint))
		{
			if (Swat == GetOwningCharacter())
			{
				SwatManager->OccupiedLookAtPoints.Remove(LastLookAtPoint);
			}
		}
	}
	
	CurrentThreatLookAtPoint = nullptr;
	
	if (!NearestThreat)
		return false;

	if (!GetOwningCharacter()->IsOnSWATTeam())
		return false;
	
	const FLookAtPoint* BestLookAtPoint = nullptr;

	const FVector AverageSwatLocation = USWATManager::Get(this)->GetAverageSwatLocation();

	if (AverageSwatLocation == FVector::ZeroVector)
		return false;
	
	#if !UE_BUILD_SHIPPING
	if (CVarRonDrawSwatQuadrantCoverage.GetValueOnAnyThread() > 0)
	{
		DrawDebugBox(GetWorld(), AverageSwatLocation, FVector(10.0f), FColor::Magenta, false, 0.099f, 0, 1.0f);
		DrawDebugBox(GetWorld(), NearestThreat->GetActorLocation(), FVector(5.0f), FColor::Orange, false, 0.099f, 0, 1.0f);
		DrawDebugLine(GetWorld(), GetOwningCharacter()->GetActorLocation(), NearestThreat->GetActorLocation(), FColor::Orange, false, 0.099f, 0, 1.0f);
	}
	#endif
	
	//DrawDebugLine(GetWorld(), GetOwningCharacter()->GetActorLocation(), GetOwningCharacter()->GetActorLocation() + ThreatTrackingIgnoredDirection * 100.0f, FColor::Orange, false, 0.025f, 0, 1.0f);
	
	AThreatAwarenessActor* ChosenTAA = NearestThreat;
	
	// favor threat points that have more than one look at point
	if (NearestThreat->SwatLookAtPoints.Num() == 1)
	{
		if (AThreatAwarenessActor* TAA = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(GetOwningCharacter()->GetNavAgentLocation(), NearestThreat->PathableThreatAwarenessActors))
		{
			ChosenTAA = TAA;
		}
	}

	TArray<const FLookAtPoint*> DoorwayLookAtPoints;
	for (const FLookAtPoint& LookAtPoint : NearestThreat->SwatLookAtPoints)
	{
		if (LookAtPoint.LinkedDoor && (LookAtPoint.LinkedDoor->IsDoorwayOnly() || LookAtPoint.LinkedDoor->IsOpenBeyondIncrementThreshold()))
		{
			float ZHeightDifference = FMath::Abs(LookAtPoint.LinkedDoor->GetActorLocation().Z - GetOwningCharacter()->GetActorLocation().Z);
			if (ZHeightDifference < 200.0f)
			{
				// is stacked on?
				bool bIsStackedOn = false;
				UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
				{
					if (Activity->GetStackUpDoor() == LookAtPoint.LinkedDoor && !Activity->IsActivityComplete())
					{
						bIsStackedOn = true;
						return false;
					}

					return true;
				});

				if (!bIsStackedOn)
				{
					// can't be fucked making them non-const. i just need to modify the Z so swat wont look up if the mid door location is above their head!!!
					const_cast<FLookAtPoint*>(&LookAtPoint)->Location.Z = GetOwningCharacter()->GetActorLocation().Z + 50.0f;
					DoorwayLookAtPoints.Add(&LookAtPoint);
				}
			}
		}
	}

	/*
	for (const FLookAtPoint& LookAtPoint : ChosenTAA->SwatLookAtPoints)
	{
		DrawDebugLine(GetWorld(), ChosenTAA->GetActorLocation(), FVector(LookAtPoint.Location), FColor::Orange, false, 1.0f);
	}
	*/
	
	TArray<const FLookAtPoint*> DoorLookAtPoints;
	for (const FLookAtPoint& LookAtPoint : ChosenTAA->SwatLookAtPoints)
	{
		if (LookAtPoint.LinkedDoor && (!LookAtPoint.LinkedDoor->IsDoorwayOnly() || !LookAtPoint.LinkedDoor->IsOpenBeyondIncrementThreshold()))
		{
			float ZHeightDifference = FMath::Abs(LookAtPoint.LinkedDoor->GetActorLocation().Z - GetOwningCharacter()->GetActorLocation().Z);
			if (ZHeightDifference < 200.0f)
			{
				// is stacked on?
				bool bIsStackedOn = false;
				UActivityManager::IterateAllActivitiesOfType<UTeamStackUpActivity>([&](UTeamStackUpActivity* Activity)
				{
					if (Activity->GetStackUpDoor() == LookAtPoint.LinkedDoor && !Activity->IsActivityComplete())
					{
						bIsStackedOn = true;
						return false;
					}

					return true;
				});

				if (!bIsStackedOn)
				{
					DoorLookAtPoints.Add(&LookAtPoint);
				}
			}
		}
	}
	
	TArray<const FLookAtPoint*> NonDoorLookAtPoints;
	for (const FLookAtPoint& LookAtPoint : ChosenTAA->SwatLookAtPoints)
	{
		if (!LookAtPoint.LinkedDoor)
		{
			NonDoorLookAtPoints.Add(&LookAtPoint);
		}
	}

	FCollisionQueryParams QueryParameters = GetOwningCharacter()->GetCollisionQueryParameters();
	if (SwatManager)
		QueryParameters.AddIgnoredActor(SwatManager->GetSquadLeader());
	QueryParameters.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters);
	//QueryParameters.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllDoors);

	const auto FindBestLookAtPoint = [&](const TArray<const FLookAtPoint*>& InPoints, bool bSkipOccupyChecks = false)
	{
		float BestDotProduct = 1.0f;
		float ClosestThreatDistance = FLT_MAX;

		for (const FLookAtPoint* LookAtPoint : InPoints)
		{
			FVector LookAtLocation = FVector(LookAtPoint->Location);

			const ASWATCharacter* Swat = SwatManager->IsLookAtPointOccupied(LookAtPoint->Location);
			
			if (!bSkipOccupyChecks)
			{
				if (Swat && Swat != GetOwningCharacter())
				{
					// am i closer? if so, allow it
					if (FVector::Distance(GetOwningCharacter()->GetActorLocation(), LookAtLocation) < FVector::Distance(Swat->GetActorLocation(), LookAtLocation))
					{
						
					}
					else
					{
						continue;
					}
				}
			}

			const float Distance = FVector::Distance(GetOwningCharacter()->GetActorLocation(), LookAtLocation); 
			if (Distance > 2000.0f)
				continue;
			
			FVector DirectionToLookAtPoint = (LookAtLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal();

			if (GetOwningController()->GetCurrentActivity() && ThreatTrackingIgnoredDirection != FVector::ZeroVector)
			{
				if (FVector::DotProduct(ThreatTrackingIgnoredDirection, DirectionToLookAtPoint) > 0.0f)
					continue;
			}

			if (!bSkipOccupyChecks)
			{
				// any occupied look at point is close to this one?
				bool bAnyCloseBy = false;
				for (const auto& It : USWATManager::Get(this)->OccupiedLookAtPoints)
				{
					if (It.Value != Swat)
					{
						if (FVector::Distance(FVector(It.Key), LookAtLocation) < 50.0f)
						{
							bAnyCloseBy = true;
							break;
						}
					}
				}

				if (bAnyCloseBy)
				{
					continue;
				}

				// is any swat near the lookat point
					
				for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
				{
					if (swat == GetOwningCharacter())
						continue;
					
					float DistanceToPoint = FVector::Distance(LookAtLocation, swat->GetActorLocation());
					if (DistanceToPoint < 150.0f)
					{
						bAnyCloseBy = true;
						break;
					}
				}

				if (bAnyCloseBy)
					continue;
			
				// is view blocked by other swat?
				float ClosestDistance = 1000.0f;
				for (ASWATCharacter* swat : USWATManager::Get(this)->SwatAI)
				{
					if (swat == GetOwningCharacter())
						continue;
					
					if (FVector::DotProduct((LookAtLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal(), (swat->GetActorLocation() - GetOwningCharacter()->GetActorLocation()).GetSafeNormal()) > 0.0f)
					{
						FVector Point = FMath::ClosestPointOnLine(GetOwningCharacter()->GetActorLocation(), LookAtLocation, swat->GetActorLocation());
						float DistanceToPoint = FVector::Distance(Point, swat->GetActorLocation());
						if (DistanceToPoint < ClosestDistance)
						{
							ClosestDistance = DistanceToPoint;
						}
					}
				}

				if (ClosestDistance < 100.0f)
				{
					continue;
				}
			}

			FVector DirectionToAverageLocation = (AverageSwatLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();

			const float DotProduct = FVector::DotProduct(DirectionToAverageLocation, DirectionToLookAtPoint);

			if (DotProduct < BestDotProduct && Distance < ClosestThreatDistance)
			{
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
				const FVector EndTrace = LookAtLocation;

				FCollisionQueryParams Temp = QueryParameters;
				Temp.AddIgnoredActor(LookAtPoint->LinkedDoor);
				const bool bHasLOS = !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, Temp);
				
				#if !UE_BUILD_SHIPPING
				if (CVarRonDrawSwatQuadrantCoverage.GetValueOnAnyThread() > 0)
				{
					DrawDebugLine(GetWorld(), StartTrace, EndTrace, bHasLOS ? FColor::Green : FColor::Red, false, 0.099f, 0, 0.0f);
				}
				#endif
				
				if (bHasLOS)
				{
					BestDotProduct = DotProduct;
					BestLookAtPoint = LookAtPoint;
					ClosestThreatDistance = Distance;
				}
				/*
				#if !UE_BUILD_SHIPPING
				else
				{
					DrawDebugBox(GetWorld(), EndTrace, FVector(15.0f), FColor::Red, false, 1.0f);

					FString Msg;
					Msg += GetOwningCharacter()->GetName() + " tried to look here but hit something";
					DrawDebugString(GetWorld(), EndTrace, Msg, nullptr, FColor::White, 1.0f, true);
				}
				#endif
				*/
			}
		}

		return BestLookAtPoint;
	};

	BestLookAtPoint = FindBestLookAtPoint(DoorwayLookAtPoints);
	if (!BestLookAtPoint)
		BestLookAtPoint = FindBestLookAtPoint(DoorLookAtPoints);
	if (!BestLookAtPoint)
		BestLookAtPoint = FindBestLookAtPoint(NonDoorLookAtPoints);

	// do it again but with no restrictions
	bool bGotThreat = BestLookAtPoint != nullptr;
	if (!bGotThreat)
	{
		float BestDotProduct = 1.0f;
		
		for (const FLookAtPoint& LookAtPoint : ChosenTAA->SwatLookAtPoints)
		{
			FVector LookAtLocation = FVector(LookAtPoint.Location);

			FVector v1 = (AverageSwatLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();
			FVector v2 = (LookAtLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal();
			
			if (GetOwningController()->GetCurrentActivity() &&
				ThreatTrackingIgnoredDirection != FVector::ZeroVector)
			{
				//bool bIsAllowedToTrack = LookAtPoint.LinkedDoor && AllowedTrackingDoors.Contains(LookAtPoint.LinkedDoor);
				if (/*!bIsAllowedToTrack && */FVector::DotProduct(ThreatTrackingIgnoredDirection, v2) > 0.0f)
					continue;
			}

			if (FVector::Distance(GetOwningCharacter()->GetActorLocation(), LookAtLocation) < 100.0f)
				continue;
			
			const float DotProduct = FVector::DotProduct(v1, v2);
			if (DotProduct < BestDotProduct)
			{
				FHitResult Hit;
				FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
				FVector EndTrace = LookAtLocation;
				GetWorld()->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, FCollisionObjectQueryParams(ECC_WorldStatic));
				
				#if !UE_BUILD_SHIPPING
				if (CVarRonDrawSwatQuadrantCoverage.GetValueOnAnyThread() > 0)
				{
					DrawDebugLine(GetWorld(), StartTrace, EndTrace, !Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 0.099f, 0, 0.0f);
				}
				#endif
				
				if (!Hit.bBlockingHit || Hit.Distance > 400.0f)
				{
					BestDotProduct = DotProduct;
					BestLookAtPoint = &LookAtPoint;
					bGotThreat = true;
				}
			}
		}
	}

	if (!bGotThreat)
	{
		if (NearestThreat->SwatLookAtPoints.Num() > 0 && NearestThreat->SwatLookAtPoints.Num() <= 2)
		{
			BestLookAtPoint = FindBestLookAtPoint(DoorwayLookAtPoints, true);
			if (!BestLookAtPoint)
				BestLookAtPoint = FindBestLookAtPoint(DoorLookAtPoints, true);
			if (!BestLookAtPoint)
				BestLookAtPoint = FindBestLookAtPoint(NonDoorLookAtPoints, true);
			
			if (BestLookAtPoint)
				bGotThreat = true;
		}
	}
	
	if (bGotThreat)
	{
		TimeSinceGotLastThreatAwarenessActor = 0.0f;
		LastLookAtPoint = BestLookAtPoint->Location;
		CurrentThreatLookAtPoint = BestLookAtPoint;
		GetOwningController()->SetFocalPoint(FVector(BestLookAtPoint->Location));

		if (!SwatManager->OccupiedLookAtPoints.Contains(BestLookAtPoint->Location))
			SwatManager->OccupiedLookAtPoints.Add(BestLookAtPoint->Location, Cast<ASWATCharacter>(GetOwningCharacter()));

		#if !UE_BUILD_SHIPPING
		if (CVarRonDrawSwatQuadrantCoverage.GetValueOnAnyThread() > 0)
		{
			DrawDebugLine(GetWorld(), GetOwningCharacter()->GetActorLocation(), GetOwningController()->GetFocalPoint(), FColor::White, false, 0.099f, 0, 1.0f);
			DrawDebugBox(GetWorld(), GetOwningController()->GetFocalPoint(), FVector(10.0f), FColor::Cyan, false, 0.099f, 0, 2.0f);
		}
		#endif
	}

	return bGotThreat;
}

bool UTargetingComponent::TrackThreatAwarenessActors_V2()
{
	if (!ThreatLookPoint)
		return false;

	FVector ThreatLocation = ThreatLookPoint->GetActorLocation();
	
	if (ThreatLookPoint->IsDoorThreat())
		ThreatLocation = ThreatLookPoint->GetAttachedDoor()->GetDoorMidLocation();

	GetOwningController()->SetFocalPoint(ThreatLocation);
	return true;
}

bool UTargetingComponent::TrackNearestDoor()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackNearestDoor);
	
	if (!GetOwningCharacter()->IsStrafing())
		return false;
	
	if (!GetOwningCharacter()->IsActiveForMovement())
		return false;
	
	if (GetOwningCharacter()->IsTakingHostage() ||
		GetOwningCharacter()->IsBeingTakenHostage())
		return false;

	if (LastTrackedDoor && LastTrackedDoor->IsHalfwayOpen() && GetOwningController()->DoesPathGoThroughDoor(LastTrackedDoor))
	{
		FVector ClosestPoint = LastTrackedDoor->CalculateClosestPoint(GetOwningCharacter()->GetActorLocation());
		if (FVector::Distance(GetOwningCharacter()->GetActorLocation(), ClosestPoint) < 100.0f)
		{
			TrackMoveVector();
			return false;
		}
	}
	
	const bool bIsMoveStyleStrafe = GetOwningCharacter()->MoveStyle && GetOwningCharacter()->MoveStyle->bIsStrafing;
	if (!bIsMoveStyleStrafe && GetOwningController()->IsMoving())
		return false;

	FVector MoveFocus = GetOwningCharacter()->GetActorLocation() + GetOwningCharacter()->GetVelocity().GetClampedToMaxSize(50.0f);

	if (GetOwningCharacter()->GetVelocity().IsNearlyZero())
	{
		MoveFocus = GetOwningCharacter()->GetActorLocation() + GetOwningCharacter()->GetActorForwardVector() * 500.0f;
	}

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		//FCollisionQueryParams QueryParams = GetOwningCharacter()->GetCollisionQueryParameters();
		//QueryParams.AddIgnoredActors((TArray<AActor*>)GS->AllDoors);
		
		ADoor* NearestOpenDoor = nullptr;
		float NearestDist = 1000.0f;
		
		for (ADoor* Door : GS->AllDoors)
		{
			if (ADoor* SubDoor = Door->GetSubDoor())
			{
				if (SubDoor->IsOpen() && Door->IsClosed())
				{
					Door = SubDoor;
				}
			}

			FVector ClosestDoorLocation = Door->CalculateClosestPoint(GetOwningCharacter()->GetActorLocation());

			const float Dist = FVector::Distance(ClosestDoorLocation, GetOwningCharacter()->GetActorLocation());
			if (Dist < 50.0f)
				continue;

			if (Door->IsDoorwayOnly() || Door->IsOpenBeyondIncrementThreshold() || Door->IsOpening() || Door->IsClosing())
			{
				if (Dist < NearestDist)
				{
					FVector V1 = (MoveFocus - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();
					FVector V2 = (ClosestDoorLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();

					const float DotProduct = FVector::DotProduct(V1, V2);

					// Dont track doors if we're moving away
					if (GetOwningCharacter()->GetVelocity().SizeSquared() > 625.0f && DotProduct < 0.0f)
						continue;

					/*
					FVector DoorLocation = Door->GetDoorMidLocation() + (Door->IsActorInFrontOfDoorway(GetOwningCharacter()) ? Door->GetActorForwardVector() * 50.0f : Door->GetActorForwardVector() * -50.0f);

					const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
					const FVector EndTrace = DoorLocation;
					bool bHasLOS = !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, QueryParams);
					*/

					FName CurrentRoom = NearestThreat ? NearestThreat->OwningRoom : NAME_None;

					FName DoorRoom;
					if (Door->IsActorInFrontOfDoorway(GetOwningCharacter()))
					{
						if (!Door->FrontThreat)
							continue;
						
						DoorRoom = Door->FrontThreat->OwningRoom;
					}
					else
					{
						if (!Door->BackThreat)
							continue;
						
						DoorRoom = Door->BackThreat->OwningRoom;
					}

					bool bSameRoom = CurrentRoom == DoorRoom;
					
					//if (bHasLOS)// && (!GetOwningController()->IsMoving() || DotProduct > 0.0f))
					if (bSameRoom && (!GetOwningController()->IsMoving() || DotProduct > 0.0f))
					{
						NearestDist = Dist;
						NearestOpenDoor = Door;
					}
				}
			}
		}
		
		if (NearestOpenDoor)
		{
			LastTrackedDoor = NearestOpenDoor;
			GetOwningController()->SetFocus(NearestOpenDoor);
			return true;
		}
		
		ADoor* NearestDoor = nullptr;
		NearestDist = 1000.0f;
		for (ADoor* Door : GS->AllDoors)
		{
			if (ADoor* SubDoor = Door->GetSubDoor())
			{
				if (SubDoor->IsOpen() && Door->IsClosed())
				{
					Door = SubDoor;
				}
			}
			
			FVector ClosestDoorLocation = Door->CalculateClosestPoint(GetOwningCharacter()->GetActorLocation());
			
			const float Dist = FVector::Distance(ClosestDoorLocation, GetOwningCharacter()->GetActorLocation());
			if (Dist < 50.0f)
				continue;
			
			//ULog::Info(Door->GetName() + ": " + FString::SanitizeFloat(Dist));
					
			if ((Dist < NearestDist || Door->IsDoorwayOnly() || Door->IsOpenBeyond(0.5f)))
			{
				//ULog::Info("inside " + Door->GetName() + ": " + FString::SanitizeFloat(Dist));
				FVector V1 = (MoveFocus - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();
				FVector V2 = (ClosestDoorLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal2D();

				const float DotProduct = FVector::DotProduct(V1, V2);

				// Dont track doors if we're moving away
				if (GetOwningCharacter()->GetVelocity().SizeSquared() > 625.0f && DotProduct < 0.0f)
					continue;

				/*
				FVector DoorLocation = Door->GetDoorMidLocation() + (Door->IsActorInFrontOfDoorway(GetOwningCharacter()) ? Door->GetActorForwardVector() * 50.0f : Door->GetActorForwardVector() * -50.0f);
				
				const FVector StartTrace = GetOwningCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = DoorLocation;
				FHitResult OutHit;
				bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(OutHit, StartTrace, EndTrace, ECC_Visibility, QueryParams);
				*/
				
				FName CurrentRoom = NearestThreat ? NearestThreat->OwningRoom : NAME_None;

				FName DoorRoom;
				if (Door->IsActorInFrontOfDoorway(GetOwningCharacter()))
				{
					if (!Door->FrontThreat)
						continue;
					
					DoorRoom = Door->FrontThreat->OwningRoom;
				}
				else
				{
					if (!Door->BackThreat)
						continue;

					DoorRoom = Door->BackThreat->OwningRoom;
				}

				bool bSameRoom = CurrentRoom == DoorRoom;
				
				//if (bHasLOS)// && (!GetOwningController()->IsMoving() || DotProduct > 0.0f))
				if (bSameRoom && (!GetOwningController()->IsMoving() || DotProduct > 0.0f))
				{
					NearestDist = Dist;
					NearestDoor = Door;
				}
			}
		}
		
		if (NearestDoor)
		{
			LastTrackedDoor = NearestDoor;
			GetOwningController()->SetFocus(NearestDoor);
			return true;
		}
	}
	
	return false;
}

bool UTargetingComponent::TrackOverrideInterests()
{
	if (CurrentInterestZone)
	{
		FVector Location = FVector::ZeroVector;
		AActor* Actor = nullptr;
		if (CurrentInterestZone->GetCurrentInterestInfo(GetOwningCharacter(), Location, Actor))
		{
			if (Actor)
			{
				GetOwningController()->SetFocus(Actor);
				return true;
			}
			
			if (Location != FVector::ZeroVector)
			{
				GetOwningController()->SetFocalPoint(Location);
				return true;
			}
		}
	}

	return false;
}

bool UTargetingComponent::TrackNoiseRelatedEvents()
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_TrackNoiseRelatedEvents);
	
	if (GetOwningCharacter()->IsHesitating())
		return false;

	// Don't track noise stimulus whilst in cover, will misalign rotations.
	// Manually handled in the activity
	if (GetOwningCharacter()->IsTakingCover() ||
		GetOwningCharacter()->IsMovingToCover() ||
		GetOwningCharacter()->IsMovingToLandmarkCover() ||
		GetOwningCharacter()->IsTakingCoverAtLandmark())
		return false;

	if (GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UFlankingCombatMove>())
		return false;

	if (GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UChargeCombatMove>())
		return false;

	if (GetOwningController()->GetCombatActivity()->GetCombatMoveActivity<UHardCoverCombatMove>())
		return false;

	if (GetOwningController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
		return false;

	const bool bIsMoveStyleStrafe = GetOwningCharacter()->MoveStyle && GetOwningCharacter()->MoveStyle->bIsStrafing;
	if (!bIsMoveStyleStrafe && GetOwningController()->IsMoving())
		return false;
	
	if (LastHeardNoiseStimulus.Instigator == GetOwningCharacter())
		return false;
	
	if (LastHeardNoiseStimulus.Instigator)
	{
		if (GetOwningCharacter()->IsOnSWATTeam() && LastHeardNoiseStimulus.Instigator->IsOnSWATTeam())
		{
			return false;
		}

		if (GetOwningCharacter()->IsOnSWATTeam())
		{
			if (!LastHeardNoiseStimulus.Instigator->IsActive())
			{
				return false;
			}
		}
	}
	
	if (LastHeardNoiseStimulus.StimulusLocation != FVector::ZeroVector && TimeSinceLastHeardNoiseStimulus < LastHeardNoiseStimulus.ExpiryTime)
	{
		if (!GetOwningController()->IsMoving()) // only allow when not moving
		{
			if (const FPathAwarenessInfo* Info = LatestPathedAwareness.Find(EPathedAwareness::PA_Noise))
			{
				if (Info->Actor)
				{
					GetOwningController()->SetFocus(Info->Actor);
					return true;
				}
				
				GetOwningController()->SetFocalPoint(Info->Location);
				return true;
			}
			
			GetOwningController()->SetFocalPoint(LastHeardNoiseStimulus.StimulusLocation);
			return true;
		}
		
		// focus on door if near one // TODO: something a little bit better than this?? feels kinda dirty
		/*
		if (const AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (const FNavPathSharedPtr NavPath = GetOwningController()->GetRONPathFollowingComp()->GetPath())
			{
				if (NavPath.IsValid())
				{
					for (FNavPathPoint& NavPathPoint : NavPath->GetPathPoints())
					{
						if (const ADoor* ClosestDoor = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(NavPathPoint.Location, GS->AllDoors))
						{
							const float Distance = FVector::Distance(ClosestDoor->GetActorLocation(), GetOwningCharacter()->GetActorLocation());
							if (Distance > 200.0f && Distance < 500.0f &&
								FVector::Distance(NavPathPoint.Location, ClosestDoor->GetActorLocation()) < 300.0f)
							{
								return false;
							}
						}
					}
				}
			}
		}
		*/
	}
	
	return false;
}

void UTargetingComponent::FindAwarenessPath(FVector StimulusLocation, EPathedAwareness AwarenessType)
{
	SCOPE_CYCLE_COUNTER(STAT_TargetingComp_FindAwarenessPath);
	
	if (!FAISystem::IsValidLocation(StimulusLocation))
		return;

	if (PathAwarenessSearchTimeout[AwarenessType] > 0.0f)
		return;

	PathAwarenessSearchTimeout[AwarenessType] = 0.5f;
	
	if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(GetOwningCharacter()->GetMovementComponent()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetOwningCharacter(), UNavQuery_Awareness::StaticClass());
			
			FNavLocation ProjectedStartLocation(GetOwningCharacter()->GetNavAgentLocation());
			NavSys->ProjectPointToNavigation(GetOwningCharacter()->GetNavAgentLocation(), ProjectedStartLocation, FVector(75.0f, 75.0f, 200.0f));
			
			FNavLocation ProjectedEndLocation(StimulusLocation);
			NavSys->ProjectPointToNavigation(StimulusLocation, ProjectedEndLocation, FVector(75.0f, 75.0f, 200.0f));
			
			FPathFindingQuery PathFindingQuery;
			PathFindingQuery.SetAllowPartialPaths(true);
			PathFindingQuery.QueryFilter = QueryFilter;
			PathFindingQuery.NavData = NavData;
			PathFindingQuery.StartLocation = ProjectedStartLocation.Location;
			PathFindingQuery.EndLocation = ProjectedEndLocation.Location;
			
			FNavPathQueryDelegate NavDelegate;
			NavDelegate.BindUObject(this, &UTargetingComponent::OnAwarenessPathFound);
			
			const uint32 QueryId = NavSys->FindPathAsync(GetOwningCharacter()->GetNavAgentPropertiesRef(), PathFindingQuery, NavDelegate, EPathFindingMode::Regular);
			
			PathedAwarenessQueryType.Add(QueryId, AwarenessType);
			bSearchingPathAwareness = true;
			LastSearchedPathedAwareness.Add(AwarenessType, StimulusLocation);
		}
	}
}

void UTargetingComponent::OnAwarenessPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	bSearchingPathAwareness = false;
	if (!PathedAwarenessQueryType.Find(PathId))
	{
		// something went wrong
		return;
	}
	
	if (ResultType != ENavigationQueryResult::Success)
	{
		// fail safes to look directly at if we cannot get a path point to it
		switch (PathedAwarenessQueryType[PathId])
		{
			case EPathedAwareness::PA_None: break;
			case EPathedAwareness::PA_Noise:					LatestPathedAwareness.Add(EPathedAwareness::PA_Noise, FPathAwarenessInfo(LastHeardNoiseStimulus.Instigator, LastHeardNoiseStimulus.StimulusLocation, GetOwningCharacter()->GetActorLocation())); break;
			case EPathedAwareness::PA_LastKnownEnemyPosition:	LatestPathedAwareness.Add(EPathedAwareness::PA_LastKnownEnemyPosition, FPathAwarenessInfo(nullptr, LastKnownTargetPosition, GetOwningCharacter()->GetActorLocation())); break;
			default: break;
		}
	
		#if WITH_EDITOR
		if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
		{
			DrawDebugLine(GetWorld(), NavPath->GetStartLocation(), NavPath->GetEndLocation(), FColor::Red, false, 1.0f, 0 , 1.0f);
			DrawDebugPoint(GetWorld(), NavPath->GetStartLocation(), 50.0f, FColor::Red, false, 1.0f, 0);
			DrawDebugPoint(GetWorld(), NavPath->GetEndLocation(), 50.0f, FColor::Red, false, 1.0f, 0);
		}
		#endif
	}
	else
	{
		LatestPathedAwareness.Remove(PathedAwarenessQueryType[PathId]);
		CalculatePathedAwareness(PathedAwarenessQueryType[PathId], ResultType, NavPath);
	}
	
	PathedAwarenessQueryType.Remove(PathId);
}

void UTargetingComponent::CalculatePathedAwareness(EPathedAwareness AwarenessType, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	if (!GetOwningCharacter())
		return;
	
	FVector FallbackLocation = NavPath->GetEndLocation() + FVector(0.0f, 0.0f, 120.0f);
	if (NavPath->GetPathPoints().Num() > 3)
	{
		// get a path not our closest but pretty close unless path points are near
		// this helps AI look at a door instead of trying to look through a wall at a sound source
		#if WITH_EDITOR
		if (CVarRonDrawAIDebug.GetValueOnAnyThread() > 0)
		{
			DrawDebugPoint(GetWorld(), NavPath->GetPathPoints()[3].Location  + FVector(0.0f, 0.0f, 120.0f), 30.0f, FColor::Green, false, 5.0f, 0);
			DrawDebugString(GetWorld(), NavPath->GetPathPoints()[3].Location  + FVector(0.0f, 0.0f, 200.0f), "Awareness Type: " +  RON_ENUM_TO_STRING(EPathedAwareness, AwarenessType), nullptr, FColor::White, 5.0f);
		}
		#endif
		
		FallbackLocation = NavPath->GetPathPoints()[3].Location + FVector(0.0f, 0.0f, 120.0f);
	}
	
	ADoor* TheDoor = nullptr;

	if (const FRoom* CurrentRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(GetOwningCharacter()->GetActorLocation()))
	{
		TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
		if (PathPoints.Num() > 1)
		{
			//PathPoints.Pop(); // ignore last path point

			/*
			FColor DebugColor = FColor::Red;
			
			for (uint32 i = PathPoints.Num()-1; i > 0; i--)
			{
				FVector Start = PathPoints[i].Location + FVector(0.0f, 0.0f, 10.0f);
				FVector End = PathPoints[i-1].Location + FVector(0.0f, 0.0f, 10.0f);
				
				DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 1.0f, 0, 1.25f);
			}
			*/

			const TArray<ADoor*>& AllDoors = CurrentRoom->AdditionalRootDoors;
			for (ADoor* Door : AllDoors)
			{
				if (!IsValid(Door))
					continue;
				
				for (uint8 i = 0; i < PathPoints.Num(); i++)
				{
					int32 CurrentPathIndex = i;
					int32 NextPathIndex = i+1;
					
					if (PathPoints.IsValidIndex(CurrentPathIndex) && PathPoints.IsValidIndex(NextPathIndex))
					{
						FVector DoorLocation = Door->GetDoorMidLocation();
						DoorLocation.Z = Door->GetActorLocation().Z;
					
						FVector Start = PathPoints[CurrentPathIndex].Location;
						FVector End = PathPoints[NextPathIndex].Location;
					
						if (FMath::LineSphereIntersection(Start, (PathPoints[NextPathIndex].Location - PathPoints[CurrentPathIndex].Location).GetSafeNormal(), FVector::Distance(Start, End), DoorLocation, Door->GetDoorSize().Y))
						{
							TheDoor = Door;
							//DrawDebugPoint(GetWorld(), Start, 20.0f, FColor::Red, false, 1.0f);
							break;
						}
					}
				}
				
				if (TheDoor)
				{
					break;
				}
			}

			// fallback incase the navlink is disabled on the door (if locked/wedged for example)
			if (!TheDoor)
			{
				for (ADoor* Door : AllDoors)
				{
					if (!IsValid(Door))
						continue;
					
					FVector DoorLocation = Door->GetDoorMidLocation();
					DoorLocation.Z = Door->GetActorLocation().Z;
					
					//DrawDebugPoint(GetWorld(), DoorLocation, 50.0f, FColor::Purple, false, 1.0f, 0);
					//DrawDebugPoint(GetWorld(), NavPath->GetEndLocation(), 50.0f, FColor::Purple, false, 1.0f, 0);

					float Distance = FVector::Distance(NavPath->GetEndLocation(), DoorLocation);
					//LOG_NUMBER(Distance);
					if (Distance < 200.0f)
					{
						TheDoor = Door;
						break;
					}
				}
			}
		}
	}

	FVector Location = FallbackLocation;
	if (TheDoor)
	{
		if (TheDoor->GetSubDoor() && TheDoor->IsNonMainSubdoor())
			TheDoor = TheDoor->GetSubDoor();
		
		Location = TheDoor->GetDoorMidLocation();
		
		// offset it a bit
		if (FVector::Distance(GetOwningCharacter()->GetActorLocation(), TheDoor->GetDoorMidLocation()) < 300.0f)
		{
			if (TheDoor->IsPointInFrontOfDoorway(GetOwningCharacter()->GetActorLocation()))
			{
				Location += TheDoor->GetActorForwardVector() * -200.0f;
			}
			else
			{
				Location += TheDoor->GetActorForwardVector() * 200.0f;
			}
		}
	}
	
	LatestPathedAwareness.FindOrAdd(AwarenessType, FPathAwarenessInfo(TheDoor, Location, GetOwningCharacter()->GetActorLocation()));
}

void UTargetingComponent::SetFocusActor(AActor* Actor)
{
	GetOwningController()->SetFocus(Actor);
}

void UTargetingComponent::AddKnownEnemy(AReadyOrNotCharacter* Enemy, const bool bForce)
{
	if (!IsValid(Enemy))
		return;

	// can't make an enemy of yourself now..
	if (Enemy == GetOwningCharacter())
		return;

	if (Enemy->GetTeam() == GetOwningController()->GetTeam() && !bForce)
		return;

	if (GetOwningController()->IsSWAT() && Cast<APlayerCharacter>(Enemy) && !bForce)
		return;

	if (Enemy->IsDeadOrUnconscious() || Enemy->IsArrested())
		return;
	
	KnownFriendlies.Remove(Enemy);
	KnownNeutrals.Remove(Enemy);
	
	if (!KnownEnemies.Contains(Enemy))
	{
		KnownEnemies.Add(Enemy);
		
		Enemy->OnCharacterKilled.RemoveAll(GetOwningController());
		Enemy->OnCharacterKilled.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownEnemyKilled);
		
		Enemy->OnCharacterIncapacitated.RemoveAll(GetOwningController());
		Enemy->OnCharacterIncapacitated.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownEnemyIncapacitated);
		
		Enemy->OnCharacterTakeDamage.RemoveAll(GetOwningController());
		Enemy->OnCharacterTakeDamage.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownEnemyTakeDamage);
		
		Enemy->OnStunnedEvent.RemoveAll(GetOwningController());
		Enemy->OnStunnedEvent.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownEnemyStunned);
	}
}

void UTargetingComponent::AddKnownFriendly(AReadyOrNotCharacter* Friendly)
{
	if (!Friendly)
		return;
	
	if (Friendly == GetOwningCharacter())
		return;
	
	if (!KnownFriendlies.Contains(Friendly))
	{
		KnownFriendlies.AddUnique(Friendly);
		
		Friendly->OnCharacterKilled.RemoveAll(GetOwningController());
		Friendly->OnCharacterKilled.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownFriendlyKilled);
		
		Friendly->OnCharacterIncapacitated.RemoveAll(GetOwningController());
		Friendly->OnCharacterIncapacitated.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownFriendlyIncapacitated);
		
		Friendly->OnCharacterTakeDamage.RemoveAll(GetOwningController());
		Friendly->OnCharacterTakeDamage.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownFriendlyTakeDamage);
		
		Friendly->OnStunnedEvent.RemoveAll(GetOwningController());
		Friendly->OnStunnedEvent.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownFriendlyStunned);
	}
}

void UTargetingComponent::AddKnownNeutral(AReadyOrNotCharacter* Neutral)
{
	if (!Neutral)
		return;

	if (!KnownNeutrals.Contains(Neutral))
	{
		KnownNeutrals.AddUnique(Neutral);
		
		Neutral->OnCharacterKilled.RemoveAll(GetOwningController());
		Neutral->OnCharacterKilled.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownNeutralKilled);
		
		Neutral->OnCharacterIncapacitated.RemoveAll(GetOwningController());
		Neutral->OnCharacterIncapacitated.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownNeutralIncapacitated);
		
		Neutral->OnCharacterTakeDamage.RemoveAll(GetOwningController());
		Neutral->OnCharacterTakeDamage.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownNeutralTakeDamage);
		
		Neutral->OnStunnedEvent.RemoveAll(GetOwningController());
		Neutral->OnStunnedEvent.AddDynamic(GetOwningController(), &ACyberneticController::OnKnownNeutralStunned);
	}
	
	for (TObjectIterator<UTargetingComponent> It; It; ++It)
	{
		UTargetingComponent* TargetingComp = *It;
		if (!TargetingComp || TargetingComp == this || TargetingComp->KnownNeutrals.Contains(Neutral))
			continue;
		
		if (KnownFriendlies.Contains(TargetingComp->GetOwningCharacter()))
		{
			TargetingComp->AddKnownNeutral(Neutral);
		}
	}
}

bool UTargetingComponent::IsTrackedByKnownFriendly(AReadyOrNotCharacter* Target)
{
	KnownFriendlies.Remove(nullptr);
	for (AReadyOrNotCharacter* Friendly : KnownFriendlies)
	{
		ACyberneticController* CyberneticController = Cast<ACyberneticController>(Friendly->GetController());
		if (CyberneticController)
		{
			if (CyberneticController->GetFocusActor() == Friendly || CyberneticController->GetFocalPoint() == GetOwningController()->GetFocalPointOnActor(Target))
			{
				return true;
			}
		}
	}
	return false;
}

int32 UTargetingComponent::GetVisibleKnownFriendlies()
{
	int32 VisibleFriendlies = 0;
	for (AReadyOrNotCharacter* Friendly : KnownFriendlies)
	{
		if (Friendly)
		{
			if (CanCharacterBeSeen(Friendly))
			{
				VisibleFriendlies++;
			}
		}
	}
	return VisibleFriendlies;
}

bool UTargetingComponent::IsTrackedInKnownEnemies(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return false;

	for (const AReadyOrNotCharacter* Target : KnownEnemies)
	{
		if (Target == PlayerCharacter)
			return true;
	}

	return false;
}

bool UTargetingComponent::IsTrackedInKnownFriendlies(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return false;

	return KnownFriendlies.Contains(PlayerCharacter);
}

bool UTargetingComponent::IsTrackedInKnownNeutrals(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return false;

	return KnownNeutrals.Contains(PlayerCharacter);
}

bool UTargetingComponent::IsLookingAtFocalPoint(float Tolerance)
{
	FRotator ActorRotation = GetOwningCharacter()->GetActorRotation();
	FVector ActorLocation = GetOwningCharacter()->GetActorLocation();
	FVector FocalPoint = GetOwningController()->GetFocalPoint();
	return ActorRotation.Equals(UKismetMathLibrary::FindLookAtRotation(ActorLocation, FocalPoint), Tolerance);
}

void UTargetingComponent::GatherDebugText(FString& OutText)
{
	OutText += " \nKnown Enemies\n--";
	for (AReadyOrNotCharacter* enemy : KnownEnemies)
	{
		if (enemy)
		{
			OutText += " \n"  + enemy->GetName();
		}

	}
	OutText += " \nKnown Friendlies\n--";
	for (AReadyOrNotCharacter* friendly : KnownFriendlies)
	{
		if (friendly)
		{
			OutText += " \n" + friendly->GetName();
		}

	}
	OutText += " \nKnown Neutrals--";
	for (AReadyOrNotCharacter* neutral : KnownNeutrals)
	{
		if (neutral)
		{
			OutText += " \n" + neutral->GetName();
		}
	}
}

void UTargetingComponent::SetLastHeardNoiseLocation(FExposedToNoise Noise)
{
	if (Noise.bAggressive)
	{
		LastHeardAggressiveNoise = Noise;
	}

	HeardNoises.Add(Noise.Tag, Noise);
	
	LastHeardNoiseStimulus = Noise;
	TimeSinceLastHeardNoiseStimulus = 0.0f;

	// is stimulus location behind us? don't find the awareness path
	const FVector DirectionToStimulus = (Noise.StimulusLocation - GetOwningCharacter()->GetActorLocation()).GetSafeNormal();
	const float Dot = FVector::DotProduct(DirectionToStimulus, GetOwningCharacter()->GetActorForwardVector());
	
	if (Dot > 0.25f)
	{
		if (Noise.Instigator)
		{
			FindAwarenessPath(Noise.Instigator->GetNavAgentLocation(), EPathedAwareness::PA_Noise);
		}
		else
		{
			FindAwarenessPath(Noise.StimulusLocation, EPathedAwareness::PA_Noise);
		}
	}
}

FExposedToNoise UTargetingComponent::GetLastNoiseByTag(FName Tag)
{
	return *HeardNoises.Find(Tag);
}

TArray<FName> UTargetingComponent::GetLastNoisesTags()
{
	TArray<FName> Keys;
	HeardNoises.GenerateKeyArray(Keys);
	return Keys;
}
