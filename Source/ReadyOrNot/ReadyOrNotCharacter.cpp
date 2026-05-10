// Copyright Void Interactive, 2023

#include "ReadyOrNotCharacter.h"

#include "FMODWorldSubsystem.h"
#include "ObjectPoolBase.h"
#include "ObjectPoolFunctionLibrary.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotDebugSubsystem.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameSession.h"
#include "ReadyOrNotVoiceConfig.h"

#include "GameModes/CoopGS.h"

#include "DamageTypes/StunDamage.h"
#include "DamageTypes/BulletDamageType.h"
#include "DamageTypes/BleedDamageType.h"

#include "Actors/BaseGrenade.h"
#include "Actors/BloodPool.h"
#include "Actors/Door.h"
#include "Actors/ExplosionGibs.h"
#include "Actors/PairedInteractionDriver.h"
#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/Headwear.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"
#include "Actors/Projectiles/DamageProjectiles/GrenadeProjectile.h"

#include "Actors/Gameplay/CollectedEvidenceActor.h"
#include "Actors/Gameplay/ImpactEffect.h"
#include "Actors/Gameplay/EvidenceActor.h"

#include "Characters/CyberneticController.h"

#include "Objectives/NeutralizeSuspectByTag.h"

#include "SkeletalMeshMerge.h"
#include "SkinnedDecalSampler.h"
#include "Actors/AnimatedDecal.h"
#include "Actors/Items/MeleeWeapon.h"
#include "Actors/Items/NightvisionGoggles.h"
#include "Actors/Items/Pepperspray.h"
#include "Actors/Items/Shotgun.h"
#include "Actors/Sound/SoundSource.h"
#include "Actors/Gameplay/PlacedC2Explosive.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "Audio/RoNSoundData.h"
#include "Commander/RosterManager.h"

#include "Components/CharacterHealthComponent.h"
#include "Components/ArmourResourceComponent.h"
#include "Components/GibComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/MapActorComponent.h"
#include "Components/MoraleComponent.h"
#include "Components/ObjectiveMarkerComponent.h"
#include "Components/ReadyOrNotCharMovementComp.h"
#include "Components/ScoringComponent.h"

#include "AMRagdoll/Components/RagdollComponent.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Commander/CommanderGM.h"
#include "Components/PlayerPostProcessing.h"

#include "DamageTypes/LessLethal/CSGasDamageType.h"
#include "DamageTypes/LessLethal/PepperSprayDamageType.h"
#include "GameModes/CoopGM.h"
#include "GameModes/LobbyGS.h"

#include "Info/TOCManager.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/SWATManager.h"
#include "Info/Activities/TakeCoverAtLandmarkActivity.h"
#include "Info/Activities/MoveToExitActivity.h"

#include "Interfaces/Meleeable.h"
#include "lib/HitRegistrationSettings.h"

#include "Navigation/CrowdManager.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

#include "Perception/AISense_Touch.h"
#include "Subsystems/AchievementSubsystem.h"
#include "Subsystems/SubtitlesSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Character Tick"), STAT_RoNCharacterTick, STATGROUP_ReadyOrNotCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Player Marker Tick"), STAT_RoNTick_PlayerMarker, STATGROUP_ReadyOrNotCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Player Interaction Tick"), STAT_RoNTick_PlayerInteraction, STATGROUP_ReadyOrNotCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Can Be Seen From (BaseCharacter)"), STAT_CanBeSeenFrom, STATGROUP_ReadyOrNotCharacter);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Play Raw VO"), STAT_PlayRawVO, STATGROUP_ReadyOrNotCharacter);

// this was in UnrealMathUtility.h in UE5, needed here for acceleration compression
#define RON_TWO_PI			(6.28318530717f)

// custom Cartesian and Polar conversion functions to work with double type, another UE5 Change
/** Converts given Cartesian coordinate pair to Polar coordinate system. */
static FORCEINLINE void RonCartesianToPolar(const double X, const double Y, double& OutRad, double& OutAng)
{
	OutRad = FMath::Sqrt(FMath::Square(X) + FMath::Square(Y));
	OutAng = FMath::Atan2(Y, X);
}

/** Converts given Polar coordinate pair to Cartesian coordinate system. */
static FORCEINLINE void RonPolarToCartesian(const double Rad, const double Ang, double& OutX, double& OutY)
{
	OutX = Rad * FMath::Cos(Ang);
	OutY = Rad * FMath::Sin(Ang);
}

TAutoConsoleVariable<int32> CVarRonAlwaysFake(TEXT("a.RonAlwaysFake"), 0, TEXT("AI should always fake"));
TAutoConsoleVariable<bool> CVarRonShowArteryZones(TEXT("a.RonDrawArteryZones"), false, TEXT("Visualize arterial zones where bleedout hits can occur"));
TAutoConsoleVariable<int32> CVarDisplayVO(TEXT("VO.Display"), false, TEXT("Visualize VO strings"));
static TAutoConsoleVariable<int32> CVarRonDrawCharacterSnapshots(TEXT("a.RonDrawCharacterSnapshots"), 0, TEXT("Draw character snapshots for hit registration debugging"));

void AReadyOrNotCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bAiming, COND_SkipOwner);
	
	DOREPLIFETIME(AReadyOrNotCharacter, ReplicatedControlRotation);
	DOREPLIFETIME(AReadyOrNotCharacter, BlendInterpAmount);
	DOREPLIFETIME(AReadyOrNotCharacter, CharacterHealth);
	DOREPLIFETIME(AReadyOrNotCharacter, KilledBy);
	DOREPLIFETIME(AReadyOrNotCharacter, IncapacitatedBy);
	DOREPLIFETIME(AReadyOrNotCharacter, DeathReason);
	//DOREPLIFETIME(AReadyOrNotCharacter, TimeDowned);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bNVGOn);

	DOREPLIFETIME(AReadyOrNotCharacter, Rep_BodyMesh);
	DOREPLIFETIME(AReadyOrNotCharacter, Rep_FaceMesh);
	
	DOREPLIFETIME(AReadyOrNotCharacter, DefaultTeam);
	DOREPLIFETIME(AReadyOrNotCharacter, bIsBeingArrested);
	DOREPLIFETIME(AReadyOrNotCharacter, bIsBeingCarried);
	DOREPLIFETIME(AReadyOrNotCharacter, bArrestComplete);
	DOREPLIFETIME(AReadyOrNotCharacter, CurrentCarryConfirmTime);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bSurrendered);
	DOREPLIFETIME(AReadyOrNotCharacter, bSurrenderComplete);

	DOREPLIFETIME(AReadyOrNotCharacter, SpeechCharacterName);

	DOREPLIFETIME(AReadyOrNotCharacter, bPrimed);
	DOREPLIFETIME(AReadyOrNotCharacter, bOverarmThrow);
	
	DOREPLIFETIME(AReadyOrNotCharacter, DamagedByCharacters);
	DOREPLIFETIME(AReadyOrNotCharacter, DamagedByWeapons);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bOrderedToRotateForArrest);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bHasBeenReported);

	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, LastKickedDoor, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bLowReadyPointDown, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bLowReadyPointUp, COND_SkipOwner);
	
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bMovementLocked, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bAimLocked, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bActionsLocked, COND_OwnerOnly);

	DOREPLIFETIME(AReadyOrNotCharacter, Rep_FPMesh);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bRepStunned);
	DOREPLIFETIME(AReadyOrNotCharacter, bHasEverBeenStunned);
	DOREPLIFETIME(AReadyOrNotCharacter, RepStunnedWith);

	DOREPLIFETIME(AReadyOrNotCharacter, PendingCarryCharacter);
	DOREPLIFETIME(AReadyOrNotCharacter, CarriedByCharacter);
	DOREPLIFETIME(AReadyOrNotCharacter, CurrentCarryCharacter);
	
	DOREPLIFETIME(AReadyOrNotCharacter, CurrentRagdollArrestConfirmTime);
	DOREPLIFETIME(AReadyOrNotCharacter, PendingRagdollArrestCharacter);
	DOREPLIFETIME(AReadyOrNotCharacter, CurrentRagdollArrestCharacter);

	DOREPLIFETIME(AReadyOrNotCharacter, Rep_CarryArrestedAnimState);
	
	DOREPLIFETIME(AReadyOrNotCharacter, Rep_ActiveRagdollPhysAsset);
	DOREPLIFETIME(AReadyOrNotCharacter, Rep_CharacterLookOverride);
	DOREPLIFETIME(AReadyOrNotCharacter, bCarryingDead);

	DOREPLIFETIME(AReadyOrNotCharacter, bIsCrouching);
	DOREPLIFETIME(AReadyOrNotCharacter, bIsStrafing);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bBodyHit);
	DOREPLIFETIME(AReadyOrNotCharacter, bLeftFootHit);
	DOREPLIFETIME(AReadyOrNotCharacter, bRightFootHit);
	DOREPLIFETIME(AReadyOrNotCharacter, bBlockedByBodyArmor);
	DOREPLIFETIME(AReadyOrNotCharacter, bBlockedByHeadArmor);

	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, ReplicatedAcceleration, COND_SimulatedOnly);
	DOREPLIFETIME(AReadyOrNotCharacter, ReplicatedMaxSpeed);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bIsPairedInteractionPlaying);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bArrestedAsRagdoll);
	DOREPLIFETIME(AReadyOrNotCharacter, bArrestedAsRagdoll_Flipped);
	
	DOREPLIFETIME(AReadyOrNotCharacter, PhysicalAnimationComp);
	
	DOREPLIFETIME(AReadyOrNotCharacter, bBlendInPhysics);
	DOREPLIFETIME(AReadyOrNotCharacter, bStartBlendInIncapacitation);
	DOREPLIFETIME(AReadyOrNotCharacter, bBlendInIncapacitation);
	DOREPLIFETIME(AReadyOrNotCharacter, IncapacitationBlendTime);
	DOREPLIFETIME(AReadyOrNotCharacter, IncapacitationBlendOutTime);
	DOREPLIFETIME(AReadyOrNotCharacter, IncapacitationLoopAnim);
	
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bFreeLeaning, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, bIsLeaning, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, QuickLeanAmount, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, FreeLeanX, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, FreeLeanZ, COND_SkipOwner);
	
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, QuickLeanIntensity, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AReadyOrNotCharacter, QuickLeanInterpSpeed, COND_SkipOwner);

	DOREPLIFETIME(AReadyOrNotCharacter, Customization);
}

void AReadyOrNotCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		RonCartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians = FMath::FloorToInt((AccelXYRadians / RON_TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}
}

AReadyOrNotCharacter::AReadyOrNotCharacter(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UReadyOrNotCharMovementComp>(CharacterMovementComponentName))
{
	CharacterHealth = CreateDefaultSubobject<UCharacterHealthComponent>(TEXT("CharacterHealthComponent"));
	CharacterHealth->SetIsReplicated(true);
	CharacterHealth->OnDepletedResource.AddDynamic(this, &AReadyOrNotCharacter::OnHealthDepleted);
	
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(41.f, 96.0f);
	SetRootComponent(GetCapsuleComponent());

	GetCapsuleComponent()->BodyInstance.bLockXRotation = true;
	GetCapsuleComponent()->BodyInstance.bLockYRotation = true;
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	// Allow Projectiles to hit the mesh, not the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);	
	GetCapsuleComponent()->bNavigationRelevant = false;
	
	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 2.0f;

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -95.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->bCastCapsuleDirectShadow = false;
	GetMesh()->bCastCapsuleIndirectShadow = true;
	GetMesh()->bLightAttachmentsAsGroup = true;
	GetMesh()->bAlwaysCreatePhysicsState = true;
	GetMesh()->SetReceivesDecals(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetNotifyRigidBodyCollision(true);
	GetMesh()->SetAllUseCCD(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	FaceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMesh"));
	if (FaceMesh)
	{
		FaceMesh->AlwaysLoadOnClient = true;
		FaceMesh->AlwaysLoadOnServer = true;
		FaceMesh->bOwnerNoSee = true;
		FaceMesh->bUseAttachParentBound = true;
		FaceMesh->bLightAttachmentsAsGroup = true;
		FaceMesh->bCastCapsuleDirectShadow = false;
		FaceMesh->bCastCapsuleIndirectShadow = true;
		FaceMesh->SetReceivesDecals(false);
		FaceMesh->SetupAttachment(GetMesh());
		FaceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	
	MeshGearSlot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshGearSlot"));
	if (MeshGearSlot)
	{
		MeshGearSlot->AlwaysLoadOnClient = true;
		MeshGearSlot->AlwaysLoadOnServer = true;
		MeshGearSlot->bOwnerNoSee = true;
		MeshGearSlot->bUseBoundsFromMasterPoseComponent = true;
		MeshGearSlot->bCastHiddenShadow = true;
		MeshGearSlot->SetReceivesDecals(false);
		MeshGearSlot->SetupAttachment(GetMesh());
		MeshGearSlot->SetMasterPoseComponent(GetMesh());
		MeshGearSlot->AddTickPrerequisiteComponent(GetMesh());
		MeshGearSlot->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}

	SkinnedDecalSampler = CreateDefaultSubobject<USkinnedDecalSampler>(TEXT("SkinnedDecalSampler"));
	SkinnedDecalSampler->MaxDecals = 128; // TODO(killo): could be adjustable for better/worse performance
	SkinnedDecalSampler->TranslucentBlend = false;
	SkinnedDecalSampler->SetMeshComponent(GetMesh(), false);
	SkinnedDecalSampler->SetMeshComponent(GetFaceMesh(), true);
	SkinnedDecalSampler->SetMeshComponent(GetMeshGearSlot(), true);

	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Interact"));
	InteractableComponent->ActionSlot2.bUseCustomActionText = true;
	InteractableComponent->ActionSlot2.CustomActionPromptText = FText::FromString("<Red>Cannot [Interaction Action] while [PlayerState]</>");
	InteractableComponent->SetupAttachment(GetMesh(), "spine_3");

	PlayerMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Player Marker Component"));
	PlayerMarkerComponent->bEnabled = false;
	PlayerMarkerComponent->bStartHidden = true;
	PlayerMarkerComponent->bDisplayMarkerText = true;
	PlayerMarkerComponent->bDistanceScaleIcon = true;
	PlayerMarkerComponent->IconBrush.SetResourceObject(nullptr);
	PlayerMarkerComponent->SetupAttachment(GetMesh(), "head_end");

	if (FMODVoiceLine2D == nullptr)
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FOMEvent1P(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_1P.Vl_1P'"));
        
		if(FOMEvent1P.Object != nullptr)
			FMODVoiceLine2D = FOMEvent1P.Object;
	}

	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));

	FMODAudioPropagationComp = CreateDefaultSubobject<UFMODAudioPropagationComponent>(TEXT("FMODAudioPropagationComponent"));
	FMODAudioPropagationComp->SetupAttachment(GetCapsuleComponent());

	FMODVoiceAudioComp = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("FMODAudioComp"));
	FMODVoiceAudioComp->SetupAttachment(GetMesh(), "head");
	FMODVoiceAudioComp->SetRelativeLocation(FVector::ZeroVector);
	FMODVoiceAudioComp->OnEventStopped.AddDynamic(this, &AReadyOrNotCharacter::OnVoiceAudioStopped);

	GibComponent = CreateDefaultSubobject<UGibComponent>(TEXT("GibComponent"));
	GibComponent->SetBodyMesh(GetMesh());
	GibComponent->SetFaceMesh(GetFaceMesh());

	RagdollComponent = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComponent"));
	RagdollComponent->PelvisBoneName = "pelvis";
	RagdollComponent->HeadBoneName = "head";
	RagdollComponent->bUseCapsuleCollision = true;
	RagdollComponent->OnRagdollStart.AddDynamic(this, &AReadyOrNotCharacter::OnRagdollStart);
	RagdollComponent->OnRagdollBlendStop.AddDynamic(this, &AReadyOrNotCharacter::OnRagdollBlendStop);
	RagdollComponent->OnRagdollPhysBodyHit.AddDynamic(this, &AReadyOrNotCharacter::OnRagdollPhysBodyHit);
	
	if (FMODVoiceLineSpatalized == nullptr)
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FOMEvent3P(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_3P.Vl_3P'"));
        
		if (FOMEvent3P.Object != nullptr)
			FMODVoiceLineSpatalized = FOMEvent3P.Object;
	}

	GetMesh()->bEnableUpdateRateOptimizations = false;
	GetFaceMesh()->bEnableUpdateRateOptimizations = false;

	HeadBones.Empty();
	HeadBones.Add("head");
	HeadBones.Add("head_end");
	HeadBones.Add("head_equipment");
	HeadBones.Add("neck_1");

	LowerBody.Add("Spine_1");
	LowerBody.Add("Spine_2");
	LowerBody.Add("pelvis");
	LowerBody.Add("Root");

	UpperBody.Add("Spine_3");
	UpperBody.Add("Chest");

	Torso += LowerBody;
	Torso += UpperBody;

	R_Arm.Add("R_Shoulder");
	R_Arm.Add("R_Arm");
	R_Arm.Add("R_Arm_Roll_1");
	R_Arm.Add("R_Arm_Roll_2");
	R_Arm.Add("R_ForeArm");
	R_Arm.Add("R_ForeArm_Roll_1");
	R_Arm.Add("R_ForeArm_Roll_2");
	R_Arm.Add("R_Hand");

	L_Arm.Add("L_Shoulder");
	L_Arm.Add("L_Arm");
	L_Arm.Add("L_Arm_Roll_1");
	L_Arm.Add("L_Arm_Roll_2");
	L_Arm.Add("L_ForeArm");
	L_Arm.Add("L_ForeArm_Roll_1");
	L_Arm.Add("L_ForeArm_Roll_2");
	L_Arm.Add("R_Hand");

	R_Leg.Add("R_UpLeg");
	R_Leg.Add("R_UpLeg_Roll_1");
	R_Leg.Add("R_UpLeg_Roll_2");
	R_Leg.Add("R_Leg");
	R_Leg.Add("R_Leg_Roll_1");

	L_Leg.Add("L_UpLeg");
	L_Leg.Add("L_UpLeg_Roll_1");
	L_Leg.Add("L_UpLeg_Roll_2");
	L_Leg.Add("L_Leg");
	L_Leg.Add("L_Leg_Roll_1");

	R_Foot.Add("R_Foot");
	R_Foot.Add("R_Foot_Toes");

	L_Foot.Add("L_Foot");
	L_Foot.Add("L_Foot_Toes");

	L_Hand.Add("thumb_1_LE");
	L_Hand.Add("thumb_2_LE");
	L_Hand.Add("thumb_3_LE");
	L_Hand.Add("thumb_4_end_LE");
	L_Hand.Add("index_1_LE");
	L_Hand.Add("index_2_LE");
	L_Hand.Add("index_3_LE");
	L_Hand.Add("index_4_LE");
	L_Hand.Add("index_5_end_LE");
	L_Hand.Add("middle_1_LE");
	L_Hand.Add("middle_2_LE");
	L_Hand.Add("middle_3_LE");
	L_Hand.Add("middle_4_LE");
	L_Hand.Add("middle_5_end_LE");
	L_Hand.Add("ring_1_LE");
	L_Hand.Add("ring_2_LE");
	L_Hand.Add("ring_3_LE");
	L_Hand.Add("ring_4_LE");
	L_Hand.Add("ring_5_end_LE");
	L_Hand.Add("pinky_1_LE");
	L_Hand.Add("pinky_2_LE");
	L_Hand.Add("pinky_3_LE");
	L_Hand.Add("pinky_4_LE");
	L_Hand.Add("pinky_5_end_LE");
	
	R_Hand.Add("thumb_1_RI");
	R_Hand.Add("thumb_2_RI");
	R_Hand.Add("thumb_3_RI");
	R_Hand.Add("thumb_4_end_RI");
	R_Hand.Add("index_1_RI");
	R_Hand.Add("index_2_RI");
	R_Hand.Add("index_3_RI");
	R_Hand.Add("index_4_RI");
	R_Hand.Add("index_5_end_RI");
	R_Hand.Add("middle_1_RI");
	R_Hand.Add("middle_2_RI");
	R_Hand.Add("middle_3_RI");
	R_Hand.Add("middle_4_RI");
	R_Hand.Add("middle_5_end_RI");
	R_Hand.Add("ring_1_RI");
	R_Hand.Add("ring_2_RI");
	R_Hand.Add("ring_3_RI");
	R_Hand.Add("ring_4_RI");
	R_Hand.Add("ring_5_end_RI");
	R_Hand.Add("pinky_1_RI");
	R_Hand.Add("pinky_2_RI");
	R_Hand.Add("pinky_3_RI");
	R_Hand.Add("pinky_4_RI");
	R_Hand.Add("pinky_5_end_RI");

	PhysicalAnimationComp = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("Physical Animation"));
	PhysicalAnimationComp->SetIsReplicated(true);

	CapsuleCollisionRagdollTriggerThreshold = 10; // reduced so its more sensitive
	CapsuleFloorAngleRagdollTriggerThreshold = 15.0f; // this seems to be a good mid-balance so smaller angled surfaces still work fine
	CapsuleFloorAngleRagdollDelayThreshold = 0.4f;
	Anim2RagdollPelvisWakeUpTime = 0.35f; // seems to work best across all motions

	GetCharacterMovement()->bAlwaysCheckFloor = false;

	Snapshots.Reserve(MaxSnapshots);
}

void AReadyOrNotCharacter::OnRep_Customization()
{
	if (!GetWorld())
		return;

	// Force local player to be Judge in non-networked games
	if (IsLocalPlayer() && !GetWorld()->GetNetDriver())
	{
		UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
		if (ItemData)
		{
			Customization.Character = ItemData->DefaultCustomization.Character;
			Customization.Voice = ItemData->DefaultCustomization.Voice;
		}
	}
	
	// Sanitize and apply customization
	// Customization should already be sanitized by the time we receive it though
	Customization.Sanitize();
	Customization.ApplyCustomization(this);
}

FName AReadyOrNotCharacter::GetSocketOverride(FName Socket)
{
	FName* SocketOverride = SocketOverridesMap.Find(Socket);
	if (SocketOverride)
		return *SocketOverride;

	return Socket;
}

void AReadyOrNotCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetAppropriatePhysicsAsset();

	BindAllDelegates();

	UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(this);

	AddTickPrerequisiteComponent(GetMesh());

	if (DefaultAlivePhysAsset)
	{
		GetMesh()->SetPhysicsAsset(DefaultAlivePhysAsset, false);
	}

	// register perception senses...
	PerceptionStimuliComp->RegisterForSense(UAISense_Damage::StaticClass());
	PerceptionStimuliComp->RegisterForSense(UAISense_Touch::StaticClass());
	PerceptionStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuliComp->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuliComp->RegisterWithPerceptionSystem();
	
	GetMesh()->SetNotifyRigidBodyCollision(true);
	GetMesh()->OnComponentHit.RemoveAll(this);
	GetMesh()->OnComponentHit.AddDynamic(this, &AReadyOrNotCharacter::OnMeshHit);

	GetCapsuleComponent()->SetWorldRotation(FRotator(0.0f, GetCapsuleComponent()->GetComponentRotation().Yaw, 0.0f));
	
	InitCollisionPreset();

	OriginalSpawnLocation = GetNavAgentLocation();

	UAchievementStatics::BeginMission(GetWorld());
}

void AReadyOrNotCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllReadyOrNotCharacters.Remove(this);
	}
	
	GetWorldTimerManager().ClearAllTimersForObject(this);
	
	// Not associated with this object so these wouldn't get cleared automatically
	GetWorldTimerManager().ClearTimer(TH_ArteryDeath);
	GetWorldTimerManager().ClearTimer(TH_DeathRattle);
	
	TArray<AActor*> OutActors;
	GetAttachedActors(OutActors);
	for (AActor* a : OutActors)
	{
		if (a)
		{
			a->Destroy();
		}
	}

	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this))
	{
		CrowdManager->UnregisterAgent(this);
	}

	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);

	if(VoiceSoundSource)
	{
		VoiceSoundSource->OnEventStopped.RemoveAll(this);
	}
	//FMODVoiceAudioComp->OnEventStopped.RemoveAll(this);
	
	Super::EndPlay(EndPlayReason);
}

void AReadyOrNotCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	SaveCharacterSnapshot();
	
	if (!GetWorld() || (GetWorld() && (GetWorld()->IsInSeamlessTravel() || GetWorld()->bIsTearingDown)))
	{
		if (PhysicalAnimationComp)
			PhysicalAnimationComp->SetSkeletalMeshComponent(nullptr);
	}
	
	SCOPE_CYCLE_COUNTER(STAT_RoNCharacterTick);

	#if !UE_BUILD_SHIPPING
	Tick_Debug(DeltaSeconds);
	#endif

	bPlayingDeathMontage = CurrentDeathMontage && Is3PMontagePlaying(CurrentDeathMontage);
	bStartedPlayingDeath = false; // Death anim should be started already, reset flag here
	
	GetMesh()->VisibilityBasedAnimTickOption = bIsRelevant ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	GetFaceMesh()->VisibilityBasedAnimTickOption = bIsRelevant ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	GetMeshGearSlot()->VisibilityBasedAnimTickOption = bIsRelevant ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	if (CustomizationFaceMesh)
	{
		CustomizationFaceMesh->VisibilityBasedAnimTickOption = bIsRelevant ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	}
	
	DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(GetActorRotation(), LastFrameRotation);
	LastFrameRotation = GetActorRotation();

	DeltaLocation = GetActorLocation() - LastFrameLocation;
	LastFrameLocation = GetActorLocation();

	TimeSinceLastStun += DeltaSeconds;
	TimeSinceLastBulletDamage += DeltaSeconds;
	TimeSinceLastYell += DeltaSeconds;
	TimeSinceLastTakenDamage += DeltaSeconds;
	
	TimeUntilNextYell = FMath::Clamp(TimeUntilNextYell - DeltaSeconds, 0.0f, TimeUntilNextYell);
	
	CivilianRespondCooldown -= DeltaSeconds;
	SpeakCooldown -= DeltaSeconds;
	
	if (bSurrendered)
	{
		TimeNotSurrendered = 0.0f;
		SurrenderedTime += DeltaSeconds;
	}
	else
	{
		TimeNotSurrendered += DeltaSeconds;
		SurrenderedTime = 0.0f;
	}

	if (IsDeadOrUnconscious())
	{
		TimeDeadOrUnconcious += DeltaSeconds;
		//TimeDowned = 0.0f;
	}
	else
	{
		TimeDeadOrUnconcious = 0.0f;
	}

	if (IsDeadNotUnconscious())
	{
		TimeDead += DeltaSeconds;
		//TimeDowned = 0.0f;
	}

	if (IsIncapacitated())
	{
		TimeIncapacitated += DeltaSeconds;
	}

	if (IsAnimationBlocking())
	{
		AnimationBlockingTime += DeltaSeconds;
	}
	else
	{
		AnimationBlockingTime = 0.0f;
	}

	if (!IsDeadOrUnconscious())
	{
		// Blend the facial animation
		if (CurrentEmotion != ECharacterEmotion::None)
		{
			if (FacialAnimationOverrideTime <= 0.0f)
			{
				// Override time finished, time to blend.
				FacialAnimationBlend -= FacialAnimationBlendDecay * DeltaSeconds;
				if (FacialAnimationBlend <= 0.0f)
				{
					// Blend complete, revert to normal emotion
					CurrentEmotion = ECharacterEmotion::None;
					FacialAnimationPriority = 0;
				}
			}
			else
			{
				FacialAnimationOverrideTime -= DeltaSeconds;
				FacialAnimationBlend += 2 * DeltaSeconds;
			}

			FacialAnimationBlend = FMath::Clamp(FacialAnimationBlend, 0.0f, 1.0f);
		}
	}

	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(this);
	if (!PlayerCharacter && IsInRagdoll() && IsIncapacitated() && bStartBlendInIncapacitation)
	{
		IncapacitationBlendTime = FMath::Max(IncapacitationBlendTime - DeltaSeconds, 0.0f);
		
		if (IncapacitationBlendTime <= 0.0f)
		{
			bBlendInIncapacitation = true;
			bStartBlendInIncapacitation = false;
			GetCharacterMovement()->DisableMovement();
			IncapacitationBlendOutTime = 4.0f;
			
			/*
			FPhysicalAnimationData NewData_Local;
			NewData_Local.bIsLocalSimulation = true; // use local simulation for ragdoll like results
			NewData_Local.OrientationStrength = 1000.0f; // some default settings to start with, has high influence from animation
			NewData_Local.AngularVelocityStrength = 1000.0f;
			NewData_Local.PositionStrength = 1000.0f;
			NewData_Local.VelocityStrength = 500.0f;
			NewData_Local.MaxAngularForce = 0.0f;
			NewData_Local.MaxLinearForce = 0.0f;
			
			FPhysicalAnimationData NewData_LocalArms;
			NewData_LocalArms.bIsLocalSimulation = true; // use local simulation for ragdoll like results
			NewData_LocalArms.OrientationStrength = 1000.0f; // some default settings to start with, has high influence from animation
			NewData_LocalArms.AngularVelocityStrength = 1000.0f;
			NewData_LocalArms.PositionStrength = 1000.0f;
			NewData_LocalArms.VelocityStrength = 1000.0f;
			NewData_LocalArms.MaxAngularForce = 100.0f;
			NewData_LocalArms.MaxLinearForce = 100.0f;
			
			FPhysicalAnimationData NewData_Global;
			NewData_Global.bIsLocalSimulation = false; // use local simulation for ragdoll like results
			NewData_Global.OrientationStrength = 10.0f; // some default settings to start with, has high influence from animation
			NewData_Global.AngularVelocityStrength = 10.0f;
			NewData_Global.PositionStrength = 10.0f;
			NewData_Global.VelocityStrength = 50.0f;
			NewData_Global.MaxAngularForce = 0.0f;
			NewData_Global.MaxLinearForce = 0.0f;

			// passing the new properties to physical anim comp
			PhysicalAnimationComp->SetSkeletalMeshComponent(GetMesh());
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("pelvis", NewData_Global, true);
			
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("spine_1", NewData_Local, true);
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("thigh_LE", NewData_Local, true);
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("thigh_RI", NewData_Local, true);
			
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("upperarm_LE", NewData_LocalArms, true);
			PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("upperarm_RI", NewData_LocalArms, true);
				
			// usual way called AFTER setting up the data for the physical animation component
			GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
			GetMesh()->SetAllBodiesBelowSimulatePhysics("foot_LE", false, false);
			GetMesh()->SetAllBodiesBelowSimulatePhysics("foot_RI", false, false);
			*/
		}
	}

	if (HasAuthority())
	{
		Tick_Authority(DeltaSeconds);
	}

	// PVP logic
	{
		// Note(Ali): Commented out until working on PVP
		/*if (IsDowned())
		{
			if (GetWorld()->GetGameState<AReadyOrNotGameState>()->MatchState == EMatchState::MS_Playing)
			{
				TimeDowned += DeltaSeconds;
				if (TimeDowned >= GetCurrentReviveTime())
				{
					Kill();

					TimeDowned = 0.0f;
				}
			}
		}
		else */if (IsDeadNotUnconscious())
		{
			//TimeDowned = 0.0f;

			TimeDead += DeltaSeconds;

			if (const AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>())
			{
				if (gs->bPvPMode)
				{
					if (TimeDead > 60.0f)
					{
						GetWorld()->DestroyActor(this);
						return;
					}
				}
			}
		}
		//else
		//{
			//TimeDowned = 0.0f;
		//}
	}

	// Player marker visibility logic
	{
		if (PlayerMarkerComponent)
		{
			SCOPE_CYCLE_COUNTER(STAT_RoNTick_PlayerMarker);

			TimeSinceLastPlayerMarkerUpdate += DeltaSeconds;
			if (TimeSinceLastPlayerMarkerUpdate > 0.5f)
			{
				TimeSinceLastPlayerMarkerUpdate = 0.0f;
			
				bool bShowPlayerNames = false;
				UBpGameplayHelperLib::LoadShowPlayerNamesSetting(bShowPlayerNames);

				bool bNeedsUpdating = false;
				if (bPreviousShowPlayerNames != bShowPlayerNames)
				{
					bNeedsUpdating = true;
				}

				if (bShowPlayerNames && UBpGameplayHelperLib::IsFriendlyWithMe(GetWorld()->GetGameState<AReadyOrNotGameState>(), GetTeam()))
				{
					PlayerMarkerComponent->EnableObjectiveMarker();
					PlayerMarkerComponent->ShowObjectiveMarker();
					
					if (bNeedsUpdating)
					{
						bPreviousShowPlayerNames = bShowPlayerNames;

						if (GetPlayerState())
						{
							const FLinearColor IconColor = FColor::FromHex("1C85F4").ReinterpretAsLinear();
			
							PlayerMarkerComponent->SetIconColor(FLinearColor::White);
							PlayerMarkerComponent->SetMarkerTextColor(FLinearColor(IconColor.R, IconColor.G, IconColor.B, 0.75f));
							PlayerMarkerComponent->SetMarkerTextFontSize(12);
							PlayerMarkerComponent->SetMarkerText(FText::FromString(GetPlayerState()->GetPlayerName()));
							PlayerMarkerComponent->ShowMarkerText();
						}
						else
						{
							// i love downcasting!!!
							if (const ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(this))
							{
								FLinearColor NameColor = GetTeam() == ETeamType::TT_SERT_RED ? FLinearColor(1.0f, 0.099356f, 0.039631, 0.7f) : FLinearColor(0.25f, 0.28f, 1.0f, 0.7f);
								
								if (GetTeam() != ETeamType::TT_SERT_RED && GetTeam() != ETeamType::TT_SERT_BLUE)
									NameColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.65f);

								PlayerMarkerComponent->SetIconColor(FLinearColor::White);
								PlayerMarkerComponent->SetMarkerTextFontSize(11);
								PlayerMarkerComponent->SetMarkerTextColor(NameColor);
								PlayerMarkerComponent->SetMarkerText(SwatCharacter->GetSwatCharacterName());
								PlayerMarkerComponent->ShowMarkerText();
							}
						}
					}
				}
				else
				{
					PlayerMarkerComponent->HideObjectiveMarker();
					
					bPreviousShowPlayerNames = false;
				}
			}
		}
	}

	// Carry Logic
	{
		if (!UInteractionsData::IsPairedInteractionPlayingOn(this) && !bCarryingDead) // ignore if carrying a dead person
		{
			if (IsCarried())
			{
				if (CarriedByCharacter->IsDeadNotUnconscious() || CarriedByCharacter->IsIncapacitated() || CarriedByCharacter->IsInRagdoll())
				{
					OnCarryThrowComplete(CarriedByCharacter, this);
				}
			}

			if (IsCarrying())
			{
				if (CurrentCarryCharacter->IsDeadNotUnconscious() ||
					CurrentCarryCharacter->IsIncapacitated() ||
					CurrentCarryCharacter->IsInRagdoll())
				{
					OnCarryDropComplete(this, CurrentCarryCharacter);
				}
			}
			else
			{
				/*
				if (FakeCarryCharacterMesh)
				{
					GetWorld()->DestroyActor(FakeCarryCharacterMesh);
					FakeCarryCharacterMesh = nullptr;
				}
				*/
			}
		}
	}

	// Player interaction logic
	if (InteractableComponent)
	{
		SCOPE_CYCLE_COUNTER(STAT_RoNTick_PlayerInteraction);

		InteractableComponent->bEnabled = false;

		LOCAL_PLAYER;
		if (LocalPlayer && !bDisableInteraction)
		{
			if (LocalPlayer != this)
			{
				// ActionSlot1 = Use slot (Press F to [Interaction Action])
				// ActionSlot2 = Cannot [Interaction Action] while [Reason]
				// ActionSlot3 = Progress status

				const FName AnimatedIconName = DetermineAnimatedIcon_Implementation();

				InteractableComponent->bEnabled = true;
				InteractableComponent->AnimatedIconName = AnimatedIconName;
				InteractableComponent->ShowPromptAtDistance = DetermineInteractionDistance_Implementation();
				InteractableComponent->CurrentProgress = DetermineCurrentProgress_Implementation();
				InteractableComponent->ActionSlot1.bCheckForDisallowedItems = (((CanArrest() || CanArrestRagdoll()) && IsInPositionForArrest(LocalPlayer)) || CanBePickedUp());
				InteractableComponent->ActionSlot2.bCheckForDisallowedItems = false;
				InteractableComponent->ActionSlot1.InputEvent = DetermineInputEvent_Implementation();
				InteractableComponent->ActionSlot1.ActionText = DetermineActionText_Implementation();
				InteractableComponent->ActionSlot1.bCondition = CanInteract_Implementation();
				InteractableComponent->ActionSlot3.bAnimate = true;
				InteractableComponent->ActionSlot3.bLoopAnimation = true;
				InteractableComponent->ActionSlot3.bCondition = false;
				InteractableComponent->bImprintIconOnHUDUponInteraction = ShouldImprintIconOnHUD();
				InteractableComponent->SetInteractionIconState(InteractableComponent->ActionSlot1.bCondition);

				// Tick picking up character progress
				{
					if (LocalPlayer->PendingCarryCharacter == this && MaxCarryConfirmTime > 0.0f && !UInteractionsData::IsPairedInteractionPlayingOn(this))
					{
						CurrentCarryConfirmTime += DeltaSeconds;

						InteractableComponent->ActionSlot3.bUseCustomActionText = true;
						InteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "CarryingArrested");
						InteractableComponent->ActionSlot3.bCondition = true;
						InteractableComponent->ActionSlot1.bCondition = false;

						if (CurrentCarryConfirmTime >= MaxCarryConfirmTime)
						{
							LocalPlayer->Server_CarryArrestedTarget(this);
							CurrentCarryConfirmTime = 0.0f;
						}
					}
					else
					{
						if (PendingCarryCharacter)
						{
							if (LocalPlayer->GetPlayerState())
							{
								InteractableComponent->AnimatedIconName = "Empty";
								InteractableComponent->ActionSlot1.bCondition = false;
								InteractableComponent->ActionSlot3.bCondition = PendingCarryCharacter != LocalPlayer;
								InteractableComponent->ActionSlot3.bLoopAnimation = false;
								InteractableComponent->ActionSlot3.CustomActionPromptText = FText::Format(FText::FromStringTable("ActionPromptTable", "BeingCarriedBy"), FText::FromString(LocalPlayer->GetPlayerState()->GetPlayerName()));

								InteractableComponent->SetInteractionIconState(InteractableComponent->ActionSlot1.bCondition);
							}
						}
						else
						{
							CurrentCarryConfirmTime = 0.0f;
						}
					}
				}

				// Tick arresting ragdolled character progress
				{
					if (!IsOnSWATTeam() && IsInRagdoll())
					{
						if (LocalPlayer->PendingRagdollArrestCharacter == this && MaxRagdollArrestConfirmTime > 0.0f && !UInteractionsData::IsPairedInteractionPlayingOn(this))
						{
							CurrentRagdollArrestConfirmTime += DeltaSeconds;

							InteractableComponent->AnimatedIconName = "Empty";
							InteractableComponent->ActionSlot3.bUseCustomActionText = true;
							InteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "Restraining");
							InteractableComponent->ActionSlot3.bCondition = true;
							InteractableComponent->ActionSlot1.bCondition = false;

							if (CurrentRagdollArrestConfirmTime >= MaxRagdollArrestConfirmTime)
							{
								LocalPlayer->RagdollArrestTarget(this);
								CurrentRagdollArrestConfirmTime = 0.0f;
							}
						}
						else
						{
							if (PendingRagdollArrestCharacter)
							{
								if (LocalPlayer->GetPlayerState())
								{
									InteractableComponent->AnimatedIconName = "Empty";
									InteractableComponent->ActionSlot1.bCondition = false;
									InteractableComponent->ActionSlot3.bCondition = PendingRagdollArrestCharacter != LocalPlayer;
									InteractableComponent->ActionSlot3.bLoopAnimation = false;
									
									FText ArrestingText = FText::FromStringTable("ActionPromptTable", "BeingArrestedBy");
									InteractableComponent->ActionSlot3.CustomActionPromptText = FText::Format(ArrestingText, FText::FromString(LocalPlayer->GetPlayerState()->GetPlayerName()));
									

									InteractableComponent->SetInteractionIconState(InteractableComponent->ActionSlot1.bCondition);
								}
							}
							else
							{
								CurrentRagdollArrestConfirmTime = 0.0f;
							}
						}
					}
				}
			}
		}
	}

	/* this does not work 25/01/23
	if(DefaultRagdollPhysAsset)
		SetPhysicsAssetAngularMotorAnimInfluence(DefaultRagdollPhysAsset, 50000.0f, 0.0f, 0.0f, false);
	if (DefaultAlivePhysAsset)
		SetPhysicsAssetAngularMotorAnimInfluence(DefaultAlivePhysAsset, 50000.0f, 0.0f, 0.0f, false);
	if (CuffedRagdollPhysAsset)
		SetPhysicsAssetAngularMotorAnimInfluence(CuffedRagdollPhysAsset, 50000.0f, 0.0f, 0.0f, false);
	*/

	//
	// UpdateCapsuleFloorAngleRagdollTrigger(DeltaSeconds); Alex: the ragdoll manager is supposed to trigger ragdoll !
	//UpdateAnim2RagdollBlend(DeltaSeconds); /* Alex: the ragdoll manager is supposed to trigger ragdoll !
	
	if (bFreeLeaning)
	{
		CalculateLeanMovement(DeltaSeconds);

		if (IsValid(LeanAudioComponent) && IsValid(LeanAudioEvent))
		{
			LeanAudioComponent->SetParameter("LeanSpeed", LeanMovementValue);
		} 
	}
	
	// this boolean should only trigger a full lean where the character shifts his weight and therefore makes him stationary
	// quick lean is triggered by checking the QuickLeanAmount value and gets layered on top of movement
	bLeaningLeft = /*QuickLeanAmount < 0.0f ||*/ FreeLeanX < 0.0f;
	bLeaningRight = /*QuickLeanAmount > 0.0f ||*/ FreeLeanX > 0.0f;

	// Cache is outside
	{
		const FVector StartTrace = GetActorLocation();
		const FVector EndTrace = StartTrace + GetActorUpVector() * 5000;

		if (GetWorld()->IsTraceHandleValid(IsOutsideTraceHandle, false))
		{
			FTraceDatum OutData;
			if (GetWorld()->QueryTraceData(IsOutsideTraceHandle, OutData))
			{
				bCachedIsOutside = OutData.OutHits.Num() == 0;
				
				for (const FHitResult& Hit : OutData.OutHits)
				{
					if (const AActor* Actor = Hit.GetActor())
					{
						if (Actor->Tags.Contains("IsOutside"))
						{
							bCachedIsOutside = true;
							break;
						}
					}
				}
			}
		}
		else
		{
			IsOutsideTraceHandle = GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, StartTrace, EndTrace, ECC_SOUND, GetCollisionQueryParameters());
		}
	}
}

void AReadyOrNotCharacter::Tick_Authority(const float DeltaSeconds)
{
	bIsPairedInteractionPlaying = UInteractionsData::IsPairedInteractionPlayingOn(this) != nullptr;
	
	if (IsDeadNotUnconscious() && !bHasRunDeathLogic)
	{
		bHasRunDeathLogic = true;
		
		LastDeathPointDamageEvent = (FPointDamageEvent*)&LastDamageEvent;
		LastDeathRadialDamageEvent = (FRadialDamageEvent*)&LastDamageEvent;

		if (LastDamageEvent.Instigator)
			KilledBy = Cast<AReadyOrNotCharacter>(LastDamageEvent.Instigator->GetPawn());
		
		if (LastDamageEvent.DamageEvent.DamageTypeClass.Get())
		{
			bBleedoutDeath = LastDamageEvent.DamageEvent.DamageTypeClass.Get() == UBleedDamageType::StaticClass();
		}

		if (LastDamageEvent.DamageEvent.DamageTypeClass)
		{
			if (const UStunDamage* StunDamage = Cast<UStunDamage>(LastDamageEvent.DamageEvent.DamageTypeClass->GetDefaultObject()))
			{
				if (StunDamage->bNonLethal)
				{
					bNonLethalDeath = true;
				}
			}
		}
		
		if (AActor* DamageCauser = LastDamageEvent.Causer)
		{
			if (DeathReason == ECharacterDeathReason::None)
			{
				if (const ABaseMagazineWeapon* BMW = Cast<ABaseMagazineWeapon>(DamageCauser))
				{
					if (BMW->ItemCategories.Contains(EItemCategory::IC_Primary))
						DeathReason = ECharacterDeathReason::PrimaryWeapon; 
					else if (BMW->ItemCategories.Contains(EItemCategory::IC_Secondary))
						DeathReason = ECharacterDeathReason::SecondaryWeapon;
				}
				else if (Cast<ABaseGrenade>(DamageCauser))
				{
					DeathReason = ECharacterDeathReason::Grenade;
				}
				else if (Cast<ATaser>(DamageCauser))
				{
					DeathReason = ECharacterDeathReason::TasedToDeath;
				}
				else if ((LastDamageEvent.Instigator && LastDamageEvent.Instigator->GetPawn() == this) || DamageCauser == this)
				{
					DeathReason = ECharacterDeathReason::Suicide;
				}

				if (bBleedoutDeath)
					DeathReason = ECharacterDeathReason::Bleedout;
			}
		}
		
		PlayDeathAnimation();

		OnKilled(KilledBy);
		Multicast_OnKilled(LastHitBoneName, LastDamageEvent.Causer);
	}

	if (IsIncapacitated() && !bHasRunIncapLogic)
	{
		bHasRunIncapLogic = true;
		
		if (LastDamageEvent.Instigator)
			IncapacitatedBy = Cast<AReadyOrNotCharacter>(LastDamageEvent.Instigator->GetPawn());
		
		PlayDeathAnimation();

		OnIncapacitated(LastDamageEvent.Instigator ? Cast<AReadyOrNotCharacter>(LastDamageEvent.Instigator->GetPawn()) : nullptr);
		Multicast_OnIncapacitated(LastHitBoneName);
	}

	// failsafe
	if (!IsCarried())
	{
		if (IsDeadOrUnconscious() || IsIncapacitated())
		{
			if (!bIsPairedInteractionPlaying && !bStartedPlayingDeath && !bPlayingDeathMontage && (bHasRunDeathLogic || bHasRunIncapLogic))
			{
				if (!IsInRagdoll())
				{
					EnableRagdoll();
				}
			}
		}
	}
	
	bIsStrafing = bServerIsStrafing;
	ReplicatedControlRotation = GetControlRotation();
		
	// the fp mesh camera logic doesn't let the camera go below -72 degrees (but the control rotation exceeds this) so we'll just clamp this value
	ReplicatedControlRotation.Pitch = FMath::Max(ReplicatedControlRotation.GetNormalized().Pitch, -72.0f);

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		ReplicatedMaxSpeed = MovementComponent->GetMaxSpeed();
	}

	// Decay gas accumulation
	GasDamageAccumulated = FMath::Clamp(GasDamageAccumulated * DeltaSeconds * GasDamageDecay, 0.0f, GasDamageAccumulated);
	
	// Tick bone suppression values
	{
		TArray<FName> BoneNamesToRemove;
		for (auto& It : BoneSuppressionAmount)
		{
			It.Value.Strength = FMath::Max(It.Value.Strength - DeltaSeconds, 0.0f);
			if (It.Value.Strength <= 0.0f)
			{
				BoneNamesToRemove.Add(It.Key);
			}
		}
	
		for (const FName& Bone : BoneNamesToRemove)
			BoneSuppressionAmount.Remove(Bone);
	}

	bRepStunned = IsStunned();
	if (bRepStunned)
	{
		TimeSinceLastStun = 0.0f;

		for (const auto& elem : StunMap)
		{
			if (GetWorld()->GetTimerManager().IsTimerActive(elem.Key))
			{
				CurrentStunTime = GetWorld()->GetTimerManager().GetTimerElapsed(elem.Key);
				RepStunnedWith = elem.Value;
			}
		}
	}

	// Tick speech cooldown
	{
		TArray<FString> SpeechKeysToRemove;
		for (auto& Item : SpeechCooldownMap)
		{
			Item.Value -= DeltaSeconds;
			
			if (Item.Value <= 0.0f)
			{
				SpeechKeysToRemove.Add(Item.Key);
			}
		}

		for (int32 i = 0; i < SpeechKeysToRemove.Num(); i++)
		{
			SpeechCooldownMap.Remove(SpeechKeysToRemove[i]);
		}
	}

	{
		if (!VoiceSoundSource || !VoiceSoundSource->bIsActive)
		{
			if (!TOCResponseLine.IsEmpty())
			{
				Server_PlayTOCConversation();
			}
		}
	}

	if (!bIsPreviewCharacter && !Tags.Contains("Personalization"))
	{
		if (!IsCarried() && !IsDeadOrUnconscious() && !GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Z < 0.0f)
		{
			AirTime += DeltaSeconds;
			LastZVelocityInAir = GetVelocity().Z;
		}
		else
		{
			// Apply fall damage
			if (AirTime > AirTimeBeforeTakingDamage && (LastZVelocityInAir < -850 || IsInRagdoll()))
			{
				float FallDamage;
				if (ThrownByCharacter)
				{
					FallDamage = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 2.5f), FVector2D(0.0f, GetMaxHealth()), AirTime);
				}
				else
				{
					FallDamage = FMath::Abs(LastZVelocityInAir) * (0.01 * AirTime) * 2.0f;
				}

				if (FallDamage > 0.0f)
				{
					// Is about to die from fall damage?
					if (GetCurrentHealth() - FallDamage <= 0.0f)
						DeathReason = ECharacterDeathReason::FellFromHighHeight;

					// Gradually injure legs
					const int32 TicketsToRemove = FMath::CeilToInt(AirTime*2);
					CharacterHealth->DecreaseLimbTickets(ELimbType::LT_LeftLeg, TicketsToRemove);
					CharacterHealth->DecreaseLimbTickets(ELimbType::LT_RightLeg, TicketsToRemove);
					OnBodyPartDamaged.Broadcast(false, false, false, false, true, true, false, false);

					AController* EventInstigator = ThrownByCharacter ? ThrownByCharacter->GetController() : GetController();
					TakeDamage(FallDamage, FDamageEvent(UDamageType::StaticClass()), EventInstigator, this);

					#if WITH_EDITOR
					ULog::Info(GetName() + " | Took fall damage: " + FString::SanitizeFloat(FallDamage));
					#endif
				}
			}
			
			AirTime = 0.0f;
		}

		// Probabaly falling into the void, forever
		if (AirTime > 15.0f)
		{
			if (!IsDeadNotUnconscious())
			{
				AController* EventInstigator = ThrownByCharacter ? ThrownByCharacter->GetController() : GetController();

				TakeDamage(9999.0f, FDamageEvent(UDamageType::StaticClass()), EventInstigator, this);
			}
		}
	}
}

FVector AReadyOrNotCharacter::GetTargetLocation(AActor* RequestedBy) const
{
	if (GetMesh())
	{
		return GetMesh()->GetBoneLocation("head");
	}

	return Super::GetTargetLocation(RequestedBy);
}

#if !UE_BUILD_SHIPPING
void AReadyOrNotCharacter::Tick_Debug(float DeltaSeconds)
{	
	/*
	EMovementMode MovementMode = GetCharacterMovement()->MovementMode;
	ULog::Info(RON_ENUM_TO_STRING(EMovementMode, MovementMode));
	*/

	if (Blood && !IsDeadNotUnconscious() && !IsOnSWATTeam() && CVarRonShowArteryZones.GetValueOnGameThread() != 0)
	{
		for (const FArteryData& Artery : Blood->Arteries)
		{
			FVector Location = GetMesh()->GetBoneLocation(Artery.BoneName);

			if (Artery.ZoneOffset != 0.0f)
				Location += GetMesh()->GetBoneQuaternion(Artery.BoneName).Vector() * Artery.ZoneOffset;
		
			DrawDebugSphere(GetWorld(), Location, Artery.ZoneSize, 8, FColor::Yellow, false, -1, 10);
		}
	}
}
#endif

float AReadyOrNotCharacter::GetCurrentHealth() const
{
	return CharacterHealth ? CharacterHealth->GetCurrentResource() : -1.0f; 
}

float AReadyOrNotCharacter::GetMaxHealth() const
{
	return CharacterHealth ? CharacterHealth->GetMaxResource() : -1.0f; 
}

float AReadyOrNotCharacter::GetCurrentReviveHealth() const
{
	return CharacterHealth ? CharacterHealth->GetRemainingReviveHealth() : -1.0f; 
}

float AReadyOrNotCharacter::GetCurrentReviveTime() const
{
	return CharacterHealth ? CharacterHealth->GetRemainingReviveTime() : -1.0f; 
}

bool AReadyOrNotCharacter::IsFullHealth() const
{
	return CharacterHealth ? CharacterHealth->IsFullResource() : false; 
}

bool AReadyOrNotCharacter::IsLowHealth() const
{
	return CharacterHealth ? CharacterHealth->IsLowResource() : false; 
}

bool AReadyOrNotCharacter::IsHalfHealth() const
{
	return CharacterHealth ? CharacterHealth->IsHalfResource() : false; 
}

bool AReadyOrNotCharacter::IsHealthDepleted() const
{
	return CharacterHealth ? CharacterHealth->IsDepleted() : false; 
}

bool AReadyOrNotCharacter::IsReviveHealthDepleted() const
{
	return CharacterHealth ? CharacterHealth->IsReviveHealthDepleted() : false; 
}

bool AReadyOrNotCharacter::IsDowned() const
{
	return UsingReviveSystem() && IsHealthDepleted() && !IsReviveHealthDepleted() && CharacterHealth->GetRemainingRevives() > 0;
}

bool AReadyOrNotCharacter::UsingReviveSystem() const
{
	return CharacterHealth ? CharacterHealth->CanUseReviveSystem() : false;
}

void AReadyOrNotCharacter::IncreaseHealth(const float Amount)
{
	if (!CharacterHealth)
		return;
	
	if (UsingReviveSystem())
	{
		if (IsHealthDepleted())
		{
			CharacterHealth->IncreaseReviveHealth(Amount);
		}
		else
		{
			CharacterHealth->IncreaseResource(Amount);
		}
	}
	else
	{
		CharacterHealth->IncreaseResource(Amount);
	}
}

void AReadyOrNotCharacter::DecreaseHealth(const float Amount)
{
	if (!CharacterHealth)
		return;
	
	if (UsingReviveSystem())
	{
		if (IsHealthDepleted())
		{
			CharacterHealth->DecreaseReviveHealth(Amount);
		}
		else
		{
			CharacterHealth->DecreaseResource(Amount);
		}
	}
	else
	{
		CharacterHealth->DecreaseResource(Amount);
	}

	// Don't wanna play VO in this case, as gas damage is applied twice a second atm. Fine when character is alive, as they'll be stunned and so won't play, but incapacitated characters can't get stunned
	const bool bTookGasDamageWhileIncapacitated = IsIncapacitated() && IsValid(LastDamageEvent.DamageEvent.DamageTypeClass) && LastDamageEvent.DamageEvent.DamageTypeClass->IsChildOf(UCSGasDamageType::StaticClass());
	
	if (!IsPlayerControlled() && !IsStunned() && !IsDeadNotUnconscious() && !bTookGasDamageWhileIncapacitated)
		PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_PAIN, "", false);

	if (IsIncapacitated() && !bHasBroadcastIncapEvent)
	{
		AReadyOrNotCharacter* InstigatorCharacter = nullptr;
		if (IsValid(LastDamageEvent.Instigator))
			InstigatorCharacter = Cast<AReadyOrNotCharacter>(LastDamageEvent.Instigator->GetPawn());
		
		bHasBroadcastIncapEvent = true;
		OnCharacterIncapacitated.Broadcast(this, InstigatorCharacter);
		OnCharacterIncapacitated.Clear();
	}
	
	// Make 'em wince.
	Multicast_ChangeFaceEmotion(ECharacterEmotion::Wince, 5.0f, 1.0f, 0.25f, 150);
}

void AReadyOrNotCharacter::DepleteHealth()
{
	if (CharacterHealth)
	{
		CharacterHealth->DepleteResource();
	}
}

void AReadyOrNotCharacter::ResetHealth()
{
	if (CharacterHealth)
	{
		CharacterHealth->ResetResource();
		CharacterHealth->ResetReviveHealth();
	}
}

bool AReadyOrNotCharacter::AnyBodyPartHit() const
{
	return IsAnyLimbHit() || bBodyHit || bLeftFootHit || bRightFootHit;
}

bool AReadyOrNotCharacter::IsAnyLimbHit() const
{
	return IsLimbHit(ELimbType::LT_Head) || IsLimbHit(ELimbType::LT_LeftArm) || IsLimbHit(ELimbType::LT_RightArm) || IsLimbHit(ELimbType::LT_LeftLeg) || IsLimbHit(ELimbType::LT_RightLeg);
}

bool AReadyOrNotCharacter::IsLimbHit(const ELimbType Limb) const
{
	return !GetLimbHealth(Limb).HasMaxTickets();
}

bool AReadyOrNotCharacter::IsLimbBroken(const ELimbType Limb) const
{
	return CharacterHealth ? CharacterHealth->IsLimbBroken(Limb) : false;
}

FLimbHealthData AReadyOrNotCharacter::GetLimbHealth(const ELimbType Limb) const
{
	return CharacterHealth ? CharacterHealth->GetLimb_Const(Limb) : FLimbHealthData::Invalid;
}

bool AReadyOrNotCharacter::IsActive() const
{
	if (!GetController())
		return false;
	
	if (IsDeadOrUnconscious())
		return false;
	
	if (IsIncapacitated())
		return false;
	
	if (IsArrestedOrSurrendered())
		return false;

	if (bIsBeingArrested)
		return false;

	if (IsCarried())
		return false;

	if (IsInRagdoll())
		return false;
	
	if (bStartedPlayingDeath || bPlayingDeathMontage)
		return false;
	
	return true;
}

bool AReadyOrNotCharacter::IsActiveForMovement() const
{
	if (!GetController())
		return false;
	
	if (IsDeadOrUnconscious())
		return false;
	
	if (IsIncapacitated())
		return false;
	
	if (bIsBeingArrested)
		return false;

	if (IsCarried())
		return false;

	if (IsInRagdoll())
		return false;

	if (bStartedPlayingDeath || bPlayingDeathMontage)
		return false;

	if (bFreeLeaning)
		return false;
	
	if (bIsPairedInteractionPlaying)
		return false;

	if (IsStartling())
		return false;
	
	return true;
}

bool AReadyOrNotCharacter::IsActiveForVO() const
{
	if (!IsDeadOrUnconscious())
		return true;
	
	return !IsInRagdoll();
}

bool AReadyOrNotCharacter::IsDeadOrUnconscious() const
{
	return IsDeadNotUnconscious() || IsUnconsciousNotDead();
}

bool AReadyOrNotCharacter::IsInjured() const
{
	return CharacterHealth ? CharacterHealth->GetHealthStatus() > EPlayerHealthStatus::HS_Healthy : false;
}

bool AReadyOrNotCharacter::IsIncapacitated() const
{
	return CharacterHealth ? CharacterHealth->IsIncapacitated() : false;
}

EPlayerHealthStatus AReadyOrNotCharacter::GetHealthStatus() const
{
	return CharacterHealth ? CharacterHealth->GetHealthStatus() : EPlayerHealthStatus::HS_NotAvailable;
}

bool AReadyOrNotCharacter::IsDeadNotUnconscious() const
{
	return (UsingReviveSystem() && IsHealthDepleted() && IsReviveHealthDepleted()) || IsHealthDepleted();
}

bool AReadyOrNotCharacter::IsUnconsciousNotDead() const
{
	return false;
}

void AReadyOrNotCharacter::Kill()
{
	Server_Kill();
}

void AReadyOrNotCharacter::Incapacitate()
{
	Server_Incapacitate();
}

void AReadyOrNotCharacter::Server_Incapacitate_Implementation()
{
	if (IsIncapacitated() || bHasRunIncapLogic)
		return;

	DecreaseHealth(GetCurrentHealth() - 1.0f);
}

void AReadyOrNotCharacter::Server_Kill_Implementation()
{
	if (IsDeadNotUnconscious() || bHasRunDeathLogic)
		return;
	
	bHasRunDeathLogic = true;
	bBlendInIncapacitation = false;
	
	KilledBy = this;
	
	DepleteHealth();

	LastDeathPointDamageEvent = nullptr;
	LastDeathRadialDamageEvent = nullptr;
	
	PlayDeathAnimation();

	OnKilled(this);
	Multicast_OnKilled(NAME_None, this);
	
	OnCharacterKilled.Broadcast(this, this);
	OnCharacterKilled.Clear();
}

void AReadyOrNotCharacter::OnKilled(AReadyOrNotCharacter* InstigatorCharacter)
{
}

void AReadyOrNotCharacter::OnIncapacitated(AReadyOrNotCharacter* InstigatorCharacter)
{
}

void AReadyOrNotCharacter::Multicast_OnExplosiveVestDetonation_Implementation()
{
	if (!Blood)
		return;

	bShouldSpawnBloodPool = false;

	if (Blood->Gibs && GetMesh())
	{
		AExplosionGibs* Gibs = GetWorld()->SpawnActor<AExplosionGibs>(Blood->Gibs);
		Gibs->SetupGibsForSkeletalMesh(GetMesh());

		// Hide our ragdoll
		GetMesh()->SetHiddenInGame(true, true);

		if (MeshGearSlot)
			MeshGearSlot->SetCastShadow(false);

		// Ragdoll is invisible, disable sim and collision
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetSimulatePhysics(false);

		// No speaking if you are in bits and pieces
		RemoveVocalChords();

		// Set our interactable component by our bloodsplat so we can still be reported
		if (InteractableComponent)
		{
			InteractableComponent->SetHiddenInGame(false, true);
			InteractableComponent->SetUsingAbsoluteLocation(true);
			InteractableComponent->SetWorldLocation(GetActorLocation() + FVector::UpVector * -20.0f, false, nullptr, ETeleportType::None);
		}

		bIsInBitsAndPieces = true;
	}

	// Big blood splatter
	if (Blood->BigSplatterDecals.Num() > 0)
	{
		const FVector StartTrace = GetActorLocation();
		const FVector EndTrace = StartTrace + FVector::DownVector * Blood->BigSplatterTraceDistance;

		FCollisionObjectQueryParams CollisionObjectQuery;
		CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);

		const FCollisionQueryParams CollisionParams = GetCollisionQueryParameters();

		TArray<FHitResult> HitResults;
		GetWorld()->LineTraceMultiByObjectType(HitResults, StartTrace, EndTrace, CollisionObjectQuery, CollisionParams);

		for (const FHitResult& Hit : HitResults)
		{
			if (Cast<AReadyOrNotCharacter>(Hit.GetActor()))
				continue;

			FRotator Rotation = (-Hit.ImpactNormal).Rotation();
			Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

			UMaterialInstance* Material = Blood->BigSplatterDecals[FMath::RandRange(0, Blood->BigSplatterDecals.Num() - 1)];
			UGameplayStatics::SpawnDecalAttached(Material, Blood->BigSplatterDecalSize, Hit.GetComponent(), Hit.BoneName, Hit.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition);

			break;
		}
	}
}

void AReadyOrNotCharacter::OnRep_ControlRotation()
{
	ReplicatedControlRotation.Normalize();
}

void AReadyOrNotCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InventoryComp->OnItemEquipped.RemoveAll(this);
	InventoryComp->OnItemEquipped.AddDynamic(this, &AReadyOrNotCharacter::OnItemEquipped);
	
	InventoryComp->OnItemHolstered.RemoveAll(this);
	InventoryComp->OnItemHolstered.AddDynamic(this, &AReadyOrNotCharacter::OnItemHolstered);

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllReadyOrNotCharacters.AddUnique(this);
	}
}

bool AReadyOrNotCharacter::IsPlayingRootMotionFromMontage() const
{
	if (GetMesh())
	{
		if (GetMesh()->GetAnimInstance())
		{
			return GetMesh()->GetAnimInstance()->Montage_IsPlaying(nullptr) && GetMesh()->IsPlayingRootMotion();
		}
	}
	
	return false;
}

void AReadyOrNotCharacter::Multicast_TakeDamage_Implementation(const float Damage, const FDamageEvent& DamageEvent, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser)
{
	if (IsDeadNotUnconscious())
	{
		//OnCharacterKilled.Broadcast(InstigatorCharacter, this);
		//OnCharacterKilled.Clear();
	}
	else
	{
		if (DamageEvent.DamageTypeClass)
		{
			if (UStunDamage* StunDamage = Cast<UStunDamage>(DamageEvent.DamageTypeClass->GetDefaultObject()))
			{
				OnStunDamageReceived.Broadcast(Damage, DamageCauser, InstigatorCharacter, this, StunDamage);
			}

			if (UBulletDamageType* BulletDamage = Cast<UBulletDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()))
			{
				OnPointDamageReceived.Broadcast(Damage, DamageCauser, InstigatorCharacter, this, BulletDamage);
			}

			const UBleedDamageType* BleedOutDamage = Cast<UBleedDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			const UCSGasDamageType* CSGasDamage = Cast<UCSGasDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			
			if (HasAuthority() && !BleedOutDamage && !CSGasDamage)
			{
				RespondToBleedOutDamage();
			}
		}

		OnCharacterTakeDamage.Broadcast(InstigatorCharacter, this, DamageCauser, Damage, GetCurrentHealth());
	}	
}

void AReadyOrNotCharacter::RespondToBleedOutDamage()
{
}

void AReadyOrNotCharacter::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	if (IsArrested() || IsDeadOrUnconscious() || IsIncapacitated())
	{
		if (!bHasBeenReported)
		{
			InteractInstigator->Server_ReportTarget_Implementation(this);
			return;
		}

		if (CanBePickedUp())
		{
			if (IsInPositionForCarry(InteractInstigator))
			{
				if (!PendingCarryCharacter)
				{
					PendingCarryCharacter = InteractInstigator;
					InteractInstigator->PendingCarryCharacter = this;
					CurrentCarryConfirmTime = 0.0f;
						
					// If we're not a player or if max pickup time is 0,
					// instantly start picking up this character.
					// AI don't need to hold a key down to confirm their pickup
					if (!Cast<APlayerCharacter>(InteractInstigator) || MaxCarryConfirmTime <= 0)
					{
						InteractInstigator->Server_CarryArrestedTarget_Implementation(this);
					}
				}

				return;
			}
		}
	}
	
	if (CanArrest())
	{
		if (CanArrestRagdoll())
		{
			if (IsInPositionForArrest(InteractInstigator))
			{
				if (!PendingRagdollArrestCharacter)
				{
					PendingRagdollArrestCharacter = InteractInstigator;
					InteractInstigator->PendingRagdollArrestCharacter = this;
					CurrentRagdollArrestConfirmTime = 0.0f;
						
					// If we're not a player or if max pickup time is 0,
					// instantly start picking up this character.
					// AI don't need to hold a key down to confirm their pickup
					if (!Cast<APlayerCharacter>(InteractInstigator) || MaxRagdollArrestConfirmTime <= 0.0f)
					{
						InteractInstigator->DoArrestWithZipcuffs(this);
					}
				}

				return;
			}
		}

		if (IsInPositionForArrest(InteractInstigator))
		{
			InteractInstigator->DoArrestWithZipcuffs(this);
		}
		else
		{
			if (ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController()))
			{
				const FVector DirectionToInstigator = (InteractInstigator->GetActorLocation() - GetActorLocation()).GetSafeNormal();

				CyberneticController->SetFocalPoint(GetActorLocation() + DirectionToInstigator * -500.0f);
				InteractInstigator->PlayYellAnimation();
				InteractInstigator->PlayRawVO(VO_SWAT_COMMAND::CALL_SC_TURN_AROUND);
			}
		}
	}
}

void AReadyOrNotCharacter::EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	CurrentCarryConfirmTime = 0.0f;
	InteractInstigator->PendingCarryCharacter = nullptr;
	PendingCarryCharacter = nullptr;
	
	CurrentRagdollArrestConfirmTime = 0.0f;
	InteractInstigator->PendingRagdollArrestCharacter = nullptr;
	PendingRagdollArrestCharacter = nullptr;
}

UInteractableComponent* AReadyOrNotCharacter::GetInteractableComponent_Implementation() const
{
	return InteractableComponent;
}

EInputEvent AReadyOrNotCharacter::DetermineInputEvent_Implementation() const
{
	if (CanBePickedUp())
		return IE_Repeat;

	if (CanArrestRagdoll())
		return IE_Repeat;

	return IE_Pressed;
}

FName AReadyOrNotCharacter::DetermineAnimatedIcon_Implementation() const
{
	if (IsLocalPlayer())
		return NAME_None;

	LOCAL_PLAYER;
	
	if (IsArrested() || IsDeadOrUnconscious() || IsIncapacitated())
	{
		if (bHasBeenReported)
		{
			if (LocalPlayer->PendingCarryCharacter == this)
				return "Empty";
			
			if (CanBePickedUp())
			{
				if (IsInPositionForCarry(LocalPlayer))
				{
					return "Carry Arrested";
				}
			}
		}
		else
		{
			if (IsDeadNotUnconscious())
			{
				return "Report Dead";
			}
		
			if (IsIncapacitated())
			{
				return "Report Incapacitated";
			}
		
			if (IsArrested())
			{
				return "Report Arrest";
			}
		}
	}

	if (CanArrest())
	{
		if (IsInPositionForArrest(LocalPlayer))
		{
			if (IsSuspect() || IsCivilian())
				return "Restrain";
		}
		else
		{
			if (!IsInRagdoll() && !LocalPlayer->IsAny1PMontagePlaying())
			{
				return IsSuspect() ? "Order Suspect" : "Order Civilian";
			}
		}

		return NAME_None;
	}

	return NAME_None;
}

FText AReadyOrNotCharacter::DetermineActionText_Implementation() const
{
	// A little ugly, but saves having to duplicate the code
	FString ActionPromptKey = DetermineAnimatedIcon_Implementation().ToString();
	ActionPromptKey.RemoveSpacesInline();
	return FText::FromStringTable("ActionPromptTable", ActionPromptKey);
}

float AReadyOrNotCharacter::DetermineInteractionDistance_Implementation() const
{
	if ((IsArrested() || IsDeadOrUnconscious() || IsIncapacitated()) && !bHasBeenReported)
		return 500.0f;

	if (IsArrested())
	{
		LOCAL_PLAYER;
		if (IsInPositionForCarry(LocalPlayer))
			return 150.0f;
	}
	
	if (CanArrest())
	{
		if (const APlayerController* PlayerController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
		{
			if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn()))
			{
				if (IsInPositionForArrest(PlayerCharacter))
				{
					if (IsDeadOrUnconscious() || IsInRagdoll())
						return 250.0f;
					
					return 170.0f;
				}

				return 350.0f;
			}
		}
	}

	return 160.0f;
}

float AReadyOrNotCharacter::DetermineCurrentProgress_Implementation() const
{
	if (PendingCarryCharacter)
	{
		return FMath::Clamp(CurrentCarryConfirmTime/MaxCarryConfirmTime, 0.0f, 1.0f);
	}
	
	if (PendingRagdollArrestCharacter)
	{
		return FMath::Clamp(CurrentRagdollArrestConfirmTime/MaxRagdollArrestConfirmTime, 0.0f, 1.0f);
	}

	return 0.0f;
}

bool AReadyOrNotCharacter::ShouldImprintIconOnHUD() const
{
	if (IsIncapacitated() && !bHasBeenReported)
		return true;
	
	if (CanBePickedUp())
		return false;

	if (PendingRagdollArrestCharacter || CanArrestRagdoll())
		return false;

	return true;
}

void AReadyOrNotCharacter::Multicast_OnIncapacitated_Implementation(FName LastBone)
{
	LastHitBoneName = LastBone;
	
	if (PerceptionStimuliComp && HasAuthority())
	{
		PerceptionStimuliComp->UnregisterFromPerceptionSystem();
	}
	
	// requires a re-report (maybe they were reported while arrested and now must be reported again while dead)
	bHasBeenReported = false;

	if (GetController())
		GetController()->UnPossess();
}

void AReadyOrNotCharacter::OnHealthDepleted()
{
	if (!KilledBy)
	{
		if (LastDamageEvent.Instigator)
			KilledBy = Cast<AReadyOrNotCharacter>(LastDamageEvent.Instigator->GetPawn());
	}
	
	OnCharacterKilled.Broadcast(KilledBy, this);
	OnCharacterKilled.Clear();

	if (const AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->OnCharacterKilled.Broadcast(this, KilledBy);
	}
}

void AReadyOrNotCharacter::Multicast_OnKilled_Implementation(FName LastBone, AActor* DamageCauser)
{
	LastHitBoneName = LastBone;
	
	if (PerceptionStimuliComp && HasAuthority())
	{
		PerceptionStimuliComp->UnregisterFromPerceptionSystem();
	}

	bStartBlendInIncapacitation = false;
	bBlendInIncapacitation = false;
	IncapacitationLoopAnim = nullptr;
	
	DisablePhysicalAnimation();

	// Offset our blood pool spawn by the length of our death animation (good for long bleedout animations)
	if (Blood)
	{
		float BloodPoolSpawnDelay = Blood->BloodPoolSpawnDelay;
		if (CurrentDeathMontage)
			BloodPoolSpawnDelay += CurrentDeathMontage->GetPlayLength();
		
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotCharacter::SpawnBloodPool, BloodPoolSpawnDelay);
	}

	// If we're shot in the thinky bits enforce all VO to stop
	// TODO(killo): what i'd like to do is have a ellipsoid-line check where the brain would be
	// so that no death sounds are made only if you specifically hit a character through the top of the head
	if (LastHitBoneName == "head" || (GetMesh() && GetMesh()->BoneIsChildOf(LastHitBoneName, "head")))
	{
		RemoveVocalChords();
	}
	
	float ImpulseStrength = 3000.0f;
	if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
	{
		if (Weapon->GetCurrentAmmoType())
		{
			ImpulseStrength = Weapon->GetCurrentAmmoType()->DefaultRagdollImpulseStrength;

			if (HeadBones.Contains(LastHitBoneName))
			{
				ImpulseStrength = Weapon->GetCurrentAmmoType()->HeadRagdollImpulseStrength;
			}
			else if (L_Arm.Contains(LastHitBoneName) || R_Arm.Contains(LastHitBoneName))
			{
				ImpulseStrength = Weapon->GetCurrentAmmoType()->ArmRagdollImpulseStrength;
			}
			else if (L_Leg.Contains(LastHitBoneName) || R_Leg.Contains(LastHitBoneName))
			{
				ImpulseStrength = Weapon->GetCurrentAmmoType()->LegRagdollImpulseStrength;
			}
			else if (Torso.Contains(LastHitBoneName))
			{
				ImpulseStrength = Weapon->GetCurrentAmmoType()->TorsoRagdollImpulseStrength;
			}
		}
	}

	if (GetMesh()->IsSimulatingPhysics())
	{
		if (LastDeathPointDamageEvent)
		{
			const FVector Impulse = (LastDeathPointDamageEvent->HitInfo.TraceEnd - LastDeathPointDamageEvent->HitInfo.TraceStart).GetSafeNormal() * ImpulseStrength;
			GetMesh()->AddImpulseAtLocation(Impulse, LastDeathPointDamageEvent->HitInfo.Location, LastDeathPointDamageEvent->HitInfo.BoneName);
		}
		else if (LastDeathRadialDamageEvent)
		{
			GetMesh()->AddRadialImpulse(LastDeathRadialDamageEvent->Origin, LastDeathRadialDamageEvent->Params.OuterRadius, ImpulseStrength, RIF_Linear);
		}
	}
	
	GetMesh()->SetNotifyRigidBodyCollision(true);
	GetMesh()->OnComponentHit.RemoveAll(this);
	GetMesh()->OnComponentHit.AddDynamic(this, &AReadyOrNotCharacter::OnDeadHit);

	/* for early ragdoll detection depending on number of bumps */
	GetCapsuleComponent()->OnComponentHit.RemoveAll(this);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AReadyOrNotCharacter::OnCapsuleHit);
	
	// requires a re-report (maybe they were reported while arrested and now must be reported again while dead)
	bHasBeenReported = false;

	if (GetController())
		GetController()->UnPossess();

	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	if (GetCharacterMovement())
		GetCharacterMovement()->DisableMovement();
}

void AReadyOrNotCharacter::OnDeadHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//const ABulletProjectile* Bullet = Cast<ABulletProjectile>(OtherActor);
	//if (Bullet && TimeDead > 0.25f && bBulletForceTransferred)
	//{
	//	bBulletForceTransferred = false;
	//}
	//else
	{
		//if (IsInRagdoll() && !IsRagdollBlending())
		{
			const float NormalImpulseSize = NormalImpulse.Size();
			if (RagdollSoundsPlayed < MaxRagdollSounds && BodyFallEvent && NormalImpulseSize > MinimumBodyFallImpulse && !BodyFallSoundSource)
			{
				//LOG_NUMBER(NormalImpulseSize);
				FTransform BodyHitTransform;
				BodyHitTransform.SetLocation(Hit.Location);
				BodyHitTransform.SetRotation(Hit.ImpactNormal.ToOrientationQuat());
				
				FMODParam Material;
				Material.paramName = "Material";
				Material.paramVal = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
				
				const float RagdollVelocity = NormalImpulseSize/4000.0f;
				
				FMODParam Impulse;
				Impulse.paramName = "Velocity";
				Impulse.paramVal = RagdollVelocity;

				//LOG_NUMBER(RagdollVelocity);

				const TArray<FMODParam> FmodParams = { Material, Impulse };
				
				BodyFallSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), BodyFallEvent, FTransform(FRotator(), Hit.Location, FVector()), FmodParams, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(BodyFallSoundSource)
				{
					BodyFallSoundSource->Play();
					BodyFallSoundSource->OnEventStopped.AddDynamic(this, &AReadyOrNotCharacter::OnBodyFallAudioStop);
				}

				/*
				UFMODAudioComponent* AudioComponent = GetWorld()->GetSubsystem<UFMODWorldSubsystem>()->PlayAudioAttached(BodyFallEvent, GetMesh(), Hit.BoneName, false);
				AudioComponent->SetParameter("Material", UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
				AudioComponent->SetParameter("Velocity", NormalImpulseSize/750.0f);
				AudioComponent->Play();
				*/
				
				// Track death hits so we don't spam the death hit sounds
				RagdollSoundsPlayed++;
			}
		}
	}
}

void AReadyOrNotCharacter::OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsDeadOrUnconscious() || IsIncapacitated())
	{
		OnDeadHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
		return;
	}

	if (!HasAuthority())
		return;

	if (OtherActor == this)
		return;

	if (const ABulletProjectile* BulletProjectile =  Cast<ABulletProjectile>(OtherActor))
	{
		if (const AShotgun* Shotgun = Cast<AShotgun>(BulletProjectile->FiredFromWeapon))
		{
			if (Shotgun->IsLessLethalWeapon())
				return;
		}
	}
		
	const float NormalImpulseSize = NormalImpulse.Size();
	if (NormalImpulseSize > 50.0f)
	{
		//ULog::Number(NormalImpulseSize, "OnMeshHit: Velocity: ");

		// Direct hit kills character
		if (const AGrenadeProjectile* GrenadeProjectile = Cast<AGrenadeProjectile>(OtherActor))
		{
			if ((GrenadeProjectile->FiredFromPlayer != this &&
				GrenadeProjectile->bDestroyOnHit) || GrenadeProjectile->ImpactCount <= 1)
			{
				Server_ApplyPointDamage(this, 99999.0f, -NormalImpulse, Hit, nullptr, OtherActor, UDamageType::StaticClass());
				return;
			}
		}

		// Apply a small amount of damage based on the impact velocity, ignore characters
		if (OtherActor && !OtherActor->IsA(StaticClass()))
			Server_ApplyPointDamage(OtherActor, NormalImpulseSize/1000.0f, -NormalImpulse, Hit, nullptr, this, UDamageType::StaticClass());
	}
}

void AReadyOrNotCharacter::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
}

void AReadyOrNotCharacter::Server_ApplyPointDamage_Implementation(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	UGameplayStatics::ApplyPointDamage(DamagedActor, BaseDamage, HitFromDirection, HitInfo, EventInstigator, DamageCauser, DamageTypeClass);
}

bool AReadyOrNotCharacter::IsComponentRelevantForNavigation(UActorComponent* Component) const
{
	return false;
	
	/* TODO: need to test this
	if (Component == GetCapsuleComponent())
		return true;
	
	return false;
	*/
}

void AReadyOrNotCharacter::InitCollisionPreset()
{
	// Setup default collision for capsule and mesh
	
	// Capsule
	{
		const ECollisionEnabled::Type& DefaultCapsuleCollisionEnabled = GetDefault<AReadyOrNotCharacter>(GetClass())->GetCapsuleComponent()->GetCollisionEnabled();
		const ECollisionChannel& DefaultCapsuleObjectType = GetDefault<AReadyOrNotCharacter>(GetClass())->GetCapsuleComponent()->GetCollisionObjectType();
		const FCollisionResponseContainer& DefaultCapsuleCollisionResponses = GetDefault<AReadyOrNotCharacter>(GetClass())->GetCapsuleComponent()->GetCollisionResponseToChannels();
		
		DefaultCollision.CapsuleCollision.CollisionEnabled = DefaultCapsuleCollisionEnabled;
		DefaultCollision.CapsuleCollision.ObjectType = DefaultCapsuleObjectType;
		DefaultCollision.CapsuleCollision.ResponseToChannels = DefaultCapsuleCollisionResponses;
	}

	// Mesh
	{
		const ECollisionEnabled::Type& DefaultMeshCollisionEnabled = GetDefault<AReadyOrNotCharacter>(GetClass())->GetMesh()->GetCollisionEnabled();
		const ECollisionChannel& DefaultMeshObjectType = GetDefault<AReadyOrNotCharacter>(GetClass())->GetMesh()->GetCollisionObjectType();
		const FCollisionResponseContainer& DefaultMeshCollisionResponses = GetDefault<AReadyOrNotCharacter>(GetClass())->GetMesh()->GetCollisionResponseToChannels();

		DefaultCollision.MeshCollision.CollisionEnabled = DefaultMeshCollisionEnabled;
		DefaultCollision.MeshCollision.ObjectType = DefaultMeshObjectType;
		DefaultCollision.MeshCollision.ResponseToChannels = DefaultMeshCollisionResponses;
	}

	// Setup ragdoll collision for capsule and mesh
	FCollisionResponseTemplate RagdollCollisionTemplate;
	if (UCollisionProfile::Get()->GetProfileTemplate("Ragdoll", RagdollCollisionTemplate))
	{
		RagdollCollision.CapsuleCollision = RagdollCollisionTemplate;
		RagdollCollision.MeshCollision = RagdollCollisionTemplate;

		RagdollCollision.CapsuleCollision.CollisionEnabled = ECollisionEnabled::NoCollision;
		RagdollCollision.CapsuleCollision.ObjectType = ECC_Pawn;
		RagdollCollision.CapsuleCollision.ResponseToChannels.SetAllChannels(ECR_Ignore);
		RagdollCollision.CapsuleCollision.ResponseToChannels.SetResponse(ECC_WorldStatic, ECR_Block);

		RagdollCollision.MeshCollision.ObjectType = ECC_PhysicsBody;
	}

	// Setup paired interaction collision for capsule and mesh
	FCollisionResponseTemplate PairedInteractionCollisionTemplate;
	if (UCollisionProfile::Get()->GetProfileTemplate("PairedInteractionPawn", PairedInteractionCollisionTemplate))
	{
		PairedInteractionCollision.CapsuleCollision = PairedInteractionCollisionTemplate;
		PairedInteractionCollision.MeshCollision = PairedInteractionCollisionTemplate;
	}

	PreviousCollisionTemplate = &DefaultCollision;
}

void AReadyOrNotCharacter::SetCollisionPreset(const FCharacterCollisionTemplate& InCollisionTemplate)
{
	// Capsule
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(InCollisionTemplate.CapsuleCollision.CollisionEnabled);
		GetCapsuleComponent()->SetCollisionObjectType(InCollisionTemplate.CapsuleCollision.ObjectType);
		GetCapsuleComponent()->SetCollisionResponseToChannels(InCollisionTemplate.CapsuleCollision.ResponseToChannels);
	}

	// Mesh
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(InCollisionTemplate.MeshCollision.CollisionEnabled);
		GetMesh()->SetCollisionObjectType(InCollisionTemplate.MeshCollision.ObjectType);
		GetMesh()->SetCollisionResponseToChannels(InCollisionTemplate.MeshCollision.ResponseToChannels);
	}

	PreviousCollisionTemplate = &InCollisionTemplate;
}

void AReadyOrNotCharacter::SetPreviousCollisionPreset()
{
	if (PreviousCollisionTemplate)
	{
		SetCollisionPreset(*PreviousCollisionTemplate);
	}
}

bool AReadyOrNotCharacter::IsInRagdoll() const
{
	return RagdollComponent->IsInRagdoll();
}

bool AReadyOrNotCharacter::IsRagdollBlending() const
{
	return bBlendInPhysics;
	//return false;
}

void AReadyOrNotCharacter::OnRagdollStart(URagdollComponent* InRagdollComponent)
{
	ThrowEquippedItem();
		
	GetFaceMesh()->SetMasterPoseComponent(GetMesh(), true);
	GetFaceMesh()->SetAnimInstanceClass(nullptr);
	
	// When ragdoll enabled from Anim Notify, disable collision to prevent blocking of navigation and line of sight 
	SetCollisionPreset(RagdollCollision);
	//GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::EnableRagdoll, 0.0f));
}

void AReadyOrNotCharacter::OnRagdollBlendStop(URagdollComponent* InRagdollComponent)
{
	bBlendInPhysics = false;
}

void AReadyOrNotCharacter::OnRagdollPhysBodyHit(URagdollComponent* InRagdollComponent, FVector Impulse, const FHitResult& Hit)
{
	bBlendInPhysics = false;
	
	/*
	const float NormalImpulseSize = Impulse.Size();
	if (RagdollSoundsPlayed < MaxRagdollSounds && BodyFallEvent && NormalImpulseSize > MinimumBodyFallImpulse && !UFMODBlueprintStatics::EventInstanceIsValid(BodyFallInstance))
	{
		LOG_NUMBER(NormalImpulseSize);
		FTransform BodyHitTransform;

		BodyHitTransform.SetLocation(Hit.Location);
		BodyHitTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
		BodyHitTransform.SetRotation(Hit.ImpactNormal.ToOrientationQuat());
		
		FMODParam Material;
		Material.paramName = "Material";
		Material.paramVal = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
		
		FMODParam Velocity;
		Velocity.paramName = "Velocity";
		Velocity.paramVal = NormalImpulseSize/4000.0f;

		const float RagdollVelocity = NormalImpulseSize/4000.0f;
		LOG_NUMBER(RagdollVelocity);

		const TArray<FMODParam> FmodParams = { Material, Velocity };

		BodyFallInstance = FMODAudioPropagationComp->PlayEvent(BodyFallEvent, Hit.Location, FmodParams);

		// Track death hits so we don't spam the death hit sounds
		RagdollSoundsPlayed++;
	}
	*/
}

void AReadyOrNotCharacter::Multicast_EnableRagdoll_Implementation(const float Duration)
{
	// Already called on the server, may cause stack overflow if not checked here
	if (HasAuthority())
		return;
	
	EnableRagdoll(Duration);
}

void AReadyOrNotCharacter::Multicast_DisableRagdoll_Implementation()
{
	// Already called on the server, may cause stack overflow if not checked here
	if (HasAuthority())
		return;
	
	DisableRagdoll();
}

void AReadyOrNotCharacter::EnableRagdoll(const float Duration)
{
	// If we're being destroyed, ignore
	if (!IsValid(this))
		return;

	if(ASuspectCharacter* Suspect = Cast<ASuspectCharacter>(this))
	{
		if(Suspect->bLastDamageCauserIsC2)
		{
			UAchievementStatics::UnlockAchievement(GetWorld(), EAchievement::DUE_PROCESS);
		}	
	}
	
	if (Duration > 0.0f && HasAuthority())
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_GetBackupAfterRagdoll, this, &AReadyOrNotCharacter::GetBackupAfterRagdoll_Internal, Duration, false);
	}

	if (IsInRagdoll())
		return;

	ThrowEquippedItem();

	GetCharacterMovement()->StopMovementImmediately();

	SetCollisionPreset(RagdollCollision);

	if (GetMesh())
	{
		UPhysicsAsset* NewPhysicsAsset = GetAppropriatePhysicsAsset();
		SetPhysicsAsset(NewPhysicsAsset, true);
		
		if (GetMesh()->GetPhysicsAsset() == NewPhysicsAsset)
		{
			if (GetMesh()->GetAnimInstance() && GetMesh()->SkeletalMesh && GetMesh()->GetComponentSpaceTransforms().Num() > 0)
				GetMesh()->GetAnimInstance()->SavePoseSnapshot("RagdollPoseStart");

			GetMesh()->SetSimulatePhysics(true);
			GetMesh()->SetCanEverAffectNavigation(false);

			GetMesh()->SetNotifyRigidBodyCollision(true);
			GetMesh()->OnComponentHit.RemoveAll(this);
			GetMesh()->OnComponentHit.AddDynamic(this, &AReadyOrNotCharacter::OnDeadHit);

			/* for early ragdoll trigger depending on number of collision bumps */
			//GetCapsuleComponent()->OnComponentHit.RemoveAll(this);
			//GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AReadyOrNotCharacter::OnCapsuleHit);
			
			GetFaceMesh()->SetMasterPoseComponent(GetMesh(), true);
			GetFaceMesh()->SetAnimInstanceClass(nullptr);

			if (!bStartedPlayingDeath)
				StopTPMontage(nullptr, 0.25f);
		}

		#if WITH_EDITOR
		ULog::Info("Enabling Ragdoll...");
		#endif

		//bInRagdoll = GetMesh()->IsSimulatingPhysics("pelvis");

		if (HasAuthority())
		{
			// Now enable it for clients as we're not using the rep variable anymore
			Multicast_EnableRagdoll(Duration);
		}
	}
}

void AReadyOrNotCharacter::DisableRagdoll()
{
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_GetBackupAfterRagdoll);

	SetCollisionPreset(DefaultCollision);

	GetCharacterMovement()->StopMovementImmediately();

	if (GetMesh())
	{
		if (GetMesh()->GetAnimInstance() && GetMesh()->SkeletalMesh && GetMesh()->GetComponentSpaceTransforms().Num() > 0)
		{
			GetMesh()->GetAnimInstance()->SavePoseSnapshot("RagdollPoseEnd");
		}

		ResetPhysicsAsset();
		
		GetMesh()->SetSimulatePhysics(false);
		
		GetMesh()->SetNotifyRigidBodyCollision(false);
		GetMesh()->OnComponentHit.RemoveAll(this);

		GetCapsuleComponent()->OnComponentHit.RemoveAll(this);
		bCapsuleCollisionRagdolled = false;

		#if WITH_EDITOR
		ULog::Info("Disabling Ragdoll...");
		#endif
		
		//bInRagdoll = GetMesh()->IsSimulatingPhysics("pelvis");

		if (HasAuthority())
		{
			Multicast_DisableRagdoll();
		}
	}
}

void AReadyOrNotCharacter::ResetPhysicsAsset()
{
	if (UPhysicsAsset* PhysicsAsset = GetAppropriatePhysicsAsset())
	{
		Rep_ActiveRagdollPhysAsset = PhysicsAsset;
	}
	else
	{
		Rep_ActiveRagdollPhysAsset = GetClass()->GetDefaultObject<ACyberneticCharacter>()->GetMesh()->GetPhysicsAsset();
	}
	
	OnRep_ActiveRagdollPhysAsset();
}

void AReadyOrNotCharacter::Multicast_SavePoseSnapshot_Implementation(const FName& SnapshotName)
{
	if (GetMesh()->GetAnimInstance() && GetMesh()->SkeletalMesh && GetMesh()->GetComponentSpaceTransforms().Num() > 0)
	{
		GetMesh()->GetAnimInstance()->SavePoseSnapshot(SnapshotName);
	}
}

bool AReadyOrNotCharacter::GetBackupAfterRagdoll()
{
	if (IsDeadOrUnconscious())
		return false;
	
	if (!IsInRagdoll())
		return false;
	
	const FVector PelvisLocation = GetMesh()->GetSocketLocation("pelvis");

	FHitResult HitResult;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		
	const bool bIsGrounded = GetWorld()->LineTraceSingleByObjectType(HitResult, PelvisLocation, PelvisLocation - FVector::UpVector * 150.0f, ObjectQueryParams, GetCollisionQueryParameters());

	// Keep trying to test if grounded before disabling ragdoll
	if (!bIsGrounded)
	{
		EnableRagdoll(0.25f);
		return false;
	}

	DisableRagdoll();
	return true;
}

bool AReadyOrNotCharacter::GetBackupAfterRagdollArrest()
{
	return false;
}

void AReadyOrNotCharacter::OnRagdollDurationComplete()
{
}

void AReadyOrNotCharacter::GetBackupAfterRagdoll_Internal()
{
	if (GetBackupAfterRagdoll())
	{
		OnRagdollDurationComplete();
	}
}

void AReadyOrNotCharacter::Multicast_EnableRagdollBlendIn_Implementation()
{
	SetCollisionPreset(RagdollCollision);
	
	bBlendInPhysics = true;
}

void AReadyOrNotCharacter::DisablePhysicalAnimation()
{
	GetRagdollComponent()->DisablePhysicalAnimation(GetMesh());
	return;

	/*
	if (!IsInRagdoll())
	{
		FPhysicalAnimationData NewData;
		NewData.bIsLocalSimulation = false;
		NewData.OrientationStrength = 0.0f;
		NewData.AngularVelocityStrength = 0.0f;
		NewData.PositionStrength = 0.0f;
		NewData.VelocityStrength = 0.0f;
		NewData.MaxAngularForce = 0.0f;
		NewData.MaxLinearForce = 0.0f;
		
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("pelvis", NewData, true);
		
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("spine_1", NewData, true);
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("thigh_LE", NewData, true);
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("thigh_RI", NewData, true);
		
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("upperarm_LE", NewData, true);
		PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow("upperarm_RI", NewData, true);

		GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", false);
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", 0.0f);
		
		PhysicalAnimationComp->SetSkeletalMeshComponent(nullptr);
		
		UE_LOG(LogTemp, Display, TEXT("============== Disabling Physical Animation =========="))
	}
	*/
}

void AReadyOrNotCharacter::DestroyAnimInstance()
{
	if (GetMesh())
	{
		GetMesh()->SetAnimInstanceClass(nullptr);
	}
}

AReadyOrNotPlayerState* AReadyOrNotCharacter::GetRONPlayerState()
{
	return Cast<AReadyOrNotPlayerState>(GetPlayerState());
}

void AReadyOrNotCharacter::OnEquippedWeaponFire(ABaseMagazineWeapon* Weapon, bool bServer)
{
	if (bServer)
	{
		OnWeaponFire.Broadcast(this, Weapon, Weapon->GetBulletSpawn()->GetComponentRotation().Vector());
		
		const float Damage = AI_CONFIG_GET_FLOAT("FireWeaponMorale.Damage");
		const float InnerRadius = AI_CONFIG_GET_FLOAT("FireWeaponMorale.DamageInnerRadius");
		const float OuterRadius = AI_CONFIG_GET_FLOAT("FireWeaponMorale.DamageOuterRadius");
		const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("FireWeaponMorale.DamageFalloffCurve"));

		UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, Weapon->GetItemLocation(), Damage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_CIVILIAN}, Curve, "Weapon Fired");
	}
}

void AReadyOrNotCharacter::OnEquippedWeaponDryFire(ABaseMagazineWeapon* Weapon, bool bServer)
{
	if (bServer)
		OnWeaponDryFire.Broadcast(this, Weapon, Weapon->GetBulletSpawn()->GetComponentRotation().Vector());
}

void AReadyOrNotCharacter::OnEquippedWeaponMagCheck(ABaseMagazineWeapon* Weapon)
{
}

void AReadyOrNotCharacter::PlayWeaponFireAnimation(ABaseMagazineWeapon* Weapon, const bool bIsAiming, const bool bOnlyTP)
{
	if (!Weapon || !Weapon->AnimationData)
		return;

	Weapon->PlayFireAnimation(bIsAiming, IsCrouching(), !bOnlyTP);
}

void AReadyOrNotCharacter::PlayWeaponDryFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, const bool bOnlyTP)
{
	if (!Weapon || !Weapon->AnimationData)
		return;

	Weapon->PlayDryFireAnimation(bIsAiming, IsCrouching(), !bOnlyTP);
}

void AReadyOrNotCharacter::BindAllDelegates()
{
	const FOnActorSpawned::FDelegate ActorSpawnedDelegate = FOnActorSpawned::FDelegate::CreateUObject(this, &AReadyOrNotCharacter::OnActorSpawned);
	GetWorld()->AddOnActorSpawnedHandler(ActorSpawnedDelegate);
}

void AReadyOrNotCharacter::UnbindAllDelegates()
{
	OnAIStunnedPlaySound.RemoveAll(this);
	OnCharacterTakeDamage.Clear();
	OnCharacterKilled.Clear();
}

void AReadyOrNotCharacter::OnActorSpawned(AActor* Actor)
{
	if (const AReadyOrNotCharacter* PlayerCharacter = Cast<AReadyOrNotCharacter>(Actor))
	{
		LowReadyIgnoredCapsules.Add(PlayerCharacter->GetCapsuleComponent());
	}
}

void AReadyOrNotCharacter::SetLowReady(bool bUp, bool bLowReady)
{
	if (IsCarried() || IsCarrying())
		return;
	
	if (!bLowReady)
	{
		bLowReadyPointUp = bLowReadyPointDown = false;
	}
	else if (!bUp)
	{
		bLowReadyPointDown = true;
		bLowReadyPointUp = false;
	}
	else
	{
		bLowReadyPointDown = false;
		bLowReadyPointUp = true;
	}
}

bool AReadyOrNotCharacter::IsLocalPlayer() const
{
	return IsLocallyControlled() && IsPlayerControlled();
}

bool AReadyOrNotCharacter::IsOnSWATTeam() const
{
	if (GetTeam() == ETeamType::TT_NONE)
		return false;
	
	return GetTeam() != ETeamType::TT_CIVILIAN && GetTeam() != ETeamType::TT_SUSPECT;
}

ETeamType AReadyOrNotCharacter::GetTeam() const
{
	if (Tags.Contains("MyPlayerPreview"))
	{
		if (AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
		{
			return pc->GetTeamType();
		}
	}
	
	if (AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(GetPlayerState()))
	{
		return ps->GetTeam();
	}
	
	return DefaultTeam;
}

bool AReadyOrNotCharacter::IsCivilian() const
{
	return GetTeam() == ETeamType::TT_CIVILIAN;
}

bool AReadyOrNotCharacter::IsSuspect() const
{
	return GetTeam() == ETeamType::TT_SUSPECT;
}

bool AReadyOrNotCharacter::IsOnSameTeam(AReadyOrNotCharacter* A, AReadyOrNotCharacter* B)
{
	if (!A || !B)
	{
		return false;
	}

	if (A == B)
		return true;

	const AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(A->GetWorld()->GetGameState());
	if (gs && gs->bFreeForAll)
	{
		return false;
	}

	if (gs && gs->bPvPMode)
	{
		return A->GetTeam() == B->GetTeam();
	}

	if (A->GetTeam() == B->GetTeam())
	{
		return true;
	}

	if (A->IsOnSWATTeam() && B->IsOnSWATTeam())
	{
		return true;
	}
		
	if ((A->GetTeam() == ETeamType::TT_SERT_RED && B->GetTeam() == ETeamType::TT_SERT_BLUE) ||
		(A->GetTeam() == ETeamType::TT_SERT_BLUE && B->GetTeam() == ETeamType::TT_SERT_RED))
	{
		return true;
	}

	return false;
}

void AReadyOrNotCharacter::DoFreeLean()
{
	if (IsCarried() || IsCarrying())
		return;
	
	if (IsArrested())
	{
		return;
	}
	
	if (!IsValid(LeanAudioComponent))
	{
		if (LeanAudioEvent)
		{
			// TODO: use audio pool
			LeanAudioComponent = UFMODBlueprintStatics::PlayEventAttached(LeanAudioEvent, RootComponent, NAME_None,GetActorLocation(), EAttachLocation::KeepWorldPosition, true, true, false);
		}
	} 
	
	bFreeLeaning = true;
	FreeLeanX = QuickLeanAmount;
	FreeLeanZ = -FMath::Abs(QuickLeanAmount * 0.5);
	QuickLeanAmount = 0.0f;
}

void AReadyOrNotCharacter::EndFreeLean()
{
	bFreeLeaning = false;
	QuickLeanAmount = FMath::Clamp(FreeLeanX, -45.0f, 45.0f);
	FreeLeanX = 0.0f;
	FreeLeanZ = 0.0f;

	if (LeanAudioComponent)
	{
		LeanAudioComponent->Stop();
		LeanAudioComponent->Release();
		LeanAudioComponent->DestroyComponent();
	}
}

void AReadyOrNotCharacter::ToggleFreeLean()
{
	if (bFreeLeaning)
	{
		EndFreeLean();
	}
	else
	{
		DoFreeLean();
	}
}

float AReadyOrNotCharacter::GetLeanAmount(FVector Component, float& PendingVal, float MaxValue)
{
	// Trace out towards where we're leaning to. If the first object we hit is farther away than the lean amount, then we use the lean amount.
	// Otherwise, use the amount dictated by the trace. (so we don't lean through objects)
	FHitResult TraceResult(ForceInit);
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("LeanTrace")), true, this);
	TraceParams = GetCollisionQueryParameters();

	FVector Start = GetMesh()->GetBoneLocation("head");
	//FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector End;
	bool TraceHit;


	// The 10.0f and the 8.0f later on seem kind of random but they are here to prevent rubber-banding --eez
	if (PendingVal < 0)
	{
		End = (Component * (PendingVal - 20.0f)) + Start;
	}
	else
	{
		End = (Component * (PendingVal + 20.0f)) + Start;
	}

	//TraceParams.bTraceComplex = true;

	TraceHit = GetWorld()->LineTraceSingleByChannel(TraceResult, Start, End, ECC_WorldStatic, TraceParams);
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, "Lean Trace Hit: " + (TraceResult.GetActor() ? TraceResult.GetActor()->GetName() : "None") + " on component: "  + (TraceResult.GetComponent() ? TraceResult.GetComponent()->GetName() : "None"));
	//DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 10.0f, 1.0f, 1.0f);
	// Check we are not leaning into a item (hacky fix to fix leaning in suspect helmet with unknown cause)
	if (TraceHit)
	{
		if (PendingVal < 0)
		{
			PendingVal = (-TraceResult.Distance) + 20.0f;
		}
		else
		{
			PendingVal = TraceResult.Distance - 20.0f;
		}
	}

	return PendingVal;
}

void AReadyOrNotCharacter::StopLean()
{
	bLeanLeftToggle = false;
	bLeanRightToggle = false;
}


void AReadyOrNotCharacter::ToggleLeanLeft(bool bADSIsActive)
{
	if (IsCarried() || IsCarrying())
		return;
	
	bLeanRightToggle = false;
	
	if (bFreeLeaning)
		return;

	if (!bADSIsActive) 
	{
		StopLean();
	} 
	else
	{
		bLeanLeftToggle = !bLeanLeftToggle;
	}

	Lean(0.0f);
}

void AReadyOrNotCharacter::ToggleLeanRight(bool bADSIsActive)
{
	if (IsCarried() || IsCarrying())
		return;
	
	bLeanLeftToggle = false;

	if (bFreeLeaning)
		return;

	if (!bADSIsActive)
	{
		StopLean();
	}
	else
	{
		bLeanRightToggle = !bLeanRightToggle;
	}
	
	Lean(0.0f);
}

void AReadyOrNotCharacter::Lean(float Val)
{
	if (IsCarried() || IsCarrying())
		return;
	
	const float TempQuickLeanAmount = QuickLeanAmount;

	if (bLeanLeftToggle && bLeanRightToggle)
	{
		Val = 0.0f;
	}
	else if (bLeanLeftToggle)
	{
		Val = -1.0f;
	}
	else if (bLeanRightToggle)
	{
		Val = 1.0f;
	}
	
	if (IsArrested() || bIsBeingArrested)
	{
		Val = 0.0f;
	}

	bIsLeaning = Val != 0.0f;

	if (bMovementLocked)
	{
		return;
	}

	if (bIsLeaning && !bFreeLeaning)
	{
		if (GetEquippedItem() && !GetEquippedItem()->ConsumeLeanInput(Val))
		{
			const float LeanAmount = Val > 0 ? MAX_LEAN_ANGLE : -MAX_LEAN_ANGLE;

			// called when not leaning and we start leaning
			if (QuickLeanAmount == 0.0f)
			{
				OnLeanStart();
			}

			QuickLeanAmount = FMath::FInterpConstantTo(QuickLeanAmount, LeanAmount, GetWorld()->GetDeltaSeconds(), 400.0f);
			QuickLeanAmount = GetLeanAmount(GetActorRightVector()/*GetFirstPersonCameraComponent()->GetRightVector()*/, QuickLeanAmount, MAX_LEAN_ANGLE);
		}
	}
	else if (!bFreeLeaning)
	{
		// called when leaning max lean and stop leaning
		if (FMath::Abs(QuickLeanAmount) == MAX_LEAN_ANGLE)
		{
			OnLeanEnd();
		}
		
		QuickLeanAmount = FMath::FInterpTo(QuickLeanAmount, 0.0f, GetWorld()->GetDeltaSeconds(), 30.0f);
	}
	else
	{
		QuickLeanAmount = 0.0f;
	}

	//GEngine->AddOnScreenDebugMessage(105, 2.0f, FColor::White, "Lean: " + FString::SanitizeFloat(QuickLeanAmount));

	if (TempQuickLeanAmount != QuickLeanAmount)
		Server_UpdateLean(QuickLeanAmount, FreeLeanX, FreeLeanZ);
}

void AReadyOrNotCharacter::LeanUp(float Val)
{
	const float TempFreeLeanZ = FreeLeanZ;

	if (bMovementLocked)
	{
		return;
	}

	if (Val != 0.0f)
	{
		if (bFreeLeaning)
		{
			FreeLeanZ = FMath::Clamp(FreeLeanZ + (Val * MAX_FREE_LEAN_SPEED_NOT_AIMING * GetWorld()->GetDeltaSeconds()), -MAX_FREE_LEAN_ANGLE, MAX_FREE_LEAN_ANGLE - 10.0f);
			FreeLeanZ = GetLeanAmount(GetActorUpVector()/*GetFirstPersonCameraComponent()->GetUpVector()*/, FreeLeanZ, MAX_FREE_LEAN_ANGLE - 10.0f);
		}
	}

	if (!bFreeLeaning && FreeLeanZ != 0.0f)
		FreeLeanZ = FMath::FInterpTo(FreeLeanZ, 0.0f, GetWorld()->GetDeltaSeconds(), 10.0f);

	bLeaningUp = FreeLeanZ > 0.0f;
	bLeaningDown = FreeLeanZ < 0.0f;

	if (TempFreeLeanZ != FreeLeanZ)
		Server_UpdateLean(QuickLeanAmount, FreeLeanX, FreeLeanZ);
}

void AReadyOrNotCharacter::LeanRight(float Val)
{
	const float TempFreeLeanY = FreeLeanX;

	if (bMovementLocked)
	{
		return;
	}

	if (Val != 0.0f)
	{
		if (bFreeLeaning)
		{
			FreeLeanX = FMath::Clamp(FreeLeanX + (Val * (bAiming ? MAX_FREE_LEAN_SPEED_AIMING : 150) * GetWorld()->GetDeltaSeconds()), -MAX_FREE_LEAN_ANGLE, MAX_FREE_LEAN_ANGLE);
			FreeLeanX = GetLeanAmount(GetActorRightVector()/*GetFirstPersonCameraComponent()->GetRightVector()*/, FreeLeanX, MAX_FREE_LEAN_ANGLE);
			QuickLeanAmount = FreeLeanX;
		}
	}

	if (!bFreeLeaning && FreeLeanX != 0.0f)
	{
		FreeLeanX = FMath::FInterpTo(FreeLeanX, 0.0f, GetWorld()->GetDeltaSeconds(), 10.0f);
		QuickLeanAmount = FreeLeanX;
	}

	if (TempFreeLeanY != FreeLeanX)
		Server_UpdateLean(QuickLeanAmount, FreeLeanX, FreeLeanZ);
}

void AReadyOrNotCharacter::ToggleLean(float Val)
{
	if (Val < 0.0f)
		ToggleLeanLeft();
	else if (Val > 0.0f)
		ToggleLeanRight();

	Lean(Val);
}

void AReadyOrNotCharacter::CalculateLeanMovement(float DeltaTime)
{
	//float DeltaSeconds = DeltaTime;
	LeanPos_LastFrame = LeanPos_CurrentFrame;
	LeanPos_CurrentFrame = GetMesh()->GetBoneLocation("head");
	LeanMovementValue = FMath::Clamp(FVector::Dist(LeanPos_CurrentFrame, LeanPos_LastFrame) * DeltaTime,0.0f, 1.0f);
	//GEngine->AddOnScreenDebugMessage(105, 2.0f, FColor::White, "FPCamLeanMove: " + FString::SanitizeFloat(LeanMovementPerFrame));
}

void AReadyOrNotCharacter::Server_UpdateLean_Implementation(float QuickLean, float newFreeLeanY, float NewFreeLeanZ)
{
	QuickLeanAmount = QuickLean;
	FreeLeanX = newFreeLeanY;
	FreeLeanZ = NewFreeLeanZ;
}

void AReadyOrNotCharacter::StartMelee()
{
	if (!GetController<APlayerController>() || !GetController<APlayerController>()->PlayerCameraManager)
		return;
	
	if (IsAnimationBlocking())
	{
		return;
	}

	if (IsLowReady() && UReadyOrNotFunctionLibrary::IsInLobby())
	{
		return;
	}

	if (UReadyOrNotFunctionLibrary::IsInDefusalWarmup())
	{
		return;
	}

	if (!GetEquippedItem())
		return;

	// Trace forward by a little bit to find our victim
	FHitResult MeleeTraceResult;
	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Visibility);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetInventoryComponent()->GetInventoryItems());
	
	const FVector EyesLocation = GetController<APlayerController>()->PlayerCameraManager->GetCameraLocation();
	const FVector StartTrace = EyesLocation;
	const FVector EndTrace = StartTrace + GetControlRotation().Vector() * MeleeRange;

	if (GetWorld()->LineTraceSingleByObjectType(MeleeTraceResult, StartTrace, EndTrace, CollisionObjectQueryParams, CollisionQueryParams))
	{
		if (GetEquippedItem() /*&& GetEquippedItem()->IsA(ABaseItem::StaticClass())*/ && GetEquippedItem()->AnimationData)
		{
			// If we have found a victim, issue a MeleeImpact on them and play the animation
			if (!GetEquippedItem()->HandleMelee(MeleeTraceResult))
			{
				//RecentMeleeVictim = Cast<AReadyOrNotCharacter>(MeleeTraceResult.GetActor());
			}

			GetEquippedItem()->PlayItemAnimation(GetEquippedItem()->AnimationData->MeleeHit, false);

			//if (GetEquippedItem()->AnimationData->MeleeHit.Body_FP && GetEquippedItem()->AnimationData->MeleeHit.Body_TP)
			//{
			//	Play1PMontage_NonClient(GetEquippedItem()->AnimationData->MeleeHit.Body_FP);
			//	Multicast_Play3PMontage(GetEquippedItem()->AnimationData->MeleeHit.Body_TP);
			//}

			Client_PlayScreenShake_Implementation(GetEquippedItem()->MeleeUserCameraShake);
		}
	}
	else
	{
		if (GetEquippedItem()/* && GetEquippedItem()->IsA(ABaseItem::StaticClass())*/ && GetEquippedItem()->AnimationData)
		{
			GetEquippedItem()->PlayItemAnimation(GetEquippedItem()->AnimationData->MeleeMiss, false);
			
			//if (GetEquippedItem()->AnimationData->MeleeMiss.Body_FP && GetEquippedItem()->AnimationData->MeleeMiss.Body_TP)
			//{
			//	Play1PMontage_NonClient(GetEquippedItem()->AnimationData->MeleeMiss.Body_FP);
			//	Multicast_Play3PMontage(GetEquippedItem()->AnimationData->MeleeMiss.Body_TP);
			//}

			Client_PlayScreenShake_Implementation(GetEquippedItem()->MeleeUserCameraShake);
		}
	}
	
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bDrawMeleeRange)
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, MeleeTraceResult.bBlockingHit ? FColor::Green : FColor::Red, false, 1.5f, 0, 2.0f);
	#endif

	#if WITH_EDITOR
	ULog::ObjectName(MeleeTraceResult.GetActor());
	#endif
}

bool AReadyOrNotCharacter::IsReloading() const
{
	if (const ABaseMagazineWeapon* Weapon = GetEquippedWeapon())
	{
		return Weapon->IsCurrentlyReloading();
	}
	
	return false;
}

bool AReadyOrNotCharacter::CanMelee() const
{
	if (IsLowReady() && UReadyOrNotFunctionLibrary::IsInLobby())
	{
		return false;
	}

	if (!IsActive())
		return false;

	return true;
}

void AReadyOrNotCharacter::OnMeleeTrace(const FHitResult HitResult, const bool bLocal)
{
	if (AActor* Actor = HitResult.GetActor())
	{
		if (Actor->Implements<UMeleeable>())
		{
			// Is not the server?
			if (bLocal)
			{
				UFMODEvent* ImpactSound = Execute_GetMeleeImpactSound(Actor);
				UParticleSystem* ImpactParticle = Execute_GetMeleeImpactParticle(Actor);

				// Don't bother multicasting if both are null
				if (ImpactSound || ImpactParticle)
				{
					PlayMeleeImpactEffects(ImpactSound, ImpactParticle);
				}
			}
			else
			{
				// Call the actor's custom implementation of what happens when they're melee'd by this character
				Execute_OnMelee(Actor, this, HitResult);
			}
		}
	}
}

// Called from notify
void AReadyOrNotCharacter::DoMelee(bool bLocal)
{
	if (!GetController<APlayerController>() || !GetController<APlayerController>()->PlayerCameraManager)
		return;

	if (!CanMelee())
		return;
	
	FHitResult MeleeTraceResult;

	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Destructible);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetInventoryComponent()->GetInventoryItems());

	const FVector EyesLocation = GetController<APlayerController>()->PlayerCameraManager->GetCameraLocation();
	const FVector StartTrace = EyesLocation;
	const FVector EndTrace = StartTrace + GetControlRotation().Vector() * MeleeRange;

	// Trace forward by a little bit to find our victim
	GetWorld()->SweepSingleByObjectType(MeleeTraceResult, StartTrace, EndTrace, FQuat::Identity, CollisionObjectQueryParams, FCollisionShape::MakeSphere(10.0f), CollisionQueryParams);
	if (MeleeTraceResult.GetActor())
	{
		OnMeleeTrace(MeleeTraceResult, bLocal);
	}
	
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bDrawMeleeRange)
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, MeleeTraceResult.bBlockingHit ? FColor::Blue : FColor::Orange, false, 1.5f, 0, 2.0f);
	#endif
}

void AReadyOrNotCharacter::Client_PlayMeleeImpactEffects_Implementation()
{
	Client_PlayScreenShake_Implementation(MeleeCameraShake);

	UFMODBlueprintStatics::PlayEventAttached(FPMeleeImpactFMODEvent, GetMesh(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
}

void AReadyOrNotCharacter::PlayMeleeImpactEffects(UFMODEvent* ImpactSound, UParticleSystem* ImpactParticle)
{
	if (ImpactSound)
		UFMODBlueprintStatics::PlayEventAttached(ImpactSound, GetMesh(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);

	if (ImpactParticle)
		UGameplayStatics::SpawnEmitterAttached(ImpactParticle, GetMesh(), NAME_None, FVector(-10.0f, 80.0f, 150.0f), FRotator::ZeroRotator, FVector::OneVector);
}

void AReadyOrNotCharacter::OnMelee_Implementation(AReadyOrNotCharacter* Attacker, FHitResult Hit)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;
	
	// Cannot melee self
	if (Attacker == this)
		return;
		
	if (Cast<ABallisticsShield>(GetEquippedItem()))
		return;
	
	if (!IsArrestedOrSurrendered())
	{
		StartStun(EStunType::ST_Stung, Attacker);
	}

	Client_PlayMeleeImpactEffects();

	DamagedByCharacters.AddUnique(Attacker);

	OnMeleeHitTaken.Broadcast(Attacker);

	#if WITH_EDITOR
	ULog::Info("Meleed by " + Attacker->GetName());
	#endif
}

UFMODEvent* AReadyOrNotCharacter::GetMeleeImpactSound_Implementation() const
{
	return TPMeleeImpactFMODEvent;
}

UParticleSystem* AReadyOrNotCharacter::GetMeleeImpactParticle_Implementation() const
{
	return MeleeImpactParticle;
}

bool AReadyOrNotCharacter::ShouldPlayMeleeEffectsLocally_Implementation() const
{
	return true;
}

bool AReadyOrNotCharacter::CanArrest() const
{
	if (IsOnSWATTeam())
		return false;

	if (bStartedPlayingDeath)
		return false;
	
	if (bArrestComplete || bIsBeingArrested)
		return false;

	// Can't arrest people who are missing arms
	if (GetGibComponent() && (GetGibComponent()->IsLimbGibbed(EGibAreas::GA_LeftArm) || GetGibComponent()->IsLimbGibbed(EGibAreas::GA_RightArm)))
		return false;
	
	if (IsSurrendered())
		return true;

	if ((IsDeadOrUnconscious() && bHasBeenReported) || (IsIncapacitated() && bHasBeenReported) || IsInRagdoll())
		return true;
	
	return false;
}

bool AReadyOrNotCharacter::IsArrested() const
{
	return !IsDeadOrUnconscious() && bArrestComplete;
}

bool AReadyOrNotCharacter::IsArrestedAndDead() const
{
	return IsDeadOrUnconscious() && bArrestComplete;
}

bool AReadyOrNotCharacter::IsArrestedAndIncapacitated() const
{
	return IsIncapacitated() && bArrestComplete;
}

bool AReadyOrNotCharacter::IsStartling() const
{
	for (auto& It : PlayedTableMontageMap3P)
	{
		if (It.Key.Contains("tp_startle") &&
			It.Value == GetCurrentMontage())
		{
			return true;
		}
	}

	return false;
}

bool AReadyOrNotCharacter::IsBeingCarried() const
{
	return bIsBeingCarried || UInteractionsData::IsPairedInteractionPlayingOn(CarryArrestedInteractionData, this);
}

bool AReadyOrNotCharacter::IsBeingArrested() const
{
	return bIsBeingArrested/* && UInteractionsData::IsPairedInteractionPlayingOn(this)*/;
}

bool AReadyOrNotCharacter::IsDoingArrest()
{
	if (const AZipcuffs* Zipcuffs = GetInventoryComponent()->GetInventoryItemOfClass_Native<AZipcuffs>(AZipcuffs::StaticClass(), false))
	{
		return Zipcuffs->bDoingArrest;
	}

	return false;
}

void AReadyOrNotCharacter::DoArrestWithZipcuffs(AReadyOrNotCharacter* Target)
{
	if (!Target)
		return;
	
	if (!Target->CanArrest())
		return;
	
	if (AZipcuffs* Zipcuffs = GetInventoryComponent()->GetInventoryItemOfClass_Native<AZipcuffs>(AZipcuffs::StaticClass(), false))
	{
		if (GetLocalRole() < ROLE_Authority)
		{
			Zipcuffs->Server_ArrestStart(Target);
		}
		else
		{
			Zipcuffs->Server_ArrestStart_Implementation(Target);
		}
	}
}

void AReadyOrNotCharacter::Arrest(AReadyOrNotCharacter* PlayerMakingArrest)
{
	if (!PlayerMakingArrest)
		return;

	ThrowAllWeapons();

	LockAllActions();
	
	PlayerMakingArrest->CurrentlyArresting = this;
	
	LastCharacterMakingArrest = PlayerMakingArrest;
	bIsBeingArrested = true;
}

void AReadyOrNotCharacter::CancelArrest(AReadyOrNotCharacter* PlayerMakingArrest)
{
	UnlockAllActions();

	bIsBeingArrested = false;

	if (LastCharacterMakingArrest)
	{
		LastCharacterMakingArrest->CurrentlyArresting = nullptr;
		LastCharacterMakingArrest = nullptr;
	}
	
	if (!PlayerMakingArrest)
		return;
	
	PlayerMakingArrest->CurrentlyArresting = nullptr;
}

void AReadyOrNotCharacter::ArrestComplete(AReadyOrNotCharacter* PlayerMakingArrest, AZipcuffs* Zipcuffs)
{
	UnlockAllActions();

	bIsBeingArrested = false;
	bArrestComplete = true;

	if (LastCharacterMakingArrest)
	{
		LastCharacterMakingArrest->CurrentlyArresting = nullptr;
		LastCharacterMakingArrest = nullptr;
	}
	
	if (CuffedRagdollPhysAsset)
	{
		Rep_ActiveRagdollPhysAsset = CuffedRagdollPhysAsset;
		OnRep_ActiveRagdollPhysAsset();
	}

	if (bArrestedAsRagdoll)
	{
		Multicast_SavePoseSnapshot("RagdollPoseStart");
		Multicast_SavePoseSnapshot_Implementation("RagdollPoseStart");

		/*
		if (GetMesh()->GetAnimInstance() && GetMesh()->SkeletalMesh && GetMesh()->GetComponentSpaceTransforms().Num() > 0)
			GetMesh()->GetAnimInstance()->SavePoseSnapshot("RagdollPoseStart");
		*/
		
		//OnBlendRagdollAnimFinished();
	}
	
	Multicast_ChangeFaceEmotion(ECharacterEmotion::Sad, 30.0f, 1.0f, 0.1f, 100);
	
	if (!PlayerMakingArrest || !Zipcuffs)
		return;

	if (!PlayerMakingArrest->GetController())
		return;
	
	PlayerMakingArrest->CurrentlyArresting = nullptr;

	ArrestedBy = PlayerMakingArrest;
	
	if (AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(PlayerMakingArrest->GetController()))
	{
		if (ACyberneticCharacter* arrested = Cast<ACyberneticCharacter>(this)) {
			if (arrested->IsSuspect() && arrested->IsPlayingDead()) {
				UAchievementStatics::UnlockAchievement(GetWorld(), EAchievement::THE_MAGICIAN);
			}
		}

		pc->OnArrest();
	}

	if (const AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->OnCharacterArrested.Broadcast(this, ArrestedBy);
	}
}

bool AReadyOrNotCharacter::IsCarried() const
{
	return IsValid(CarriedByCharacter);
}

bool AReadyOrNotCharacter::IsCarrying() const
{
	return IsValid(CurrentCarryCharacter);
}

bool AReadyOrNotCharacter::IsPlayingCarryAnims() const
{
	if (CarryArrestedInteractionData && DropArrestedInteractionData && ThrowArrestedInteractionData)
	{
		return Is3PMontagePlaying(CarryArrestedInteractionData->DriverMontage) ||
				Is3PMontagePlaying(DropArrestedInteractionData->DriverMontage) ||
				Is3PMontagePlaying(ThrowArrestedInteractionData->DriverMontage);
	}
	
	return false;
}

bool AReadyOrNotCharacter::IsDropping() const
{
	return CurrentCarryCharacter == nullptr && IsPlayingCarryAnims();
}

bool AReadyOrNotCharacter::IsBeingThrown() const
{
	return ThrownByCharacter != nullptr;
}

bool AReadyOrNotCharacter::IsGettingUp() const
{
	return false;
}

void AReadyOrNotCharacter::OnRep_CurrentCarryCharacterChanged()
{
	if (CurrentCarryCharacter)
	{
		CurrentCarryCharacter->LockMovementAndActions();
		
		MoveIgnoreActorAdd(CurrentCarryCharacter);
		CurrentCarryCharacter->MoveIgnoreActorAdd(this);

		CurrentCarryCharacter->AttachToComponent(/*CurrentCarryCharacter->CarriedByCharacter->*/GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false), "CarryRest_TP");
		//CurrentCarryCharacter->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform, "CarryRest_TP");
		CurrentCarryCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);
		CurrentCarryCharacter->GetMesh()->SetOwnerNoSee(false);
		CurrentCarryCharacter->GetMesh()->SetCastHiddenShadow(true);

		/*
		if (FakeCarryCharacterMesh)
		{
			MoveIgnoreActorRemove(FakeCarryCharacterMesh);
			GetWorld()->DestroyActor(FakeCarryCharacterMesh);
		}

		// Fake carry skeletal mesh for FP view, because the real character attached to you looks weird in FP
		FakeCarryCharacterMesh = GetWorld()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), CurrentCarryCharacter->GetActorTransform());
		if (FakeCarryCharacterMesh)
		{
			FakeCarryCharacterMesh->SetOwner(this);
			FakeCarryCharacterMesh->bNetLoadOnClient = false;
			FakeCarryCharacterMesh->SetReplicates(false);
			FakeCarryCharacterMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "CarryRest_FP");
			FakeCarryCharacterMesh->SetActorEnableCollision(false);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetSkeletalMesh(CurrentCarryCharacter->GetMesh()->SkeletalMesh);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetAnimation(CarrySlaveIdleLoop);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetRelativeTransform(FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, 0.0f, -70.0f)));
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetOnlyOwnerSee(true);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetCastShadow(false);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			FakeCarryCharacterMesh->GetSkeletalMeshComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

			MoveIgnoreActorAdd(FakeCarryCharacterMesh);
		}
		*/
	}
	else
	{
		/*
		if (FakeCarryCharacterMesh)
		{
			MoveIgnoreActorRemove(FakeCarryCharacterMesh);
			GetWorld()->DestroyActor(FakeCarryCharacterMesh);
		}
		*/
	}
	OnCarryingChanged.Broadcast();
}

void AReadyOrNotCharacter::ResetThrownByCharacter()
{
	ThrownByCharacter = nullptr;
}

void AReadyOrNotCharacter::ReactToCarryThrow()
{
	EnableRagdoll(0.65f);
}

void AReadyOrNotCharacter::OnRep_CurrentRagdollArrestCharacterChanged()
{
}

EAnimWeaponType AReadyOrNotCharacter::GetCurrentWeaponAnimType() const
{
	if (IsArrested() || IsArrestedAndDead() || IsCarried())
		return EAnimWeaponType::CWT_Arrested;

	if (IsSurrenderedFor(1.5f) || bSurrenderComplete)
		return EAnimWeaponType::CWT_Surrendered;

	if (const ABaseItem* EquippedItem = GetEquippedItem())
	{
		if (EquippedItem->ItemClass == EItemClass::IC_Melee)
			return EAnimWeaponType::CWT_Unarmed;
		
		if (EquippedItem->ContainsItemCategory(EItemCategory::IC_Primary))
			return EAnimWeaponType::CWT_Rifle;
		
		return EAnimWeaponType::CWT_Pistol;
	}

	return EAnimWeaponType::CWT_Unarmed;
}

UAnimMontage* AReadyOrNotCharacter::PlayMontageFromTable(const FString& Animation)
{
	if (Animation.IsEmpty() || Animation == "None")
    	return nullptr;

	if (UInteractionsData::IsPairedInteractionPlayingOn(this))
		return nullptr;

	if (LastMontageTableStreamableHandle.IsValid() && LastMontageTableStreamableHandle->IsLoadingInProgress())
		return nullptr;
    
    if (!GetMesh())
		return nullptr;
    
    // fail this because it will stop us getting up if another anim is called
    if (GetMesh()->IsSimulatingPhysics() && !Animation.Contains("tp_getup"))
    {
    	return nullptr;
    }

	// No montages allowed when dead
	if (IsDeadNotUnconscious())
	{
		return nullptr;
	}

	if (IsIncapacitated())
	{
		return nullptr;
	}

	if (bStartedPlayingDeath || bPlayingDeathMontage)
		return nullptr;
	
	// No montages allowed while being carried
	if (IsCarried())
		return nullptr;

	// No montages when ragdolling
	if (IsInRagdoll())
	{
		return nullptr;
	}

    if (IsPlayingNonInterruptibleMontage(Animation))
		return nullptr;

	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	//ULog::Info("Playing table animation: " + Animation);

    if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
    {
    	if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup", false))
    	{
    		EAnimWeaponType cwt = GetCurrentWeaponAnimType();
    		if (!LookupRow->AnimData.Find(cwt))
    		{
    			if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
    			{
    				// use any 
    				cwt = EAnimWeaponType::CWT_Any;
    			}
    			else
    			{
    				#if !UE_BUILD_SHIPPING
    				ULog::Warning("Attempting to play " + Animation + " but no animation specified for the active weapon type [" + AnimWeaponTypeToString(cwt) + "]");
    				#endif

    				return nullptr;
    			}
    		}

    		if (!LookupRow->bRestartIfAlreadyPlaying && IsTableMontagePlaying(Animation))
    			return nullptr;

    		if (GetEquippedItem())
    		{
    			for (const auto& k : LookupRow->OverrideAnimation)
    			{
    				if (GetEquippedItem()->IsA(k.Key))
    				{
    					return PlayMontageFromTable(k.Value);
    				}
    			}
    		}

    		if (LookupRow->bNoCanPlayWhileStrafing)
    		{
    			if (bIsStrafing)
    			{
    				return nullptr;
    			}
    		}
    		
    		if (LookupRow->bNoCanPlayWhileNotStrafing)
    		{
    			if (!bIsStrafing)
    			{
    				return nullptr;
    			}
    		}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
    		FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;
    		if (WeaponAnimData.AnimMontages.Num() > 0)
    		{
    			if (TableMontageActiveCooldowns.Find(Animation))
    			{
    				// cooldown still active
    				return nullptr;
    			}
    			
    			if (IsAny3PMontageActive())
    			{
    				if (LookupRow->bCanQueue)
    				{
     					TableMontageQueue.AddUnique(Animation);
    					return nullptr;
    				}
    			}
    			
    			if (LastMontageTableStreamableHandle.IsValid() && !LastMontageTableStreamableHandle->HasLoadCompleted())
    			{
    				return nullptr;
    			}
    			
    			UAnimMontage* AnimMontage = WeaponAnimData.AnimMontages[FMath::RandRange(0, WeaponAnimData.AnimMontages.Num() - 1)];
    			if (Animation.StartsWith("fp"))
    			{
    				Play1PMontageDeferred(AnimMontage, Animation);
    			}
    			else if (Animation.StartsWith("shared"))
    			{
    				Play3PMontageDeferred(AnimMontage, Animation);
    				Play1PMontageDeferred(AnimMontage, Animation);
    			}
    			else
    			{
    				Play3PMontageDeferred(AnimMontage, Animation);
    			}
    			
    			LastTableMontagePlayed = Animation;
    			
    			if (LookupRow->Cooldown > 0.0f)
    			{
    				TableMontageActiveCooldowns.Add(Animation, LookupRow->Cooldown);
    			}
    			
    			return AnimMontage;
    		}
    	}
    }
	
    return nullptr;
}

UAnimMontage* AReadyOrNotCharacter::PlayMontageFromTableWithIndex(const FString& Animation, int32 Index)
{
	// fail this because it will stop us getting up if another anim is called
	if (GetMesh()->IsSimulatingPhysics() && !Animation.Contains("tp_getup"))
		return nullptr;

	if (!GetMesh()->GetAnimInstance())
		return nullptr;

	if (IsPlayingNonInterruptibleMontage(Animation))
		return nullptr;

	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup"))
		{
			EAnimWeaponType cwt = GetCurrentWeaponAnimType();
			if (!LookupRow->AnimData.Find(cwt))
			{
				if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
				{
					// use any 
					cwt = EAnimWeaponType::CWT_Any;
				}
				else
				{
					#if !UE_BUILD_SHIPPING
					ULog::Warning("Attempting to play " + Animation + " but no animation specified for the active weapon type [" + AnimWeaponTypeToString(cwt) + "]");
					#endif

					return nullptr;
				}
			}

			if (!LookupRow->bRestartIfAlreadyPlaying && IsTableMontagePlaying(Animation))
				return nullptr;

			if (GetEquippedItem())
			{
				for (const auto& k : LookupRow->OverrideAnimation)
				{
					if (GetEquippedItem()->IsA(k.Key))
					{
						return PlayMontageFromTableWithIndex(k.Value, 0);
					}
				}
			}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			if (WeaponAnimData.AnimMontages.IsValidIndex(Index))
			{
				UAnimMontage* AnimMontage = WeaponAnimData.AnimMontages[Index];

				if (Animation.StartsWith("fp"))
				{
					Play1PMontageDeferred(AnimMontage, Animation);
				}
				else if (Animation.StartsWith("shared"))
				{
					Play1PMontageDeferred(AnimMontage, Animation);
					Play3PMontageDeferred(AnimMontage, Animation);
				}
				else
				{
					Play3PMontageDeferred(AnimMontage, Animation);
				}
				
				LastTableMontagePlayed = Animation;
				
				return AnimMontage;
			}
		}
	}
	
	return nullptr;
}

int32 AReadyOrNotCharacter::GetMontageAnimCountFromTable(const FString& Animation) const
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return 0;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup"))
		{
			EAnimWeaponType cwt = GetCurrentWeaponAnimType();
			if (!LookupRow->AnimData.Find(cwt))
			{
				if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
				{
					// use any 
					cwt = EAnimWeaponType::CWT_Any;
				}
				else
				{
					return 0;
				}
			}

			if (!LookupRow->bRestartIfAlreadyPlaying && IsTableMontagePlaying(Animation))
				return 0;

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			const FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			return WeaponAnimData.AnimMontages.Num();
		}
	}
	
	return 0;
}

bool AReadyOrNotCharacter::IsTableMontagePlaying(const FString& Animation) const
{
	UAnimMontage* const* MontagePtr = PlayedTableMontageMap3P.Find(Animation);
	if (!MontagePtr || !(*MontagePtr))
		return false;

	if (LastMontageTableStreamableHandle.IsValid() && LastMontageTableStreamableHandle->IsLoadingInProgress())
		return true;

	//V_LOGM(LogReadyOrNot, "Table Montage Playing? %s %d", *Animation, PlayedTableMontageMap[Animation] == GetMesh()->GetAnimInstance()->GetCurrentActiveMontage() || (GetMesh1P() && PlayedTableMontageMap[Animation] == GetMesh1P()->GetAnimInstance()->GetCurrentActiveMontage()));

	return GetMesh() && GetMesh()->GetAnimInstance() && GetMesh()->GetAnimInstance()->Montage_IsPlaying(*MontagePtr);
}

bool AReadyOrNotCharacter::IsAnyTableMontagePlaying() const
{
	for (auto k : PlayedTableMontageMap3P)
	{
		if (IsTableMontagePlaying(k.Key))
		{
			return true;
		}
	}
	
	return false;
}

bool AReadyOrNotCharacter::IsAnyTableMontagePlaying(FString& OutMontage) const
{
	for (auto k : PlayedTableMontageMap3P)
	{
		if (IsTableMontagePlaying(k.Key))
		{
			OutMontage = k.Key;
			return true;
		}
	}
	return false;
}

bool AReadyOrNotCharacter::IsTableMontage(UAnimMontage* Montage) const
{
	for (const auto& k : PlayedTableMontageMap3P)
	{
		if (k.Value == Montage)
			return true;
	}
	return false;
}

bool AReadyOrNotCharacter::IsTableMontagePlayingWithTimeRemaining(const FString& Animation, float& TimeRemaining) const
{
	TimeRemaining = 0.0f;

	if (!PlayedTableMontageMap3P.Find(Animation))
		return false;
	
	if (LastMontageTableStreamableHandle.IsValid() && LastMontageTableStreamableHandle->IsLoadingInProgress())
		return true;

	if (GetMesh())
	{
		if (const UAnimInstance* Instance = GetMesh()->GetAnimInstance())
		{
			if (UAnimMontage* CurrentMontage = Instance->GetCurrentActiveMontage())
			{
				if (PlayedTableMontageMap3P[Animation] == CurrentMontage)
				{
					TimeRemaining = CurrentMontage->GetPlayLength() - Instance->Montage_GetPosition(CurrentMontage);
					return true;
				}
			}
		}
	}
	
	return false;
}

bool AReadyOrNotCharacter::IsMontagePlayingWithTimeRemaining(const UAnimMontage* Animation, float& TimeRemaining) const
{
	TimeRemaining = 0.0f;

	if (!Animation)
		return false;
	
	if (LastMontageTableStreamableHandle.IsValid() && LastMontageTableStreamableHandle->IsLoadingInProgress())
		return true;

	if (GetMesh() && GetMesh()->GetAnimInstance() && Animation == GetMesh()->GetAnimInstance()->GetCurrentActiveMontage())
	{
		TimeRemaining = GetMesh()->GetAnimInstance()->GetCurrentActiveMontage()->GetPlayLength() - GetMesh()->GetAnimInstance()->Montage_GetPosition(GetMesh()->GetAnimInstance()->GetCurrentActiveMontage());
		return true;
	}
	
	return false;
}

void AReadyOrNotCharacter::StopTPMontageFromTable_Implementation(const FString& Animation, float BlendoutTime)
{
	if (UAnimMontage** FoundMontage = PlayedTableMontageMap3P.Find(Animation))
	{
		if (*FoundMontage)
		{
			StopTPMontage(*FoundMontage, BlendoutTime);
			//ULog::Info("Stopping table montage: " + Animation);
		}
		
		PlayedTableMontageMap3P.Remove(Animation);
	}
}

void AReadyOrNotCharacter::StopTPAnimMontage(UAnimMontage* AnimMontage)
{
	UAnimInstance* AnimInstance = (GetMesh()) ? GetMesh()->GetAnimInstance() : nullptr;
	const UAnimMontage* MontageToStop = (AnimMontage) ? AnimMontage : GetCurrentMontage();
	const bool bShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	if (bShouldStopMontage)
	{
		AnimInstance->Montage_Stop(MontageToStop->BlendOut.GetBlendTime(), MontageToStop);
	}
}

bool AReadyOrNotCharacter::IsPlayingNonInterruptibleMontage(const FString& MontageNameTryingToBePlayed) const
{
	if (!IsTableMontagePlaying(LastTableMontagePlayed))
		return false;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (const FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*LastTableMontagePlayed, "Animation Lookup"))
		{
			if (!LookupRow->bCanAnimationBeInterupted && !LookupRow->CanOnlyBeInteruptedBy.Contains(MontageNameTryingToBePlayed))
			{
				return true;
			}
		}
	}
	
	return false;
}

UAnimMontage* AReadyOrNotCharacter::GetMontageFromTable(const FString& Animation) const
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup"))
		{
			EAnimWeaponType cwt = GetCurrentWeaponAnimType();
			if (!LookupRow->AnimData.Find(cwt))
			{
				if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
				{
					cwt = EAnimWeaponType::CWT_Any;
				}
				else
				{
					return nullptr;
				}
			}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			if (WeaponAnimData.AnimMontages.Num() > 0)
			{
				return WeaponAnimData.AnimMontages[FMath::RandRange(0, WeaponAnimData.AnimMontages.Num() - 1)];
			}
		}
	}
	
	return nullptr;
}

UAnimMontage* AReadyOrNotCharacter::GetMontageFromTableWithIndex(const FString& Animation, const int32 Index) const
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup"))
		{
			EAnimWeaponType cwt = GetCurrentWeaponAnimType();
			if (!LookupRow->AnimData.Find(cwt))
			{
				if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
				{
					cwt = EAnimWeaponType::CWT_Any;
				}
				else
				{
					return nullptr;
				}
			}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			if (WeaponAnimData.AnimMontages.Num() > 0)
			{
				if (WeaponAnimData.AnimMontages.IsValidIndex(Index))
				{
					return WeaponAnimData.AnimMontages[Index];
				}
			}
			
			return nullptr;
		}
	}
	
	return nullptr;
}

bool AReadyOrNotCharacter::DoesMontageFromTableExist(const FString& Animation) const
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return false;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*Animation, "Animation Lookup"))
		{
			EAnimWeaponType cwt = GetCurrentWeaponAnimType();
			if (!LookupRow->AnimData.Find(cwt))
			{
				if (LookupRow->AnimData.Find(EAnimWeaponType::CWT_Any))
				{
					cwt = EAnimWeaponType::CWT_Any;
				}
				else
				{
					return false;
				}
			}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			const FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			return WeaponAnimData.AnimMontages.Num() > 0;
		}
	}
	
	return false;
}

void AReadyOrNotCharacter::StopTPMontage(UAnimMontage* AnimMontage, float BlendoutTime)
{
	if (!AnimMontage)
		return;
	
	Multicast_Stop3PMontage(AnimMontage, BlendoutTime);
	Multicast_Stop3PMontage_Implementation(AnimMontage, BlendoutTime);
}

FString AReadyOrNotCharacter::GetLastTableMontagePlayed() const
{
	return LastTableMontagePlayed;
}

TArray<FString> AReadyOrNotCharacter::GetTableMontageQueue() const
{
	return TableMontageQueue;
}

APairedInteractionDriver* AReadyOrNotCharacter::PlayPairedInteraction(UInteractionsData* InteractionData, AActor* Driver, AActor* Slave, ABaseItem* OptionalItem)
{
	#if WITH_EDITOR
	ensure(GetLocalRole() == ROLE_Authority);
	#endif
	
	if (GetLocalRole() < ROLE_Authority)
		return nullptr;

	return InteractionData->PlayPairedInteraction(Driver, Slave, OptionalItem);
}

void AReadyOrNotCharacter::Multicast_PauseAllAnims_Implementation(bool bPaused)
{
	// 	if (GetMesh()->GetAnimInstance())
	// 		GetMesh()->GetAnimInstance()->Montage_SetPlayRate(nullptr, 0.0f);

	GetMesh()->bPauseAnims = bPaused;


	//V_LOGM(LogReadyOrNot, "%s is dead.. stopping animations", *GetName());
}

void AReadyOrNotCharacter::Play3PMontage(UAnimMontage* NewMontage, const float StartTime, const float PlayRate)
{
	if (!NewMontage || !GetWorld())
		return;

	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return;
	
	// paired interactions cannot be overriden because they can involve 2 characters!
	if (!bStartedPlayingDeath) // allow death animations to bypass this
	{
		for (APairedInteractionDriver* Driver : GameState->AllPairedInteractionActors)
		{
			if (Driver)
			{
				if (Driver->IsPlayingAnimationForCharacter(this, NewMontage))
				{
					return;
				}
			}
		}
	}

	if (NewMontage == CurrentDeathMontage && !bStartedPlayingDeath)
		return;

	// Special Case
	if (Tags.Contains("PreviewCharacter"))
	{
		if (IsTableMontage(NewMontage))
		{
			Multicast_Play3PMontage_Implementation(NewMontage, StartTime,  PlayRate);
		}
		else
		{
			return;
		}
	}

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		Server_Play3PMontage(NewMontage, StartTime, PlayRate);
	}
}

void AReadyOrNotCharacter::Play3PMontageDeferred_Implementation(UAnimMontage* Montage, const FString& AnimationName)
{
	PlayedTableMontageMap3P.Add(AnimationName, Montage);
	Play3PMontage(Montage, 0.0f);
}

void AReadyOrNotCharacter::PlayLocal3PMontage(UAnimMontage* NewMontage, float PlayRate)
{
	if (!GetMesh()->GetAnimInstance())
		return;

	if (!NewMontage)
		return;

	if (Tags.Contains("PreviewCharacter") || GetTearOff() || !GetPlayerState())
	{
		if (Tags.Contains("PreviewCharacter") && !NewMontage->GetName().Contains("tp_pregame"))
		{
			return;
		}
	}

	GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate, EMontagePlayReturnType::Duration);
}

void AReadyOrNotCharacter::PlayNonLocal3PMontage(UAnimMontage* NewMontage, float PlayRate)
{
	Server_Play3PMontage(NewMontage, 0.0f, PlayRate);
}

void AReadyOrNotCharacter::Server_PlayNonLocal3PMontage_Implementation(UAnimMontage* NewMontage)
{
	Multicast_PlayNonLocal3PMontage_Implementation(NewMontage);
}

bool AReadyOrNotCharacter::Server_PlayNonLocal3PMontage_Validate(UAnimMontage* NewMontage)
{
	return true;
}

void AReadyOrNotCharacter::Multicast_PlayNonLocal3PMontage_Implementation(UAnimMontage* NewMontage)
{
	if (IsLocalPlayer())
		return;

	if (GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(NewMontage);
	}
}


void AReadyOrNotCharacter::Multicast_Stop3PMontage_Implementation(UAnimMontage* Montage, float BlendoutTime)
{
	if (GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(BlendoutTime, Montage);

		#if !UE_BUILD_SHIPPING
		/*
		if (Montage)
			ULog::Info("Stopped Montage: " + Montage->GetName());
		else
			ULog::Info("Stopped Active Montage: " + GetNameSafe(GetCurrentMontage()));
		*/
		#endif
	}
}

void AReadyOrNotCharacter::Multicast_Play3PMontage_Implementation(UAnimMontage* NewMontage, float StartTime, float PlayRate)
{
	if (!NewMontage)
		return;

	if (!GetMesh()->GetAnimInstance())
		return;
	
	//ULog::Info("Playing Montage: " + NewMontage->GetName());

	// make slower movement anims quicker
	// if (PlayRate == 1.0f)
	// {
	// 	float Vel = GetVelocity().Size();
	// 	float VelNormalized = UKismetMathLibrary::NormalizeToRange(Vel, 200.0f, 350.0f);
	// 	VelNormalized = FMath::Clamp(VelNormalized, 0.0f, 0.15f);
	// 	float AnimSpeed = 1.05f - (VelNormalized * 0.1f);
	// 	PlayRate = AnimSpeed;
	// }

	GetMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate, EMontagePlayReturnType::Duration, StartTime);
}

void AReadyOrNotCharacter::Server_Play3PMontage_Implementation(UAnimMontage* NewMontage, float StartTime, float PlayRate)
{
	if (!NewMontage)
		return;

	Multicast_Play3PMontage(NewMontage, StartTime, PlayRate);
	Multicast_Play3PMontage_Implementation(NewMontage, StartTime, PlayRate);
}

bool AReadyOrNotCharacter::Server_Play3PMontage_Validate(UAnimMontage* NewMontage, float StartTime, float PlayRate)
{
	return true;
}

bool AReadyOrNotCharacter::CanPlayDeathAnimation() const
{
	return true;
}

bool AReadyOrNotCharacter::PlayDeathAnimation()
{
	if (!HasAuthority())
		return false;

	//ULog::Info("Trying to play death animation...");

	Multicast_SavePoseSnapshot("DeathPoseStart");
	Multicast_SavePoseSnapshot_Implementation("DeathPoseStart");
	
	if (bPlayingDeathMontage || bStartedPlayingDeath)
	{
		return false;
	}
	
	CurrentDeathMontage = nullptr;

	if (IsInRagdoll())
		return false;

	if (!CanPlayDeathAnimation())
	{
		EnableRagdoll();
		return false;
	}
	
	ACyberneticController* CyberneticController = Cast<ACyberneticController>(GetController());
	bool bCanPlayFaint = CyberneticController && !CyberneticController->IsMoving() && !IsArrested() && !(IsSurrenderedFor(1.0f) || bSurrenderComplete); // Still can faint if early in surrender
	
	if (LastHitBoneName == "neck_1" && LastArterialHitBone == LastHitBoneName && IsDeadNotUnconscious())
	{
		CurrentDeathMontage = GetMontageFromTable("tp_bleedout_neck");
	}
	else if (bCanPlayFaint && LastHitBoneName.IsNone() && !LastArterialHitBone.IsNone())
	{
		CurrentDeathMontage = GetMontageFromTable("tp_bleedout_faint");
	}
	else if (HeadBones.Contains(LastHitBoneName))
	{
		CurrentDeathMontage = GetMontageFromTable("tp_death_head");
	}
	else if (L_Arm.Contains(LastHitBoneName))
	{
		CurrentDeathMontage = GetMontageFromTable("tp_death_l_arm");
	}
	else if (R_Arm.Contains(LastHitBoneName))
	{
		CurrentDeathMontage = GetMontageFromTable("tp_death_r_arm");
	}
	else if (L_Leg.Contains(LastHitBoneName) || R_Leg.Contains(LastHitBoneName))
	{
		CurrentDeathMontage = GetMontageFromTable("tp_death_leg");
	}
	else if (Torso.Contains(LastHitBoneName))
	{
		CurrentDeathMontage = GetMontageFromTable("tp_death_torso");
	}

	if (bIsPairedInteractionPlaying)
	{
		CurrentDeathMontage = nullptr;
		bStartedPlayingDeath = true;
		EnableRagdoll();
		return false;
	}

	// if no death montages found simply ragdoll
	if (!CurrentDeathMontage)
	{
		V_LOGM(LogReadyOrNot, "-------------- NO DEATH MOTION FOUND RAGDOLLING");
		EnableRagdoll();
		return false;
	}

	#if WITH_EDITOR
	V_LOGM(LogReadyOrNot, "--------------- PLAYING: %s", *CurrentDeathMontage->GetName());
	#endif

	bHasBeenReported = false;

	Multicast_PlayDeathAnimation(CurrentDeathMontage);
	
	return true;
}

void AReadyOrNotCharacter::Multicast_PlayDeathAnimation_Implementation(UAnimMontage* Montage)
{
	CurrentDeathMontage = Montage;

	if (!CurrentDeathMontage)
		return;

	ThrowEquippedItem();
	
	bStartedPlayingDeath = true;
	bBlendInPhysics = true;

	float BlendOutTime = 0.25f;
	if (GetCurrentMontage())
		BlendOutTime = GetCurrentMontage()->BlendOut.GetBlendTime();
	
	SetAppropriatePhysicsAsset(true);
	
	Multicast_Stop3PMontage_Implementation(nullptr, BlendOutTime);
	Multicast_Play3PMontage_Implementation(CurrentDeathMontage, 0.0f, 1.0f);
}

void AReadyOrNotCharacter::StopDeathAnimation()
{
	if (CurrentDeathMontage)
	{
		StopTPMontage(CurrentDeathMontage);
	}
	
	CurrentDeathMontage = nullptr;
	bPlayingDeathMontage = false;
	bStartedPlayingDeath = false;
}

void AReadyOrNotCharacter::OnBlendRagdollAnimFinished()
{
	EnableRagdoll();
	
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
			if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(IncapMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference))
			{
				IncapacitationLoopAnim = AnimSequence;
			}
		}
		
		if (UAnimMontage* CurrentMontage = GetCurrentMontage())
		{
			const float CurrentMontagePosition = GetMesh()->GetAnimInstance()->Montage_GetPosition(nullptr);
			
			IncapacitationBlendTime = FMath::Max(CurrentMontage->GetPlayLength() - CurrentMontagePosition, 0.0f) * 2.0f;
		}
		else
		{
			IncapacitationBlendTime = 0.0f;
		}

		if (!IncapacitationLoopAnim)
		{
			bStartBlendInIncapacitation = false;
		}
	}
}

UReadyOrNotWeaponAnimData* AReadyOrNotCharacter::GetCurrentWeaponAnimData() const
{
	return InventoryComp->GetEquippedItem() ? InventoryComp->GetEquippedItem()->AnimationData : nullptr;
}

bool AReadyOrNotCharacter::Is3PMontagePlaying(const UAnimMontage* Montage) const
{
	if (!Montage)
		return false;
	
	if (GetMesh()->GetAnimInstance())
	{
		return GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage);
	}
	
	return false;
}

bool AReadyOrNotCharacter::IsAny3PMontageActive() const
{
	if (const USkeletalMeshComponent* SkMesh = GetMesh())
	{
		if (const UAnimInstance* Instance = SkMesh->GetAnimInstance())
		{
			return Instance->MontageInstances.Num() > 0;
		}
	}
	
	return false;
}

bool AReadyOrNotCharacter::IsMontageSlotPlaying(FName SlotName) const
{
	if (GetMesh())
	{
		if (const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			const float SlotWeight = AnimInstance->GetSlotMontageGlobalWeight(SlotName);
			return SlotWeight >= 1.0f || FMath::IsNearlyEqual(SlotWeight, 1.0f, 0.001f);
		}
	}

	return false;
}

bool AReadyOrNotCharacter::IsFullBodyMontagePlaying() const
{
	return IsMontageSlotPlaying("DefaultSlot");
}

bool AReadyOrNotCharacter::IsUpperBodyMontagePlaying() const
{
	return IsMontageSlotPlaying("Upperbody");
}

void AReadyOrNotCharacter::Play1PMontageDeferred_Implementation(UAnimMontage* Montage, const FString& AnimationName)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::Play1PMontage(UAnimMontage* NewMontage, float PlayRate)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::Play1PMontage_NonClient(UAnimMontage* NewMontage, float PlayRate)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::PlayLocal1PMontage(UAnimMontage* NewMontage, float PlayRate)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::StopFPAnimMontage(UAnimMontage* AnimMontage, float BlendoutTime)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::Client_Play1PMontage_Implementation(UAnimMontage* NewMontage, float PlayRate)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::Multicast_Stop1PMontage_Implementation(UAnimMontage* Montage, float BlendoutTime)
{
	//Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::OnRep_MeshReplicated()
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
				i->GetItemMesh()->EmptyOverrideMaterials();
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

	SkinnedDecalSampler->ClearAllDecals();
	SkinnedDecalSampler->SetupMaterials();
}

void AReadyOrNotCharacter::UpdateOverridesFromCharacterLookOverrideDataTable(FString LookOverride)
{
	//reset character look override
	CharacterLookOverride = FCharacterLookOverride();
	Rep_CharacterLookOverride = LookOverride;

	if (LookOverride.IsEmpty())
		return;
	
	if (const UDataTable* dt = UBpGameplayHelperLib::GetCharacterLookOverrideDataTable())
	{
		if (const FCharacterLookOverride* LookupRow = dt->FindRow<FCharacterLookOverride>(*LookOverride, "Character Override Lookup", false))
		{
			CharacterLookOverride = *LookupRow;
			ArmorOverrideMapTP = CharacterLookOverride.ArmorOverrideMap;
			SpeechCharacterName = CharacterLookOverride.SpeechCharacterName.IsEmpty() ? "" : CharacterLookOverride.SpeechCharacterName;
			Server_ChangeTPMesh(CharacterLookOverride.BodyMeshOverride, CharacterLookOverride.FaceMeshOverride);
		}
	}

	OnRep_MeshReplicated();
}

USkeletalMesh* AReadyOrNotCharacter::GetTPMeshOverride(TSubclassOf<ABaseArmour> InArmourClass, bool& bFound)
{
	bFound = false;
	
	for (const auto& It : ArmorOverrideMapTP)
	{
		if (It.Key == InArmourClass)
		{
			bFound = true;
			return It.Value;
		}
	}
	
	/*if (ArmorOverrideMapTP.Find(InArmourClass))
	{
		bFound = true;
		return ArmorOverrideMapTP[InArmourClass];
	}*/
	
	return nullptr;
}

void AReadyOrNotCharacter::ForceMeshUsingOverride_Implementation(USkeletalMesh* InFPMesh, USkeletalMesh* InTPMesh,
                                                                 USkeletalMesh* InFaceMesh)
{
	if (InFPMesh)
	{
		CharacterLookOverride.FPMeshOverride = InFPMesh;
	}
	if (InTPMesh)
	{
		CharacterLookOverride.BodyMeshOverride = InTPMesh;
	}
	if (InFaceMesh)
	{
		CharacterLookOverride.FaceMeshOverride = InFaceMesh;
	}
	OnRep_MeshReplicated();
}

void AReadyOrNotCharacter::Server_ChangeTPMesh_Implementation(USkeletalMesh* Body, USkeletalMesh* Face)
{
	
}

bool AReadyOrNotCharacter::Server_ChangeTPMesh_Validate(USkeletalMesh* Body, USkeletalMesh* Face)
{
	return true;
}

bool AReadyOrNotCharacter::CanReportNow_Implementation()
{
	if (bHasBeenReported)
	{
		return false;
	}

	if (!IsDeadOrUnconscious() && !IsIncapacitated() && !bArrestComplete)
	{
		return false;
	}
	
	return true;
}

FString AReadyOrNotCharacter::GetSpeechTypeForReport_Implementation()
{
	if (GetTeam() == ETeamType::TT_CIVILIAN)
	{
		if (IsDeadNotUnconscious())
		{
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN;
		}
		
		if (IsIncapacitated())
		{
			return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN;
		}
		
		if (bArrestComplete)
		{
			return VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_CIVILIAN;
		}
	}
	else if (GetTeam() == ETeamType::TT_SUSPECT)
	{
		if (IsDeadNotUnconscious())
		{
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT;
		}
		
		if (IsIncapacitated())
		{
			return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT;
		}
		
		if (bArrestComplete)
		{
			return VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_SUSPECT;
		}
	}
	else if (GetTeam() == ETeamType::TT_SERT_BLUE || GetTeam() == ETeamType::TT_SERT_RED)
	{
		if (IsDeadOrUnconscious())
		{
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_SWAT;
		}
		
		return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SWAT;
	}

	return IsDeadNotUnconscious() ? VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN : VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN;
}

void AReadyOrNotCharacter::ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation)
{
	if (!Reporter)
		return;
	
	if (IsDeadOrUnconscious() || IsIncapacitated())
	{
		Client_PlayFMODEvent2D(Reporter->ReportPlayerDeadFMODEvent);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);	
	}
	else if (IsArrested())
	{
		Client_PlayFMODEvent2D(Reporter->ReportPlayerArrestedFMODEvent);
	}
	else
	{
		Client_PlayFMODEvent2D(Reporter->ReportPlayerGeneralFMODEvent);
	}

	if (Reporter->GetEquippedItem() && Reporter->GetEquippedItem()->AnimationData && bPlayAnimation)
	{
		Reporter->Client_Play1PMontage(Reporter->GetEquippedItem()->AnimationData->RadioSelect.Body_FP);
		Reporter->Play3PMontage(Reporter->GetEquippedItem()->AnimationData->RadioSelect.Body_TP);
	}
	
	bHasBeenReported = true;

	#if !UE_BUILD_SHIPPING
	ULog::Info(GetName() + " reported by " + Reporter->GetName());
	#endif
}

void AReadyOrNotCharacter::GetReportableFMODEvents(UFMODEvent*& OutDeadFMODEvent, UFMODEvent*& OutArrestedFMODEvent, UFMODEvent*& OutGeneralFMODEvent)
{
	OutDeadFMODEvent = ReportPlayerDeadFMODEvent;
	OutArrestedFMODEvent = ReportPlayerArrestedFMODEvent;
	OutGeneralFMODEvent = ReportPlayerGeneralFMODEvent;
}

void AReadyOrNotCharacter::OnCarryPickupComplete(AActor* Driver, AActor* Slave)
{
	const AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);
	
	// We are the carrier
	if (DriverCharacter == this)
	{
		OnCarryPickupComplete_Slave(Slave);
	}
}

void AReadyOrNotCharacter::OnCarryPickupComplete_Driver(AActor* Driver)
{
}

void AReadyOrNotCharacter::OnCarryPickupComplete_Slave(AActor* Slave)
{
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);

	if (SlaveCharacter)
	{
		SlaveCharacter->CarriedByCharacter = this;
		CurrentCarryCharacter = SlaveCharacter;
			
		SlaveCharacter->SetOwner(this);
		SlaveCharacter->LockMovementAndActions();

		if (!SlaveCharacter->IsDeadOrUnconscious() && !SlaveCharacter->IsIncapacitated())
		{
			SlaveCharacter->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_PICKED_UP);
		}

		OnRep_CurrentCarryCharacterChanged();
	}
}

void AReadyOrNotCharacter::OnCarryDropComplete(AActor* Driver, AActor* Slave)
{
	OnCarryDropComplete_Driver(Driver);
	OnCarryDropComplete_Slave(Slave);
}

void AReadyOrNotCharacter::OnCarryDropComplete_Driver(AActor* Driver)
{
	const AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);
	
	// We are the carrier
	if (DriverCharacter == this)
	{
		CurrentCarryCharacter = nullptr;
		DriverCharacter->GetInventoryComponent()->EquipLastEquippedWeapon(true);
		
		bCarryingDead = false;

		OnRep_CurrentCarryCharacterChanged();
	}
}

void AReadyOrNotCharacter::OnCarryDropComplete_Slave(AActor* Slave)
{
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);

	if (SlaveCharacter)
	{
		SlaveCharacter->GetMesh()->SetOwnerNoSee(false);
		SlaveCharacter->GetMesh()->SetCastHiddenShadow(false);
		SlaveCharacter->SetOwner(nullptr);
		SlaveCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
		SlaveCharacter->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
		SlaveCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
		SlaveCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SlaveCharacter->SetActorRotation(FRotator(0.0f, SlaveCharacter->GetActorRotation().Yaw, 0.0f));

		MoveIgnoreActorRemove(SlaveCharacter);
		SlaveCharacter->MoveIgnoreActorRemove(SlaveCharacter->CarriedByCharacter);

		SlaveCharacter->bIsBeingCarried = false;
		SlaveCharacter->CarriedByCharacter = nullptr;

		if (IsCivilian())
			SlaveCharacter->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_IMMUNE);
	}
}

void AReadyOrNotCharacter::OnCarryThrowComplete(AActor* Driver, AActor* Slave)
{
	OnCarryThrowComplete_Driver(Driver);
	OnCarryThrowComplete_Slave(Slave);
}

void AReadyOrNotCharacter::OnCarryThrowComplete_Driver(AActor* Driver)
{
	const AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);

	// We are the carrier
	if (DriverCharacter == this)
	{
		MoveIgnoreActorRemove(CurrentCarryCharacter);

		CurrentCarryCharacter = nullptr;
		DriverCharacter->GetInventoryComponent()->EquipLastEquippedWeapon(true);

		bCarryingDead = false;
		
		OnRep_CurrentCarryCharacterChanged();
	}
}

void AReadyOrNotCharacter::OnCarryThrowComplete_Slave(AActor* Slave)
{
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);
	
	if (SlaveCharacter)
	{
		if (SlaveCharacter->CarriedByCharacter == this)
		{
			SlaveCharacter->GetMesh()->SetOwnerNoSee(false);
			SlaveCharacter->GetMesh()->SetCastHiddenShadow(false);
			
			SlaveCharacter->SetOwner(nullptr);

			//SlaveCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//SlaveCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
			SlaveCharacter->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
			SlaveCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
			SlaveCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

			SlaveCharacter->MoveIgnoreActorRemove(SlaveCharacter->CarriedByCharacter);

			SlaveCharacter->ReactToCarryThrow();

			SlaveCharacter->bIsBeingCarried = false;
			SlaveCharacter->ThrownByCharacter = SlaveCharacter->CarriedByCharacter;
			SlaveCharacter->CarriedByCharacter = nullptr;
		}
	}
}

bool AReadyOrNotCharacter::CanBePickedUp() const
{
	if (IsDeadOrUnconscious() || IsInRagdoll() || IsBeingArrested())
		return false;

	if (!CarryArrestedInteractionData || !DropArrestedInteractionData || !ThrowArrestedInteractionData)
		return false;
		
	if (!bHasBeenReported)
		return false;

	if (!IsArrested())
		return false;

	if (IsStunned() || IsTableMontagePlaying("tp_bashed"))
		return false;

	if (IsCarried() || IsBeingCarried())
		return false;

	if (IsValid(CarriedByCharacter))
		return false;

	if (PendingCarryCharacter && PendingCarryCharacter != this)
		return false;
	
	if (IsGettingUp())
		return false;

	if (GetWorld()->GetGameState<ALobbyGS>())
		return false;
	
	return true;
}

bool AReadyOrNotCharacter::CanCarryCharacter(AReadyOrNotCharacter* CharacterToCarry) const
{
	if (!IsValid(CharacterToCarry))
		return false;

	if (!Cast<ACyberneticCharacter>(this)) // ew.. but i dont wanna make a damn virtual function
	{
		if (CharacterToCarry->IsDeadOrUnconscious() || CharacterToCarry->IsBeingArrested() || CharacterToCarry->IsInRagdoll())
			return false;
	}

	if (IsStunned())
		return false;

	if (!(CharacterToCarry->IsArrested() || CharacterToCarry->IsArrestedAndDead() || CharacterToCarry->IsInRagdoll() || CharacterToCarry->IsDeadOrUnconscious() || CharacterToCarry->IsIncapacitated()))
		return false;

	// Can't carry yourself :P
	if (CharacterToCarry == this)
		return false;

	if (IsCarrying() || IsBeingCarried())
		return false;

	// If already carried by someone, don't allow carry
	if (IsValid(CharacterToCarry->CarriedByCharacter))
		return false;

	// Must be the same person who is preparing to carry
	if (CharacterToCarry->PendingCarryCharacter && CharacterToCarry->PendingCarryCharacter != this)
		return false;

	if (!CarryArrestedInteractionData)
		return false;

	if (GetWorld()->GetGameState<ALobbyGS>())
		return false;

	if (UInteractionsData::IsPairedInteractionPlayingOn(this))
		return false;
	
	return true;
}

bool AReadyOrNotCharacter::CanDropCharacter(AReadyOrNotCharacter* CharacterToDrop) const
{
	if (!IsValid(CharacterToDrop))
		return false;

	if (!Cast<ACyberneticCharacter>(this)) // ew.. but i dont wanna make a damn virtual function
	{
		if (CharacterToDrop->IsDeadOrUnconscious() || CharacterToDrop->IsBeingArrested() || CharacterToDrop->IsInRagdoll())
			return false;
	}

	// Can't drop yourself :P
	if (CharacterToDrop == this)
		return false;

	if (!CharacterToDrop->IsCarried() || !IsCarrying())
		return false;

	if (!DropArrestedInteractionData)
		return false;

	if (UInteractionsData::IsPairedInteractionPlayingOn(this))
		return false;
	
	return true;
}

void AReadyOrNotCharacter::Server_CarryArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter)
{
	if (!CanCarryCharacter(ArrestedCharacter))
		return;

	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(ArrestedCharacter);

	const FCarryArrestedAnimState CarryArrestAnimState_Master =
	{
		CarryMasterIdleLoop
	};
	
	const FCarryArrestedAnimState CarryArrestAnimState_Slave =
	{
		CarrySlaveIdleLoop
	};

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ArrestedCharacter->PendingCarryCharacter = nullptr;
	ArrestedCharacter->LockMovementAndActions();
	ArrestedCharacter->Rep_CarryArrestedAnimState = CarryArrestAnimState_Slave;

	Rep_CarryArrestedAnimState = CarryArrestAnimState_Master;
	CarriedByCharacter = nullptr;

	bCarryingDead = ArrestedCharacter->IsDeadOrUnconscious() || ArrestedCharacter->IsIncapacitated() || ArrestedCharacter->IsInRagdoll() || ArrestedCharacter->IsArrestedAndDead();

	CarryArrestedInteractionData->bAllowDeadSlaveInteraction = true; // todo: remove this and make true in the data asset instead
	if (APairedInteractionDriver* CarryInteractionDriver = PlayPairedInteraction(CarryArrestedInteractionData, this, ArrestedCharacter, nullptr))
	{
		ArrestedCharacter->bIsBeingCarried = true;
		CarryInteractionDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryPickupComplete_Driver);
		CarryInteractionDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryPickupComplete_Slave);
	}
	else
	{
		ArrestedCharacter->bIsBeingCarried = false;
	}

	CurrentCarryConfirmTime = 0.0f;
	PendingCarryCharacter = nullptr;
}

void AReadyOrNotCharacter::CarryArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter)
{
#ifdef CARRY_ARRESTED
	/*
	if (FakeCarryCharacterMesh)
	{
		GetWorld()->DestroyActor(FakeCarryCharacterMesh);
		FakeCarryCharacterMesh = nullptr;
	}
	*/

	Server_CarryArrestedTarget(ArrestedCharacter);
#endif
}

void AReadyOrNotCharacter::DropArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter)
{
#ifdef CARRY_ARRESTED
	/*
	if (FakeCarryCharacterMesh)
	{
		GetWorld()->DestroyActor(FakeCarryCharacterMesh);
		FakeCarryCharacterMesh = nullptr;
	}
	*/

	Server_DropArrestedTarget(ArrestedCharacter);
#endif
}

void AReadyOrNotCharacter::ThrowArrestedTarget(AReadyOrNotCharacter* ArrestedCharacter)
{
#ifdef CARRY_ARRESTED
	/*
	if (FakeCarryCharacterMesh)
	{
		GetWorld()->DestroyActor(FakeCarryCharacterMesh);
		FakeCarryCharacterMesh = nullptr;
	}
	*/

	Server_ThrowArrestedTarget(ArrestedCharacter);
#endif
}

void AReadyOrNotCharacter::Server_DropArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter)
{
#ifdef CARRY_ARRESTED
	if (!CanDropCharacter(ArrestedCharacter))
		return;
		
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(ArrestedCharacter);

	ArrestedCharacter->GetMesh()->SetOwnerNoSee(false);
	ArrestedCharacter->GetMesh()->SetCastHiddenShadow(false);
	ArrestedCharacter->SetOwner(nullptr);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	ArrestedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	ArrestedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
	
	ArrestedCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	//ArrestedCharacter->SetActorRotation(FRotator(0.0f, ArrestedCharacter->GetActorRotation().Yaw, 0.0f));

	DropArrestedInteractionData->bAllowDeadSlaveInteraction = true; // todo: remove this and make true in the data asset instead
	if (APairedInteractionDriver* CarryInteractionDriver = PlayPairedInteraction(DropArrestedInteractionData, this, ArrestedCharacter, nullptr))
	{
		CarryInteractionDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryDropComplete_Driver);
		CarryInteractionDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryDropComplete_Slave);
		//CarryInteractionDriver->Event_OnPairedInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryDropComplete);
	}
#endif
}

void AReadyOrNotCharacter::Server_ThrowArrestedTarget_Implementation(AReadyOrNotCharacter* ArrestedCharacter)
{
#ifdef CARRY_ARRESTED
	if (!CanDropCharacter(ArrestedCharacter))
		return;
	
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(ArrestedCharacter);

	ArrestedCharacter->GetMesh()->SetOwnerNoSee(false);
	ArrestedCharacter->GetMesh()->SetCastHiddenShadow(false);
	ArrestedCharacter->GetMesh()->SetAllBodiesSimulatePhysics(false);
	
	ArrestedCharacter->SetOwner(nullptr);

	ArrestedCharacter->GetCapsuleComponent()->SetSimulatePhysics(false);
#if defined(PLATFORM_XB1) || defined(PLATFORM_PS4)
	ArrestedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_NavWalking);
#else
	ArrestedCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
#endif
	ArrestedCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ArrestedCharacter->SetActorRotation(FRotator(0.0f, ArrestedCharacter->GetActorRotation().Yaw, 0.0f));

	ArrestedCharacter->ThrownByCharacter = this;
	
	if (APairedInteractionDriver* CarryInteractionDriver = PlayPairedInteraction(ThrowArrestedInteractionData, this, ArrestedCharacter, nullptr))
	{
		CarryInteractionDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryThrowComplete_Driver);
		CarryInteractionDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryThrowComplete_Slave);

		//CarryInteractionDriver->Event_OnPairedInteractionFinished.AddDynamic(this, &AReadyOrNotCharacter::OnCarryThrowComplete);
	}
#endif
}

void AReadyOrNotCharacter::Server_ReportTarget_Implementation(AActor* Character)
{
	if (!Character || !IsValid(this))
	{
		return;
	}
	
	if (!Execute_CanReportNow(Character))
	{
		return;
	}
	
	const AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!gs)
		return;

	if (gs->bPvPMode)
	{
		if (AReadyOrNotCharacter* PlayerCharacter = Cast<AReadyOrNotCharacter>(Character))
        {
            if (AReadyOrNotPlayerState* PS = GetPlayerState<AReadyOrNotPlayerState>())
            {
            	if (!PS->bIsVIP)
            	{
            		if (!PlayerCharacter->bHasBeenReported)
            		{
            			ENQUEUE_INGAMELOG_MESSAGE_PVP({Cast<AReadyOrNotCharacter>(PlayerCharacter->KilledBy), PlayerCharacter, EPVPEvent::PlayerKilled, FText::FromString(""), PlayerCharacter->DeathReason});
            			PlayerCharacter->bHasBeenReported = true;
    
            			PS->PointsFromReportingKills++;
    
            			#if WITH_EDITOR
            			ULog::Number(PS->PointsFromReportingKills, "PointsFromReportingKills: ");
            			#endif
    
            			UFMODBlueprintStatics::PlayEvent2D(this, ReportPlayerDeadFMODEvent, true);
    
            			Client_PlayScreenShake(ReportToTOC_PVP_CameraShake);
            		}
            		else if (PlayerCharacter->IsArrested() && !PlayerCharacter->bHasBeenReported)
            		{
            			ENQUEUE_INGAMELOG_MESSAGE_PVP({Cast<AReadyOrNotCharacter>(PlayerCharacter->ArrestedBy), PlayerCharacter, EPVPEvent::PlayerArrested});
            			PlayerCharacter->bHasBeenReported = true;
    
            			PS->PointsFromReportingArrests++;
            	
            			#if WITH_EDITOR
            			ULog::Number(PS->PointsFromReportingArrests, "PointsFromReportingArrests: ");
            			#endif
    
            			UFMODBlueprintStatics::PlayEvent2D(this, ReportPlayerArrestedFMODEvent, true);
    
            			Client_PlayScreenShake(ReportToTOC_PVP_CameraShake);
            		}
            	}
            }
        }
	}
	else
	{
		Server_ReportToTOC(Character);
	}	
}

void AReadyOrNotCharacter::Server_ReportToTOC_Implementation(AActor* Actor, bool bPlayAnimation, bool bTocResponse)
{
	if (bHasBeenReported)
		return;
	
	Execute_ReportToTOC(Actor, this, bPlayAnimation);

	if (bPlayAnimation)
		Multicast_OnTargetReported();
		
	const FString SpeechType = Execute_GetSpeechTypeForReport(Actor);
	if (SpeechType.IsEmpty())
		return;
    
	FString TocLine = "";
    
	if (SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_DEAD_SWAT) || SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT) || SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN))
	{
		TocLine = VO_TOC::TOC_DEATH;
	}
	else if (SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_SUSPECT) || SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_CIVILIAN))
	{
		TocLine = VO_TOC::TOC_ARREST;
	}
	else if (SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SWAT) || SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT) || SpeechType.Contains(VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN))
	{
		TocLine = VO_TOC::TOC_INCAPACITATED;
	}

	if (AReadyOrNotCharacter* ReportCharacter = Cast<AReadyOrNotCharacter>(Actor))
	{
		if (ReportCharacter->bPendingROEViolateResponseOnReport)
		{
			ReportCharacter->bPendingROEViolateResponseOnReport = false;
			TocLine = VO_TOC::TOC_ROE_VIOLATE;
		}
	}

	TOCResponseLine = "";
	
	PlayRawVO(VO_SWAT_GENERAL::CALL_TOC);
	
	if (VoiceSoundSource)
	{
		AReadyOrNotCharacter* This = this;
		VoiceSoundSource->OnProgrammerSoundLengthReady.AddWeakLambda(This, [=](float Length)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(This, FTimerDelegate::CreateUFunction(This, "PlayReportSpeech", SpeechType, TocLine), Length);
		});
	}
	else
	{
		PlayReportSpeech(SpeechType, bTocResponse ? TocLine : "");
	}
}

void AReadyOrNotCharacter::Multicast_OnTargetReported_Implementation()
{
	PlayRadioSelectAnimation();
}

void AReadyOrNotCharacter::Server_PlayTOCConversation_Implementation()
{
	if (!TOCResponseLine.IsEmpty())
	{
		PlayTOCResponse(TOCResponseLine, true, ETOCPriority::ETP_LowPriority);
		TOCResponseLine = "";
	}
}

void AReadyOrNotCharacter::PlayTOCResponse(const FString Line, const bool bIsNetworked, const ETOCPriority Priority, const bool bCanPrefix, const float Delay)
{
	if (ATOCManager* TOC = ATOCManager::Get())
	{
		if (Delay > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::PlayTOCResponse, Line, bIsNetworked, Priority, bCanPrefix, -1.0f), Delay, false);
		}
		else
		{
			TOC->bCanPlayPrefix = bCanPrefix;
			TOC->StartTOCResponse(Line, bIsNetworked, Priority);
		}
	}
}

void AReadyOrNotCharacter::PlayROEViolateTOCResponse()
{
	if (const ATOCManager* TOC = ATOCManager::Get())
	{
		if (!bPendingROEViolateResponse)
		{
			if (!TOC->IsTOCSpeakingLine(VO_TOC::TOC_ROE_VIOLATE))
			{
				PlayTOCResponse(VO_TOC::TOC_ROE_VIOLATE, true, ETOCPriority::ETP_Flush, false, TOC->ROEViolateTOCResponseDelay);
				
				if (TOC->ROEViolateTOCResponseDelay > 0.0f)
				{
					bPendingROEViolateResponse = true;
					UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotCharacter::ResetROEViolateResponseFlag, TOC->ROEViolateTOCResponseDelay, false);
				}
				else
				{
					bPendingROEViolateResponse = false;
				}
			}
			else
			{
				bPendingROEViolateResponse = false;
			}
		}
	}
}

void AReadyOrNotCharacter::ResetROEViolateResponseFlag()
{
	bPendingROEViolateResponse = false;
}

void AReadyOrNotCharacter::RagdollArrestTarget(AReadyOrNotCharacter* RagdollCharacter)
{
	if (!RagdollCharacter)
		return;

	if (!RagdollCharacter->IsInRagdoll())
		return;
	
	if (RagdollCharacter->bPlayingDeathMontage)
		return;

	DoArrestWithZipcuffs(RagdollCharacter);

	RagdollCharacter->PendingRagdollArrestCharacter = nullptr;
	
	CurrentRagdollArrestConfirmTime = 0.0f;
	PendingRagdollArrestCharacter = nullptr;
}

bool AReadyOrNotCharacter::CanArrestRagdoll() const
{
	if (IsOnSWATTeam())
		return false;

	if (IsInRagdoll())
	{
		if (IsDeadNotUnconscious() || IsIncapacitated())
		{
			if (!bHasBeenReported)
				return false;
		}

		// Don't want to arrest an invisible corpse
		if (bIsInBitsAndPieces)
			return false;
	
		return !bArrestComplete;
	}
		
	return false;
}

void AReadyOrNotCharacter::Multicast_PlayRawVO_Implementation(const FString& SpecificFile, const FString& OverrideSpeakerName, bool bIgnoreIfAlreadyPlaying)
{
	if (bCannotSpeak)
		return;
	
	if (VoiceSoundSource && VoiceSoundSource->bIsRunning && bIgnoreIfAlreadyPlaying)
		return;
	
	if (IsDeadOrUnconscious())
		return;
	
	const FString SpeakerName = OverrideSpeakerName.IsEmpty() ? GetSpeechCharacterName() : OverrideSpeakerName;
	if (SpeakerName == "None" || SpeakerName.IsEmpty())
	{
		return;
	}
	
	if (UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get())
	{
		FString OutFileName, OutFilePath;
		if (VoiceConfig->GetSpecificVoiceLine(SpecificFile, SpeakerName, OutFilePath, OutFileName))
		{
			if(VoiceSoundSource)
			{
				VoiceSoundSource->Stop();
			}
			VoiceSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), nullptr, FTransform(FRotator(), GetMesh()->GetSocketLocation("head"), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
			VoiceSoundSource->Attach(GetMesh(), "head");
			VoiceSoundSource->OnEventStopped.AddDynamic(this, &AReadyOrNotCharacter::OnVoiceAudioStopped);
			
			VoiceSoundSource->Event = GetAppropriateVoiceLineEvent();
			SetupVoiceLineParameters(SpecificFile);
			VoiceSoundSource->SetProgrammerSoundName(OutFilePath);
			VoiceSoundSource->bWantsProgrammerSoundLength = true;
			VoiceSoundSource->Play();
			
			PlayVoiceOverSubtitles(VoiceSoundSource, SpeakerName, OutFileName);
		}
	}
}

bool AReadyOrNotCharacter::PlayRawVO(const FString& VoiceLine, const FString& OverrideSpeakerName, bool bIgnoreIfAlreadyPlaying)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayRawVO)

	if (UBpGameplayHelperLib::IsVoiceOverSuspended(GetWorld()))
		return false;
	
	if (VoiceSoundSource && VoiceSoundSource->bIsRunning && bIgnoreIfAlreadyPlaying)
	{
		return false;
	}

	if (!CanPlayVO(VoiceLine))
		return false;
	
	const FString SpeakerName = OverrideSpeakerName.IsEmpty() ? GetSpeechCharacterName() : OverrideSpeakerName;
	if (SpeakerName == "None" || SpeakerName.IsEmpty())
	{
		return false;
	}

	if (UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get())
	{
		FString OutFileName, OutFilePath;

		FString ChosenVoiceline = VoiceLine;
		FString MutatedVoiceline = MutateVoiceline(VoiceLine);

		bool bVoiceLineFound;

		// always use a speaker in shipping
		#if UE_BUILD_SHIPPING
		bVoiceLineFound = VoiceConfig->GetRandomVoiceLine(MutatedVoiceline, SpeakerName, OutFilePath, OutFileName);
		#else
		if (VoiceLine.IsEmpty())
			bVoiceLineFound = VoiceConfig->GetRandomVoiceLineForSpeaker(SpeakerName, OutFilePath, OutFileName);
		else
			bVoiceLineFound = VoiceConfig->GetRandomVoiceLine(MutatedVoiceline, SpeakerName, OutFilePath, OutFileName);
		#endif

		if (bVoiceLineFound)
			ChosenVoiceline = MutatedVoiceline;
		else // incase the mutated voiceline wasnt found, fallback to the original voiceline string
			bVoiceLineFound = VoiceConfig->GetRandomVoiceLine(VoiceLine, SpeakerName, OutFilePath, OutFileName);
		
		if (bVoiceLineFound)
		{
			Multicast_PlayRawVO(OutFileName, OverrideSpeakerName, bIgnoreIfAlreadyPlaying);
			LastVoiceLinePlayed = ChosenVoiceline;

			#if !UE_BUILD_SHIPPING
			if (CVarDisplayVO.GetValueOnAnyThread() > 0)
				DrawDebugString(GetWorld(), GetActorLocation(), SpeakerName + "_" + OutFileName, nullptr, FColor::White, 2.0f, true);
			#endif

			return true;
		}
	}

	return false;
}

void AReadyOrNotCharacter::PlayVoiceOverSubtitles(USoundSource* SoundSource, const FString& SpeakerName, const FString& VoiceLine)
{
	if (!SoundSource)
		return;

	const float* UseRadio = SoundSource->ParameterCache.Find("UseRadio");
	bool bIgnoreDistance = UseRadio && *UseRadio != 0.0f;

	if (!bIgnoreDistance)
	{
		const float* Occlusion = SoundSource->ParameterCache.Find("Occlusion");
		if (Occlusion && *Occlusion > 0.8f)
			return;
		
		const float* PropagationDistance = SoundSource->ParameterCache.Find("PropagateDistance");
		if (PropagationDistance && *PropagationDistance > 15.0f)
			return;
	}

	// Determine the speaker type from the character
	// FName SpeakerTag = NAME_None;
	// switch (Character->GetTeam())
	// {
	// case ETeamType::TT_SQUAD:
	// case ETeamType::TT_SERT_RED:
	// case ETeamType::TT_SERT_BLUE: SpeakerTag = USubtitlesStatics::SwatTag; break;
	// case ETeamType::TT_SUSPECT: SpeakerTag = USubtitlesStatics::SuspectTag; break;
	// case ETeamType::TT_CIVILIAN: SpeakerTag = USubtitlesStatics::CivilianTag; break;
	// default: SpeakerTag = USubtitlesStatics::DefaultTag;
	// }
	
	FName SpeakerTag = NAME_None;
	switch (GetTeam())
	{
	case ETeamType::TT_SQUAD:
	case ETeamType::TT_SERT_RED:
	case ETeamType::TT_SERT_BLUE: SpeakerTag = FSpeakerTags::SwatTag; break;
	default: SpeakerTag = FSpeakerTags::UnknownTag;
	}
	
	FString FallbackName = ""; 
	switch (GetTeam())
	{
	case ETeamType::TT_SQUAD:
	case ETeamType::TT_SERT_RED:
	case ETeamType::TT_SERT_BLUE: FallbackName = "DefaultSwat"; break;
	case ETeamType::TT_SUSPECT: FallbackName = "DefaultSuspect"; break;
	case ETeamType::TT_CIVILIAN: FallbackName = "DefaultCivilian"; break;
	default: ;
	}

	FString ScreenName;
	
	ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(this);
	UCustomizationCharacter* CustomizationCharacter = Cast<UCustomizationCharacter>(Customization.Character);
	
	if (SwatCharacter)
	{
		ScreenName = SwatCharacter->GetSwatCharacterName().ToString();
	}
	else if (CustomizationCharacter)
	{
		ScreenName = CustomizationCharacter->Name.ToString();
	}
	
	FSubtitleParameters Parameters;
	Parameters.Speaker = SpeakerName;
	Parameters.VoiceLine = VoiceLine;
	Parameters.SpeakerTag = SpeakerTag;
	Parameters.FallbackName = FallbackName;
	Parameters.ScreenNameOverride = ScreenName;
	
	SoundSource->OnProgrammerSoundLengthReady.AddWeakLambda(this, [this](float Length, FSubtitleParameters Parameters)
	{
		Parameters.Length = Length;
		USubtitlesStatics::PlaySubtitles(this, Parameters);
	}, Parameters);
}

void AReadyOrNotCharacter::OnBodyFallAudioStop()
{
	BodyFallSoundSource = nullptr;
}

void AReadyOrNotCharacter::OnVoiceAudioStopped()
{
	VoiceSoundSource = nullptr;

	OnVoiceAudioStoppedDelegate.Broadcast(this);
}

bool AReadyOrNotCharacter::CanPlayVO(const FString& VoiceLine) const
{
	if (bCannotSpeak)
		return false;
	
	if (IsActiveForVO())
		return true;

	if (VoiceLine.Contains("Death"))
		return true;

	if (IsIncapacitated() && (VoiceLine.Contains("Pain") || VoiceLine.Contains("Incap")))
		return true;
	
	return false;
}

void AReadyOrNotCharacter::RemoveVocalChords()
{
	bCannotSpeak = true;
	
	//FMODVoiceAudioComp->Stop();
	if(VoiceSoundSource)
	{
		VoiceSoundSource->Stop();
		VoiceSoundSource = nullptr;
	}
}

void AReadyOrNotCharacter::PlayRawVOWithCooldown(FString VoiceLine, float Cooldown, FString OverrideSpeakerName)
{
	if (VoiceLine.IsEmpty())
		return;
	
	if (SpeechCooldownMap.Num() > 0 && SpeechCooldownMap.Find(VoiceLine))
		return;

	if (!VoiceSoundSource)
	{
		if (PlayRawVO(VoiceLine, OverrideSpeakerName))
		{
			SpeechCooldownMap.Add(VoiceLine, Cooldown);
		}
	}
}

void AReadyOrNotCharacter::PlayReportSpeech(FString Voiceline, FString InTOCLine)
{
	bool bTocWaitingOnAnyone = false;
	if (USWATManager* SwatManager = USWATManager::Get(this))
	{
		for (ASWATCharacter* Swat : SwatManager->SwatAI)
		{
			if (!Swat->TOCResponseLine.IsEmpty())
			{
				//ULog::Info("Waiting on " + Swat->GetName());
				bTocWaitingOnAnyone = true;
				break;
			}
		}
	}
	
	if (!bTocWaitingOnAnyone)
		TOCResponseLine = InTOCLine;
	
	PlayRawVO(Voiceline, "", false);
}

void AReadyOrNotCharacter::PlayRadioSelectAnimation()
{
	if (ABaseItem* EquippedItem = GetEquippedItem())
	{
		if (EquippedItem->AnimationData)
		{
			if (EquippedItem->AnimationData->RadioSelect.Body_FP)
			{
				Play1PMontage(EquippedItem->AnimationData->RadioSelect.Body_FP);
			}

			if (EquippedItem->AnimationData->RadioSelect.Body_TP)
			{
				Play3PMontage(EquippedItem->AnimationData->RadioSelect.Body_TP);
			}

			if (EquippedItem->AnimationData->RadioSelect.Gun_FP)
			{
				EquippedItem->PlayFPMontage(EquippedItem->AnimationData->RadioSelect.Gun_FP);
			}

			if (EquippedItem->AnimationData->RadioSelect.Gun_TP)
			{
				EquippedItem->PlayTPMontage(EquippedItem->AnimationData->RadioSelect.Gun_TP);
			}
		}
	}
}

bool AReadyOrNotCharacter::HasSpecificSpeech(FString VoiceLine)
{
	const FString SpeakerName = GetSpeechCharacterName();
	if (SpeakerName == "None" || SpeakerName.IsEmpty())
	{
		return false;
	}

	
	FString OutFileName, OutFilePath;
	if (UReadyOrNotVoiceConfig::Get()->GetRandomVoiceLine(VoiceLine, SpeakerName, OutFilePath, OutFileName))
	{
		return true;
	}
	return false;

}

UFMODEvent* AReadyOrNotCharacter::GetAppropriateVoiceLineEvent()
{
	// If we check if we can equip headwear we can't get our headwear if we're carrying somebody
	AHeadwear* Headwear = GetInventoryComponent()->GetInventoryItemOfClass_Native<AHeadwear>(AHeadwear::StaticClass(), false);

	// Added extra check, if(Headwear) is used a crash occurs when null 
	if(Headwear == nullptr || Headwear->bUseMaskVoiceFilter == false)
	{
		return IsLocalPlayer() ? FMODVoiceLine2D : FMODVoiceLineSpatalized;
	}
	
	if (IsLocalPlayer() ? Headwear->GetVoiceLineEventLocalOverride() : Headwear->GetVoiceLineEventSpatalizedOverride())
	{
		return IsLocalPlayer() ? Headwear->GetVoiceLineEventLocalOverride() : Headwear->GetVoiceLineEventSpatalizedOverride();
	}
	
	return IsLocalPlayer() ? FMODVoiceLine2D : FMODVoiceLineSpatalized;
}

void AReadyOrNotCharacter::SetupVoiceLineParameters(const FString& FileName)
{
	if (!VoiceSoundSource)
		return;
	
	if (!IsOnSWATTeam())
		return;

	static const TArray<FString> RadioExceptions =
	{
		VO_SUSPECTS_AND_CIVILIAN::BARK_PAIN,
		VO_SWAT_GENERAL::CALL_PAIN_GRUNT,
		VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT,
		VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN,
		VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_CIVILIAN,
		VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_SUSPECT,
		VO_SWAT_GENERAL::CALL_YELL_AT_CIVILIAN,
		VO_SWAT_GENERAL::CALL_YELL_AT_SUSPECT,
		VO_SWAT_GENERAL::CALL_SHOT_AT_BY_SUSPECT
	};

	bool bUseRadio = true;
	
	FString VoiceLine, Variation;
	const bool bSuccess = FileName.Split("_", &VoiceLine, &Variation, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	
	if (bSuccess)
		bUseRadio = !RadioExceptions.Contains(VoiceLine);

	VoiceSoundSource->SetParameter("UseRadio", bUseRadio ? 1.0f : 0.0f);
}

void AReadyOrNotCharacter::PlaySpecificDebugVoiceLine(FString FileName)
{
	FString OutFileName, OutFilePath;
	UReadyOrNotVoiceConfig::Get()->GetSpecificVoiceLine(FileName, "SwatJudge", OutFilePath, OutFileName);
}

void AReadyOrNotCharacter::PlayRandomDebugVoiceLine(FString Line)
{
	PlayRawVO(Line, "SwatJudge");
	FString OutFileName, OutFilePath;
	UReadyOrNotVoiceConfig::Get()->GetRandomVoiceLine(Line, "SwatJudge", OutFilePath, OutFileName);
}

void AReadyOrNotCharacter::PlayRandomDebugConversation()
{
	for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
	{
		It->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::RELOADING);
	}
}

void AReadyOrNotCharacter::Server_Yell_Implementation()
{
	if (!IsActive())
		return;

	if (IsAnimationBlocking())
		return;

	int32 NumArmedTargets = 0;
	bool bAnyTargetHasKnife = false;
	bool bAnyHiding = false;

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (ACyberneticCharacter* CyberneticCharacter : GS->AllAICharacters)
		{
			if (!IsValid(CyberneticCharacter) || !IsValid(CyberneticCharacter->GetCyberneticsController()))
				continue;

			if (CyberneticCharacter->IsOnSWATTeam())
				continue;
			
			if (!CyberneticCharacter->IsActive())
				continue;
			
			if (CyberneticCharacter->IsPlayingDead())
				continue;

			if (CyberneticCharacter->GetCyberneticsController()->GetCurrentActivity<UMoveToExitActivity>())
				continue;

			FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(this, CyberneticCharacter);
			const bool bLOS = !GetWorld()->LineTraceTestByChannel(GetMesh()->GetSocketLocation("head_end"), CyberneticCharacter->GetMesh()->GetSocketLocation("head_end"), ECC_Visibility, CollisionQueryParams);

			if (!bLOS)
			{
				continue;
			}

			// todo: if directly looking at landmark, ignore the LOS check?

			if (const UTakeCoverAtLandmarkActivity* Activity = CyberneticCharacter->GetCyberneticsController()->GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
			{
				if (!Activity->IsMovingToLandmark())
					bAnyHiding = true;
			}
			
			if (CyberneticCharacter->GetEquippedItem())
			{
				NumArmedTargets++;
			}

			if (CyberneticCharacter->GetEquippedItem<AMeleeWeapon>())
			{
				bAnyTargetHasKnife = true;
			}
		}
	}

	FString VO = NumArmedTargets > 0 ? VO_SWAT_GENERAL::CALL_YELL_AT_SUSPECT : VO_SWAT_GENERAL::CALL_YELL_AT_CIVILIAN;
	if (NumArmedTargets == 1) // yelling at specific character
	{
		if (bAnyHiding)
			VO = VO_SWAT_GENERAL::CALL_YELL_HIDING;
		else if (bAnyTargetHasKnife)
			VO = VO_SWAT_GENERAL::CALL_WEAPON_DROP_KNIFE;
	}
	else // yelling at multiple people
	{
		if (bAnyHiding)
			VO = VO_SWAT_GENERAL::CALL_YELL_HIDING;
		else if (NumArmedTargets > 0)
			VO = FMath::RandBool() ? VO_SWAT_GENERAL::CALL_WEAPON_DROP_GENERIC : VO_SWAT_GENERAL::CALL_YELL_AT_SUSPECT;
	}
	
	PlayRawVO(VO, "", false);

	PlayYellAnimation();

	if (!GetWorld()->GetTimerManager().IsTimerActive(TH_OnYellExecute))
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_OnYellExecute, this, &AReadyOrNotCharacter::OnYellExecute, 0.1f, false);
	}

	if (AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(GetPlayerState()))
	{
		ps->TotalYells += 1;
	}
}

bool AReadyOrNotCharacter::Server_Yell_Validate()
{
	return true;
}

void AReadyOrNotCharacter::OnYellExecute()
{
	if (!IsActive())
		return;
	
	TArray<ACyberneticCharacter*> SelectedCivilians;
	bool bDirectlyYelledAtAnyAI = false;
	
	FVector TraceStart = GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT);
	
	if (AReadyOrNotPlayerController* PC = GetRONPlayerController())
	{
		if (PC->PlayerCameraManager)
			TraceStart = PC->PlayerCameraManager->GetCameraLocation();
	}
	
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		ACyberneticCharacter* CyberneticCharacter = *It;
		
		if (!IsValid(CyberneticCharacter) || !IsValid(CyberneticCharacter->GetCyberneticsController()))
			continue;

		if (!CyberneticCharacter->IsActive() && !CyberneticCharacter->IsPlayingDead() && !CyberneticCharacter->bDeactivated)
			continue;
		
		if (CyberneticCharacter->IsOnSWATTeam())
			continue;

		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(this, CyberneticCharacter);

		//DrawDebugLine(GetWorld(), GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT), CyberneticCharacter->GetMesh()->GetSocketLocation("head_end"), FColor::Purple, false, 5.0f, 0, 1.0f);
		//DrawDebugLine(GetWorld(), TraceStart, CyberneticCharacter->GetMesh()->GetSocketLocation("head_end"), FColor::Blue, false, 5.0f, 0, 1.0f);
		
		const bool bLOS = !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, CyberneticCharacter->GetMesh()->GetSocketLocation("head_end"), ECC_Visibility, CollisionQueryParams);

		CyberneticCharacter->OnOfficerShouted(this, bLOS);

		if (!bLOS)
		{
			if (CyberneticCharacter->IsCivilian())
			{
				float Dist = (CyberneticCharacter->GetActorLocation() - GetActorLocation()).Size();
				if (Dist < 1500.0f)
				{
					SelectedCivilians.Add(CyberneticCharacter);
				}
			}

			continue;
		}

		bDirectlyYelledAtAnyAI = true;

		#if !UE_BUILD_SHIPPING
		if (CVarRonAlwaysFake.GetValueOnAnyThread() == 1)
		{
			CyberneticCharacter->bHasEverFakeSurrendered = false;
			CyberneticCharacter->FakeSurrender();
			continue;
		}
		#endif

		if (bLOS)
		{
			float NegotiatorChance = URosterManager::GetSquadTraitValue("Negotiator", GetWorld());
			if (!CyberneticCharacter->bNegotiatorTried && UKismetMathLibrary::RandomBoolWithWeight(NegotiatorChance))
				CyberneticCharacter->Surrender();

			CyberneticCharacter->bNegotiatorTried = true;
			
			float Amount = 0.025f;
			if (IsPlayerControlled())
				Amount = 0.05f;
				
			UMoraleComponent::LowerMoraleOnCharacter(CyberneticCharacter, Amount, "Yell");
		}
	}

	if (!bDirectlyYelledAtAnyAI && SelectedCivilians.Num() > 0 && CivilianRespondCooldown <= 0.0f)
	{
		CivilianRespondCooldown = 15.0f;
		SelectedCivilians[FMath::RandRange(0, SelectedCivilians.Num() - 1)]->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_SEEN);
	}

	TimeSinceLastYell = 0.0f;
	
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), TraceStart, 1.0f, this, 0.0f, "SWATYellForCompliance");
}

void AReadyOrNotCharacter::PlayYellAnimation()
{
	if (ABaseItem* EquippedItem = GetEquippedItem())
	{
		if (EquippedItem->AnimationData)
		{
			if (EquippedItem->AnimationData->Yell.Body_FP)
			{
				Play1PMontage(EquippedItem->AnimationData->Yell.Body_FP);
			}

			if (EquippedItem->AnimationData->Yell.Body_TP)
			{
				Play3PMontage(EquippedItem->AnimationData->Yell.Body_TP);
			}

			if (EquippedItem->AnimationData->Yell.Gun_FP)
			{
				EquippedItem->PlayFPMontage(EquippedItem->AnimationData->Yell.Gun_FP);
			}

			if (EquippedItem->AnimationData->Yell.Gun_TP)
			{
				EquippedItem->Client_PlayFPMontage(EquippedItem->AnimationData->Yell.Gun_TP);
			}
		}
	}
}

bool AReadyOrNotCharacter::CanYell() const
{
	if (!GetEquippedItem())
		return false;

	if (TimeUntilNextYell > 0.0f)
		return false;

	if (!GetController())
		return false;

	if (bArrestComplete || IsDeadOrUnconscious())
		return false;

	if (IsAnimationBlocking())
		return false;

	if (UReadyOrNotFunctionLibrary::IsInLobby() && IsLowReady())
		return false;
	
	return Cast<ABaseWeapon>(GetEquippedItem()) ||
			Cast<ABaseGrenade>(GetEquippedItem()) ||
			Cast<ABallisticsShield>(GetEquippedItem()) ||
			GetEquippedItem()->ContainsItemCategory(EItemCategory::IC_Zipcuffs) ||
			GetEquippedItem()->ContainsItemCategory(EItemCategory::IC_OCSpray);
}

void AReadyOrNotCharacter::Yell()
{
	const AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (GameState && GameState->bPvPMode)
		return;

	if (IsAnimationBlocking())
		return;

	if (!CanYell())
		return;

	TimeUntilNextYell = 1.0f;
	
	Server_Yell();
}

float AReadyOrNotCharacter::TakeDamage(const float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(HasAuthority(), TEXT("TakeDamage called from client. Only call TakeDamage from server"));
	#endif

	if (!ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser))
	{
		return 0.0f;
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		const FName HitBone = PointDamageEvent->HitInfo.BoneName;
		for (const FName& Bone : DamageExcludedBones)
		{
			if (Bone == HitBone || GetMesh()->BoneIsChildOf(HitBone, Bone))
			{
				return 0.0f;
			}
		}
	}
	
	AActor::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (EventInstigator && EventInstigator != Controller)
	{
		LastHitBy = EventInstigator;
	}

	float FinalDamage = Damage;

	FCharacterDamageEvent Event;
	Event.RawDamage = Damage;
	Event.FinalDamage = FinalDamage;
	Event.DamageEvent = DamageEvent;
	Event.Instigator = EventInstigator;
	Event.Causer = DamageCauser;
	
	LastDamageEvent = Event;
	
	const bool bApplyDamage = OnTakeDamage(FinalDamage, DamageEvent, EventInstigator, DamageCauser);

	if (const ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(DamageCauser))
	{
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
			
			ApplyDismembermentDamage(PointDamageEvent->HitInfo, DamageCauser);
		}
	}

	if (bApplyDamage)
	{
		TimeSinceLastTakenDamage = 0.0f;

		DecreaseHealth(FinalDamage);

		/*
		#if !UE_BUILD_SHIPPING
		FName HitBone = NAME_None;
		if (const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent)
		{
			if (PointDamageEvent->HitInfo.BoneName.IsValid() &&
				!PointDamageEvent->HitInfo.BoneName.IsNone())
			{
				HitBone = PointDamageEvent->HitInfo.BoneName;
			}
		}

		ULog::Info(FString::Printf(TEXT("[%s] Damaged Bone: %s | RawDamage: %.3f | FinalDamage: %.3f | CurrentHealth: %.3f | DamageType: %s | Instigator: %s | Causer: %s"), *GetName(), *HitBone.ToString(), Damage, FinalDamage, GetCurrentHealth(), *GetNameSafe(DamageEvent.DamageTypeClass.Get()), *GetNameSafe(EventInstigator), *GetNameSafe(DamageCauser)));
		#endif
		*/
	}
	
	AReadyOrNotCharacter* InstigatorCharacter = EventInstigator ? Cast<AReadyOrNotCharacter>(EventInstigator->GetPawn()) : nullptr;
	
	OnCharacterTakeDamage.Broadcast(InstigatorCharacter, this, DamageCauser, FinalDamage, GetCurrentHealth());
	
	Multicast_TakeDamage(FinalDamage, DamageEvent, InstigatorCharacter, DamageCauser);

	return FinalDamage;
}

bool AReadyOrNotCharacter::OnTakeDamage(float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	return true;
}

bool AReadyOrNotCharacter::ShouldTakeDamage(const float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	if (GetLocalRole() < ROLE_Authority)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: No Authority");
		#endif
		
		return false;
	}

	if (!CanBeDamaged())
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: bCanTakeDamage is false");
		#endif
		
		return false;
	}

	if (!GetWorld())
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: No valid world");
		#endif
		
		return false;
	}

	if (!GetWorld()->GetAuthGameMode())
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: Applying damage on client");
		#endif
		
		return false;
	}

	if (Damage <= 0.0f)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: Damage to apply is 0.0");
		#endif
		
		return false;
	}
	
	if (!DamageEvent.DamageTypeClass || !DamageEvent.DamageTypeClass.Get())
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: DamageTypeClass is null");
		#endif
		
		return false;
	}
	
	#if !UE_BUILD_SHIPPING
	// Gods shall take no damage
	if (bGodMode)
	{
		#if !UE_BUILD_SHIPPING
		//ULog::Warning("Can't apply damage to " + GetName() + ". Reason: God mode is enabled");
		#endif
		
		return false;
	}
	#endif

	if (!IsAffectedByDamageType(Cast<UDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject())))
	{
		#if !UE_BUILD_SHIPPING
		const UDamageType* Type = Cast<UDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
		ULog::Warning("Can't apply damage to " + GetName() + ". Reason: Not affected by " + GetNameSafe(Type));
		#endif
		
		return false;
	}
	
	// TODO: condense into function
	// Immune to gas damage if wearing gas mask
	const UStunDamage* StunDamage = Cast<UStunDamage>(DamageEvent.DamageTypeClass->GetDefaultObject());
	if ((StunDamage && StunDamage->StunType == EStunType::ST_Gassed) || Cast<UPepperSprayDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()))
	{
		if (InventoryComp->GetHeadwear())
		{
			if (InventoryComp->GetHeadwear()->ContainsItemCategory(EItemCategory::IC_GasMask))
			{
				return false;
			}
		}
	}

	if (const AReadyOrNotGameMode* RONGM = Cast<AReadyOrNotGameMode>(GetWorld()->GetAuthGameMode()))
		return RONGM->CanTakeDamage(EventInstigator, GetController());

	return true;
}

bool AReadyOrNotCharacter::TryApplyStunDamage(UStunDamage* InStunDamage, float& Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!InStunDamage || !DamageCauser)
		return false;

	if (!IsAffectedByDamageType(InStunDamage))
		return false;

 	FVector DamageOrigin = DamageCauser->GetActorLocation();
	
	if (InStunDamage->bCausesSuppression)
	{
		FSuppressionData Data;
		Data.Strength = InStunDamage->SuppressionAmount;
		Data.Origin = DamageOrigin;
		Data.Direction = (DamageOrigin - GetActorLocation()).GetSafeNormal2D();
		Data.Distance = FVector::Distance(DamageOrigin, GetActorLocation());
		Data.Instigator = EventInstigator ? EventInstigator->GetPawn<AReadyOrNotCharacter>() : nullptr;
		
		Multicast_InflictSuppression(Data, InStunDamage->SuppressionCameraShake, true);
	}

	InStunDamage->ScriptedStunEvent(this, Damage, DamageEvent, EventInstigator, DamageCauser);

	if (InStunDamage->bCauseHealthDamage)
	{
		if (EventInstigator)
		{
			if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(EventInstigator->GetPawn()))
			{
				AICharacter->bHasDamagedSWATTeam = true;
				AICharacter->ScoringComponent->RevokeAllPenalties();
			}
		}
		
		//DecreaseHealth(Damage);
	}

	if (InStunDamage->bPlayAudioWhenHit)
	{
		Client_PlayFMODEvent2D(InStunDamage->StunSoundEffect);
	}
		
	UGameplayStatics::PlaySound2D(GetWorld(), InStunDamage->ImpactSound);

	// Count number of stunned enemies
	// Achievement SAY_HELLO
	if (const AGrenadeProjectile* GrenadeProjectile = Cast<AGrenadeProjectile>(DamageCauser))
	{
		if (IsSuspect() && DamageCauser && DamageCauser->GetInstigator() && 
			DamageCauser->GetInstigator()->IsLocallyControlled() &&
			DamageCauser->GetInstigator()->IsPlayerControlled() &&
			GrenadeProjectile->FiredFromWeapon)
		{
			FString weaponName = GrenadeProjectile->FiredFromWeapon->GetClass()->GetName();
			if (weaponName == "Launcher_M320_Bang_C" || weaponName == "Launcher_M320_Gas_C" || weaponName == "Launcher_M320_Stinger_C")
			{
				UAchievementStatics::IncreaseAchievementStat(GetWorld(), EAchievementStats::PROGRESS_M320, 1);
			}
		}
	}

	return true;
}

bool AReadyOrNotCharacter::TryApplyBulletDamage(float& Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage <= 0.0f || !DamageCauser)
	{
		return false;
	}

	if (const ABulletProjectile* Bullet = Cast<ABulletProjectile>(DamageCauser))
	{
		if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(Bullet->FiredFromWeapon))
		{
			if (FMath::FRandRange(0.0f, 1.0f) < Weapon->DamageSeverityChance)
			{
				// don't allow insane values here
				Damage *= FMath::FRandRange(1.0f, Weapon->DamageSeverityMultiplier);
			}

			if (EventInstigator)
			{
				if (AReadyOrNotPlayerState* RONPS = EventInstigator->GetPlayerState<AReadyOrNotPlayerState>())
				{
					RONPS->BulletsHit++;
					RONPS->BulletsHitThisLife++;
				}

				if (AReadyOrNotCharacter* PlayerCharacter = Cast<AReadyOrNotCharacter>(EventInstigator->GetPawn()))
				{
					DamagedByCharacters.Remove(nullptr);
					DamagedByCharacters.AddUnique(PlayerCharacter);
				}
			}

			DamagedByWeapons.Remove(nullptr);
			DamagedByWeapons.AddUnique(Cast<ABaseWeapon>(DamageCauser));
			
			TimeSinceLastBulletDamage = 0.0f;
		}

		return true;
	}
	
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		
		if (PointDamageEvent->HitInfo.GetActor() == this)
		{
			const FName HitBone = PointDamageEvent->HitInfo.BoneName;
			LastHitBoneName = HitBone;
			
			ApplyArteryDamage(PointDamageEvent->HitInfo, DamageCauser);
			ApplyDamageToBone(Damage, HitBone, *PointDamageEvent, EventInstigator, DamageCauser);

			if (EventInstigator)
			{
				if (AReadyOrNotPlayerState* RONPS = EventInstigator->GetPlayerState<AReadyOrNotPlayerState>())
				{
					RONPS->BulletsHit++;
					RONPS->BulletsHitThisLife++;
				}

				if (AReadyOrNotCharacter* PlayerCharacter = Cast<AReadyOrNotCharacter>(EventInstigator->GetPawn()))
				{
					DamagedByCharacters.Remove(nullptr);
					DamagedByCharacters.AddUnique(PlayerCharacter);
				}

				if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(EventInstigator->GetPawn()))
				{
					AICharacter->bHasDamagedSWATTeam = true;
					AICharacter->ScoringComponent->RevokeAllPenalties();
				}
			}

			DamagedByWeapons.Remove(nullptr);
			DamagedByWeapons.AddUnique(Cast<ABaseWeapon>(DamageCauser));
			
			TimeSinceLastBulletDamage = 0.0f;

			OnPlayerHit.Broadcast(Damage, HitBone);

			return true;
		}
	}

	return false;
}

bool AReadyOrNotCharacter::DamageHitHead(const FPointDamageEvent& DamageEvent)
{
	const FHitResult& Hit = DamageEvent.HitInfo;
	if (Hit.GetActor() != this)
	{
		return false;
	}

	return HeadBones.Contains(Hit.BoneName);
}

void AReadyOrNotCharacter::Client_OnBoneDamaged_Implementation(const FName& BoneHit)
{
	OnBoneDamaged.Broadcast(BoneHit);
}

void AReadyOrNotCharacter::Client_OnBodyPartDamaged_Implementation(bool bInHeadHit, bool bInBodyHit, bool bInLeftArmHit, bool bInRightArmHit, bool bInLeftLegHit, bool bInRightLegHit, bool bInLeftFootHit, bool bInRightFootHit)
{
	bBodyHit = bInBodyHit;
	bLeftFootHit = bInLeftFootHit;
	bRightFootHit = bInRightFootHit;
	
	OnBodyPartDamaged.Broadcast(bInHeadHit, bInBodyHit, bInLeftArmHit, bInRightArmHit, bInLeftLegHit, bInRightLegHit, bInLeftFootHit, bInRightFootHit);
}

void AReadyOrNotCharacter::ApplyDamageToBone(float& Damage, const FName& HitBone, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	bBlockedByBodyArmor = false;
	bBlockedByHeadArmor = false;

	float HeadDamageMultiplier = 1.0f;
	float UpperBodyDamageMultiplier = 1.0f;
	float LowerBodyDamageMultiplier = 1.0f;
	float ArmDamageMultiplier = 1.0f;
	float HandDamageMultiplier = 1.0f;
	float LegDamageMultiplier = 1.0f;
	float FootDamageMultiplier = 1.0f;

	if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
	{
		const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
		if (AmmoType)
		{
			HeadDamageMultiplier = AmmoType->HeadDamageMultiplier;
			UpperBodyDamageMultiplier = AmmoType->UpperBodyDamageMultiplier;
			LowerBodyDamageMultiplier = AmmoType->LowerBodyDamageMultiplier;
			ArmDamageMultiplier = AmmoType->ArmDamageMultiplier;
			HandDamageMultiplier = AmmoType->HandDamageMultiplier;
			LegDamageMultiplier = AmmoType->LegDamageMultiplier;
			FootDamageMultiplier = AmmoType->FootDamageMultiplier;
		}
	}

	if (HeadBones.Contains(HitBone))
	{
		Damage *= HeadDamageMultiplier;
		ApplyHeadDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (UpperBody.Contains(HitBone))
	{
		Damage *= UpperBodyDamageMultiplier;
		ApplyUpperBodyDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (LowerBody.Contains(HitBone))
	{
		Damage *= LowerBodyDamageMultiplier;
		ApplyLowerBodyDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (L_Foot.Contains(HitBone)) 
	{
		Damage *= FootDamageMultiplier;
		ApplyLeftFootDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (R_Foot.Contains(HitBone))
	{
		Damage *= FootDamageMultiplier;
		ApplyRightFootDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (R_Leg.Contains(HitBone))
	{
		Damage *= LegDamageMultiplier;
		ApplyRightLegDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (L_Leg.Contains(HitBone))
	{
		Damage *= LegDamageMultiplier;
		ApplyLeftLegDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (L_Arm.Contains(HitBone))
	{
		Damage *= ArmDamageMultiplier;
		ApplyLeftArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (L_Hand.Contains(HitBone))
	{
		Damage *= HandDamageMultiplier;
		ApplyLeftArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (R_Arm.Contains(HitBone))
	{
		Damage *= ArmDamageMultiplier;
		ApplyRightArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
	else if (R_Hand.Contains(HitBone))
	{
		Damage *= HandDamageMultiplier;
		ApplyRightArmDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	if (HitBone != NAME_None)
	{
		OnBoneDamaged.Broadcast(HitBone);

		const bool bHeadHit = IsLimbHit(ELimbType::LT_Head);
		const bool bRightArmHit = IsLimbHit(ELimbType::LT_RightArm);
		const bool bLeftArmHit = IsLimbHit(ELimbType::LT_LeftArm);
		const bool bRightLegHit = IsLimbHit(ELimbType::LT_RightLeg);
		const bool bLeftLegHit = IsLimbHit(ELimbType::LT_LeftLeg);

		OnBodyPartDamaged.Broadcast(bHeadHit, bBodyHit, bLeftArmHit, bRightArmHit, bLeftLegHit, bRightLegHit, bLeftFootHit, bRightFootHit);
		
		Client_OnBoneDamaged(HitBone);
		Client_OnBodyPartDamaged(bHeadHit, bBodyHit, bLeftArmHit, bRightArmHit, bLeftLegHit, bRightLegHit, bLeftFootHit, bRightFootHit);
	}
}

void AReadyOrNotCharacter::ApplyHeadDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_Head, 1);

	const ABaseArmour* HeadArmour = HeadArmour = Cast<ABaseArmour>(InventoryComp->GetHeadArmour());

	if (HeadArmour)
	{
		if (!HeadArmour->HasRemainingProtection())
		{
			DepleteHealth();
		}
	}
	else
	{
		DepleteHealth();
	}
}

void AReadyOrNotCharacter::ApplyUpperBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ApplyBodyDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	
	bBodyHit = Damage > 0.0f;
}

void AReadyOrNotCharacter::ApplyLowerBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ApplyBodyDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	
	bBodyHit = Damage > 0.0f;
}

void AReadyOrNotCharacter::ApplyLeftArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_LeftArm, 1);
}

void AReadyOrNotCharacter::ApplyRightArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_RightArm, 1);
}

void AReadyOrNotCharacter::ApplyLeftLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_LeftLeg, 1);
}

void AReadyOrNotCharacter::ApplyRightLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_RightLeg, 1);
}

void AReadyOrNotCharacter::ApplyLeftFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_LeftLeg, 1);
	bLeftFootHit = true;
}

void AReadyOrNotCharacter::ApplyRightFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CharacterHealth->DecreaseLimbTickets(ELimbType::LT_RightLeg, 1);
	bRightFootHit = true;
}

void AReadyOrNotCharacter::ApplyBodyDamage(float& Damage, const FPointDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
}

bool AReadyOrNotCharacter::AreBonesInSameGroup(const FName& BoneA, const FName& BoneB) const
{
	if (HeadBones.Contains(BoneA) && HeadBones.Contains(BoneB))
		return true;

	if (Torso.Contains(BoneA) && Torso.Contains(BoneB))
		return true;

	if (L_Arm.Contains(BoneA) && L_Arm.Contains(BoneB))
		return true;

	if (R_Arm.Contains(BoneA) && R_Arm.Contains(BoneB))
		return true;
	
	if (L_Hand.Contains(BoneA) && L_Hand.Contains(BoneB))
		return true;
	
	if (R_Hand.Contains(BoneA) && R_Hand.Contains(BoneB))
		return true;
	
	if (L_Leg.Contains(BoneA) && L_Leg.Contains(BoneB))
		return true;
	
	if (R_Leg.Contains(BoneA) && R_Leg.Contains(BoneB))
		return true;
	
	if (L_Foot.Contains(BoneA) && L_Foot.Contains(BoneB))
		return true;

	if (R_Foot.Contains(BoneA) && R_Foot.Contains(BoneB))
		return true;

	return false;
}

bool AReadyOrNotCharacter::IsHeadBone(const FName& Bone) const
{
	return HeadBones.Contains(Bone);
}

bool AReadyOrNotCharacter::IsBodyBone(const FName& Bone) const
{
	return Torso.Contains(Bone);
}

bool AReadyOrNotCharacter::IsArmBone(const FName& Bone) const
{
	return L_Arm.Contains(Bone) || R_Arm.Contains(Bone);
}

bool AReadyOrNotCharacter::IsLegBone(const FName& Bone) const
{
	return L_Leg.Contains(Bone) || R_Leg.Contains(Bone);
}

bool AReadyOrNotCharacter::IsHandBone(const FName& Bone) const
{
	return L_Hand.Contains(Bone) || R_Hand.Contains(Bone);
}

bool AReadyOrNotCharacter::IsFootBone(const FName& Bone) const
{
	return L_Foot.Contains(Bone) || R_Foot.Contains(Bone);
}

void AReadyOrNotCharacter::Multicast_SpawnBloodEffects_Implementation(FHitResult Hit, float WoundSize, AController* HitInstigator)
{
	// Don't spawn blood effects on the instigator's machine if they're a client as they're predicted
	if (IsValid(HitInstigator) && HitInstigator->IsLocalPlayerController() && !HitInstigator->HasAuthority())
		return;
	
	SpawnBloodEffects(Hit, WoundSize);
}

void AReadyOrNotCharacter::PredictHitEffects(FHitResult Hit, float WoundSize)
{
	APlayerCharacter* HitPlayerCharacter = Cast<APlayerCharacter>(Hit.HitObjectHandle.FetchActor());
	ABaseArmour* Armour = GetArmourForBone(Hit.BoneName);
	if (!Armour)
	{
		SpawnBloodEffects(Hit, WoundSize);
	}
	else if (HitPlayerCharacter) // don't predict armour hits for players yet
	{
		PlayArmourHitEffects(Armour, Hit);
	}
}

void AReadyOrNotCharacter::SpawnBloodEffects(FHitResult Hit, float WoundSize)
{
	if (!Blood)
		return;
	
	// Play sound effects from being hit in certain areas
	FTransform EventTransform;
	EventTransform.SetLocation(Hit.ImpactPoint);
	EventTransform.SetRotation(Hit.ImpactNormal.Rotation().Quaternion());

	if (!IsDeadOrUnconscious())
	{
		FFMODEventInstance HitEventInstance = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), Blood->HitEvent, EventTransform, false);
		if (HitEventInstance.Instance)
		{
			HitEventInstance.Instance->setParameterByName("Armor", IsBoneArmored(Hit.BoneName));
			HitEventInstance.Instance->setParameterByName("Local", IsLocalPlayer() ? 1.0f : 0.0f);
			HitEventInstance.Instance->start();
		}
	}
	else
	{
		UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), Blood->DeadHitEvent, EventTransform, true);
	}

	// Spawn blood impact decals
	if (SkinnedDecalSampler && SkinnedDecalSampler->MaxDecals > SkinnedDecalsSpawned)
	{
		int32 SubUV = FMath::RandRange(0, Blood->SkinnedDecalImageCount);

		SkinnedDecalSampler->SpawnDecal(Hit.ImpactPoint, Hit.ImpactNormal.ToOrientationQuat(), Hit.BoneName, WoundSize, SubUV);
		SkinnedDecalsSpawned++;
	}

	// Don't spawn blood entry particles on player character
	if (!IsLocalPlayer() && Blood->HitEntryParticles.Num() > 0)
	{
		UParticleSystem* Particle = Blood->HitEntryParticles[FMath::RandRange(0, Blood->HitEntryParticles.Num() - 1)];
		if (Particle)
		{
			// TODO: optimize
			UGameplayStatics::SpawnEmitterAttached(Particle, GetMesh(), Hit.BoneName, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition);
		}
	}

	// Limit max times surface blood decals can spawn on dead characters
	if (IsDeadOrUnconscious())
	{
		if (DeadSurfaceBloodDecalsSpawned > 12)
			return;
		
		DeadSurfaceBloodDecalsSpawned++;
	}
	
	// Only spawn blood splatters for valid hits
	FVector TraceDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	if (TraceDirection == FVector::ZeroVector)
		return;

	FVector TraceStart = Hit.ImpactPoint;
	FVector TraceEnd = TraceStart + TraceDirection * Blood->SplatterMaxTraceDistance;
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams CollisionQueryParams = GetCollisionQueryParameters();
	CollisionQueryParams.bIgnoreTouches = true;

	bool bWasHeadshot = Blood->HeadshotSplatterBones.Contains(Hit.BoneName);
	bool bWasBodyshot = Blood->AnimatedSplatterBones.Contains(Hit.BoneName);
	bool bIsDead = IsDeadNotUnconscious();

	// not sure how this works, delegate goes out of scope
	FTraceDelegate Delegate = FTraceDelegate::CreateWeakLambda(this, [&, bIsDead, bWasHeadshot, bWasBodyshot](const FTraceHandle& Handle, FTraceDatum& Data)
	{
		if (!IsValid(Blood))
			return;
		
		for (FHitResult& HitResult : Data.OutHits)
		{
			if (!HitResult.bBlockingHit || HitResult.Distance <= 20.0f)
				continue;

			// Don't spawn regular decals onto characters
			if (Cast<AReadyOrNotCharacter>(HitResult.GetActor()))
				continue;

			const bool bIsSurfaceVertical = FMath::Abs(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)) <= 0.5f;

			// Play headshot FMOD event
			if (!bIsDead && bWasHeadshot && (!GibComponent || !GibComponent->IsLimbGibbed(EGibAreas::GA_Head)))
			{
				USoundSource* HeadshotSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), Blood->HeadshotEvent, FTransform(FRotator(), GetMesh()->GetSocketLocation("head"), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				
				if(HeadshotSoundSource)
				{
					HeadshotSoundSource->Attach(GetMesh(), "head");
					HeadshotSoundSource->Play();
				}
			}
			
			// Don't spawn headshot splatters for non-killing headshots
			if (!bIsDead && bWasHeadshot && HitResult.Distance <= Blood->HeadshotMaxSplatterDistance)
			{
				SpawnHeadshotSplatterEffects(HitResult);
			}
			// Don't spawn animated blood decals on too steep surfaces
			else if (!bIsDead && bWasBodyshot && HitResult.Distance <= Blood->AnimatedSplatterMaxDistance && bIsSurfaceVertical)
			{
				SpawnAnimatedBloodSplatterEffects(HitResult);
			}
			// Otherwise, spawn regular blood splatters
			else
			{
				SpawnBloodSplatterEffects(HitResult);
			}
		}
	});
	GetWorld()->AsyncLineTraceByObjectType(EAsyncTraceType::Single, TraceStart, TraceEnd, ObjectQueryParams, CollisionQueryParams, &Delegate);
}

void AReadyOrNotCharacter::Multicast_SpawnDismembermentEffects_Implementation(EGibAreas GibArea, FHitResult HitResult)
{
	FVector Impulse = -HitResult.ImpactNormal * 500.0f;
	GibComponent->Gib(GibArea, Impulse);

	// Don't allow any arterial hits after a gibbing, also trick bloodpool into spawning at the gib bone
	LastArterialHitBone = FName(GibComponent->GetGibBone(GibArea));

	// Stop all sounds from playing if we've shot off their head
	if (GibArea == EGibAreas::GA_Head)
	{
		GetRagdollComponent()->RequestAnim2RagdollBlend(0.0f);
		RemoveVocalChords();
	}

	// Stop all death animations if we lose both legs
	if (GibComponent->IsLimbGibbed(EGibAreas::GA_LeftLeg) && GibComponent->IsLimbGibbed(EGibAreas::GA_RightLeg))
	{
		GetRagdollComponent()->RequestAnim2RagdollBlend(0.0f);
	}
	
	// Play gore FMOD event
	USoundSource* GoreSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), Blood->GoreEvent, FTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal);
	GoreSoundSource->Attach(GetMesh(), GibComponent->GetBoneAttachSocket(GibArea));
	GoreSoundSource->Play();
	
	if (SkinnedDecalSampler && SkinnedDecalSampler->MaxDecals > SkinnedDecalsSpawned)
	{
		int32 SubUV = FMath::RandRange(0, Blood->SkinnedDecalImageCount);
		float Size = 160.0f; // FMath::RandRange(100.0f, 150.0f);

		FName Bone = FName(GibComponent->GetGibBone(GibArea));

		FVector Location = GetMesh()->GetBoneLocation(Bone);
		FRotator Rotation = GetMesh()->GetBoneQuaternion(Bone).Rotator();// + FRotator(90.0f, 0.0f, 0.0f);

		SkinnedDecalSampler->SpawnDecal(Location, Rotation.Quaternion(), Bone, Size, SubUV);
		SkinnedDecalsSpawned++;
	}
	
	if (Blood->DismembermentParticles.Num() <= 0)
		return;
	
	UParticleSystem* ParticleSystem = Blood->DismembermentParticles[FMath::RandRange(0, Blood->DismembermentParticles.Num() - 1)];
	UParticleSystemComponent* ParticleComponent =
		UGameplayStatics::SpawnEmitterAttached(ParticleSystem, GetMesh(), GibComponent->GetBoneAttachSocket(GibArea), FVector::ZeroVector, FRotator(-90.0f, 0.0f, 0.0f));
	
	if (!ParticleComponent)
		return;
	
	ParticleComponent->OnParticleCollide.AddDynamic(this, &AReadyOrNotCharacter::OnDismembermentParticleCollision);
}

void AReadyOrNotCharacter::Multicast_SpawnArterialBloodEffects_Implementation(FHitResult HitResult, FName Artery)
{	
	LastArterialHitBone = Artery;
	bIsArterialBleeding = true;
	
	if (Blood && Blood->ArteryParticles.Num() > 0)
	{
		UParticleSystem* ParticleSystem = Blood->ArteryParticles[FMath::RandRange(0, Blood->ArteryParticles.Num() - 1)];
		
		if (UParticleSystemComponent* ParticleComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, HitResult.GetComponent(), HitResult.BoneName, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition))
		{
			ParticleComponent->OnParticleCollide.AddDynamic(this, &AReadyOrNotCharacter::OnArteryBleedParticleCollision);
		}
	}
}

void AReadyOrNotCharacter::ApplyArteryDamage(FHitResult HitResult, AActor* DamageCauser)
{
	if (!Blood || !GetMesh())
		return;

	if (IsOnSWATTeam())
		return;

	// Don't allow arterial hits if we've already received one or any limb has gibbed
	if (!LastArterialHitBone.IsNone() || (GibComponent && GibComponent->IsAnyLimbGibbed()))
		return;

	if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
	{
		const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
		if (!AmmoType)
			return;
		
		if (!UKismetMathLibrary::RandomBoolWithWeight(AmmoType->ArteryHitChance))
			return;
	}
	
	for (const FArteryData& Artery : Blood->Arteries)
	{
		// Bone must match, otherwise our line-sphere check later may consider a hand-to-neck shot valid and causes
		// the bleedout to appear from the incorrect wound. Shots that penetrate into another part of the body will
		// call the arterial damage function again, but with a hitresult from the correct wound spot. -killo
		if (Artery.BoneName != HitResult.BoneName)
			continue;
		
		FVector Location = GetMesh()->GetBoneLocation(Artery.BoneName);
		if (Location == FVector::ZeroVector)
			continue;

		if (Artery.ZoneOffset != 0.0f)
			Location += GetMesh()->GetBoneQuaternion(Artery.BoneName).Vector() * Artery.ZoneOffset;

		FVector Direction = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
		float Distance = FVector::Distance(HitResult.TraceStart, HitResult.ImpactPoint);
		
		if (!FMath::LineSphereIntersection<double>(HitResult.TraceStart, Direction, Distance + 100.0f, Location, Artery.ZoneSize))
			continue;

		Multicast_SpawnArterialBloodEffects(HitResult, Artery.BoneName);
		
		if (Artery.DeathTime > 0.0f && !(TH_ArteryDeath.IsValid() || TH_DeathRattle.IsValid()))
		{
			FTimerDelegate DeathDelegate = FTimerDelegate::CreateWeakLambda(this, [&]()
			{
				KilledBy = this;
				LastHitBy = nullptr;
				LastHitBoneName = NAME_None; // No death animation, just pass out
				DepleteHealth();
			});
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_ArteryDeath, this, DeathDelegate, Artery.DeathTime);

			FTimerDelegate DeathRattleDelegate = FTimerDelegate::CreateWeakLambda(this, [&]()
			{			
				PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::BARK_BREATHING, "", false);
			});
			TH_DeathRattle = GetWorldTimerManager().SetTimerForNextTick(DeathRattleDelegate);
		}
		else
		{
			KilledBy = this;
			LastHitBy = nullptr;
			DepleteHealth();
		}
	}
}

void AReadyOrNotCharacter::ApplyDismembermentDamage(FHitResult HitResult, AActor* DamageCauser)
{
	if (!GibComponent)
		return;

	if (!HitResult.BoneName.IsValid())
		return;
	
	EGibAreas GibArea = EGibAreas::GA_None;
	if (GibComponent->IsBoneInGibArea(HitResult.BoneName, EGibAreas::GA_Head))
		GibArea = EGibAreas::GA_Head;
	else if (GibComponent->IsBoneInGibArea(HitResult.BoneName, EGibAreas::GA_LeftArm))
		GibArea = EGibAreas::GA_LeftArm;
	else if (GibComponent->IsBoneInGibArea(HitResult.BoneName, EGibAreas::GA_RightArm))
		GibArea = EGibAreas::GA_RightArm;
	else if (GibComponent->IsBoneInGibArea(HitResult.BoneName, EGibAreas::GA_LeftLeg))
		GibArea = EGibAreas::GA_LeftLeg;
	else if (GibComponent->IsBoneInGibArea(HitResult.BoneName, EGibAreas::GA_RightLeg))
		GibArea = EGibAreas::GA_RightLeg;
	
	if (GibArea != EGibAreas::GA_None && !GibComponent->IsLimbGibbed(GibArea))
	{
		float DamageToDeal = 0.0f;
		if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
		{
			const FAmmoTypeData* AmmoType = Weapon->GetCurrentAmmoType();
			if (AmmoType)
				DamageToDeal = AmmoType->DismembermentDamage;
		}
	
		float& DismembermentDamage = DismembermentDamageMap.FindOrAdd(GibArea, 0.0f);
		DismembermentDamage += DamageToDeal;

		if (DismembermentDamage >= 1.0f && UKismetMathLibrary::RandomBoolWithWeight(0.3f))
		{
			Multicast_SpawnDismembermentEffects(GibArea, HitResult);

			DepleteHealth();
			CharacterHealth->SetHealthStatus(EPlayerHealthStatus::HS_Dead);
		}
	}
}

void AReadyOrNotCharacter::SpawnBloodSplatterEffects(FHitResult HitResult)
{
	// Regular blood splatters
	if (Blood->Splatters.Num() <= 0 || !HitResult.bBlockingHit)
		return;

	// Randomize the material used
	UMaterialInterface* DecalMaterial = Blood->Splatters[FMath::RandRange(0, Blood->Splatters.Num() - 1)];
		
	// Randomize width and height of the decal
	float SideLength = FMath::RandRange(Blood->SplatterSizeRange.X, Blood->SplatterSizeRange.Y);
	FVector DecalSize = FVector(12.0f, SideLength, SideLength);

	// Randomly flip the decal
	DecalSize.Y *= FMath::RandBool() ? 1.0f : -1.0f;
	DecalSize.Z *= FMath::RandBool() ? 1.0f : -1.0f;
	
	// Ensure the decal is oriented into the surface, randomize rotation
	FRotator Rotation = (-HitResult.ImpactNormal).Rotation();
	Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));
	
	UDecalComponent* SpawnedDecal = UGameplayStatics::SpawnDecalAttached(DecalMaterial, DecalSize, HitResult.GetComponent(), HitResult.BoneName, HitResult.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition);
	if (SpawnedDecal)
		SpawnedDecal->SetFadeScreenSize(Blood->DecalFadeScreenSize);
}

void AReadyOrNotCharacter::SpawnAnimatedBloodSplatterEffects(FHitResult HitResult)
{
	// Animated blood splatter
	if (Blood->AnimatedSplatters.Num() <= 0 || !Blood->AnimatedDecalClass)
		return;

	// Randomize the material used
	UMaterialInterface* Material = Blood->AnimatedSplatters[FMath::RandRange(0, Blood->AnimatedSplatters.Num() - 1)];

	// Ensure the decal is oriented into the surface
	FRotator Rotation = (-HitResult.ImpactNormal).Rotation();

	AAnimatedDecal* AnimatedDecal = GetWorld()->SpawnActor<AAnimatedDecal>(Blood->AnimatedDecalClass, HitResult.ImpactPoint, Rotation);
	if (AnimatedDecal && AnimatedDecal->Decal)
	{
		// Randomize width and height of the decal
		float SideLength = FMath::RandRange(Blood->AnimatedSplatterSizeRange.X, Blood->AnimatedSplatterSizeRange.Y);
		FVector DecalSize = FVector(12.0f, SideLength, SideLength);

		// Randomly flip the decal
		DecalSize.Z *= FMath::RandBool() ? 1.0f : -1.0f;

		AnimatedDecal->SetAnimatedDecalMaterial(Material);
		AnimatedDecal->Decal->SetFadeScreenSize(Blood->DecalFadeScreenSize);
		AnimatedDecal->Decal->SetSortOrder(1); // Draw above non-animated decals
		AnimatedDecal->Decal->DecalSize = DecalSize;
		
		AnimatedDecal->AnimationCurve = Blood->AnimatedBloodCurve;
		AnimatedDecal->AnimationTimescale = Blood->AnimatedBloodTimescale;
	}
}

void AReadyOrNotCharacter::SpawnHeadshotSplatterEffects(FHitResult HitResult)
{
	// Headshot blood splatter
	if (Blood->HeadshotSplatters.Num() <= 0)
		return;
	
	// Random material
	UMaterialInterface* Material = Blood->HeadshotSplatters[FMath::RandRange(0, Blood->HeadshotSplatters.Num() - 1)];

	// Randomize width and height of the decal
	float SideLength = FMath::RandRange(Blood->HeadshotSplatterSizeRange.X, Blood->HeadshotSplatterSizeRange.Y);
	FVector DecalSize = FVector(12.0f, SideLength, SideLength);

	// Randomly flip the decal
	DecalSize.Y *= FMath::RandBool() ? 1.0f : -1.0f;
	DecalSize.Z *= FMath::RandBool() ? 1.0f : -1.0f;
	
	// Ensure the decal is oriented into the surface, randomize rotation
	FRotator Rotation = (-HitResult.ImpactNormal).Rotation();
	Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

	// TODO: optimize
	UDecalComponent* SpawnedDecal = UGameplayStatics::SpawnDecalAttached(Material, DecalSize, HitResult.GetComponent(), HitResult.BoneName, HitResult.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition);
	if (SpawnedDecal)
		SpawnedDecal->SetFadeScreenSize(Blood->DecalFadeScreenSize);

	// Headshot splatter meshes
	if (!Blood->HeadshotDecalMesh)
		return;

	UPrimitiveComponent* AttachToPrimitive = HitResult.GetComponent();
	if (!AttachToPrimitive || !AttachToPrimitive->bReceivesDecals)
		return;

	for (int32 i = 0; i < FMath::RandRange(6, 9); i++)
	{
		FVector Point = FVector(0.0f);
		Point.Y += FMath::RandRange(-50.0f, 50.0f);
		Point.Z += FMath::RandRange(-50.0f, 50.0f);
		
		Point = (HitResult.ImpactNormal.Rotation()).RotateVector(Point);
		Point += HitResult.ImpactPoint;
		
		Point += HitResult.ImpactNormal * 0.5f;

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

		// Don't spawn meshes if not near a surface
		if (!GetWorld()->OverlapAnyTestByObjectType(Point, FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(2.5f)))
			continue;

		AActor* MeshOwner = AttachToPrimitive->GetOwner() ? AttachToPrimitive->GetOwner() : GetWorld()->GetWorldSettings();

		// TODO: optimize, spawn hidden at game start then activate it here
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(MeshOwner, UStaticMeshComponent::StaticClass());
		if (!MeshComponent)
			continue;

		MeshComponent->SetStaticMesh(Blood->HeadshotDecalMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->bReceivesDecals = false;

		MeshComponent->AttachToComponent(HitResult.GetComponent(),FAttachmentTransformRules::KeepRelativeTransform, HitResult.BoneName);

		FQuat MeshRotation = FQuat(HitResult.ImpactNormal.Rotation());
		MeshRotation *= FQuat(FVector::RightVector, FMath::DegreesToRadians(90.0f));
		MeshRotation *= FQuat(FVector::UpVector, FMath::DegreesToRadians(FMath::RandRange(0.0f, 360.0f)));

		MeshComponent->SetWorldLocationAndRotation(Point, MeshRotation);
		MeshComponent->SetWorldScale3D(FVector(FMath::RandRange(1.0f, 3.0f)));
		
		MeshComponent->RegisterComponent();
	}
}

void AReadyOrNotCharacter::OnArteryBleedParticleCollision(FName EventName, float EmitterTime, int32 ParticleTime,
	FVector Location, FVector Velocity, FVector Direction, FVector Normal, FName BoneName, UPhysicalMaterial* PhysMat)
{
	if (!Blood)
		return;

	SpawnBloodTrailEffects(EventName, Location, Normal, Blood->ArteryParticleCollisionDecals, Blood->ArteryParticleCollisionChance, Blood->ArteryParticleCollisionSizeRange);
}

void AReadyOrNotCharacter::OnDismembermentParticleCollision(FName EventName, float EmitterTime, int32 ParticleTime,
	FVector Location, FVector Velocity, FVector Direction, FVector Normal, FName BoneName, UPhysicalMaterial* PhysMat)
{
	if (!Blood)
		return;
	
	SpawnBloodTrailEffects(EventName, Location, Normal, Blood->DismembermentParticleCollisionDecals, Blood->DismembermentParticleCollisionChance, Blood->ArteryParticleCollisionSizeRange);
}

void AReadyOrNotCharacter::SpawnBloodTrailEffects(FName EventName, FVector Location, FVector Normal, TArray<UMaterialInterface*>& Materials, float SpawnChance, FVector2D SizeRange)
{
	if (EventName != "Blood_Collider")
		return;

	if (!Blood)
		return;

	if (!UKismetMathLibrary::RandomBoolWithWeight(SpawnChance))
		return;

	if (Materials.Num() <= 0)
		return;

	// Randomize the material used
	UMaterialInterface* DecalMaterial = Materials[FMath::RandRange(0, Materials.Num() - 1)];
		
	// Randomize width and height of the decal
	float SideLength = FMath::RandRange(SizeRange.X, SizeRange.Y);
	FVector DecalSize = FVector(5.0f, SideLength, SideLength);

	// Randomly flip the decal
	DecalSize.Y *= FMath::RandBool() ? 1.0f : -1.0f;
	DecalSize.Z *= FMath::RandBool() ? 1.0f : -1.0f;
	
	// Ensure the decal is oriented into the surface, randomize rotation
	FRotator Rotation = (-Normal).Rotation();
	Rotation.Add(0.0f, 0.0f, FMath::RandRange(0.0f, 360.0f));

	UDecalComponent* SpawnedDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), DecalMaterial, DecalSize, Location, Rotation);
	if (SpawnedDecal)
		SpawnedDecal->SetFadeScreenSize(Blood->DecalFadeScreenSize);
}

void AReadyOrNotCharacter::SpawnBloodPool()
{
	if (!bShouldSpawnBloodPool || !Blood || !Blood->BloodPoolClass)
		return;
	
	FName SpawnBone = !LastArterialHitBone.IsNone() ? LastArterialHitBone : Blood->BloodPoolSpawnBone;
			
	FHitResult FloorHit;
	FVector Location = GetMesh()->GetBoneLocation(SpawnBone);
	GetWorld()->LineTraceSingleByObjectType(FloorHit, Location, Location + FVector(0.0f, 0.0f, -500.0f), FCollisionObjectQueryParams(ECC_WorldStatic));

	bool bIsHorizontalSurface = FVector::DotProduct(FVector::UpVector, FloorHit.ImpactNormal) >= 0.75f;
	if (!bIsHorizontalSurface)
		return;
	
	ABloodPool* BloodPool = GetWorld()->SpawnActor<ABloodPool>(Blood->BloodPoolClass);
	if (BloodPool)
	{
		BloodPool->Decal->SetSortOrder(1);
		BloodPool->SetActorLocation(FloorHit.ImpactPoint);
		BloodPool->SetActorRotation(BloodPool->GetActorRotation().Add(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f));
	}
}

float AReadyOrNotCharacter::GetWoundSize(AActor* DamageCauser)
{
	if (const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser))
	{
		if (Weapon->GetCurrentAmmoType())
		{
			return Weapon->GetCurrentAmmoType()->WoundSize;
		}
	}
	return 35.0f;
}

bool AReadyOrNotCharacter::IsBoneArmored(FName BoneName) const
{
	if (HeadBones.Contains(BoneName))
	{
		return GetInventoryComponent()->GetHeadArmour() && GetInventoryComponent()->GetHeadArmour()->HasRemainingProtection();
	}

	if (UpperBody.Contains(BoneName) || BoneName == "spine_1" || BoneName == "spine_2")
	{
		return GetInventoryComponent()->GetArmour() && GetInventoryComponent()->GetArmour()->HasRemainingProtection();
	}

	return false;
}

ABaseArmour* AReadyOrNotCharacter::GetArmourForBone(FName BoneName)
{
	if (!GetInventoryComponent())
		return nullptr;
	
	if (HeadBones.Contains(BoneName))
	{
		return GetInventoryComponent()->GetHeadwear();
	}
	if (UpperBody.Contains(BoneName) || LowerBody.Contains(BoneName))
	{
		return GetInventoryComponent()->GetArmour();
	}

	return nullptr;
}

void AReadyOrNotCharacter::PlayArmourHitEffects(ABaseArmour* Armour, FHitResult Hit)
{
	if (!Armour || !Armour->ArmourHitParticle)
		return;

	const FTransform Transform(Hit.ImpactNormal.Rotation() + FRotator(-90, 0, 0), Hit.ImpactPoint);
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Armour->ArmourHitParticle, Transform);

	if (SkinnedDecalSampler && SkinnedDecalSampler->MaxDecals > SkinnedDecalsSpawned && Blood)
	{
		int32 SubUV = FMath::RandRange(0, Blood->SkinnedDecalImageCount);
		
		SkinnedDecalSampler->SpawnDecal(Hit.ImpactPoint, Hit.ImpactNormal.ToOrientationQuat(), Hit.BoneName, 25.0f, SubUV);
		SkinnedDecalsSpawned++;
	}
}

bool AReadyOrNotCharacter::IsOnlyStunnedWithGas() const
{
	// How many different types of stun damage are currently applied
	int32 StunTypesNum = 0;
	bool bGassed = false;
	for (const auto& k : StunMap)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(k.Key))
		{
			StunTypesNum++;

			if (k.Value == EStunType::ST_Gassed)
			{
				bGassed = true;
			}
		}
	}

	return (bGassed && StunTypesNum == 1);
}

bool AReadyOrNotCharacter::IsStunnedWith(const EStunType StunType) const
{
	if (!HasAuthority())
	{
		if (RepStunnedWith == StunType)
		{
			return bRepStunned;
		}

		if (StunType == EStunType::ST_None)
		{
			return bRepStunned;
		}
		
		return false;
	}
	
	if (StunType == EStunType::ST_None)
	{
		for (const auto& k : StunMap)
		{
			if (GetWorld()->GetTimerManager().IsTimerActive(k.Key))
			{
				return true;
			}
		}
	}
	
	for (const auto& k : StunMap)
	{
		if (k.Value == StunType && GetWorld()->GetTimerManager().IsTimerActive(k.Key))
		{
			return true;
		}
	}
	
	return false;
}

void AReadyOrNotCharacter::StartStun(EStunType StunType, AActor* StunCauser)
{
	#if !UE_BUILD_SHIPPING
	if (bGodMode)
		return;
	#endif
	
	if (StunType == EStunType::ST_None)
	{
		return;
	}

	float Duration = 6.0f;
	if (StunType == EStunType::ST_Gassed)
	{
		Duration = 1.0f;
	}

	bHasEverBeenStunned = true;
	
	if (bIsBeingArrested)
		return;
	
	CurrentStunDuration = Duration;
	
	GetWorld()->GetTimerManager().SetTimer(TH_EndStunTimer, FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::EndStun, StunType), Duration, false);
	StunMap.Add(TH_EndStunTimer, StunType);

	OnStunnedEvent.Broadcast(this, Duration, StunType, StunCauser);
}

void AReadyOrNotCharacter::EndStun(const EStunType StunType)
{
	StunMap.Empty();

	OnStunnedEndedEvent.Broadcast(StunType);
}

bool AReadyOrNotCharacter::IsPepperSprayedLocationValid(const FHitResult& Hit, APepperspray* Pepperspray)
{
	if (!Pepperspray)
		return false;
	
	const float ImpactDelta = FVector::DotProduct(GetActorForwardVector(), UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Hit.Location).Vector());
	const bool bHitFront = ImpactDelta < 0.0f;

	bool bValidHit = false;

	// Hit the head
	if (HeadBones.Contains(Hit.BoneName))
	{
		// Hit the front. Maximum casuality!
		if (bHitFront)
		{
			bValidHit = true;
		}
		// Hit the back
		else
		{
			bValidHit = true;
		}
	}
	// Hit the torso
	else if (UpperBody.Contains(Hit.BoneName) && bHitFront)
	{
		bValidHit = true;
	}

	return bValidHit;
}

void AReadyOrNotCharacter::StartPepperSprayed(APepperspray* PeppersprayUsed)
{
	if (!PeppersprayUsed)
	{
		return;
	}
	
	GetWorld()->GetTimerManager().SetTimer(TH_PepperSprayed, FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::EndPepperSprayed), 4, false);
}

void AReadyOrNotCharacter::EndPepperSprayed()
{
	TH_PepperSprayed.Invalidate();

	// Need to stop the pepperspray montage if it's running
	StopTPMontageFromTable("tp_swt_pain_flash");
}

void AReadyOrNotCharacter::StartBeingTasered(float PingStunDuration, ATaser* WeaponUsed)
{
	StartStun(EStunType::ST_Tased, WeaponUsed);
}

bool AReadyOrNotCharacter::CanBeTased()
{
	for (int32 i = 0; i < GetInventoryComponent()->GetInventoryItems().Num(); i++)
	{
		if (GetInventoryComponent()->GetInventoryItems()[i] && GetInventoryComponent()->GetInventoryItems()[i]->bTaserDamageBlocked)
		{
			return false;
		}
	}

	return true;
}

bool AReadyOrNotCharacter::IsAffectedByDamageType(UDamageType* DamageType) const
{
	for (int32 i = 0; i < GetInventoryComponent()->GetInventoryItems().Num(); i++)
	{
		if (GetInventoryComponent()->GetInventoryItems()[i])
		{
			for (const TSubclassOf<class UDamageType>& BlockedDamageType : GetInventoryComponent()->GetInventoryItems()[i]->BlockAnyDamageFrom)
			{
				if (DamageType->IsA(BlockedDamageType))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool AReadyOrNotCharacter::IsAffectedByDamageTypeClass(TSubclassOf<UDamageType> DamageType) const
{
	for (int32 i = 0; i < GetInventoryComponent()->GetInventoryItems().Num(); i++)
	{
		if (GetInventoryComponent()->GetInventoryItems()[i] && GetInventoryComponent()->GetInventoryItems()[i]->BlockAnyDamageFrom.Contains(DamageType))
		{
			for (TSubclassOf<class UDamageType> BlockedDamageType : GetInventoryComponent()->GetInventoryItems()[i]->BlockAnyDamageFrom)
			{
				if (DamageType->IsChildOf(BlockedDamageType))
				{
					return false;
				}
			}
		}
	}
	return true;
}

bool AReadyOrNotCharacter::HasBeenDamagedByLethal() const
{
	for (const ABaseWeapon* Weapon : DamagedByWeapons)
	{
		if (Weapon && Weapon->IsLethalWeapon())
			return true;
	}

	return false;
}

bool AReadyOrNotCharacter::HasBeenDamagedByLessLethal() const
{
	for (const ABaseWeapon* Weapon : DamagedByWeapons)
	{
		if (Weapon && Weapon->IsLessLethalWeapon())
			return true;
	}

	return false;
}

void AReadyOrNotCharacter::Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal)
{
}

void AReadyOrNotCharacter::Multicast_InflictSuppression_NoLineOfSight_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal)
{
}

void AReadyOrNotCharacter::PickupEvidence(AActor* InEvidence)
{
	if (!InEvidence)
		return;
	
	PendingEvidence = InEvidence;

	if (ABaseItem* Item = Cast<ABaseItem>(InEvidence))
	{
		Server_CollectEvidence(Item);
	}
	else if (AEvidenceActor* ItemAsEvidenceActor = Cast<AEvidenceActor>(InEvidence))
	{
		Server_CollectEvidenceActor(ItemAsEvidenceActor);
	}
}

void AReadyOrNotCharacter::CollectPendingEvidence()
{
	if (!PendingEvidence)
		return;
	
	if (ABaseItem* Item = Cast<ABaseItem>(PendingEvidence))
	{
		if (Item->IsEvidence())
		{
			Server_CollectEvidence(Item);
		}
	}
	else if (AEvidenceActor* ItemAsEvidenceActor = Cast<AEvidenceActor>(PendingEvidence))
	{
		if (!ItemAsEvidenceActor->IsEvidenceCollected())
		{
			ACollectedEvidenceActor* CollectedEvidenceActor = SpawnEvidenceCollectionBag(ItemAsEvidenceActor->GetActorTransform());
			CollectedEvidenceActor->Tags = ItemAsEvidenceActor->Tags;
			Server_CollectEvidenceActor(ItemAsEvidenceActor);
		}
	}
	else if (ACollectedEvidenceActor* ItemAsBag = Cast<ACollectedEvidenceActor>(PendingEvidence))
	{
		ItemAsBag->SetActorHiddenInGame(true);
	}
	
	PendingEvidence = nullptr;
}

ACollectedEvidenceActor* AReadyOrNotCharacter::SpawnEvidenceCollectionBag(FTransform SpawnTransform)
{
	if (!GetWorld())
	{
		return nullptr;
	}
	
	if (ACollectedEvidenceActor* EvidenceBag = GetWorld()->SpawnActorDeferred<ACollectedEvidenceActor>(CollectedEvidenceClass, SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		const FRotator SpawnRot = FRotator(GetActorRotation().Pitch, GetActorRotation().Yaw + 90.0f, GetActorRotation().Roll);

		SpawnTransform.SetLocation(SpawnTransform.GetLocation() + FVector(0.0f, 0.0f, 25.0f));
		SpawnTransform.SetRotation(SpawnRot.Quaternion());
		SpawnTransform.SetScale3D(FVector::OneVector);
		
		EvidenceBag->FinishSpawning(SpawnTransform);

		return EvidenceBag;
	}

	return nullptr;
}

void AReadyOrNotCharacter::BeginEvidenceCollection_COOP(AActor* InEvidenceActor, UInteractableComponent* CollectionInteractableComp, float CollectionTime)
{
	if (!InEvidenceActor)
		return;

	LockAllActions();
	
	bIsCollectingEvidence = true;

	if (GetEquippedItem())
	{
		GetEquippedItem()->PlayHolster();
	}

	if (CollectionInteractableComp)
	{
		CollectionInteractableComp->bShowIconWhenActionsLocked = true;
		CollectionInteractableComp->ActionSlot2.bUseCustomActionText = true;
		CollectionInteractableComp->CurrentProgress = 0.0f;
		CollectionInteractableComp->bOverrideTickInterval = true;
		CollectionInteractableComp->SetComponentTickInterval(0.0167f);
	}
	
	if (CollectionTime <= 0.0f)
		CompleteEvidenceCollection_COOP(InEvidenceActor);
	else
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_CompleteEvidenceCollection, this, FTimerDelegate::CreateUObject(this, &AReadyOrNotCharacter::CompleteEvidenceCollection_COOP, InEvidenceActor), CollectionTime, false);
}

void AReadyOrNotCharacter::EndEvidenceCollection_COOP(UInteractableComponent* CollectionInteractableComp)
{
	UnlockAllActions();

	bIsCollectingEvidence = false;
	bCollectionAnimHasTriggered = false;

	if (GetEquippedItem())
	{
		GetEquippedItem()->ClientPlayDraw(false);
	}

	if (CollectionInteractableComp)
	{
		CollectionInteractableComp->bShowIconWhenActionsLocked = false;
		CollectionInteractableComp->ActionSlot2.bUseCustomActionText = false;
		CollectionInteractableComp->CurrentProgress = 0.0f;
		CollectionInteractableComp->bOverrideTickInterval = false;
	}

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_CompleteEvidenceCollection);
}

void AReadyOrNotCharacter::CompleteEvidenceCollection_COOP(AActor* InEvidenceActor)
{
	UnlockAllActions();

	bIsCollectingEvidence = false;
	bCollectionAnimHasTriggered = false;

	if (GetEquippedItem())
	{
		GetEquippedItem()->PlayDraw(false);
	}
	
	if (!InEvidenceActor)
		return;


	if (AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(InEvidenceActor))
	{
		if (!EvidenceActor->IsEvidenceCollected())
		{
			EvidenceActor->CompleteEvidenceCollection_COOP();
		}
	}
	else if (ABaseItem* BaseItem = Cast<ABaseItem>(InEvidenceActor))
	{
		BaseItem->CompleteEvidenceCollection_COOP(this);
	}
}

void AReadyOrNotCharacter::TriggerCollectionAnim()
{
	Play1PMontage(CollectingLoopAnim1P);
	Play3PMontage(CollectingLoopAnim3P);
	bCollectionAnimHasTriggered = true;
}

void AReadyOrNotCharacter::StopEvidenceCollectingAnims()
{
	StopFPAnimMontage(CollectingLoopAnim1P);
	StopTPMontage(CollectingLoopAnim3P);
}

float AReadyOrNotCharacter::GetEvidenceCollectionTime() const
{
	if (IsLocalPlayer())
		return CollectingLoopAnim1P->GetPlayLength() - 0.24f;
	
	return CollectingLoopAnim3P->GetPlayLength() - 0.24f;
}

void AReadyOrNotCharacter::Multicast_ChangeFaceEmotion_Implementation(ECharacterEmotion NewEmotion, float OverrideTime, float Blend, float BlendDecay, int32 Priority)
{
	if (Priority < FacialAnimationPriority)
	{
		// Lower priority, don't alter anything
		return;
	}

	CurrentEmotion = NewEmotion;
	FacialAnimationOverrideTime = OverrideTime;
	FacialAnimationBlendTarget = Blend;
	FacialAnimationBlendDecay = BlendDecay;
	FacialAnimationPriority = Priority;
}

void AReadyOrNotCharacter::LockAllActions()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockAllActions()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockMovementAndActions()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockMovementAndActions()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockMovement()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockMovement()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockAim()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockAim()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockItemSelection()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockItemSelection()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockCommandMenu()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockCommandMenu()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockWeaponAttachments()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockWeaponAttachments()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::LockCantedSight()
{
	// Overriden in PlayerCharacter
}

void AReadyOrNotCharacter::UnlockCantedSight()
{
	// Overriden in PlayerCharacter
}

bool AReadyOrNotCharacter::OpenDoor(ADoor* Door, bool bOpenDoor)
{
	if (Door)
	{
		if (bOpenDoor)
		{
			Door->OpenDoor(this);
		}
		else
		{
			Door->CloseDoor(this);
		}

		return true;
	}

	return false;
}

void AReadyOrNotCharacter::KickDoor(ADoor* Door)
{
	if (Door)
	{
		// cannot kick an open door
		if (!Door->CanKickDoor(this))
			return;

		LastKickedDoor = Door;
		
		// Use the regular kick sequence
		UInteractionsData* ChosenDoorInteractionData = (Door->IsPointInFrontOfDoor(GetActorLocation()) ? DoorKickInteractionFront : DoorKickInteractionBack);

		// Play the correct kick sequence
		if (ChosenDoorInteractionData)
		{
			PlayPairedInteraction(ChosenDoorInteractionData, Door, this, nullptr);
			if (ChosenDoorInteractionData->SharedItemMontage)
				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotCharacter::UnlockAllActions, ChosenDoorInteractionData->SharedItemMontage->GetPlayLength() + 0.25f);
			else
				UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotCharacter::UnlockAllActions, 1.25f);
        	
			LockMovementAndActions();
		}
		else
		{
			UnlockAllActions();
		}
	}
}

void AReadyOrNotCharacter::SetPendingDoorForKick(ADoor* Door)
{
	LastKickedDoor = Door;
}

bool AReadyOrNotCharacter::IsOpeningDoor(ADoor* Door) const
{
	if (Door == QueuedDoorToOpen)
		return Door->IsOpening();

	return false;
}

bool AReadyOrNotCharacter::IsClosingDoor(ADoor* Door) const
{
	if (Door == QueuedDoorToClose)
		return Door->IsClosing();

	return false;
}

bool AReadyOrNotCharacter::CanPushDoor(ADoor* Door) const
{
	return true;
}

ABaseItem* AReadyOrNotCharacter::GetEquippedItem() const
{
	return InventoryComp ? InventoryComp->GetEquippedItem() : nullptr;
}

ABaseMagazineWeapon* AReadyOrNotCharacter::GetEquippedWeapon() const
{
	ABaseItem* EquippedItem = GetEquippedItem();
	
	if (const ABallisticsShield* BallisticsShield = Cast<ABallisticsShield>(EquippedItem))
	{
		return BallisticsShield->PistolEquippedWithShield;
	}

	return Cast<ABaseMagazineWeapon>(EquippedItem);
}

TArray<ABaseItem*> AReadyOrNotCharacter::GetRemovedItems() const
{
	return InventoryComp->GetRemovedInventoryItems();
}

void AReadyOrNotCharacter::OnItemEquipped(ABaseItem* NewEquippedItem)
{
	TInlineComponentArray<UItemVisualizationComponent*> VisualizationComponents;
	GetComponents(VisualizationComponents);
	
	for (UItemVisualizationComponent* V : VisualizationComponents)
	{
		V->UpdateItemVisualizationComponent();
	}
}

void AReadyOrNotCharacter::OnItemHolstered(ABaseItem* HolsteredItem)
{
	if (ABaseMagazineWeapon* HolsteredWeapon = Cast<ABaseMagazineWeapon>(HolsteredItem))
	{
		HolsteredWeapon->OnWeaponFire.RemoveAll(this);
		HolsteredWeapon->OnWeaponDryFire.RemoveAll(this);
	}
}

void AReadyOrNotCharacter::ThrowEquippedItem()
{
	if (InventoryComp)
	{
		ThrownItem = GetEquippedItem();
		InventoryComp->ThrowEquippedItem();
	}
}

void AReadyOrNotCharacter::ThrowAllWeapons()
{
	if (InventoryComp)
	{
		ThrownItem = GetEquippedItem();
		InventoryComp->ThrowAllWeapons();
	}
}

void AReadyOrNotCharacter::ThrowAllItems()
{
	if (InventoryComp)
	{
		InventoryComp->ThrowAllItems();
	}
}

void AReadyOrNotCharacter::ToggleNightvisionGoggles()
{
	/*
	Play Montage Animation on FP - TP for Nightvision

	FP Body sequences also control when to trigger NVGs on
	FP body sequences control camera fading which is set in the anim graph
	NVG anim bp controls visiblity of NVG in first person

	*/

	if (IsAnimationBlocking())
		return;

	if (IsCarried() || IsCarrying())
		return;

	ANightvisionGoggles* nvg = Cast<ANightvisionGoggles>(GetInventoryComponent()->GetInventoryItemOfClass(ANightvisionGoggles::StaticClass()));
	if (nvg)
	{
		if (GetEquippedItem())
		{
			if (GetEquippedItem()->SoundData)
			{
				if (bNVGOn)
				{
					GetEquippedItem()->PlayFMODAudio(GetEquippedItem()->SoundData->NightvisionOff);
				}
				else
				{
					GetEquippedItem()->PlayFMODAudio(GetEquippedItem()->SoundData->NightvisionOn);
				}
			}

			if (GetEquippedItem()->AnimationData)
			{
				if (bNVGOn)
				{
					// play the required animations
					// there is no need for crouch variants in first person right now
					if (!nvg->IsMontagePlaying(GetEquippedItem()->AnimationData->DisableNVG.Gun_FP))
					{
						nvg->PlayFPMontage(GetEquippedItem()->AnimationData->DisableNVG.Gun_FP);

						nvg->PlayTPMontage(GetEquippedItem()->AnimationData->DisableNVG.Gun_TP);
					}

					// BODY MOTIONS
					Play1PMontage(GetEquippedItem()->AnimationData->DisableNVG.Body_FP);
					Play3PMontage(GetEquippedItem()->AnimationData->DisableNVG.Body_TP);
				}
				else
				{
					// play the required animations
					// there is no need for crouch variants in first person right now
					if (!nvg->IsMontagePlaying(GetEquippedItem()->AnimationData->EnableNVG.Gun_FP))
					{
						nvg->PlayFPMontage(GetEquippedItem()->AnimationData->EnableNVG.Gun_FP);

						nvg->PlayTPMontage(GetEquippedItem()->AnimationData->EnableNVG.Gun_TP);
					}

					// BODY MOTIONS
					Play1PMontage(GetEquippedItem()->AnimationData->EnableNVG.Body_FP);
					Play3PMontage(GetEquippedItem()->AnimationData->EnableNVG.Body_TP);
				}
			}
		}
	}
}

void AReadyOrNotCharacter::EnableNightVisionGoggles()
{
	ANightvisionGoggles* NVG = Cast<ANightvisionGoggles>(GetInventoryComponent()->GetInventoryItemOfClass(ANightvisionGoggles::StaticClass()));
	if (NVG)
	{
		Server_RepNVGOn(true);
		bNVGOn = true;
		NVG->SpawnNightvisionWidget();
		NVG->UpdateNVGPostProcess();
		NVG->SetNightvisionGlobalMaterialParameters(true);
	}
}

void AReadyOrNotCharacter::Server_RepNVGOn_Implementation(bool bIsOn)
{
	bNVGOn = bIsOn;
	
	OnNightVisionGogglesToggled.Broadcast(this, bIsOn);
}

void AReadyOrNotCharacter::DisableNightVisionGoggles()
{
	ANightvisionGoggles* NVG = Cast<ANightvisionGoggles>(GetInventoryComponent()->GetInventoryItemOfClass(ANightvisionGoggles::StaticClass()));
	if (NVG)
	{
		Server_RepNVGOn(false);
		bNVGOn = false;
		NVG->DestroyNightvisionWidget();
		NVG->UpdateNVGPostProcess();
		NVG->SetNightvisionGlobalMaterialParameters(false);
	}
}

float AReadyOrNotCharacter::GetDeltaRotationToCharacter(AReadyOrNotCharacter* Character)
{
	if (!Character)
		return 0.0f;

	FVector v1 = GetControlRotation().Vector();
	FVector v2 = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Character->GetActorLocation()).Vector();
	v1.Z = 0;
	v2.Z = 0;
	
	return FVector::DotProduct(v1, v2);
}

FCollisionQueryParams AReadyOrNotCharacter::GetCollisionQueryParameters() const
{
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	
	if (GetInventoryComponent())
		CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)GetInventoryComponent()->GetInventoryItems());
	
	CollisionQueryParams.AddIgnoredComponents((TArray<UPrimitiveComponent*>)LowReadyIgnoredCapsules);
	
	return CollisionQueryParams;
}

TArray<AActor*> AReadyOrNotCharacter::GetCollisionIgnoredActors() const
{
	TArray<AActor*> OutActors;
	OutActors.Add(const_cast<AReadyOrNotCharacter*>(this));
	OutActors.Append(GetInventoryComponent()->GetInventoryItems());

	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		OutActors.Append(GameState->SpectatePawns);
	}
	
	return OutActors;
}

TArray<UPrimitiveComponent*> AReadyOrNotCharacter::GetCollisionIgnoredComponents() const
{
	TArray<UPrimitiveComponent*> OutComponents;
	
	OutComponents.Append((TArray<UPrimitiveComponent*>)LowReadyIgnoredCapsules);
	
	return OutComponents;
}

bool AReadyOrNotCharacter::HasLineOfSightTo(const FVector& Location) const
{
	if (Location == FVector::ZeroVector)
		return false;
	
	const FVector StartTrace = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
	const FVector EndTrace = Location;
	return !GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, GetCollisionQueryParameters());
}

bool AReadyOrNotCharacter::IsOutside()
{
	return bCachedIsOutside;
}

bool AReadyOrNotCharacter::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{
	SCOPE_CYCLE_COUNTER(STAT_CanBeSeenFrom);
	
	FCollisionQueryParams CollisionParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(const_cast<AReadyOrNotCharacter*>(this), const_cast<AReadyOrNotCharacter*>(Cast<AReadyOrNotCharacter>(IgnoreActor)));
	CollisionParams.bTraceComplex = true;
	CollisionParams.AddIgnoredActor(IgnoreActor);
	
	FVector StartTrace = ObserverLocation;
	FVector EndTrace = GetMesh()->GetBoneLocation("head");

	OutSeenLocation = EndTrace;
	
	#ifdef ENHANCED_SIGHT_DETECTION
	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(const_cast<AActor*>(IgnoreActor)))
	{
		AI->SeenBone = "head";
	}
	#endif
	
	bool bHit = GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, CollisionParams);
	NumberOfLoSChecksPerformed++;
	//DrawDebugLine(GetWorld(), StartTrace, EndTrace, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, GetWorld()->GetDeltaSeconds() + 0.001f);
	
	// fix one way collisions
	if (!bHit)
	{
		bHit = GetWorld()->LineTraceTestByChannel(EndTrace + GetActorRightVector() * 5.0f, StartTrace + GetActorRightVector() * 5.0f, ECC_Visibility, CollisionParams);
		NumberOfLoSChecksPerformed++;
		//DrawDebugLine(GetWorld(), EndTrace + GetActorRightVector() * 5.0f, StartTrace + GetActorRightVector() * 5.0f, HeadHitReverse.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f);
		return !bHit;
	}

	return !bHit;	
}

void AReadyOrNotCharacter::Client_SetControlRotation_Implementation(FRotator NewRotation)
{
	if (GetController())
	{
		GetController()->SetControlRotation(NewRotation);
	}
}

void AReadyOrNotCharacter::SpawnFootstepEffect()
{
	// explain to me how dead people can walk huh!
	if (IsDeadOrUnconscious() || IsInRagdoll())
		return;

	if (IsBeingCarried())
		return;

	if (!bIsRelevant)
		return;

	OnFootstep.Broadcast();

	// Footstep event and audio component
	UFMODEvent* FootstepsEvent = IsLocalPlayer() ? FootstepsLocal : FootstepsRemote;
	ABaseArmour* CurrentArmour = GetInventoryComponent()->GetArmour();
	int32 Stance = 0, Speed = 0, Surface = 2;
	if (!GetFMODFootstepParameters(Stance, Speed, Surface))
		return;
	
	// Third Person
	if (!IsLocalPlayer())
	{
		USoundSource* FootstepSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), FootstepsEvent, FTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if (FootstepSoundSource)
		{
			FootstepSoundSource->Attach(GetMesh(), "ik_foot_root");
			FootstepSoundSource->Transform = FTransform(FRotator(), GetMesh()->GetBoneLocation("ik_foot_root"), FVector(1, 1, 1));
			
			if (IsOnSWATTeam())
			{
				FootstepSoundSource->SetParameter("Armor", CurrentArmour ? CurrentArmour->bIsHeavy : false);
				FootstepSoundSource->SetParameter("Stance", Stance);
			}
			else
			{
				FootstepSoundSource->SetParameter("Velocity", GetVelocity().Size());
			}
			
			FootstepSoundSource->SetParameter("Surface", Surface);
			FootstepSoundSource->Play();
		}

		// Play Footstep Foley
		if (bShouldPlayFootstepFoley)
		{
			if (!bPlayEveryStep)
				bShouldPlayFootstepFoley = false;
			
			USoundSource* FootstepFoleySoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), CurrentFootstepFoleyEventRemote, FTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
			if (FootstepFoleySoundSource)
			{
				FootstepFoleySoundSource->Attach(GetMesh(), "ik_foot_root");
				
				FootstepFoleySoundSource->SetParameter("Stance", Stance);
				FootstepFoleySoundSource->SetParameter("Surface", Surface);

				if(FootstepSoundSource)
				{
					FootstepSoundSource->AddChild(FootstepFoleySoundSource);
				}
				FootstepFoleySoundSource->Play();
			}
		}

		// Movement foley (third person only)
		USoundSource* MovementFoleySoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), MovementFoley, FTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if (MovementFoleySoundSource)
		{
			MovementFoleySoundSource->Attach(GetMesh(), MovementFoleySocket);
			MovementFoleySoundSource->Transform = FTransform(FRotator(), GetMesh()->GetBoneLocation(MovementFoleySocket), FVector(1, 1, 1));

			MovementFoleySoundSource->Play();
		}
	}
	// First Person
	else
	{
		USoundSource* FootstepSoundSource = USoundSource::CreateFirstPersonSound(GetWorld(), FootstepsEvent, FTransform(), {}, false);
		if(FootstepSoundSource)
		{
			FootstepSoundSource->Attach(GetMesh(), "ik_foot_root");
			
			if (IsOnSWATTeam())
			{
				FootstepSoundSource->SetParameter("Stance", Stance);
			}
			
			FootstepSoundSource->SetParameter("Surface", Surface);
			FootstepSoundSource->Play();
		}
		
		// Play Footstep Foley
		if (bShouldPlayFootstepFoley)
		{
			if (!bPlayEveryStep)
				bShouldPlayFootstepFoley = false;
			
			USoundSource* FootstepFoleySoundSource = USoundSource::CreateFirstPersonSound(GetWorld(), CurrentFootstepFoleyEvent, FTransform(), {}, false);
			if (FootstepFoleySoundSource)
			{
				FootstepFoleySoundSource->Attach(GetMesh(), "ik_foot_root");
				
				if (IsOnSWATTeam())
				{
					FootstepFoleySoundSource->SetParameter("Stance", Stance);
				}
				
				FootstepFoleySoundSource->Play();
			}
		}
	}
	
	// Movement Layer
	UFMODEvent* MovementLayerEvent = nullptr;
	//UFMODAudioComponent* MovementLayer = nullptr;
	if (GetEquippedItem() && GetEquippedItem()->SoundData != nullptr)
	{
		MovementLayerEvent = GetEquippedItem()->SoundData->MovementLayer;
		//FMODMovementAudioComp->Event = MovementLayerEvent;
		//MovementLayer = UFMODBlueprintStatics::PlayEventAttached(MovementLayerEvent, GetMesh(), "ik_foot_root", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, false, true);
	}
	
	if (MovementLayerEvent != nullptr)
	{
		UFMODWorldSubsystem::Get(this)->PlayAudioAttached_AnimNotify(MovementLayerEvent, GetMesh(), "ik_foot_root");
		//FMODMovementAudioComp->Play();
	}

	// Footstep impact effect
	FVector LeftFootLoc = GetMesh()->GetBoneLocation("foot_le");
	FVector RightFootLoc = GetMesh()->GetBoneLocation("foot_ri");
	FTransform SpawnTransform = LeftFootLoc.Z < RightFootLoc.Z ? FTransform(LeftFootLoc) : FTransform(RightFootLoc);

	FVector DownLoc = SpawnTransform.GetLocation();
	DownLoc.Z -= 100;
	
	FHitResult DownTrace;
	FCollisionQueryParams CollisionQueryParams = GetCollisionQueryParameters();
	CollisionQueryParams.bReturnPhysicalMaterial = true;
	GetWorld()->LineTraceSingleByChannel(DownTrace, LeftFootLoc, DownLoc, ECC_Visibility, CollisionQueryParams);

	bool bSpawnImpactForSurface = false;
	UPhysicalMaterial* PhysicalMaterial = DownTrace.PhysMaterial.Get();
	if (IsValid(PhysicalMaterial))
	{
		switch (PhysicalMaterial->SurfaceType)
		{
		case SurfaceType2:		// Asphalt
		case SurfaceType9:		// Dirt
		case SurfaceType21:		// Gravel
		case SurfaceType25:		// Leaves
		case SurfaceType41:		// Sand
		case SurfaceType43:		// Soil
			bSpawnImpactForSurface = true;
		default: break;
		};
	}
	
	if (DownTrace.bBlockingHit && bSpawnImpactForSurface)
	{
		const FName FootstepPool = GetVelocity().Size2D() > 280.0f ? "Fast Footstep Impact Pool" : "Slow Footstep Impact Pool";
		if (UObjectPoolBase* FootstepEffectsPool = UObjectPoolFunctionLibrary::GetObjectPool(this, FootstepPool))
		{
			if (AImpactEffect* Impact = FootstepEffectsPool->GetActorFromPool<AImpactEffect>())
			{
				Impact->MaxLifespan = 5.0f;
				Impact->bTraceComplex = true;
				
				Impact->PooledActor_BeginPlay();
				Impact->TriggerImpactEffect(DownTrace);
			}
		}
	}
}

bool AReadyOrNotCharacter::GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface)
{
	Speed = FMath::CeilToInt(UKismetMathLibrary::NormalizeToRange(GetVelocity().Size2D(), 0.0f, 350.0f) * 5.0f);

	FCollisionQueryParams CollisionParams = GetCollisionQueryParameters();
	CollisionParams.bReturnPhysicalMaterial = true;

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		CollisionParams.AddIgnoredActors((TArray<AActor*>)GS->AllReadyOrNotCharacters);
	
	if (AReadyOrNotLevelScript* LS = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
		CollisionParams.AddIgnoredActors((TArray<AActor*>)LS->BlockingVolumesInLevel);
	
	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FVector StartTrace = GetActorLocation();
	FVector EndTrace = StartTrace + GetActorUpVector() * -300.0f;

	FHitResult HitResult;

	/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
	//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
	//GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);
	GetWorld()->LineTraceSingleByObjectType(HitResult, StartTrace, EndTrace, CollisionObjectQueryParams, CollisionParams);

	UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

	Surface = (int32)HitSurfaceType;
	
	if (!IsValid(HitPhysMat) || Surface <= 0)
	{
		Surface = 2;
	}

	return HitResult.bBlockingHit;
}

void AReadyOrNotCharacter::StartFoley(bool bShouldPlayEveryStep, UFMODEvent* LocalFoleyEvent, UFMODEvent* RemoteFoleyEvent)
{
	bShouldPlayFootstepFoley = true;
	bPlayEveryStep = bShouldPlayEveryStep;
	CurrentFootstepFoleyEvent = LocalFoleyEvent;
	CurrentFootstepFoleyEventRemote = RemoteFoleyEvent;
}

void AReadyOrNotCharacter::StopFoley()
{
	bShouldPlayFootstepFoley = false;
}

void AReadyOrNotCharacter::SetPhysicsAsset(UPhysicsAsset* NewPhysicsAsset, const bool bForce)
{
	if (!HasAuthority() && !bForce)
		return;
	
	Rep_ActiveRagdollPhysAsset = NewPhysicsAsset;

	// New physics asset set?
	if (GetMesh()->GetPhysicsAsset() != Rep_ActiveRagdollPhysAsset)
	{
		OnRep_ActiveRagdollPhysAsset();
	}
}

void AReadyOrNotCharacter::SetAppropriatePhysicsAsset(bool bForce)
{
	SetPhysicsAsset(GetAppropriatePhysicsAsset(), bForce);
}

UPhysicsAsset* AReadyOrNotCharacter::GetAppropriatePhysicsAsset()
{
	if (IsArrestedAndDead() || (IsIncapacitated() && bArrestComplete) || IsArrested())
	{
		return CuffedRagdollPhysAsset;
	}

	if (IsDeadOrUnconscious() || IsIncapacitated())
	{
		return DefaultRagdollPhysAsset;
	}
	
	return DefaultAlivePhysAsset;
}

void AReadyOrNotCharacter::OnRep_ActiveRagdollPhysAsset()
{
	if (GetMesh()->GetPhysicsAsset() != Rep_ActiveRagdollPhysAsset)
	{
		GetMesh()->SetPhysicsAsset(Rep_ActiveRagdollPhysAsset, true);
	}
}

bool AReadyOrNotCharacter::HasRecentlyShot()
{
	if (const ABaseMagazineWeapon* bmw = Cast<ABaseMagazineWeapon>(GetEquippedItem()))
	{
		if (bmw->TimeSinceLastShot < 1.0f)
			return true;
	}
	return false;
}

void AReadyOrNotCharacter::Server_Interact_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_Interact(Interactable, this, InInteractableComponent);
}

void AReadyOrNotCharacter::Server_EndInteract_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_EndInteract(Interactable, this, InInteractableComponent);
}

void AReadyOrNotCharacter::Server_DoubleTapInteract_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_DoubleTapInteract(Interactable, this, InInteractableComponent);
}

void AReadyOrNotCharacter::Server_MeleeInteract_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_MeleeInteract(Interactable, this, InInteractableComponent);
}

void AReadyOrNotCharacter::Server_Interact_PrimaryUse_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_Fire(Interactable, this, InInteractableComponent);
}

void AReadyOrNotCharacter::Server_EndInteract_PrimaryUse_Implementation(UObject* Interactable, UInteractableComponent* InInteractableComponent)
{
	if (InInteractableComponent)
		Execute_EndFire(Interactable, this, InInteractableComponent);
}

ANeutralizeSuspectByTag* AReadyOrNotCharacter::GetNeutralizeSuspectTag()
{
	if(NeutralizeSuspectTag != nullptr)
		return NeutralizeSuspectTag;

	NeutralizeSuspectTag = Cast<ANeutralizeSuspectByTag>(UGameplayStatics::GetActorOfClass(GetWorld(), ANeutralizeSuspectByTag::StaticClass()));
	return NeutralizeSuspectTag;
}

void AReadyOrNotCharacter::Multicast_AddMoveIgnoreActor_Implementation(AReadyOrNotCharacter* MoveIgnoreCharacter, bool bAdd)
{
	if (!MoveIgnoreCharacter)
		return;
	
	if (bAdd)
	{
		MoveIgnoreActorAdd(MoveIgnoreCharacter);
		MoveIgnoreCharacter->MoveIgnoreActorAdd(this);
	} else
	{
		MoveIgnoreActorRemove(MoveIgnoreCharacter);
		MoveIgnoreCharacter->MoveIgnoreActorRemove(this);
	}
}

void AReadyOrNotCharacter::Server_KickFailQueuedDoor_Implementation()
{
	LastKickedDoor = nullptr;
}

bool AReadyOrNotCharacter::Server_KickFailQueuedDoor_Validate()
{
	return true;
}

void AReadyOrNotCharacter::Server_KickBreakQueuedDoor_Implementation()
{
	if (LastKickedDoor)
	{
		LastKickedDoor->KickDoor(this, LastKickedDoor->IsPendingSubDoorKick());
	}
	
	LastKickedDoor = nullptr;
}

bool AReadyOrNotCharacter::Server_KickBreakQueuedDoor_Validate()
{
	return true;
}

void AReadyOrNotCharacter::Server_KickQueuedDoor_Implementation()
{
	if (LastKickedDoor)
	{
		LastKickedDoor->KickDoor(this, LastKickedDoor->IsPendingSubDoorKick());
	}
	
	LastKickedDoor = nullptr;
}

bool AReadyOrNotCharacter::Server_KickQueuedDoor_Validate()
{
	return true;
}

void AReadyOrNotCharacter::Server_CollectEvidenceActor_Implementation(AEvidenceActor* InEvidenceActor)
{
	if (!InEvidenceActor)
		return;
	InEvidenceActor->ActorPickedUp(this);
	InEvidenceActor->DestroyEvidence();
}

bool AReadyOrNotCharacter::Server_CollectEvidenceActor_Validate(AEvidenceActor* InEvidenceActor)
{
	return true;
}

void AReadyOrNotCharacter::Server_CollectEvidence_Implementation(ABaseItem* Item)
{
	if (IsValid(Item))
	{
		if (Item->IsActorBeingDestroyed())
			return;
		
		// ##UE5UPGRADE##
		//if (Item->IsPendingKillOrUnreachable())
		//	return;
		
		if (Item->IsEvidence())
		{
			if (IsValid(Item->ScoringComponent))
			{
				FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "EvidenceSecuredWithName"), Item->ItemName);
				Item->ScoringComponent->GiveAllScores(true, true, ScoreText);
			}

			Item->SetActorHiddenInGame(true);
			Item->SetActorEnableCollision(false);

			if (Item->GetItemMesh())
				Item->GetItemMesh()->SetSimulatePhysics(false);

			Item->OnEvidenceCollected.Broadcast();
			OnEvidenceCollected.Broadcast(Item);

			if (ACollectedEvidenceActor* CollectedEvidenceActor = SpawnEvidenceCollectionBag(Item->GetItemMesh() ? Item->GetItemMesh()->GetComponentTransform() : Item->GetActorTransform()))
			{
				UScoringComponent* NewScoringComp = NewObject<UScoringComponent>(CollectedEvidenceActor, FName("ScoringComp"), RF_NoFlags, Item->ScoringComponent);
				NewScoringComp->RegisterComponent();
				NewScoringComp->SetIsReplicated(true);
				NewScoringComp->AddToScorePool();
				NewScoringComp->GiveAllScores();
				CollectedEvidenceActor->Tags = Item->Tags;
			}
			
			// Destroy this after collecting it
			Item->Destroy();
		}
	}
}

bool AReadyOrNotCharacter::Server_CollectEvidence_Validate(ABaseItem* Item)
{
	return true;
}

void AReadyOrNotCharacter::Client_PlayFMODEvent2D_Implementation(UFMODEvent* Event)
{
	UFMODBlueprintStatics::PlayEventAttached(Event, GetMesh(), "head", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
}

void AReadyOrNotCharacter::Client_PlayScreenShake_Implementation(TSubclassOf<ULegacyCameraShake> CameraShake)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ClientStartCameraShake(CameraShake);
	}
}

AReadyOrNotPlayerController* AReadyOrNotCharacter::GetRONPlayerController()
{
	return Cast<class AReadyOrNotPlayerController>(GetController());
}

FString AReadyOrNotCharacter::GetSpeechCharacterName() const
{
	if (IsLocalPlayer())
	{
		if (UReadyOrNotFunctionLibrary::IsSinglePlayer(GetWorld()) || SpeechCharacterName.IsEmpty())
		{
			return "SwatJudge";
		}
	}

	if (!CharacterLookOverride.SpeechCharacterName.IsEmpty())
		return CharacterLookOverride.SpeechCharacterName;
	
	return SpeechCharacterName;
}

bool AReadyOrNotCharacter::IsInPositionForCarry(const AReadyOrNotCharacter* Carrier) const
{
#ifdef CARRY_ARRESTED
	if (CanBePickedUp())
	{
		const FVector V1 = GetActorRotation().Vector();
		const FVector V2 = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Carrier->GetActorLocation()).Vector();

		if (FVector::DotProduct(V1, V2) > 0.0f)
		{
			return true;
		}

		return false;
	}
#endif
	return false;
}

bool AReadyOrNotCharacter::IsInPositionForArrest(const AReadyOrNotCharacter* ArrestTarget) const
{
	if (CanArrest())
	{
		if ((IsDeadOrUnconscious() || IsIncapacitated() || IsInRagdoll()) && !bBlendInPhysics)
		{
			return true; // Can arrest from any direction when in ragdoll
		}
		
		const FVector V1 = UKismetMathLibrary::GetForwardVector(GetMesh()->GetSocketRotation("pelvis"));
		const FVector V2 = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ArrestTarget->GetActorLocation()).Vector();

		if (FVector::DotProduct(V1, V2) > 0.0f)
		{
			return false;
		}

		return true;
	}

	return false;
}

bool AReadyOrNotCharacter::CanInteract_Implementation() const
{
	//return CanShowActionPrompt1() && (!bHasBeenReported || CanArrest() || IsSurrendered() || CanBePickedUp() || CanArrestRagdoll());

	if (!CanShowActionPrompt1())
		return false;

	if (CanArrest())
		return true;

	if (CanArrestRagdoll() || PendingRagdollArrestCharacter)
		return true;

	if (IsSurrendered())
		return true;

	LOCAL_PLAYER;
	if (IsInPositionForCarry(LocalPlayer))
		return true;

	if (!bHasBeenReported)
		return true;

	return false;
}

bool AReadyOrNotCharacter::CanShowActionPrompt1() const
{
	return !InteractableComponent->AnimatedIconName.IsNone();
}

void AReadyOrNotCharacter::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	// Is new stimulus?
	if (Stimulus.GetAge() == 0.0f)
	{
		FActorSense SightSense;
		SightSense.Actor = this;
		SightSense.Tag = Stimulus.Tag;
		SightSense.Stimulus = Stimulus;
		const bool bHeardNoise = InSenseController->HasBeenExposedToAnyNoise(1.0f, 2000.0f, (int32)EAITargetType::Enemy);
		SightSense.SenseReactionTime = bHeardNoise && !InSenseController->bHasEverSpottedEnemyBefore ? 0.0f : InSenseController->GetReactionTime(EActorSenseType::Sight);
		SightSense.SenseForgetTime = 30.0f;
		
		InSenseController->AddActorSightSense(SightSense);
	}
}

void AReadyOrNotCharacter::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (IsOnSameTeam(this, InSenseController->GetCharacter()))
		return;
	
	// Is new stimulus?
	if (Stimulus.GetAge() == 0.0f)
	{
		FActorSense SoundSense;
		SoundSense.Actor = this;
		SoundSense.Tag = Stimulus.Tag;
		SoundSense.Stimulus = Stimulus;
		SoundSense.SenseReactionTime = InSenseController->GetReactionTime(EActorSenseType::Sound);
		SoundSense.SenseForgetTime = 30.0f;
		
		InSenseController->AddActorSoundSense(SoundSense);
	}
}

void AReadyOrNotCharacter::OnAIDamageSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	FActorSense DamageSense;
	DamageSense.Actor = this;
	DamageSense.Tag = Stimulus.Tag;
	DamageSense.Stimulus = Stimulus;
	DamageSense.SenseReactionTime = InSenseController->GetReactionTime(EActorSenseType::Damage);
	DamageSense.SenseForgetTime = 30.0f;
	
	InSenseController->AddActorDamageSense(DamageSense);
}

TArray<UAnimMontage*> AReadyOrNotCharacter::GetMontagesFromTableRow(FString RowName)
{
	TArray<UAnimMontage*> OutputArray;

	if (!UBpGameplayHelperLib::GetRoNData())
		return OutputArray;

	if (const UDataTable* dt = UBpGameplayHelperLib::GetRoNData()->AnimationDataLookupTable)
	{
		if (FAnimationDataTable* LookupRow = dt->FindRow<FAnimationDataTable>(*RowName, "Animation Lookup"))
		{
			EAnimWeaponType cwt = EAnimWeaponType::CWT_Any;

			if (LookupRow->AnimData.Find(cwt))
			{
				// use any 
				cwt = EAnimWeaponType::CWT_Any;
			}
			else
			{
				bool bSecondMatch = false;
				for (const auto& row : LookupRow->AnimData)
				{
					cwt = row.Key;
					bSecondMatch = true;
				}

				if (!bSecondMatch)
				{
					return OutputArray;
				}
			}

			const FAnimStanceData StanceData = *LookupRow->AnimData.Find(cwt);
			const FAnimWeaponData WeaponAnimData = IsCrouching() ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;

			if (WeaponAnimData.AnimMontages.Num() > 0)
			{
				return WeaponAnimData.AnimMontages;
			}
		}
	}

	return OutputArray;
}

void AReadyOrNotCharacter::OnRep_ReplicatedAcceleration()
{
	if (UReadyOrNotCharMovementComp* RonMovementComponent = Cast<UReadyOrNotCharMovementComp>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel = RonMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * RON_TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		double X, Y;
		RonPolarToCartesian(AccelXYMagnitude, AccelXYRadians, X, Y);
		
		FVector UnpackedAcceleration;
		UnpackedAcceleration.X = (float)X;
		UnpackedAcceleration.Y = (float)Y;
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]

		RonMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void AReadyOrNotCharacter::TestPhysicalAnimationComponent()
{
	/*
	if (!IsLocalPlayer())
	{
		// set mesh for physical animation component
		GetPhysicalAnimationComp()->SetSkeletalMeshComponent(GetMesh());
		FPhysicalAnimationData NewData;
		NewData.bIsLocalSimulation = false;
		NewData.OrientationStrength = 1000.0f;
		NewData.AngularVelocityStrength = 100.0f;
		NewData.PositionStrength = 1000.0f;
		NewData.VelocityStrength = 100.0f;
		NewData.MaxLinearForce = 0.0f;
		NewData.MaxAngularForce = 0.0f;
		GetPhysicalAnimationComp()->ApplyPhysicalAnimationSettingsBelow("pelvis", NewData, true);
		GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
		V_LOGM(LogReadyOrNot, "-------------- PHYSICAL ANIMATION ACTIVE!");
	}
	*/
}

void AReadyOrNotCharacter::UpdateCapsuleFloorAngleRagdollTrigger(float DeltaSeconds)
{
	// dont bother if we triggered ragdoll from a bump
	if (bCapsuleCollisionRagdolled)
		return;

	// make sure movement component is valid!
	if (GetCharacterMovement())
	{
		// only update if we are actually having a valid hit
		if (GetCharacterMovement()->CurrentFloor.HitResult.bBlockingHit)
		{
			float NormalDotProduct = FVector::DotProduct(FVector::UpVector, GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal);
			float CurrentFloorAngle = FMath::RadiansToDegrees(FMath::Acos(NormalDotProduct));

			if (CurrentFloorAngle >= CapsuleFloorAngleRagdollTriggerThreshold)
			{
				if ( ( IsDeadOrUnconscious() || IsIncapacitated() ) && !IsInRagdoll() && !bCapsuleFloorAngleRagdolled)
				{
					CapsuleFloorAngleDelayCounter += DeltaSeconds;

					if (CapsuleFloorAngleDelayCounter >= CapsuleFloorAngleRagdollDelayThreshold)
					{
						UE_LOG(LogTemp, Warning, TEXT("AReadyOrNotCharacter::UpdateCapsuleFloorAngleRagdollTrigger Triggering early ragdoll due to floor angle!"));

						// draw debug sphere at location the ragdoll was triggered, just for visual clue if this was really triggered...
						DrawDebugSphere(GetWorld(), GetActorLocation(), 16.0f, 32.0f, FColor::Magenta, true, 5.0f, 0, 0.1f);

						//EnableRagdoll(0.5f);
						RequestAnim2RagdollBlend(0.55f);

						bCapsuleFloorAngleRagdolled = true;
						CapsuleFloorAngleDelayCounter = 0.0f;
					}
				}
			}
		}
	}
}

void AReadyOrNotCharacter::RequestAnim2RagdollBlend(float Duration)
{
	/* only allow new request if we are not already doing one*/
	if (!IsInRagdoll() && !bBlendingAnim2Ragdoll)
	{
		UE_LOG(LogTemp, Warning, TEXT("AReadyOrNotCharacter::RequestAnim2RagdollBlend Starting new Anim 2 Ragdoll Blend!"));

		// call original just to make sure
		EnableRagdoll();

		GetMesh()->SetCollisionProfileName("Ragdoll", true);

		// Always enable CCD on the pelvis to reduce whole body clipping into/through walls due to velocity/framerate.
		GetMesh()->SetUseCCD(true, "pelvis");

		GetMesh()->SetAllBodiesBelowSimulatePhysics("pelvis", true, true);
		//GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", 1.0f, false, true);
		bBlendInPhysics = true;

		bBlendingAnim2Ragdoll = true;
		CurrentAnim2RagdollEndTime = Duration;
	}
}

void AReadyOrNotCharacter::UpdateAnim2RagdollBlend(float DeltaSeconds)
{
	if (bBlendingAnim2Ragdoll)
	{
		UE_LOG(LogTemp, Warning, TEXT("AReadyOrNotCharacter::UpdateAnim2RagdollBlend Performing Ragdoll Blend!"));

		bBlendInPhysics = true;

		CurrentAnim2RagdollTime += DeltaSeconds;
		CurrentAnim2RagdollBlendValue = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, CurrentAnim2RagdollEndTime), FVector2D(0.0f, 1.0f), CurrentAnim2RagdollTime);
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("pelvis", CurrentAnim2RagdollBlendValue, false, true);

		/* if we reach the full blend strength stop blending */
		if (CurrentAnim2RagdollBlendValue >= 1.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("AReadyOrNotCharacter::UpdateAnim2RagdollBlend Ragdoll blend completed!"));

			if (GetCapsuleComponent())
			{
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
			}

			if (GetCharacterMovement())
			{
				GetCharacterMovement()->StopMovementImmediately();
				GetCharacterMovement()->DisableMovement();
			}
			GetMesh()->GetAnimInstance()->StopAllMontages(0.15f);

			CurrentAnim2RagdollTime = 0.0f;
			bBlendInPhysics = false;
			bBlendingAnim2Ragdoll = false;
		}
	}
}

void AReadyOrNotCharacter::SetPhysicsAssetAngularMotorAnimInfluence(UPhysicsAsset* PhysicsAsset, float InSpring, float InDamping, float InForceLimit, bool bSkipCustomPhysicsType)
{
	if (!PhysicsAsset)
	{
		return;
	}

	for (int32 i = 0; i < GetMesh()->Constraints.Num(); i++)
	{
		if (bSkipCustomPhysicsType)
		{
			int32 BodyIndex = PhysicsAsset->FindBodyIndex(GetMesh()->Constraints[i]->JointName);
			if (BodyIndex != INDEX_NONE && PhysicsAsset->SkeletalBodySetups[BodyIndex]->PhysicsType != PhysType_Default)
			{
				continue;
			}
		}
		GetMesh()->Constraints[i]->SetAngularDriveParams(InSpring, InDamping, InForceLimit);

		if(GetMesh()->Constraints[i]->JointName == "root")
			continue;

		if (GetMesh()->Constraints[i]->JointName == "pelvis")
			continue;

		GetMesh()->Constraints[i]->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
	}
}

void AReadyOrNotCharacter::SaveCharacterSnapshot()
{
	if (!GetMesh() || !HasAuthority())
		return;

	const UHitRegistrationSettings* HitRegistrationSettings = GetDefault<UHitRegistrationSettings>();

	FBox CharacterBounds = GetMesh()->Bounds.GetBox();

	// Increase bounding box in direction this character was moving
	FVector VelocityPoint = CharacterBounds.GetCenter() + (GetVelocity() * HitRegistrationSettings->SnapshotVelocityFactor);
	CharacterBounds += VelocityPoint;

	// Increase bounding box by a set amount for forgiveness
	float ForgivenessAmount = FMath::Max(0.0f, HitRegistrationSettings->HitscanForgiveness);
	CharacterBounds = CharacterBounds.ExpandBy(ForgivenessAmount);

	// Enforce minimum size of the bounding box
	FVector MinimumExtent = HitRegistrationSettings->MinimumSnapshotExtent;
	CharacterBounds += FBox::BuildAABB(CharacterBounds.GetCenter(), MinimumExtent.GetAbs());
	
	FCharacterSnapshot Snapshot;
	Snapshot.Time = GetWorld()->TimeSeconds;
	Snapshot.BoundingBox = CharacterBounds;
	
	Snapshots.Add(Snapshot);
	
	if (Snapshots.Num() >= MaxSnapshots)
		Snapshots.RemoveAt(0, 1, false);

	if (CVarRonDrawCharacterSnapshots.GetValueOnAnyThread() != 0 && !IsLocalPlayer())
	{
		DrawDebugBox(GetWorld(), CharacterBounds.GetCenter(), CharacterBounds.GetExtent(), FColor::Yellow, false);
	}
}
