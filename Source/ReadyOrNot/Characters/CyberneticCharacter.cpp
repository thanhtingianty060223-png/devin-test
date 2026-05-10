// Copyright Void Interactive, 2022

#include "CyberneticCharacter.h"

#include "ReadyOrNotGameMode.h"
#include "GameModes/CoopGM.h"

#include "CyberneticController.h"

#include "ReadyOrNotAIConfig.h"

#include "SkinnedDecalSampler.h"

#include "Animation/MoveStyle/RoNMoveStyleComponent.h"

#include "AI/Archetypes/AIArchetypeData.h"

#include "Actors/Door.h"
#include "Actors/BaseGrenade.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/Items/Headwear.h"
#include "Actors/Items/Taser.h"
#include "Actors/Items/MeleeWeapon.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Actors/Gameplay/AISpawn.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/CoverLandmark.h"
#include "Actors/CoverLandmarkProxy.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"

#include "Components/InteractableComponent.h"
#include "Components/MoraleComponent.h"
#include "Components/ReadyOrNotCharMovementComp.h"
#include "Components/ObjectiveMarkerComponent.h"
#include "Components/ScoringComponent.h"
#include "Components/CharacterHealthComponent.h"

#include "DamageTypes/TrapDamage.h"
#include "DamageTypes/LessLethal/BeanbagDamageType.h"
#include "DamageTypes/BulletDamageType.h"
#include "DamageTypes/LessLethal/PepperballDamageType.h"
#include "DamageTypes/LessLethal/PepperSprayDamageType.h"

#include "Info/SuspectsAndCivilianManager.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/SWATManager.h"
#include "Info/ScoringManager.h"
#include "Info/Activities/MoveIntoLOSActivity.h"
#include "Info/Activities/PickupItemActivity.h"
#include "Info/Activities/PlayDeadActivity.h"
#include "Info/Activities/TakeCoverActivity.h"
#include "Info/Activities/Team/ArrestTargetActivity.h"
#include "Info/Activities/Team/DisarmDoorTrapActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/TraverseHoleActivity.h"
#include "Info/Activities/InvestigateStimulusActivity.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

#include "NavigationSystem.h"
#include "ReadyOrNotAISystem.h"
#include "Actors/PairedInteractionDriver.h"
#include "Actors/SuspectArmour.h"
#include "AI/AIAction.h"
#include "AI/TrailerSWATCharacter.h"
#include "Commander/RosterManager.h"
#include "DamageTypes/LessLethal/CSGasDamageType.h"
#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/MoveToActivity.h"
#include "Info/Activities/TakeCoverAtLandmarkActivity.h"
#include "Info/Activities/TakeHostageActivity.h"
#include "Info/Activities/WorldBuildingActivity.h"
#include "Info/Activities/CombatMove/ChargeCombatMove.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Cyber Character Tick"), STAT_RoNCyberTick, STATGROUP_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Cyber Character Tick (Server)"), STAT_RoNCyberTickServer, STATGROUP_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Is Being Looked At"), STAT_RoNIsBeingLookedAt, STATGROUP_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Ragdoll Transform Tick"), STAT_RoNRagdollTransformTick, STATGROUP_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Do Low Ready Trace"), STAT_RoNDoLowReadyTrace, STATGROUP_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Should Low Ready Now"), STAT_ShouldLowReadyNow, STATGROUP_PlayerCharacter);

static TAutoConsoleVariable<float> CVarRonLowerMorale(TEXT("a.RonAILowerMorale"), 0, TEXT("0 = Don't force lowering morale. >0 = Force lowering morale at the specified rate per frame"));
static TAutoConsoleVariable<bool> CVarDetailedCyberneticsDebug(TEXT("a.RonShowCyberneticsDetailedDebug"), false, TEXT("Show detailed debugging information when looking at cybernetics"));
static TAutoConsoleVariable<int32> CVarRonNoSurrender(TEXT("a.RonNoSurrender"), 0, TEXT("Disable surrendering"));
static TAutoConsoleVariable<int32> CVarAICharacterDrawTransform(TEXT("AI.DrawTransform"), 0, TEXT("Draw AI transforms"));
static TAutoConsoleVariable<int32> CVarRonDrawMovementBase(TEXT("a.RonAIDrawMovementBase"), 0, TEXT("Draw Movement Base"));

ACyberneticCharacter::ACyberneticCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.TickInterval = 0.0167f;
	
	NetUpdateFrequency = 30.0f;
	MinNetUpdateFrequency = 2.0f;
	NetPriority = 3.0f;
	
	IFMODStudioModule::Get();

	MoveStyle = CreateDefaultSubobject<URoNMoveStyleComponent>(TEXT("MoveStyle"));

	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->bAutoAddToScorePool = false;
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;
	
	GetCharacterMovement()->NavAgentProps.AgentRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	GetCharacterMovement()->NavAgentProps.AgentHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f;
	GetCharacterMovement()->bUseRVOAvoidance = false;
	
	if (VoiceLineEventMask == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> TPEvent(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_Mask_3P.Vl_Mask_3P'"));
		VoiceLineEventMask = TPEvent.Object;
	}
	
	if (FootstepsLocal == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> ObjFootstepsLocalEvent(TEXT("FMODEvent'/Game/FMOD/Events/Character/RUS/Footsteps/3P/1P_FootstepTotal_RUS.1P_FootstepTotal_RUS'"));
		FootstepsLocal = ObjFootstepsLocalEvent.Object;
	}
	
	if (FootstepsRemote == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> ObjFootstepsRemoteEvent(TEXT("FMODEvent'/Game/FMOD/Events/Character/RUS/Footsteps/3P/3P_FootstepTotal_RUS.3P_FootstepTotal_RUS'"));
		FootstepsRemote = ObjFootstepsRemoteEvent.Object;
	}

	PlayerMarkerComponent->bEnabled = false;

	CurrentPitchInterpolated = 0.0f;
	CurrentYawInterpolated = 0.0f;

	FocalPointInterpSpeed = 1.5f;
	FocusTurnSpeed = 10.0f;
	TurnDegreesPerSecond = 80.0f;
	FocalPointInterpCurve = EAlphaBlendOption::ExpOut;
	ActorRotationInterpStandingSpeed = 5.0f;
	ActorRotationInterpMovingSpeed = 7.0f;
	AimOffsetInterpSpeed = 8.0f;
}

void ACyberneticCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACyberneticCharacter, CharacterMeshData);
	DOREPLIFETIME(ACyberneticCharacter, AttachedMeshData);
	DOREPLIFETIME(ACyberneticCharacter, AttachedSkeletalMeshData);
	DOREPLIFETIME(ACyberneticCharacter, SimulatingAttachedStaticMeshes);
	DOREPLIFETIME(ACyberneticCharacter, CachedHitScanResult);

	DOREPLIFETIME(ACyberneticCharacter, Rep_CoverAnimState);
	DOREPLIFETIME(ACyberneticCharacter, Rep_TakeHostageAnimState);
	DOREPLIFETIME(ACyberneticCharacter, Rep_HidingAnimState);
	DOREPLIFETIME(ACyberneticCharacter, CurrentCoverLandmarkInUse);
	DOREPLIFETIME(ACyberneticCharacter, LastCoverLandmarkUsed);
	
	DOREPLIFETIME(ACyberneticCharacter, Rep_FocalPoint);
	DOREPLIFETIME(ACyberneticCharacter, Rep_FocalActor);
	DOREPLIFETIME(ACyberneticCharacter, Rep_HeadFocalPoint);
	DOREPLIFETIME(ACyberneticCharacter, CombatState);

	DOREPLIFETIME(ACyberneticCharacter, bIsMoving);
	DOREPLIFETIME(ACyberneticCharacter, bHasEverShot);
	DOREPLIFETIME(ACyberneticCharacter, bIsKnockedOut);
	DOREPLIFETIME(ACyberneticCharacter, bRecoveringFromRagdoll);
	DOREPLIFETIME(ACyberneticCharacter, bIsPlayingDead);
	DOREPLIFETIME(ACyberneticCharacter, bIsFakeSurrender);
	DOREPLIFETIME(ACyberneticCharacter, bHasEverFakeSurrendered);

	DOREPLIFETIME(ACyberneticCharacter, Rep_WorldBuildingAnimState);
	
	DOREPLIFETIME(ACyberneticCharacter, RagdollMeshLocation);
	DOREPLIFETIME(ACyberneticCharacter, RagdollMeshRotation);
	
	DOREPLIFETIME(ACyberneticCharacter, CurrentWallHoleTraversalInUse);
	DOREPLIFETIME(ACyberneticCharacter, LastWallHoleTraversalUsed);
	DOREPLIFETIME(ACyberneticCharacter, Rep_HoleTraversalAnimState);
	DOREPLIFETIME(ACyberneticCharacter, Rep_AimOffsetFocalPoint);
	DOREPLIFETIME(ACyberneticCharacter, bIsRaisingWeapon);
	DOREPLIFETIME(ACyberneticCharacter, bIsLoweringWeapon);
}

void ACyberneticCharacter::BeginPlay()
{ 
	Super::BeginPlay();

	// reset defaults in this
	MoveDataOverride.Empty();

	bAlwaysRelevant = false;

	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->AddGameEndListener(this);
	}

	OnGetupComplete.RemoveAll(this);
	OnGetupComplete.AddDynamic(this, &ACyberneticCharacter::OnGetupAfterRagdollComplete);

	RequiredTimeNotBeingLookedAt = 15.0f;
	
	RandGenUnarmedCalmPick = FMath::RandRange(0, CurMoveDataBlock.UnarmedCalmStyles.Num() - 1);
	RandGenUnarmedPanicPick = FMath::RandRange(0, CurMoveDataBlock.UnarmedPanicStyles.Num() - 1);

	bShouldKnifeUpCloseWhenBeingLookedAt = FMath::FRandRange(0.0f, 1.0f) < AI_CONFIG_GET_FLOAT("SuspectKnifeFakeForwardChance");
	
	StunAccuracyPenaltyRecovery = AI_CONFIG_GET_FLOAT("AccuracyPenaltyRecovery");
	PepperSprayAccuracyPenaltyRecovery = AI_CONFIG_GET_FLOAT("PepperSprayAccuracyPenaltyRecovery");
	
	
	TimeSinceHeardOfficerYell = FLT_MAX;

	CachedIntimidatorValue = URosterManager::GetSquadTraitValue("Intimidator", GetWorld());
	
	if (GibComponent)
	{
		GibComponent->OnGib.AddUObject(this, &ACyberneticCharacter::HandleLimbDismembered);
	}

	SpawnLocation = GetActorLocation();
}

void ACyberneticCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->RemoveGameEndListener(this);
		
		GameState->AllAICharacters.Remove(this);
		GameState->bCharactersDirty = true;
	}
}

void ACyberneticCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
#ifndef NO_BUENO
	SCOPE_CYCLE_COUNTER(STAT_RoNCyberTick);

	if (!GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown))
	{
		return;
	}

	if (IsInRagdoll() && !bPlayingDeathMontage) // Alex 24/11/23 during death animation do not shift the actor!!!!
	{
		const FVector SmoothedLocation = FMath::VInterpTo(GetActorLocation(), RagdollMeshLocation, DeltaSeconds, 2.0f);
		SetActorLocation(SmoothedLocation, true, nullptr, ETeleportType::ResetPhysics);
	}

	TimeSinceLastTakenDamage += DeltaSeconds;
	TimeSinceLastTakenStunDamage += DeltaSeconds;
	
	if (IsArrestedOrSurrendered() || IsIncapacitated() || IsDeadOrUnconscious())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	else
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	
	if (IsDeadOrUnconscious() || IsIncapacitated() || IsCarried() || IsPlayingDead())
	{
		AimOffset = FVector2D::ZeroVector;
		PreviousAimOffset = FVector2D::ZeroVector;
		
		bDrawingWeapon = false;
		bPickingUpWeapon = false;
		bIsRaisingWeapon = false;
		bIsLoweringWeapon = false;

		FakeSurrenderTime = 0.0f;
		DrawingWeaponTime = 0.0f;
		PickingUpWeaponTime = 0.0f;
		TimeSinceLastShot = 0.0f;
		RaisingWeaponTime = 0.0f;
		LoweringWeaponTime = 0.0f;
		HesitationTime = 0.0f;
		
		return;
	}

	if (bDeactivated)
	{
		return;
	}

	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
		return;

	SuppressionAmount = FMath::Max(SuppressionAmount - DeltaSeconds, 0.0f);
	ForceFireGunDelay = FMath::Max(ForceFireGunDelay - DeltaSeconds, 0.0f);
	StunAccuracyPenalty = FMath::Max(StunAccuracyPenalty - StunAccuracyPenaltyRecovery * DeltaSeconds, 0.0f);
	PepperSprayAccuracyPenalty = FMath::Max(PepperSprayAccuracyPenalty - PepperSprayAccuracyPenaltyRecovery * DeltaSeconds, 0.0f);

	TimeSinceHeardOfficerYell += DeltaSeconds;

	bDrawingWeapon = IsDrawingWeapon();
	bPickingUpWeapon = IsPickingUpWeapon();
	bIsRaisingWeapon = IsRaisingWeapon();
	bIsLoweringWeapon = IsLoweringWeapon();

	bIsFakeSurrender ? FakeSurrenderTime += DeltaSeconds : FakeSurrenderTime = 0.0f;
	bDrawingWeapon ? DrawingWeaponTime += DeltaSeconds : DrawingWeaponTime = 0.0f;
	bPickingUpWeapon ? PickingUpWeaponTime += DeltaSeconds : PickingUpWeaponTime = 0.0f;
	bHasEverShot ? TimeSinceLastShot += DeltaSeconds : TimeSinceLastShot = 0.0f;
	bIsRaisingWeapon ? RaisingWeaponTime += DeltaSeconds : RaisingWeaponTime = 0.0f;
	bIsLoweringWeapon ? LoweringWeaponTime += DeltaSeconds : LoweringWeaponTime = 0.0f;
	IsHesitating() ? HesitationTime += DeltaSeconds : HesitationTime = 0.0f;
	
	//GetCapsuleComponent()->bNavigationRelevant = false;
	//GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	if (IsArrestedOrSurrendered())
	{
		GetCapsuleComponent()->SetCapsuleRadius(50.0f);
	}

	// for network purposes, this is needed to ensure consistency between clients
	if (IsTakingCoverAtLandmark())
	{
		if (CurrentCoverLandmarkInUse)
		{
			if (CurrentCoverLandmarkInUse->bDisableCollision)
				MoveIgnoreActorAdd(CurrentCoverLandmarkInUse->CoverObject.LoadSynchronous());

			for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : CurrentCoverLandmarkInUse->IgnoredMeshActors)
			{
				MoveIgnoreActorAdd(MeshActor.LoadSynchronous());
			}
	
			MoveIgnoreActorAdd(CurrentCoverLandmarkInUse->IdlePoint);
			
			for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
				MoveIgnoreActorAdd(V);
	
			InteractableComponent->IgnoreInteractionBlockingActors.AddUnique(CurrentCoverLandmarkInUse->CoverObject.LoadSynchronous());
			InteractableComponent->IgnoreInteractionBlockingActors.AddUnique(CurrentCoverLandmarkInUse->IdlePoint);
		}
	}
	else
	{
		if (LastCoverLandmarkUsed)
		{
			MoveIgnoreActorRemove(LastCoverLandmarkUsed->CoverObject.LoadSynchronous());
			MoveIgnoreActorRemove(LastCoverLandmarkUsed->IdlePoint);

			for (const TSoftObjectPtr<AStaticMeshActor>& MeshActor : LastCoverLandmarkUsed->IgnoredMeshActors)
			{
				MoveIgnoreActorRemove(MeshActor.LoadSynchronous());
			}
			
			for (ABlockingVolume* V : Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor())->BlockingVolumesInLevel)
				MoveIgnoreActorRemove(V);
			
			InteractableComponent->IgnoreInteractionBlockingActors.Remove(LastCoverLandmarkUsed->CoverObject.LoadSynchronous());
			InteractableComponent->IgnoreInteractionBlockingActors.Remove(LastCoverLandmarkUsed->IdlePoint);

			LastCoverLandmarkUsed = nullptr;
		}
	}

	// Push arrested/surrendered logic
	if (IsArrestedOrSurrendered())
	{
		const float DistToArrestPush = (GetActorLocation() - ArrestedPushLocation).Size2D();
		if (ArrestedPushLocation != FVector::ZeroVector && DistToArrestPush > 30.0f)
		{
			//DrawDebugPoint(GetWorld(), ArrestedPushLocation, 10.0f, FColor::White, false, 1.0f, 1);
			const FVector Direction = (ArrestedPushLocation - GetActorLocation()).GetSafeNormal();
			AddMovementInput(Direction);
		}
		else
		{
			ArrestedPushLocation = FVector::ZeroVector;
		}
	}
	
	if (IsInRagdoll() && !bPlayingDeathMontage) // Alex 24/11/23 during death animation do not shift the actor!!!!
	{
		SetActorLocationAndRotation(RagdollMeshLocation, RagdollMeshRotation, true, nullptr, ETeleportType::ResetPhysics);
	}
#else
	if (!NoBuenoTextRender)
	{
		NoBuenoTextRender = NewObject<UTextRenderComponent>(this, UTextRenderComponent::StaticClass());
		NoBuenoTextRender->RegisterComponent();
		TArray<FString> PossibleText = {"42.", "Bueno", "No Bueno", "NDA!", "Ruh Roh Where did the code go?", "Maybe Bueno?", "Call an ambulance I can't move!"};
		NoBuenoTextRender->SetText(PossibleText[FMath::RandRange(0, PossibleText.Num() - 1)]);
		NoBuenoTextRender->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "head");
		NoBuenoTextRender->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		NoBuenoTextRender->SetHorizontalAlignment(EHTA_Center);
		NoBuenoTextRender->SetWorldSize(5.0f);
	}
	else
	{
		APlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (PlayerController)
		{
			FVector Location;
			FRotator Rotation;
			PlayerController->GetPlayerViewPoint(Location, Rotation);
			FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Location);
			NoBuenoTextRender->SetWorldRotation(LookAtRotator);
		}
	}
#endif
}

#if !UE_BUILD_SHIPPING
void ACyberneticCharacter::Tick_Debug(const float DeltaSeconds)
{
	Super::Tick_Debug(DeltaSeconds);

	if (CVarAICharacterDrawTransform.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 150.0f, FColor::Red);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorRightVector() * 150.0f, FColor::Green);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + GetActorUpVector() * 150.0f, FColor::Blue);

		DrawDebugCapsule(GetWorld(), GetCapsuleComponent()->GetComponentLocation(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetComponentQuat(), FColor::Blue);
		
		DrawDebugLine(GetWorld(), GetNavAgentLocation(), GetNavAgentLocation() + GetCharacterMovement()->CurrentFloor.HitResult.Normal * 50.0f, FColor::Orange, false, 0.033f);

		//DrawDebugBox(GetWorld(), GetMesh()->GetSocketLocation("pelvis"), FVector(10.0f), FColor::Orange, false, DeltaSeconds + 0.025f, 0, 1.5f);

		//DrawDebugLine(GetWorld(), GetMesh()->GetSocketLocation("pelvis"), GetMesh()->GetSocketLocation("pelvis") + UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")) * 100.0f, FColor::Orange);
		
		//float UpDot = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), FVector::UpVector);
		//float RightDot = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), GetActorRightVector());

		//DrawDebugLine(GetWorld(), GetMesh()->GetSocketLocation("pelvis"), GetMesh()->GetSocketLocation("pelvis") + FVector::UpVector * 100.0f, FColor::Purple);
		//DrawDebugLine(GetWorld(), GetMesh()->GetSocketLocation("pelvis"), GetMesh()->GetSocketLocation("pelvis") + UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")) * 100.0f, FColor::Orange);
		//DrawDebugLine(GetWorld(), GetMesh()->GetComponentLocation(), GetMesh()->GetComponentLocation() + UKismetMathLibrary::GetForwardVector(GetMesh()->GetComponentRotation()) * 1000.0f, FColor::Magenta, false, 0.033f);

		//DrawDebugString(GetWorld(), GetMesh()->GetSocketLocation("pelvis"), FString::Printf(TEXT("Up dot: %.2f\nRight dot: %.2f"), UpDot, RightDot), nullptr, FColor::White, DeltaSeconds + 0.001f, true);

		//LOG_NUMBER(UpDot);
		//LOG_NUMBER(RightDot);
	}

	if (CVarRonDrawAIDebug.GetValueOnAnyThread() > 0)
	{
		if (const AReadyOrNotPlayerController* LocalPC = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			LocalPC->GetPlayerViewPoint(CameraLocation, CameraRotation);

			CameraRotation.Pitch = UKismetMathLibrary::FindLookAtRotation(CameraLocation, GetActorLocation()).Pitch;

			/*
			const float DotProduct = FVector::DotProduct( CameraRotation.Vector(), UKismetMathLibrary::FindLookAtRotation(CameraLocation, GetActorLocation()).Vector());

			const float Dist = (CameraLocation - GetActorLocation()).Size();

			if (DotProduct > 0.99f && Dist < 2000.0f)
			{
				TArray<FDebugData> DebugData;
				GatherDebugData_Implementation(DebugData);

				FString DebugString = "";
				
				for (const FDebugData& dt : DebugData)
				{
					DebugString += dt.Label.ToString() + ": " + dt.Value.ToString() + "\n";
				}
				
				DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 150.0f), DebugString, nullptr, FColor::Yellow, DeltaSeconds, true);
			}
			*/
		}
	}

	if (CVarRonLowerMorale.GetValueOnAnyThread() > 0 && GetGameTimeSinceCreation() > 3.0f) // wait a bit
	{
		UMoraleComponent::LowerMoraleOnCharacter(this, CVarRonLowerMorale.GetValueOnAnyThread() * DeltaSeconds, "CVarLowerMorale");
	}

	/*if (IsSelectedInEditor())
	{
		FString DebugMessage = FString(IsStrafing() ? "Strafing" : "Not Strafing") + LINE_TERMINATOR +
								RON_ENUM_TO_STRING(EAnimWeaponType, GetCurrentWeaponAnimType());
		
		ULog::Info(DebugMessage);
	}*/
	
	if (Archetype)
	{
		if (Archetype->bOverrideAimSettings)
		{
			FocalPointInterpSpeed = Archetype->FocalPointInterpSpeed;
			FocalPointInterpCurve = Archetype->FocalPointInterpCurve;
			FocusTurnSpeed = Archetype->FocusTurnSpeed;
			TurnDegreesPerSecond = Archetype->TurnDegreesPerSecond;
			ActorRotationInterpStandingSpeed = Archetype->ActorRotationInterpStandingSpeed;
			ActorRotationInterpMovingSpeed = Archetype->ActorRotationInterpMovingSpeed;
			AimOffsetInterpSpeed = Archetype->AimOffsetInterpSpeed;
		}
	}
	
	if (CVarRonDrawMovementBase.GetValueOnAnyThread() != 0)
	{
		if (GetMovementBase())
		{
			FString DebugStr = "MovementBase: {0}:{1}";
			DebugStr = FString::Format(*DebugStr, {GetMovementBase()->GetName(), GetMovementBase()->GetOwner()->GetName()});
			DrawDebugString(GetWorld(), GetActorLocation(), DebugStr, nullptr, FColor::White, 0.0167f);
		}
	}
}
#endif

void ACyberneticCharacter::Tick_Authority(const float DeltaSeconds)
{
	Super::Tick_Authority(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_RoNCyberTickServer);

	if (bDeactivated)
		return;
	
	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
		return;
	
	TimeSinceLastAggressiveForce += DeltaSeconds;
	
	bWantsReload = ShouldReload();
	
	if (!bHasAddedScoreGroups && GetGameTimeSinceCreation() > 1.0f)
	{
		AddScoreGroups();
	}

	// Playdead failsafe
	if (IsPlayingDead())
	{
		TimePlayingDead += DeltaSeconds;

		if (TimePlayingDead > 90.0f)
		{
			StopPlayingDead();
		}
	}
	else
	{
		TimePlayingDead = 0.0f;
	}
	
	if (GetGameTimeSinceCreation() > 5.0f && !bHasSentCharacterMeshData)
	{
		Multicast_SendCharacterMeshData(CharacterMeshData);
		bHasSentCharacterMeshData = true;
	}

	TimeSinceLastMoveStyleUpdate += DeltaSeconds;
	if (TimeSinceLastMoveStyleUpdate > MoveStyleUpdateTime)
	{
		TimeSinceLastMoveStyleUpdate = 0.0f;
		UpdateDefaultMoveStyle();
	}
	
	if (AvoidanceLocation != FVector::ZeroVector)
	{
		AvoidanceTime -= DeltaSeconds;
		
		if (GetCyberneticsController())
			GetCyberneticsController()->AbortMove();
		
		//DrawDebugBox(GetWorld(), AvoidanceLocation, FVector(15.0f), FColor::Magenta, false, DeltaSeconds+0.01, 0, 1.5);
		
		const FVector SmoothedLocation = FMath::VInterpTo(GetActorLocation(), AvoidanceLocation, DeltaSeconds, 5.0f);
		SetActorLocation(SmoothedLocation, true, nullptr, ETeleportType::ResetPhysics);
		
		FVector A = GetActorLocation();
		A.Z = 0;
		
		FVector B = AvoidanceLocation;
		B.Z = 0;
		
		FVector C;
		if (AvoidanceTime <= 0.0f || FVector::PointsAreNear(A, B, 15.0f) || !UReadyOrNotAISystem::ProjectPointToNav(SmoothedLocation, C, FVector(10.0f, 10.0f, 130.0f)) || !HasLineOfSightTo(AvoidanceLocation))
		{
			AvoidanceLocation = FVector::ZeroVector;
			AvoidanceTime = 0.0f;
		}
	}

	NoPathCooldown = FMath::Max(NoPathCooldown - DeltaSeconds, 0.0f);

	ForceComplianceStrength = FMath::Max(ForceComplianceStrength - (DeltaSeconds * 1.5f), 0.0f);
	
	TimeSinceLastPlayDead += DeltaSeconds;
	TimeSinceAtLastCoverLandmark += DeltaSeconds;

	TArray<FString> CooldownKeysToRemove;
	for (auto& Cooldown : TableMontageActiveCooldowns)
	{
		if (Cooldown.Value <= 0.0f)
		{
			CooldownKeysToRemove.AddUnique(Cooldown.Key);
		}
		else
		{
			TableMontageActiveCooldowns[Cooldown.Key] -= DeltaSeconds;
		}
	}
    
	for (const FString& Cooldown : CooldownKeysToRemove)
	{
		TableMontageActiveCooldowns.Remove(Cooldown);
	}
    
	if (TableMontageQueue.Num() > 0)
	{
		if (!IsAny3PMontageActive())
		{
			if (PlayMontageFromTable(TableMontageQueue[0]))
			{
				TableMontageQueue.RemoveAt(0);

				// If this montage was added to the montage focal map, remove it, the focal point is most likely stale
				// Fixes an issue where the targeting comp would use an old focal point resulting in incorrect AI rotations
				if (const ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController()))
					CyberneticController->GetTargetingComp()->ClearMontageFocalPoint();
			}
		}
	}

	// failsafe, in case the animation is cancelled
	if (QueuedDoorToOpen && ! IsTableMontagePlaying("tp_swat_door_push_open"))
	{
		QueuedDoorToOpen = nullptr;
	}
	
	if (QueuedDoorToClose && ! IsTableMontagePlaying("tp_swat_door_push_close"))
	{
		QueuedDoorToClose = nullptr;
	}

	// Cache visible swat
	if (!IsOnSWATTeam())
	{
		if (VisibleSwatTraceHandles.Num() > 0)
		{
			bool bAnyValid = false;
			uint8 VisibleSwat = 0;
			for (const FTraceHandle& Handle : VisibleSwatTraceHandles)
			{
				if (GetWorld()->IsTraceHandleValid(Handle, false))
				{
					bAnyValid = true;
					
					FTraceDatum OutTraceData;
					if (GetWorld()->QueryTraceData(Handle, OutTraceData))
					{
						if (OutTraceData.OutHits.Num() == 0)
						{
							VisibleSwat++;
						}
					}
				}
			}
			
			float SwatContribution = FMath::Max(1.0f, CachedIntimidatorValue);
			float FinalPercentage = (static_cast<float>(VisibleSwat) / 5.0f) * SwatContribution; // 5 is max swat presence. 1 player, 4 swat ai
			
			if (!bAnyValid || VisibleSwat == 0)
				VisibleSwatPercentage = 0.0f;
			else
				VisibleSwatPercentage = FMath::Clamp(FinalPercentage, 0.0f, 1.0f);

			VisibleSwatTraceHandles.Empty(5);
		}
		else
		{
			for (AReadyOrNotCharacter* Character : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters)
			{
				if (Character && Character->IsOnSWATTeam() && Character->IsActive() && !Cast<ATrailerSWATCharacter>(Character))
				{
					const FVector StartTrace = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
					const FVector EndTrace = Character->GetActorLocation();
					VisibleSwatTraceHandles.Add(GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(this, Character)));
				}
			}
		}
	}

	// Cover data update
	{
		const bool bTakingCover = IsTakingCover();

		if (!bTakingCover)
		{
			ActiveCoverFireType = ECoverFireType::None;
		}
		
		FCoverAnimStateMachineData CoverAnimState;
		CoverAnimState.bIsInCover = bTakingCover;
		CoverAnimState.bIsFiring = ActiveCoverFireType > ECoverFireType::None;
		CoverAnimState.bIsReturningToIdle = bTakingCover && ActiveCoverFireType == ECoverFireType::None;
		CoverAnimState.ActiveCoverDirection = ActiveCoverDirection;
		CoverAnimState.ActiveCoverFirePose = ActiveCoverFirePose;
		CoverAnimState.ActiveCoverIdlePose = ActiveCoverIdlePose;

		Rep_CoverAnimState = CoverAnimState;
	}

	// world bulding data update
	{
		bool bIsWorldBuilding = false;
		
		FWorldBuildingAnimState AnimState;

		if (GetCyberneticsController())
		{
			if (UWorldBuildingActivity* Activity = GetCyberneticsController()->GetCurrentActivity<UWorldBuildingActivity>())
			{
				AnimState.bIsLooping = Activity->GetActiveStateID() >= 2 && Activity->GetActiveStateID() < 5 && !Activity->bOneShotAnimationDataTable;
				
				if (Activity->GetActiveStateID() > 0 && Activity->GetActiveStateID() < 5)
				{
					bIsWorldBuilding = true;
					
					AnimState.LoopAnim = Activity->Loop;
				}
			}
		}
		
		AnimState.bIsPlaying = bIsWorldBuilding;
		if (!bIsWorldBuilding)
		{
			AnimState.bIsLooping = false;
		}

		Rep_WorldBuildingAnimState = AnimState;
	}

	// hostage state update
	{
		if (IsDeadOrUnconscious() || IsIncapacitated() || IsInRagdoll())
		{
			Rep_TakeHostageAnimState.bIsLooping = false;
			Rep_TakeHostageAnimState.bIsTakingHostage = false;
		}
	}

	if (!IsActive())
	{
		Rep_CoverAnimState.bIsInCover = false;
		Rep_CoverAnimState.bIsFiring = false;
		Rep_CoverAnimState.bIsReturningToIdle = false;
		Rep_HidingAnimState.bIsHiding = false;
		Rep_WorldBuildingAnimState.bIsPlaying = false;
		Rep_WorldBuildingAnimState.bIsLooping = false;
	}

	if (IsStunned())
	{
		bWasStunned = true;
		TimeSinceExitStunned = 0.0f;
	}
	else
	{
		if (bWasStunned)
		{
			TimeSinceExitStunned += DeltaSeconds;
		}
	}

	if (IsSurrendered() && !IsArrested() && !bIsBeingArrested)
	{
		if (CanExitSurrender() && !IsExitingSurrender())
		{
			TimeUntilNextLookCheck -= DeltaSeconds;
			if (TimeUntilNextLookCheck <= 0.0f)
			{
				TimeUntilNextLookCheck = 1.0f;
				if (!IsBeingLookedAt(0.0f, DistanceToClosestPawn, ClosestPawn))
				{
					TimeNotBeingLookedAt += 1.0f;
				}
				else
				{
					TimeNotBeingLookedAt = 0.0f;
				}
			}
		}
	}
	else
	{
		TimeNotBeingLookedAt = 0.0f;
	}

	if (FAISystem::IsValidLocation(Rep_FocalPoint) && Rep_FocalPoint != FVector::ZeroVector)
	{
		//bHasLOSToFocalPoint = HasLineOfSightTo(Rep_FocalPoint);
		
		// async trace for current focal point
		{
			if (GetWorld()->IsTraceHandleValid(FocalPointLOSTraceHandle, false))
			{
				FTraceDatum OutTraceData;
				if (GetWorld()->QueryTraceData(FocalPointLOSTraceHandle, OutTraceData))
				{
					bHasLOSToFocalPoint = OutTraceData.OutHits.Num() == 0;
				}
			}
			else
			{
				const FVector StartTrace = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = Rep_FocalPoint;
				FocalPointLOSTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, StartTrace, EndTrace, ECC_Visibility, GetCollisionQueryParameters());
			}
		}

		if (ABaseMagazineWeapon* bmw = GetEquippedWeapon())
		{
			const FVector& FireDirection = bmw->GetBulletSpawn()->GetForwardVector();
			const FVector& DirectionToFocalPoint = (Rep_FocalPoint - bmw->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
			float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);
			if (FVector::Distance(Rep_FocalPoint, bmw->GetBulletSpawn()->GetComponentLocation()) < 100.0f)
			{
				TargetRotationDelta = 1.0f;
			}

			if (TargetRotationDelta >= FireAngleThreshold)
			{
				TimeInsideFireAngleThreshold += DeltaSeconds;
			}
			else
			{
				TimeInsideFireAngleThreshold = 0.0f;
			}
		}
		else
		{
			TimeInsideFireAngleThreshold = 0.0f;
		}
	}
	else
	{
		TimeInsideFireAngleThreshold = 0.0f;
		bHasLOSToFocalPoint = false;
	}

	TimeSinceLastTasered += DeltaSeconds;

	if (IsArrestedOrSurrendered())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
		
	if (!bIsBeingArrested && ((IsSurrendered() && !IsArrested()) || (!IsArrested() && bOrderedToRotateForArrest)))
	{
		SetIsStrafing(false);
	}

	FVector PelvisLocation = GetMesh()->GetSocketLocation("pelvis");
	FVector RelativeMeshLocation = GetDefault<ACyberneticCharacter>(GetClass())->GetMesh()->GetRelativeLocation();
	FVector PelvisLocation_Offset = PelvisLocation + FVector(0.0f, 0.0f, FMath::Abs(RelativeMeshLocation.Z));

	if (IsInRagdoll())
	{
		SCOPE_CYCLE_COUNTER(STAT_RoNRagdollTransformTick);
	
		// Determine wether the ragdoll is facing up or down and set the target rotation accordingly.
		const FRotator PelvisRot = GetMesh()->GetSocketRotation("pelvis");
		float UpDot = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), FVector::UpVector);
		bool bRagdollFaceUp = UpDot > 0.0f;
		RagdollMeshRotation = FRotator(GetActorRotation().Pitch, bRagdollFaceUp ? PelvisRot.Yaw - 180.0f : PelvisRot.Yaw, GetActorRotation().Roll);

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		
		//DrawDebugLine(GetWorld(), PelvisLocation, PelvisLocation - FVector::UpVector * 150.0f, bIsGrounded ? FColor::Green : FColor::Red, false, DeltaSeconds + 0.025f, 0, 1.5f);

		FTraceDatum OutData;
		if (GroundCheckHandle.IsValid() && GetWorld()->QueryTraceData(GroundCheckHandle, OutData))
		{
			FVector CapsuleLocation = PelvisLocation_Offset;

			if (OutData.OutHits.Num() > 0)
			{
				FHitResult Hit = OutData.OutHits[0];
				if (Hit.IsValidBlockingHit() && Hit.ImpactPoint != FVector::ZeroVector)
				{
					CapsuleLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, FMath::Abs(RelativeMeshLocation.Z) + 3.0f); // add a little padding to eliminate popping
				}
			}
		
			RagdollMeshLocation = CapsuleLocation;
		}

		GroundCheckHandle = GetWorld()->AsyncLineTraceByObjectType(EAsyncTraceType::Single, PelvisLocation, PelvisLocation - FVector::UpVector * 150.0f, ObjectQueryParams, GetCollisionQueryParameters());
	}
	else
	{
		RagdollMeshLocation = PelvisLocation_Offset;
	}

	// Throw weapon failsafe
	if (bSurrendered)
	{
		TimeSurrendered += DeltaSeconds;
		if (TimeSurrendered > 1.0f && GetEquippedItem())
		{
			ThrowEquippedItem();
		}
	} 

	if (!IsSurrendered())
	{
		TimeSurrendered = 0.0f;
	}
	
	if (!GetCyberneticsController())
		return;

	bool bLowReadyOverriden = false;
	if (UBaseActivity* Activity = GetCyberneticsController()->GetCurrentActivity())
	{
		bool bLowReadyOverride = false;
		if (Activity->GetLowReadyOverride(bLowReadyOverride))
		{
			SetLowReady(false, bLowReadyOverride);
			bLowReadyOverriden = true;
		}
	}

	if (!bLowReadyOverriden)
	{
		if (CanLowReady())
		{
			SetLowReady(false, ShouldLowReadyNow());
		}
		else
		{
			SetLowReady(false, false);
		}
	}
	
	if (AReadyOrNotCharacter* TrackedTarget = GetCyberneticsController()->GetTrackedTarget())
	{
		if (GetCyberneticsController()->IsCharacterEnemy(TrackedTarget) && !bIsFakeSurrender)
		{
			if (!GetEquippedWeapon())
			{
				if (IsActive() && IsSuspect() && TrackedTarget->IsOnSWATTeam() && !IsPlayingDead())
				{
					UMoraleComponent::LowerMoraleOnCharacter(this, 0.25f * DeltaSeconds, "Tracking target with no equipped weapon");
				}
			}
				
			if (TrackedTarget->GetDeltaRotationToCharacter(this) < 0.0f && !IsPlayingDead())
			{
				Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("NoEquippedWeaponStress", 1.0f), 0.0f, 1.0f) * DeltaSeconds;
			}
		}
		
		if (TrackedTarget->GetDeltaRotationToCharacter(this) > 0.95f)
		{
			float Distance = FVector::Distance(TrackedTarget->GetActorLocation(), GetActorLocation());
			if (Distance < 500.0f)
			{
				if (APlayerCharacter* Player = Cast<APlayerCharacter>(TrackedTarget))
				{
					if (Player->bAiming)
					{
						Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("PlayerADSStress", 0.15f), 0.0f, 1.0f) * DeltaSeconds;
						//LOG_NUMBER(Stress);
					}
				}
			}
		}
	}

	if (!IsDeadOrUnconscious() && !IsIncapacitated() && !IsInRagdoll())
	{
		if (!IsOnSWATTeam())
		{
			if (FNavPathSharedPtr Path = GetCyberneticsController()->GetRONPathFollowingComp()->GetPath())
			{
				if (Path.IsValid() && Path->GetPathPoints().Num() > 0)
				{
					const float Distance = Path->GetLengthFromPosition(GetNavAgentLocation(), GetCyberneticsController()->GetRONPathFollowingComp()->GetNextPathIndex());
					const float DistanceRemainingOnPath = Path->GetLength() - Distance;

					/*
					const FSplineCurves* Spline = GetCyberneticsController()->GetRONPathFollowingComp()->GetSplineCurvePath();
					const float InputKey = FNavMeshSplinePath::GetInputKeyClosestToWorldLocation(Spline, FTransform(), GetMovementComponent()->GetActorFeetLocation());
					const float Distance = FNavMeshSplinePath::GetDistanceAlongSplineAtInputKey(Spline, InputKey);
					const float DistanceRemainingOnPath = Spline->GetSplineLength() - Distance;
					*/
					
					if (DistanceRemainingOnPath >= 350.0f)
						MoveStyle->SetMovementGaitByName("jog");
					else
						MoveStyle->SetMovementGaitByName("walk");
				}
			}
			else
			{
				MoveStyle->SetMovementGaitByName("walk");
			}
			
			if (ReasonsToStandStill.Num() > 0 || IsBeingArrested() || IsGettingUp() || bRecoveringFromRagdoll)
			{
				GetCharacterMovement()->MaxWalkSpeed = 0.0f;
			}
			else if (ReasonsToWalk.Num() > 0 || IsArrestedOrSurrendered() || IsStunned())
			{
				MoveStyle->SetMovementGaitByName("walk", true);
			}
			else if (ReasonsToSprint.Num() > 0)
			{
				if (!MoveStyle->SetMovementGaitByName("sprint", true))
				{
					MoveStyle->SetMovementGaitByName("jog", true);
				}
			}
		}
	}
}

bool ACyberneticCharacter::IsBeingLookedAt(const float Threshold, float& OutClosestDistance, APawn*& OutClosestPawn)
{
	SCOPE_CYCLE_COUNTER(STAT_RoNIsBeingLookedAt);

	AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GS)
		return false;

	OutClosestPawn = nullptr;
	OutClosestDistance = BIG_DIST;

	// sort all characters based on our location (to reduce unnecesary traces)
	TArray<AReadyOrNotCharacter*> SortedCharacters = GS->AllReadyOrNotCharacters;
	SortedCharacters.Remove(this);
	SortedCharacters.RemoveAll([this](AReadyOrNotCharacter* Element)
	{
		return !Element->IsOnSWATTeam();
	});
	
	SortedCharacters.Sort([this](AReadyOrNotCharacter& Lhs, AReadyOrNotCharacter& Rhs)
	{
		return FVector::Distance(GetActorLocation(), Lhs.GetActorLocation()) < FVector::Distance(GetActorLocation(), Rhs.GetActorLocation());
	});

	for (AReadyOrNotCharacter* Character : SortedCharacters)
	{
		const float Dist = FVector::Distance(Character->GetActorLocation(), GetActorLocation());

		if (Dist < 1750.0f)
		{
			const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), (GetActorLocation() - Character->GetActorLocation()).GetSafeNormal());
			if (DotProduct > Threshold)
			{
				FHitResult Hit;
				const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Character, this);
				if (!GetWorld()->LineTraceSingleByChannel(Hit, Character->GetActorLocation(), GetActorLocation(), ECC_Visibility, CollisionQueryParams))
				{
					OutClosestDistance = Dist;
					OutClosestPawn = Character;
					return true;
				}
			}
		}
	}

	return false;
}

void ACyberneticCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
		return;

	if (UInteractionsData::IsPairedInteractionPlayingOn(this))
		return;

	// No movement allowed while being carried by someone
	if (IsCarried())
		return;

	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void ACyberneticCharacter::StopAllMovement()
{
	if (ACyberneticController* AIController = Cast<ACyberneticController>(GetController()))
	{
		AIController->StopMovement();
	}
}

FVector ACyberneticCharacter::GetFocalPoint() const
{
	return Rep_FocalPoint;
}

FRotator ACyberneticCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool ACyberneticCharacter::GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface)
{
	if (GetCharacterMovement()->IsCrouching() || IsCrouching())
	{
		Stance = 1; // crouch
	}
	else if (GetVelocity().Size() < 150.0f)
	{
		Stance = 2; // walk
	}
	else
	{
		Stance = 3; // normal
	}
	
	return Super::GetFMODFootstepParameters(Stance, Speed, Surface);
}

void ACyberneticCharacter::AddMoveDataOverrides(FAIMoveDataBlock& OutMoveData)
{
	if (!MoveDataOverride.UnarmedMovementStyle.IsNone())
		OutMoveData.UnarmedMovementStyle = MoveDataOverride.UnarmedMovementStyle;
	
	if (!MoveDataOverride.RifleMovementStyle.IsNone())
		OutMoveData.RifleMovementStyle = MoveDataOverride.RifleMovementStyle;
	
	if (!MoveDataOverride.RifleStrafeMovementStyle.IsNone())
		OutMoveData.RifleStrafeMovementStyle = MoveDataOverride.RifleStrafeMovementStyle;
	
	if (!MoveDataOverride.PistolMovementStyle.IsNone())
		OutMoveData.PistolMovementStyle = MoveDataOverride.PistolMovementStyle;
	
	if (!MoveDataOverride.PistolStrafeMovementStyle.IsNone())
		OutMoveData.PistolStrafeMovementStyle = MoveDataOverride.PistolStrafeMovementStyle;
	
	if (!MoveDataOverride.PistolStrafeCrouchMovementStyle.IsNone())
		OutMoveData.PistolStrafeCrouchMovementStyle = MoveDataOverride.PistolStrafeCrouchMovementStyle;
	
	if (!MoveDataOverride.ComplyMovementStyle.IsNone())
		OutMoveData.ComplyMovementStyle = MoveDataOverride.ComplyMovementStyle;
	
	if (!MoveDataOverride.CuffedMovementStyle.IsNone())
		OutMoveData.CuffedMovementStyle = MoveDataOverride.CuffedMovementStyle;
	
	if (!MoveDataOverride.StunnedMovementStyle.IsNone())
		OutMoveData.StunnedMovementStyle = MoveDataOverride.StunnedMovementStyle;
	
	if (!MoveDataOverride.GassedMovementStyle.IsNone())
		OutMoveData.GassedMovementStyle = MoveDataOverride.GassedMovementStyle;
	
	if (!MoveDataOverride.InjuredMovementStyle.IsNone())
		OutMoveData.InjuredMovementStyle = MoveDataOverride.InjuredMovementStyle;
	
	if (!MoveDataOverride.SprintMovementStyle.IsNone())
		OutMoveData.SprintMovementStyle = MoveDataOverride.SprintMovementStyle;
}

void ACyberneticCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(NewController->PlayerState);
	if (ps)
	{
		ps->SetPlayerName("Bot");
		ps->SetIsABot(true);
	}
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllAICharacters.AddUnique(this);
		GS->bCharactersDirty = true;
	}
}

void ACyberneticCharacter::AddScoreGroups()
{
	if (GetTeam() == ETeamType::TT_CIVILIAN)
	{
		ScoringComponent->AddToScorePool(AScoringManager::SCORE_REPORT_CIVILIAN);
	}
	else if (GetTeam() == ETeamType::TT_SUSPECT)
	{
		ScoringComponent->AddToScorePool(AScoringManager::SCORE_REPORT_SUSPECT);
	}
	else if (IsOnSWATTeam())
	{
		ScoringComponent->AddToScorePool(AScoringManager::SCORE_NO_OFFICERS_DEAD);
		ScoringComponent->GiveAllScores(true, false);
	}
	
	bHasAddedScoreGroups = true;
}

void ACyberneticCharacter::RespondToFriendlyFire(AReadyOrNotCharacter* InstigatorCharacter)
{
	// give a warning look at the player. Note: could be an event we handle on any penalty given, this feels ugly to put here
	{
		if (ASWATCharacter* Swat = USWATManager::Get(this)->GetClosestSWATInSightToActor(InstigatorCharacter))
		{
			if (Swat->GetCyberneticsController())
			{
				const bool bIsStackUpOnDoor = Swat->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>() ||
											Swat->GetCyberneticsController()->GetCurrentActivity<UDoorInteractionActivity>() ||
											Swat->GetCyberneticsController()->GetCurrentActivity<UScanDoorActivity>();
				
				if (!bIsStackUpOnDoor && !Swat->IsFullBodyMontagePlaying())
				{
					if (UBaseCombatActivity* CombatActivity = Swat->GetCyberneticsController()->GetCombatActivity())
					{
						if (!UReadyOrNotAISystem::WasRecentlyInCombat(2.5f))
						{
							CombatActivity->ScriptedLookAtActor(InstigatorCharacter, 2.0f);
							Swat->PlayRawVO(VO_SWAT_GENERAL::CALL_FRIENDLY_FIRE);
						}
					}
				}
			}
		}
	}
}

void ACyberneticCharacter::IncreaseStress(const float Amount)
{
	Stress += Amount;
}

void ACyberneticCharacter::DecreaseStress(const float Amount)
{
	Stress = FMath::Max(Stress - Amount, 0.0f);
}

void ACyberneticCharacter::OnRep_CharacterMeshData()
{
	if (CharacterMeshData.Body)
	{
		V_LOGM(LogReadyOrNot, "Setting body to %s", *CharacterMeshData.Body->GetName());
		GetMesh()->SetSkeletalMesh(CharacterMeshData.Body, !GetMesh()->SkeletalMesh);
		GetMesh()->EmptyOverrideMaterials();
	}

	if (CharacterMeshData.Head)
	{
		V_LOGM(LogReadyOrNot, "Setting head to %s", *CharacterMeshData.Head->GetName());
		GetFaceMesh()->SetSkeletalMesh(CharacterMeshData.Head, !GetFaceMesh()->SkeletalMesh);
		GetFaceMesh()->EmptyOverrideMaterials();
	}
	
	SkinnedDecalSampler->ClearAllDecals();
	SkinnedDecalSampler->SetupMaterials();

	V_LOGM(LogReadyOrNot, "CharacterMesh: Received Character Mesh with guid %s Head? %d Body? %d", *CharacterMeshData.Guid.ToString(), CharacterMeshData.Head != nullptr, CharacterMeshData.Body != nullptr);

	if (!CharacterMeshData.CharacterSpeechHandle.IsEmpty())
		SetSpeechCharacterName(CharacterMeshData.CharacterSpeechHandle);

	if (CharacterMeshData.Footsteps)
	{
		FootstepsLocal = CharacterMeshData.Footsteps;
		FootstepsRemote = CharacterMeshData.Footsteps;
	}
	if (CharacterMeshData.MovementFoley)
	{
		MovementFoley = CharacterMeshData.MovementFoley;
		MovementFoleySocket = CharacterMeshData.MovementFoleySocket;
	}
	
	bFemale = CharacterMeshData.bIsFemale;
	bChild = CharacterMeshData.bIsChild;

	DamageExcludedBones = CharacterMeshData.DamageExcludedBones;
}

void ACyberneticCharacter::Multicast_SendCharacterMeshData_Implementation(FCharacterMesh RPC_CharacterMeshData)
{
	CharacterMeshData = RPC_CharacterMeshData;
	OnRep_CharacterMeshData();
}

bool ACyberneticCharacter::IsUnjustifiedUseOfForce(AReadyOrNotCharacter* Aggressor, ABaseItem* ForceWeapon, UDamageType* ForceUsed) const
{
	// No aggressor, don't check anything
	if (!Aggressor)
		return false;

	if (Aggressor == this)
		return false;

	#if !UE_BUILD_SHIPPING
	// Gods shall recieve no punishment
	if (Aggressor->HasGodMode())
		return false;
	#endif

	// If aggressor is on same team as swat, always unjustified
	if (Aggressor->IsOnSWATTeam() && IsOnSWATTeam())
		return true;
	
	if (bIsArterialBleeding)
		return false;

	if (ForceUsed)
	{
		// Check for stun damage first (Beanbag, sting, flash, gas, tazer, etc.)
		// Applying an AOE stun damage in a room full of arrested AI is justified, except for direct
		// stun damage, like taser, etc.
		if (UStunDamage* StunDamage = Cast<UStunDamage>(ForceUsed))
		{
			const EStunType StunDamageType = StunDamage->StunType;
			
			const bool bAOEStunForceUsed = StunDamageType == EStunType::ST_Flash ||
											StunDamageType == EStunType::ST_Gassed ||
											StunDamageType == EStunType::ST_Stung;

			// Leaving pepperspray as a subclass of stundamage for now
			// TODO: Possibly change this for clearer code, as pepperspray is no longer a stun
			const bool bDirectStunForceUsed = Cast<UPepperballDamageType>(StunDamage) ||
												Cast<UPepperSprayDamageType>(StunDamage) ||
												Cast<UBeanbagDamageType>(StunDamage) ||
												StunDamageType == EStunType::ST_Beanbag ||
												StunDamageType == EStunType::ST_Tased ||
												StunDamageType == EStunType::ST_Pepperball ||
												StunDamageType == EStunType::ST_Rubberball;

			// If we used general stun force, it's justified
			if (bAOEStunForceUsed)
				return false;

			// If we used direct stun force whilst arrested, it's not justified
			// TODO: Maybe only unjustified if we've been hit by a direct stun force more than once, to prevent accidental unauthorized force applied
			if (bDirectStunForceUsed)
			{
				if (IsArrested() || bIsBeingArrested || IsCarried())
					return true;

				if (TimeSinceLastAggressiveForce < 4.0f)
					return false;
				
				if (IsSurrenderedFor(1.5f))
					return true;
			}

			return false;
		}
	}
	
	// Always unjustified if a civilian is killed by the aggressor, regardless of their AI state or game mode
	if (IsCivilian())
	{
		const APlayerCharacter* Pawn = LastDamageEvent.Instigator ? LastDamageEvent.Instigator->GetPawn<APlayerCharacter>() : nullptr;
		
		if (KilledBy == Aggressor || IncapacitatedBy == Aggressor || Pawn != nullptr)
		{
			if (IsArrested() || bIsBeingArrested || IsCarried())
				return true;
			
			if (TimeSinceLastAggressiveForce < 2.0f)
				return false;
			
			if (IsSurrenderedFor(1.5f))
				return true;
			
			if (bHasDamagedSWATTeam || Aggressor->HasBeenDamagedByCharacter(this))
				return false;
			
			return true;
		}
	}

	// Priority 0: Arrested or Surrendered. Always unjustified if arrested, knocked out or surrendered (with a buffer of timer for surrendered check)
	if (IsArrested() || bIsBeingArrested || IsCarried())
		return true;
	
	if (TimeSinceLastAggressiveForce < 4.0f)
		return false;
	
	if (IsSurrenderedFor(1.5f)) // Wait a bit before considering justification
		return true;

	if (((IsDeadOrUnconscious() || IsIncapacitated()) && TimeIncapacitated > 3.0f) && !bIsArterialBleeding)
		return true;
	
	if (bHasEverExitedSurrender && IsSuspect())
		return false;

	// Priority 1: Game mode exclusions. Force always justified if a mode has ROE evaulation disabled
	// Still need to check the above, if we taze/melee/pepperspray an arrested AI
	if (const ACoopGM* CoopGM = GetWorld()->GetAuthGameMode<ACoopGM>())
	{
		if (CoopGM->IsROEDisabled())
			return false;
	}

	// Priority 2: SWAT team damage. If the aggressor (player) or swat team was ever damaged by this AI, force is justified
	if (bHasDamagedSWATTeam || Aggressor->HasBeenDamagedByCharacter(this))
		return false;
	
	// Priority 3: Explosive vest, if not surrendered always a justified use of force
	if (IsWearingExplosiveVest())
		return false;

	// Priority 4: AI behaviour states
	if (IsTableMontagePlaying("tp_melee"))
		return false;

	if (IsRaisingWeapon())
		return false;

	if (IsTakingHostage())
		return false;
	
	if (bHasEverShot)
		return false;

	if (bAimingAtTarget || bDrawingWeapon || bPickingUpWeapon)
		return false;
	
	if (bIsPlayingDead && TimePlayingDead > 2.0f)
		return true;
	
	if (bIsFakeSurrender || bHasEverFakeSurrendered)
		return false;

	if (!bAimingAtTarget && (!IsStrafing() && TimeNotStrafing < 2.0f))
		return false;
	
	if (IsTakingCover())
		return false;
	
	if (IsGettingUp())
		return true;

	if (IsHesitatingFor(1.5f) || IsStartling())
		return true;

	if (IsTakingCoverAtLandmark() || bIsExitingLandmark)
		return true;

	if (const ACyberneticController* AIController = GetCyberneticsController())
	{
		if (const UBaseCombatActivity* CombatActivity = AIController->GetCombatActivity())
		{
			if (CombatActivity->TimeSpentWithWeaponUp > 0.0f)
				return false;

			if (CombatActivity->GetCombatMoveActivity() != nullptr)
				return false;
		}
		
		if (AIController->GetAwarenessState() < EAIAwarenessState::Suspicious)
			return true;
	}
	
	return false;
}

float ACyberneticCharacter::GetVisibleSWATPercentage() const
{
	return VisibleSwatPercentage;
}

void ACyberneticCharacter::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	if (bIsPlayingDead)
	{
		if (!bHasBeenReported)
		{
			InteractInstigator->Server_ReportTarget_Implementation(this);
			return;
		}
	}
	
	Super::Interact_Implementation(InteractInstigator, InInteractableComponent);
}

FName ACyberneticCharacter::DetermineAnimatedIcon_Implementation() const
{
	if (bIsPlayingDead && !bHasBeenReported)
	{
		return "Report Dead";
	}

	return Super::DetermineAnimatedIcon_Implementation();
}

FText ACyberneticCharacter::DetermineActionText_Implementation() const
{
	if (bIsPlayingDead && !bHasBeenReported)
	{
		FString ActionPromptKey = !IsFullHealth() ? "ReportDead" : "ReportDOA"; 
		return FText::FromStringTable("ActionPromptTable", ActionPromptKey);
	}
	
	return Super::DetermineActionText_Implementation();
}

bool ACyberneticCharacter::CanInteract_Implementation() const
{
	if (bIsPlayingDead && !bHasBeenReported)
	{
		return CanShowActionPrompt1();
	}

	return Super::CanInteract_Implementation();
}

float ACyberneticCharacter::DetermineInteractionDistance_Implementation() const
{
	if (bIsPlayingDead && !bHasBeenReported)
	{
		return 500.0f;
	}

	return Super::DetermineInteractionDistance_Implementation();
}

bool ACyberneticCharacter::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (bDiedWhilstTraversingHole || bDiedWhilstHiding)
		return true;
	
	return Super::CanInteractThroughHitActors_Implementation(Hit);
}

void ACyberneticCharacter::Secure_Implementation(AReadyOrNotCharacter* InInstigator)
{
	if (AZipcuffs* Zipcuffs = Cast<AZipcuffs>(InInstigator->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Zipcuffs)))
	{
		Zipcuffs->Server_ArrestStart(this);
	}
}

bool ACyberneticCharacter::IsSecured_Implementation() const
{
	if (IsOnSWATTeam())
		return true;
	
	if (bIsInBitsAndPieces)
		return true;
	
	// Can't arrest people who are missing arms or head
	if (GibComponent && (GibComponent->IsLimbGibbed(EGibAreas::GA_LeftArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_RightArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_Head)))
		return true;
	
	if (IsRagdollBlending()) // failsafe
		return true;

	return IsArrested() || IsArrestedAndDead();
}

FVector ACyberneticCharacter::GetLocation_Implementation() const
{
	return GetActorLocation();
}

bool ACyberneticCharacter::CanBeSecured_Implementation() const
{
	if (IsOnSWATTeam())
		return false;
	
	if (bIsInBitsAndPieces)
		return false;
	
	// Can't arrest people who are missing arms
	if (GibComponent && (GibComponent->IsLimbGibbed(EGibAreas::GA_LeftArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_RightArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_Head)))
		return false;
	
	if (IsRagdollBlending()) // failsafe
		return false;

	if (!IsDeadOrUnconscious())
	{
		if (!CanArrest())
			return false;
	}
	
	return !bArrestComplete;
}

bool ACyberneticCharacter::CanBeSecuredByTrailers_Implementation() const
{
	if (IsOnSWATTeam())
		return false;
	
	if (bIsInBitsAndPieces)
		return false;
	
	return bArrestComplete || IsDeadOrUnconscious() || IsIncapacitated() || IsInRagdoll();
}

bool ACyberneticCharacter::CanIssueCommand_Implementation() const
{
	return true;
}

AActor* ACyberneticCharacter::GetCommandActor_Implementation() const
{
	return const_cast<ACyberneticCharacter*>(this);
}

void ACyberneticCharacter::OnSurrenderFinished(const FString& CustomMontage)
{
	if (IsArrested())
		return;

	if (PlayMontageFromTable(CustomMontage.IsEmpty() ? "tp_surrender_exit" : CustomMontage))
	{
		ResetSurrenderStates();

		if (CustomMontage.Contains("fake"))
			bIsFakeSurrender = true;
	}
}

bool ACyberneticCharacter::SurrenderExit(const ESurrenderExitType ExitType, const FVector FocalPoint)
{
	if (IsArrested())
		return false;

	if (!IsSurrendered())
		return false;
	
	FString Montage;
	switch (ExitType)
	{
		case ESurrenderExitType::None:		Montage = "tp_surrender_exit"; break;
		case ESurrenderExitType::Default:	Montage = "tp_surrender_exit"; break;
		case ESurrenderExitType::Gun:		Montage = "tp_surrender_exit_fake"; break;
		case ESurrenderExitType::Knife:		Montage = "tp_surrender_exit_fake_knife"; break;
		default:							Montage = "tp_surrender_exit"; break;
	}
	
	if (IsTableMontagePlaying(Montage))
		return false;

	if (PlayMontageFromTableWithFocalPoint(Montage, FocalPoint))
	{
		TimeSinceLastAggressiveForce = 0.0f;
		
		if (ExitType == ESurrenderExitType::Knife)
			PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::KNIFE_THE_PLAYER, 3.0f);
		
		bIsFakeSurrender = ExitType > ESurrenderExitType::Default;
			
		if (bIsFakeSurrender)
		{
			// Threating someone again... revoke all penalties that was given to the player by this AI
			ScoringComponent->RevokeAllPenalties();
		}
		
		bHasEverExitedSurrender = true;
		
		UMoraleComponent::ResetMoraleOnCharacter(this);

		Stress = StartingStress;

		bSurrendered = false;
		SurrenderedTime = 0.0f;
		
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::ResetSurrenderStates, 0.2f);
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::ResetFakeSurrenderStates, 1.0f);
		
		OnExitedSurrender.Broadcast(this, ExitType);

		return true;
	}

	return false;
}

bool ACyberneticCharacter::IsExitingSurrender() const
{
	return IsTableMontagePlaying("tp_surrender_exit") ||
			IsTableMontagePlaying("tp_surrender_exit_fake") ||
			IsTableMontagePlaying("tp_surrender_exit_fake_knife");
}

bool ACyberneticCharacter::IsHiding() const
{
	return Rep_HidingAnimState.bIsHiding && Rep_HidingAnimState.bLooping;
}

bool ACyberneticCharacter::IsProtectedAgainstInstantKnockout() const
{
	for (ABaseItem* Item : InventoryComp->GetInventoryItems())
	{
		if (const AHeadwear* Headgear = Cast<AHeadwear>(Item))
		{
			if (Headgear->bProtectsAgainstInstantKnockout)
			{
				return true;
			}
		}
	}

	return false;
}

void ACyberneticCharacter::Surrender()
{
	#if !UE_BUILD_SHIPPING
	if (CVarRonNoSurrender.GetValueOnAnyThread() == 1)
		return;
	#endif

	if (IsOnSWATTeam())
		return;

	if (IsInRagdoll() || bRecoveringFromRagdoll || (LastGetUpMontage && GetCurrentMontage() == LastGetUpMontage))
		return;

	if (IsExitingSurrender())
		return;

	if (IsTableMontagePlaying("tp_surrender") ||
		IsTableMontagePlaying("tp_fake_surrender"))
		return;

	if (bSurrendered)
		return;
	
	if (IsSurrenderedFor(1.5f))
	{
		bSurrenderComplete = true;
		return;
	}
	
	if (IsArrested())
	{
		bSurrendered = true;
		bIsFakeSurrender = false;
		return;
	}

	if (bCommitingSuicide)
		return;

	FString SurrenderAnim = "tp_surrender";

	if (GetCyberneticsController() && GetCyberneticsController()->GetTrackedTarget())
	{
		if (AssignedAIData->bChanceToSurrenderWithItem)
		{
			if (AssignedAIData->SurrenderItems.Num() > 0)
			{
				float Chance = AI_CONFIG_GET_FLOAT("ChanceToSurrenderWithItem", 0.1f);
				if (AssignedAIData->bOverrideSurrenderWithItemChance)
					Chance = AssignedAIData->SurrenderWithItemChance;
				
				const bool bSurrenderWithItem = Chance >= 1.0f || FMath::FRand() < Chance;
				if (bSurrenderWithItem)
				{
					if (AssignedAIData->SurrenderItems.Num() == 1)
					{
						SurrenderAnim = "tp_surrender_with_" + AssignedAIData->SurrenderItems[0];
					}
					else
					{
						SurrenderAnim = "tp_surrender_with_" + AssignedAIData->SurrenderItems[FMath::RandRange(0, AssignedAIData->SurrenderItems.Num() - 1)];
					}

					bSurrenderingWithItem = true;
					
					if (!DoesMontageFromTableExist(SurrenderAnim))
					{
						SurrenderAnim = "tp_surrender";
						bSurrenderingWithItem = false;
					}
				}
			}
		}
	}

	if (const TSoftObjectPtr<UAnimMontage> ChosenAnim = PlayMontageFromTable(SurrenderAnim))
	{
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_COMPLIANT, "", false);

		bSurrendered = true;
		bSurrenderComplete = false;
		bIsFakeSurrender = false;

		if (GetCyberneticsController())
			GetCyberneticsController()->AbortMove();

		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::Event_SurrenderCompleted, ChosenAnim->GetPlayLength() - 1.0f);

		Multicast_ChangeFaceEmotion(IsCivilian() ? ECharacterEmotion::Afraid : ECharacterEmotion::Angry, 15.0f, 1.0f, 0.1f, 50);

		OnSurrendered.Broadcast(this);
	}
}

void ACyberneticCharacter::FakeSurrender()
{
	if (!GetCyberneticsController())
		return;
	
	if (IsArrested())
		return;

	if (IsInRagdoll())
		return;
	
	if (IsCivilian())
	{
		Surrender();
		return;
	}
	
	if (bIsFakeSurrender)
		return;

	if (IsTableMontagePlaying("tp_fake_surrender"))
		return;
	
	if (bHasEverFakeSurrendered || IsWearingExplosiveVest()) // Absolutely prevent explosive vest suspects from fake surrendering
	{
		Surrender();
		return;
	}

	if (PlayMontageFromTable("tp_fake_surrender"))
	{
		SurrenderedTime = 0.0f;
		bSurrendered = false;
		bSurrenderComplete = false;
		bIsFakeSurrender = true;
		bHasEverFakeSurrendered = true;
		
		GetCyberneticsController()->AbortMove();
		PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::BARK_COMPLIANT);

		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::ResetFakeSurrenderStates, 1.0f);

		OnFakeSurrendered.Broadcast(this);
	}
}

void ACyberneticCharacter::Event_SurrenderCompleted()
{
	bSurrendered = true;
	bSurrenderComplete = true;
	bIsFakeSurrender = false;

	InventoryComp->ThrowAllWeapons();
}

void ACyberneticCharacter::OnRep_Surrendered()
{
	if (bSurrendered)
	{
		if (!bHasPlayedSurrenderAnim)
		{
			bHasPlayedSurrenderAnim = true;
			PlayMontageFromTable("tp_surrender");
		}
	}
	else
	{
		bHasPlayedSurrenderAnim = false;
	}
}

bool ACyberneticCharacter::IsWearingExplosiveVest() const
{
	if (const UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		return InventoryComponent->IsWearingExplosiveVest();
	}

	return false;
}

bool ACyberneticCharacter::IsWearingHeadArmor() const
{
	if (const UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		return InventoryComponent->IsWearingHeadArmour();
	}

	return false;
}

void ACyberneticCharacter::ReactToHeadBeanbagHit(float Damage, FPointDamageEvent* HitEvent)
{
	if (IsDeadOrUnconscious())
		return;

	if (IsProtectedAgainstInstantKnockout())
		return;
	
	if (!IsOnSWATTeam())
	{
		DepleteHealth();
		//Knockout(5.0f);
	}
}

void ACyberneticCharacter::OnKilled(AReadyOrNotCharacter* InstigatorCharacter)
{
	Super::OnKilled(InstigatorCharacter);

	ReasonsToSprint.Empty();
	ReasonsToWalk.Empty();

	bIsMoving = false;
	bIsStrafing = false;	
	
	if (GetCyberneticsController())
	{
		ACyberneticCharacter* ClosestFriendlyOrNeutral = UReadyOrNotFunctionLibrary::FindClosestActor<ACyberneticCharacter>(GetWorld(), GetActorLocation(), 1250.0f, [&](const ACyberneticCharacter* Character, const float Distance)
		{
			if (Character == this)
				return false;

			if (Character->IsDeadOrUnconscious() || Character->IsIncapacitated() || Character->IsPlayingDead())
				return false;
			
			return GetCyberneticsController()->IsCharacterFriendly(const_cast<ACyberneticCharacter*>(Character)) ||
					GetCyberneticsController()->IsCharacterNeutral(const_cast<ACyberneticCharacter*>(Character));
		});

		if (ClosestFriendlyOrNeutral)
		{
			ClosestFriendlyOrNeutral->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED, false);
		}
	}
}

void ACyberneticCharacter::OnIncapacitated(AReadyOrNotCharacter* InstigatorCharacter)
{
	ReasonsToSprint.Empty();
	ReasonsToWalk.Empty();
	
	bIsMoving = false;
	bIsStrafing = false;	
	
	GetAudioComp()->Stop();
	PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_INCAP_IDLE, "", false);
	
	if (GetCyberneticsController())
	{
		ACyberneticCharacter* ClosestFriendlyOrNeutral = UReadyOrNotFunctionLibrary::FindClosestActor<ACyberneticCharacter>(GetWorld(), GetActorLocation(), 1250.0f, [&](const ACyberneticCharacter* Character, const float Distance)
		{
			if (Character == this)
				return false;

			if (Character->IsDeadOrUnconscious() || Character->IsIncapacitated() || Character->IsPlayingDead())
				return false;
			
			return GetCyberneticsController()->IsCharacterFriendly(const_cast<ACyberneticCharacter*>(Character)) ||
					GetCyberneticsController()->IsCharacterNeutral(const_cast<ACyberneticCharacter*>(Character));
		});

		if (ClosestFriendlyOrNeutral)
		{
			ClosestFriendlyOrNeutral->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED, false);
		}
	}
}

void ACyberneticCharacter::ApplyHeadDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ABaseArmour* HeadArmour = nullptr;
	if (GetInventoryComponent())
		HeadArmour = Cast<ABaseArmour>(GetInventoryComponent()->GetHeadArmour());

	bBlockedByHeadArmor = false;
	if (HeadArmour)
	{
		bBlockedByHeadArmor = HeadArmour->HandleDamage(Damage, DamageEvent, DamageCauser);
		Multicast_PlayArmourHitEffects(HeadArmour, DamageEvent.HitInfo, EventInstigator);
	} 
	else
	{
		if (DamageEvent.DamageTypeClass)
		{
			if (Cast<UBeanbagDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()))
			{
				DepleteHealth();
				
				return;
			}
		}

		Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	}

	if (!bBlockedByHeadArmor)
	{
		CharacterHealth->DecreaseLimbTickets(ELimbType::LT_Head, 1);
	}
}

void ACyberneticCharacter::ApplyLeftArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyLeftArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyRightArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyRightArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyLeftLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyLeftLegDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyRightLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyRightLegDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyLeftFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyLeftFootDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyRightFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	Super::ApplyRightFootDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ACyberneticCharacter::ApplyBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ABaseArmour* Armour = GetInventoryComponent() ? GetInventoryComponent()->GetArmour() : nullptr;
	if (Armour)
	{
		Armour->HandleDamage(Damage, DamageEvent, DamageCauser);
		Multicast_PlayArmourHitEffects(Armour, DamageEvent.HitInfo, EventInstigator);
	}
	else
	{
		Multicast_SpawnBloodEffects(DamageEvent.HitInfo, GetWoundSize(DamageCauser), EventInstigator);
	}
}

void ACyberneticCharacter::Multicast_PlayArmourHitEffects_Implementation(ABaseArmour* Armour, FHitResult Hit, AController* HitInstigator)
{
	// Don't spawn blood effects on the instigator's machine if they're a client as they're predicted
	if (IsValid(HitInstigator) && HitInstigator->IsLocalPlayerController() && !HitInstigator->HasAuthority())
		return;

	PlayArmourHitEffects(Armour, Hit);
}

FString ACyberneticCharacter::GetSpeechTypeForReport_Implementation()
{
	if (bIsPlayingDead)
	{
		if (IsCivilian())
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN;
		
		if (IsSuspect())
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT;
	}
	
	return Super::GetSpeechTypeForReport_Implementation();
}

void ACyberneticCharacter::ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation)
{
	Super::ReportToTOC_Implementation(Reporter, bPlayAnimation);

	FText ScoreName = FText::FromStringTable("ScoringTable", "TargetReported");
	
	if (IsCivilian())
		ScoreName = AScoringManager::BONUS_CIVILIAN_REPORTED;
	else if (IsSuspect())
		ScoreName = AScoringManager::BONUS_SUSPECT_REPORTED;
	else if (IsOnSWATTeam())
		ScoreName = AScoringManager::BONUS_DOWNED_OFFICER_REPORTED;
	
	const bool bShouldGiveFakeScore = bIsPlayingDead;
	if (bShouldGiveFakeScore)
	{
		ScoringComponent->GiveFakeScore(ScoreName, true);
		return;
	}

	ScoringComponent->GiveScore(ScoreName);

	ScoringComponent->DisplayBonuses(true, ScoreName);
	
	if (ScoringComponent->LastGivenPenalty.IsValid())
	{
		FText LastGivenPenalty = ScoringComponent->LastGivenPenalty.ScoreName;
		ScoringComponent->DisplayPenalties(true, LastGivenPenalty);
	}
	else
		ScoringComponent->DisplayPenalties();
}

UAnimMontage* ACyberneticCharacter::PlayMontageFromTableWithFocalPoint(const FString& Animation, const FVector& FocalPoint)
{
	if (UAnimMontage* Montage = PlayMontageFromTable(Animation))
	{
		if (const ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController()))
		{
			CyberneticController->GetTargetingComp()->ClearMontageFocalPoint();

			if (FocalPoint != FVector::ZeroVector)
			{
				CyberneticController->GetTargetingComp()->SetMontageFocalPoint(Montage, FocalPoint);
			}
		}

		return Montage;
	}
	
	return nullptr;
}

UAnimMontage* ACyberneticCharacter::PlayMontageFromTableWithIndexWithFocalPoint(const FString& Animation, const int32 Index, const FVector& FocalPoint)
{
	UAnimMontage* Montage = PlayMontageFromTableWithIndex(Animation, Index);
	
	if (FocalPoint == FVector::ZeroVector)
		return Montage;

	if (Montage)
	{
		if (const ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController()))
		{
			CyberneticController->GetTargetingComp()->ClearMontageFocalPoint();

			if (FocalPoint != FVector::ZeroVector)
			{
				CyberneticController->GetTargetingComp()->SetMontageFocalPoint(Montage, FocalPoint);
			}
		}

		return Montage;
	}
	
	return nullptr;
}

bool ACyberneticCharacter::PlayMontageWithFocalPoint(UAnimMontage* Montage, const FVector& FocalPoint)
{
	if (Montage)
	{
		Play3PMontage(Montage);
		
		if (FocalPoint == FVector::ZeroVector)
			return false;

		if (const ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController()))
		{
			CyberneticController->GetTargetingComp()->ClearMontageFocalPoint();

			if (FocalPoint != FVector::ZeroVector)
			{
				CyberneticController->GetTargetingComp()->SetMontageFocalPoint(Montage, FocalPoint);
			}
		}

		return true;
	}
	
	return false;
}

void ACyberneticCharacter::StopAnimationMontage(UAnimMontage* Montage)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.25f, Montage);
	}
}

bool ACyberneticCharacter::CanPlayDeathAnimation() const
{
	if (bDiedWhilstTraversingHole)
		return false;

	if (IsTraversingHole())
		return false;

	if (IsTakingCoverAtLandmark())
		return false;
	
	return Super::CanPlayDeathAnimation();
}

void ACyberneticCharacter::OnEquippedWeaponFire(ABaseMagazineWeapon* Weapon, bool bServer)
{
	Super::OnEquippedWeaponFire(Weapon, bServer);

	bHasEverShot = true;
	
	TimeSinceLastAggressiveForce = 0.0f;
	
	OnAIFire.Broadcast(this, Weapon, Weapon->GetBulletSpawn()->GetForwardVector());
	
	PlayShootingWeaponConversation();
	UMoraleComponent::IncreaseMoraleOnCharacter(this, AI_CONFIG_GET_FLOAT("FireWeaponMorale.Gain", 0.005f), "Weapon Fired");

	ScoringComponent->RevokeAllPenalties();

	if (Weapon->AnimationData)
	{
		UAnimMontage* RecoilMontage = nullptr;
		if (Weapon->AnimationData->Recoil_Level_01.Body_TP)
		{
			RecoilMontage = Weapon->AnimationData->Recoil_Level_01.Body_TP;
		}
			
		if (Weapon->AnimationData->FireSingle.Num() > 0)
		{
			RecoilMontage = Weapon->AnimationData->FireSingle[0].Body_TP;
		}
		
		const bool bIsRecoilAnimPlaying = GetCurrentMontage() == RecoilMontage;
		if (IsAny3PMontageActive() && !bIsRecoilAnimPlaying)
			return;
		
		Play3PMontage(RecoilMontage);
	}
}

void ACyberneticCharacter::PlayBarkOrStartConversation(FString SpeechRow, bool bHasSharedCooldown /*= false*/, float Cooldown /*= 3.0f*/)
{
	if (IsDeadOrUnconscious() || IsIncapacitated())
		return;
	
	if (USuspectsAndCivilianManager* SuspectsAndCivilianManager = USuspectsAndCivilianManager::Get(this))
	{
		if (bHasSharedCooldown)
			SuspectsAndCivilianManager->TriggerSharedBarkOrConversation(SpeechRow, this, Cooldown);
		else if (SpeechRow.Contains(VO_PREFIXES::BARK))
			PlayRawVO(SpeechRow);
		else
			SuspectsAndCivilianManager->PlayBarkOrStartConversation(SpeechRow, this);
	}
	else
	{
		if (bHasSharedCooldown)
			PlayRawVOWithCooldown(SpeechRow, Cooldown);
		else
			PlayRawVO(SpeechRow);
	}
}

bool ACyberneticCharacter::ToggleDoor(ADoor* Door, const bool bOpen, bool bBypassLock)
{
	if (!Door)
		return false;

	if (Door->IsDoorwayOnly())
		return false;

	if (Door->IsOpenBeyondCloseThreshold() && bOpen)
		return false;
	
	if (Door->AllMajorDoorChunksDestroyed())
		return false;

	if (Door->IsJammed())
		return false;
	
	if (IsTableMontagePlaying("tp_swat_door_push_open") || IsTableMontagePlaying("tp_npc_door_push_open"))
		return false;

	if (IsTableMontagePlaying("tp_swat_door_push_close") || IsTableMontagePlaying("tp_npc_door_push_close"))
		return false;
		
	if (!IsActive())
		return false;

	// Cant open because we know a trap is here
	if (Door->TeamKnowsDoorTrapState(IsSuspect()))
	{
		if (IsOnSWATTeam())
		{
			if (USWATManager* SWATManager = USWATManager::Get(this))
			{
				const ETeamType CommandTeam = GetCyberneticsController() && GetCyberneticsController()->GetCurrentActivity<UTeamBaseActivity>() ? GetCyberneticsController()->GetCurrentActivity<UTeamBaseActivity>()->SharedData->CommandTeam : GetTeam();
				const FVector CommandLocation = GetCyberneticsController() && GetCyberneticsController()->GetCurrentActivity<UTeamBaseActivity>() ? GetCyberneticsController()->GetCurrentActivity<UTeamBaseActivity>()->SharedData->CommandLocation : GetActorLocation();
				if (Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live)
				{
					PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_NO_MIRRORGUN);
					if (!GetCyberneticsController()->HasActivityType(UDisarmDoorTrapActivity::StaticClass()))
					{
						SWATManager->GiveDisarmTrapOnDoorCommand(Door, CommandTeam, CommandLocation);
					}
				
					return false;
				}
			}
		}

		if (Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live)
		{
			return false;
		}
	}

	// Prevents door opening from far distances
	if (FVector::Distance(Door->GetDoorMidLocation(), GetActorLocation()) > 500.0f)
		return false;
	
	if (!Door->CanOpenDoor(this))
		return false;
	
	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
	{
		Door->OpenDoor(this);
		Door->OpenSubDoor(this);
	}
	else
	{
		if (bOpen)
		{
			QueuedDoorToOpen = Door;
			QueuedDoorToClose = nullptr;
			
			if (Door->IsOpening())
				return false;
		}
		else
		{
			QueuedDoorToOpen = nullptr;
			QueuedDoorToClose = Door;

			if (Door->IsClosing())
				return false;
		}
		
		if (IsOnSWATTeam())
		{
			bool bHandsFull = false;
			if (IsCarrying())
			{
				bHandsFull = true;
			}
			
			if (bHandsFull)
			{
				if (bOpen)
					Door->OpenDoor(this);
				else
					Door->CloseDoor(this);
						
				return true;
			}
			
			if (PlayMontageFromTableWithFocalPoint(bOpen ? "tp_swat_door_push_open" : "tp_swat_door_push_close", Door->GetDoorHandleFront()->GetComponentLocation()))
			{
				PlayRawVO(Door->CanCloseDoor(this) ? VO_SWAT_GENERAL::RESPONSE_CLOSE_DOOR : VO_SWAT_GENERAL::RESPONSE_OPEN_DOOR); 
			}
		}
		else
		{
			if (bOpen)
			{
				if (!bBypassLock)
					bBypassLock = Door->bSuspectAlwaysUnlocks && IsSuspect();

				if (bBypassLock)
				{
					Door->UnlockDoor();
				}
				
				const bool bDoorUnlocked = (!Door->IsLocked() || bBypassLock) && !Door->IsJammed();
				if (bDoorUnlocked && Door->IsOpenBy_Angle(75.0f))
				{
					if (MoveStyle->ActiveGaitName == "walk")
						Door->OpenDoor(this);
					else	
						Door->BodyRamDoor(this);
				}
			}
			else
			{
				Door->CloseDoor(this);
			}
			
			QueuedDoorToOpen = nullptr;
			QueuedDoorToClose = nullptr;
		}
	}

	return true;
}

void ACyberneticCharacter::Server_KickQueuedDoor_Implementation()
{
	if (LastKickedDoor)
	{
		LastKickedDoor->KickDoor(this);
		if (LastKickedDoor->GetSubDoor())
		{
			LastKickedDoor->GetSubDoor()->KickDoor(this);
		}
	}
	
	LastKickedDoor = nullptr;
}

void ACyberneticCharacter::Server_KickBreakQueuedDoor_Implementation()
{
	if (LastKickedDoor)
	{
		LastKickedDoor->KickDoor(this, true);
	}
	
	LastKickedDoor = nullptr;
}

bool ACyberneticCharacter::CanEverSuicide() const
{
	return AssignedAIData->bCanEverSuicide;
}

bool ACyberneticCharacter::CanExitSurrender() const
{
	if (UActivityManager::AnyAIHasActivity<UArrestTargetActivity>([&](const UArrestTargetActivity* Activity)
	{
		if (Activity->ArrestTarget == this)
			return true;

		return false;
	}))
	{
		return false;
	}
	
	if (bIsBeingArrested)
		return false;

	if (bHasEverExitedSurrender)
		return false;
	
	return true;
}

ESurrenderExitType ACyberneticCharacter::DetermineSurrenderExitType() const
{
	if (!CanExitSurrender())
		return ESurrenderExitType::None;

	if (TimeSurrendered < 5.0f)
		return ESurrenderExitType::None;

	if (IsSuspect())
	{
		// Must be facing the enemy before doing a fake
		if (ClosestPawn)
		{
			const float DotProduct = FVector::DotProduct(GetActorForwardVector(), (ClosestPawn->GetActorLocation() - GetActorLocation()).GetSafeNormal2D());

			if (bShouldKnifeUpCloseWhenBeingLookedAt && DotProduct > 0.5f)
			{
				if (DistanceToClosestPawn < 250.0f)
				{
					return ESurrenderExitType::Knife;
				}
			}
			
			if (TimeNotBeingLookedAt > 2.0f)
			{
				if (IsWearingExplosiveVest())
					return ESurrenderExitType::Default;
				
				return DistanceToClosestPawn > 300.0f || DotProduct < 0.5f ? ESurrenderExitType::Gun : ESurrenderExitType::Knife; 
			}
		}
		else
		{
			if (TimeNotBeingLookedAt > 4.0f)
			{
				return ESurrenderExitType::Default;
			}
		}
	}
	else
	{
		if (TimeNotBeingLookedAt > RequiredTimeNotBeingLookedAt)
		{
			return ESurrenderExitType::Default;
		}
	}

#if UE_BUILD_DEVELOPMENT
	//DrawDebugString(GetWorld(), GetActorLocation(), FString::Printf(TEXT("{%.2f}/{%.2f}"),  TimeNotBeingLookedAt, RequiredTimeNotBeingLookedAt), nullptr, bLookedAtThisFrame ? FColor::Green : FColor::Red, 0.001f + DeltaSeconds);
#endif

	return ESurrenderExitType::None;
}

bool ACyberneticCharacter::IsPickingUpWeapon() const
{
	if (IsTableMontagePlaying("tp_pickup_rifle") || IsTableMontagePlaying("tp_pickup_pistol") || IsTableMontagePlaying("tp_pickup_item"))
		return true;
	
	if (!GetCyberneticsController())
		return false;

	if (const UPickupItemActivity* PickupItemActivity = GetCyberneticsController()->GetActivity<UPickupItemActivity>())
	{
		return PickupItemActivity->IsPickingUpItem();
	}
	
	if (const UPickupItemActivity* PickupItemActivity = GetCyberneticsController()->GetCurrentActivity<UPickupItemActivity>())
	{
		return PickupItemActivity->IsPickingUpItem();
	}

	return false;
}

bool ACyberneticCharacter::IsTakingCover() const
{
	if (!GetCyberneticsController())
		return false;

	if (!IsActive())
		return false;

	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsInCover();
	}
	
	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetCurrentActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsInCover();
	}

	return false;
}

bool ACyberneticCharacter::IsTakingCoverAtLandmark() const
{
	return Rep_HidingAnimState.bIsHiding;
}

bool ACyberneticCharacter::IsMovingToCover() const
{
	if (!GetCyberneticsController())
		return false;

	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsMovingToCover();
	}
	
	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetCurrentActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsMovingToCover();
	}

	return false;
}

bool ACyberneticCharacter::IsMovingToLandmarkCover() const
{
	if (!GetCyberneticsController())
		return false;

	if (const UTakeCoverAtLandmarkActivity* TakeCoverActivity = GetCyberneticsController()->GetActivity<UTakeCoverAtLandmarkActivity>())
	{
		return TakeCoverActivity->IsMovingToLandmark();
	}
	
	if (const UTakeCoverAtLandmarkActivity* TakeCoverActivity = GetCyberneticsController()->GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
	{
		return TakeCoverActivity->IsMovingToLandmark();
	}

	return false;
}

bool ACyberneticCharacter::IsFiringFromCover() const
{
	if (!GetCyberneticsController())
		return false;

	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsCoverFiring();
	}
	
	if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetCurrentActivity<UTakeCoverActivity>())
	{
		return TakeCoverActivity->IsCoverFiring();
	}

	return false;
}

bool ACyberneticCharacter::IsTakingHostage() const
{
	if (GetCyberneticsController())
	{
		if (const UTakeHostageActivity* TakeHostageActivity = GetCyberneticsController()->GetCurrentActivity<UTakeHostageActivity>())
		{
			return TakeHostageActivity->GetActiveStateID() > 0;
		}
	}
	
	return false;
}

bool ACyberneticCharacter::IsBeingTakenHostage() const
{
	return TakenHostageBy != nullptr;
}

bool ACyberneticCharacter::IsBeginningHostageTake() const
{
	if (TakenHostageBy)
	{
		return TakenHostageBy->IsBeginningHostageTake();
	}
		
	if (GetCyberneticsController())
	{
		if (const UTakeHostageActivity* TakeHostageActivity = GetCyberneticsController()->GetCurrentActivity<UTakeHostageActivity>())
		{
			return TakeHostageActivity->GetActiveStateID() == 1;
		}
	}
	
	return false;
}

bool ACyberneticCharacter::IsEndingHostageTake() const
{
	if (TakenHostageBy)
	{
		return TakenHostageBy->IsEndingHostageTake();
	}
		
	if (GetCyberneticsController())
	{
		if (const UTakeHostageActivity* TakeHostageActivity = GetCyberneticsController()->GetCurrentActivity<UTakeHostageActivity>())
		{
			return TakeHostageActivity->GetActiveStateID() == 3;
		}
	}
	
	return false;
}

UAnimMontage* ACyberneticCharacter::PlayMontageFromTable(const FString& Animation)
{
	// No montages allowed whilst hiding
	if (Rep_HidingAnimState.bIsHiding)
		return nullptr;

	if (bCommitingSuicide)
		return nullptr;

	if (Rep_HoleTraversalAnimState.bIsTraversing)
		return nullptr;

	if (IsPlayingDead())
		return nullptr;

	if (GetCyberneticsController())
	{
		GetCyberneticsController()->GetTargetingComp()->ClearMontageFocalPoint();

		if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetActivity<UTakeCoverActivity>())
		{
			if (TakeCoverActivity->IsPlayingCoverEnterAnims() ||
				TakeCoverActivity->IsPlayingCoverExitAnims())
				return nullptr;
		}
	}
	
	return Super::PlayMontageFromTable(Animation);
}

void ACyberneticCharacter::StartBeingTasered(float PingStunDuration, ATaser* WeaponUsed)
{
	if (TimesTasered > 0 && TimeSinceLastTasered < 3.0f)
	{
		// Abuse
		if (WeaponUsed && WeaponUsed->LeftProjectile)
			AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(WeaponUsed->ProjectileHitResult.GetActor()), this);
	}

	// Tasering suspect/civ in the head will cause knockout for duration
	// Todo: require animation for getting back up
	if (HeadBones.Contains(WeaponUsed->ProjectileHitResult.BoneName))
	{
		Knockout(10.0f);
		if (GetTeam() == ETeamType::TT_CIVILIAN)
		{
			// Abuse
			if (WeaponUsed && WeaponUsed->LeftProjectile)
				AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(WeaponUsed->ProjectileHitResult.GetActor()), this);
		}
	}

	TimesTasered++;
	Super::StartBeingTasered(PingStunDuration, WeaponUsed);

	if (IsSurrendered())
	{
		// Abuse
		if (WeaponUsed && WeaponUsed->LeftProjectile)
			AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(WeaponUsed->LeftProjectile->GetAttachParentActor()), this);
	}

	if (IsUnconsciousNotDead())
	{
		// Abuse
		if (WeaponUsed && WeaponUsed->LeftProjectile)
			AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(WeaponUsed->LeftProjectile->GetAttachParentActor()), this);
	}

	// Taser will cause civilians to surrender
	if (GetTeam() == ETeamType::TT_CIVILIAN)
	{
		if (TimesTasered > 1 || bChild)
		{
			// Abuse
			if (WeaponUsed && WeaponUsed->LeftProjectile)
				AReadyOrNotGameMode::AddAbuse(Cast<APlayerCharacter>(WeaponUsed->LeftProjectile->GetAttachParentActor()), this);
		}
	}
}

bool ACyberneticCharacter::OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		
		// if dead just spawn blood effects
		if (IsDeadOrUnconscious())
		{
			const FHitResult Hit = PointDamageEvent->HitInfo;
			Multicast_SpawnBloodEffects(Hit, GetWoundSize(DamageCauser), EventInstigator);

			if ((bPlayingDeathMontage && (IsBodyBone(Hit.BoneName) && TimeDead > 1.5f)) || (IsHeadBone(Hit.BoneName) && TimeDead > 1.0f))
			{
				// wrap this around test as it was causing some conflicts with existing cases
				if (!IsInRagdoll())
				{
					StopTPMontage(CurrentDeathMontage);
					CurrentDeathMontage = nullptr;
					bPlayingDeathMontage = false;
					EnableRagdoll();
				}
			}
			
			return false;
		}

		bool bAllowHitReactions = true;
		if (Archetype)
			bAllowHitReactions = !Archetype->bIgnoreDamageHitReactions;

		if (bAllowHitReactions)
		{
			if (IsTableMontagePlaying("tp_hits"))
			{
				if (FMath::FRand() < 0.3f)
					PlayMontageFromTable("tp_hits");
			}
			else
			{
				FName HitBone = PointDamageEvent->HitInfo.BoneName;
				
				float HitAnimationChance = 0.2f;
				if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
				{
					const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
					if (AmmoType)
						HitAnimationChance = AmmoType->HitsChance;
				}

				if (UKismetMathLibrary::RandomBoolWithWeight(HitAnimationChance) && !IsOnSWATTeam())
				{
					if (L_Arm.Contains(HitBone))
						PlayMontageFromTable("tp_hits_l_arm");
					else if (R_Arm.Contains(HitBone))
						PlayMontageFromTable("tp_hits_r_arm");
					else if (L_Leg.Contains(HitBone))
						PlayMontageFromTable("tp_hits_r_leg");
					else if (R_Leg.Contains(HitBone))
						PlayMontageFromTable("tp_hits_l_leg");
					else
						PlayMontageFromTable("tp_hits");
				}
				else
				{
					if (HeadBones.Contains(HitBone))
						PlayMontageFromTable("tp_swat_hit_pain_head");
					else if (L_Arm.Contains(HitBone))
						PlayMontageFromTable("tp_swt_hit_pain_arms");
					else if (R_Arm.Contains(HitBone))
						PlayMontageFromTable("tp_swt_hit_pain_arms");
					else if (L_Leg.Contains(HitBone))
						PlayMontageFromTable("tp_swt_hit_pain_legs");
					else if (R_Leg.Contains(HitBone))
						PlayMontageFromTable("tp_swt_hit_pain_legs");
					else
						PlayMontageFromTable("tp_swt_hit_pain_belly");
				}
			}
		}
	}

	// AI must be in behind the C2 explosive to take damage.. it will not damage the other way
	if (const APlacedC2Explosive* C2Explosive = Cast<APlacedC2Explosive>(DamageCauser))
	{
		if (C2Explosive->TargetItem)
		{
			const FVector V1 = C2Explosive->GetActorRightVector().GetSafeNormal2D();
			const FVector V2 = (GetActorLocation() - C2Explosive->GetActorLocation()).GetSafeNormal2D();

			if (FVector::DotProduct(V1, V2) > 0.0f)
			{
				return false;
			}
		}
	}

	if (/*UTrapDamage* TrapDamage = */Cast<UTrapDamage>(DamageEvent.DamageTypeClass->GetDefaultObject()))
	{
		return false;
	}

	UMoraleComponent::LowerMoraleOnCharacter(this, 0.1f, "Any Damage");
	
	UDamageType* DamageType = Cast<UDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
	
	if (EventInstigator)
	{
		if (APlayerCharacter* InstigatorCharacter = Cast<APlayerCharacter>(EventInstigator->GetPawn()))
		{
			#if !UE_BUILD_SHIPPING
			if (!InstigatorCharacter->HasGodMode())
			{
			#endif
				if (!IsOnSWATTeam() && IsUnjustifiedUseOfForce(InstigatorCharacter, Cast<ABaseItem>(DamageCauser), DamageType))
				{
					if (!IsDeadOrUnconscious())
					{
						ScoringComponent->GivePenalty(AScoringManager::PENALTY_UNAUTHORIZED_FORCE, bHasBeenReported);

						RespondToFriendlyFire(InstigatorCharacter);
						
						if (HasLineOfSightToCharacter(InstigatorCharacter) && IsCivilian()) // Only allow civilians, TOC should be more lax on suspects
						{
							PlayROEViolateTOCResponse();
							
							bPendingROEViolateResponseOnReport = false;
						}
					}
				}
			#if !UE_BUILD_SHIPPING
			}
			#endif

			if (IsSuspect())
			{
				if (IsLowHealth() && InstigatorCharacter->IsOnSWATTeam())
				{
					PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::PLAYER_NEARLY_KILLS_ENEMY, true, 5.0f);			
				}
			}
		}
	}
	
	if (GetCyberneticsController())
	{
		if (EventInstigator && EventInstigator->GetCharacter())
		{
			// Apply a damage report event whenever we try and damage another AI
			UAISense_Damage::ReportDamageEvent(GetWorld(), this, EventInstigator->GetPawn(), Damage, EventInstigator->GetCharacter()->GetActorLocation(), GetActorLocation());
		}

		if (DamageEvent.DamageTypeClass)
		{
			if (const UStunDamage* StunDamage = Cast<UStunDamage>(DamageEvent.DamageTypeClass->GetDefaultObject()))
			{
				const EStunType StunType = StunDamage->StunType;

				switch (StunType)
				{
					case EStunType::ST_Tased:
					{
						UMoraleComponent::LowerMoraleOnCharacter(this, FMath::Abs(AI_CONFIG_GET_FLOAT("TaserMorale.Damage")), "Taser");
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("TaserStunDamage");
					}
					break;
					
					case EStunType::ST_Gassed:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("GrenadeStunDamage");
					break;
					
					case EStunType::ST_Flash:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("GrenadeStunDamage");
					break;
					
					case EStunType::ST_Stung:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("GrenadeStunDamage");
					break;
					
					case EStunType::ST_Beanbag:
					{
						UMoraleComponent::LowerMoraleOnCharacter(this, AI_CONFIG_GET_FLOAT("BeanbagShotgunMorale.Damage"), "Beanbag");
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("BeanbagShotgunStunDamage");
					}
					break;

					case EStunType::ST_Pepperball:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("PepperballStunDamage");

					break;

					case EStunType::ST_Rubberball:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("RubberballStunDamage");
					break;

					case EStunType::ST_Pepperspray:
						StunHealthRemaining -= AI_CONFIG_GET_FLOAT("GrenadeStunDamage");
					break;
					
					default:
					break;
				}

				if (StunType != EStunType::ST_None && !IsOnSWATTeam())
				{
					if (StunHealthRemaining <= 0.0f)
					{
						StunHealthRemaining = 100.0f;
						TimeSinceLastTakenStunDamage = 0.0f;

						switch (StunType)
						{
							case EStunType::ST_Gassed:			/* PlayMontageFromTable("tp_gas"); */ break;
							case EStunType::ST_Stung:			PlayMontageFromTable("tp_stinger"); break;
							case EStunType::ST_Flash:			PlayMontageFromTable("tp_flashbang"); break;
							case EStunType::ST_Tased:			PlayMontageFromTable("tp_taser"); break;
							case EStunType::ST_Pepperball:		PlayMontageFromTable("tp_stinger"); break;
							case EStunType::ST_Rubberball:		PlayMontageFromTable("tp_stinger"); break;
							case EStunType::ST_Pepperspray:		break; // Civilian AI will play montage from StartStun 
							default:							PlayMontageFromTable("tp_hits"); break;
						}
						
						PlayStunnedVoiceLine(StunType, false);

						GetCyberneticsController()->OnStunDamageTaken(StunType);
					}
					else
					{
						PlayStunnedVoiceLine(StunType, true);
						return false;
					}
				}
			}
			else
			{
				GetCyberneticsController()->BulletsFiredTowardsAccuracyPenalty /= 2;
			}
		}
	}

	const UBulletDamageType* BulletDamage = Cast<UBulletDamageType>(DamageType);
	UStunDamage* StunDamage = Cast<UStunDamage>(DamageType);

	bool bTookStunDamage = false;
	if (StunDamage)
		bTookStunDamage = TryApplyStunDamage(StunDamage, Damage, DamageEvent, EventInstigator, DamageCauser);
	
	if (BulletDamage)
		TryApplyBulletDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (bTookStunDamage)
	{
		if (DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->IsChildOf(UStunDamage::StaticClass()))
		{
			float SurrenderChance = URosterManager::GetSquadTraitValue("Pacifier", GetWorld());
			if (UKismetMathLibrary::RandomBoolWithWeight(SurrenderChance))
				Surrender();
		}
	}
	
	return true;
}

void ACyberneticCharacter::OnOfficerShouted_Implementation(AReadyOrNotCharacter* Shouter, const bool bLOS)     
{
	OnHeardOfficerYell.Broadcast(Shouter, bLOS);
	
	bHeardYellFromOfficer = true;
	TimeSinceHeardOfficerYell = 0.0f;
	
	const FVector DirectionToEnemy = (GetActorLocation() - Shouter->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, Shouter->GetActorForwardVector());

	// Is the shouter yelling at us?
	if (Cast<ASWATCharacter>(Shouter) || ForwardDotProduct > 0.975f)
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::BarkNonCompliant, 0.85f);

		if (IsArrestedOrSurrendered())
			PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_IMMUNE);
	}
}

void ACyberneticCharacter::BarkNonCompliant()
{
	if (IsWearingExplosiveVest())
		return;
	
	if (IsActive() && !IsPlayingDead())
	{
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_NON_COMPLIANT);
	}
}

void ACyberneticCharacter::DrawWeapon()
{
	if (IsDeadOrUnconscious() || IsArrestedOrSurrendered() || !GetEquippedItem())
		return;

	PlayMontageFromTable("tp_draw_weapon");
}

void ACyberneticCharacter::DoLowReadyTrace()
{
	SCOPE_CYCLE_COUNTER(STAT_RoNDoLowReadyTrace);
	
	if (!GetCyberneticsController())
		return;
	
	if (!HasAuthority())
		return;
	
	if (!IsOnSWATTeam())
		return;

	if (GetCyberneticsController()->GetTrackedTarget())
	{
		return;
	}

	if (!GetEquippedItem())
		return;
	
	FCollisionObjectQueryParams LowReadyObjectTraceParams;
	LowReadyObjectTraceParams.AddObjectTypesToQuery(ECC_WorldStatic);
	LowReadyObjectTraceParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	LowReadyObjectTraceParams.AddObjectTypesToQuery(ECC_Pawn);
	LowReadyObjectTraceParams.AddObjectTypesToQuery(ECC_Destructible);

	FCollisionQueryParams LowReadyTraceCollisionParams(FName("LineOfSight"));
	LowReadyTraceCollisionParams = GetCollisionQueryParameters();
	
	// Ignore all blocking volumes
	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetLevel()->GetLevelScriptActor()))
	{
		LowReadyTraceCollisionParams.AddIgnoredActors((TArray<AActor*>)LevelScript->BlockingVolumesInLevel);
	}

	//float DistanceScale = (1.0f - LastNonADSRunSpeedPercent) * -30.0f;
	CachedSpine3Offset = GetCapsuleComponent()->GetComponentLocation() - GetMesh()->GetBoneLocation("spine_3");
	
	FVector LowReadyTraceStart = GetCapsuleComponent()->GetComponentLocation() - CachedSpine3Offset;
	FVector LowReadyTraceEnd = LowReadyTraceStart + ((GetActorRotation() + FRotator(AimOffset.X, AimOffset.Y, 0.0f)).Vector() * 500.0f);
	FVector LowReadyTraceEndLeft = LowReadyTraceStart + ((GetActorRotation() + FRotator(AimOffset.X, AimOffset.Y + -7.0f, 0.0f)).Vector() * 500.0f);
	FVector LowReadyTraceEndRight = LowReadyTraceStart + ((GetActorRotation() + FRotator(AimOffset.X, AimOffset.Y + 7.0f, 0.0f)).Vector() * 500.0f);

	FHitResult LowReadyTraceHit;
	FHitResult LowReadyTraceHitLeft;
	FHitResult LowReadyTraceHitRight;

	GetWorld()->LineTraceSingleByObjectType(LowReadyTraceHit, LowReadyTraceStart, LowReadyTraceEnd, LowReadyObjectTraceParams, LowReadyTraceCollisionParams);
	GetWorld()->LineTraceSingleByObjectType(LowReadyTraceHitLeft, LowReadyTraceStart, LowReadyTraceEndLeft, LowReadyObjectTraceParams, LowReadyTraceCollisionParams);
	GetWorld()->LineTraceSingleByObjectType(LowReadyTraceHitRight, LowReadyTraceStart, LowReadyTraceEndRight, LowReadyObjectTraceParams, LowReadyTraceCollisionParams);

	#if !UE_BUILD_SHIPPING
	if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), LowReadyTraceStart, LowReadyTraceEnd, LowReadyTraceHit.bBlockingHit ? FColor::Red : FColor::White, false, 0.25f);
		DrawDebugLine(GetWorld(), LowReadyTraceStart, LowReadyTraceEndLeft, LowReadyTraceHitLeft.bBlockingHit ? FColor::Red : FColor::White, false, 0.25f);
		DrawDebugLine(GetWorld(), LowReadyTraceStart, LowReadyTraceEndRight, LowReadyTraceHitRight.bBlockingHit ? FColor::Red : FColor::White, false, 0.25f);
	}
	#endif

	constexpr float MaxLeanTraceDistance = 150.0f;

	FHitResult LeanLeftTrace, LeanRightTrace;
	FVector LeanTraceStart = GetCapsuleComponent()->GetComponentLocation();
	FVector LeanLeftTraceEnd = LeanTraceStart + ((GetActorRotation() + FRotator(0.0f, -45.0f, 0.0f)).Vector() * MaxLeanTraceDistance);
	FVector LeanRightTraceEnd = LeanTraceStart + ((GetActorRotation() + FRotator(0.0f, 45.0f, 0.0f)).Vector() * MaxLeanTraceDistance);
	
	GetWorld()->LineTraceSingleByObjectType(LeanLeftTrace, LeanTraceStart, LeanLeftTraceEnd, LowReadyObjectTraceParams, LowReadyTraceCollisionParams);
	GetWorld()->LineTraceSingleByObjectType(LeanRightTrace, LeanTraceStart, LeanRightTraceEnd, LowReadyObjectTraceParams, LowReadyTraceCollisionParams);

	#if !UE_BUILD_SHIPPING
	if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), LeanTraceStart, LeanLeftTrace.TraceEnd, LeanLeftTrace.bBlockingHit ? FColor::Red : FColor::White, false, 0.25f);
		DrawDebugLine(GetWorld(), LeanTraceStart, LeanRightTrace.TraceEnd, LeanRightTrace.bBlockingHit ? FColor::Red : FColor::White, false, 0.25f);
	}
	#endif
	
	bool bLookingAtSwat = false;
	if (APlayerCharacter* pc = Cast<APlayerCharacter>(LowReadyTraceHit.GetActor()))
	{
		bLookingAtSwat = pc->IsOnSWATTeam();
	}

	if (!bLookingAtSwat)
	{
		if (APlayerCharacter* pc = Cast<APlayerCharacter>(LowReadyTraceHitLeft.GetActor()))
		{
			bLookingAtSwat = pc->IsOnSWATTeam();
		}
	}

	if (!bLookingAtSwat)
	{
		if (APlayerCharacter* pc = Cast<APlayerCharacter>(LowReadyTraceHitRight.GetActor()))
		{
			bLookingAtSwat = pc->IsOnSWATTeam();
		}
	}
	
	bool bShouldLowReady = false;

	// add a small low ready buffer so if we are low ready and that pushes us out of the zone we dont flip flop
	if ((LowReadyTraceHit.Distance > 0.0f && LowReadyTraceHit.Distance <  MaxLeanTraceDistance) ||
		(LowReadyTraceHitLeft.Distance > 0.0f && LowReadyTraceHitLeft.Distance <  MaxLeanTraceDistance) ||
		(LowReadyTraceHitRight.Distance > 0.0f && LowReadyTraceHitRight.Distance < MaxLeanTraceDistance) ||
		bLookingAtSwat)
	{
		bShouldLowReady = true;
	}
	
	for (AReadyOrNotCharacter* KnownEnemy : GetCyberneticsController()->GetTargetingComp()->KnownEnemies)
	{
		if (KnownEnemy == LowReadyTraceHit.GetActor())
		{
			bShouldLowReady = false;
			break;
		}
	}

	if (bShouldLowReady)
	{
		SetLowReady(false, true);
	}
	else
	{
		SetLowReady(false, false);
	}
}

void ACyberneticCharacter::MeleeVictim(AReadyOrNotCharacter* Victim)
{
	if (!Victim)
		return;
	
	if (!CanMelee())
		return;

	// Are we facing the victim?
	if (GetDotProductTo(Victim) < 0.0f)
		return;
	
	TimeSinceLastAggressiveForce = 0.0f;

	if (IsSuspect() && IsWearingExplosiveVest())
	{
		if (IsTableMontagePlaying("tp_spct_detonatevest"))
			return;
		
		PlayMontageFromTable("tp_spct_detonatevest");
		Multicast_ChangeFaceEmotion(ECharacterEmotion::Angry, 5.0f, 1.0f, 0.25f, 1);
		return;
	}
	
	if (const AMeleeWeapon* MeleeWeapon = Cast<AMeleeWeapon>(GetEquippedItem()))
	{
		if (IsTableMontagePlaying(MeleeWeapon->MeleeMontage))
			return;
		
		//PlayMontageFromTableWithFocalPoint(MeleeWeapon->MeleeMontage, Victim->GetActorLocation() + (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 200.0f);
		PlayMontageFromTable(MeleeWeapon->MeleeMontage);
		Multicast_ChangeFaceEmotion(ECharacterEmotion::Angry, 5.0f, 1.0f, 0.25f, 1);
		return;
	}

	if (IsTableMontagePlaying("tp_melee"))
		return;

	//PlayMontageFromTableWithFocalPoint("tp_melee", Victim->GetActorLocation() + (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 200.0f);
	PlayMontageFromTable("tp_melee");
	Multicast_ChangeFaceEmotion(ECharacterEmotion::Angry, 5.0f, 1.0f, 0.25f, 1);
}

// Called from anim notify
void ACyberneticCharacter::DoMelee(bool bLocal)
{
	if (PendingMeleeTarget)
	{
		FHitResult FakeHit;
		FakeHit.TraceStart = GetActorLocation();
		FakeHit.TraceEnd = PendingMeleeTarget->GetActorLocation();
		FakeHit.Distance = 100.0f;
		FakeHit.ImpactPoint = PendingMeleeTarget->GetActorLocation();
		FakeHit.ImpactNormal = (FakeHit.TraceEnd - FakeHit.TraceStart).GetSafeNormal();
		FakeHit.BoneName = NAME_None;
		FakeHit.Component = PendingMeleeTarget->GetMesh();
		// ##UE5UPGRADE## Compatibility
		FakeHit.HitObjectHandle = PendingMeleeTarget;
		
		OnMeleeTrace(FakeHit, bLocal);
		PendingMeleeTarget = nullptr;
		return;
	}
	
	// Trace forward by a little bit to find our victim
	FHitResult MeleeTraceResult;
	FCollisionQueryParams Params = GetCollisionQueryParameters();

	//FVector TraceDirection = (GetActorRotation() + FRotator(AimOffset.X, AimOffset.Y, 0.0f)).Vector();
	FVector StartTrace = GetActorLocation();
	FVector EndTrace = StartTrace + (GetActorForwardVector() * 150.0f);

	//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Cyan, false, 2.0f, 0, 5.0f);
	
	if (GetWorld()->SweepSingleByChannel(MeleeTraceResult, StartTrace, EndTrace, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(10.0f), Params))
	{
		OnMeleeTrace(MeleeTraceResult, bLocal);
	}
}

void ACyberneticCharacter::OnMeleeTrace(const FHitResult HitResult, const bool bLocal)
{
	Super::OnMeleeTrace(HitResult, bLocal);
	
	bHasDamagedSWATTeam = true;
	
	bHasEverShot = true;
	
	ScoringComponent->RevokeAllPenalties();
	
	if (GetEquippedItem<AMeleeWeapon>())
		PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::KNIFE_THE_PLAYER, 3.0f);
	else
		PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::BARK_HIT_THE_PLAYER, 3.0f);

	UMoraleComponent::IncreaseMoraleOnCharacter(this, 0.25f, "Melee Target");

	if (AReadyOrNotCharacter* HitCharacter = Cast<AReadyOrNotCharacter>(HitResult.GetActor()))
	{
		RecentMeleeVictim = HitCharacter;

		if (int32* ValuePtr = MeleeCountMap.Find(RecentMeleeVictim))
		{
			++(*ValuePtr);
		}
		else
		{
			MeleeCountMap.Add(RecentMeleeVictim, 1);
		}
	}
}

bool ACyberneticCharacter::CanMelee() const
{
	if (IsStunned())
		return false;
	
	return Super::CanMelee();
}

void ACyberneticCharacter::PlayShootingWeaponConversation()
{
	PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::TELL_SHOOTING, true, 10.0f);
}

bool ACyberneticCharacter::CanPlayVO(const FString& VoiceLine) const
{
	if (bDeactivated)
		return false;
	
	if (bCannotSpeak)
		return false;
	
	if (IsGettingUp())
		return false;
	
	if (TimeSinceLastPlayDead < 2.0f || TimeSinceAtLastCoverLandmark < 2.0f || bIsExitingLandmark)
		return false;
	
	if (bCommitingSuicide && VoiceLine.Contains("Pain"))
		return true;
	
	if (Rep_HidingAnimState.bIsHiding && !VoiceLine.Contains("Death"))
		return false;
	
	return Super::CanPlayVO(VoiceLine);
}

void ACyberneticCharacter::PlayDead(const float Duration, const bool bPlayVO)
{
	if (IsDeadOrUnconscious())
		return;
	
	if (bStartedPlayingDeath || bPlayingDeathMontage)
		return;

	if (IsInRagdoll() || IsRagdollBlending())
		return;
	
	if (bIsPlayingDead)
		return;
	
	if (bPlayVO)
	{
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_DEATH, "", false);
		UAISense_Hearing::ReportNoiseEvent(this, GetActorLocation(), 2.0f, this, 0.0f, "DeathScream");
	}
	
	ScoringComponent->RevokeAllPenalties();

	PlayDeathAnimation();

	bIsPlayingDead = true;
	bHasBeenReported = false;
	
	TimePlayingDead = 0.0f;
	TimeSinceLastPlayDead = 0.0f;

	if (GetCyberneticsController())
	{
		GetCyberneticsController()->RemoveAllActivitiesExcept(UPlayDeadActivity::StaticClass());
		
		ACyberneticCharacter* ClosestFriendlyOrNeutral = UReadyOrNotFunctionLibrary::FindClosestActor<ACyberneticCharacter>(GetWorld(), GetActorLocation(), 1250.0f, [&](const ACyberneticCharacter* Character, const float Distance)
		{
			if (Character == this)
				return false;

			if (Character->IsDeadOrUnconscious() || Character->IsIncapacitated() || Character->IsPlayingDead())
				return false;
			
			return GetCyberneticsController()->IsCharacterFriendly(const_cast<ACyberneticCharacter*>(Character)) ||
					GetCyberneticsController()->IsCharacterNeutral(const_cast<ACyberneticCharacter*>(Character));
		});

		if (ClosestFriendlyOrNeutral)
		{
			ClosestFriendlyOrNeutral->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED, false);
		}
	}
}

void ACyberneticCharacter::StopPlayingDead()
{
	if (!bIsPlayingDead)
		return;

	TimePlayingDead = 0.0f;
	TimeSinceLastPlayDead = 0.0f;
	
	bIsPlayingDead = false;
	bHasBeenReported = false;

	GetAudioComp()->Stop();

	GetBackupAfterRagdoll();
}

void ACyberneticCharacter::Knockout(const float Duration, const bool bPlayVO)
{
	if (bIsKnockedOut)
		return;
	
	bIsKnockedOut = true;

	if (bPlayVO)
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_PAIN, "", false);

	EnableRagdoll(Duration);

	if (GetCyberneticsController())
	{
		GetCyberneticsController()->ClearActivityList();
		GetCyberneticsController()->bStopDecisionMaking = true;
		GetCyberneticsController()->ForceEndAllActions();
	}
}

void ACyberneticCharacter::StopKnockout()
{
	if (!bIsKnockedOut)
		return;

	bIsKnockedOut = false;

	GetBackupAfterRagdoll();
}

void ACyberneticCharacter::GetUp(const bool bFlip)
{
	if (IsDeadOrUnconscious() || IsIncapacitated())
		return;

	bRecoveringFromRagdoll = true;

	Multicast_SavePoseSnapshot("RagdollPoseEnd");
	Multicast_SavePoseSnapshot_Implementation("RagdollPoseEnd");

	FString Animation = "tp_getup_facing";
	const bool bKnockoutFacingUp = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), FVector::UpVector) >= 0.0f;
	if (bKnockoutFacingUp)
	{
		Animation += "_up";
	}
	else
	{
		Animation += "_down";
	}

	if (bFlip)
		Animation += "_180";

	Multicast_Stop3PMontage(nullptr, 0.0f);

	LastGetUpMontage = GetMontageFromTable(Animation);
	
	Server_Play3PMontage(LastGetUpMontage);
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::ResetRagdollStates, 1.5f);
}

void ACyberneticCharacter::OnRagdollDurationComplete()
{
	Super::OnRagdollDurationComplete();

	if (bIsKnockedOut)
	{
		bIsKnockedOut = false;
	}

	if (bIsPlayingDead)
	{
		bIsPlayingDead = false;
		bHasBeenReported = false;
	
		TimeSinceLastPlayDead = 0.0f;

		GetAudioComp()->Stop();
	}
}

void ACyberneticCharacter::ResetRagdollStates()
{
	bRecoveringFromRagdoll = false;
}

void ACyberneticCharacter::ResetPlayDeadState()
{
	bIsPlayingDead = false;
}

void ACyberneticCharacter::ReactToCarryThrow()
{
	Knockout(0.65f);
}

void ACyberneticCharacter::ResetThrownByCharacter()
{
	Super::ResetThrownByCharacter();

	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetCapsuleComponent()->SetSimulatePhysics(false);
}

void ACyberneticCharacter::ResetSurrenderStates()
{
	SurrenderedTime = 0.0f;
	bSurrendered = false;
	bSurrenderComplete = false;
}

void ACyberneticCharacter::ResetFakeSurrenderStates()
{
	bIsFakeSurrender = false;
}

bool ACyberneticCharacter::GetBackupAfterRagdoll()
{
	if (IsDeadOrUnconscious() || IsIncapacitated())
		return false;
	
	if (!IsInRagdoll())
		return false;
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const FVector PelvisLocation = GetMesh()->GetSocketLocation("pelvis");

	const bool bIsGrounded = GetWorld()->LineTraceTestByObjectType(PelvisLocation, PelvisLocation - FVector::UpVector * 150.0f, ObjectQueryParams, GetCollisionQueryParameters());
	
	// Keep trying to test if grounded before disabling ragdoll
	if (!bIsGrounded)
	{
		EnableRagdoll(0.25f);
		return false;
	}

	DisableRagdoll();

	bRecoveringFromRagdoll = true;

	GetUp();

	return true;
}

void ACyberneticCharacter::OnGetupAfterRagdollComplete()
{
	bRecoveringFromRagdoll = false;
	ThrownByCharacter = nullptr;

	if (GetCyberneticsController())
	{
		GetCyberneticsController()->ClearActivityList();
		GetCyberneticsController()->bStopDecisionMaking = false;
	}
}

bool ACyberneticCharacter::IsGettingUp() const
{
	return Is3PMontagePlaying(LastGetUpMontage) ||
			IsTableMontagePlaying("tp_getup_facing_down") ||
			IsTableMontagePlaying("tp_getup_facing_up") ||
			IsTableMontagePlaying("tp_getup_facing_left") ||
			IsTableMontagePlaying("tp_getup_facing_right");
}

bool ACyberneticCharacter::IsPlayingHitReaction() const // todo
{
	return IsTableMontagePlaying("tp_swat_hit_pain_head") ||
			IsTableMontagePlaying("tp_swt_hit_pain_arms") ||
			IsTableMontagePlaying("tp_swt_hit_pain_legs") ||
			IsTableMontagePlaying("tp_swt_hit_pain_belly");
}

bool ACyberneticCharacter::IsPlayingFullBodyHitReaction() const
{
	return IsTableMontagePlaying("tp_hits") ||
			IsTableMontagePlaying("tp_hits_armoured") ||
			IsTableMontagePlaying("tp_hits_l_arm") ||
			IsTableMontagePlaying("tp_hits_r_arm") ||
			IsTableMontagePlaying("tp_hits_l_leg") ||
			IsTableMontagePlaying("tp_hits_r_leg");
}

void ACyberneticCharacter::PickupEvidence(AActor* InEvidence)
{
	if (!InEvidence)
		return;
	
	PendingEvidence = InEvidence;

	if (IsOnSWATTeam())
	{
		PlayMontageFromTableWithFocalPoint("tp_swat_collect_evidence", InEvidence->GetActorLocation());
	}
}

void ACyberneticCharacter::EndStun(EStunType StunType)
{
	Super::EndStun(StunType);
	
	// if (StunType == EStunType::ST_Gassed)
	// {
	// 	PlayMontageFromTable("tp_gas");	
	// 	PlayStunnedVoiceLine(EStunType::ST_Gassed);
	// }
}

bool ACyberneticCharacter::IsActive() const
{
	if (bDeactivated)
	{
		//ULog::Info(GetName() + " | is inactive. deactivated");
		return false;
	}

	if (!GetCyberneticsController())
	{
		return false;
	}
	
	if (!GetCyberneticsController()->GetTargetingComp())
	{
		return false;
	}
	
	if (IsDeadOrUnconscious())
	{
		//ULog::Info(GetName() + " | is inactive. Dead");
		return false;
	}

	if (IsIncapacitated())
	{
		//ULog::Info(GetName() + " | is inactive. Incapacitated");
		return false;
	}

	if (IsArrested())
	{
		//ULog::Info(GetName() + " | is inactive. Arrested");
		return false;
	}
	
	if (IsSurrenderedFor(1.5f) || bSurrenderComplete)
	{
		//ULog::Info(GetName() + " | is inactive. Surrendered");
		return false;
	}

	if (bIsBeingArrested)
	{
		//ULog::Info(GetName() + " | is inactive. Is being arrested");
		return false;
	}

	if (IsCarried())
	{
		//ULog::Info(GetName() + " | is inactive. Is carried");
		return false;
	}
	
	if (IsBeingTakenHostage())
	{
		//ULog::Info(GetName() + " | is inactive. Taken hostage by " + GetNameSafe(TakenHostageBy));
		return false;
	}
	
	if (IsInRagdoll() && !IsPlayingDead())
	{
		//ULog::Info(GetName() + " | is inactive. in ragdoll");
		return false;
	}

	if (IsGettingUp() || bRecoveringFromRagdoll)
	{
		//ULog::Info(GetName() + " | is inactive. getting up from ragdoll");
		return false;
	}

	if (!IsPlayingDead())
	{
		if (bStartedPlayingDeath || bPlayingDeathMontage)
		{
			//ULog::Info(GetName() + " | is inactive. playing death montage");
			return false;
		}
	}

	return true;
}

bool ACyberneticCharacter::IsActiveForMovement() const
{
	if (AvoidanceLocation != FVector::ZeroVector)
		return false;
	
	if (ReasonsToStandStill.Num() > 0)
		return false;
	
	return Super::IsActiveForMovement();
}

bool ACyberneticCharacter::IsActiveForThinking() const
{
	if (bDeactivated)
		return false;

	if (!GetCyberneticsController())
		return false;

	if (GetCyberneticsController()->bStopUtilityTick)
		return false;
	
	if (IsDeadOrUnconscious())
		return false;
	
	if (IsIncapacitated())
		return false;
	
	if (IsArrested() || bIsBeingArrested)
		return false;
	
	if (IsCarried())
		return false;
	
	if (IsBeingTakenHostage())
		return false;
		
	if (IsGettingUp())
		return false;
	
	return bIsRelevant;
}

bool ACyberneticCharacter::IsAnimationBlocking() const
{
	if (IsStartling())
		return true;

	if (IsHesitating())
		return true;

	if (IsDrawingWeapon())
		return true;

	if (IsGettingUp())
		return true;

	if (IsPlayingFullBodyHitReaction())
		return true;

	if (IsPickingUpWeapon())
		return true;
	
	if (IsPlayingStunAnimation())
		return true;

	if (bRecoveringFromRagdoll)
		return true;

	if (IsExitingSurrender())
		return true;

	return false;
}

bool ACyberneticCharacter::IsAffectedByDamageType(UDamageType* DamageType) const
{
	if (Cast<UCSGasDamageType>(DamageType))
	{
		const bool bImmune = AssignedAIData ? AssignedAIData->bImmuneToGas : false;
		if (bImmune)
		{
			return false;
		}
	}
	
	return Super::IsAffectedByDamageType(DamageType);
}

bool ACyberneticCharacter::IsDrawingWeapon() const
{
	if (const ABaseItem* EquippedItem = GetEquippedItem())
	{
		if (EquippedItem->IsPlayingDraw())
			return true;
	}
	
	if (IsTableMontagePlaying("tp_draw_weapon"))
		return true;

	return false;
}

bool ACyberneticCharacter::IsRecoiling() const
{
	ABaseItem* EquippedItem = GetEquippedItem();
	
	if (const ABallisticsShield* BallisticsShield = Cast<ABallisticsShield>(EquippedItem))
	{
		EquippedItem = BallisticsShield->PistolEquippedWithShield;
	}

	if (EquippedItem)
	{
		const UAnimMontage* RecoilMontage = nullptr;
		if (EquippedItem->AnimationData)
		{
			if (EquippedItem->AnimationData->Recoil_Level_01.Body_TP)
			{
				RecoilMontage = EquippedItem->AnimationData->Recoil_Level_01.Body_TP;
			}
				
			if (EquippedItem->AnimationData->FireSingle.Num() > 0)
			{
				RecoilMontage = EquippedItem->AnimationData->FireSingle[0].Body_TP;
			}
		}

		return RecoilMontage != nullptr && GetCurrentMontage() == RecoilMontage;
	}

	return false;
}

bool ACyberneticCharacter::HasLineOfSightToCharacter(AReadyOrNotCharacter* InCharacter) const
{
	return !GetWorld()->LineTraceTestByChannel(InCharacter->GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT), GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT), ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(const_cast<ACyberneticCharacter*>(this), InCharacter));
}

bool ACyberneticCharacter::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{
	
	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this) || bDeactivated)
		return false;
	
	return Super::CanBeSeenFrom(ObserverLocation, OutSeenLocation, NumberOfLoSChecksPerformed, OutSightStrength, IgnoreActor, bWasVisible, UserData);
}

void ACyberneticCharacter::DestroyPlayerMarkerComponent()
{
	DESTROY_COMPONENT(PlayerMarkerComponent);
}

bool ACyberneticCharacter::IsBreaching() const
{
	if (const ACyberneticController* CyberneticController = GetCyberneticsController())
	{
		if (const UTeamBreachAndClearActivity* BreachAndClearActivity = CyberneticController->GetCurrentActivity<UTeamBreachAndClearActivity>())
		{
			return !BreachAndClearActivity->HasBreached();
		}
	}

	return false;
}

/* Alex 18/11/22 made sure all paths use the same interpolation method to make interp speed globally tweakable */
void ACyberneticCharacter::UpdateAimOffset()
{
	PreviousAimOffset = AimOffset;

	const bool bIsActiveForMovement = !IsDeadOrUnconscious() && !IsIncapacitated() && !IsInRagdoll();
	if (!bIsActiveForMovement)
	{
		AimOffset = FVector2D::ZeroVector;
		return;
	}

	bool bPlayingPairedInteraction = false;
	bool bDisallowAimOffsetDuringPairedInteraction = false;
	
	if (const APairedInteractionDriver* Driver = UInteractionsData::IsPairedInteractionPlayingOn(this))
	{
		bPlayingPairedInteraction = true;
		
		if (!Driver->GetInteractionData()->bAllowAimOffsetDuringInteraction)
		{
			bDisallowAimOffsetDuringPairedInteraction = true;
		}
	}
	
	//const bool bIsHoldingItemForAimReset = Cast<AMultitool>(GetEquippedItem()) || Cast<AZipcuffs>(GetEquippedItem());
	//const bool bIsPlayingAnimForAimReset = IsTableMontagePlaying("tp_swt_lockpick") || IsTableMontagePlaying("tp_melee");

	const bool bShouldZeroAimOffset = bIsBeingArrested ||
									IsArrested() ||
									(IsSurrendered() && !IsAny3PMontageActive()) ||
									//bIsHoldingItemForAimReset ||
									//bIsPlayingAnimForAimReset ||
									(GetActorLocation() - Rep_FocalPoint).Size() < 100.0f;
									//(IsBreaching() && !Cast<AReadyOrNotCharacter>(Rep_FocalActor));

	FVector2D FinalResult = FVector2D::ZeroVector;
	
	if (!bShouldZeroAimOffset)
	{
		if (IsBeingTakenHostage())
		{
			FinalResult = TakenHostageBy->AimOffset;
		}
		else if (!bDisallowAimOffsetDuringPairedInteraction && (IsStrafing() || IsAny3PMontageActive() || bPlayingPairedInteraction))
		{
			//const FVector BaseLocation = GetActorLocation() + FVector::UpVector * 70.0f;
			const FVector BaseLocation = GetMesh()->GetSocketLocation("weapon_right");
			const FVector LookAtDirection = (Rep_FocalPoint - BaseLocation).GetSafeNormal();
			const FRotator AimOffset3D = (LookAtDirection.Rotation() - GetActorRotation()).GetNormalized();

			const float TargetPitch = FMath::ClampAngle(AimOffset3D.Pitch * 1.5f, -90.0f, 90.0f);
			const float TargetYaw = FMath::ClampAngle(-AimOffset3D.Yaw, -90.0f, 90.0f);
			
			FinalResult = FVector2D(TargetYaw, TargetPitch);
		}
	}
	
	AimOffset = FinalResult;
}

bool ACyberneticCharacter::IsStrafing() const
{
	return bIsStrafing;
}

bool ACyberneticCharacter::IsHesitating() const
{
	return MoveStyle->Rep_MoveStyleName == MovementStyleData.HesitationMoveStyle ||
			MoveStyle->Rep_MoveStyleName == MovementStyleData.HesitationRifleMoveStyle ||
			IsTableMontagePlaying("tp_hesitate");
}

ABaseArmour* ACyberneticCharacter::GetArmour() const
{
	if (const UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		return InventoryComponent->GetArmour();
	}

	return nullptr;
}

void ACyberneticCharacter::SetIsStrafing(const bool bNewStrafing, const bool bPlayBlendAnimation)
{
	if (!GetCyberneticsController() || !GetEquippedWeapon())
	{
		bServerIsStrafing = false;
		return;
	}
	
	if (IsSuspect())
	{
		if (IsTakingCover() && !IsFiringFromCover())
		{
			bServerIsStrafing = false;
			return;
		}
		
		//const float StressUntilWeaponRaise = AI_CONFIG_GET_FLOAT("StressUntilWeaponRaise", 0.5f);
		//if (GetCyberneticsController()->GetAwarenessState() != EAIAwarenessState::Alerted && Stress < StressUntilWeaponRaise)
		//{
		//	bServerIsStrafing = false;
		//	return;
		//}
	}

	if (bServerIsStrafing != bNewStrafing &&
		bPlayBlendAnimation &&
		GetEquippedWeapon() &&
		!IsOnSWATTeam() &&
		!IsStunned() &&
		!IsPlayingStunAnimation() && // ignore stun animations in this
		!bIsFakeSurrender &&
		!bSurrendered)
	{
		bool bIsAnimationBlocking = IsAnimationBlocking();
		if (bIsAnimationBlocking && GetCurrentMontage())
		{
			const float BlendOutTime = GetCurrentMontage()->GetDefaultBlendOutTime() + 0.1f;
			float TimeRemaining = 0.0f;
			if (IsMontagePlayingWithTimeRemaining(GetCurrentMontage(), TimeRemaining))
			{
				if (TimeRemaining <= BlendOutTime)
				{
					bIsAnimationBlocking = false;
				}
			}
		}
		
		if (!bIsAnimationBlocking)
		{
			// Raise weapon
			if (!bServerIsStrafing)
			{
				if (!bWasWeaponRaised)
				{
					StopTPMontageFromTable(GetLastTableMontagePlayed());
					PlayedTableMontageMap3P.Empty();
					
					if (PlayMontageFromTable("tp_raise"))
					{
						bWasWeaponRaised = true;
					}
				}
			}
			// Lower weapon
			else
			{
				if (bWasWeaponRaised)
				{
					StopTPMontageFromTable(GetLastTableMontagePlayed());
					PlayedTableMontageMap3P.Empty();
					
					if (PlayMontageFromTable("tp_lower"))
					{
						bWasWeaponRaised = false;
					}
				}
			}
		}
	}

	if (ABallisticsShield* Shield = GetEquippedItem<ABallisticsShield>())
	{
		if (bNewStrafing)
		{
			if (!bWasWeaponRaised)
			{
				Shield->OnItemSecondaryUsed();
				bWasWeaponRaised = true;
			}
		}
		else
		{
			if (bWasWeaponRaised)
			{
				Shield->OnItemEndSecondaryUse();
				bWasWeaponRaised = false;
			}
		}
	}

	bServerIsStrafing = bNewStrafing;

	//MoveStyle->SetIsStrafing(bNewStrafing);
}

void ACyberneticCharacter::OnRep_SimulatingAttachedStaticMeshes()
{
	for (UStaticMeshComponent* StaticMeshComponent : SimulatingAttachedStaticMeshes)
	{
		if (StaticMeshComponent)
		{
			StaticMeshComponent->SetSimulatePhysics(true);
		}
	}
}

void ACyberneticCharacter::OnRep_AttachedMeshData()
{
	for (int32 i = 0; i < AttachedMeshData.Num(); i++)
	{
		if (AttachedMeshData[i].StaticMeshComponent)
		{
			AttachedMeshData[i].StaticMeshComponent->SetStaticMesh(AttachedMeshData[i].StaticMesh);
			AttachedMeshData[i].StaticMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachedMeshData[i].Socket);
			AttachedMeshData[i].StaticMeshComponent->SetIsReplicated(true);
			AttachedMeshData[i].StaticMeshComponent->SetCanEverAffectNavigation(false);
			AttachedMeshData[i].StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}
 
void ACyberneticCharacter::OnRep_AttachedSkeletalMeshData()
{
	for (int32 i = 0; i < AttachedSkeletalMeshData.Num(); i++)
	{
		if (AttachedSkeletalMeshData[i].SkeletalMeshComponent)
		{
			AttachedSkeletalMeshData[i].SkeletalMeshComponent->SetSkeletalMesh(AttachedSkeletalMeshData[i].SkeletalMesh);
			AttachedSkeletalMeshData[i].SkeletalMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachedSkeletalMeshData[i].Socket);
			AttachedSkeletalMeshData[i].SkeletalMeshComponent->SetIsReplicated(true);
			AttachedSkeletalMeshData[i].SkeletalMeshComponent->SetCanEverAffectNavigation(false);
			AttachedSkeletalMeshData[i].SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (AttachedSkeletalMeshData[i].bUseMasterPose)
			{
				AttachedSkeletalMeshData[i].SkeletalMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				AttachedSkeletalMeshData[i].SkeletalMeshComponent->SetMasterPoseComponent(GetMesh());
			}
		}
	}
}

void ACyberneticCharacter::HandleLimbDismembered(FName Bone)
{
	if (!GetMesh())
		return;

	// Need to hide/detach attached meshes for this character
	for (FAttachedMeshData& AttachedMesh : AttachedMeshData)
	{
		if (!AttachedMesh.StaticMeshComponent)
			continue;
		
		FName AttachSocket = AttachedMesh.StaticMeshComponent->GetAttachSocketName();
		if (AttachSocket == NAME_None)
			continue;

		FName SocketBone = GetMesh()->GetSocketBoneName(AttachSocket);
		if (SocketBone == Bone || GetMesh()->BoneIsChildOf(SocketBone, Bone))
		{
			// AttachedMesh.StaticMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			// AttachedMesh.StaticMeshComponent->SetSimulatePhysics(true);
			// AttachedMesh.StaticMeshComponent->AddImpulse(FVector::UpVector * 150.0f);
			// AttachedMesh.StaticMeshComponent->AddAngularImpulseInRadians(FMath::VRand() * 100.0f);
			AttachedMesh.StaticMeshComponent->SetStaticMesh(nullptr);
		}
	}

	// Do the same for skinned meshes
	for (FAttachedSkeletalMeshData& AttachedMesh : AttachedSkeletalMeshData)
	{
		if (!AttachedMesh.SkeletalMeshComponent)
			continue;

		FName AttachSocket = AttachedMesh.SkeletalMeshComponent->GetAttachSocketName();
		if (AttachSocket == NAME_None)
			continue;

		FName SocketBone = GetMesh()->GetSocketBoneName(AttachSocket);
		if (SocketBone == Bone || GetMesh()->BoneIsChildOf(SocketBone, Bone))
		{
			AttachedMesh.SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		}
	}
}

void ACyberneticCharacter::GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const
{
	// this is intentially using the capsule
	// when using the actual eyes from the mesh (headbone) this did not work and caused erronous things
	// suchs as when you would get in a Z offset with the AI their viewpoint would spin round
	// do not change this it is at perfect eye level

	/*
	if (GetFaceMesh()->SkeletalMesh)
	{
		if (GetFaceMesh()->GetBoneIndex("r_eye") != INDEX_NONE)
		{
			Rotation = GetFaceMesh()->GetBoneQuaternion("r_eye").Rotator() + FRotator(0.0f, -90.0f , 0.0f);
			Rotation.Pitch = 0.0f;
			Rotation.Roll = 0.0f;
		}
		else
		{
			Rotation = GetFaceMesh()->GetComponentRotation() + FRotator(0.0f, 90.0f, 0.0f);
		}
	}
	else
	{
		Rotation = GetCapsuleComponent()->GetComponentRotation() + FRotator(AimOffset.X, AimOffset.Y, 0.0f);
	}
	*/
	
	Location = GetMesh()->GetBoneLocation("head") + Rotation.Vector() * 5.0f;
	Rotation = GetCapsuleComponent()->GetComponentRotation();
	
	//DrawDebugLine(GetWorld(), Location, Location + Rotation.Vector() * 5000.0f, FColor::Blue, false, 0.01f, 0, 1);
}

void ACyberneticCharacter::FinishAISpawning(AAISpawn* Spawner, const FAIDataLookupTable* AIData)
{
	if (!AIData)
	{
		AssignedAIData = nullptr;
		return;
	}
	
	AssignedAIData = AIData;

	Archetype = AssignedAIData->Archetype;
	
	if (!Archetype)
	{
		if (IsSuspect())
		{
			Archetype = DefaultSuspectArchetype;
		}
		else
		{
			Archetype = DefaultCivilianArchetype;
		}
	}

	if (UAIArchetypeData* const* ArchetypeOverridePtr = AssignedAIData->GameModeArchetypeOverride.Find(UReadyOrNotFunctionLibrary::GetCOOPMode()))
	{
		Archetype = *ArchetypeOverridePtr;
	}
	
	if (Spawner)
	{
		SpawnLocation = Spawner->GetActorLocation();
		SpawnedFromSpawner = Spawner;
		
		for (FName tag : AssignedAIData->Tags)
		{
			Tags.AddUnique(tag);
		}
		
		for (FName tag : Spawner->SpawnData.SpawnWithTags)
		{
			//V_LOGM(LogReadyOrNot, "Spawning With Tag: %s", *tag.ToString());
			Tags.AddUnique(tag);
		}
		
		SpawnData = &Spawner->SpawnData;
		ActivityRouteCollection = Spawner->SpawnData.ActivityRouteCollection;
		
		SetActorRotation(Spawner->GetActorRotation());

		if (Spawner->ArchetypeOverride)
		{
			Archetype = Spawner->ArchetypeOverride;
		}

		bDisableTurnInPlace = true;
		ReasonsToStandStill.AddUnique("SpawnRotation");

		// Clear after one second
		FTimerDelegate Delegate;
		Delegate.BindWeakLambda(this, [this]
		{
			bDisableTurnInPlace = false;
			ReasonsToStandStill.Remove("SpawnRotation");
		});
		
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, Delegate, 1.0f);
	}

	if (Archetype)
	{
		if (Archetype->bOverrideAimSettings)
		{
			FocalPointInterpSpeed = Archetype->FocalPointInterpSpeed;
			FocalPointInterpCurve = Archetype->FocalPointInterpCurve;
			FocusTurnSpeed = Archetype->FocusTurnSpeed;
			TurnDegreesPerSecond = Archetype->TurnDegreesPerSecond;
			ActorRotationInterpStandingSpeed = Archetype->ActorRotationInterpStandingSpeed;
			ActorRotationInterpMovingSpeed = Archetype->ActorRotationInterpMovingSpeed;
			AimOffsetInterpSpeed = Archetype->AimOffsetInterpSpeed;
		}

		if (Archetype->bDisableVO)
		{
			RemoveVocalChords();
		}
	}

	if (AssignedAIData->RandomCharacterMesh.Num() > 0)
	{
		CharacterMeshData = AssignedAIData->RandomCharacterMesh[FMath::RandRange(0, AssignedAIData->RandomCharacterMesh.Num() - 1)];
		CharacterMeshData.Guid = FGuid::NewGuid();
		
		V_LOGM(LogReadyOrNot, "CharacterMesh: Setting Character Mesh with guid %s Head? %d Body? %d", *CharacterMeshData.Guid.ToString(), CharacterMeshData.Head != nullptr, CharacterMeshData.Body != nullptr);
		OnRep_CharacterMeshData();
		CurrentFaceROM = CharacterMeshData.FaceROM;
	}

	// Create faction manager if not exists
	if (const FAIFactionTable* FactionTable = AssignedAIData->Faction.GetRow<FAIFactionTable>("Finish AI Spawn"))
	{
		if (FactionTable->Name != "None")
		{
			FactionData = *FactionTable;
			
			if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
			{
				if (GameState->AIFactionManagers.Contains(FactionTable->Name))
				{
					if (AAIFactionManager** FactionManagerPtr = GameState->AIFactionManagers.Find(FactionTable->Name))
					{
						FactionManager = *FactionManagerPtr;
						if (FactionManager)
						{
							FactionManager->AddCharacter(this);
						}
					}
				}
				else
				{
					if (FactionTable->Manager)
					{
						if (AAIFactionManager* NewFactionManager = GetWorld()->SpawnActor<AAIFactionManager>(FactionTable->Manager))
						{
							NewFactionManager->SetOwner(GameState);
							NewFactionManager->SetReplicates(true);
							NewFactionManager->AddCharacter(this);

							FactionManager = NewFactionManager;
							
							GameState->AIFactionManagers.Add(FactionTable->Name, NewFactionManager);
						}
					}
				}
			}
		}
	}

	// assign the movedata block so we access it later
	CurMoveDataBlock = AssignedAIData->DefaultMoveData;
	MovementStyleData = AssignedAIData->MovementStyle;
	
	DefaultTeam = AssignedAIData->SpawningTeamType;

	if (AssignedAIData->bOverrideControllerClass)
		AIControllerClass = AssignedAIData->ControllerClass;
	
	SpawnDefaultController();
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ACyberneticCharacter::EquipLoadoutOnAI, false), 1.0f);
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &ACyberneticCharacter::EquipArmourOnAI), 2.0f);

	if (PerceptionStimuliComp)
	{
		PerceptionStimuliComp->UnregisterFromSense(UAISense_Damage::StaticClass());
		PerceptionStimuliComp->UnregisterFromSense(UAISense_Hearing::StaticClass());
		PerceptionStimuliComp->UnregisterFromSense(UAISense_Sight::StaticClass());
	}

	// Initialize max health
	switch (GetTeam())
	{
		case ETeamType::TT_SQUAD:		CharacterHealth->SetMaxResource(AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f)); break;
		case ETeamType::TT_SERT_RED:	CharacterHealth->SetMaxResource(AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f)); break;
		case ETeamType::TT_SERT_BLUE:	CharacterHealth->SetMaxResource(AI_CONFIG_GET_FLOAT("SwatHealth", 250.0f)); break;
		case ETeamType::TT_SUSPECT:		CharacterHealth->SetMaxResource(AI_CONFIG_GET_FLOAT("SuspectHealth", 160.0f)); break;
		case ETeamType::TT_CIVILIAN:	CharacterHealth->SetMaxResource(AI_CONFIG_GET_FLOAT("CivilianHealth", 100.0f)); break;
		default:						CharacterHealth->SetMaxResource(200.0f); break;
	}

	CharacterHealth->SetCurrentResourceToMax();
	
	for (int32 i = 0; i < CharacterMeshData.AttachedMeshData.Num(); i++)
	{
		FAttachedMeshData* MeshData = &CharacterMeshData.AttachedMeshData[i];
		MeshData->StaticMeshComponent = NewObject<UStaticMeshComponent>(this);
		MeshData->StaticMeshComponent->RegisterComponent();
		AttachedMeshData.Add(*MeshData);
		OnRep_AttachedMeshData();
	}

	for (int32 i = 0; i < CharacterMeshData.AttachedSkeletalMeshData.Num(); i++)
	{
		FAttachedSkeletalMeshData* MeshData = &CharacterMeshData.AttachedSkeletalMeshData[i];
		MeshData->SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
		MeshData->SkeletalMeshComponent->RegisterComponent();
		AttachedSkeletalMeshData.Add(*MeshData);
		OnRep_AttachedSkeletalMeshData();
	}
	
	if (GetTeam() == ETeamType::TT_CIVILIAN || GetTeam() == ETeamType::TT_SUSPECT)
	{
		DESTROY_COMPONENT(PlayerMarkerComponent)
	}
	
	if (ACoopGM* CoopGM = GetWorld()->GetAuthGameMode<ACoopGM>())
	{
		OnCharacterKilled.AddDynamic(CoopGM, &ACoopGM::AIKilled);
		OnCharacterIncapacitated.AddDynamic(CoopGM, &ACoopGM::AIIncapacitated);
		OnPlayerArrested.AddDynamic(CoopGM, &ACoopGM::AIArrested);
		OnSurrendered.AddDynamic(CoopGM, &ACoopGM::AISurrendered);
	}

	#if WITH_EDITOR
	FString Label = GetName();
	for (FName& Tag : Tags)
	{
		Label += "_" + Tag.ToString();
	}
	
	SetActorLabel(Label);
	#endif
	
	OnAIFinishSpawning.Broadcast();
}

void ACyberneticCharacter::StartStun(const EStunType StunType, AActor* StunCauser)
{
	ACyberneticController* CyberneticController = GetCyberneticsController();
	if (!IsValid(CyberneticController))
		return;
	
	if (StunType == EStunType::ST_None)
	{
		return;
	}

	float Duration = AI_CONFIG_GET_FLOAT("AIStunDuration");
	
	if (StunType == EStunType::ST_Beanbag)
	{
		Duration = AI_CONFIG_GET_FLOAT("BeanbagStunDuration");
	}
	
	bHasEverBeenStunned = true;
	
	if (bIsBeingArrested)
		return;

	if (IsPlayingDead())
		return;
	
	if (TimeSinceLastPlayDead < 2.0f)
		return;

	if (StunType == EStunType::ST_Gassed)
	{
		Duration = 4.0f;
		if (!IsTableMontagePlaying("tp_surrender"))
			GetMesh()->GetAnimInstance()->StopAllMontages(0.5);

		// If AI has a moveto activity (such as for door avoidance) we need to clear it to allow flee combat move to run
		if (UMoveToActivity* MoveToActivity = CyberneticController->GetCurrentActivity<UMoveToActivity>())
		{
			CyberneticController->FinishActivity(MoveToActivity, false, true);
		}
	}

	float RiotControlTrait = URosterManager::GetSquadTraitValue("RiotControl", GetWorld());
	Duration += FMath::Max(0.0f, RiotControlTrait);

	StopTPMontageFromTable("tp_fake_surrender");
	StopTPMontageFromTable("tp_surrender_exit_fake");
	StopTPMontageFromTable("tp_surrender_exit_fake_knife");

	CurrentStunDuration = Duration;
	
	PlayStunnedVoiceLine(StunType);

	TSoftObjectPtr<UAnimMontage> AnimMontage = nullptr;
	
	//ULog::Info("Lower on Stun");
	
	if (!IsStunned())
		UMoraleComponent::LowerMoraleOnCharacter(CyberneticController->GetCharacter(), 0.05f, "Stun");

	CyberneticController->BulletsFiredTowardsAccuracyPenalty = 0;
	
	//if (IsArrested() || SurrenderedTime > 3.0f || StunType == EStunType::ST_Tased)
	{
		switch (StunType)
		{
			case EStunType::ST_Flash:			PlayMontageFromTable("tp_flashbang"); break;
			case EStunType::ST_Beanbag:			PlayMontageFromTable("tp_stinger");	break;
			case EStunType::ST_Stung:			PlayMontageFromTable(FMath::RandBool() ? "tp_stinger" : "tp_concussion"); break;
			case EStunType::ST_Gassed:			/* PlayMontageFromTable("tp_gas"); */ break;
			case EStunType::ST_Tased:			PlayMontageFromTable("tp_taser"); break;
			case EStunType::ST_Pepperball:		PlayMontageFromTable("tp_stinger"); break;
			case EStunType::ST_Rubberball:		PlayMontageFromTable("tp_stinger"); break;
			case EStunType::ST_Pepperspray:		PlayMontageFromTable("tp_stinger"); break;
			case EStunType::ST_None:			break;
			default: break;
		}
	}
	
	GetWorld()->GetTimerManager().SetTimer(TH_EndStunTimer, FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::EndStun, StunType), Duration, false);
	StunMap.Add(TH_EndStunTimer, StunType);
	
	Stress += FMath::Clamp(AI_CONFIG_GET_FLOAT("StunStress", 0.1f), 0.0f, 1.0f);

	OnStunnedEvent.Broadcast(this, Duration, StunType, StunCauser);
}

void ACyberneticCharacter::PlayStunnedVoiceLine(const EStunType StunType, const bool bIsImmune)
{
	FString VoiceLine = "";

	if (IsStunnedWith(StunType) && StunType != EStunType::ST_Gassed)
		return;
	
	switch (StunType)
	{
		case EStunType::ST_Flash:			VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_FLASHED; break;
		case EStunType::ST_Stung:			VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_STUNNED; break;
		case EStunType::ST_Gassed:			VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_GAS_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_GASSED; break;
		case EStunType::ST_Tased:			VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_TASER_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_TASERED; break;
		case EStunType::ST_Beanbag:			VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_STUNNED; break;
		case EStunType::ST_Pepperball:		VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_STUNNED; break;
		case EStunType::ST_Rubberball:		VoiceLine = bIsImmune ? VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE : VO_SUSPECTS_AND_CIVILIAN::BARK_STUNNED; break;
		default: break;
	}

	PlayRawVO(VoiceLine);
}

bool ACyberneticCharacter::IsPlayingStunAnimation() const
{
	return IsTableMontagePlaying("tp_gas") ||
			IsTableMontagePlaying("tp_stinger") ||
			IsTableMontagePlaying("tp_flashbang") ||
			IsTableMontagePlaying("tp_concussion") ||
			IsTableMontagePlaying("tp_taser");
}

void ACyberneticCharacter::StartPepperSprayed(APepperspray* PeppersprayUsed)
{
	if (!PeppersprayUsed)
		return;
	
	const float AccuracyPenalty = AI_CONFIG_GET_FLOAT("PepperSprayAccuracyPenalty");
	PepperSprayAccuracyPenalty = AccuracyPenalty;

	PlayMontageFromTable("tp_swt_pain_flash");

	Super::StartPepperSprayed(PeppersprayUsed);

	PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_GASSED);
}

void ACyberneticCharacter::ArrestComplete(AReadyOrNotCharacter* PlayerMakingArrest, class AZipcuffs* Zipcuffs)
{
	Super::ArrestComplete(PlayerMakingArrest, Zipcuffs);

	StopKnockout();
	StopPlayingDead();

	PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_ARRESTED);
	
	GetInventoryComponent()->ThrowEquippedItem();
	
	if (ThrownItem)
	{
		ThrownItem->MarkAsEvidence(true);
	}

	// Get back up after being arrested as a ragdoll
	if (bArrestedAsRagdoll)
	{
		if (IsDeadOrUnconscious() || IsIncapacitated())
		{
			EnableRagdoll();
		}
		else
		{
			GetUp();
		}
	}

	const bool bCanGiveScore = !bArrestedAsRagdoll || (bArrestedAsRagdoll && !IsDeadOrUnconscious() && !IsIncapacitated());
	
	if (bCanGiveScore)
	{
		ScoringComponent->GiveScore(FText::FromStringTable("ScoringTable", "Arrest"));
	}
	
	UpdateDefaultMoveStyle(); // immediately update the move style to fix pose snapping
}

void ACyberneticCharacter::Fire()
{
	ABaseItem* EquippedItem = GetEquippedItem();
	ABaseMagazineWeapon* EquippedWeapon;
	
	if (const ABallisticsShield* BallisticsShield = Cast<ABallisticsShield>(EquippedItem))
	{
		EquippedWeapon = BallisticsShield->PistolEquippedWithShield;
	}
	else
	{
		EquippedWeapon = Cast<ABaseMagazineWeapon>(EquippedItem);
	}
	
	Fire(EquippedWeapon);
}

bool ACyberneticCharacter::IsArrestCapable(APlayerCharacter* PlayerCharacter) const
{
	if (HasDamagedSWAT())
		return false;

	if (CanArrest())
		return false;
	
	if (PlayerCharacter)
	{
		if (PlayerCharacter->HasBeenDamagedByCharacter(this))
			return false;
	}

	return true;
}

bool ACyberneticCharacter::IsActiveForCombat() const
{
	if (!GetCyberneticsController())
		return false;
	
	if (IsDeadOrUnconscious())
		return false;
	
	if (IsPlayingDead())
		return false;

	if (IsInRagdoll())
		return false;

	if (IsArrested())
		return false;
	
	if (IsBeingTakenHostage())
		return false;

	if (IsHesitating())
		return false;

	if (IsExitingSurrender())
		return false;
	
	if (bStartedPlayingDeath || bPlayingDeathMontage)
		return false;

	return true;
}

void ACyberneticCharacter::OnMelee_Implementation(AReadyOrNotCharacter* Attacker, FHitResult Hit)
{
	if (!Attacker)
		return;

	if (const AMeleeWeapon* MeleeWeapon = Cast<AMeleeWeapon>(Attacker->GetEquippedItem()))
	{
		TakeDamage(MeleeWeapon->MeleeDamage, FDamageEvent(UDamageType::StaticClass()), Attacker->GetController(), Attacker);
	}

	if (Attacker->IsDeadOrUnconscious() || Attacker->IsArrestedOrSurrendered())
		return;

	if (!IsSurrendered() && Attacker->GetEquippedItem<ABallisticsShield>())
	{
		PlayMontageFromTable("tp_stumble");
		PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_BASHED, true, 1.0f);
	}
	else
	{
		// Wait for the AI to get down first
		if (!IsSurrendered() || (IsSurrendered() && IsSurrenderedFor(2.0f)))
		{
			PlayMontageFromTable("tp_bashed");
			PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_BASHED, true, 1.0f);

			if (IsOnSWATTeam())
			{
				FTimerDelegate d;
				d.BindWeakLambda(this, [=]()
				{
					PlayRawVO(VO_SWAT_GENERAL::CALL_FRIENDLY_FIRE);
				});

				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, d, 0.25f);
			}
		}
	}

	if (Attacker->IsOnSWATTeam())
	{
		if (IsArrested() || IsSurrenderedFor(1.5f) || IsInRagdoll() && !IsDeadNotUnconscious())
		{
			ScoringComponent->GivePenalty(AScoringManager::PENALTY_UNAUTHORIZED_FORCE, true);
		}
	}

	UMoraleComponent::LowerMoraleOnCharacter(this, AI_CONFIG_GET_FLOAT("BashMorale.Damage"), "Bash");

	OnMeleeHitTaken.Broadcast(Attacker);
}

void ACyberneticCharacter::Server_DoMelee_Implementation()
{
	if (RecentMeleeVictim)
	{
		TimeSinceLastAggressiveForce = 0.0f;
		
		ScoringComponent->RevokeAllPenalties();
		
		// no knifing behind
		if (GetDotProductTo(RecentMeleeVictim) > -0.1f)
		{
			Execute_OnMelee(RecentMeleeVictim, this, FHitResult());
		}
		
		RecentMeleeVictim = nullptr;
	}
}

void ACyberneticCharacter::ForceFireGun(const float Chance)
{
	if (Chance <= 0.0f)
		return;
	
	if (IsTraversingHole())
		return;
	
	if (ForceFireGunDelay > 0.0f)
		return;
	
	if (FMath::FRand() < FMath::Clamp(Chance, 0.0f, 1.0f))
	{
		if (ABaseMagazineWeapon* Weapon = GetEquippedWeapon())
		{
			if (!Weapon->HasAmmo())
				return;
			
			ScoringComponent->RevokeAllPenalties();
			
			ForceFireGunDelay = 1.0f;
			Weapon->bAIFireAtBulletSpawn = true;
			Weapon->OnFireAtBulletSpawn();
			Weapon->bAIFireAtBulletSpawn = false;

			ULog::Info(GetName() + " | Force Fired Gun: " + Weapon->GetName());
		}
	}
}

bool ACyberneticCharacter::Fire(ABaseMagazineWeapon* Weapon)
{
	if (!Weapon)
		return false;

	if (!GetEquippedItem() || (GetEquippedItem() != Weapon))
		return false;

	if (!IsActive())
		return false;

	if (IsStunned() || HasRecentlyTakenStunDamage())
		return false;

	// ensure that we don't shoot side-ways at a target
	const FVector& FireDirection = Weapon->GetBulletSpawn()->GetForwardVector();
	const FVector& DirectionToFocalPoint = (Rep_FocalPoint - Weapon->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
	const float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);

	if (TargetRotationDelta < FireAngleThreshold)
		return false;
	
	Weapon->OnFireAtBulletSpawn();
	return true;
}

void ACyberneticCharacter::EquipLoadoutOnAI(const bool bForce)
{
	if (!bFinishedEquippingLoadout || bForce)
	{
		float NoWeaponChance = AI_CONFIG_GET_FLOAT("SuspectChanceToSpawnWithNoWeapon");
		if (FMath::RandRange(0.0f, 1.0f) < NoWeaponChance)
		{
			GetInventoryComponent()->DestroyInventoryItem(GetEquippedItem());
			return;
		}
		
		if (DefaultTeam == ETeamType::TT_SUSPECT)
		{
			if (GetSpawnData()->bForceNoWeapon)
				return;

			if (GetSpawnData()->ForceWeaponOverride)
			{
				FSavedLoadout Loadout = FSavedLoadout();
				Loadout.Primary = SpawnData->ForceWeaponOverride;
				Loadout.PrimaryAmmoSlotsCount = 4;

				FLoadoutEquipOptions Options = FLoadoutEquipOptions();
				Options.bSanitizeLoadout = false;
				
				UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, this, Options);
				
				if (GetEquippedItem())
				{
					GetEquippedItem()->Tags.Append(Tags);
				}
				
				bFinishedEquippingLoadout = true;

				return;
			}
			
			FSavedLoadout Loadout = UAIData::GetLoadout(AssignedAIData);
			if (Loadout.IsValid())
			{
				TSubclassOf<ABaseWeapon> WeaponToSpawn;
				int32 SlotCount;
				if (Loadout.Primary)
				{
					WeaponToSpawn = Loadout.Primary;
					SlotCount = Loadout.PrimaryAmmoSlotsCount;
				} 
				else
				{
					WeaponToSpawn = Loadout.Secondary;
					SlotCount = Loadout.SecondaryAmmoSlotsCount;
				}

				if (SlotCount <= 0)
					SlotCount = 4;
				
				if (WeaponToSpawn)
				{
					ABaseWeapon* Weapon = GetWorld()->SpawnActor<ABaseWeapon>(WeaponToSpawn);
					if (Weapon)
					{
						GetInventoryComponent()->AddInventoryItem(Weapon);
						GetInventoryComponent()->PutItemInHands(Weapon, true, true);

						TArray<FName> PossibleAmmo = AssignedAIData->AIAmmoTypeSelection;
						TArray<FName> CompatibleAmmo = Weapon->GetAmmunitionTypes();
						for (FName AmmoType : AssignedAIData->AIAmmoTypeSelection)
						{
							if (!CompatibleAmmo.Contains(AmmoType))
							{
								PossibleAmmo.Remove(AmmoType);
							}
						}

						if (PossibleAmmo.Num() > 0)
						{
							if (ABaseMagazineWeapon* MagazineWeapon = Cast<ABaseMagazineWeapon>(Weapon))
							{
								FName AmmoType = PossibleAmmo[FMath::RandRange(0, PossibleAmmo.Num() - 1)];
								int32 MagazineCount = SlotCount;
								
								TArray<FName> AmmoTypes;
								AmmoTypes.Init(AmmoType, MagazineCount);
								
								MagazineWeapon->SetMagazineCount(MagazineCount, AmmoTypes);
							}
						}
						else
						{
							if (ABaseMagazineWeapon* MagazineWeapon = Cast<ABaseMagazineWeapon>(Weapon))
							{
								int32 MagazineCount = SlotCount;
								
								TArray<FName> AmmoTypes;
								AmmoTypes.Init(NAME_None, 1);
								
								MagazineWeapon->SetMagazineCount(MagazineCount, AmmoTypes);
							}
						}	
					}
					
					// Add attachments to the primary weapon
					if (ABaseWeapon* bw = Cast<ABaseWeapon>(InventoryComp->GetInventoryItemOfType(EItemCategory::IC_Primary)))
					{
						bw->AddAttachment(Loadout.PrimaryScope, true);
						bw->AddAttachment(Loadout.PrimaryIlluminator, true);
						bw->AddAttachment(Loadout.PrimaryGrip, true);
						bw->AddAttachment(Loadout.PrimaryMuzzle, true);
						bw->AddAttachment(Loadout.PrimaryStock, true);
						bw->AddAttachment(Loadout.PrimaryUnderbarrel, true);
						bw->AddAttachment(Loadout.PrimaryOverbarrel, true);
						bw->AddAttachment(Loadout.PrimaryAmmunition, true);
					}
				}
			}
		
			if (GetEquippedItem())
			{
				GetEquippedItem()->Tags.Append(Tags);
			}
		}

		bFinishedEquippingLoadout = true;
	}
}

void ACyberneticCharacter::EquipArmourOnAI()
{
	if (bFinishedEquippingArmour)
		return;

	#if !UE_BUILD_SHIPPING
	ensure(GetInventoryComponent()->GetSpawnedGear().Armor == nullptr);
	ensure(GetInventoryComponent()->GetSpawnedGear().Helmet == nullptr);
	#endif

	ASuspectArmour* SuspectArmour = nullptr;
	ASuspectArmour* SuspectHelmet = nullptr;

	// Spawner body armour override
	FName ArmourName = GetSpawnData()->ForceBodyArmourOverride;

	// Data table body armour override
	if (ArmourName.IsNone() && AssignedAIData->AIBodyArmourOverride)
	{
		SuspectArmour = GetWorld()->SpawnActor<ASuspectArmour>(AssignedAIData->AIBodyArmourOverride);
	}

	// Select from AI body armour data table
	if (ArmourName.IsNone() && AssignedAIData->AIBodyArmourSelection.Num() > 0)
		ArmourName = AssignedAIData->AIBodyArmourSelection[FMath::RandRange(0, AssignedAIData->AIBodyArmourSelection.Num() - 1)];

	UDataTable* DataTable = UBpGameplayHelperLib::GetSuspectArmourDataTable();
	if (DataTable)
	{
		if (!SuspectArmour && !ArmourName.IsNone())
		{
			FSuspectArmourData* ArmourData = DataTable->FindRow<FSuspectArmourData>(ArmourName, "Suspect Armour Equip");

			#if WITH_EDITOR
			FMessageLog PIEMessageLog = FMessageLog(FName("PIE"));
			if (!ArmourData)
			{
				PIEMessageLog.Error(FText::FromString(FString::Printf(TEXT("Could not find AI armour data table row %s"), *ArmourName.ToString())));
			}
			if (ArmourData && ArmourData->bIsHelmet)
			{
				PIEMessageLog.Error(FText::FromString(FString::Printf(TEXT("Tried to equip helmet %s as a vest"), *ArmourName.ToString())));
			}
			#endif

			if (ArmourData && !ArmourData->bIsHelmet)
			{
				UClass* ArmourClass = ArmourData->BlueprintClass.LoadSynchronous();
				SuspectArmour = GetWorld()->SpawnActor<ASuspectArmour>(ArmourClass ? ArmourClass : ASuspectArmour::StaticClass());
				SuspectArmour->SetArmourData(*ArmourData);

				if (ArmourData->Footsteps)
				{
					CharacterMeshData.Footsteps = ArmourData->Footsteps;
					OnRep_CharacterMeshData();
				}
			}
		}

		if (!SuspectHelmet && AssignedAIData->AIHelmetSelection.Num() > 0)
		{
			FName HelmetName = AssignedAIData->AIHelmetSelection[FMath::RandRange(0, AssignedAIData->AIHelmetSelection.Num() - 1)];
			FSuspectArmourData* ArmourData = DataTable->FindRow<FSuspectArmourData>(HelmetName, "Suspect Helmet Equip");

			#if WITH_EDITOR
			FMessageLog PIEMessageLog = FMessageLog(FName("PIE"));
			if (!ArmourData)
			{
				PIEMessageLog.Error(FText::FromString(FString::Printf(TEXT("Could not find AI helmet data table row %s"), *ArmourName.ToString())));
			}
			if (ArmourData && !ArmourData->bIsHelmet)
			{
				PIEMessageLog.Error(FText::FromString(FString::Printf(TEXT("Tried to equip vest %s as a helmet"), *ArmourName.ToString())));
			}
			#endif

			if (ArmourData && ArmourData->bIsHelmet)
			{
				UClass* ArmourClass = ArmourData->BlueprintClass.LoadSynchronous();
				SuspectHelmet = GetWorld()->SpawnActor<ASuspectArmour>(ArmourClass ? ArmourClass : ASuspectArmour::StaticClass());
				SuspectHelmet->SetArmourData(*ArmourData);
			}
		}
	}

	float SpeedMultiplier = 1.0f;
	float AccelerationMultiplier = 1.0f;
	if (SuspectArmour)
	{
		GetInventoryComponent()->AddInventoryItem(SuspectArmour);
		GetInventoryComponent()->GetSpawnedGear().Armor = SuspectArmour;

		SpeedMultiplier *= SuspectArmour->GetArmourSpeedMultiplier();
		AccelerationMultiplier *= SuspectArmour->GetArmourAccelerationMultiplier();
	}
	if (SuspectHelmet)
	{
		GetInventoryComponent()->AddInventoryItem(SuspectHelmet);
		GetInventoryComponent()->GetSpawnedGear().Helmet = SuspectHelmet;

		SpeedMultiplier *= SuspectHelmet->GetArmourSpeedMultiplier();
		AccelerationMultiplier *= SuspectHelmet->GetArmourAccelerationMultiplier();
	}

	MoveStyle->SetCharacterSpeedMultiplier(SpeedMultiplier);
	MoveStyle->SetCharacterAccelerationMultiplier(AccelerationMultiplier);

	bFinishedEquippingArmour = true;
}

void ACyberneticCharacter::PlayWeaponFireAnimation(ABaseMagazineWeapon* Weapon, const bool bIsAiming, const bool bOnlyTP)
{
	if (!Weapon || !Weapon->AnimationData)
		return;
	
	Play3PMontage(Weapon->AnimationData->Recoil_Level_01.Body_TP);
}

void ACyberneticCharacter::Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal)
{
	SuppressionAmount = FMath::Clamp(SuppressionAmount + SuppressionData.Strength, 0.0f, 1.0f);

	UMoraleComponent::LowerMoraleOnCharacter(this, AI_CONFIG_GET_FLOAT("SuppressionMorale.Damage", 0.025f), "Suppression");
	
	if (GetCyberneticsController())
	{
		GetCyberneticsController()->BulletsFiredTowardsAccuracyPenalty--;
		GetCyberneticsController()->BulletsFiredTowardsAccuracyPenalty = FMath::Max(0u, GetCyberneticsController()->BulletsFiredTowardsAccuracyPenalty);
	}

	PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING, 10.0f);
	
	Super::Multicast_InflictSuppression_Implementation(SuppressionData, CameraShake, bLessLethal);
}

FRotator ACyberneticCharacter::GetFireAtRotation(FVector BulletSpawnLocation, FRotator BulletSpawnRotation)
{
	ACyberneticController* OwnerController = Cast<ACyberneticController>(GetController());
	if (OwnerController)
	{
		AReadyOrNotCharacter* TargetEnemy = OwnerController->GetTrackedTarget();
		if (TargetEnemy)
		{
			ABaseWeapon* bw = Cast<ABaseWeapon>(GetEquippedItem());
			if (bw)
			{
				// Force Aim At Target (100 health = 75% chance, 10 health = 10% chance to force hit)
				const bool bForceAimAt = FMath::FRandRange(0.0f, 1.0f) < FMath::Clamp(TargetEnemy->GetHealthComponent()->GetCurrentResource(), 0.1f, 0.5f);
				if (TargetEnemy)
				{
					FRotator SpawnRot = UKismetMathLibrary::FindLookAtRotation(bw->GetBulletSpawn()->GetComponentLocation(), OwnerController->GetTargetingComp()->GetLastKnownEnemyPosition());
					if (!bForceAimAt)
					{
						SpawnRot.Yaw += FMath::RandRange(-0.6f, 0.6f);
						SpawnRot.Pitch += FMath::RandRange(-0.6f, 0.6f);
					}
					
					return SpawnRot;
				}
			}
		}
	}
	
	return FRotator::ZeroRotator;
}

UFMODEvent* ACyberneticCharacter::GetAppropriateVoiceLineEvent()
{
	if (IsOnSWATTeam())
		return Super::GetAppropriateVoiceLineEvent();
	
	return FMODVoiceLineSpatalized;
}

FVector ACyberneticCharacter::GetNavAgentLocation() const
{
	return GetCharacterMovement()->GetActorFeetLocation();
}

bool ACyberneticCharacter::CanArrest() const
{
	if (IsOnSWATTeam())
		return false;

	// Don't want to arrest an invisible corpse
	if (bIsInBitsAndPieces)
		return false;

	if (bArrestComplete || bIsBeingArrested)
		return false;
	
	// Can't arrest people who are missing arms
	if (GetGibComponent() && (GibComponent->IsLimbGibbed(EGibAreas::GA_LeftArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_RightArm) || GibComponent->IsLimbGibbed(EGibAreas::GA_Head)))
		return false;
	
	if (IsInRagdoll())
		return true;
	
	if (IsSurrenderedFor(1.5f))
		return true;

	if ((IsDeadOrUnconscious() || IsPlayingDead()) && bHasBeenReported)
		return true;
	
	return false;
}

bool ACyberneticCharacter::CanReportNow_Implementation()
{
	if (bIsPlayingDead)
		return true;
	
	return Super::CanReportNow_Implementation();
}

UScoringComponent* ACyberneticCharacter::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

#if WITH_EDITOR
void ACyberneticCharacter::PlayDebugVoiceLineFromTable()
{
	PlayRawVO(DebugVoiceLine);
}

void ACyberneticCharacter::PlayDebugConversation()
{
	PlayBarkOrStartConversation(DebugVoiceLine);
}
#endif

void ACyberneticCharacter::Multicast_OnKilled_Implementation(FName LastBoneHit, AActor* DamageCauser)
{	
	if (IsSuspect() && GetEquippedItem() && AssignedAIData)
	{
		ForceFireGun(AssignedAIData->ChanceToFireGunOnDeath);
	}

	if (bCommitingSuicide)
	{
		if (FMath::RandBool())
			PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_PAIN, "", false);
	}
	else
	{
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_DEATH, "", false);
		UAISense_Hearing::ReportNoiseEvent(this, GetActorLocation(), 2.0f, this, 0.0f, "DeathScream");
	}
	
	if (ThrownItem)
	{
		ThrownItem->MarkAsEvidence(true);
	}

	ReasonsToSprint.Empty();
	ReasonsToWalk.Empty();
	
	bIsMoving = false;
	bIsStrafing = false;	
	
	if (ACyberneticController* CyberneticController = GetCyberneticsController())
	{
		//CyberneticController->GetRONPathFollowingComp()->SetCrowdSimulationState(ECrowdSimulationState::Disabled);
		
		CyberneticController->Destroy();
	}

	if (ThrownByCharacter && DeathReason == ECharacterDeathReason::FellFromHighHeight)
	{
		KilledBy = ThrownByCharacter;
	}

	// If we're not an AI or a God
	if (APlayerCharacter* KilledByPlayer = Cast<APlayerCharacter>(KilledBy))
	{
		#if !UE_BUILD_SHIPPING
		if (!KilledByPlayer->HasGodMode())
		{
		#endif
			if (IsOnSWATTeam())
			{
				if (KilledBy->IsOnSWATTeam())
				{
					const FString Suffix = KilledByPlayer->GetPlayerState() ? " (" + KilledByPlayer->GetPlayerState()->GetPlayerName() + ")" : "";

					FText ScoreText = FText::Format(FText::FromString("{0}{1}"), AScoringManager::PENALTY_FRIENDLY_TEAM_KILL, FText::FromString(Suffix));
					ScoringComponent->GivePenalty(AScoringManager::PENALTY_FRIENDLY_TEAM_KILL, false, ScoreText);
					
					PlayROEViolateTOCResponse();
				}
				else
				{
					ScoringComponent->TakeAllScores();
				}
		
				ScoringComponent->ChangeScoreGroup(AScoringManager::SCORE_REPORT_DOWNED_OFFICER);
			}
			else
			{
				ScoringComponent->TakeAllScores();

				if (IsUnjustifiedUseOfForce(KilledByPlayer, KilledByPlayer->GetEquippedItem()))
				{
					ScoringComponent->GivePenalty(AScoringManager::PENALTY_UNAUTHORIZED_FORCE);
					ScoringComponent->GivePenalty(AScoringManager::PENALTY_UNAUTHORIZED_DEADLY_FORCE);

					if (HasLineOfSightToCharacter(KilledByPlayer) && IsCivilian())
					{
						PlayROEViolateTOCResponse();
						
						bPendingROEViolateResponseOnReport = false;
					}
					else
					{
						bPendingROEViolateResponseOnReport = true;
					}
				}
			}

			if (AScoringManager* ScoringManager = AScoringManager::Get())
			{
				// Re-enable report score
				FText BonusScoreName = FText::FromStringTable("ScoringTable", "Report");
				if (IsSuspect())
					BonusScoreName = AScoringManager::BONUS_SUSPECT_REPORTED;
				else if (IsCivilian())
					BonusScoreName = AScoringManager::BONUS_CIVILIAN_REPORTED;

				// If already reported, but AI is killed, we need to report again, so renable the report score
				if (FScoreBonus* ReportScore = ScoringManager->GetBonus(ScoringComponent, BonusScoreName))
				{
					ReportScore->bEnabled = true;
					ReportScore->bGiven = false;
				}
			}
		#if !UE_BUILD_SHIPPING
		}
		#endif
	}

	if (IsCivilian())
	{
		ScoringComponent->GivePenalty(AScoringManager::PENALTY_CIVILIAN_KILLED);
	}

	Super::Multicast_OnKilled_Implementation(LastBoneHit, DamageCauser);
}

bool ACyberneticCharacter::CanArrestRagdoll() const
{
	if (IsOnSWATTeam())
		return false;
	
	if (IsInRagdoll())
	{
		if (IsDeadNotUnconscious() || IsPlayingDead())
		{
			if (!bHasBeenReported)
				return false;
		}
	
		return !bArrestComplete;
	}
		
	return false;
}

void ACyberneticCharacter::OnRep_MeshReplicated()
{
	if (CharacterLookOverride.BodyMeshOverride)
	{
		Rep_BodyMesh = CharacterLookOverride.BodyMeshOverride;		
	}
	
	if (CharacterLookOverride.FaceMeshOverride)
	{
		Rep_FaceMesh = CharacterLookOverride.FaceMeshOverride;
	}

	if (!IsLocalPlayer())
	{
		for (ABaseItem* i : GetInventoryComponent()->GetInventoryItems())
		{
			if (i)
			{
				i->GetItemMesh()->SetSkeletalMesh(i->GetAppropriateSkeletalMesh());
				i->GetItemMesh()->OverrideMaterials.Empty();
			}
		}
	}
	
	if (Rep_BodyMesh)
	{
		GetMesh()->SetSkeletalMesh(Rep_BodyMesh, !GetMesh()->SkeletalMesh);
		GetMesh()->EmptyOverrideMaterials();
	}

	if (Rep_FaceMesh)
	{
		GetFaceMesh()->SetSkeletalMesh(Rep_FaceMesh, !GetFaceMesh()->SkeletalMesh);
		GetFaceMesh()->EmptyOverrideMaterials();
	}
	else
	{
		GetFaceMesh()->SetSkeletalMesh(nullptr);
	}

	SkinnedDecalSampler->ClearAllDecals();
	SkinnedDecalSampler->SetupMaterials();
}

void ACyberneticCharacter::GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData)
{
	#if !UE_BUILD_SHIPPING
	Super::GatherDebugData_Implementation(OutDebugData);

	if (!GetCyberneticsController())
		return;

	FString MovementStatusString = "None";
	switch (GetCyberneticsController()->GetPathFollowingComponent()->GetStatus())
	{
		case EPathFollowingStatus::Idle:		MovementStatusString = "Idle"; break;
		case EPathFollowingStatus::Waiting:		MovementStatusString = "Waiting"; break;
		case EPathFollowingStatus::Paused:		MovementStatusString = "Paused"; break;
		case EPathFollowingStatus::Moving:		MovementStatusString = "Moving"; break;
		default: break;
	}
	
	OutDebugData.Add({"Health", FText::AsNumber(GetCurrentHealth())});
	OutDebugData.Add({"Move Style", FText::FromName(MoveStyle->Rep_MoveStyleName)});
	OutDebugData.Add({"Gait", FText::FromName(MoveStyle->ActiveGaitName)});

	if (ReasonsToSprint.Num() > 0)
	{
		FString SprintingReasons = "";
		int32 i = 0;
		for (const FString& Reason : ReasonsToSprint)
		{
			SprintingReasons += Reason;
			
			if (i < ReasonsToSprint.Num()-1)
				SprintingReasons += ", ";
			
			i++;
		}
		
		OutDebugData.Add({"Reasons To Sprint", FText::FromString(SprintingReasons)});
	}
	
	if (IsOnSWATTeam())
		OutDebugData.Add({"Squad Position", FText::FromString(ENUM_TO_STRING(ESquadPosition, SquadPosition, false))});
	
	//OutDebugData.Add({"Crouching", FText::FromString(bIsCrouched ? "true" : "false")});
	
	OutDebugData.Add({"Moving", FText::FromString(bIsMoving ? "true" : "false") });
	OutDebugData.Add({"Moving On Path", FText::FromString(GetCyberneticsController()->IsActivelyMovingOnPath() ? "true" : "false") });
	OutDebugData.Add({"Movement Status", FText::FromString(MovementStatusString) });
	OutDebugData.Add({"Current Move Request ID", FText::FromString(GetCyberneticsController()->GetPathFollowingComponent()->GetCurrentRequestId().ToString()) });

	OutDebugData.Add({"Aim Offset", FText::FromString(AimOffset.ToString())});
	OutDebugData.Add({"Target Focal Point", GetCyberneticsController()->TargetFocalPoint.ToText()});
	OutDebugData.Add({"Focal Point", Rep_FocalPoint.ToText()});
	
	OutDebugData.Add({"Equipped Item", FText::FromString(GetNameSafe(GetEquippedItem())) });
	
	ABaseMagazineWeapon* bmw = Cast<ABaseMagazineWeapon>(GetEquippedItem());
	if (bmw)
	{
		OutDebugData.Add({"Ammo", FText::FromString(FString::FromInt(bmw->GetAmmo()))});
		OutDebugData.Add({"Mags", FText::FromString(FString::FromInt(bmw->GetMagazineCount()))});
		OutDebugData.Add({"Time Until Next Fire", FText::FromString(FString::SanitizeFloat(bmw->RefireDelayTimer))});
		
		const FVector& FireDirection = bmw->GetBulletSpawn()->GetForwardVector();
		const FVector& DirectionToFocalPoint = (Rep_FocalPoint - bmw->GetBulletSpawn()->GetComponentLocation()).GetSafeNormal();
		const float TargetRotationDelta = FVector::DotProduct(FireDirection, DirectionToFocalPoint);
		
		OutDebugData.Add({"Aiming Dot Product", FText::AsNumber(TargetRotationDelta)});
	}
	
	OutDebugData.Add({"Weapon Anim Type", FText::FromString(RON_ENUM_TO_STRING(EAnimWeaponType, GetCurrentWeaponAnimType()))});

	OutDebugData.Add({"Surrendered", FText::FromString(IsSurrendered() ? "True" : "False")});
	OutDebugData.Add({"Arrested", FText::FromString(IsArrested() ? "True" : "False")});
	OutDebugData.Add({"Stunned", FText::FromString(IsStunned() ? "True" : "False")});
	OutDebugData.Add({"Hesitating", FText::FromString(IsHesitating() ? "True" : "False")});
	//OutDebugData.Add({"Ragdoll", FText::FromString(IsInRagdoll() ? "True" : "False")});

	if (GetCyberneticsController())
	{
		OutDebugData.Add({"Awareness State", FText::FromString(RON_ENUM_TO_STRING(EAIAwarenessState, GetCyberneticsController()->GetAwarenessState()))});
		OutDebugData.Add({"Tracked Target", FText::FromString((GetCyberneticsController()->GetTrackedTarget() ? GetCyberneticsController()->GetTrackedTarget()->GetName() : "None"))});
		OutDebugData.Add({"Last Tracked Target", FText::FromString(GetNameSafe(GetCyberneticsController()->GetLastTrackedEnemy()))});
		OutDebugData.Add({"Aiming At Target", FText::FromString(bAimingAtTarget ? "True" : "False")});
		OutDebugData.Add({"Time Tracking Target", FText::AsNumber(GetCyberneticsController()->GetTargetingComp()->GetTimeTrackingTarget())});
		
		if (const UBaseCombatActivity* CombatActivity = GetCyberneticsController()->GetCombatActivity<UBaseCombatActivity>())
		{
			OutDebugData.Add({"Time With Weapon Up", FText::AsNumber(CombatActivity->TimeSpentWithWeaponUp)});
		}
		
		//OutDebugData.Add({"Path Acceleration", FText::AsNumber(GetCyberneticsController()->GetRONPathFollowingComp()->GetAcceleration()) });
		if (GetCyberneticsController()->GetMoraleComp())
			OutDebugData.Add({"Morale", FText::FromString(FString::SanitizeFloat(GetCyberneticsController()->GetMoraleComp()->GetMorale()))});
		
		if (GetCyberneticsController()->GetTargetingComp())
		{
			OutDebugData.Add({"Threat", FText::AsNumber(GetCyberneticsController()->GetTargetingComp()->Threat)});
			OutDebugData.Add({"Tension", FText::AsNumber(GetCyberneticsController()->GetTargetingComp()->Tension)});
		}
		
		OutDebugData.Add({"Focus Actor", FText::FromString(GetCyberneticsController()->GetFocusActor() ? GetCyberneticsController()->GetFocusActor()->GetName() : "None")});
		OutDebugData.Add({"Is Strafing", FText::FromString(IsStrafing() ? "True" : "False")});

		if (const UBaseActivity* CurrentActivity = GetCyberneticsController()->GetCurrentActivity())
		{
			OutDebugData.Add({"Current Activity", FText::FromString(CurrentActivity->GetName())});
			OutDebugData.Add({"Active State", FText::FromString(CurrentActivity->GetActiveStateName())});
		}
		
		OutDebugData.Add({"Activity Queue", FText::FromString(GetCyberneticsController()->GetActivityQueueAsString())});
		//OutDebugData.Add({"Num Activity Queue", FText::AsNumber(GetCyberneticsController()->GetActivityQueueCount())});

		if (const UBaseCombatActivity* CombatActivity = GetCyberneticsController()->GetCombatActivity<UBaseCombatActivity>())
		{
			OutDebugData.Add({"Combat Activity", FText::FromString(CombatActivity->GetName())});
			OutDebugData.Add({"Active State", FText::FromString(CombatActivity->GetActiveStateName())});
			//OutDebugData.Add({"Engagement Type", FText::FromString(RON_ENUM_TO_STRING(ECombatEngagementType, CombatActivity->GetCombatEngagementType()))});
			OutDebugData.Add({"Combat Move Activity", FText::FromString(GetNameSafe(CombatActivity->GetCombatMoveActivity<UBaseCombatMoveActivity>()))});
		}
	}

	if (GetCyberneticsController() && GetCyberneticsController()->GetTargetingComp())
	{
		if (AThreatAwarenessActor* NearestThreat = GetCyberneticsController()->GetTargetingComp()->GetNearestThreat())
		{
			OutDebugData.Add({"", FText::FromString("")});
			OutDebugData.Add({"Nearest Threat", FText::FromString(NearestThreat->GetName())});
			
			if (NearestThreat->GetAttachedDoor())
				OutDebugData.Add({"Nearest Threat (Door)", FText::FromString(NearestThreat->GetAttachedDoor()->GetName())});
			
			OutDebugData.Add({"Nearest Threat (Threat Level)", FText::FromString(ENUM_TO_STRING(EThreatLevel, EThreatLevel(NearestThreat->GetThreatLevel()), false))});
		}
		OutDebugData.Add({"Tracking", FText::FromString(ENUM_TO_STRING(ETargetingCompTracking, GetCyberneticsController()->GetTargetingComp()->GetTrackingType(), false))});
	}
	#endif
}

void ACyberneticCharacter::OnActorSpawned(AActor* Actor)
{
	ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(Actor);
	if (CyberneticCharacter)
	{
		MoveIgnoreActorAdd(CyberneticCharacter);
		CyberneticCharacter->MoveIgnoreActorAdd(this);
	}
	Super::OnActorSpawned(Actor);
}

void ACyberneticCharacter::GatherDebugText_Implementation(FString& OutText)
{
	#if !UE_BUILD_SHIPPING
	Super::GatherDebugText_Implementation(OutText);
	if (CVarDetailedCyberneticsDebug.GetValueOnGameThread() == 0)
		return;

	OutText += " \nAimOffset: " + AimOffset.ToString();
	
	// Activities List
	if (GetCyberneticsController())
	{
		
		if (GetCyberneticsController()->GetFocusActor())
		{
			OutText += " \nFocus Actor: " + GetCyberneticsController()->GetFocusActor()->GetName(); 
		}
		FString IsStrafingTxt = (IsStrafing() ? "True" : "False");
		OutText += " \nIs Strafing: " + IsStrafingTxt;
		OutText += " \n---Activity Queue-- \n" + GetCyberneticsController()->GetActivityQueueAsString();

		OutText += " \nTargetEnemy: " + (GetCyberneticsController()->GetTrackedTarget() ? GetCyberneticsController()->GetTrackedTarget()->GetName() : "None");
		for (FString s : ReasonsToStandStill)
		{
			OutText += " \nReasons To Stand Still: " + s;
		}

		GetCyberneticsController()->GetTargetingComp()->GatherDebugText(OutText);
		if (GetCyberneticsController()->GetCombatActivity<UBaseCombatActivity>())
		{
			GetCyberneticsController()->GetCombatActivity<UBaseCombatActivity>()->GatherDebugText(OutText);
		}
		
	}
	OutText += " \nFocal Point: " + Rep_FocalPoint.ToString();
	if (IsSurrendered())
	{
		OutText += " \nAI has surrendered! ";
	}
	if (IsArrested())
	{
		OutText += " \nAI is arrested! ";
	}
	#endif
}

void ACyberneticCharacter::DrawVisualDebug_Implementation()
{
	#if !UE_BUILD_SHIPPING
	if (CVarRonToggleAIDebugLines.GetValueOnAnyThread() <= 0)
		return;
	
	ACyberneticController* CyberneticController = GetCyberneticsController();
	if (!CyberneticController)
		return;

	UBaseActivity* Activity = CyberneticController->GetCurrentActivity();
	if (Activity)
	{
		DrawDebugPoint(GetWorld(), Activity->GetLocation(), 10.0f, FColor::Green);
		DrawDebugLine(GetWorld(), GetActorLocation(), Activity->GetLocation(), FColor::Green);
	}

	if (CyberneticController->GetPathFollowingComponent()->GetPathDestination() != FVector::ZeroVector)
	{
		DrawDebugPoint(GetWorld(), CyberneticController->GetPathFollowingComponent()->GetPathDestination(), 10.0f, FColor::Yellow);
		DrawDebugLine(GetWorld(), GetActorLocation(), CyberneticController->GetPathFollowingComponent()->GetPathDestination(), FColor::Yellow);
	}

	if (CyberneticController->GetTargetingComp() && CyberneticController->GetTargetingComp()->GetNearestThreat())
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), CyberneticController->GetTargetingComp()->GetNearestThreat()->GetActorLocation(), FColor::Blue);
		DrawDebugPoint(GetWorld(), CyberneticController->GetTargetingComp()->GetNearestThreat()->GetActorLocation(), 10.0f, FColor::Blue);
	}

	if (Rep_FocalPoint != FVector::ZeroVector)
	{
		DrawDebugPoint(GetWorld(), Rep_FocalPoint, 10.0f, FColor::Red);
		DrawDebugLine(GetWorld(), GetActorLocation(), Rep_FocalPoint, FColor::Red);
	}
	#endif
}

TArray<FDebugData> ACyberneticCharacter::GetDebugInfoOnROE() const
{
	#if !UE_BUILD_SHIPPING
	TArray<FDebugData> DebugData;
	DebugData.Reserve(20);

	LOCAL_PLAYER;
	if (!LocalPlayer)
		return DebugData;

	FString CurrentMontageName = "None";
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr; 
	if (AnimInstance)
	{
		if (UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage())
			CurrentMontageName = CurrentMontage->GetName();
	}

	UDamageType* CurrentDamageType = nullptr;

	if (ABaseWeapon* EquippedWeapon = LocalPlayer->GetEquippedItem<ABaseWeapon>())
	{
		if (EquippedWeapon->DamageType)
		{
			CurrentDamageType = EquippedWeapon->DamageType->GetDefaultObject<UDamageType>();
		}
	}
	else if (ABaseGrenade* EquippedGrenade = LocalPlayer->GetEquippedItem<ABaseGrenade>())
	{
		if (EquippedGrenade->DetonationDamage.IsValidIndex(0))
		{
			if (const TSubclassOf<UDamageType> GrenadeDamageTypeClass = EquippedGrenade->DetonationDamage[0].DamageType)
			{
				CurrentDamageType = GrenadeDamageTypeClass->GetDefaultObject<UDamageType>();
			}
		}
	}
	
	DebugData.Add({"AI Character: ", FText::FromString(GetName())});
	DebugData.Add({"Team: ", FText::FromString(RON_ENUM_TO_STRING(ETeamType, GetTeam()))});
	DebugData.Add({"Current Montage: ", FText::FromString(CurrentMontageName)});
	DebugData.Add({"Is Unjustified Use of Force: ", FText::FromString(IsUnjustifiedUseOfForce(LocalPlayer, LocalPlayer->GetEquippedItem(), CurrentDamageType) ? "true" : "false")});
	DebugData.Add({"Aggressor In God Mode: ", FText::FromString(LocalPlayer->HasGodMode() ? "true" : "false")});
	DebugData.Add({"Aggressor Was Damaged by this AI: ", FText::FromString(LocalPlayer->HasBeenDamagedByCharacter(this) ? "true" : "false")});
	DebugData.Add({"Same Team As Aggressor: ", FText::FromString(IsOnSameTeam(LocalPlayer, const_cast<ACyberneticCharacter*>(this)) ? "true" : "false")});
	DebugData.Add({"Is Surrendered: ", FText::FromString(IsSurrendered() ? "true" : "false")});
	DebugData.Add({"Is Fake Surrendering: ", FText::FromString(bIsFakeSurrender ? "true" : "false")});
	DebugData.Add({"Is Arrested: ", FText::FromString(IsArrested() ? "true" : "false")});
	DebugData.Add({"Is Being Arrested: ", FText::FromString(bIsBeingArrested ? "true" : "false")});
	DebugData.Add({"Is Stunned: ", FText::FromString(IsStunned() ? "true" : "false")});
	DebugData.Add({"Is Ragdoll: ", FText::FromString(IsInRagdoll() ? "true" : "false")});
	DebugData.Add({"Can Be Arrested: ", FText::FromString(CanArrest() ? "true" : "false")});
	DebugData.Add({"Has Ever Damaged SWAT: ", FText::FromString(HasDamagedSWAT() ? "true" : "false")});
	DebugData.Add({"Has Ever Shot Weapon: ", FText::FromString(bHasEverShot ? "true" : "false")});
	DebugData.Add({"Is Aiming At Target: ", FText::FromString(bAimingAtTarget ? "true" : "false")});
	DebugData.Add({"Is Drawing Weapon: ", FText::FromString(bDrawingWeapon ? "true" : "false")});
	DebugData.Add({"Is Picking Up Weapon: ", FText::FromString(bPickingUpWeapon ? "true" : "false")});
	DebugData.Add({"Is Fleeing: ", FText::FromString(bIsFleeing ? "true" : "false")});
	DebugData.Add({"Damaged By Lethal: ", FText::FromString(HasBeenDamagedByLethal() ? "true" : "false")});
	DebugData.Add({"Damaged By Less Lethal: ", FText::FromString(HasBeenDamagedByLessLethal() ? "true" : "false")});
	DebugData.Add({"Combat State: ", FText::FromString(RON_ENUM_TO_STRING(ECombatState, CombatState))});

	return DebugData;
	#else
	return {};
	#endif
}

void ACyberneticCharacter::UpdateDefaultMoveStyle()
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

	bOverridingMoveStyle = true;
	
	if (bArrestComplete)
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.ArrestedMoveStyle);
		return;
	}

	if (IsSurrenderedFor(1.5f) || bSurrenderComplete)
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.SurrenderedMoveStyle);
		return;
	}

	if (IsStunned())
	{
		MoveStyle->SetMovementStyleByName(MovementStyleData.StunnedMoveStyle);
		return;
	}

	if (const UBaseActivity* Activity = GetCyberneticsController()->GetCurrentActivity())
	{
		const FName Override = Activity->GetMoveStyleOverride();
		if (Override != NAME_None)
		{
			MoveStyle->SetMovementStyleByName(Override);
			return;
		}
	}

	if (GetCyberneticsController()->GetCombatActivity())
	{
		if (const UBaseCombatMoveActivity* Activity = GetCyberneticsController()->GetCombatActivity()->GetCombatMoveActivity())
		{
			const FName Override = Activity->GetMoveStyleOverride();
			if (Override != NAME_None)
			{
				MoveStyle->SetMovementStyleByName(Override);
				return;
			}
		}
	}

	if (GetCyberneticsController()->BestAction)
	{
		if (const UAIAction* CustomAction = GetCyberneticsController()->BestAction->GetCustomAction(GetCyberneticsController()))
		{
			const FName Override = CustomAction->GetMoveStyleOverride();
			if (Override != NAME_None)
			{
				MoveStyle->SetMovementStyleByName(Override);
				return;
			}
		}
	}
	
	bOverridingMoveStyle = false;

	static const FName UnarmedMoveStyleMale = "male01_shared_unarmed";
	static const FName UnarmedMoveStyleFemale = "female01_shared_unarmed";
	static const FName UnarmedInjuredMoveStyleMale = "male01_civilian_injured";
	static const FName RifleLoweredInjuredMoveStyle = "male01_suspect_rifle_injured";
	static const FName RifleStrafeInjuredMoveStyle = "male01_suspect_rifle_strafe_injured";
	static const FName PistolStrafeInjuredMoveStyle = "male01_suspect_pistol_strafe_injured";
	static const FName PistolLoweredInjuredMoveStyle = "male01_suspect_pistol_injured";
	static const FName RifleSuspiciousMoveStyle = "male01_suspect_rifle_suspicious";
	static const FName PistolSuspiciousMoveStyle = "male01_suspect_pistol_suspicious";
	const FName DefaultUnarmedMoveStyle = bFemale ? UnarmedMoveStyleFemale : UnarmedMoveStyleMale;
	const FName UsedUnarmedMoveStyle = MovementStyleData.UnarmedMoveStyle.IsNone() ? DefaultUnarmedMoveStyle : MovementStyleData.UnarmedMoveStyle;

	const bool IsInjured = IsHalfHealth() || bIsArterialBleeding;
	if (GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Unarmed)
	{
		MoveStyle->SetMovementStyleByName(IsInjured ? UnarmedInjuredMoveStyleMale : UsedUnarmedMoveStyle);
	}
	else
	{
		bool bScriptedFiring = false;
		bool bCombatActivityWantsNoStrafe = false;
		if (const UBaseCombatActivity* CombatActivity = GetCyberneticsController()->GetCombatActivity())
		{
			bScriptedFiring = CombatActivity->IsTryingToFireAtScriptedActor();
			bCombatActivityWantsNoStrafe = !CombatActivity->ShouldStrafe();
		}
		
		const bool bIsUnAlert = GetCyberneticsController()->GetAwarenessState() == EAIAwarenessState::Unalerted;
		const bool bIsSuspicious = GetCyberneticsController()->GetAwarenessState() == EAIAwarenessState::Suspicious;
			
		const float StressUntilWeaponRaise = AI_CONFIG_GET_FLOAT("StressUntilWeaponRaise", 0.5f);
		if (bIsSuspicious && !bHasEverShot && !bScriptedFiring && Stress < StressUntilWeaponRaise)
		{
			if (GetCurrentWeaponAnimType() == EAnimWeaponType::CWT_Pistol)
				MoveStyle->SetMovementStyleByName(PistolSuspiciousMoveStyle);
			else
				MoveStyle->SetMovementStyleByName(RifleSuspiciousMoveStyle);
		}
		else
		{
			const bool bShouldLower = bIsUnAlert && bCombatActivityWantsNoStrafe && !bScriptedFiring;
			
			// weapon is lowered
			if (IsLowReady() || bShouldLower)
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
	}
}

bool ACyberneticCharacter::IsSameFaction(ACyberneticCharacter* OtherAI) const
{
	if (!OtherAI)
		return false;
	
	if (FactionData.Name.IsEmpty() || FactionData.Name == "None")
		return false;
	
	if (OtherAI->FactionData.Name.IsEmpty() || OtherAI->FactionData.Name == "None")
		return false;
	
	return FactionData.Name == OtherAI->FactionData.Name;
}

FSpawnData* ACyberneticCharacter::GetSpawnData() 
{
	if (SpawnData)
	{
		return SpawnData;
	}
	
	return &DefaultSpawnData;
}

const FSpawnData* ACyberneticCharacter::GetSpawnData() const
{
	if (SpawnData)
	{
		return SpawnData;
	}
	
	return &DefaultSpawnData;
}

int32 ACyberneticCharacter::GetMeleeCountFor(const AReadyOrNotCharacter* Character)
{
	if (const int32* ValuePtr = MeleeCountMap.Find(Character))
	{
		return *ValuePtr;
	}

	return 0;
}

bool ACyberneticCharacter::IsDamagedByLethal() const
{
	for (ABaseWeapon* Weapon : DamagedByWeapons)
	{
		if (Weapon && Weapon->IsLethalWeapon())
		{
			return true;
		}
	}

	return false;
}

bool ACyberneticCharacter::IsDamagedByLessLethal() const
{
	for (ABaseWeapon* Weapon : DamagedByWeapons)
	{
		if (Weapon && Weapon->IsLessLethalWeapon())
		{
			return true;
		}
	}

	return false;
}

bool ACyberneticCharacter::IsTraversingHole() const
{
	if (!GetCyberneticsController())
		return false;
	
	return GetCyberneticsController()->GetCurrentActivity<UTraverseHoleActivity>() != nullptr;
}

bool ACyberneticCharacter::IsRaisingWeapon() const
{
	return IsTableMontagePlaying("tp_raise");
}

bool ACyberneticCharacter::IsLoweringWeapon() const
{
	return IsTableMontagePlaying("tp_lower");
}

bool ACyberneticCharacter::ShouldReload() const
{
	if (const ABaseMagazineWeapon* EquippedWeapon = GetEquippedWeapon())
	{
		/*
		if (const UTakeCoverActivity* TakeCoverActivity = GetCyberneticsController()->GetCurrentActivity<UTakeCoverActivity>())
		{
			if (TakeCoverActivity->IsInCover())
			{
				return EquippedWeapon->GetAmmo() < TakeCoverActivity->GetReloadAtAmmoCount();
			}
		}
		*/
		
		return !EquippedWeapon->HasAmmo();
	}

	return false;
}

bool ACyberneticCharacter::CanLowReady() const
{
	if (!GetEquippedWeapon())
		return false;
	
	// No low ready when in cover
	if (IsTakingCover() || IsTakingCoverAtLandmark())
		return false;

	// No low ready when traversing a wall hole
	if (IsTraversingHole())
		return false;

	if (FMath::Abs(QuickLeanAmount) > 0.0f)
		return false;
	
	if (GetCyberneticsController())
	{
		if (GetCyberneticsController()->GetTrackedTarget())
			return false;
		
		// focusing on stair threat?
		// todo: make function
		const AThreatAwarenessActor* TAA = Cast<AThreatAwarenessActor>(GetCyberneticsController()->GetFocusActor());
		if (!TAA || TAA->GetThreatLevel() != EThreatLevel::TL_Stairs)
		{
			TAA = GetCyberneticsController()->GetTargetingComp()->GetNearestExtremeThreat();
			if (TAA && TAA->GetThreatLevel() == EThreatLevel::TL_Stairs)
			{
				if (FVector::Distance(TAA->GetActorLocation(), GetActorLocation() + FVector(0.0f, 0.0f, 70.0f)) < 500.0f)
				{
					return false;
				}
			}
		}
		
		if (TAA && TAA->GetThreatLevel() == EThreatLevel::TL_Stairs)
			return false;
		
		if (GetCyberneticsController()->BestCombatMoveAction != nullptr)
			return false;

		if (GetCyberneticsController()->GetCurrentActivity<UInvestigateStimulusActivity>() != nullptr)
			return false;
	}
	
	if (!bIsMoving)
		return true;

	return true;
}

bool ACyberneticCharacter::ShouldLowReadyNow()
{
	SCOPE_CYCLE_COUNTER(STAT_ShouldLowReadyNow);

	if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
		return false;
	
	const ABaseMagazineWeapon* EquippedWeapon = GetEquippedWeapon();
	if (!EquippedWeapon)
		return false;
	
	float Extra = 0.0f;
	// to prevent low ready oscilation when lowering the weapon, the spine could have moved a few centimeters back
	// and they would stop low ready and then the spine would move forward a bit and then low ready again, over and over.
	// so add a bit of extra padding to when we did hit something last frame, they must move back a reasonable amount to cancel low ready
	// and revert back to no padding when they trace nothing
	if (bPreviousLowReadyTraceHit)
		Extra = 15.0f;
	
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector End = Start + GetActorForwardVector() * (100.0f + Extra);
	
	if (GetWorld()->IsTraceHandleValid(LowReadyCheckHandle, false))
	{
		FTraceDatum OutTraceData;
		if (GetWorld()->QueryTraceData(LowReadyCheckHandle, OutTraceData))
		{
			bPreviousLowReadyTraceHit = false;
			
			if (OutTraceData.OutHits.Num() == 0)
			{
				//DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 0.1f);
				return false;
			}

			#if !UE_BUILD_SHIPPING
			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.05f);
			#endif

			bPreviousLowReadyTraceHit = true;
			return true;
		}
	}
	else
	{
		FCollisionQueryParams QueryParams = GetCollisionQueryParameters();
		QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllReadyOrNotCharacters);
		QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllItems);
		
		LowReadyCheckHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Test, Start, End, ECC_Visibility, QueryParams);
		
		//#if !UE_BUILD_SHIPPING
		//DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.05f);
		//#endif
	}

	return bPreviousLowReadyTraceHit;
}

bool ACyberneticCharacter::IsOpeningDoor(ADoor* Door) const
{
	return (IsTableMontagePlaying("tp_npc_door_push_open") || IsTableMontagePlaying("tp_swat_door_push_open")) && Door == QueuedDoorToOpen;
}

bool ACyberneticCharacter::IsClosingDoor(ADoor* Door) const
{
	return (IsTableMontagePlaying("tp_npc_door_push_open") || IsTableMontagePlaying("tp_swat_door_push_open")) && Door == QueuedDoorToClose;
}

bool ACyberneticCharacter::CanPushDoor(ADoor* Door) const
{
	return !Door->IsOpenAtOrBeyond(0.75f);
}

/*
void ACyberneticCharacter::OnBlendRagdollAnimFinished()
{
	EnableRagdoll();
	
	//UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::PlayIncapacitatedLoop, 3.0f);
}

void ACyberneticCharacter::PlayIncapacitatedLoop()
{
	if (!CharacterHealth->IsIncapacitationEnabled())
		return;

	IncapacitationLoopAnim = nullptr;

	if (CharacterHealth->IsIncapacitated())
	{
		bStartBlendInIncapacitation = true;

		FString Animation = "tp_incap_facing";

		const float UpDotProduct = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), FVector::UpVector);
		const float RightDotProduct = FVector::DotProduct(UKismetMathLibrary::GetRightVector(GetMesh()->GetSocketRotation("pelvis")), GetActorRightVector());
		const bool bIncapacitatedFacingUp = UpDotProduct >= 0.0f;
		if (bIncapacitatedFacingUp)
		{
			if (RightDotProduct > UpDotProduct)
				Animation += "_side";
			else
				Animation += "_up";
		}
		else
		{
			if (RightDotProduct > 0.85f)
				Animation += "_side";
			else
				Animation += "_down";
		}

		ULog::Info(Animation);

		if (UAnimMontage* IncapMontage = GetMontageFromTable(Animation))
		{
			//if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(IncapMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference))
			//{
			//	IncapacitationLoopAnim = AnimSequence;
			//}

			DisableRagdoll();
			bRecoveringFromRagdoll = true;


			if (GetMesh()->GetAnimInstance() && GetMesh()->SkeletalMesh && GetMesh()->GetComponentSpaceTransforms().Num() > 0)
			{
				GetMesh()->GetAnimInstance()->SavePoseSnapshot("RagdollPoseEnd");
			}

			// play the montage that loops forever!
			Multicast_Stop3PMontage(nullptr, 0.0f);
			Play3PMontage(IncapMontage);

			bRagdolledAndIncapacitated = true;

			// reset blending state after playing the motion?
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ACyberneticCharacter::ResetRagdollStates, 1.5f);
		}

		//if (UAnimMontage* CurrentMontage = GetCurrentMontage())
		//{
		//	const float CurrentMontagePosition = GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);

		//	IncapacitationBlendTime = FMath::Max(CurrentMontage->GetPlayLength() - CurrentMontagePosition, 0.0f) * 2.0f;
		//}
		//else
		//{
		//	IncapacitationBlendTime = 0.0f;
		//}

		//if (!IncapacitationLoopAnim)
		//{
		//	bStartBlendInIncapacitation = false;
		//}
	}
}
*/

/* alex: moved from human base anim instance because this needs to be accessed by other anim instances at the same time */
FRotator ACyberneticCharacter::GetLookAtRotation(float YawLimit, float PitchLimit) const
{
	FRotator LookAtRotation = FRotator::ZeroRotator;

	if (IsDeadOrUnconscious() || IsInRagdoll() || IsCarried())
		return FRotator::ZeroRotator;

	if (Rep_HeadFocalPoint == FVector::ZeroVector)
		return FRotator::ZeroRotator;

	FVector EyesLocation;
	FRotator EyesRotation;
	GetActorEyesViewPoint(EyesLocation, EyesRotation);

	FVector v1 = UKismetMathLibrary::FindLookAtRotation(EyesLocation, Rep_HeadFocalPoint).Vector();
	FVector v2 = GetActorForwardVector();

	const float DotProduct2D = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
	if (DotProduct2D > 0.0f)
	{
		LookAtRotation = v1.Rotation().GetNormalized();
	}

	if (LookAtRotation != FRotator::ZeroRotator)
	{
		LookAtRotation = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRotation, GetActorRotation() + FRotator(-AimOffset.X, AimOffset.Y, 0.0f));
		LookAtRotation.Yaw = FMath::Clamp(LookAtRotation.Yaw, -YawLimit, YawLimit);
		LookAtRotation.Pitch = FMath::Clamp(LookAtRotation.Pitch, -PitchLimit, PitchLimit);
		LookAtRotation.Roll = LookAtRotation.Pitch * -1.0f;
		LookAtRotation.Pitch = 0;

		return LookAtRotation;
	}

	//UE_LOG(LogTemp, Warning, TEXT("desiredOrientation %f"), desiredOrientation);

	return FRotator::ZeroRotator;
}