// Copyright Void Interactive, 2023

#include "Door.h"

#include "BaseGrenade.h"
#include "CableComponent.h"
#include "CoverLandmark.h"
#include "NavLinkCustomComponent.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotDebugSubsystem.h"

#include "StackUpActor.h"

#include "Items/DoorRam.h"
#include "Items/DoorJam.h"
#include "Items/C2Explosive.h"
#include "Items/Multitool.h"
#include "Items/Optiwand.h"

#include "Gameplay/PlacedC2Explosive.h"
#include "Gameplay/TrapActorAttachedToDoor.h"

#include "Characters/CyberneticController.h"

#include "Components/DoorwayComponent.h"
#include "Components/MirrorPortalComponent.h"
#include "Components/DestructibleDoorChunkComponent.h"
#include "Components/InteractableComponent.h"

#include "Net/UnrealNetwork.h"

#include "Info/SuspectsAndCivilianManager.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "WorldDataGenerator.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Characters/AI/SWATCharacter.h"
#include "Commander/RosterManager.h"
#include "Components/MoraleComponent.h"
#include "Components/SplineComponent.h"
#include "DamageTypes/TrapDamage.h"
#include "DamageTypes/LessLethal/CSGasDamageType.h"
#include "Engine/BrushBuilder.h"

#include "Engine/DemoNetDriver.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/Activities/Team/DisarmDoorTrapActivity.h"
#include "Items/BreachingShotgun.h"
#include "Items/LockpickGun.h"
#include "NavAreas/NavArea_Default.h"
#include "NavAreas/NavArea_Null.h"

#include "Navigation/ReadyOrNotNavAreas.h"
#include "Navigation/NavLinkProxy.h"
#include "Navigation/ReadyOrNotNavQueries.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "Perception/AISense_Hearing.h"

#include "Projectiles/DamageProjectiles/GrenadeProjectile.h"

#include "Senses/ReadyOrNotAISense_Sight.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

DECLARE_CYCLE_STAT(TEXT("Door ~ Tick"), STAT_DoorTick, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Check Door Blocked"), STAT_CheckDoorBlockedByTraceChannel, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Check Door Hit"), STAT_CheckDoorHit, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Action Prompt Conditions"), STAT_TickActionPromptConditions, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Action Prompt Update"), STAT_TickActionUpdate, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Take Damage"), STAT_DoorTakeDamage, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Nav Modifier Volume Update"), STAT_NavModiferVolumeUpdate, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Attached Wedge Location Update"), STAT_AttachedWedgeLocationUpdate, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Timeline Ticks"), STAT_TimelineTicks, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Door Chunk Nav Update"), STAT_ChunkNavUpdate, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Update Door Rotation"), STAT_UpdateDoorRotation, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Nav Link Checks"), STAT_NavLinkChecks, STATGROUP_Door);
DECLARE_CYCLE_STAT(TEXT("Door ~ Cache Attached Actors"), STAT_AttachedActorChecks, STATGROUP_Door);

const FName ADoor::MOVE_DOOR_NOISE_TAG = "MoveDoor";
const FName ADoor::KICK_DOOR_NOISE_TAG = "KickDoor";
const FName ADoor::EXPLODE_DOOR_NOISE_TAG = "ExplodeDoor";
const FName ADoor::RAM_DOOR_NOISE_TAG = "RamDoor";

TAutoConsoleVariable<int32> CVarRonDrawDoorInteractions(TEXT("a.RonDrawDoorInteractions"), 1, TEXT("0 = No draw door interaction editor sprites. 1 = Draw door interaction editor sprites"));
TAutoConsoleVariable<int32> CVarRonDrawDoorPathTestPoints(TEXT("a.RonDrawDoorPathTestPoints"), 0, TEXT("0 = No draw door path test points. 1 = Draw door path test points"));
TAutoConsoleVariable<int32> CVarRonDrawDoorCollisions(TEXT("a.RonDrawDoorCollisions"), 0, TEXT("0 = No draw door collisions. 1 = Draw door collisions"));

ADoor::ADoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;

	IFMODStudioModule::Get();

	bCanPlayerInteract = true;
	bLocked = false;
	
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Static);

	DoorStatic = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorStatic"));
	DoorStatic->SetCollisionObjectType(ECC_DOOR);
	DoorStatic->SetGenerateOverlapEvents(false);
	DoorStatic->SetNotifyRigidBodyCollision(true);
	DoorStatic->SetCanEverAffectNavigation(true);
	DoorStatic->SetAllUseCCD(true);
	DoorStatic->SetupAttachment(RootComponent);
	DoorStatic->ComponentTags.AddUnique("NoCover");
	
	ChunkComponents.Empty();
	DoorChunk0 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk0");
	DoorChunk0->SetupAttachment(RootComponent);
	// Fix AI getting stuck on door
	DoorChunk0->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ChunkComponents.Add(DoorChunk0);

	DoorChunk1 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk1");
	DoorChunk1->SetupAttachment(RootComponent);
	// Fix AI getting stuck on door
	DoorChunk1->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ChunkComponents.Add(DoorChunk1);

	DoorChunk2 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk2");
	DoorChunk2->SetupAttachment(RootComponent);
	// Fix AI getting stuck on door
	DoorChunk2->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ChunkComponents.Add(DoorChunk2);

	DoorChunk3 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk3");
	DoorChunk3->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk3);

	DoorChunk4 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk4");
	DoorChunk4->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk4);

	DoorChunk5 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk5");
	DoorChunk5->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk5);

	DoorChunk6 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk6");
	DoorChunk6->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk6);

	DoorChunk7 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk7");
	DoorChunk7->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk7);

	DoorChunk8 = CreateDefaultSubobject<UDestructibleDoorChunkComponent>("DoorChunk8");
	DoorChunk8->SetupAttachment(RootComponent);
	ChunkComponents.Add(DoorChunk8);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube1m(TEXT("StaticMesh'/Game/ThirdParty/Geometry/Meshes/1M_Cube.1M_Cube'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlackUnlitMat(TEXT("Material'/Game/ThirdParty/KingWashLaundromat/Materials/Masters/Pure_black.Pure_black'"));
	
	LightBlocker = CreateDefaultSubobject<UStaticMeshComponent>("Light Blocker");
	LightBlocker->SetStaticMesh(Cube1m.Object);
	LightBlocker->SetMaterial(0, BlackUnlitMat.Object);
	LightBlocker->SetSimulatePhysics(false);
	LightBlocker->BodyInstance.SetMassOverride(0.0f);
	LightBlocker->SetEnableGravity(false);
	LightBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LightBlocker->SetCollisionObjectType(ECC_WorldStatic);
	LightBlocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	LightBlocker->SetGenerateOverlapEvents(false);
	LightBlocker->SetCanEverAffectNavigation(false);
	LightBlocker->SetVisibility(false);
	LightBlocker->SetCastShadow(false);
	LightBlocker->SetHiddenInGame(true);
	LightBlocker->CanCharacterStepUpOn = ECB_No;
	LightBlocker->bApplyImpulseOnDamage = false;
	LightBlocker->bReplicatePhysicsToAutonomousProxy = false;
	LightBlocker->SetComponentTickEnabled(false);
	LightBlocker->PrimaryComponentTick.bStartWithTickEnabled = false;
	LightBlocker->SetupAttachment(RootComponent);
	LightBlocker->SetRelativeLocation(FVector(0.0f, 60.0f, 115.0f));
	LightBlocker->SetRelativeScale3D(FVector(0.107145f, 1.235962f, 2.3f));
	
	DoorHandleFront = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorHandleFront"));
	DoorHandleFront->SetGenerateOverlapEvents(false);
	DoorHandleFront->SetupAttachment(DoorStatic);
	DoorHandleFront->bNavigationRelevant = false;
	DoorHandleFront->SetCanEverAffectNavigation(false);
	DoorHandleFront->bNavigationRelevant = false;
	DoorHandleFront->ComponentTags.AddUnique("NoCover");

	DoorHandleBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorHandleBack"));
	DoorHandleBack->SetGenerateOverlapEvents(false);
	DoorHandleBack->SetupAttachment(DoorStatic);
	DoorHandleBack->bNavigationRelevant = false;
	DoorHandleBack->SetCanEverAffectNavigation(false);
	DoorHandleBack->bNavigationRelevant = false;
	DoorHandleBack->ComponentTags.AddUnique("NoCover");

	Doorway->SetupAttachment(RootComponent);
	Doorway->SetRelativeLocation(FVector(0.0f, 60.0f, 115.0f));

	LockpickArea = CreateDefaultSubobject<USceneComponent>(TEXT("Lockpick Highlight Component"));
	LockpickArea->SetRelativeLocation(FVector(0.0f, 114.0f, 113.0f));
	LockpickArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	LockpickArea->bNavigationRelevant = false;
	LockpickArea->SetCanEverAffectNavigation(false);

	DoorArea = CreateDefaultSubobject<USceneComponent>(TEXT("Door Highlight Component"));
	DoorArea->SetRelativeLocation(FVector(0.0f, 60.0f, 160.0f));
	DoorArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	DoorArea->bNavigationRelevant = false;
	DoorArea->SetCanEverAffectNavigation(false);

	C2Area = CreateDefaultSubobject<USceneComponent>(TEXT("C2 Highlight Component"));
	C2Area->SetRelativeLocation(FVector(0.0f, 114.0f, 170.0f));
	C2Area->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	C2Area->bNavigationRelevant = false;
	C2Area->SetCanEverAffectNavigation(false);

	MirrorgunArea = CreateDefaultSubobject<USceneComponent>(TEXT("Optiwand Area Component"));
	MirrorgunArea->SetRelativeLocation(FVector(0.0f, 60.0f, 15.0f));
	MirrorgunArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	MirrorgunArea->bNavigationRelevant = false;
	MirrorgunArea->SetCanEverAffectNavigation(false);

	FrontMirrorPoint = CreateDefaultSubobject<UMirrorPortalComponent>(TEXT("FrontMirrorPoint"));
	FrontMirrorPoint->SetRelativeLocation(FVector(10.0f, 0.0f, -10.0f));
	FrontMirrorPoint->SetupAttachment(MirrorgunArea, TEXT("DoorIconSocket"));
	FrontMirrorPoint->bNavigationRelevant = false;
	FrontMirrorPoint->SetCanEverAffectNavigation(false);
    FrontMirrorPoint->SetArrowColor(FColor::Orange);

	BackMirrorPoint = CreateDefaultSubobject<UMirrorPortalComponent>(TEXT("BackMirrorPoint"));
	BackMirrorPoint->SetRelativeLocation(FVector(-10.0f, 0.0f, -10.0f));
	BackMirrorPoint->SetRelativeRotation(FRotator(0, -180.0f, 0.0f));
	BackMirrorPoint->SetupAttachment(MirrorgunArea, TEXT("DoorIconSocket"));
	BackMirrorPoint->bNavigationRelevant = false;
	BackMirrorPoint->SetCanEverAffectNavigation(false);
    BackMirrorPoint->SetArrowColor(FColor::Cyan);
	
	BSGArea = CreateDefaultSubobject<USceneComponent>(TEXT("BSG Highlight Component"));
	BSGArea->SetRelativeLocation(FVector(0.0f, 3.0f, 180.0f));
	BSGArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	BSGArea->bNavigationRelevant = false;
	BSGArea->SetCanEverAffectNavigation(false);

	WedgeArea = CreateDefaultSubobject<USceneComponent>(TEXT("Doorjam Highlight Component"));
	WedgeArea->SetRelativeLocation(FVector(0.0f, 110.0f, 15.0f));
	WedgeArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	WedgeArea->bNavigationRelevant = false;
	WedgeArea->SetCanEverAffectNavigation(false);

	BatteringRamArea = CreateDefaultSubobject<USceneComponent>(TEXT("Ram Area Component"));
	BatteringRamArea->SetRelativeLocation(FVector(0.0f, 60.0f, 115.0f));
	BatteringRamArea->SetupAttachment(DoorStatic, TEXT("DoorIconSocket"));
	BatteringRamArea->bNavigationRelevant = false;
	BatteringRamArea->SetCanEverAffectNavigation(false);

	FMODAudioPropagationComp = CreateDefaultSubobject<UFMODAudioPropagationComponent>(TEXT("FMODAudioPropagationComp"));
	FMODAudioPropagationComp->SetRelativeLocation(FVector(0.0f, 60.0f, 130.0f));
	FMODAudioPropagationComp->AttenuationDetails.bOverrideAttenuation = true;
	FMODAudioPropagationComp->AttenuationDetails.MinimumDistance = 2.0f;
	FMODAudioPropagationComp->AttenuationDetails.MaximumDistance = 6.0f;
	FMODAudioPropagationComp->OcclusionDetails.bEnableOcclusion = true;
	FMODAudioPropagationComp->OcclusionDetails.OcclusionTraceChannel = ECC_Visibility;
	FMODAudioPropagationComp->SetupAttachment(RootComponent);

	// Interactable components setup
	DoorOpenInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Open Interactable Comp"));
	DoorOpenInteractableComp->bDistanceFadeIcon = false;
	DoorOpenInteractableComp->ActionSlot1.bUseCustomActionText = true;
	DoorOpenInteractableComp->ActionSlot1.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "DoorClosed");
	DoorOpenInteractableComp->ActionSlot1.bCheckForDisallowedItems = false;
	DoorOpenInteractableComp->ActionSlot2.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "OpenDoor"));
	DoorOpenInteractableComp->AnimatedIconName = "Open Door";
	DoorOpenInteractableComp->SetInteractionIconSize(48.0f, 48.0f);
	DoorOpenInteractableComp->SetupAttachment(DoorArea);

	DoorPushInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Push Interactable Comp"));
	DoorPushInteractableComp->bDistanceFadeIcon = false;
	DoorPushInteractableComp->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PeekDoor"));
	DoorPushInteractableComp->ActionSlot1.bCheckForDisallowedItems = false;
	DoorPushInteractableComp->ActionSlot2.Init("Use", IE_Repeat, FText::FromStringTable("ActionPromptTable", "CheckDoorLock"));
	DoorPushInteractableComp->ActionSlot2.bCheckForDisallowedItems = false;
	DoorPushInteractableComp->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "DoorClosed");
	DoorPushInteractableComp->ActionSlot3.bCheckForDisallowedItems = false;
	DoorPushInteractableComp->AnimatedIconName = "Peek Door";
	DoorPushInteractableComp->SetRelativeLocation(FVector(0.0f, 54.0f, -60.0f));
	DoorPushInteractableComp->SetRelativeLocation(FVector(0.0f, 54.0f, -60.0f));
	DoorPushInteractableComp->SetInteractionIconSize(48.0f, 48.0f);
	DoorPushInteractableComp->SetupAttachment(DoorArea);

	DoorSublinkOpenInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Sublink Open Interactable Comp"));
	DoorSublinkOpenInteractableComp->bDistanceFadeIcon = false;
	DoorSublinkOpenInteractableComp->ActionSlot1.bUseCustomActionText = true;
	DoorSublinkOpenInteractableComp->ActionSlot1.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "DoorLocked");
	DoorSublinkOpenInteractableComp->ActionSlot2.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "OpenBothDoors"));
	DoorSublinkOpenInteractableComp->AnimatedIconName = "Open Both Doors";
	DoorSublinkOpenInteractableComp->SetRelativeLocation(FVector(0.0f, 60.0f, -10.0f));
	DoorSublinkOpenInteractableComp->SetupAttachment(DoorArea);

	DoorSublinkPushInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Sublink Push Interactable Comp"));
	DoorSublinkPushInteractableComp->bDistanceFadeIcon = false;
	DoorSublinkPushInteractableComp->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PeekBothDoors"));
	DoorSublinkPushInteractableComp->ActionSlot1.bCheckForDisallowedItems = false;
	DoorSublinkPushInteractableComp->AnimatedIconName = "Peek Both Doors";
	DoorSublinkPushInteractableComp->SetRelativeLocation(FVector(0.0f, 60.0f, -35.0f));
	DoorSublinkPushInteractableComp->SetupAttachment(DoorArea);

	DoorKickInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Kick Interactable Comp"));
	DoorKickInteractableComp->bDistanceFadeIcon = false;
	DoorKickInteractableComp->MinShowPromptAtDistance = 100.0f;
	DoorKickInteractableComp->ShowPromptAtDistance = 150.0f;
	DoorKickInteractableComp->InteractCircleSize = 36.0f;
	DoorKickInteractableComp->InteractIconSize = 42.0f;
	DoorKickInteractableComp->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "KickDoor"));
	DoorKickInteractableComp->ActionSlot2.bUseCustomActionText = true;
	DoorKickInteractableComp->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "CannotKickWith");
	DoorKickInteractableComp->AnimatedIconName = "Kick Door";
	DoorKickInteractableComp->bHideUponInteraction = true;
	DoorKickInteractableComp->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
	DoorKickInteractableComp->SetupAttachment(DoorArea);
	
	DoorSublinkKickInteractableComp = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Sublink Kick Interactable Comp"));
	DoorSublinkKickInteractableComp->bDistanceFadeIcon = false;
	DoorSublinkKickInteractableComp->MinShowPromptAtDistance = 100.0f;
	DoorSublinkKickInteractableComp->ShowPromptAtDistance = 150.0f;
	DoorSublinkKickInteractableComp->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "KickBothDoors"));
	DoorSublinkKickInteractableComp->ActionSlot2.bUseCustomActionText = true;
	DoorSublinkKickInteractableComp->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "CannotKickWith");
	DoorSublinkKickInteractableComp->AnimatedIconName = "Kick Both Doors";
	DoorSublinkKickInteractableComp->bHideUponInteraction = true;
	DoorSublinkKickInteractableComp->SetRelativeLocation(FVector(0.0f, 60.0f, -80.0f));
	DoorSublinkKickInteractableComp->SetupAttachment(DoorArea);
	
	LockpickInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Lockpick Interactable Component"));
	LockpickInteractableComponent->bDistanceFadeIcon = false;
	LockpickInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipMultitool"));
	LockpickInteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	LockpickInteractableComponent->ActionSlot2.Init("Fire", IE_Repeat, FText::FromStringTable("ActionPromptTable", "PickLock"));
	LockpickInteractableComponent->ActionSlot3.bUseCustomActionText = true;
	LockpickInteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "Unlocking");
	LockpickInteractableComponent->ActionSlot3.bAnimate = true;
	LockpickInteractableComponent->ActionSlot3.bLoopAnimation = true;
	LockpickInteractableComponent->AnimatedIconName = "Lockpick Door";
	LockpickInteractableComponent->ShowPromptAtDistance = 160.0f;
	LockpickInteractableComponent->SetInteractionIconSize(36.0f, 44.0f);
	LockpickInteractableComponent->SetupAttachment(LockpickArea);	

	C2InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("C2 Interactable Component"));
	C2InteractableComponent->bDistanceFadeIcon = false;
	C2InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	C2InteractableComponent->ShowPromptAtDistance = 160.0f;
	C2InteractableComponent->RequiredLookAtPercentage = 0.98f;
	C2InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipC2"));
	C2InteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	C2InteractableComponent->ActionSlot2.Init("Fire", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PlantC2"));
	C2InteractableComponent->AnimatedIconName = "C2 Door";
	C2InteractableComponent->SetupAttachment(C2Area);

	WedgeInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Wedge Interactable Component"));
	WedgeInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
	WedgeInteractableComponent->bDistanceFadeIcon = false;
	WedgeInteractableComponent->bImprintIconOnHUDUponInteraction = true;
	WedgeInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipWedge"));
	WedgeInteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	WedgeInteractableComponent->ActionSlot2.Init("Fire", IE_Pressed, FText::FromStringTable("ActionPromptTable", "DeployWedge"));
	WedgeInteractableComponent->AnimatedIconName = "Wedge Door";
	WedgeInteractableComponent->ShowPromptAtDistance = 200.0f;
	WedgeInteractableComponent->SetupAttachment(WedgeArea);

	OptiwandInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Optiwand Interactable Component"));
	OptiwandInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
	OptiwandInteractableComponent->bDistanceFadeIcon = false;
	OptiwandInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipOptiwand"));
	OptiwandInteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	OptiwandInteractableComponent->ActionSlot2.Init("Fire", IE_Repeat, FText::FromStringTable("ActionPromptTable", "MirrorUnderDoor"));
	OptiwandInteractableComponent->ActionSlot3.bUseCustomActionText = true;
	OptiwandInteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "MirrorBlocked");
	OptiwandInteractableComponent->AnimatedIconName = "Mirror Under Door";
	OptiwandInteractableComponent->bDistanceFadeIcon = false;
	OptiwandInteractableComponent->ShowPromptAtDistance = 200.0f;
	OptiwandInteractableComponent->SetInteractionIconSize(36.0f, 44.0f);
	OptiwandInteractableComponent->SetupAttachment(MirrorgunArea);

	BSGInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("BSG Interactable Component"));
	BSGInteractableComponent->bDistanceFadeIcon = false;
	BSGInteractableComponent->ShowPromptAtDistance = 110.0f;
	BSGInteractableComponent->bImprintIconOnHUDUponInteraction = true;
	BSGInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipBreachingShotgun"));
	BSGInteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	BSGInteractableComponent->AnimatedIconName = "Breach Door";
	BSGInteractableComponent->ShowPromptAtDistance = 200.0f;
	BSGInteractableComponent->SetupAttachment(BSGArea);

	BSGInteractableComponent_2 = CreateDefaultSubobject<UInteractableComponent>(TEXT("BSG Interactable Component 2"));
	BSGInteractableComponent_2->bDistanceFadeIcon = false;
	BSGInteractableComponent_2->ShowPromptAtDistance = 110.0f;
	BSGInteractableComponent_2->bImprintIconOnHUDUponInteraction = true;
	BSGInteractableComponent_2->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipBreachingShotgun"));
	BSGInteractableComponent_2->ActionSlot1.bCheckForDisallowedItems = false;
	BSGInteractableComponent_2->AnimatedIconName = "Breach Door";
	BSGInteractableComponent_2->ShowPromptAtDistance = 200.0f;
	BSGInteractableComponent_2->SetRelativeLocation(FVector(0.0f, 0.0f, -120.0f));
	BSGInteractableComponent_2->SetupAttachment(BSGArea);

	DoorRamInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Door Ram Interactable Component"));
	DoorRamInteractableComponent->bDistanceFadeIcon = false;
	DoorRamInteractableComponent->bImprintIconOnHUDUponInteraction = true;
	DoorRamInteractableComponent->MinShowPromptAtDistance = 0.0f;
	DoorRamInteractableComponent->ShowPromptAtDistance = 135.0f;
	DoorRamInteractableComponent->RequiredLookAtPercentage = 0.98f;
	DoorRamInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipBatteringRam"));
	DoorRamInteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	DoorRamInteractableComponent->AnimatedIconName = "Ram Door";
	DoorRamInteractableComponent->SetRelativeLocation(FVector(0.0f, 30.0f, -35.0f));
	DoorRamInteractableComponent->SetupAttachment(BatteringRamArea);

	DoorBlockerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Door Blocker Component"));
	DoorBlockerComponent->SetRelativeLocation(FVector(0.0f, 60.0f, 10.0f));
	DoorBlockerComponent->SetRelativeScale3D(FVector(0.15f, 2.0f, 1.0f));
	DoorBlockerComponent->SetBoxExtent(FVector(32.0f, 50.0f, 64.0f));
	DoorBlockerComponent->SetCollisionProfileName("Volume");
	DoorBlockerComponent->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
	DoorBlockerComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	DoorBlockerComponent->bNavigationRelevant = true;
	DoorBlockerComponent->bDynamicObstacle = true;
	DoorBlockerComponent->AreaClass = UNavArea_Null::StaticClass();
	DoorBlockerComponent->SetVisibility(false);
	DoorBlockerComponent->SetHiddenInGame(true);
	DoorBlockerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorBlockerComponent->SetCanEverAffectNavigation(true);
	DoorBlockerComponent->SetupAttachment(RootComponent);

	const auto SetupBreachBlocker = [&](UBoxComponent*& InComponent, FName ComponentName)
	{
		InComponent = CreateDefaultSubobject<UBoxComponent>(ComponentName);
		InComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		InComponent->SetCollisionProfileName("Volume");
		InComponent->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
		InComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		InComponent->bNavigationRelevant = true;
		InComponent->bDynamicObstacle = true;
		InComponent->AreaClass = UNavArea_Null::StaticClass();
		InComponent->SetVisibility(false);
		InComponent->SetHiddenInGame(true);
		InComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InComponent->SetCanEverAffectNavigation(false);
		InComponent->SetupAttachment(RootComponent);
	};

	SetupBreachBlocker(BreachBlocker1Component, "Breach Blocker 1 Component");
	SetupBreachBlocker(BreachBlocker2Component, "Breach Blocker 2 Component");
	SetupBreachBlocker(BreachBlocker3Component, "Breach Blocker 3 Component");

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> M_Scorch(TEXT("Material'/Game/ThirdParty/BallisticsVFX/Decals/Decals_C2Scorch_Door.Decals_C2Scorch_Door'"));
	C2ExplosionDecal = M_Scorch.Object;

	#if WITH_EDITORONLY_DATA
	CreateEditorComponents();
	#endif
	
	TypeOfDoor.DataTable = UBpGameplayHelperLib::GetDoorLookupDataTable();
	TypeOfTrap.DataTable = UBpGameplayHelperLib::GetTrapLookupDataTable();
	
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_PushCurve(TEXT("CurveFloat'/Game/Curves/C_DoorPush.C_DoorPush'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_OpenCurve(TEXT("CurveFloat'/Game/Curves/C_DoorOpen.C_DoorOpen'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_CloseCurve(TEXT("CurveFloat'/Game/Curves/C_DoorClose.C_DoorClose'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_RamCurve(TEXT("CurveFloat'/Game/Curves/C_DoorRam.C_DoorRam'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_LockedCurve(TEXT("CurveFloat'/Game/Curves/C_DoorLocked.C_DoorLocked'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_ExplodeCurve(TEXT("CurveFloat'/Game/Curves/C_DoorExplode.C_DoorExplode'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_BreachCurve(TEXT("CurveFloat'/Game/Curves/C_DoorBreach.C_DoorBreach'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_HandleOpenCurve(TEXT("CurveFloat'/Game/Curves/C_DoorHandleOpen.C_DoorHandleOpen'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_HandlePushCurve(TEXT("CurveFloat'/Game/Curves/C_DoorHandlePush.C_DoorHandlePush'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_HandleOpenLockedCurve(TEXT("CurveFloat'/Game/Curves/C_DoorHandleOpen_Locked.C_DoorHandleOpen_Locked'"));

	DoorPushCurve = C_PushCurve.Object;
	DoorOpenCurve = C_OpenCurve.Object;
	DoorCloseCurve = C_CloseCurve.Object;
	DoorKickSuccessCurve = C_RamCurve.Object;
	DoorKickFailCurve = C_LockedCurve.Object;
	DoorLockedCurve = C_LockedCurve.Object;
	DoorRamCurve = C_RamCurve.Object;
	DoorExplodeCurve = C_ExplodeCurve.Object;
	DoorBreachCurve = C_BreachCurve.Object;
	DoorHandleOpenCurve = C_HandleOpenCurve.Object;
	DoorHandlePushCurve = C_HandlePushCurve.Object;
	DoorHandleLockedCurve = C_HandleOpenLockedCurve.Object;
	
	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 20.0f;
	NetPriority = 3.0f;
	bHasEverBeenOpenedBySwat = false;
	
	DoorKillDistance.Empty();
	DoorKillDistance.Add(EDoorDamageType::DDT_Blasting, 300.0f);
	DoorKillDistance.Add(EDoorDamageType::DDT_Shotgunning, 100.0f);
	DoorKillDistance.Add(EDoorDamageType::DDT_Ramming, 0.0f);
	DoorKillDistance.Add(EDoorDamageType::DDT_Kicking, 0.0f);

	DoorStunDistance.Empty();
	DoorStunDistance.Add(EDoorDamageType::DDT_Blasting, 500.0f);
	DoorStunDistance.Add(EDoorDamageType::DDT_Shotgunning, 200.0f);
	DoorStunDistance.Add(EDoorDamageType::DDT_Ramming, 100.0f);
	DoorStunDistance.Add(EDoorDamageType::DDT_Kicking, 100.0f);
	
	bReplicates = true;
}

void ADoor::SetupTags()
{
	if (DoorStatic)
		DoorStatic->ComponentTags.AddUnique("NoCover");

	if (DoorHandleFront)
		DoorHandleFront->ComponentTags.AddUnique("NoCover");
	
	if (DoorHandleBack)
		DoorHandleBack->ComponentTags.AddUnique("NoCover");
}

#if WITH_EDITORONLY_DATA
void ADoor::CreateEditorComponents()
{
	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_DoorOpenIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/open_full_1.open_full_1'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_DoorPushIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/open_15.open_15'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_DoorKickIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/kick.kick'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_LockpickIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/lockpick-multitool.lockpick-multitool'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_C2ChargeIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/c2-charge.c2-charge'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_OptiwandIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/optiwand.optiwand'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_WedgeIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/wedge.wedge'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_BSGIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/bsg.bsg'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_BatteringRamIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/battering-ram.battering-ram'"));
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_CenterFedRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/centerfed.centerfed'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_CornerRightFedRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/cornerright.cornerright'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_CornerLeftFedRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/cornerleft.cornerleft'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_HallwayRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/hallway.hallway'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_HallwayLeftRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/hallwayleft.hallwayleft'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_HallwayRightRoomIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/hallwayright.hallwayright'"));

	CenterIcon = T_CenterFedRoomIcon.Object;
	CornerLeftIcon = T_CornerLeftFedRoomIcon.Object;
	CornerRightIcon = T_CornerRightFedRoomIcon.Object;
	HallwayIcon = T_HallwayRoomIcon.Object;
	HallwayLeftIcon = T_HallwayLeftRoomIcon.Object;
	HallwayRightIcon = T_HallwayRightRoomIcon.Object;

	DoorOpenBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Open Billboard Component (Front)"));
	DoorOpenBillboard_Front->SetSprite(T_DoorOpenIcon.Object);
	DoorOpenBillboard_Front->bIsScreenSizeScaled = true;
	DoorOpenBillboard_Front->ScreenSize = 0.004f;
	DoorOpenBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	DoorOpenBillboard_Front->SetWorldScale3D(FVector(0.3f));
	DoorOpenBillboard_Front->bIsEditorOnly = true;
	DoorOpenBillboard_Front->SetupAttachment(DoorOpenInteractableComp);

	DoorPushBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Push Billboard Component (Front)"));
	DoorPushBillboard_Front->SetSprite(T_DoorPushIcon.Object);
	DoorPushBillboard_Front->bIsScreenSizeScaled = true;
	DoorPushBillboard_Front->ScreenSize = 0.004f;
	DoorPushBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	DoorPushBillboard_Front->SetWorldScale3D(FVector(0.3f));
	DoorPushBillboard_Front->bIsEditorOnly = true;
	DoorPushBillboard_Front->SetupAttachment(DoorPushInteractableComp);

	DoorKickBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Kick Billboard Component (Front)"));
	DoorKickBillboard_Front->SetSprite(T_DoorKickIcon.Object);
	DoorKickBillboard_Front->bIsScreenSizeScaled = true;
	DoorKickBillboard_Front->ScreenSize = 0.004f;
	DoorKickBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	DoorKickBillboard_Front->SetWorldScale3D(FVector(0.3f));
	DoorKickBillboard_Front->bIsEditorOnly = true;
	DoorKickBillboard_Front->SetupAttachment(DoorKickInteractableComp);

	LockpickBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Lockpick Billboard Component (Front)"));
	LockpickBillboard_Front->SetSprite(T_LockpickIcon.Object);
	LockpickBillboard_Front->bIsScreenSizeScaled = true;
	LockpickBillboard_Front->ScreenSize = 0.004f;
	LockpickBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 10.0f));
	LockpickBillboard_Front->SetWorldScale3D(FVector(0.3f));
	LockpickBillboard_Front->bIsEditorOnly = true;
	LockpickBillboard_Front->SetupAttachment(LockpickInteractableComponent);
	
	C2Billboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("C2 Billboard Component (Front)"));
	C2Billboard_Front->SetSprite(T_C2ChargeIcon.Object);
	C2Billboard_Front->bIsScreenSizeScaled = true;
	C2Billboard_Front->ScreenSize = 0.004f;
	C2Billboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	C2Billboard_Front->SetWorldScale3D(FVector(0.3f));
	C2Billboard_Front->bIsEditorOnly = true;
	C2Billboard_Front->SetupAttachment(C2InteractableComponent);

	OptiwandBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Optiwand Billboard Component (Front)"));
	OptiwandBillboard_Front->SetSprite(T_OptiwandIcon.Object);
	OptiwandBillboard_Front->bIsScreenSizeScaled = true;
	OptiwandBillboard_Front->ScreenSize = 0.004f;
	OptiwandBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	OptiwandBillboard_Front->SetWorldScale3D(FVector(0.3f));
	OptiwandBillboard_Front->bIsEditorOnly = true;
	OptiwandBillboard_Front->SetupAttachment(OptiwandInteractableComponent);

	WedgeBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Wedge Billboard Component (Front)"));
	WedgeBillboard_Front->SetSprite(T_WedgeIcon.Object);
	WedgeBillboard_Front->bIsScreenSizeScaled = true;
	WedgeBillboard_Front->ScreenSize = 0.004f;
	WedgeBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 5.0f));
	WedgeBillboard_Front->SetWorldScale3D(FVector(0.3f));
	WedgeBillboard_Front->bIsEditorOnly = true;
	WedgeBillboard_Front->SetupAttachment(WedgeInteractableComponent);

	BSGBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("BSG Billboard Component (Front)"));
	BSGBillboard_Front->SetSprite(T_BSGIcon.Object);
	BSGBillboard_Front->bIsScreenSizeScaled = true;
	BSGBillboard_Front->ScreenSize = 0.004f;
	BSGBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	BSGBillboard_Front->SetWorldScale3D(FVector(0.3f));
	BSGBillboard_Front->bIsEditorOnly = true;
	BSGBillboard_Front->SetupAttachment(BSGInteractableComponent);

	BSGBillboard_2_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("BSG Billboard Component 2 (Front)"));
	BSGBillboard_2_Front->SetSprite(T_BSGIcon.Object);
	BSGBillboard_2_Front->bIsScreenSizeScaled = true;
	BSGBillboard_2_Front->ScreenSize = 0.004f;
	BSGBillboard_2_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	BSGBillboard_2_Front->SetWorldScale3D(FVector(0.3f));
	BSGBillboard_2_Front->bIsEditorOnly = true;
	BSGBillboard_2_Front->SetupAttachment(BSGInteractableComponent_2);

	BatteringRamBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Battering Ram Billboard Component (Front)"));
	BatteringRamBillboard_Front->SetSprite(T_BatteringRamIcon.Object);
	BatteringRamBillboard_Front->bIsScreenSizeScaled = true;
	BatteringRamBillboard_Front->ScreenSize = 0.004f;
	BatteringRamBillboard_Front->SetRelativeLocation(FVector(13.0f, 0.0f, 0.0f));
	BatteringRamBillboard_Front->SetWorldScale3D(FVector(0.3f));
	BatteringRamBillboard_Front->bIsEditorOnly = true;
	BatteringRamBillboard_Front->SetupAttachment(DoorRamInteractableComponent);
	
	DoorOpenBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Open Billboard Component (Back)"));
	DoorOpenBillboard_Back->SetSprite(T_DoorOpenIcon.Object);
	DoorOpenBillboard_Back->bIsScreenSizeScaled = true;
	DoorOpenBillboard_Back->ScreenSize = 0.004f;
	DoorOpenBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	DoorOpenBillboard_Back->SetWorldScale3D(FVector(0.3f));
	DoorOpenBillboard_Back->bIsEditorOnly = true;
	DoorOpenBillboard_Back->SetupAttachment(DoorOpenInteractableComp);

	DoorPushBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Push Billboard Component (Back)"));
	DoorPushBillboard_Back->SetSprite(T_DoorPushIcon.Object);
	DoorPushBillboard_Back->bIsScreenSizeScaled = true;
	DoorPushBillboard_Back->ScreenSize = 0.004f;
	DoorPushBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	DoorPushBillboard_Back->SetWorldScale3D(FVector(0.3f));
	DoorPushBillboard_Back->bIsEditorOnly = true;
	DoorPushBillboard_Back->SetupAttachment(DoorPushInteractableComp);

	DoorKickBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Kick Billboard Component (Back)"));
	DoorKickBillboard_Back->SetSprite(T_DoorKickIcon.Object);
	DoorKickBillboard_Back->bIsScreenSizeScaled = true;
	DoorKickBillboard_Back->ScreenSize = 0.004f;
	DoorKickBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	DoorKickBillboard_Back->SetWorldScale3D(FVector(0.3f));
	DoorKickBillboard_Back->bIsEditorOnly = true;
	DoorKickBillboard_Back->SetupAttachment(DoorKickInteractableComp);

	LockpickBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Lockpick Billboard Component (Back)"));
	LockpickBillboard_Back->SetSprite(T_LockpickIcon.Object);
	LockpickBillboard_Back->bIsScreenSizeScaled = true;
	LockpickBillboard_Back->ScreenSize = 0.004f;
	LockpickBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 10.0f));
	LockpickBillboard_Back->SetWorldScale3D(FVector(0.3f));
	LockpickBillboard_Back->bIsEditorOnly = true;
	LockpickBillboard_Back->SetupAttachment(LockpickInteractableComponent);
	
	C2Billboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("C2 Billboard Component (Back)"));
	C2Billboard_Back->SetSprite(T_C2ChargeIcon.Object);
	C2Billboard_Back->bIsScreenSizeScaled = true;
	C2Billboard_Back->ScreenSize = 0.004f;
	C2Billboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	C2Billboard_Back->SetWorldScale3D(FVector(0.3f));
	C2Billboard_Back->bIsEditorOnly = true;
	C2Billboard_Back->SetupAttachment(C2InteractableComponent);

	OptiwandBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Optiwand Billboard Component (Back)"));
	OptiwandBillboard_Back->SetSprite(T_OptiwandIcon.Object);
	OptiwandBillboard_Back->bIsScreenSizeScaled = true;
	OptiwandBillboard_Back->ScreenSize = 0.004f;
	OptiwandBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	OptiwandBillboard_Back->SetWorldScale3D(FVector(0.3f));
	OptiwandBillboard_Back->bIsEditorOnly = true;
	OptiwandBillboard_Back->SetupAttachment(OptiwandInteractableComponent);

	WedgeBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Wedge Billboard Component (Back)"));
	WedgeBillboard_Back->SetSprite(T_WedgeIcon.Object);
	WedgeBillboard_Back->bIsScreenSizeScaled = true;
	WedgeBillboard_Back->ScreenSize = 0.004f;
	WedgeBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 5.0f));
	WedgeBillboard_Back->SetWorldScale3D(FVector(0.3f));
	WedgeBillboard_Back->bIsEditorOnly = true;
	WedgeBillboard_Back->SetupAttachment(WedgeInteractableComponent);

	BSGBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("BSG Billboard Component (Back)"));
	BSGBillboard_Back->SetSprite(T_BSGIcon.Object);
	BSGBillboard_Back->bIsScreenSizeScaled = true;
	BSGBillboard_Back->ScreenSize = 0.004f;
	BSGBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	BSGBillboard_Back->SetWorldScale3D(FVector(0.3f));
	BSGBillboard_Back->bIsEditorOnly = true;
	BSGBillboard_Back->SetupAttachment(BSGInteractableComponent);

	BSGBillboard_2_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("BSG Billboard Component 2 (Back)"));
	BSGBillboard_2_Back->SetSprite(T_BSGIcon.Object);
	BSGBillboard_2_Back->bIsScreenSizeScaled = true;
	BSGBillboard_2_Back->ScreenSize = 0.004f;
	BSGBillboard_2_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	BSGBillboard_2_Back->SetWorldScale3D(FVector(0.3f));
	BSGBillboard_2_Back->bIsEditorOnly = true;
	BSGBillboard_2_Back->SetupAttachment(BSGInteractableComponent_2);

	BatteringRamBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Battering Ram Billboard Component (Back)"));
	BatteringRamBillboard_Back->SetSprite(T_BatteringRamIcon.Object);
	BatteringRamBillboard_Back->bIsScreenSizeScaled = true;
	BatteringRamBillboard_Back->ScreenSize = 0.004f;
	BatteringRamBillboard_Back->SetRelativeLocation(FVector(-13.0f, 0.0f, 0.0f));
	BatteringRamBillboard_Back->SetWorldScale3D(FVector(0.3f));
	BatteringRamBillboard_Back->bIsEditorOnly = true;
	BatteringRamBillboard_Back->SetupAttachment(DoorRamInteractableComponent);

	RoomPositionBillboard_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Room Position Billboard Component (Front)"));
	RoomPositionBillboard_Front->SetSprite(nullptr);
	RoomPositionBillboard_Front->bIsScreenSizeScaled = true;
	RoomPositionBillboard_Front->ScreenSize = 0.004f;
	RoomPositionBillboard_Front->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
	RoomPositionBillboard_Front->SetWorldScale3D(FVector(0.3f));
	RoomPositionBillboard_Front->bIsEditorOnly = true;
	RoomPositionBillboard_Front->SetupAttachment(Doorway);
	
	RoomPositionBillboard_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Room Position Billboard Component (Back)"));
	RoomPositionBillboard_Back->SetSprite(nullptr);
	RoomPositionBillboard_Back->bIsScreenSizeScaled = true;
	RoomPositionBillboard_Back->ScreenSize = 0.004f;
	RoomPositionBillboard_Back->SetRelativeLocation(FVector(-150.0f, 0.0f, 0.0f));
	RoomPositionBillboard_Back->SetWorldScale3D(FVector(0.3f));
	RoomPositionBillboard_Back->bIsEditorOnly = true;
	RoomPositionBillboard_Back->SetupAttachment(Doorway);
	
	RoomPositionBillboard_DoubleDoor_Front = CreateDefaultSubobject<UBillboardComponent>(TEXT("Room Position Double Billboard Component (Front)"));
	RoomPositionBillboard_DoubleDoor_Front->SetSprite(nullptr);
	RoomPositionBillboard_DoubleDoor_Front->bIsScreenSizeScaled = true;
	RoomPositionBillboard_DoubleDoor_Front->ScreenSize = 0.004f;
	RoomPositionBillboard_DoubleDoor_Front->SetRelativeLocation(FVector(150.0f, 60.0f, 30.0f));
	RoomPositionBillboard_DoubleDoor_Front->SetWorldScale3D(FVector(0.3f));
	RoomPositionBillboard_DoubleDoor_Front->bIsEditorOnly = true;
	RoomPositionBillboard_DoubleDoor_Front->SetupAttachment(Doorway);
	
	RoomPositionBillboard_DoubleDoor_Back = CreateDefaultSubobject<UBillboardComponent>(TEXT("Room Position Double Billboard Component (Back)"));
	RoomPositionBillboard_DoubleDoor_Back->SetSprite(nullptr);
	RoomPositionBillboard_DoubleDoor_Back->bIsScreenSizeScaled = true;
	RoomPositionBillboard_DoubleDoor_Back->ScreenSize = 0.004f;
	RoomPositionBillboard_DoubleDoor_Back->SetRelativeLocation(FVector(-150.0f, 60.0f, 30.0f));
	RoomPositionBillboard_DoubleDoor_Back->SetWorldScale3D(FVector(0.3f));
	RoomPositionBillboard_DoubleDoor_Back->bIsEditorOnly = true;
	RoomPositionBillboard_DoubleDoor_Back->SetupAttachment(Doorway);
	#endif
}

void ADoor::DestroyEditorComponents()
{
	#if WITH_EDITORONLY_DATA
	DESTROY_COMPONENT(WedgeBillboard_Back)
	DESTROY_COMPONENT(WedgeBillboard_Front)
	
	DESTROY_COMPONENT(OptiwandBillboard_Back)
	DESTROY_COMPONENT(OptiwandBillboard_Front)
	
	DESTROY_COMPONENT(C2Billboard_Back)
	DESTROY_COMPONENT(C2Billboard_Front)
	
	DESTROY_COMPONENT(LockpickBillboard_Back)
	DESTROY_COMPONENT(LockpickBillboard_Front)
	
	DESTROY_COMPONENT(DoorKickBillboard_Back)
	DESTROY_COMPONENT(DoorKickBillboard_Front)
	
	DESTROY_COMPONENT(DoorPushBillboard_Back)
	DESTROY_COMPONENT(DoorPushBillboard_Front)
	
	DESTROY_COMPONENT(DoorOpenBillboard_Back)
	DESTROY_COMPONENT(DoorOpenBillboard_Front)
	
	DESTROY_COMPONENT(BSGBillboard_Front)
	DESTROY_COMPONENT(BSGBillboard_2_Front)
	
	DESTROY_COMPONENT(BSGBillboard_Back)
	DESTROY_COMPONENT(BSGBillboard_2_Back)

	DESTROY_COMPONENT(BatteringRamBillboard_Front)
	DESTROY_COMPONENT(BatteringRamBillboard_Back)

	DESTROY_COMPONENT(RoomPositionBillboard_Front)
	DESTROY_COMPONENT(RoomPositionBillboard_Back)
	
	DESTROY_COMPONENT(RoomPositionBillboard_DoubleDoor_Front)
	DESTROY_COMPONENT(RoomPositionBillboard_DoubleDoor_Back)
	#endif
}

void ADoor::ShowEditorComponents()
{
	#if WITH_EDITORONLY_DATA
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		WedgeBillboard_Back->SetVisibility(true);
		WedgeBillboard_Front->SetVisibility(true);
	
		OptiwandBillboard_Back->SetVisibility(true);
		OptiwandBillboard_Front->SetVisibility(true);
	
		C2Billboard_Back->SetVisibility(true);
		C2Billboard_Front->SetVisibility(true);
	
		LockpickBillboard_Back->SetVisibility(true);
		LockpickBillboard_Front->SetVisibility(true);
	
		DoorKickBillboard_Back->SetVisibility(true);
		DoorKickBillboard_Front->SetVisibility(true);
	
		DoorPushBillboard_Back->SetVisibility(true);
		DoorPushBillboard_Front->SetVisibility(true);
	
		DoorOpenBillboard_Back->SetVisibility(true);
		DoorOpenBillboard_Front->SetVisibility(true);
	
		BSGBillboard_Front->SetVisibility(true);
		BSGBillboard_2_Front->SetVisibility(true);
	
		BSGBillboard_Back->SetVisibility(true);
		BSGBillboard_2_Back->SetVisibility(true);
	
		BatteringRamBillboard_Back->SetVisibility(true);
		BatteringRamBillboard_Front->SetVisibility(true);

		RoomPositionBillboard_Back->SetVisibility(true);
		RoomPositionBillboard_Front->SetVisibility(true);
		
		RoomPositionBillboard_DoubleDoor_Back->SetVisibility(true);
		RoomPositionBillboard_DoubleDoor_Front->SetVisibility(true);
	}
	#endif
}

void ADoor::HideEditorComponents()
{
	#if WITH_EDITORONLY_DATA
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		WedgeBillboard_Back->SetVisibility(false);
		WedgeBillboard_Front->SetVisibility(false);
	
		OptiwandBillboard_Back->SetVisibility(false);
		OptiwandBillboard_Front->SetVisibility(false);
	
		C2Billboard_Back->SetVisibility(false);
		C2Billboard_Front->SetVisibility(false);
	
		LockpickBillboard_Back->SetVisibility(false);
		LockpickBillboard_Front->SetVisibility(false);
	
		DoorKickBillboard_Back->SetVisibility(false);
		DoorKickBillboard_Front->SetVisibility(false);
	
		DoorPushBillboard_Back->SetVisibility(false);
		DoorPushBillboard_Front->SetVisibility(false);
	
		DoorOpenBillboard_Back->SetVisibility(false);
		DoorOpenBillboard_Front->SetVisibility(false);
	
		BSGBillboard_Front->SetVisibility(false);
		BSGBillboard_2_Front->SetVisibility(false);
	
		BSGBillboard_Back->SetVisibility(false);
		BSGBillboard_2_Back->SetVisibility(false);
	
		BatteringRamBillboard_Back->SetVisibility(false);
		BatteringRamBillboard_Front->SetVisibility(false);
		
		RoomPositionBillboard_Back->SetVisibility(false);
		RoomPositionBillboard_Front->SetVisibility(false);
		
		RoomPositionBillboard_DoubleDoor_Back->SetVisibility(false);
		RoomPositionBillboard_DoubleDoor_Front->SetVisibility(false);
	}
	#endif
}
#endif

#if WITH_EDITOR
void ADoor::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	
	if (PropName == FName("RowName"))
	{
		Setup();
	}
	else if (PropName == FName("TrapSetup") || PropName == FName("TrapSide") || PropName == FName("bNoSpawnTrap"))
	{
		SetupTrap();
	}
	else if (PropName == FName("FrontRoomPosition"))
	{
		SetFrontRoomPosition(FrontRoomPosition);
	}
	else if (PropName == FName("BackRoomPosition"))
	{
		SetBackRoomPosition(BackRoomPosition);
	}

	bIsDoorway = IsDoorwayOnly();
	if (!bIsDoorway)
	{
		bNoAutomaticClearing = false;
	}
}

void ADoor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	TypeOfDoor.DataTable = UBpGameplayHelperLib::GetDoorLookupDataTable();
	TypeOfTrap.DataTable = UBpGameplayHelperLib::GetTrapLookupDataTable();
	if (TypeOfDoor.RowName == NAME_None)
	{
		TypeOfDoor.RowName = "Doorway";
		SetupDoor();
	}
}
#endif

void ADoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoor, DoorData);
	DOREPLIFETIME(ADoor, TrapData);
	DOREPLIFETIME(ADoor, TypeOfDoor);
	
	DOREPLIFETIME(ADoor, bClientReset);
	DOREPLIFETIME(ADoor, bLocked);
	DOREPLIFETIME(ADoor, bDoorBroken);
	DOREPLIFETIME(ADoor, bDoorJammed);
	DOREPLIFETIME(ADoor, bDoorHandlesBroken);
	
	DOREPLIFETIME(ADoor, bSWATKnowsLockState);
	DOREPLIFETIME(ADoor, bSWATKnowsTrapState);
	DOREPLIFETIME(ADoor, bSuspectKnowsLockState);
	DOREPLIFETIME(ADoor, bSuspectKnowsTrapState);
	
	DOREPLIFETIME(ADoor, bC2Placed);
	
	DOREPLIFETIME(ADoor, OpenCloseAmount);
	DOREPLIFETIME(ADoor, DoorHandlePitchAmount);
	
	DOREPLIFETIME(ADoor, AttachedTrap);
	DOREPLIFETIME(ADoor, bTrapInFront);

	DOREPLIFETIME(ADoor, AttachedWedge);

	DOREPLIFETIME(ADoor, ChunkComponents);
	DOREPLIFETIME(ADoor, DestroyedChunkIdxs);
	
	DOREPLIFETIME(ADoor, LastDoorDamage);
	DOREPLIFETIME(ADoor, LastDoorUser);
}

void ADoor::BeginPlay()
{
	Super::BeginPlay();

	Init();

	if (!IsDoorwayOnly())
	{
		DoorStatic->SetSimulatePhysics(false);
	
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;

		TimeSinceLastActionPromptUpdate = 0.1f;

		DoorStatic->SetCanEverAffectNavigation(true);
		
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorOpenClose, {DoorOpenCurve, DoorCloseCurve}, false, 1.0f, "Tick_DoorOpenClose");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorPush, DoorPushCurve, false, 1.0f, "Tick_DoorPush");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorHandleOpen, DoorHandleOpenCurve, false, 1.0f, "Tick_DoorHandle_Open");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorHandlePush, DoorHandlePushCurve, false, 1.0f, "Tick_DoorHandle_Push");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorHandleLocked, DoorHandleLockedCurve, false, 1.0f, "Tick_DoorHandleLocked");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorLocked, DoorLockedCurve, false, 1.0f, "Tick_DoorLocked");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorRam, DoorRamCurve, false, 1.0f, "Tick_DoorRam", "Finished_DoorRam");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorBreach, DoorBreachCurve, false, 1.0f, "Tick_DoorBreach");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorExplode, DoorExplodeCurve, false, 1.0f, "Tick_DoorExplode", "Finished_DoorExplode");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorKickSuccess, DoorKickSuccessCurve, false, 1.0f, "Tick_DoorKick_Success", "Finished_DoorKick_Success");
		UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_DoorKickFail, DoorKickFailCurve, false, 1.0f, "Tick_DoorKick_Fail");
	}

	{
		// Disable all nav blocking thigns!!
		TInlineComponentArray<UActorComponent*> Components;
		GetComponents(Components);
		for (UActorComponent* Comp : Components)
		{
			if (IsDoorwayOnly())
			{
				if (Comp != DoorBlockerComponent)
				{
					Comp->bNavigationRelevant = false;
					Comp->SetCanEverAffectNavigation(false);
				}
			}
			else
			{
				Comp->bNavigationRelevant = Comp == DoorStatic || ChunkComponents.Contains(Comp);
				Comp->SetCanEverAffectNavigation(Comp == DoorStatic || ChunkComponents.Contains(Comp));
			}

			if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				NavSys->UpdateActorInNavOctree(*this);
				NavSys->UpdateComponentInNavOctree(*Comp);
			}
		}
	}

	if (IsDoorwayOnly())
	{
		//SetActorTickEnabled(false);
		SetActorTickInterval(1.0f);

		DisableAllInteractables();
	}

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllDoors.AddUnique(this);
		
		if (GS->bInPlanningMenu)
		{
			DisableAllInteractables();
		}
	}
	
	if (!IsDoorwayOnly())
	{
		C2ExplosionDecalComponent = UReadyOrNotFunctionLibrary::CreateDecalComponent(this, C2ExplosionDecal, FVector(8.0f, 96.0f, 96.0f));
		if (C2ExplosionDecalComponent)
		{
			C2ExplosionDecalComponent->bDestroyOwnerAfterFade = false;
		}
	}

	// update clear point stage properties
	{
		const auto UpdateStage = [](TArray<FClearPoint>& InClearPoints)
		{
			for (uint8 i = 0; i < InClearPoints.Num(); i++)
			{
				InClearPoints[i].Stage = i;
			}
		};

		UpdateStage(FrontLeftClearPoints);
		UpdateStage(FrontRightClearPoints);
		UpdateStage(BackLeftClearPoints);
		UpdateStage(BackRightClearPoints);
	}
	
	#if WITH_EDITORONLY_DATA
	DestroyEditorComponents();
	#endif

	OperatingStates.Empty();

	EnableNavLink();
}

void ADoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllDoors.Remove(this);
	}
}

void ADoor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
	#endif

	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		#if WITH_EDITOR
		if (DEBUG_SUBSYSTEM->bDrawDoorKillStunDistances)
			DrawKillStunDistances(DeltaTime);
		#endif
		
		if (DEBUG_SUBSYSTEM->bForceOpenAllDoors)
		{
			OpenDoor_Debug();

			if (IsFullyOpen())
				DEBUG_SUBSYSTEM->bForceOpenAllDoors = false;
		}
		else if (DEBUG_SUBSYSTEM->bForceCloseAllDoors)
		{
			CloseDoor_Debug();
			
			if (IsClosed())
            	DEBUG_SUBSYSTEM->bForceCloseAllDoors = false;
		}
	}
	#endif

	SCOPE_CYCLE_COUNTER(STAT_DoorTick);

	// failsafe deactivate breach blockers after 5 secs
	if (TimeSinceBreachBlockersActivated != 86400.0f)
	{
		TimeSinceBreachBlockersActivated += DeltaTime;
		if (TimeSinceBreachBlockersActivated > 5.0f)
		{
			DeactivateBreachBlockers();
		}
	}
	
	CurrentStackUpActivities.RemoveAll([this](UTeamStackUpActivity* Activity)
	{
		return (Activity->IsActivityComplete() ||
				(Activity->GetOwningController() && Activity->GetOwningController()->GetActivity<UTeamStackUpActivity>() != Activity)/* ||
				Activity->GetStackUpDoor() != this*/);
	});
	
	// failsafe deactivate door blockers
	if (CurrentStackUpActivities.Num() == 0 && CurrentActivities.Num() == 0 && !IsTrapLive())
	{
		if (DoorBlockerComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision ||
			DoorBlockerComponent->CanEverAffectNavigation())
		{
			DeactivateBreachBlockers();
			DeactivateDoorBlocker();
		}
	}
	else
	{
		TimeWithoutStackUpActivity = 0.0f;
	}
	
	// Trap trigger conditions
	if (IsTrapLive())
	{
		ActivateDoorBlocker_Trap();
		
		if (bTrapInFront)
		{
			if ((IsOpen_Backward() && IsOpenBeyond_Angle(IncrementAngle) && !AllBottomDoorChunksBroken()) || (DriveSubDoor && DriveSubDoor->IsOpen_Forward() && DriveSubDoor->IsOpenBeyond_Angle(IncrementAngle) && !DriveSubDoor->AllBottomDoorChunksBroken()))
			{
				AttachedTrap->Server_OnTrapTriggered(LastDoorUser);
			}
		}
		else
		{
			if ((IsOpen_Forward() && IsOpenBeyond_Angle(IncrementAngle) && !AllBottomDoorChunksBroken()) || (DriveSubDoor && DriveSubDoor->IsOpen_Backward() && DriveSubDoor->IsOpenBeyond_Angle(IncrementAngle) && !DriveSubDoor->AllBottomDoorChunksBroken()))
			{
				AttachedTrap->Server_OnTrapTriggered(LastDoorUser);
			}
		}
	}

	if (IsDoorwayOnly())
	{
		return;
	}
	
	{
		SCOPE_CYCLE_COUNTER(STAT_AttachedWedgeLocationUpdate);
		
		if (AttachedWedge)
		{
			AttachedWedge->SetActorLocation(WedgeInteractableComponent->GetComponentLocation() - FVector::UpVector * 5.0f);
		}
	}

	TimeUntilNextPathTest = FMath::Max(TimeUntilNextPathTest - DeltaTime, 0.0f);

	{
		SCOPE_CYCLE_COUNTER(STAT_TimelineTicks);
		
		TL_DoorOpenClose.TickTimeline(DeltaTime);
		TL_DoorPush.TickTimeline(DeltaTime);
		TL_DoorLocked.TickTimeline(DeltaTime);
		TL_DoorRam.TickTimeline(DeltaTime);
		TL_DoorBreach.TickTimeline(DeltaTime);
		TL_DoorExplode.TickTimeline(DeltaTime);
		TL_DoorHandleOpen.TickTimeline(DeltaTime);
		TL_DoorHandlePush.TickTimeline(DeltaTime);
		TL_DoorHandleLocked.TickTimeline(DeltaTime);
		TL_DoorKickSuccess.TickTimeline(DeltaTime);
		TL_DoorKickFail.TickTimeline(DeltaTime);
	}

	// whenever i delete this, some shit just doesnt work when in multiplayer ¯\_(ツ)_/
	if (CreationTime > 1.0f)
	{
		bClientReset = false;
	}

	TimeSinceLastBlocked += DeltaTime;

	// Is door mesh attached to us?
	//if (IsDestructible() && !AllMajorDoorChunksDestroyed() || (!IsDestructible() && DoorStatic->GetAttachParent() == RootComponent))
	if (DoorStatic->GetAttachParent() == RootComponent)
	{
		if (AnyHingesLeft())
		{
			if (IsOpen() && !bHasOpenDoorEventBroadcasted)
			{
				OnDoorOpened.Broadcast();
				bHasOpenDoorEventBroadcasted = true;
				bHasCloseDoorEventBroadcasted = false;
			}

			if (DriveSubDoor)
			{
				if (DriveSubDoor->IsOpen() && !DriveSubDoor->bHasOpenDoorEventBroadcasted)
				{
					OnSubDoorOpened.Broadcast();
				}
			}

			if (IsOpenBy_Angle(4.0f) && !bHasCloseDoorEventBroadcasted && bHasOpenDoorEventBroadcasted)
			{
				OnDoorClosed.Broadcast();
				bHasCloseDoorEventBroadcasted = true;
				bHasOpenDoorEventBroadcasted = false;

				if (FMath::Abs(LastStartAngle) >= MaxOpenClose/2.0f)
					Multicast_PlayDoorSound(EDoorInteraction::Close, nullptr, {MakeOcclusionParam()});
			}
		}
		
		CheckDoorHit();
		
		//DrawDebugDirectionalArrow(GetWorld(), Doorway->GetComponentLocation(), Doorway->GetComponentLocation() + GetActorForwardVector() * 200.0f, 200.0f, FColor::Red, false, 0.15f, 0, 2.0f);
		//DrawDebugDirectionalArrow(GetWorld(), Doorway->GetComponentLocation(), Doorway->GetComponentLocation() + GetActorRightVector() * 200.0f, 200.0f, FColor::Green, false, 0.15f, 0, 2.0f);
		//DrawDebugDirectionalArrow(GetWorld(), Doorway->GetComponentLocation(), Doorway->GetComponentLocation() + GetDoorMesh()->GetForwardVector() * 200.0f, 200.0f, FColor::Green, false, 0.15f, 0, 2.0f);

		//DrawDebugLine(GetWorld(), GetDoorway()->GetComponentLocation(), GetDoorway()->GetComponentLocation() + GetActorRightVector().RotateAngleAxis(GetOpenAmount(), FVector::UpVector) * 100.0f, FColor::Red, false, DeltaTime + 0.02f, 0, 1.5f);
		
		// Door rotation
		{
			SCOPE_CYCLE_COUNTER(STAT_UpdateDoorRotation);
			
			const FRotator DoorRelativeRotation = DoorStatic->GetRelativeRotation();
			
			// hack to make sure we're not pushing changes to the navmesh every micron
			if (FMath::Abs(DoorRelativeRotation.Yaw - OpenCloseAmount) > 0.01f)
			{
				const FRotator SmoothedDoorRotation = FMath::RInterpTo(DoorRelativeRotation, FRotator(DoorRelativeRotation.Pitch, OpenCloseAmount, DoorRelativeRotation.Roll), DeltaTime, 15.0f);
				DoorStatic->SetRelativeRotation(SmoothedDoorRotation);

				// Update all door chunk rotation to match the door's current rotation
				for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
				{
					if (Chunk && !Chunk->IsSimulatingPhysics())
					{
						Chunk->SetRelativeRotation(SmoothedDoorRotation);
					}
				}
			}

			if (DoorHandleFront && !DoorHandleFront->IsSimulatingPhysics())
			{
				const FRotator DoorFrontHandleRelativeRotation = DoorHandleFront->GetRelativeRotation();
				const FRotator SmoothedHandleRotation = FMath::RInterpTo(DoorFrontHandleRelativeRotation, FRotator(DoorHandlePitchAmount, DoorFrontHandleRelativeRotation.Yaw, DoorFrontHandleRelativeRotation.Roll), DeltaTime, 15.0f);

				DoorHandleFront->SetRelativeRotation(SmoothedHandleRotation);
			}

			if (DoorHandleBack && !DoorHandleBack->IsSimulatingPhysics())
			{
				const FRotator DoorBackHandleRelativeRotation = DoorHandleBack->GetRelativeRotation();
				const FRotator SmoothedHandleRotation = FMath::RInterpTo(DoorBackHandleRelativeRotation, FRotator(DoorHandlePitchAmount, DoorBackHandleRelativeRotation.Yaw, DoorBackHandleRelativeRotation.Roll), DeltaTime, 15.0f);

				DoorHandleBack->SetRelativeRotation(SmoothedHandleRotation);
			}
		}
	}

	// Failsafe to ensure this doesnt happen
	if (IsOpen() && IsLocked())
	{
		UnlockDoor();
	}

	if (AllMajorDoorChunksDestroyed() && NavLinkProxy)
	{
		DisableNavLink();
		DestroyNavLink();
	}
	
	bool bCanPerformNavLinkChecks = NavLinkProxy != nullptr;

	// don't disable navlinks on broken doors just if its locked
	if (bCanPerformNavLinkChecks)
	{
		SCOPE_CYCLE_COUNTER(STAT_NavLinkChecks);
	
		if (IsJammed() || OperatingStates.Num() > 0 || CurrentActivities.Num() > 0 || CurrentStackUpActivities.Num() > 0)
		{
			DisableNavLink();
		}
		else
		{
			TestCanPathBothSidesOfDoor();
			
			if (NavLinkProxy)
			{
				TSubclassOf<UNavArea> ClosedNavArea = bHasEverBeenOpenedBySwat ? UNavArea_HasBeenOpenedDoor::StaticClass() : UNavArea_ClosedDoor::StaticClass();
				
				if (IsLocked())
				{
					if (bSuspectAlwaysUnlocks)
					{
						ClosedNavArea = UNavArea_LockedDoorSuspect::StaticClass();
					}
					else
					{
						ClosedNavArea = UNavArea_LockedDoor::StaticClass();
					}
				}

				if (bForceClosedDoorNavArea)
				{
					ClosedNavArea = UNavArea_ClosedDoor::StaticClass();
				}
				
				if (IsJammed())
				{
					ClosedNavArea = UNavArea_LockedDoor::StaticClass();
				}
				
				if (IsDoorBroken() || IsOpen())
				{
					ClosedNavArea = UNavArea_HasBeenOpenedDoor::StaticClass();
				}
				
				if (IsTrapLive())
				{
					ClosedNavArea = UNavArea_TrappedDoor::StaticClass();
				}
				
				if (NavLinkProxy->GetSmartLinkComp()->GetEnabledArea() != ClosedNavArea)
				{
					NavLinkProxy->GetSmartLinkComp()->SetEnabledArea(ClosedNavArea);
					NavLinkProxy->GetSmartLinkComp()->SetEnabled(true);
					
					if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						NavSys->UpdateActorInNavOctree(*NavLinkProxy);
						NavSys->UpdateComponentInNavOctree(*NavLinkProxy->GetSmartLinkComp());
					}
				}
			}
		}
	}

	// Local player dependent code below
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		DisableAllInteractables();
		return;
	}
	
	if (!GEngine->GameViewport)
		return;

	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
	{
		DisableAllInteractables();
		return;
	}
	
	if (FVector::Distance(PlayerCharacter->GetActorLocation(), GetActorLocation()) > 1000.0f)
	{
		DisableAllInteractables();
		return;
	}
	
	{
		SCOPE_CYCLE_COUNTER(STAT_TickActionPromptConditions);
		
		if (!bCanPlayerInteract /*|| !CanInteract_Implementation()*/)
		{
			DisableAllInteractables();
			
			return;
		}
		
		const bool bTeamKnowsLockState = TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());
		const bool bIsBlocked = (bTeamKnowsLockState && IsLocked()) || (bDoorJammed || bDoorBroken);
		
		// Lockpicking progress
		{
			float CurrentOperatingTime = 0.0f, MaxOperatingTime = 0.0f;
			bool bLockpickItemEquipped = false;
			
			if (AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
			{
				CurrentOperatingTime = Multitool->GetCurrentOperatingTime();
				MaxOperatingTime = Multitool->GetMaxOperatingTime();
				bLockpickItemEquipped = Multitool->CurrentToolKit == EMultitoolFunctions::MF_Lockpick;
			}
			else if (ALockpickGun* LockpickGun = Cast<ALockpickGun>(PlayerCharacter->GetEquippedItem()))
			{
				CurrentOperatingTime = LockpickGun->GetCurrentOperatingTime();
				MaxOperatingTime = LockpickGun->GetMaxOperatingTime();
				bLockpickItemEquipped = true;
			}
			
			if (bLockpickItemEquipped)
			{
				LockpickInteractableComponent->SetAnimatedIconName(CurrentOperatingTime > 0.0f && IsLocked() ? "Empty" : IsLocked() ? "Lockpick Door" : "Empty");
				LockpickInteractableComponent->bOverrideTickInterval = true;
				LockpickInteractableComponent->SetComponentTickInterval(0.0167f);
				LockpickInteractableComponent->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, MaxOperatingTime), FVector2D(0.0f, 1.0f), CurrentOperatingTime);
				LockpickInteractableComponent->ShowPromptAtDistance = 160.0f;
				LockpickInteractableComponent->ActionSlot1.bCondition = false;
				LockpickInteractableComponent->ActionSlot2.bCondition = CanLockpickDoor(PlayerCharacter) && LockpickInteractableComponent->CurrentProgress <= 0.0f;
				LockpickInteractableComponent->ActionSlot3.bCondition = LockpickInteractableComponent->CurrentProgress > 0.0f;
				
				if (DriveSubDoor && LockpickInteractableComponent->IsFocused() && LockpickInteractableComponent->CurrentProgress > 0.0f)
				{
					if (DriveSubDoor->LockpickInteractableComponent)
					{
						DriveSubDoor->LockpickInteractableComponent->bEnabled = false;
					}
				}
			}
			else
			{
				//LockpickInteractableComponent->SetAnimatedIconName(IsLocked() ? "Lockpick Door" : "Empty");
				//LockpickInteractableComponent->ActionSlot2.bCondition = false;

				LockpickInteractableComponent->SetAnimatedIconName(IsLocked() ? "Lockpick Door" : "");
				LockpickInteractableComponent->bOverrideTickInterval = false;
				LockpickInteractableComponent->ShowPromptAtDistance = 160.0f;
				LockpickInteractableComponent->CurrentProgress = 0.0f;
				LockpickInteractableComponent->ActionSlot2.bCondition = false;
				LockpickInteractableComponent->ActionSlot3.bCondition = false;
			}
		}

		// C2 placement points
		//if (DoorData.C2PlacementPoints.Num() > 0)
		{
			if (AC2Explosive* C2 = Cast<AC2Explosive>(PlayerCharacter->GetEquippedItem()))
			{
				C2->MaxPlacementDistance = C2InteractableComponent->ShowPromptAtDistance;
				
				C2InteractableComponent->bOverrideTickInterval = true;
				C2InteractableComponent->SetComponentTickInterval(0.0167f);
				C2InteractableComponent->SetWorldLocation(UKismetMathLibrary::VLerp(C2InteractableComponent->GetComponentLocation(), GetPlacementLocation_Implementation(C2->LastGoodPlacement), 12.0f * DeltaTime));

				//DrawDebugSphere(GetWorld(), GetPlacementLocation_Implementation(C2->LastGoodPlacement), 5, 12, FColor::Green, false, 0.033f);
			}
			else
			{
				C2InteractableComponent->ResetToOriginalLocation();
				C2InteractableComponent->bOverrideTickInterval = false;
				//C2InteractableComponent->ShowPromptAtDistance = 110.0f;
			}
		}

		// door check progress
		if (IsClosed() && IsLockable() && !bIsBlocked)
		{
			if (bHeldPushDoor)
			{
				PushDoorHoldTime += DeltaTime;
				if (PushDoorHoldTime > 0.2f)
					DoorPushInteractableComp->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.2f, 0.2f+0.25f), FVector2D(0.0f, 1.0f), PushDoorHoldTime);
				else
					DoorPushInteractableComp->CurrentProgress = 0.0f;
			}
			else
			{
				DoorPushInteractableComp->CurrentProgress = 0.0f;
				PushDoorHoldTime = 0.0f;
			}

			if (bHeldPushDoor && PushDoorHoldTime > 0.2f+0.25f)
			{
				bHeldPushDoor = false;
				if (IsLocked())
				{
					PlayLockedAnimation();
					PlayLockedSound();
				}
				else
				{
					PlayLockedAnimation();
					PlayHandleAnimation();
				}

				SetDoorLockKnowledge(PlayerCharacter->IsSuspect(), true);
			}
		}
		else
		{
			DoorPushInteractableComp->CurrentProgress = 0.0f;
			PushDoorHoldTime = 0.0f;
		}
		
		/*
		#if WITH_EDITOR
		if (PlacedC2)
		{
			if (const FDoorData* Data = TypeOfDoor.GetRow<FDoorData>("Debug"))
			{
				//DrawDebugSphere(GetWorld(), PlacedC2->GetActorLocation() + PlacedC2->GetActorRightVector() * 100.0f, 5, 12, FColor::Green, false, 0.033f);
				FHitResult FakeHit;
				FakeHit.TraceStart = PlacedC2->GetActorLocation() + PlacedC2->GetActorRightVector() * 100.0f;

				if (IsPointInFrontOfDoor(FakeHit.TraceStart))
				{
					PlacedC2->SetActorLocation(DoorStatic->GetComponentLocation() + Data->C2PlacementPoint_Front.RotateAngleAxis(GetActorRotation().Yaw + DoorStatic->GetRelativeRotation().Yaw, FVector::UpVector));
				}
				else
				{
					PlacedC2->SetActorLocation(DoorStatic->GetComponentLocation() + Data->C2PlacementPoint_Back.RotateAngleAxis(GetActorRotation().Yaw + DoorStatic->GetRelativeRotation().Yaw, FVector::UpVector));
				}
				
				PlacedC2->SetActorRotation(GetPlacementRotation_Implementation(FakeHit));
			}
		}
		#endif
		*/

		// Optiwand interactable camera tracking
		{
			if (/*AOptiwand* Optiwand = */Cast<AOptiwand>(PlayerCharacter->GetEquippedItem()) && (!IsActorSameSideAsTrap(PlayerCharacter) || (IsActorSameSideAsTrap(PlayerCharacter) && !IsTrapLive())))
			{
				FVector2D ViewportSize;
				GEngine->GameViewport->GetViewportSize(ViewportSize);
					
				FVector CameraLocation;
				FVector CameraDirection;
				UGameplayStatics::DeprojectScreenToWorld(PlayerCharacter->GetController<APlayerController>(), ViewportSize/2, CameraLocation, CameraDirection);

				FVector EndLocation = CameraLocation + CameraDirection * FVector::Distance(OptiwandInteractableComponent->GetComponentLocation(), CameraLocation);
				EndLocation = MirrorgunArea->GetComponentTransform().InverseTransformPosition(EndLocation);

				const float DoorSize = GetDoorSize().Y - 15.0f;
				const float NewRelativeY = FMath::Clamp(EndLocation.Y, -DoorSize, DoorSize);
				OptiwandInteractableComponent->SetRelativeLocation(FVector(OptiwandInteractableComponent->GetRelativeLocation().X, NewRelativeY, OptiwandInteractableComponent->GetRelativeLocation().Z));
				BackMirrorPoint->SetRelativeLocation(FVector(BackMirrorPoint->GetRelativeLocation().X, NewRelativeY, BackMirrorPoint->GetRelativeLocation().Z));
				FrontMirrorPoint->SetRelativeLocation(FVector(FrontMirrorPoint->GetRelativeLocation().X, NewRelativeY, FrontMirrorPoint->GetRelativeLocation().Z));
			}
			else
			{
				OptiwandInteractableComponent->ResetToOriginalLocation();
				
				BackMirrorPoint->SetRelativeLocation(FVector(BackMirrorPoint->GetRelativeLocation().X, 0.0f, BackMirrorPoint->GetRelativeLocation().Z));
				FrontMirrorPoint->SetRelativeLocation(FVector(FrontMirrorPoint->GetRelativeLocation().X, 0.0f, FrontMirrorPoint->GetRelativeLocation().Z));
			}
		}

		TimeSinceLastActionPromptUpdate += DeltaTime;
		if (TimeSinceLastActionPromptUpdate > 0.1f)
		{
			SCOPE_CYCLE_COUNTER(STAT_TickActionUpdate);

			TimeSinceLastActionPromptUpdate = 0.0f;
			
			// Action prompt conditions
			const bool bC2Equipped = UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_C2Explosive);
			const bool bHasBreachingShotgun = UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_BreachingShotgun);
			const bool bHasBatteringRam = UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_BatteringRam);
			const bool bHasDoorJam = UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Doorjam);
			const bool bHasOptiwand = UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Optiwand);
			const bool bCanOpenDoor = CanOpenDoor(PlayerCharacter);
			const bool bCanCloseDoor = CanCloseDoor(PlayerCharacter);
			const bool bCanPushDoor = CanPushDoor(PlayerCharacter);
			const bool bCanKickDoor = CanKickDoor(PlayerCharacter);
			//const bool bCanRamDoor = CanRamDoor(PlayerCharacter);
			const bool bCanMirrorUnderDoor = CanMirrorUnderDoor(PlayerCharacter);
			const bool bCanEquipOptiwand = CanEquipOptiwand(PlayerCharacter);
			const bool bCanEquipBreachingShotgun = CanEquipBreachingShotgun(PlayerCharacter);
			const bool bCanEquipBatteringRam = CanEquipBatteringRam(PlayerCharacter);
			const bool bIsOpen = IsOpen();
			const bool bCanInteract = CanInteract_Implementation() && !bC2Equipped;
			const bool bCanPeekDoor = CanPeekDoor(PlayerCharacter);
			const bool bDoorOpenInteractionValid = !((bTeamKnowsLockState && IsLocked()) || (bDoorJammed || IsMiddleChunkBroken()));

			FString DoorState = DoorStateAsString(PlayerCharacter->IsSuspect());
			
			DoorOpenInteractableComp->bEnabled = !IsMiddleChunkBroken() && !bC2Equipped;
			DoorOpenInteractableComp->SetAnimatedIconName(!IsMiddleChunkBroken() && (bCanOpenDoor || !bDoorOpenInteractionValid) ? "Open Door" : "");
			DoorOpenInteractableComp->SetInteractionIconState(bDoorOpenInteractionValid);
			
			DoorOpenInteractableComp->ActionSlot1.CustomActionPromptText = DoorState.IsEmpty() ? FText::GetEmpty() : FText::FromStringTable("ActionPromptTable", DoorState);
			DoorOpenInteractableComp->ActionSlot1.bCondition = !bDoorOpenInteractionValid;
			DoorOpenInteractableComp->ActionSlot1.bCheckForDisallowedItems = bDoorOpenInteractionValid;
			DoorOpenInteractableComp->ActionSlot2.bCondition = bCanOpenDoor;
			DoorOpenInteractableComp->ActionSlot2.bCheckForDisallowedItems = bDoorOpenInteractionValid;
			FString ActionPromptKey = bCanCloseDoor ? "CloseDoor" : "OpenDoor";
			DoorOpenInteractableComp->ActionSlot2.ActionText = FText::FromStringTable("ActionPromptTable", ActionPromptKey);

			DoorPushInteractableComp->bEnabled = bCanInteract;
			DoorPushInteractableComp->SetAnimatedIconName(!IsHandleBroken() && (bCanPushDoor || !bDoorOpenInteractionValid) ? "Peek Door" : "");
			DoorPushInteractableComp->SetInteractionIconState(bDoorOpenInteractionValid);
			DoorPushInteractableComp->ActionSlot1.bCondition = bCanPeekDoor && bCanOpenDoor;
			ActionPromptKey = bCanPushDoor && !bIsOpen ? "PeekDoor" : "CloseDoor";
			DoorPushInteractableComp->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", ActionPromptKey);
			DoorPushInteractableComp->ActionSlot2.bCondition = IsClosed() && IsLockable() && !bTeamKnowsLockState;
			DoorPushInteractableComp->ActionSlot2.bCheckForDisallowedItems = false;
			DoorPushInteractableComp->ActionSlot3.bUseCustomActionText = true;
			DoorPushInteractableComp->ActionSlot3.bCondition = bTeamKnowsLockState && !bHasEverBeenOpenedBySwat && !(DriveSubDoor ? DriveSubDoor->bHasEverBeenOpenedBySwat : false); // todo: wont work in pvp if in suspects team, need to refactor this
			ActionPromptKey = DoorState.IsEmpty() ? "DoorUnlocked" : DoorState;
			DoorPushInteractableComp->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", ActionPromptKey);

			if (DriveSubDoor)
			{
				const bool bIsSubDoorOpen = DriveSubDoor->IsOpen();
				
				DoorSublinkOpenInteractableComp->bEnabled = bCanInteract;
				DoorSublinkOpenInteractableComp->SetAnimatedIconName(bCanOpenDoor ? "Open Both Doors" : "");
				DoorSublinkOpenInteractableComp->SetInteractionIconState(bDoorOpenInteractionValid);
				DoorSublinkOpenInteractableComp->ActionSlot1.bCheckForDisallowedItems = bDoorOpenInteractionValid;
				DoorSublinkOpenInteractableComp->ActionSlot2.bCheckForDisallowedItems = bDoorOpenInteractionValid;
				DoorSublinkOpenInteractableComp->ActionSlot1.bCondition = bTeamKnowsLockState && IsLocked();
				DoorSublinkOpenInteractableComp->ActionSlot2.bCondition = (SubDoor_CanOpenDoors(PlayerCharacter) || SubDoor_CanCloseDoors(PlayerCharacter)) && (MainSubDoor_CanShowOpenDoorPrompt() || NonMainSubDoor_CanShowOpenDoorPrompt());
				ActionPromptKey = bCanCloseDoor ? "CloseBothDoors" : "OpenBothDoors";
				DoorSublinkOpenInteractableComp->ActionSlot2.ActionText = FText::FromStringTable("ActionPromptTable", ActionPromptKey);

				DoorSublinkPushInteractableComp->bEnabled = bCanInteract && (bMainSubDoor || (bCanPeekDoor && (bIsOpen || bIsSubDoorOpen)));
				DoorSublinkPushInteractableComp->SetAnimatedIconName(bCanPushDoor ? "Peek Both Doors" : "");
				DoorSublinkPushInteractableComp->SetInteractionIconState(bDoorOpenInteractionValid);
				DoorSublinkPushInteractableComp->ActionSlot1.bCheckForDisallowedItems = bDoorOpenInteractionValid;
				DoorSublinkPushInteractableComp->ActionSlot2.bCheckForDisallowedItems = bDoorOpenInteractionValid;
				DoorSublinkPushInteractableComp->ActionSlot1.bCondition = DriveSubDoor->CanPeekDoor(PlayerCharacter) && bCanPeekDoor && !(bTeamKnowsLockState && IsLocked()) && bCanOpenDoor;
				ActionPromptKey = bCanPushDoor && (!bIsOpen || !bIsSubDoorOpen) ? "PeekBothDoors" : "CloseBothDoors";
				DoorSublinkPushInteractableComp->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", ActionPromptKey);

				DoorSublinkKickInteractableComp->bEnabled = bCanInteract && (bMainSubDoor || (bCanKickDoor && (bIsOpen || bIsSubDoorOpen))) /*&& (!bKickAlwaysFails || !DriveSubDoor->bKickAlwaysFails)*/;
				DoorSublinkKickInteractableComp->ActionSlot1.bCondition = DriveSubDoor->CanKickDoor(PlayerCharacter) && bCanKickDoor && !(bIsOpen || DriveSubDoor->IsOpen());

				if (PlayerCharacter->GetEquippedItem())
				{
					DoorSublinkKickInteractableComp->SetAnimatedIconName(bCanKickDoor ? "Kick Both Doors" : (PlayerCharacter->GetEquippedItem()->bDisallowKicking ? "Empty" : ""));
					DoorSublinkKickInteractableComp->SetInteractionIconState(bCanKickDoor);
					DoorSublinkKickInteractableComp->ActionSlot2.bCondition = PlayerCharacter->GetEquippedItem()->bDisallowKicking;
					DoorSublinkKickInteractableComp->ActionSlot2.CustomActionPromptText = FText::Format(FText::FromStringTable("ActionPromptTable", "CannotKickWith"), PlayerCharacter->GetEquippedItem()->ItemName);
				}

				if (bMainSubDoor)
					WedgeInteractableComponent->SetRelativeLocation(FVector(WedgeInteractableComponent->GetRelativeLocation().X, 10.0f, WedgeInteractableComponent->GetRelativeLocation().Z));
			}
			else
			{
				DoorSublinkOpenInteractableComp->bEnabled = false;
				DoorSublinkPushInteractableComp->bEnabled = false;
				DoorSublinkKickInteractableComp->bEnabled = false;
			}

			const bool bKickDoorEnabled = !IsMiddleChunkBroken() && !AllMajorDoorChunksDestroyed() && !bC2Equipped;
			DoorKickInteractableComp->bEnabled = bKickDoorEnabled;
			DoorKickInteractableComp->ActionSlot1.bCondition = bCanKickDoor;

			if (PlayerCharacter->GetEquippedItem())
			{
				DoorKickInteractableComp->SetAnimatedIconName(bCanKickDoor ? "Kick Door" : (PlayerCharacter->GetEquippedItem()->bDisallowKicking || PlayerCharacter->IsLimbBroken(ELimbType::LT_RightLeg) ? "Empty" : ""));
				DoorKickInteractableComp->SetInteractionIconState(bCanKickDoor);
				
				if (PlayerCharacter->IsLimbBroken(ELimbType::LT_RightLeg))
				{
					DoorKickInteractableComp->ActionSlot1.bCondition = false;
					DoorKickInteractableComp->ActionSlot2.bCondition = true;
					DoorKickInteractableComp->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "CannotKickWithBrokenLegs");
				}
				else
				{
					DoorKickInteractableComp->ActionSlot2.bCondition = PlayerCharacter->GetEquippedItem()->bDisallowKicking;
					DoorKickInteractableComp->ActionSlot2.CustomActionPromptText = FText::Format(FText::FromStringTable("ActionPromptTable", "CannotKickWith"), PlayerCharacter->GetEquippedItem()->ItemName);
				}
			}
			
			LockpickInteractableComponent->bEnabled = bCanInteract && IsLockable();
			LockpickInteractableComponent->bShowIconWhenActionsLocked = true;

			bool bLockpickItemEquipped = false;
			
			if (const AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
			{
				bLockpickItemEquipped = Multitool->CurrentToolKit == EMultitoolFunctions::MF_Lockpick;
			}
			else if (Cast<ALockpickGun>(PlayerCharacter->GetEquippedItem()))
			{
				bLockpickItemEquipped = true;
			}
			
			LockpickInteractableComponent->ActionSlot1.bCondition = !bLockpickItemEquipped && IsLocked() && !IsElectronicDoor() && TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());

			if (PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_LockpickGun))
			{
				LockpickInteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", "EquipLockpickGun");
			}
			else if (PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Multitool))
			{
				LockpickInteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", "EquipLockpick");
			}
			
			C2InteractableComponent->bEnabled = !IsHandleBroken() && ( (bCanInteract && !PlayerCharacter->IsMoving() ) || (bC2Equipped && !bC2Placed)) && bCanUseC2;
			C2InteractableComponent->ActionSlot1.bCondition = CanEquipC2Explosive(PlayerCharacter);
			C2InteractableComponent->ActionSlot2.bCondition = CanPlaceC2Explosive(PlayerCharacter);

			WedgeInteractableComponent->bEnabled = bCanInteract && (!DriveSubDoor || (DriveSubDoor && bMainSubDoor)) && bHasDoorJam && bCanUseWedge;
			WedgeInteractableComponent->ActionSlot1.bCondition = CanEquipWedge(PlayerCharacter);
			WedgeInteractableComponent->ActionSlot2.bCondition = CanDeployWedge(PlayerCharacter);

			OptiwandInteractableComponent->bEnabled = bCanInteract && bHasOptiwand && bCanUseOptiwand;
			OptiwandInteractableComponent->SetAnimatedIconName(DoorData.bCanMirrorUnderDoor ? "Mirror Under Door" : "Mirror Blocked");
			OptiwandInteractableComponent->ActionSlot1.bCondition = bCanEquipOptiwand;
			OptiwandInteractableComponent->ActionSlot1.bLoopAnimation = false;
			OptiwandInteractableComponent->ActionSlot2.bCondition = bCanMirrorUnderDoor;
			OptiwandInteractableComponent->ActionSlot2.bLoopAnimation = false;
			OptiwandInteractableComponent->ActionSlot3.bCondition = !DoorData.bCanMirrorUnderDoor && !IsAnyInteractionPlaying();
			OptiwandInteractableComponent->ActionSlot3.bLoopAnimation = false;
			OptiwandInteractableComponent->bImprintIconOnHUDUponInteraction = bCanEquipOptiwand;

			BSGInteractableComponent->bEnabled = bCanInteract && !PlayerCharacter->IsMoving() && bHasBreachingShotgun && bCanUseBSG;
			BSGInteractableComponent->ActionSlot1.bCondition = bCanEquipBreachingShotgun;

			BSGInteractableComponent_2->bEnabled = bCanInteract && !PlayerCharacter->IsMoving() && bHasBreachingShotgun && bCanUseBSG;
			BSGInteractableComponent_2->ActionSlot1.bCondition = bCanEquipBreachingShotgun;

			DoorRamInteractableComponent->bEnabled = bCanInteract && !PlayerCharacter->IsMoving() && bHasBatteringRam && bCanUseRam;
			DoorRamInteractableComponent->ActionSlot1.bCondition = bCanEquipBatteringRam;
		}
	}
}

#if WITH_EDITOR
void ADoor::EditorTick(const float DeltaSeconds)
{
	//ISGAMEVIEWRETURN();

	/*
	TArray<FString> OutErrors, OutWarnings;
	GetErrors(OutErrors, OutWarnings);
	for (FString s : OutErrors)
	{
		const int32 Id = GetUniqueID();
		GEngine->AddOnScreenDebugMessage(Id+1, 5, FColor::Orange, s);
	}
	*/

	if (IsSelectedInEditor())
	{
		const auto ConvertRelativeToWorld = [this](TArray<FClearPoint>& InClearPoints)
		{
			for (FClearPoint& Point : InClearPoints)
			{
				if (Point.Location_Relative != FVector::ZeroVector)
				{
					FTransform T = GetActorTransform();
					if (IsNonMainSubdoor())
						T = DriveSubDoor->GetActorTransform();
					Point.Location = FVector(FIntVector((T.TransformPosition(FVector(FIntVector(Point.Location_Relative))))));
				}
			}
		};
		
		ConvertRelativeToWorld(FrontRightClearPoints);
		ConvertRelativeToWorld(FrontLeftClearPoints);
		ConvertRelativeToWorld(BackRightClearPoints);
		ConvertRelativeToWorld(BackLeftClearPoints);
	}
	
	// update clear point stage properties
	{
		const auto UpdateStage = [](TArray<FClearPoint>& InClearPoints)
		{
			for (uint8 i = 0; i < InClearPoints.Num(); i++)
			{
				InClearPoints[i].Stage = i;
			}
		};

		UpdateStage(FrontLeftClearPoints);
		UpdateStage(FrontRightClearPoints);
		UpdateStage(BackLeftClearPoints);
		UpdateStage(BackRightClearPoints);
	}

	if (bFirstEditorTick)
	{
		DisableNavLink();
		
		FrontMirrorPoint->SetArrowColor(FColor::Red);
		BackMirrorPoint->SetArrowColor(FColor::Cyan);
	}

	if (LightBlocker)
	{
		if (IsDoorwayOnly())
		{
			if (LightBlocker->IsVisible() || LightBlocker->IsVisibleInEditor())
			{
				LightBlocker->SetVisibility(false);
				LightBlocker->SetCastShadow(false);
			}
		}
	}

	bFirstEditorTick = false;

	if (IsSelectedInEditor())
	{
		OperatingStates.Empty();
		
		// always ensure 1 scale on the X direction only so it isn't stretched (some doors, although rare, do need to be scaled on the Y and Z)
		if (GetActorScale3D() != FVector::OneVector)
		{
			SetActorScale3D(FVector(1.0f, GetActorScale3D().Y, GetActorScale3D().Z));
		}

		if (DoorStatic)
		{
			if (DoorStatic->GetComponentScale() != FVector::OneVector)
			{
				DoorStatic->SetWorldScale3D(FVector(1.0f, DoorStatic->GetComponentScale().Y, DoorStatic->GetComponentScale().Z));
			}
		}

		UDestructibleDoorChunkComponent* Chunks[9] = {DoorChunk0, DoorChunk1, DoorChunk2, DoorChunk3, DoorChunk4, DoorChunk5, DoorChunk6, DoorChunk7, DoorChunk8};

		for (uint8 i = 0; i < 9; i++)
		{
			if (Chunks[i]->GetComponentScale() != FVector::OneVector)
			{
				Chunks[i]->SetWorldScale3D(FVector(1.0f, Chunks[i]->GetComponentScale().Y, Chunks[i]->GetComponentScale().Z));
			}
		}
		
		FVector OG = DoorBlockerComponent->GetUnscaledBoxExtent();
		DoorBlockerComponent->SetBoxExtent(FVector(OG.X, GetDoorSize().Y/1.9f, OG.Z));
		
		DrawStackUpPoints(FrontLeftStackUpPoints, FColor::Yellow);
		DrawStackUpPoints(FrontRightStackUpPoints, FColor::Cyan);

		DrawStackUpPoints(BackLeftStackUpPoints, FColor::Yellow);
		DrawStackUpPoints(BackRightStackUpPoints, FColor::Cyan);

		if (IsNonMainSubdoor())
		{
			FrontLeftClearPoints = {};
			FrontRightClearPoints = {};
			BackLeftClearPoints = {};
			BackRightClearPoints = {};
			
			DriveSubDoor->DrawClearPointsV2(DriveSubDoor->FrontLeftClearPoints);
			DriveSubDoor->DrawClearPointsV2(DriveSubDoor->FrontRightClearPoints);
			DriveSubDoor->DrawClearPointsV2(DriveSubDoor->BackLeftClearPoints);
			DriveSubDoor->DrawClearPointsV2(DriveSubDoor->BackRightClearPoints);
		}
		else
		{
			DrawClearPointsV2(BackLeftClearPoints);
			DrawClearPointsV2(BackRightClearPoints);
			DrawClearPointsV2(FrontLeftClearPoints);
			DrawClearPointsV2(FrontRightClearPoints);
		}

		if (bDrawC2PlacementPoints)
		{
			if (const FDoorData* data = TypeOfDoor.GetRow<FDoorData>("EditorTick"))
			{
				//int32 Index = 0;
				//for (const FVector& C2Point : data->C2PlacementPoints)
				{
					//if (data->C2PlacementPointOffsets_Back.IsValidIndex(Index))
					//	FinalPoint_Back += data->C2PlacementPointOffsets_Back[Index];
					
					DrawDebugSphere(GetWorld(), DoorStatic->GetComponentLocation() + data->C2PlacementPoint_Back.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector), 5.0f, 8, FColor::Orange, false, 0.033f, 0, 0.0f);
					DrawDebugSphere(GetWorld(), DoorStatic->GetComponentLocation() + data->C2PlacementPoint_Front.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector), 5.0f, 8, FColor::Orange, false, 0.033f, 0, 0.0f);

					/*
					if (IsPointInFrontOfDoor(C2Point))
					{
						FVector FinalPoint_Front = C2Point;
						if (data->C2PlacementPointOffsets_Front.IsValidIndex(Index))
							FinalPoint_Front += data->C2PlacementPointOffsets_Front[Index];

						DrawDebugSphere(GetWorld(), DoorStatic->GetComponentLocation() + FinalPoint_Front.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector), 5.0f, 8, FColor::Orange, false, 0.033f, 0, 0.0f);
					}
					else
					{
						FVector FinalPoint_Back = C2Point;
						if (data->C2PlacementPointOffsets_Back.IsValidIndex(Index))
							FinalPoint_Back += data->C2PlacementPointOffsets_Back[Index];
						
						DrawDebugSphere(GetWorld(), DoorStatic->GetComponentLocation() + FVector(-FinalPoint_Back.X, FinalPoint_Back.Y, FinalPoint_Back.Z).RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector), 5.0f, 8, FColor::Orange, false, 0.033f, 0, 0.0f);
					}
					*/

					//Index++;
				}
			}
		}
	}

	if (bDrawDoorwayExtent)
	{
		// Draw Doorway Collision Extent using DataTable values
		const FVector DataTableDoorLocation = DoorStatic->GetComponentLocation() + (DoorStatic->GetRightVector() * Doorway->GetUnscaledBoxExtent().Y) + (DoorStatic->GetUpVector() * Doorway->GetUnscaledBoxExtent().Z) + DoorData.DoorwayOffset.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector);
		const FVector DataTableBoxExtent = Doorway->GetUnscaledBoxExtent() *= DoorData.DoorwayOffsetScale;
		DrawDebugBox(GetWorld(), DataTableDoorLocation, DataTableBoxExtent, DoorStatic->GetComponentRotation().Quaternion(), FColor::Yellow, false, 0.05f, 0, 0.5f);

		// Draw Doorway Collision Extent using Debug values
		const FVector DebugDoorLocation = DoorStatic->GetComponentLocation() + (DoorStatic->GetRightVector() * Doorway->GetUnscaledBoxExtent().Y) + (DoorStatic->GetUpVector() * Doorway->GetUnscaledBoxExtent().Z) + DebugDoorwayOffset.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector);
		const FVector DebugBoxExtent = Doorway->GetUnscaledBoxExtent() *= DebugDoorwayScale;
		DrawDebugBox(GetWorld(), DebugDoorLocation, DebugBoxExtent, DoorStatic->GetComponentRotation().Quaternion(), FColor::Emerald, false, 0.05f, 0, 0.5f);
	}

	// Show/Hide door editor billboards from console variable
	{
		const bool bSpritesVisible = CVarRonDrawDoorInteractions.GetValueOnAnyThread() > 0 && TypeOfDoor.RowName != "Doorway";

		TInlineComponentArray<UBillboardComponent*> BillboardComponents(this, true);
		GetComponents(BillboardComponents, true);
		BillboardComponents.Remove(RoomPositionBillboard_Back);
		BillboardComponents.Remove(RoomPositionBillboard_Front);
		BillboardComponents.Remove(RoomPositionBillboard_DoubleDoor_Back);
		BillboardComponents.Remove(RoomPositionBillboard_DoubleDoor_Front);
		RoomPositionBillboard_Back->SetVisibility(CVarRonDrawDoorInteractions.GetValueOnAnyThread() > 0);
		RoomPositionBillboard_Front->SetVisibility(CVarRonDrawDoorInteractions.GetValueOnAnyThread() > 0);
		RoomPositionBillboard_DoubleDoor_Back->SetVisibility(CVarRonDrawDoorInteractions.GetValueOnAnyThread() > 0);
		RoomPositionBillboard_DoubleDoor_Front->SetVisibility(CVarRonDrawDoorInteractions.GetValueOnAnyThread() > 0);

		for (UBillboardComponent* UBillboardComponent : BillboardComponents)
		{
			UBillboardComponent->SetVisibility(bSpritesVisible);
		}
	}

	// Show/Hide door editor billboards if door doesn't support interactable
	{
		if (!IsDoorwayOnly())
		{
			C2Billboard_Front->SetVisibility(bCanUseC2);
			C2Billboard_Back->SetVisibility(bCanUseC2);

			BSGBillboard_Front->SetVisibility(bCanUseBSG);
			BSGBillboard_2_Front->SetVisibility(bCanUseBSG);

			BSGBillboard_Back->SetVisibility(bCanUseBSG);
			BSGBillboard_2_Back->SetVisibility(bCanUseBSG);

			WedgeBillboard_Front->SetVisibility(bCanUseWedge);
			WedgeBillboard_Back->SetVisibility(bCanUseWedge);

			BatteringRamBillboard_Back->SetVisibility(bCanUseRam);
			BatteringRamBillboard_Front->SetVisibility(bCanUseRam);

			OptiwandBillboard_Front->SetVisibility(bCanUseOptiwand);
			OptiwandBillboard_Back->SetVisibility(bCanUseOptiwand);
		}
	}

}

bool ADoor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void ADoor::Reset()
{
	Super::Reset();

	if (!GetWorld() && GetWorld()->bIsTearingDown)
		return;

	if (IsDoorwayOnly())
		return;
	
	if (GetLocalRole() >= ROLE_Authority)
	{
		if (const ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0))
		{
			if (const AReadyOrNotPlayerState* PS = Character->GetPlayerState<AReadyOrNotPlayerState>())
			{
				if (PS->bJoinInProgress)
					return;
			}
		}
		
		CreationTime = GetWorld()->GetTimeSeconds();

		Setup();
		
		bClientReset = true;
		ForceDoorReset();
		bDoorBroken = false;
		bDoorJammed = false;
		bDoorHandlesBroken = false;
		bHasEverBeenOpenedBySwat = false;
		
		{
			// Disable all nav blocking thigns!!
			TInlineComponentArray<UActorComponent*> Components;
			GetComponents(Components);
			for (UActorComponent* Comp : Components)
			{
				if (IsDoorwayOnly())
				{
					if (Comp != DoorBlockerComponent)
					{
						Comp->bNavigationRelevant = false;
						Comp->SetCanEverAffectNavigation(false);
					}
				}
				else
				{
					Comp->bNavigationRelevant = Comp == DoorStatic || ChunkComponents.Contains(Comp);
					Comp->SetCanEverAffectNavigation(Comp == DoorStatic || ChunkComponents.Contains(Comp));
				}

				if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
				{
					NavSys->UpdateActorInNavOctree(*this);
					NavSys->UpdateComponentInNavOctree(*Comp);
				}
			}
		}

		if (!IsDoorwayOnly())
		{
			DoorStatic->SetSimulatePhysics(false);
			DoorStatic->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);

			DoorHandleFront->SetSimulatePhysics(false);
			DoorHandleBack->SetSimulatePhysics(false);

			if (DoorHandleFront->IsValidLowLevel())
				DoorHandleFront->AttachToComponent(DoorStatic, FAttachmentTransformRules::SnapToTargetIncludingScale);
			
			if (DoorHandleFront->IsValidLowLevel())
				DoorHandleBack->AttachToComponent(DoorStatic, FAttachmentTransformRules::SnapToTargetIncludingScale);

			DoorHandleFront->SetRelativeLocation(DoorData.DoorHandleFrontRelativeTransform.GetLocation());
			DoorHandleFront->SetRelativeRotation(DoorData.DoorHandleFrontRelativeTransform.GetRotation());
			DoorHandleBack->SetRelativeLocation(DoorData.DoorHandleBackRelativeTransform.GetLocation());
			DoorHandleBack->SetRelativeRotation(DoorData.DoorHandleBackRelativeTransform.GetRotation());

			for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
			{
				if (Chunk)
				{
					Chunk->Restore();
					Chunk->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				}
			}
			
			CloseDoor(nullptr, true, false);
		}

		DestroyedChunkIdxs.Empty();
	}
}

void ADoor::PostLoad()
{
	Super::PostLoad();

	TypeOfDoor.DataTable = UBpGameplayHelperLib::GetDoorLookupDataTable();
	TypeOfTrap.DataTable = UBpGameplayHelperLib::GetTrapLookupDataTable();
	
	SetupTags();

	FVector OG = DoorBlockerComponent->GetUnscaledBoxExtent();
	DoorBlockerComponent->SetBoxExtent(FVector(OG.X, GetDoorSize().Y/1.9f, OG.Z));

	#if WITH_EDITOR
	bIsDoorway = IsDoorwayOnly();
	if (!bIsDoorway)
	{
		bNoAutomaticClearing = false;
	}
	#endif
}

void ADoor::PreSave(const ITargetPlatform* TargetPlatform)
{
	Super::PreSave(TargetPlatform);

	SetupTags();

	/*
	#if WITH_EDITOR
	SetFolderPath("Doors");
	SetActorLabel(TypeOfDoor.RowName.ToString() + "_" + FString::FromInt(GetFName().GetNumber()));
	#endif
	*/
	
	FVector OG = DoorBlockerComponent->GetUnscaledBoxExtent();
	DoorBlockerComponent->SetBoxExtent(FVector(OG.X, GetDoorSize().Y/1.9f, OG.Z));

	DeactivateDoorBlocker();
	//RemoveDoorBlocker();
}

void ADoor::DrawClearPoints(TArray<AThreatAwarenessActor*> ClearPoints, FColor Color)
{
	ClearPoints.Remove(nullptr);
	for (int32 i = 0; i < ClearPoints.Num(); i++)
	{
		FVector startLoc = i == 0 ? GetDoorway()->GetComponentLocation() : ClearPoints[i-1]->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		FVector endLoc = ClearPoints[i]->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		DrawDebugLine(GetWorld(), startLoc, endLoc, Color, false, 0.05f, 0);
	}
}

void ADoor::DrawClearPointsV2(TArray<FClearPoint> ClearPoints, float Lifetime)
{
	for (int32 i = 0; i < ClearPoints.Num(); i++)
	{
		FColor PointColor = FColor::White;
		switch (ClearPoints[i].Direction)
		{
			case EClearDirection::Right:
				PointColor = FColor::Yellow;
			break;
			
			case EClearDirection::Forward:
				PointColor = FColor::Magenta;
			break;
			
			default:
			break;
		}
		
		DrawDebugPoint(GetWorld(), FVector(ClearPoints[i].Location), 10.0f, PointColor, false, Lifetime);
		
		if (ClearPoints.IsValidIndex(i+1))
		{
			DrawDebugDirectionalArrow(GetWorld(), FVector(ClearPoints[i].Location), FVector(ClearPoints[i+1].Location), 150.0f, FColor::Cyan, false, Lifetime);
			//DrawDebugDirectionalArrow(GetWorld(), FVector(ClearPoints[i].Location), FVector(ClearPoints[i].Location) + ClearPoints[i].Normal * 50.0f, 50.0f, FColor::Orange, false, Lifetime);
		}

		for (const ACoverLandmark* Landmark : ClearPoints[i].CoverLandmarks)
		{
			if (Landmark)
			{
				DrawDebugLine(GetWorld(), FVector(ClearPoints[i].Location), Landmark->GetActorLocation(), FColor::Magenta, false, Lifetime);
			}
		}
	}
}

void ADoor::DrawStackUpPoints(TArray<AStackUpActor*> StackUpPoints, FColor Color)
{
	StackUpPoints.Remove(nullptr);
	for (int32 i = 0; i < StackUpPoints.Num(); i++)
	{
		FVector startLoc =  GetDoorway()->GetComponentLocation();
		FVector endLoc = StackUpPoints[i]->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		
		DrawDebugLine(GetWorld(), startLoc, endLoc, Color, false, 0.05f, 0);
		DrawDebugBox(GetWorld(), StackUpPoints[i]->GetActorLocation() + FVector::UpVector *35.0f, FVector(35.0f), StackUpPoints[i]->GetDebugColor(), false, 0.05f, 0, 2.5f);
	}
}

void ADoor::CalculateRoomPositioning()
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GetWorld()))
	{
		WorldData->GenerateDoorPositions(this);
	}
}

void ADoor::GenerateStackUpPoints()
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GetWorld()))
	{
		WorldData->DeleteDoorStackUps(this);
		WorldData->GenerateDoorStackUpsV2(this);
	}
}

void ADoor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InteractableComponent)
{
	if (!InteractInstigator || !InteractableComponent)
		return;

	const bool bCanInteract = !AllMajorDoorChunksDestroyed();

	if (!bCanInteract || !bCanPlayerInteract)
		return;
	
	if (InteractableComponent == DoorOpenInteractableComp)
	{
		if (!InteractInstigator->GetEquippedItem() ||
			(InteractInstigator->GetEquippedItem() && !InteractInstigator->GetEquippedItem()->IsDoorPushAnimationPlaying()))
		{
			OpenDoor(InteractInstigator);
		}
		
		// Play the door push animation if we have it.
		if (InteractInstigator->GetEquippedItem())
		{
			InteractInstigator->GetEquippedItem()->PlayDoorPushAnimation();
		}
	}
	else if (InteractableComponent == DoorPushInteractableComp)
	{
		const bool bTeamKnowsLockState = TeamKnowsDoorLockState(InteractInstigator->IsSuspect());
		
		if (IsClosed() && IsLockable() && !bTeamKnowsLockState)
		{
			bHeldPushDoor = true;
			PushDoorHoldTime = 0.0f;
		}
		else
		{
			PeekDoor(InteractInstigator, IncrementAngle);
		}
	}
	else if (InteractableComponent == DoorSublinkOpenInteractableComp)
	{
		if (DriveSubDoor)
		{
			if (!InteractInstigator->GetEquippedItem() || (InteractInstigator->GetEquippedItem() && !InteractInstigator->GetEquippedItem()->IsDoorPushAnimationPlaying()))
			{
				DriveSubDoor->OpenDoor_SpecificAngle(InteractInstigator, -OpenDoor(InteractInstigator));
			}
		}
		else
		{
			if (!InteractInstigator->GetEquippedItem() || (InteractInstigator->GetEquippedItem() && !InteractInstigator->GetEquippedItem()->IsDoorPushAnimationPlaying()))
			{
				OpenDoor(InteractInstigator);
			}
		}

		// Play the door push animation if we have it.
		if (InteractInstigator->GetEquippedItem())
		{
			InteractInstigator->GetEquippedItem()->PlayDoorPushAnimation();
		}
	}
	else if (InteractableComponent == DoorSublinkPushInteractableComp)
	{
		if (DriveSubDoor)
		{
			DriveSubDoor->PeekDoor(InteractInstigator, IncrementAngle);
			PeekDoor(InteractInstigator, IncrementAngle);
		}
		else
		{
			PushDoor(InteractInstigator, IncrementAngle);
		}
	}
	
	else if (InteractableComponent == DoorKickInteractableComp)
	{
		if (CanKickDoor(InteractInstigator))
		{
			bPendingSubDoorKick = false;
			InteractInstigator->KickDoor(this);
		}
	}
	else if (InteractableComponent == DoorSublinkKickInteractableComp)
	{
		if (CanKickDoor(InteractInstigator))
		{
			bPendingSubDoorKick = true;
			InteractInstigator->KickDoor(this);
		}
	}
	
	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (InteractableComponent == C2InteractableComponent)
		{
			InteractCharacter->EquipC2();
		}
		else if (InteractableComponent == LockpickInteractableComponent)
		{
			if (InteractCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_LockpickGun))
			{
				InteractCharacter->EquipItemOfType(EItemCategory::IC_LockpickGun);
			}
			else
			{
				InteractCharacter->Server_EquipMultitool_Implementation(EMultitoolFunctions::MF_Lockpick);
			}
		}
		else if (InteractableComponent == WedgeInteractableComponent)
		{
			InteractCharacter->EquipDoorJam();
		}
		else if (InteractableComponent == OptiwandInteractableComponent)
		{
			if (CanEquipOptiwand(InteractCharacter))
			{
				InteractCharacter->EquipMirrorgun();
			}
		}
		else if (InteractableComponent == BSGInteractableComponent || InteractableComponent == BSGInteractableComponent_2)
		{
			InteractCharacter->EquipBreachingShotgun();
		}
		else if (InteractableComponent == DoorRamInteractableComponent)
		{
			InteractCharacter->EquipBatteringRam();
		}
	}
}

void ADoor::EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	OperatingStates.Remove("Player");
	
	if (InInteractableComponent == DoorPushInteractableComp)
	{
		if (bHeldPushDoor)
		{
			bHeldPushDoor = false;
			if (PushDoorHoldTime < 0.2f)
				PeekDoor(InteractInstigator, IncrementAngle);
		}
	}
}

void ADoor::Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InteractableComponent)
{
	if (!InteractInstigator || !InteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (InteractableComponent == LockpickInteractableComponent)
		{
			InteractCharacter->StartLockPicking(this);
			
			OperatingStates.AddUnique("Player");
		}
		else if (InteractableComponent == WedgeInteractableComponent)
		{
			InteractCharacter->JamDoor(this);
			
			OperatingStates.AddUnique("Player");
		}
		else if (InteractableComponent == C2InteractableComponent)
		{
			InteractCharacter->C2Door(this);
			
			OperatingStates.AddUnique("Player");
		}
	}
}

void ADoor::EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InteractableComponent)
{
	OperatingStates.Remove("Player");
	
	if (!InteractInstigator || !InteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (InteractableComponent == LockpickInteractableComponent)
		{
			InteractCharacter->StopLockPicking(this);
		}
		else if (InteractableComponent == WedgeInteractableComponent)
		{
			InteractCharacter->StopUsingMultitool(this);
		}
	}
}

void ADoor::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (InteractCharacter->LastInteractableComponent &&
			InteractCharacter->LastInteractableComponent == InInteractableComponent)
		{
			EndFire_Implementation(InteractInstigator, InInteractableComponent);
		}
	}
}

bool ADoor::CanInteract_Implementation() const
{
	return !bDoorBroken && !AllMajorDoorChunksDestroyed();
}

bool ADoor::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (Hit.GetActor())
	{
		return Hit.GetActor()->IsA(ADoor::StaticClass());
	}

	return false;
}

void ADoor::CheckDoorHit()
{
	SCOPE_CYCLE_COUNTER(STAT_CheckDoorHit);

	if (AllMajorDoorChunksDestroyed())
		return;

	if (AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		for (AReadyOrNotCharacter* Character : GameState->AllReadyOrNotCharacters)
		{
			if (!Character)
				continue;

			if (!UReadyOrNotSignificanceManager::IsActorRelevant(Character))
				continue;

			if (Character->IsTableMontagePlaying("tp_stumble"))
				continue;

			FVector LastPawnPosition = Character->GetActorLocation();

			if (FVector::Distance(LastPawnPosition, GetDoorMidLocation()) < 350.0f && !Character->IsDeadOrUnconscious() && CanPushDoor(Character))
			{
				FTransform TestTransform;
				TestTransform.SetLocation(DoorStatic->GetComponentLocation() + (DoorStatic->GetRightVector() * Doorway->GetUnscaledBoxExtent().Y) + (DoorStatic->GetUpVector() * Doorway->GetUnscaledBoxExtent().Z) + DoorData.DoorwayOffset.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector));
				TestTransform.SetRotation(DoorStatic->GetComponentRotation().Quaternion());
				TestTransform.SetScale3D(DoorStatic->GetComponentScale());

				FVector BoxExtent = Doorway->GetUnscaledBoxExtent() * DoorData.DoorwayOffsetScale;
				BoxExtent.X += Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

				const bool bInFrontOfDoor = IsActorInFrontOfDoor(Character);
				if (UKismetMathLibrary::IsPointInBoxWithTransform(LastPawnPosition, TestTransform, BoxExtent))
				{
					FVector ClosestPoint = CalculateClosestPoint(Character->GetActorLocation());
					
					// Runs every time we're inside the hitbox
					if (CharactersOverlappingDoor.Contains(Character))
					{
						if (Character->IsArrestedOrSurrendered())
						{
							//DrawDebugBox(GetWorld(), TestTransform.GetLocation(), BoxExtent, TestTransform.GetRotation(), FColor::Red, false, -1, 0, 1.0f);
							//DrawDebugPoint(GetWorld(), ClosestPoint, 30.0f, FColor::Green);

							if (bInFrontOfDoor)
							{
								//Player->SetActorLocation(ClosestPoint + DoorStatic->GetForwardVector().GetSafeNormal2D() * Player->GetCapsuleComponent()->GetUnscaledCapsuleRadius());

								Character->AddMovementInput(DoorStatic->GetForwardVector().GetSafeNormal2D() * Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 10.0f);
							}
							else
							{
								//Player->SetActorLocation(ClosestPoint - DoorStatic->GetForwardVector().GetSafeNormal2D() * Player->GetCapsuleComponent()->GetUnscaledCapsuleRadius());

								Character->AddMovementInput(DoorStatic->GetForwardVector().GetSafeNormal2D() * Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 10.0f);
							}
						}
						else
						{
							if (Character->CanPushDoor(this))
							{
								if (((IsOpen() && !bDoorBroken) || (CanPushDoorWhileBroken())) && !IsAnyInteractionPlaying()/* && Player->IsMovingForward()*/)
								{
									PushDoor(Character, PhysicalPushAmount, false, false);
								}
							}
						}
					}
					// Runs one time. Good for one time events
					else
					{
						CharactersOverlappingDoor.AddUnique(Character);

						if (!Character->IsArrestedOrSurrendered())
						{
							if (!AllMajorDoorChunksDestroyed() && IsOpen())
							{
								if (IsOpening() && IsActorBehindDoor_Relative(Character))
								{
									TimeSinceLastBlocked = 0.0f;
									OpenCloseAmountSinceLastBlocked = FMath::Abs(OpenCloseAmount);
									CloseDoor(Character, false, false);

									FVector Direction = (ClosestPoint - Character->GetActorLocation()).GetSafeNormal2D();

									const ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(Character);
									if (AI && AI->GetCyberneticsController())
									{
										FVector MoveLocation = AI->GetNavAgentLocation() + -Direction * 100.0f;
										AI->GetCyberneticsController()->GiveMoveTo(MoveLocation);
									}
									
									OnDoorMovementBlocked.Broadcast();
								}
								else if (IsClosing())
								{
									OpenDoor_SpecificAngle(Character, bInFrontOfDoor ? MaxOpenClose : -MaxOpenClose, false, false);
									OnDoorMovementBlocked.Broadcast();
								}

								if (LastDoorUser != Character)
								{
									if (!APlayerCharacter::IsOnSameTeam(LastDoorUser, Character) && LastDoorDamage != EDoorDamageType::DDT_None)
									{
										ApplyDoorDamage(LastDoorDamage, Character);
									}
								}
							}
						}
					}

					//ULog::Vector(ClosestPoint, false, "Closest Point: ");
					//ULog::Vector(Pawn->GetActorLocation(), false, "Pawn location: ");
					//ULog::Vector(Pawn->GetActorLocation() - LastPawnPosition, false, "Moved pawn by: ");
				}
				else
				{
					CharactersOverlappingDoor.Remove(Character);
				}

				#if WITH_EDITOR
				if (CVarRonDrawDoorCollisions.GetValueOnAnyThread() > 0)
				{
					FColor Color = UKismetMathLibrary::IsPointInBoxWithTransform(LastPawnPosition, TestTransform, BoxExtent) ? FColor::Emerald : FColor::Yellow;
					DrawDebugBox(GetWorld(), TestTransform.GetLocation(), BoxExtent, DoorStatic->GetComponentRotation().Quaternion(), Color, false, 0.1f, 0, 2.f);
				}
				#endif
			}
		}
	}
}

void ADoor::OnRep_ClientResetDoor()
{
	if (!bClientReset)
		return;
	
	bClientReset = false;
	bDoorBroken = false;
	bDoorJammed = false;
	bC2Placed = false;
	bDoorHandlesBroken = false;
	bHasEverBeenOpenedBySwat = false;

	Setup();

	if (!IsDoorwayOnly())
	{
		DoorStatic->SetSimulatePhysics(false);
		DoorStatic->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);

		DoorHandleFront->SetSimulatePhysics(false);
		DoorHandleBack->SetSimulatePhysics(false);

		if (DoorHandleFront->IsValidLowLevel())
			DoorHandleFront->AttachToComponent(DoorStatic, FAttachmentTransformRules::SnapToTargetIncludingScale);
			
		if (DoorHandleFront->IsValidLowLevel())
			DoorHandleBack->AttachToComponent(DoorStatic, FAttachmentTransformRules::SnapToTargetIncludingScale);

		DoorHandleFront->SetRelativeLocation(DoorData.DoorHandleFrontRelativeTransform.GetLocation());
		DoorHandleFront->SetRelativeRotation(DoorData.DoorHandleFrontRelativeTransform.GetRotation());
		DoorHandleBack->SetRelativeLocation(DoorData.DoorHandleBackRelativeTransform.GetLocation());
		DoorHandleBack->SetRelativeRotation(DoorData.DoorHandleBackRelativeTransform.GetRotation());

		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (Chunk)
			{
				Chunk->Restore();
				Chunk->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
			
		CloseDoor(nullptr, true, false);
	}

	DestroyedChunkIdxs.Empty();
}

FVector ADoor::GetTargetLocation(AActor* RequestedBy) const
{
	return GetDoorHighlight()->GetComponentLocation();
}

void ADoor::ApplyDoorDamage(const EDoorDamageType InDoorDamage, AReadyOrNotCharacter* Victim)
{
	if (!Victim)
		return;
	
	if (InDoorDamage == EDoorDamageType::DDT_None)
		return;

	if (Cast<ASWATCharacter>(Victim))
		return;

	if (InDoorDamage == EDoorDamageType::DDT_Kicking ||
		InDoorDamage == EDoorDamageType::DDT_Ramming)
	{
		Victim->MoveIgnoreActorAdd(this);
		Victim->PlayMontageFromTable("tp_stumble");
	}
	
	switch (InDoorDamage)
	{
		case EDoorDamageType::DDT_None: break;
		case EDoorDamageType::DDT_Blasting:		Victim->StartStun(EStunType::ST_Stung); break;
		case EDoorDamageType::DDT_Shotgunning:	Victim->StartStun(EStunType::ST_Beanbag); break;
		case EDoorDamageType::DDT_Ramming:		Victim->StartStun(EStunType::ST_Stung); break;
		case EDoorDamageType::DDT_Kicking:		Victim->StartStun(EStunType::ST_Stung); break;
		default: ;
	}

	ULog::Info("Applying door damage (" + DoorDamageTypeToString(InDoorDamage) + ") to " + Victim->GetName());
}

void ADoor::ApplyRandomDamageToChunks(const float MinDamage, const float MaxDamage)
{
	if (!DoorData.bIsDestructible)
		return;
	
	auto OnChunkDestroyed = [&](UDestructibleDoorChunkComponent* Chunk)
	{
		Chunk->AddImpulse(GetActorForwardVector() * -5000.0f);
	};
	
	for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
	{
		if (Chunk)
		{
			Chunk->DoDamage(FMath::RandRange(FMath::Abs(MinDamage), FMath::Abs(MaxDamage)));
			Chunk->CheckSupportChunks();
			
			if (Chunk->IsDestroyed())
			{
				OnChunkDestroyed(Chunk);
			}
		}
	}

	if (AnyChunksDestroyed())
	{
		BreakDoor(false);
	}
}

void ADoor::DisableDoorChunkNavigation()
{
	for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
	{
		if (Chunk)
		{
			if (Chunk->IsSimulatingPhysics())
			{
				Chunk->SetCanEverAffectNavigation(false);
				if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
				{
					NavSys->UpdateActorInNavOctree(*this);
					NavSys->UpdateComponentInNavOctree(*Chunk);
				}
			}		
		}
	}
}

void ADoor::BlockAllDoorways()
{
	AWorldDataGenerator::Get(GetWorld())->BlockAllDoorways();
}

void ADoor::UnblockAllDoorways()
{
	AWorldDataGenerator::Get(GetWorld())->UnblockAllDoorways();
}

void ADoor::ActivateDoorBlocker()
{
	ActivateDoorBlocker(false);
}

void ADoor::ActivateDoorBlockerForWorldGen()
{
	ActivateDoorBlocker(true);
}

void ADoor::ActivateDoorBlocker_Trap()
{
	ActivateDoorBlocker(true);
}

void ADoor::DeactivateDoorBlocker()
{
	if (!DoorBlockerComponent->CanEverAffectNavigation())
		return;
	
	DoorBlockerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorBlockerComponent->SetCanEverAffectNavigation(false);

	if (DriveSubDoor)
	{
		DriveSubDoor->DoorBlockerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DriveSubDoor->DoorBlockerComponent->SetCanEverAffectNavigation(false);
	}

	if (const UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (DriveSubDoor)
			NavSys->UpdateComponentInNavOctree(*DriveSubDoor->DoorBlockerComponent);
		
		NavSys->UpdateComponentInNavOctree(*DoorBlockerComponent);
	}
}

void ADoor::ActivateDoorBlocker(bool bForce)
{
	if (bNoNavBlockerForGen && !bForce) // && GetWorld()->WorldType != EWorldType::Editor)
		return;
	
	// if already enabled, ignore..
	if (DoorBlockerComponent->CanEverAffectNavigation())
		return;
	
	DoorBlockerComponent->SetRelativeScale3D(FVector(0.15f, 2.0f, 1.0f));
	DoorBlockerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorBlockerComponent->SetCanEverAffectNavigation(true);

	if (DriveSubDoor)
	{
		DriveSubDoor->DoorBlockerComponent->SetRelativeScale3D(FVector(0.15f, 2.0f, 1.0f));
		DriveSubDoor->DoorBlockerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DriveSubDoor->DoorBlockerComponent->SetCanEverAffectNavigation(true);
	}

	if (const UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (DriveSubDoor)
			NavSys->UpdateComponentInNavOctree(*DriveSubDoor->DoorBlockerComponent);
		
		NavSys->UpdateComponentInNavOctree(*DoorBlockerComponent);
	}
}

void ADoor::SetDoorBlockerAreaClass(TSubclassOf<UNavAreaBase> NewNavArea)
{
	DoorBlockerComponent->AreaClass = NewNavArea;
}

void ADoor::ActivateBreachBlockers(bool bFront)
{
	float FurthestStackUpDistance = 0.0f;
	float FurthestStackUpDistance_Right = 0.0f;
	
	{
		const TArray<AStackUpActor*>& StackUpActors = GetStackupsForArea(bFront ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_BackRight);
		if (StackUpActors.Num() > 0)
		{
			float FurthestStackUpDistance_Forward = ((StackUpActors.Last()->GetActorLocation() - GetDoorMidLocation()) * GetActorForwardVector()).Size();
			FurthestStackUpDistance = FurthestStackUpDistance_Forward;
			if (!bFront)
				FurthestStackUpDistance_Forward = -FurthestStackUpDistance_Forward;
				
			float RightDistance = ((StackUpActors.Last()->GetActorLocation() - GetDoorMidLocation()) * GetActorRightVector()).Size();
			FurthestStackUpDistance_Right = RightDistance;
			BreachBlocker1Component->SetRelativeLocation(FVector(FurthestStackUpDistance_Forward, RightDistance + 130.0f, 0.0f));
			BreachBlocker1Component->SetBoxExtent(FVector(FMath::Abs(FurthestStackUpDistance_Forward), 1.0f, 32.0f));
		}
		else
		{
			BreachBlocker1Component->SetRelativeLocation(FVector::ZeroVector);
			BreachBlocker1Component->SetBoxExtent(FVector::ZeroVector);
		}
	}

	{
		const TArray<AStackUpActor*>& StackUpActors = GetStackupsForArea(bFront ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_BackLeft);
		if (StackUpActors.Num() > 0)
		{
			float FurthestStackUpDistance_Forward = ((StackUpActors.Last()->GetActorLocation() - GetDoorMidLocation()) * GetActorForwardVector()).Size();
			if (FurthestStackUpDistance_Forward > FurthestStackUpDistance)
				FurthestStackUpDistance = FurthestStackUpDistance_Forward;
			
			if (!bFront)
				FurthestStackUpDistance_Forward = -FurthestStackUpDistance_Forward;
			float RightDistance = ((StackUpActors.Last()->GetActorLocation() - GetDoorMidLocation()) * GetActorRightVector()).Size();
			if (RightDistance > FurthestStackUpDistance_Right)
				FurthestStackUpDistance_Right = RightDistance;
			BreachBlocker2Component->SetRelativeLocation(FVector(FurthestStackUpDistance_Forward, -(RightDistance + 130.0f), 0.0f));
			BreachBlocker2Component->SetBoxExtent(FVector(FMath::Abs(FurthestStackUpDistance_Forward), 1.0f, 32.0f));
		}
		else
		{
			BreachBlocker2Component->SetRelativeLocation(FVector::ZeroVector);
			BreachBlocker2Component->SetBoxExtent(FVector::ZeroVector);
		}
	}

	FurthestStackUpDistance = (FurthestStackUpDistance + 250.0f);
	if (!bFront)
		FurthestStackUpDistance = -FurthestStackUpDistance;
	
	BreachBlocker3Component->SetRelativeLocation(FVector(FurthestStackUpDistance, 60.0f, 0.0f));
	BreachBlocker3Component->SetBoxExtent(FVector(1.0f, FurthestStackUpDistance_Right + 100.0f, 32.0f));
	
	if (!BreachBlocker1Component->IsZeroExtent())
	{
		BreachBlocker1Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BreachBlocker1Component->SetCanEverAffectNavigation(true);
	}

	if (!BreachBlocker2Component->IsZeroExtent())
	{
		BreachBlocker2Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BreachBlocker2Component->SetCanEverAffectNavigation(true);
	}
	
	if (!BreachBlocker3Component->IsZeroExtent())
	{
		BreachBlocker3Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BreachBlocker3Component->SetCanEverAffectNavigation(true);
	}

	if (const UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->UpdateComponentInNavOctree(*BreachBlocker1Component);
		NavSys->UpdateComponentInNavOctree(*BreachBlocker2Component);
		NavSys->UpdateComponentInNavOctree(*BreachBlocker3Component);
	}

	TimeSinceBreachBlockersActivated = 0.0f;
}

void ADoor::DeactivateBreachBlockers()
{
	TimeSinceBreachBlockersActivated = 86400.0f;
	
	BreachBlocker1Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BreachBlocker1Component->SetCanEverAffectNavigation(false);
	
	BreachBlocker2Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BreachBlocker2Component->SetCanEverAffectNavigation(false);
	
	BreachBlocker3Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BreachBlocker3Component->SetCanEverAffectNavigation(false);

	if (const UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->UpdateComponentInNavOctree(*BreachBlocker1Component);
		NavSys->UpdateComponentInNavOctree(*BreachBlocker2Component);
		NavSys->UpdateComponentInNavOctree(*BreachBlocker3Component);
	}
}

void ADoor::ToggleLightBlocker()
{
	if (LightBlocker)
	{
		if (IsDoorwayOnly())
		{
			LightBlocker->SetCastShadow(false);
			LightBlocker->SetVisibility(false);
			LightBlocker->SetHiddenInGame(true);
		}
		else
		{
			const bool bIsVisible = LightBlocker->IsVisible() || LightBlocker->IsVisibleInEditor();
			LightBlocker->SetCastShadow(!bIsVisible);
			LightBlocker->SetVisibility(!bIsVisible);
			LightBlocker->SetHiddenInGame(true);
		}
	}
}

void ADoor::Setup()
{
	SetupDoor();
	
	ResetDoorLockKnowledge();
	ResetDoorTrapKnowledge();

	if (bAlwaysLocked)
	{
		SetLocked(true);
	}
	else
	{
		if (bOverrideLockChance)
			SetLocked(FMath::FRand() <= LockedChance);
	}

	if (!HasAuthority())
		return;

	SetupTrap();
}

void ADoor::SetupDoor()
{
	if (!GetWorld())
		return;

	ChunkComponents.Empty(9);
	ChunkComponents.Add(DoorChunk0);
	ChunkComponents.Add(DoorChunk1);
	ChunkComponents.Add(DoorChunk2);
	ChunkComponents.Add(DoorChunk3);
	ChunkComponents.Add(DoorChunk4);
	ChunkComponents.Add(DoorChunk5);
	ChunkComponents.Add(DoorChunk6);
	ChunkComponents.Add(DoorChunk7);
	ChunkComponents.Add(DoorChunk8);

	const FName RowName = TypeOfDoor.RowName; // Need to make a copy because if TypeOfDoor.RowName has a value of NAME_None, the IsNone() check would fail for some reason, thus logging a warning to the console
	if (!TypeOfDoor.IsNull() && !RowName.IsNone())
	{
		if (const FDoorData* Data = TypeOfDoor.GetRow<FDoorData>("Setup Door"))
		{
			DoorData = *Data;

			OnRep_DoorDataUpdated();

			bHasEverBeenOpenedBySwat = false;
			bDoorBroken = AnyChunksDestroyed();

			if (IsDoorwayOnly())
			{
				DoorBlockerComponent->SetRelativeScale3D(FVector(0.15f, 2.0f, 1.0f));

				FVector OG = DoorBlockerComponent->GetUnscaledBoxExtent();
				DoorBlockerComponent->SetBoxExtent(FVector(OG.X, GetDoorSize().Y/1.9f, OG.Z));
				
				#if WITH_EDITORONLY_DATA
				HideEditorComponents();
				#endif

				if (GetWorld()->WorldType != EWorldType::Editor)
				{
					#if WITH_EDITORONLY_DATA
					DestroyEditorComponents();
					#endif
				}

				if (LightBlocker)
				{
					LightBlocker->SetCastShadow(false);
					LightBlocker->SetVisibility(false);
				}

				if (GetWorld()->WorldType != EWorldType::Editor)
				{
					DestroyAllInteractionComponents();
					DestroyAllChunkComponents();

					DESTROY_COMPONENT(LightBlocker);

					DESTROY_COMPONENT(DoorHandleFront);
					DESTROY_COMPONENT(DoorHandleBack);
					
					DESTROY_COMPONENT(DoorArea);
					DESTROY_COMPONENT(LockpickArea);
					DESTROY_COMPONENT(C2Area);
					DESTROY_COMPONENT(MirrorgunArea);
					DESTROY_COMPONENT(FrontMirrorPoint);
					DESTROY_COMPONENT(BackMirrorPoint);
					DESTROY_COMPONENT(BSGArea);
					DESTROY_COMPONENT(WedgeArea);
					DESTROY_COMPONENT(BatteringRamArea);
					DESTROY_COMPONENT(FMODAudioPropagationComp);

					ChunkComponents.Empty();
				}

				return;
			}
			
			#if WITH_EDITORONLY_DATA
			ShowEditorComponents();
			#endif

			if (DoorHandleFront)
			{
				DoorHandleFront->SetCanEverAffectNavigation(false);
			}
			
			if (DoorHandleBack)
			{
				DoorHandleBack->SetCanEverAffectNavigation(false);
			}
			
			if (LightBlocker)
			{
				LightBlocker->SetCastShadow(true);
			}

			if (!IsDestructible())
			{
				if (GetWorld()->WorldType != EWorldType::Editor)
				{
					DestroyAllChunkComponents();
					ChunkComponents.Empty();
				}
			}
		}
	}

	if (StartingOpenAngle != 0.0f)
		OpenCloseAmount = StartingOpenAngle;
}

void ADoor::TriggerAlarm(AReadyOrNotCharacter* DoorInteractionInstigator)
{
	if (!bAlarmTriggered && bIsElectronicDoor && bLocked)
	{
		Multicast_PlayElectronicDoorSound(DoorData.AlarmSound);
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 0.0f, "AlarmTrap");

		bAlarmTriggered = true;
	}
}

void ADoor::CheckKeycards(AReadyOrNotCharacter* DoorInteractionInstigator)
{
	bool bHasKeycardAccess = DoorInteractionInstigator->IsSuspect() ? bSuspectsHaveKeycard : bSWATHasKeycard;
	if (IsLocked() && IsElectronicDoor() && bHasKeycardAccess)
	{
		SetLocked(false);
		Multicast_PlayElectronicDoorSound(DoorData.KeycardSound);
	}
}

void ADoor::OnRep_DoorDataUpdated()
{
	MaxOpenClose = FMath::Abs(DoorData.DoorMaxOpenClose);

	NumSuccessfulKicksToBreakDown = FMath::Clamp(DoorData.NumSuccessfulKicksToBreakDown, 0, 100);
	DoorKickSuccessChance = FMath::Clamp(DoorData.DoorKickSuccessChance, 0.1f, 1.0f);

	KickedParticleSystem = nullptr;
	LockBrokenParticleSystem = nullptr;
	
	if (IsDoorwayOnly())
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (IsValid(Chunk))
			{
				Chunk->SetStaticMesh(nullptr);
			}
		}

		ChunkComponents.Empty();

		if (DoorHandleFront)
			DoorHandleFront->SetStaticMesh(nullptr);
		
		if (DoorHandleBack)
			DoorHandleBack->SetStaticMesh(nullptr);
		
		if (DoorStatic)
		{
			DoorStatic->SetStaticMesh(nullptr);
			DoorStatic->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		return;
	}

	KickedParticleSystem = DoorData.KickedParticleSystem.LoadSynchronous();
	LockBrokenParticleSystem = DoorData.LockBrokenParticleSystem.LoadSynchronous();
	
	if (IsDestructible())
	{
		if (DoorStatic)
			DoorStatic->SetStaticMesh(nullptr);

		int32 i = 0;
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (IsValid(Chunk))
			{
				if (DoorData.DestructibleChunks.IsValidIndex(i))
				{
					// for (const int32 ChunkIndex : DoorData.DestructibleChunks[i].SupportChunks)
					// {
					// 	if (ChunkComponents.IsValidIndex(ChunkIndex))
					// 		Chunk->AddSupportChunk(ChunkComponents[ChunkIndex]);
					// }

					// Top
					if(i+3 < 9)
						Chunk->AddSupportChunk(ChunkComponents[i+3]);
					// Bottom
					if(i-3 > -1)
						Chunk->AddSupportChunk(ChunkComponents[i-3]);
					
					// Top
					if(i%3 == 0)
						Chunk->AddSupportChunk(ChunkComponents[i+1]);
					else if(i%3 == 1)
					{
						Chunk->AddSupportChunk(ChunkComponents[i-1]);
						Chunk->AddSupportChunk(ChunkComponents[i+1]);
					}
					if(i%3== 2)
						Chunk->AddSupportChunk(ChunkComponents[i-1]);
					
					FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(EAttachmentRule::KeepRelative, false);
					Chunk->AttachToComponent(GetRootComponent(), AttachmentRules);
					Chunk->SetStaticMesh(DoorData.DestructibleChunks[i].Mesh);
					Chunk->SetIsDoorHandle(DoorData.DestructibleChunks[i].bIsDoorHandleChunk);
					Chunk->SetIsHinge(DoorData.DestructibleChunks[i].bIsHinge);
					Chunk->SetCannotKickIfDestroyed(DoorData.DestructibleChunks[i].bCannotKickIfDestroyed);
				}
			}

			i++;
		}
	}
	else
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (IsValid(Chunk))
			{
				Chunk->SetStaticMesh(nullptr);
			}
		}

		ChunkComponents.Empty();

		if (DoorStatic)
		{
			DoorStatic->SetStaticMesh(DoorData.DoorMesh);
			DoorStatic->SetSimulatePhysics(false);
		}
	}

	if (DoorStatic)
	{
		DoorStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DoorStatic->SetCollisionObjectType(ECC_DOOR);
		DoorStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	
	if (DoorData.bCustomLockpickLocation)
	{
		if (IsValid(LockpickInteractableComponent))
			LockpickInteractableComponent->SetRelativeLocation(DoorData.LockpickRelativeLocation);
	}

	if (DoorData.bCustomDoorPeekLocation)
	{
		if (IsValid(DoorPushInteractableComp))
			DoorPushInteractableComp->SetRelativeLocation(DoorData.DoorPeekRelativeLocation);
	}

	if (DoorHandleFront)
	{
		if (DoorData.bDoorHandleFront)
		{
			DoorHandleFront->SetStaticMesh(DoorData.DoorHandle);
			DoorHandleFront->SetRelativeTransform(DoorData.DoorHandleFrontRelativeTransform);
		}
		else
		{
			DoorHandleFront->SetStaticMesh(nullptr);
		}
	}

	if (DoorHandleBack)
	{
		if (DoorData.bDoorHandleBack)
		{
			DoorHandleBack->SetStaticMesh(DoorData.DoorHandle);
			DoorHandleBack->SetRelativeTransform(DoorData.DoorHandleBackRelativeTransform);
		}
		else
		{
			DoorHandleBack->SetStaticMesh(nullptr);
		}
	}

	// Destroy all attached decals (like c2 blast)
	if (LockpickArea)
	{
		TArray<USceneComponent*> ChildrenComponents;
		LockpickArea->GetChildrenComponents(true, ChildrenComponents);
		
		for (USceneComponent* ChildComponent : ChildrenComponents)
		{
			if (UDecalComponent* AttachedDecalComponent = Cast<UDecalComponent>(ChildComponent))
			{
				AttachedDecalComponent->DestroyComponent(true);
			}
		}
	}

	DoorBlockerComponent->SetRelativeScale3D(FVector(0.15f, 2.0f, 1.0f));
	
	FVector OG = DoorBlockerComponent->GetUnscaledBoxExtent();
	DoorBlockerComponent->SetBoxExtent(FVector(OG.X, GetDoorSize().Y/1.9f, OG.Z));
}

void ADoor::SetFrontRoomPosition(EDoorRoomPosition NewPosition)
{
	FrontRoomPosition = NewPosition;

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		if (DriveSubDoor)
		{
			if (bMainSubDoor)
			{
				switch (NewPosition)
				{
					case EDoorRoomPosition::Center:			RoomPositionBillboard_DoubleDoor_Front->SetSprite(CenterIcon); break;
					case EDoorRoomPosition::CornerLeft:		RoomPositionBillboard_DoubleDoor_Front->SetSprite(CornerLeftIcon); break;
					case EDoorRoomPosition::CornerRight:	RoomPositionBillboard_DoubleDoor_Front->SetSprite(CornerRightIcon); break;
					case EDoorRoomPosition::Hallway:		RoomPositionBillboard_DoubleDoor_Front->SetSprite(HallwayIcon); break;
					case EDoorRoomPosition::HallwayLeft:	RoomPositionBillboard_DoubleDoor_Front->SetSprite(HallwayLeftIcon); break;
					case EDoorRoomPosition::HallwayRight:	RoomPositionBillboard_DoubleDoor_Front->SetSprite(HallwayRightIcon); break;
					default: ;
				}
				
				RoomPositionBillboard_Front->SetSprite(nullptr);
				RoomPositionBillboard_Front->SetVisibility(false);
				RoomPositionBillboard_Back->SetSprite(nullptr);
				RoomPositionBillboard_Back->SetVisibility(false);

				//DriveSubDoor->SetFrontRoomPosition(NewPosition);
			}
			else
			{
				RoomPositionBillboard_Front->SetSprite(nullptr);
				RoomPositionBillboard_Front->SetVisibility(false);
				RoomPositionBillboard_Back->SetSprite(nullptr);
				RoomPositionBillboard_Back->SetVisibility(false);
				RoomPositionBillboard_DoubleDoor_Back->SetSprite(nullptr);
				RoomPositionBillboard_DoubleDoor_Front->SetSprite(nullptr);
				RoomPositionBillboard_DoubleDoor_Back->SetVisibility(false);
				RoomPositionBillboard_DoubleDoor_Front->SetVisibility(false);
			}
		}
		else
		{
			switch (NewPosition)
			{
				case EDoorRoomPosition::Center:			RoomPositionBillboard_Front->SetSprite(CenterIcon); break;
				case EDoorRoomPosition::CornerLeft:		RoomPositionBillboard_Front->SetSprite(CornerLeftIcon); break;
				case EDoorRoomPosition::CornerRight:	RoomPositionBillboard_Front->SetSprite(CornerRightIcon); break;
				case EDoorRoomPosition::Hallway:		RoomPositionBillboard_Front->SetSprite(HallwayIcon); break;
				case EDoorRoomPosition::HallwayLeft:	RoomPositionBillboard_Front->SetSprite(HallwayLeftIcon); break;
				case EDoorRoomPosition::HallwayRight:	RoomPositionBillboard_Front->SetSprite(HallwayRightIcon); break;
				default: ;
			}
		}
	}
	#endif
}

void ADoor::SetBackRoomPosition(EDoorRoomPosition NewPosition)
{
	BackRoomPosition = NewPosition;

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		if (DriveSubDoor)
		{
			if (bMainSubDoor)
			{
				switch (NewPosition)
				{
					case EDoorRoomPosition::Center:			RoomPositionBillboard_DoubleDoor_Back->SetSprite(CenterIcon); break;
					case EDoorRoomPosition::CornerLeft:		RoomPositionBillboard_DoubleDoor_Back->SetSprite(CornerLeftIcon); break;
					case EDoorRoomPosition::CornerRight:	RoomPositionBillboard_DoubleDoor_Back->SetSprite(CornerRightIcon); break;
					case EDoorRoomPosition::Hallway:		RoomPositionBillboard_DoubleDoor_Back->SetSprite(HallwayIcon); break;
					case EDoorRoomPosition::HallwayLeft:	RoomPositionBillboard_DoubleDoor_Back->SetSprite(HallwayLeftIcon); break;
					case EDoorRoomPosition::HallwayRight:	RoomPositionBillboard_DoubleDoor_Back->SetSprite(HallwayRightIcon); break;
					default: ;
				}
				
				RoomPositionBillboard_Front->SetSprite(nullptr);
				RoomPositionBillboard_Front->SetVisibility(false);
				RoomPositionBillboard_Back->SetSprite(nullptr);
				RoomPositionBillboard_Back->SetVisibility(false);

				//DriveSubDoor->SetBackRoomPosition(NewPosition);
			}
			else
			{
				RoomPositionBillboard_Front->SetSprite(nullptr);
				RoomPositionBillboard_Front->SetVisibility(false);
				RoomPositionBillboard_Back->SetSprite(nullptr);
				RoomPositionBillboard_Back->SetVisibility(false);
				RoomPositionBillboard_DoubleDoor_Back->SetSprite(nullptr);
				RoomPositionBillboard_DoubleDoor_Front->SetSprite(nullptr);
				RoomPositionBillboard_DoubleDoor_Back->SetVisibility(false);
				RoomPositionBillboard_DoubleDoor_Front->SetVisibility(false);
			}
		}
		else
		{
			switch (NewPosition)
			{
				case EDoorRoomPosition::Center:			RoomPositionBillboard_Back->SetSprite(CenterIcon); break;
				case EDoorRoomPosition::CornerLeft:		RoomPositionBillboard_Back->SetSprite(CornerLeftIcon); break;
				case EDoorRoomPosition::CornerRight:	RoomPositionBillboard_Back->SetSprite(CornerRightIcon); break;
				case EDoorRoomPosition::Hallway:		RoomPositionBillboard_Back->SetSprite(HallwayIcon); break;
				case EDoorRoomPosition::HallwayLeft:	RoomPositionBillboard_Back->SetSprite(HallwayLeftIcon); break;
				case EDoorRoomPosition::HallwayRight:	RoomPositionBillboard_Back->SetSprite(HallwayRightIcon); break;
				default: ;
			}
		}
	}
	#endif
}

void ADoor::SetupTrap()
{
	if (!HasAuthority())
		return;
	
	if (bNoSpawnTrap)
	{
		if (AttachedTrap)
		{
			AttachedTrap->TrapDeInit();
			AttachedTrap->Destroy();
			AttachedTrap = nullptr;
		}

		AttachTrap(nullptr);
		
		return;
	}
	
	if (!bMainSubDoor && DriveSubDoor)
	{
		if (DriveSubDoor->GetAttachedTrap())
		{
			bTrapInFront = IsActorInFrontOfDoor(DriveSubDoor->GetAttachedTrap());

			AttachTrap(DriveSubDoor->GetAttachedTrap(), false);
		}

		return;
	}

	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		if (AttachedTrap)
		{
			AttachedTrap->TrapInit();

			bTrapInFront = IsActorInFrontOfDoor(AttachedTrap);
			return;
		}
	}

	if ((bMainSubDoor && DriveSubDoor) || (!bMainSubDoor && !DriveSubDoor))
	{
		if (AttachedTrap)
		{
			AttachedTrap->TrapDeInit();
			AttachedTrap->Destroy();
			AttachedTrap = nullptr;
		}

		AttachTrap(nullptr);
	}

	ForEachAttachedActors([&](AActor* AttachedActor)
	{
		if (ATrapActorAttachedToDoor* TrapAttachedToDoor = Cast<ATrapActorAttachedToDoor>(AttachedActor))
		{
			if (TrapAttachedToDoor != AttachedTrap)
			{
				TrapAttachedToDoor->TrapDeInit();
				TrapAttachedToDoor->Destroy();
			}
		}
	
		return true;
	});

	if (AttachedTrap)
		return;

	if (!HasAuthority())
		return;

	// Need to make a copy because if TypeOfDoor.RowName has a value of NAME_None, the IsNone() check would fail for some reason, thus logging a warning to the console
	const FName RowName = TypeOfTrap.RowName;
	if (!TypeOfTrap.IsNull() && !RowName.IsNone())
	{
		if (FTrapData* Data = TypeOfTrap.GetRow<FTrapData>("TrapLookupData"))
		{
			TrapData = *Data;
			
			FTransform RelativeTransform = TrapData.TrapRelativeTransform;

			const bool bInvertTrap = TrapSide == EDoorTrapSide::Back;
			
			if (bInvertTrap)
			{
				RelativeTransform.SetLocation(FVector(-RelativeTransform.GetLocation().X, RelativeTransform.GetLocation().Y, RelativeTransform.GetLocation().Z));
				//RelativeTransform.SetRotation(RelativeTransform.GetRotation().Inverse());
			}

			FHitResult Hit, Hit2;
			//float RightOffset = bShouldInvertTrap ? -15.0f : ((GetDoorSize().Y*2)-20.0f);
			float RightOffset = GetDoorSize().Y*2;
			FVector Start = DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * RightOffset + DoorStatic->GetForwardVector() * (bInvertTrap ? -50.0f : 50.0f) + FVector::UpVector * RelativeTransform.GetLocation().Z;
			FVector End = DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * RightOffset + FVector::UpVector * RelativeTransform.GetLocation().Z;
			GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);
			//DrawDebugLine(GetWorld(), Start, End, Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 2.0f);
			//DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.0f, FColor::Orange, false, 2.0f);

			FVector LocalHitLoc = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
			FVector LocalEnd = GetActorTransform().InverseTransformPosition(End);
			LocalHitLoc.Z = RelativeTransform.GetLocation().Z;
			LocalEnd.Z = RelativeTransform.GetLocation().Z;
			
			if (Hit.bBlockingHit)
			{
				FVector Offset = (LocalHitLoc - LocalEnd).GetAbs();
				//LOG_VECTOR(Offset);

				if (bInvertTrap)
					RelativeTransform.SetLocation(RelativeTransform.GetLocation() - Offset);
				else
					RelativeTransform.SetLocation(RelativeTransform.GetLocation() + Offset);
				
				RelativeTransform.SetLocation(RelativeTransform.GetLocation() + FVector::ForwardVector * (bInvertTrap ? TrapData.InvertOffset : -5.0f));
			}
			
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(DoorStatic->GetComponentLocation() + RelativeTransform.GetLocation());
			SpawnTransform.SetRotation(RelativeTransform.GetRotation());
			SpawnTransform.SetScale3D(RelativeTransform.GetScale3D());

			//DrawDebugBox(GetWorld(), SpawnTransform.GetLocation(), FVector(15.0f), FColor::Red, false, 1.0f);
				
			if (ATrapActorAttachedToDoor* SpawnedTrap = GetWorld()->SpawnActorDeferred<ATrapActorAttachedToDoor>(TrapData.TrapClass, SpawnTransform))
			{
				AttachTrap(SpawnedTrap);

				SpawnedTrap->SetActorRelativeTransform(RelativeTransform);
					
				SpawnedTrap->AttachedToDoor = this;
				SpawnedTrap->TrapType = TrapData.TrapType;
				
				SpawnedTrap->FinishSpawning(SpawnTransform);
				SpawnedTrap->TrapInit();

				//if (!Hit.bBlockingHit)
				{
					RightOffset = -15.0f;
					Start = DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * RightOffset + DoorStatic->GetForwardVector() * (bInvertTrap ? -50.0f : 50.0f) + FVector::UpVector * RelativeTransform.GetLocation().Z;
					End = DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * RightOffset + FVector::UpVector * RelativeTransform.GetLocation().Z;
					GetWorld()->LineTraceSingleByChannel(Hit2, Start, End, ECC_Visibility);
					//DrawDebugLine(GetWorld(), Start, End, Hit2.bBlockingHit ? FColor::Green : FColor::Red, false, 2.0f);
					//DrawDebugPoint(GetWorld(), Hit2.ImpactPoint, 10.0f, FColor::Orange, false, 2.0f);
				}
				
				if (Hit2.bBlockingHit)
				{
					FVector LocalHitLoc2 = GetActorTransform().InverseTransformPosition(Hit2.ImpactPoint);
					float Offset = LocalHitLoc2.X - RelativeTransform.GetLocation().X;
					SpawnedTrap->CutCableComponent1->EndLocation.Y = Offset;
					SpawnedTrap->CutCableComponent2->EndLocation.Y = Offset;

					SpawnedTrap->SplineComponent->SetLocationAtSplinePoint(1, FMath::ClosestPointOnLine(SpawnedTrap->GetActorLocation(), SpawnedTrap->GetActorTransform().TransformPosition(SpawnedTrap->CutCableComponent1->EndLocation), SpawnedTrap->SplineComponent->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World)), ESplineCoordinateSpace::World);
					SpawnedTrap->SplineComponent->SetLocationAtSplinePoint(2, FMath::ClosestPointOnLine(SpawnedTrap->GetActorLocation(), SpawnedTrap->GetActorTransform().TransformPosition(SpawnedTrap->CutCableComponent1->EndLocation), SpawnedTrap->SplineComponent->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::World)), ESplineCoordinateSpace::World);
					SpawnedTrap->SplineComponent->SetLocationAtSplinePoint(3, FMath::ClosestPointOnLine(SpawnedTrap->GetActorLocation(), SpawnedTrap->GetActorTransform().TransformPosition(SpawnedTrap->CutCableComponent1->EndLocation), SpawnedTrap->SplineComponent->GetLocationAtSplinePoint(3, ESplineCoordinateSpace::World)), ESplineCoordinateSpace::World);
				}
				
				bTrapInFront = IsActorInFrontOfDoor(SpawnedTrap);
			}
		}
		else
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to find trap data for row '%s'"), *TypeOfTrap.RowName.ToString());
		}
	}
	
	Tick(0.0f);
}

void ADoor::SetElectronicallyLocked(bool bIsLocked)
{
	bIsElectronicDoor = true;
	SetLocked(true);

	// TODO(killo): localizable text
}

void ADoor::OnRep_TrapDataUpdated()
{
}

FRotator ADoor::GetInteractionRotation() const
{
	return DoorStatic->GetSocketRotation("InteractionSocket");
}

FVector ADoor::GetInteractionLocation() const
{
	return Doorway->GetComponentLocation();
}

FString ADoor::DoorStateAsString(const bool bIsSuspect) const
{
	if (bDoorBroken)
		return "DoorBroken";

	if (bDoorJammed)
		return "DoorJammed";

	if (IsLocked() && TeamKnowsDoorLockState(bIsSuspect))
		return "DoorLocked";

	return FString();
}

bool ADoor::IsJammed() const
{
	return bDoorJammed || (DriveSubDoor ? DriveSubDoor->bDoorJammed : false);
}

bool ADoor::IsDoorwayOnly() const
{
	return DoorData.DestructibleChunks.Num() == 0 && DoorData.DoorMesh == nullptr;
}

bool ADoor::IsMirrorBlocked() const
{
	return !DoorData.bCanMirrorUnderDoor;
}

void ADoor::Server_SetLockKnowledgeState_Implementation(const bool bSuspectTeam, const bool bNewKnowledgeState)
{
	SetDoorLockKnowledge(bSuspectTeam, bNewKnowledgeState);
}

void ADoor::Server_SetTrapKnowledgeState_Implementation(const bool bSuspectTeam, const bool bNewKnowledgeState)
{
	SetDoorTrapKnowledge(bSuspectTeam, bNewKnowledgeState);
}

void ADoor::DecreaseNumKicksToBreakDown(AReadyOrNotCharacter* DoorKickCharacter, bool& bShouldOpenDoor, bool& bCanBreakLock, float KickChanceOffset)
{
	TriggerAlarm(DoorKickCharacter);

	if (bKickAlwaysFails)
	{
		bShouldOpenDoor = false;
		return;
	}
	
	const float FinalKickChance = FMath::Clamp(DoorKickSuccessChance - (IsLocked() ? LockedChance/10.0f : 0.0f) - KickChanceOffset, 0.0f, 1.0f);

	if (FMath::FRand() < FinalKickChance)
	{
		NumSuccessfulKicksToBreakDown = FMath::Clamp<uint8>(NumSuccessfulKicksToBreakDown - 1, 0, NumSuccessfulKicksToBreakDown);

		if (DriveSubDoor)
		{
			DriveSubDoor->NumSuccessfulKicksToBreakDown = NumSuccessfulKicksToBreakDown;
		}
	}

	// Increase chance after every kick, more likely for door to be kicked open
	DoorKickSuccessChance = FMath::Clamp(DoorKickSuccessChance + 0.05f, 0.1f, 1.0f);

	bShouldOpenDoor = NumSuccessfulKicksToBreakDown <= 0;
}

void ADoor::LockDoor(const bool bLockSubDoor)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;
	
	bLocked = true;
	Multicast_SetLocked(true);
	
	if (DriveSubDoor && bLockSubDoor)
	{
		DriveSubDoor->bLocked = true;
		DriveSubDoor->Multicast_SetLocked(true);
	}
}

void ADoor::Multicast_SetLocked_Implementation(bool bShouldLocked)
{
	bLocked = bShouldLocked;
}

void ADoor::UnlockDoor(const bool bUnlockSubDoor)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;
	
	bLocked = false;
	Multicast_SetLocked(false);
	
	if (DriveSubDoor && bUnlockSubDoor)
	{
		DriveSubDoor->bLocked = false;
		DriveSubDoor->Multicast_SetLocked(false);
	}
}

bool ADoor::IsLocked() const
{
	return DoorData.bLockable && bLocked;
}

bool ADoor::IsIgnoredForFlee()
{
	return bIgnoreForFlee;
}

bool ADoor::IsLockable() const
{
	return DoorData.bLockable;
}

bool ADoor::IsOpen() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(OpenThreshold);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsClosed() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	
	return FMath::IsNearlyZero(A, 1.0f) || A == 0.0f || OpenCloseAmount == 0.0f;
}

bool ADoor::IsOpen_Forward() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = DoorStatic->GetRelativeRotation().Yaw;
	const float B = FMath::Abs(OpenThreshold);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsOpen_Backward() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = DoorStatic->GetRelativeRotation().Yaw;
	const float B = -FMath::Abs(OpenThreshold);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A <= B;
}

bool ADoor::IsFullyOpen_Forward() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = DoorStatic->GetRelativeRotation().Yaw;
	const float B = FMath::Abs(MaxOpenClose);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsFullyOpen_Backward() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = DoorStatic->GetRelativeRotation().Yaw;
	const float B = -FMath::Abs(MaxOpenClose);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A <= B;
}

bool ADoor::IsFullyOpen() const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(MaxOpenClose);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsHalfwayOpen() const
{
	return IsOpenAtOrBeyond(0.5f);
}

bool ADoor::IsOpenBeyondIncrementThreshold() const
{
	return IsOpenAtOrBeyond_Angle(IncrementAngle);
}

bool ADoor::IsOpenBeyondCloseThreshold() const
{
	return IsOpenAtOrBeyond_Angle(CloseThreshold);
}

bool ADoor::IsOpenBy(const float Percentage) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(MaxOpenClose)*FMath::Clamp(Percentage, 0.0f, 1.0f);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A <= B;
}

bool ADoor::IsOpenBy_Angle(const float Angle) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(Angle);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A <= B;
}

bool ADoor::IsOpenBeyond(const float Percentage) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(MaxOpenClose)*FMath::Clamp(Percentage, 0.0f, 1.0f);

	return !FMath::IsNearlyEqual(A, B, 0.01f) && A > B;
}

bool ADoor::IsOpenAtOrBeyond(const float Percentage) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(MaxOpenClose)*FMath::Clamp(Percentage, 0.0f, 1.0f);

	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsOpenBeyond_Angle(const float Angle) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(Angle);
	
	return !FMath::IsNearlyEqual(A, B, 0.01f) && A > B;
}

bool ADoor::IsOpenAtOrBeyond_Angle(const float Angle) const
{
	if (IsDoorwayOnly() || AllMajorDoorChunksDestroyed() || DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return true;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(Angle);
	
	return FMath::IsNearlyEqual(A, B, 0.01f) || A >= B;
}

bool ADoor::IsAttachedToRoot() const
{
	return DoorStatic->GetAttachParent() == RootComponent;
}

float ADoor::GetOpenAmountAsPercentage() const
{
	if (IsDoorwayOnly())
		return MaxOpenClose;
	
	const float A = FMath::Abs(DoorStatic->GetRelativeRotation().Yaw);
	const float B = FMath::Abs(MaxOpenClose);

	return FMath::Clamp(A/B, 0.0f, 1.0f);
}

bool ADoor::IsStackUpDisabled(const FVector CommandLocation) const
{
	return (IsPointInFrontOfDoorway(CommandLocation) && bCanIssueOrdersOnFrontSide) || (!IsPointInFrontOfDoorway(CommandLocation) && bCanIssueOrdersOnBackSide);
}

bool ADoor::CanPushDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;
	
	if (PlayerCharacter && (PlayerCharacter->IsDeadOrUnconscious() || PlayerCharacter->IsIncapacitated()))
		return false;

	if (const ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(PlayerCharacter))
	{
		if (const ACyberneticController* Controller = CyberneticCharacter->GetCyberneticsController())
		{
			if (Controller->GetCurrentActivity<UDoorInteractionActivity>() ||
				Controller->GetCurrentActivity<UTeamStackUpActivity>())
			{
				return false;
			}
		}
	}
	
	return !IsFullyOpen() || (IsFullyOpen_Backward() && IsActorInFrontOfDoor(PlayerCharacter)) || (IsFullyOpen_Forward() && !IsActorInFrontOfDoor(PlayerCharacter));
}

bool ADoor::CanPullDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	return !CanPushDoor(PlayerCharacter);
}

bool ADoor::SubDoor_CanOpenDoors(AReadyOrNotCharacter* PlayerCharacter) const
{
	return DriveSubDoor && (!DriveSubDoor->IsFullyOpen() && !IsFullyOpen()) && CanOpenDoor(PlayerCharacter);
}

bool ADoor::SubDoor_CanCloseDoors(AReadyOrNotCharacter* PlayerCharacter) const
{
	return DriveSubDoor && (DriveSubDoor->CanCloseDoor(PlayerCharacter) && CanCloseDoor(PlayerCharacter)) && CanOpenDoor(PlayerCharacter);
}

bool ADoor::MainSubDoor_CanShowOpenDoorPrompt() const
{
	const bool bIsOpen = IsOpen();
	const bool bSubDoorCanInteract = DriveSubDoor && DriveSubDoor->CanInteract_Implementation();
	const bool bIsSubDoorOpen = DriveSubDoor && DriveSubDoor->IsOpen();

	return bMainSubDoor && bSubDoorCanInteract && ((!bIsOpen && !bIsSubDoorOpen) || (bIsOpen && bIsSubDoorOpen) || (IsOpenBy_Angle(IncrementAngle) || DriveSubDoor->IsOpenBy_Angle(IncrementAngle)));
}

bool ADoor::NonMainSubDoor_CanShowOpenDoorPrompt() const
{
	const bool bIsOpen = IsOpen();
	const bool bIsFullyOpen = IsFullyOpen();
	const bool bSubDoorCanInteract = DriveSubDoor && DriveSubDoor->CanInteract_Implementation();
	const bool bIsSubDoorOpen = DriveSubDoor && DriveSubDoor->IsOpen();
	const bool bIsSubDoorFullyOpen = DriveSubDoor && DriveSubDoor->IsFullyOpen();
	
	return !bMainSubDoor && bSubDoorCanInteract && ((bIsSubDoorOpen && !bIsSubDoorFullyOpen) || (bIsFullyOpen && bIsSubDoorOpen) || (bIsOpen && !bIsFullyOpen && !bIsSubDoorOpen));
}

bool ADoor::CanPeekDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (!IsOpenBy_Angle(IncrementAngle))
		return false;
	if (!CanPushDoor(PlayerCharacter))
		return false;
	
	return true;
}

void ADoor::RemoveWedges()
{
	if (AttachedWedge)
	{
		if (AttachedWedge->PlacedBy)
		{
			AttachedWedge->PlacedBy->GetInventoryComponent()->AddInventoryItem(AttachedWedge);
		}
		
		AttachedWedge = nullptr;
	}
	
	bDoorJammed = false;
}

bool ADoor::IsAnyAIOpening() const
{
	if (GetWorld() && GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (const ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
		{
			if (AI->IsOpeningDoor(const_cast<ADoor*>(this)))
			{
				return true;
			}
		}
	}
	
	return IsOpening();
}

bool ADoor::IsAnyAIClosing() const
{
	for (TActorIterator<ACyberneticCharacter>It(GetWorld()); It; ++It)
	{
		if (It->IsClosingDoor(const_cast<ADoor*>(this)))
		{
			return true;
		}
	}
	
	return IsOpening();
}

TArray<AStackUpActor*> ADoor::GetStackupsForArea(const EStackupGenArea StackupArea) const
{
	switch (StackupArea)
	{
		case EStackupGenArea::SGA_None:
		return {};

		case EStackupGenArea::SGA_FrontLeft:
		return FrontLeftStackUpPoints;

		case EStackupGenArea::SGA_FrontRight:
		return FrontRightStackUpPoints;

		case EStackupGenArea::SGA_BackLeft:
		return BackLeftStackUpPoints;
		
		case EStackupGenArea::SGA_BackRight:
		return BackRightStackUpPoints;

		case EStackupGenArea::SGA_All:
		{
			TArray<AStackUpActor*> StackUpActors;
			StackUpActors.Append(FrontLeftStackUpPoints);
			StackUpActors.Append(FrontRightStackUpPoints);
			StackUpActors.Append(BackLeftStackUpPoints);
			StackUpActors.Append(BackRightStackUpPoints);
			return StackUpActors;
		}

		default:
		break;
	}

	return {};
}

FName ADoor::GetFrontThreatOwningRoom() const
{
	#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(FrontThreat, TEXT("Door '%s' has no front threat awareness actor!"), *GetNameSafe(this));
	#endif

	if (FrontThreat)
	{
		return FrontThreat->OwningRoom;
	}

	return NAME_None;
}

FName ADoor::GetBackThreatOwningRoom() const
{
	#if !UE_BUILD_SHIPPING
	ensureMsgf(BackThreat, TEXT("Door '%s' has no back threat awareness actor!"), *GetNameSafe(this));
	#endif

	if (BackThreat)
	{
		return BackThreat->OwningRoom;
	}

	return NAME_None;
}

FVector ADoor::GetDoorMidLocation() const
{
	return Doorway->GetComponentLocation();
}

FVector ADoor::GetWedgeLocation() const
{
	if (GetSubDoor())
	{
		return GetDoorMidLocation() + GetActorRightVector() * 65.0f;
	}

	return IsValid(WedgeInteractableComponent) ? WedgeInteractableComponent->GetComponentLocation() : GetActorLocation();
}

FVector ADoor::GetBestDoorInteraction_FromLocation(const FVector& InInteractionLocation, bool bDoorwayBased) const
{
	return GetBestDoorInteraction_FromStackUpArea(FindStackUpAreaFromLocation(InInteractionLocation), bDoorwayBased);
}

FVector ADoor::GetBestDoorInteraction_FromStackUpArea(const EStackupGenArea& InStackUpArea, bool bDoorwayBased) const
{
	float MaxInteractionLength = 50.0f;
	
	FHitResult Hit;
	FVector StartTrace = bDoorwayBased ? GetActorLocation() :  GetWedgeHighlight()->GetComponentLocation();
	if (InStackUpArea == EStackupGenArea::SGA_FrontLeft || InStackUpArea == EStackupGenArea::SGA_FrontRight)
	{
		StartTrace += GetActorForwardVector() * 50.0f;
	}
	else
	{
		StartTrace += GetActorForwardVector() * -50.0f;
	}
	
	FVector EndTrace = StartTrace;
	if (InStackUpArea == EStackupGenArea::SGA_BackLeft || InStackUpArea == EStackupGenArea::SGA_FrontLeft)
	{
		EndTrace += GetActorRightVector() * -MaxInteractionLength;
	}
	else if (InStackUpArea == EStackupGenArea::SGA_BackRight || InStackUpArea == EStackupGenArea::SGA_FrontRight)
	{
		EndTrace += GetActorRightVector() * (MaxInteractionLength + 100.0f) ;
	}
	
	GetWorld()->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, FCollisionObjectQueryParams(ECC_WorldStatic));
	if (Hit.bBlockingHit)
	{
		MaxInteractionLength = Hit.Distance;
	}

	#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), StartTrace, EndTrace, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 0.1f, 0 , 1);
	#endif
	
	if (bDoorwayBased)
	{
		return Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
	}
	
	switch (InStackUpArea)
	{
		case EStackupGenArea::SGA_FrontLeft:
		if (IsOpen_Backward()) // Is open towards us?
			return GetWedgeHighlight()->GetComponentLocation() - GetActorRightVector() * MaxInteractionLength;
	
		return GetActorLocation() - GetActorRightVector() * MaxInteractionLength + GetActorForwardVector() * 50.0f;

		case EStackupGenArea::SGA_FrontRight:
		return GetWedgeHighlight()->GetComponentLocation() + GetActorRightVector() * MaxInteractionLength + GetActorForwardVector() * 50.0f;

		case EStackupGenArea::SGA_BackLeft:
		if (IsOpen_Forward()) // Is open towards us?
			return GetWedgeHighlight()->GetComponentLocation() - GetActorRightVector() * MaxInteractionLength + GetActorForwardVector() * -50.0f;

		return GetActorLocation() - GetActorRightVector() * MaxInteractionLength;

		case EStackupGenArea::SGA_BackRight:
		return GetWedgeHighlight()->GetComponentLocation() + GetActorRightVector() * MaxInteractionLength + GetActorForwardVector() * -50.0f;

		default:
		return GetActorLocation();
	}
}

EStackupGenArea ADoor::FindStackUpAreaFromLocation(const FVector& InInteractionLocation) const
{
	if (InInteractionLocation == FVector::ZeroVector)
		return EStackupGenArea::SGA_All;
	
	const bool bIsOnFrontSideOfDoor = IsPointInFrontOfDoorway(InInteractionLocation);
	const bool bIsOnRightSideOfDoor = IsPointRightOfDoorway(InInteractionLocation);

	EStackupGenArea StackUpArea;
	if (bIsOnFrontSideOfDoor)
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
	}
	else
	{
		StackUpArea = bIsOnRightSideOfDoor ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
	}

	return StackUpArea;
}

void ADoor::FlipStackUpArea(EStackupGenArea& OutStackUpArea, const bool bHorizontalFlip, const bool bVerticalFlip)
{
	switch (OutStackUpArea)
	{
		case EStackupGenArea::SGA_FrontLeft:
			OutStackUpArea = EStackupGenArea::SGA_FrontRight;

		if (bVerticalFlip)
			OutStackUpArea = bHorizontalFlip ? EStackupGenArea::SGA_BackRight : EStackupGenArea::SGA_BackLeft;
		break;

		case EStackupGenArea::SGA_FrontRight:
			OutStackUpArea = EStackupGenArea::SGA_FrontLeft;
		
		if (bVerticalFlip)
			OutStackUpArea = bHorizontalFlip ? EStackupGenArea::SGA_BackLeft : EStackupGenArea::SGA_BackRight;
		break;
			
		case EStackupGenArea::SGA_BackLeft:
			OutStackUpArea = EStackupGenArea::SGA_BackRight;
		
		if (bVerticalFlip)
			OutStackUpArea = bHorizontalFlip ? EStackupGenArea::SGA_FrontRight : EStackupGenArea::SGA_FrontLeft;
		break;

		case EStackupGenArea::SGA_BackRight:
			OutStackUpArea = EStackupGenArea::SGA_BackLeft;
		
		if (bVerticalFlip)
			OutStackUpArea = bHorizontalFlip ? EStackupGenArea::SGA_FrontLeft : EStackupGenArea::SGA_FrontRight;
		break;
			
		default:
			break;
	}
}

void ADoor::SetAllElectronicLocks(UObject* WorldContextObject, bool bLocked)
{
	for (TActorIterator<ADoor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		if (Door && Door->IsElectronicDoor())
		{
			Door->SetLocked(bLocked);
		}
	}
}

void ADoor::SetSWATHasAllKeycards(UObject* WorldContextObject)
{
	for (TActorIterator<ADoor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		ADoor* Door = *It;
		if (Door)
		{
			Door->bSWATHasKeycard = true;
			Door->SetDoorLockKnowledge(false, false); // Hack to make "locked" door HUD stuff go away
		}
	}
}

FVector ADoor::CalculateClosestPoint(FVector Location) const
{
	const FBox Box{-Doorway->GetUnscaledBoxExtent(), Doorway->GetUnscaledBoxExtent()};

	// Transform the location into the local coordinate space of the box transform.
	const FVector LocalPoint = GetDoorway()->GetComponentTransform().InverseTransformPosition(Location);

	// Compare it against the box's axis-aligned bounds in local space.
	FVector LocalClosest = Box.GetClosestPointTo(LocalPoint);
	const float DoorSize = GetDoorSize().Y - 30.0f;
	LocalClosest.Y = FMath::Clamp(LocalClosest.Y, -DoorSize, DoorSize);
	
	// Finally, transform the local closest point back into world space.
	return GetDoorway()->GetComponentTransform().TransformPosition(LocalClosest);
}

void ADoor::PlayDoorKickSound(AReadyOrNotCharacter* Kicker, const float Result)
{
	FMODParam KickResult;
	KickResult.SetValues("kickResult", Result);
	
	Multicast_PlayDoorDamageSound(EDoorDamageType::DDT_Kicking, {KickResult});

	//UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 0.0f, KICK_DOOR_NOISE_TAG);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);
}

void ADoor::PlayDoorSound(const EDoorInteraction DoorInteraction, AReadyOrNotCharacter* DoorInteractionInstigator, const TArray<FMODParam>& Params)
{
	if (!FMODAudioPropagationComp)
		return;
	
	if (bDoorBroken || AllMajorDoorChunksDestroyed() || FMODAudioPropagationComp->IsPlaying())
		return;

	#if !UE_BUILD_SHIPPING
	for (const FMODParam& Param : Params)
	{
		FString DoorInteractionAsString = ENUM_TO_STRING(EDoorInteraction, DoorInteraction, false);
		ULog::Info(DoorInteractionAsString + " | " + Param.paramName.ToString() + ": " + FString::SanitizeFloat(Param.paramVal));
	}
	#endif
	
	switch (DoorInteraction)
	{
		case EDoorInteraction::None:
		break;
		
		case EDoorInteraction::Open:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), IsOpenBy_Angle(2.0f) ? DoorData.OpenSound : DoorData.ManipulateSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 1250.0f, MOVE_DOOR_NOISE_TAG);
			break;
		}
	
		case EDoorInteraction::Close:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.CloseSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}
	
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 500.0f, MOVE_DOOR_NOISE_TAG);
			break;
		}
	
		case EDoorInteraction::Peek:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), LastTargetAngle == 0.0f ? DoorData.PushCloseSound : DoorData.PushOpenSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}
		}
		break;
		
		case EDoorInteraction::Push:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), LastTargetAngle == 0.0f ? DoorData.PushCloseSound : DoorData.PushOpenSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}
		}
		break;

		case EDoorInteraction::Kick:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.KickSuccessSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}

			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);
		}
		break;
		
		case EDoorInteraction::Kick_Fail:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.KickFailSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}

			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);
		}
		break;
		
		case EDoorInteraction::Ram:
		{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.RamSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				if(DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}

			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 1300.0f, RAM_DOOR_NOISE_TAG);
		}
		break;

		default:
		break;
	}
}

void ADoor::PlayDoorDamageSound(const EDoorDamageType DoorDamage, const TArray<FMODParam>& Params)
{
	switch (DoorDamage)
	{
		case EDoorDamageType::DDT_Blasting:
            //if (DoorData.DestructionSound)
            //{
            //	UFMODBlueprintStatics::PlayEventAtLocation(this, DoorData.DestructionSound, GetTransform(), true);
            //	FMODAudioPropagationComp->PlayEvent(DoorData.DestructionSound, GetActorLocation(), Params);
            //}
		break;

		case EDoorDamageType::DDT_Ramming:
            if (DoorData.RamSound)
            {
            	USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.RamSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
            	if(DoorSoundSource)
            	{
            		DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
            		DoorSoundSource->bIsDoorAttachedSound = true;
            		DoorSoundSource->Attach(GetRootComponent(), "");
            		DoorSoundSource->Play();
            	}
            }
		break;

		case EDoorDamageType::DDT_Shotgunning:
            //if (DoorData.DestructionSound)
            //{
            //	UFMODBlueprintStatics::PlayEventAtLocation(this, DoorData.DestructionSound, GetTransform(), true);
            //	FMODAudioPropagationComp->PlayEvent(DoorData.DestructionSound, GetActorLocation(), Params);
            //}
		break;

		case EDoorDamageType::DDT_Kicking:
            if (DoorData.KickSuccessSound)
            {
            	USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.KickSuccessSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, true);
            	if(DoorSoundSource)
            	{
            		DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
            		DoorSoundSource->bIsDoorAttachedSound = true;
            		DoorSoundSource->Attach(GetRootComponent(), "");
            		DoorSoundSource->Play();
            	}
            }
		break;

		case EDoorDamageType::DDT_Bash:
			if (DoorData.BashSound)
			{
				USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.BashSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, true);
				if (DoorSoundSource)
				{
					DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
					DoorSoundSource->bIsDoorAttachedSound = true;
					DoorSoundSource->Attach(GetRootComponent(), "");
					DoorSoundSource->Play();
				}
			}
		break;

		default:
        break;
	}

	if (DoorDamage != EDoorDamageType::DDT_None)
		GetWorld()->GetTimerManager().SetTimer(ReactToDoorDamagedHandle, this, &ADoor::AIResponseToDoorDamage, ReactToDoorDamagedDelay, false);
}

void ADoor::PlayLockedSound()
{
	USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), DoorData.LockedSound, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
	if(DoorSoundSource)
	{
		DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
		DoorSoundSource->bIsDoorAttachedSound = true;
		DoorSoundSource->Attach(GetRootComponent(), "");
		DoorSoundSource->Play();
	}

	if (bIsElectronicDoor)
		Multicast_PlayElectronicDoorSound(DoorData.KeycardDenySound);

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 500.0f, MOVE_DOOR_NOISE_TAG);
}

void ADoor::AIResponseToDoorDamage()
{
	float ClosestDist = 1000.0f;
	ACyberneticCharacter* ClosestSpeaker = nullptr;
	for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (AI && AI->IsActive() && AI->IsSuspect())
		{
			const float TestDistance = (AI->GetActorLocation() - GetActorLocation()).Size();
			if (TestDistance < ClosestDist)
			{
				ClosestDist = TestDistance;
				ClosestSpeaker = AI;
			}
		}
	}
	
	if (ClosestSpeaker)
	{
		ClosestSpeaker->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_TRIGGER_ACTIVATED, true);
	}
}

void ADoor::SetLocked(const bool bNewLocked)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (IsOpen() || FMath::Abs(OpenCloseAmount) > 0.0f)
	{
		UnlockDoor();
		return;
	}
	
	if (!IsLockable() || bDoorBroken)
	{
		UnlockDoor();
	}
	else
	{
		if (bLocked != bNewLocked)
		{
			bLocked = bNewLocked;
			Multicast_SetLocked(bNewLocked);
		
			if (DriveSubDoor)
			{
				DriveSubDoor->bLocked = bNewLocked;
				DriveSubDoor->Multicast_SetLocked(bNewLocked);
			}
		}
	}

	ResetDoorLockKnowledge();
}

void ADoor::OpenSubDoor(AReadyOrNotCharacter* DoorOpenCharacter, const bool bInstant, const bool bAnimateDoorHandle)
{
	if (DriveSubDoor && !DriveSubDoor->IsOpen() && !DriveSubDoor->IsOpening())
	{
		DriveSubDoor->OpenDoor(DoorOpenCharacter, bInstant, bAnimateDoorHandle);
	}
}

void ADoor::CloseSubDoor(AReadyOrNotCharacter* DoorCloseCharacter, bool bInstant, bool bAnimateDoorHandle)
{
	if (DriveSubDoor && !DriveSubDoor->IsClosed() && !DriveSubDoor->IsClosing())
	{
		DriveSubDoor->CloseDoor(DoorCloseCharacter, bInstant, bAnimateDoorHandle);
	}
}

bool ADoor::IsSubDoorOpen()
{
	return DriveSubDoor && DriveSubDoor->IsOpen();
}

bool ADoor::CanKickDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;
	
	if (!PlayerCharacter)
		return IsOpenBy(0.5f) && !IsMiddleChunkBroken() && !bDoorJammed && !IsAnyInteractionPlayingExcept(TL_DoorPush);

	const bool bCanBreakOff_Forward = DoorData.bCanBreakOffOneWayDoorWithKick ? IsActorInFrontOfDoor(PlayerCharacter) && DoorData.bCanBreakOffOneWayDoorWithKick : !IsActorInFrontOfDoor(PlayerCharacter);
	const bool bCanBreakOff_Backward = DoorData.bCanBreakOffOneWayDoorWithKick ? IsActorInFrontOfDoor(PlayerCharacter) && DoorData.bCanBreakOffOneWayDoorWithKick : IsActorInFrontOfDoor(PlayerCharacter);
	const bool bOneWayCheck = (!bOneWay || (bOneWay && (OneWayDirection == EDoorOneWayDirection::Forward && bCanBreakOff_Forward) || (OneWayDirection == EDoorOneWayDirection::Backward && bCanBreakOff_Backward)));

	const bool bDisallowedItemEquipped = PlayerCharacter->GetEquippedItem() && PlayerCharacter->GetEquippedItem()->bDisallowKicking;
	const bool bIsRightLegBroken = PlayerCharacter->IsLimbBroken(ELimbType::LT_RightLeg);

	bool bPlayerOnlyChecks = !bDisallowedItemEquipped && !bIsRightLegBroken;
	if (Cast<ACyberneticCharacter>(PlayerCharacter))
		bPlayerOnlyChecks = true;
	
	return IsOpenBy(0.5f) && !IsMiddleChunkBroken() && !bDoorJammed && !IsAnyInteractionPlayingExcept(TL_DoorPush) && bOneWayCheck && !PlayerCharacter->IsCrouching() && bPlayerOnlyChecks;
}

bool ADoor::CanOpenDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;

	if (!PlayerCharacter)
		return !IsMiddleChunkBroken() && !bDoorJammed;

	const bool bTeamKnowsLockState = TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());

	const bool bBypassLockCheck = PlayerCharacter->IsSuspect() && bSuspectAlwaysUnlocks;
	
	const bool bLockCheck = bBypassLockCheck || (!bTeamKnowsLockState || (bTeamKnowsLockState && !IsLocked()));

	const bool bChunkCheck = Cast<ACyberneticCharacter>(PlayerCharacter) ? true : !IsMiddleChunkBroken();
	
	return !bDoorJammed && bChunkCheck && bLockCheck && !IsNonDoorInteractionPlaying();
}

bool ADoor::CanCloseDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;
	
	if (!PlayerCharacter)
		return false;
	
	const bool bTeamKnowsLockState = TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());

	if (bDoorJammed || IsMiddleChunkBroken() || (bTeamKnowsLockState && IsLocked()) || IsNonDoorInteractionPlaying())
		return false;

	if (IsActorInFrontOfDoor(PlayerCharacter))
	{
		if (IsOpen_Forward())
		{
			if (IsOpenBeyondCloseThreshold())
			{
				return true;
			}
		}
		else
		{
			if (IsOpen())
			{
				return true;
			}
		}
	}
	else
	{
		if (IsOpen_Backward())
		{
			if (IsOpenBeyondCloseThreshold())
			{
				return true;
			}
		}
		else
		{
			if (IsOpen())
			{
				return true;
			}
		}
	}

	return false;
}

bool ADoor::CanMirrorUnderDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Optiwand) && !bDoorBroken && !IsOpenBeyond_Angle(IncrementAngle) && DoorData.bCanMirrorUnderDoor;
}

bool ADoor::CanEquipMultitool(AReadyOrNotCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return !bDoorBroken && IsLocked();

	if (const AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
	{
		return Multitool->CurrentToolKit != EMultitoolFunctions::MF_Lockpick && !bDoorBroken && IsLocked() && TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());
	}

	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Multitool) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) && !bDoorBroken && IsLocked() && TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());
}

bool ADoor::CanEquipC2Explosive(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_C2Explosive) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_C2Explosive) && !bC2Placed && !bDoorBroken && IsClosed() && !IsAnyInteractionPlaying();
}

bool ADoor::CanEquipWedge(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Doorjam) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Doorjam) && !bDoorJammed && !bDoorBroken && !IsOpen() && !IsAnyInteractionPlaying();
}

bool ADoor::CanEquipOptiwand(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Optiwand) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Optiwand) && !bDoorBroken && !IsOpenBeyond_Angle(IncrementAngle) && DoorData.bCanMirrorUnderDoor && !IsAnyInteractionPlaying();
}

bool ADoor::CanEquipBreachingShotgun(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_BreachingShotgun) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_BreachingShotgun) && !bDoorBroken && DoorData.bIsDestructible && !IsAnyInteractionPlaying() && !IsOpen();
}

bool ADoor::CanEquipBatteringRam(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_BatteringRam) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_BatteringRam) && !bDoorBroken && !IsAnyInteractionPlaying() && !IsOpen();
}

bool ADoor::CanRamDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_BatteringRam) && !bDoorBroken;
}

bool ADoor::CanLockpickDoor(AReadyOrNotCharacter* PlayerCharacter) const
{
	return (UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) || UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_LockpickGun)) && !bDoorBroken && !bIsElectronicDoor && IsLocked() && TeamKnowsDoorLockState(PlayerCharacter->IsSuspect());
}

bool ADoor::CanPlaceC2Explosive(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_C2Explosive) && !bC2Placed && !bDoorBroken && !IsOpen();
}

bool ADoor::CanDeployWedge(AReadyOrNotCharacter* PlayerCharacter) const
{
	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Doorjam) && !bDoorJammed && !bDoorBroken;
}

bool ADoor::CanTakeDamage(const float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	// Only server can do damage
	if (GetLocalRole() < ROLE_Authority)
		return false;

	if (Damage <= 0.0f || !DamageEvent.DamageTypeClass || !DamageCauser)
		return false;

	return true;
}

float ADoor::TakeDamage(const float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	SCOPE_CYCLE_COUNTER(STAT_DoorTakeDamage);

	#pragma region Damage Checks
	if (!CanTakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser))
	{
		#if WITH_EDITOR 
		if (HasAuthority())
		{
			ULog::Warning("Cannot apply damage to " + GetName());
		}
		#endif 
		
		return 0.0f;
	}
	#pragma endregion

	#pragma region 2.Initialization Logic
	float FinalDamage = DamageAmount;
	
	AReadyOrNotCharacter* InstigatorCharacter = EventInstigator ? Cast<AReadyOrNotCharacter>(EventInstigator->GetPawn()) : nullptr;

	FHitResult PointHit;
	FVector Impulse;
	DamageEvent.GetBestHitInfo(this, DamageCauser, PointHit, Impulse);

	UStunDamage* StunDamage = Cast<UStunDamage>(DamageEvent.DamageTypeClass->GetDefaultObject());
	if (StunDamage)
	{
		FinalDamage *= StunDamage->DoorDamageMultiplier;
	}

	if (ABaseMagazineWeapon* BMW = Cast<ABaseMagazineWeapon>(DamageCauser))
	{
		if (BMW->ItemCategories.Contains(EItemCategory::IC_BreachingShotgun))
		{
			FinalDamage *= 10.0f;
		}

		// Trigger an alarm when applicable through threshold
		if (FinalDamage > 10.0f)
		{
			TriggerAlarm(InstigatorCharacter);
		}
	}
	#pragma endregion

	#pragma region 3.Damage Logic
	if (APlacedC2Explosive* C2 = Cast<APlacedC2Explosive>(DamageCauser))
	{
		ExplodeDoor(InstigatorCharacter, C2, true);

		return DamageAmount;
	}

	if (UTrapDamage* TrapDamage = Cast<UTrapDamage>(DamageEvent.DamageTypeClass->GetDefaultObject()))
	{
		if (TrapDamage->bDestroyAllDoorChunks)
		{
			OpenCloseAmount = DamageCauser ? (IsActorInFrontOfDoor(DamageCauser) ? MaxOpenClose : -MaxOpenClose) : MaxOpenClose;

			if (IsDestructible())
			{
				if (DamageCauser)
					DestroyAllChunks(IsActorInFrontOfDoor(DamageCauser) ? -DoorStatic->GetForwardVector() : DoorStatic->GetForwardVector(), 300.0f, true);
				else
					DestroyAllChunks(GetActorForwardVector(), 300.0f, true);
			}
			else
			{

				BreakDoor(false, nullptr);
				const bool bCauserInFront = IsActorInFrontOfDoor(DamageCauser);
				DoorStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				DoorStatic->SetWorldLocation(DoorStatic->GetComponentLocation() + GetActorForwardVector() * (bCauserInFront ? -70.0f : 70.0f));
				DoorStatic->SetSimulatePhysics(true);
				DoorStatic->SetMassOverrideInKg(NAME_None, 100.0f);
				DoorStatic->SetLinearDamping(0.05f);
				DoorStatic->SetCollisionObjectType(ECC_PhysicsBody);
				DoorStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				DoorStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
				DoorStatic->AddImpulse((bCauserInFront ? -GetActorForwardVector() : GetActorForwardVector()) * 15500.0f);
				DoorStatic->SetCanEverAffectNavigation(false);
				DoorStatic->SetWorldScale3D(FVector(0.92f));

				LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
				LastTargetAngle = bCauserInFront ? MaxOpenClose : -MaxOpenClose;
				OpenCloseAmount = LastTargetAngle;

				LastDoorDamage = EDoorDamageType::DDT_Blasting;

				DistanceMovedThisFrame = 0.0f;
				AccumulatedDistance = 0.0f;
				TimeMovedThisFrame = 0.0f;
				AccumulatedTime = 0.0f;

				OnDoorExploded.Broadcast(this, nullptr);
			}
		}

		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	// Only explosive projectiles allowed to destroy doors
	if (Cast<AGrenadeProjectile>(DamageCauser) && (!StunDamage || (StunDamage && StunDamage->StunType == EStunType::ST_None)))
	{
		CollapseDoor(InstigatorCharacter, DamageCauser->GetActorLocation());
		ExplodeDoor(InstigatorCharacter, DamageCauser, false);
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	AReadyOrNotCharacter* DamageCauserCharacter = Cast<AReadyOrNotCharacter>(DamageCauser);
	if (!InstigatorCharacter)
		InstigatorCharacter = DamageCauserCharacter;
	
	if (Cast<ADoorRam>(DamageCauser))
	{
		LastDoorDamage = EDoorDamageType::DDT_Ramming;

		if (IsDestructible())
		{
			if (UDestructibleDoorChunkComponent* Chunk = Cast<UDestructibleDoorChunkComponent>(PointHit.GetComponent()))
			{
				RamDoor(InstigatorCharacter);

				DestroyChunk(Chunk, IsPointInFrontOfDoor(PointHit.ImpactPoint) ? -GetActorForwardVector() : GetActorForwardVector());

				BreakDoor(false, InstigatorCharacter);
			}
		}
		else
		{
			RamDoor(InstigatorCharacter);
		}
		
		return FinalDamage;
	}

	if (DamageEvent.GetTypeID() == FRadialDamageEvent::ClassID)
	{
		FRadialDamageEvent* RadialDamage = (FRadialDamageEvent*)&DamageEvent;
		for (FHitResult hit : RadialDamage->ComponentHits)
		{
			if (UDestructibleDoorChunkComponent* Chunk = Cast<UDestructibleDoorChunkComponent>(hit.GetComponent()))
			{
				Chunk->DoDamage(FinalDamage);
				// Check damage, CS Gas does <1.0f damage so don't provide impulse
				if (!Cast<ABaseGrenade>(DamageCauser))
				{
					Chunk->AddRadialImpulse(RadialDamage->Origin, 5000.0f, -10000.0f, RIF_Constant);
				}
			}
		}
		
		if (StunDamage && StunDamage->bCanPushDoorWithForce)
		{
			if (IsPointInFrontOfDoor(RadialDamage->Origin))
			{
				OpenDoor_SpecificAngle(InstigatorCharacter, MaxOpenClose * StunDamage->DoorPushScale);
			}
			else
			{
				OpenDoor_SpecificAngle(InstigatorCharacter, -MaxOpenClose * StunDamage->DoorPushScale);
			}
		}
	}

	if (StunDamage && StunDamage->bDamageAllDoorPiecesAtOnce)
	{
		DestroyAllChunks(IsPointInFrontOfDoor(PointHit.ImpactPoint) ? - GetActorForwardVector() : GetActorForwardVector());
	}

	//const bool bHitLock = FVector::Distance(PointHit.Location, LockpickArea->GetComponentLocation()) < 30.0f;
	//const bool bCanMoveDoor = IsDestructible() ? (bHitLock || DoorChunk5->IsDestroyed()) : bHitLock; 
	float BreachPushAngle = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 1000.0f), FVector2D(45.0f, 15.0f), FVector::Distance(PointHit.Location, DamageCauser->GetActorLocation()));
	if (IsDestructible())
	{
		if (UDestructibleDoorChunkComponent* Chunk = Cast<UDestructibleDoorChunkComponent>(PointHit.GetComponent()))
		{
			// Is handle chunk destroyed?
			//if (DoorChunk5->IsDestroyed())
			//{
			//	BreachPushAngle = 25.0f;
			//}
			
			if (/*ABreachingShotgun* BreachShotgun = */Cast<ABreachingShotgun>(DamageCauser))
			{
				DestroyChunk(Chunk, IsPointInFrontOfDoor(PointHit.ImpactPoint) ? -GetActorForwardVector() : GetActorForwardVector());
				
				// if (Chunk->IsHinge())
				// {
				// 	Multicast_CheckSupports();
				// 	Multicast_CheckSupports_Implementation();
				// }
				// else
				// {
				// 	//if (bCanMoveDoor)
				// 		BreachDoor(Cast<AReadyOrNotCharacter>(DamageCauser->GetOwner()), BreachPushAngle);
				// }

				Multicast_CheckSupports();
				Multicast_CheckSupports_Implementation();
				if(!Chunk->IsHinge())
				{
					BreachDoor(Cast<AReadyOrNotCharacter>(DamageCauser->GetOwner()), BreachPushAngle);
				}
			}
			else
			{
				// if (Chunk->IsHinge())
				// {
				// 	Multicast_CheckSupports();
				// 	Multicast_CheckSupports_Implementation();
				// }

				Multicast_CheckSupports();
				Multicast_CheckSupports_Implementation();

				// If damage causer is a character, do 1.0 damage, they shouldnt break door chunks
				Chunk->DoDamage(DamageCauserCharacter ? 1.0f : FinalDamage);
			}
		}
	}
	else
	{
		if (/*ABreachingShotgun* BreachShotgun = */Cast<ABreachingShotgun>(DamageCauser))
		{
			//if (bCanMoveDoor)
				BreachDoor(Cast<AReadyOrNotCharacter>(DamageCauser->GetOwner()), BreachPushAngle);
		}
	}

	if (InstigatorCharacter && InstigatorCharacter->IsInRagdoll() && InstigatorCharacter->GetVelocity().Size() > 50.0f)
	{
		if (!IsLocked() && !IsOpenBeyondCloseThreshold())
		{
			RamDoor(InstigatorCharacter);
		}
	}

	if (AnyChunksDestroyed())
	{
		BreakDoor(false);
	}
	#pragma endregion
	
	return Super::TakeDamage(FinalDamage, DamageEvent, EventInstigator, DamageCauser);
}

void ADoor::Init()
{
	DestroyNavLink();

	Setup();
	EnableNavLink();
	
	DeactivateDoorBlocker();

	if (bSuspectAlwaysUnlocks)
	{
		Server_SetLockKnowledgeState(true, true);
	}
}

void ADoor::PlayLockedAnimation()
{
	PlayTimelines({&TL_DoorHandleLocked, &TL_DoorLocked});
}

void ADoor::PlayHandleAnimation()
{
	PlayTimeline(TL_DoorHandleOpen, false, false);
}

void ADoor::DestroyChunk_Index(const int32 ChunkIndex, const FVector& Impulse, const float ImpulseStrength)
{
	if (ChunkComponents.IsValidIndex(ChunkIndex))
	{
		DestroyChunk(ChunkComponents[ChunkIndex], Impulse, ImpulseStrength);
	}
}

void ADoor::DestroyChunk(UDestructibleDoorChunkComponent* InChunk, const FVector& Impulse, float ImpulseStrength)
{
	if (InChunk && !InChunk->IsDestroyed())
	{
		InChunk->SetCanEverAffectNavigation(false);
		
		InChunk->OnChunkDestroyed();
		InChunk->AddImpulse(Impulse * FMath::Abs(ImpulseStrength * FMath::FRandRange(1.0f, 10.0f)));
	}
}

void ADoor::DestroyAllChunks(const FVector& Impulse, const float ImpulseStrength, const bool bKeepHinges)
{
	/*const bool bBottomRowChunks = Chunk == DoorChunk0 || Chunk == DoorChunk1 || Chunk == DoorChunk2;
	const bool bMidRowChunks = Chunk == DoorChunk3 || Chunk == DoorChunk4 || Chunk == DoorChunk5;
	const bool bTopRowChunks = Chunk == DoorChunk6 || Chunk == DoorChunk7 || Chunk == DoorChunk8;

	const float ZOffset = bBottomRowChunks ? 60.0f : bMidRowChunks ? 120.0f : bTopRowChunks ? 180 : 0.0f;
	*/
		
	for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
	{
		if (!IsValid(Chunk))
			continue;
		
		Chunk->SetCanEverAffectNavigation(false);
		
		if (bKeepHinges)
		{
			if (Chunk == DoorChunk0 && Chunk == DoorChunk3 && Chunk == DoorChunk6)
			{
				Chunk->SetGenerateOverlapEvents(false);
				Chunk->SetNotifyRigidBodyCollision(false);
				Chunk->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			}
		}
		
		if (!bKeepHinges || (bKeepHinges && Chunk != DoorChunk0 && Chunk != DoorChunk3 && Chunk != DoorChunk6))
		{
			DestroyChunk(Chunk, Impulse, ImpulseStrength);
		}
	}

	Multicast_CheckSupports();
    Multicast_CheckSupports_Implementation();
}

void ADoor::StopAllTimelines(const bool bStopTimelinesIfAlreadyPlaying)
{
	StopTimeline(TL_DoorOpenClose, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorPush, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorLocked, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorRam, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorBreach, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorExplode, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorHandleOpen, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorHandlePush, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorHandleLocked, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorKickSuccess, bStopTimelinesIfAlreadyPlaying);
	StopTimeline(TL_DoorKickFail, bStopTimelinesIfAlreadyPlaying);
}

void ADoor::PlayTimeline(FTimeline& Timeline, const bool bRestartIfAlreadyPlaying, const bool bStopAllTimelines, const bool bStopTimelinesIfAlreadyPlaying)
{
	if (bStopAllTimelines)
		StopAllTimelines(bStopTimelinesIfAlreadyPlaying);

	if (bRestartIfAlreadyPlaying)
	{
		Timeline.PlayFromStart();
	}
	else if (!Timeline.IsPlaying())
	{
		Timeline.PlayFromStart();
	}
}

void ADoor::PlayTimelines(TArray<FTimeline*> Timelines, const TArray<bool> bRestartIfAlreadyPlaying, const bool bStopAllTimelines, const bool bStopTimelinesIfAlreadyPlaying)
{
	if (bStopAllTimelines)
		StopAllTimelines(bStopTimelinesIfAlreadyPlaying);

	Timelines.Remove(nullptr);

	if (bRestartIfAlreadyPlaying.Num() == 0)
	{
		for (FTimeline* Timeline : Timelines)
		{
			if (!Timeline->IsPlaying())
			{
				Timeline->PlayFromStart();
			}
		}
	}
	else
	{
		int32 i = 0;

		for (FTimeline* Timeline : Timelines)
		{
			if (bRestartIfAlreadyPlaying.IsValidIndex(i) && bRestartIfAlreadyPlaying[i])
			{
				Timeline->PlayFromStart();
			}
			else if (!Timeline->IsPlaying())
			{
				Timeline->PlayFromStart();
			}

			i++;
		}
	}
}

void ADoor::StopTimeline(FTimeline& Timeline, const bool bStopIfAlreadyPlaying)
{
	if (bStopIfAlreadyPlaying)
	{
		Timeline.Stop();
	}
	else if (!Timeline.IsPlaying())
	{
		Timeline.Stop();
	}
}

void ADoor::SetDoorHasEverBeenOpenedBySwat()
{
	//if (!bHasEverBeenOpenedBySwat)
	{
		//V_LOGM(LogReadyOrNot, "(%s) Setting Door Has Ever Been Opened", *GetName());
		bHasEverBeenOpenedBySwat = true;
	}
}

void ADoor::DestroyAllInteractionComponents()
{	
	DESTROY_COMPONENT(DoorOpenInteractableComp)
	DESTROY_COMPONENT(DoorSublinkOpenInteractableComp)
	DESTROY_COMPONENT(DoorSublinkPushInteractableComp)
	DESTROY_COMPONENT(DoorSublinkKickInteractableComp)
	DESTROY_COMPONENT(DoorPushInteractableComp)
	DESTROY_COMPONENT(DoorKickInteractableComp)
	DESTROY_COMPONENT(LockpickInteractableComponent)
	DESTROY_COMPONENT(C2InteractableComponent)
	DESTROY_COMPONENT(OptiwandInteractableComponent)
	DESTROY_COMPONENT(WedgeInteractableComponent)
	DESTROY_COMPONENT(BSGInteractableComponent)
	DESTROY_COMPONENT(BSGInteractableComponent_2)
	DESTROY_COMPONENT(DoorRamInteractableComponent)
}

void ADoor::DestroyAllChunkComponents()
{
	DESTROY_COMPONENT(DoorChunk0)
	DESTROY_COMPONENT(DoorChunk1)
	DESTROY_COMPONENT(DoorChunk2)
	DESTROY_COMPONENT(DoorChunk3)
	DESTROY_COMPONENT(DoorChunk4)
	DESTROY_COMPONENT(DoorChunk5)
	DESTROY_COMPONENT(DoorChunk6)
	DESTROY_COMPONENT(DoorChunk7)
	DESTROY_COMPONENT(DoorChunk8)
}

FMODParam ADoor::MakeOcclusionParam() const
{
	if (!FMODAudioPropagationComp || IsDoorwayOnly())
		return {};
	
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		LOCAL_PLAYER;

		FHitResult Hit;
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(this);
		CollisionQueryParams.AddIgnoredActor(LocalPlayer);
		CollisionQueryParams.AddIgnoredComponents((TArray<UPrimitiveComponent*>)ChunkComponents);
		
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		GetWorld()->LineTraceSingleByChannel(Hit, FMODAudioPropagationComp->GetComponentLocation(), CameraLocation, ECC_Visibility, CollisionQueryParams);

		const float MinZ = FMath::Min(CameraLocation.Z, FMODAudioPropagationComp->GetComponentLocation().Z);
		const float MaxZ = FMath::Max(CameraLocation.Z, FMODAudioPropagationComp->GetComponentLocation().Z);
		const float ZHeightDifference = MaxZ - MinZ;

		if (ZHeightDifference > 150.0f)
		{
			return {"Occlusion", Hit.bBlockingHit ? FMath::GetMappedRangeValueClamped<float,float>(FVector2f(100.0f, 600.0f), FVector2f(0.85f, 1.0f), Hit.Distance) : FMath::GetMappedRangeValueClamped<float,float>(FVector2f(300.0f, 1500.0f), FVector2f(0.0f, 1.0f), GetDistanceToLocalPlayer())};
		}
	}

	return {"Occlusion", FMath::GetMappedRangeValueClamped<float,float>(FVector2f(300.0f, 1750.0f), FVector2f(0.0f, 1.0f), GetDistanceToLocalPlayer())};
}

float ADoor::GetDistanceToLocalPlayer() const
{
	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		return FVector::Distance(CameraLocation, FMODAudioPropagationComp->GetComponentLocation());
	}

	return 0.0f;
}

void ADoor::BreakDoorHandles()
{
	bDoorHandlesBroken = true;

	OnRep_DoorHandlesBroken();
}

void ADoor::AttachTrap(ATrapActorAttachedToDoor* NewTrap, const bool bAttachToDoor)
{
	AttachedTrap = NewTrap;

	ResetDoorTrapKnowledge();

	if (NewTrap && bAttachToDoor)
	{
		if (bMainSubDoor || (!bMainSubDoor && !DriveSubDoor))
		{
			NewTrap->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}

		if (bMainSubDoor && DriveSubDoor)
		{
			DriveSubDoor->AttachTrap(NewTrap);
		}
	}
}

void ADoor::AttachWedge(ADoorJam* NewWedge)
{
	AttachedWedge = NewWedge;
}

void ADoor::OnRep_DestroyedChunkIdxChanged()
{
	for (int32 i = 0; i < DestroyedChunkIdxs.Num(); i++)
	{
		const int32 ChunkIdx = DestroyedChunkIdxs[i];
		if (ChunkComponents.IsValidIndex(ChunkIdx) && ChunkComponents[ChunkIdx])
		{
			ChunkComponents[ChunkIdx]->OnChunkDestroyed();
		}
	}
}

void ADoor::Multicast_CheckSupports_Implementation()
{
	// determine if we have any hinges left if not break the door off
	// attach all the chunks to the first chunk that is not destroyed then simulate on it
	if (!AnyHingesLeft())
	{
		UDestructibleDoorChunkComponent* RootChunk = nullptr;
		for (UDestructibleDoorChunkComponent* chunk : ChunkComponents)
		{
			if (!chunk)
				continue;

			if (!chunk->IsDestroyed())
			{
				if (!RootChunk)
				{
					RootChunk = chunk;
				}
				else
				{
					chunk->AttachToComponent(RootChunk, FAttachmentTransformRules::KeepRelativeTransform);
				}
			}
		}

		BreakDoorHandles();
	}

	for (UDestructibleDoorChunkComponent* chunk : ChunkComponents)
	{
		if (chunk)
		{
			chunk->CheckSupportChunks();
		}
	}
}

void ADoor::EnableNavLink()
{
	if (IsDoorwayOnly())
	{
		return;
	}
	
	if (!NavLinkProxy)
	{
		FTransform SpawnTransform = Doorway->GetComponentTransform();
		SpawnTransform.SetScale3D(FVector::OneVector);
		NavLinkProxy = GetWorld()->SpawnActor<ANavLinkProxy>(ANavLinkProxy::StaticClass(), SpawnTransform);
	}

	if (NavLinkProxy)
	{
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavLinkProxy->SetOwner(this);
			NavLinkProxy->SetSmartLinkEnabled(true);
			NavLinkProxy->GetSmartLinkComp()->SetNavigationRelevancy(true);

			FTransform Transform = Doorway->GetComponentTransform();
			Transform.SetScale3D(FVector::OneVector);

			const FVector LinkStartPoint = NavLinkProxy->GetSmartLinkComp()->GetStartPoint();
			const FVector LinkEndPoint = NavLinkProxy->GetSmartLinkComp()->GetEndPoint();

			const FVector FrontDoorPointWorld = Transform.TransformPosition(FrontDoorPoint);
			const FVector BackDoorPointWorld = Transform.TransformPosition(BackDoorPoint);
			
			if (!LinkStartPoint.Equals(FrontDoorPointWorld, 0.1) || !LinkEndPoint.Equals(BackDoorPointWorld, 0.1))
			{
				NavLinkProxy->GetSmartLinkComp()->SetLinkData(FrontDoorPoint, BackDoorPoint, ENavLinkDirection::BothWays);
				NavSys->UpdateActorAndComponentsInNavOctree(*NavLinkProxy);
			}
		}
	}
}

void ADoor::SetSubDoor(ADoor* InDoor, bool bInMainDoor)
{
	DriveSubDoor = InDoor;
	bMainSubDoor = bInMainDoor;
}

void ADoor::TestCanPathBothSidesOfDoor()
{
	if (bSearchingPath)
		return;

	if (TimeUntilNextPathTest > 0.0f)
		return;
	
	if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(AReadyOrNotCharacter::StaticClass()->GetDefaultObject<AReadyOrNotCharacter>()->GetNavAgentPropertiesRef()))
		{
			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, UNavQuery_DoorTest::StaticClass());

			FVector BaseLocation = GetDoorMidLocation();
			BaseLocation.Z = GetActorLocation().Z;

			FVector Front = BaseLocation + GetActorForwardVector() * 50.0f;
			FVector Back = BaseLocation + GetActorForwardVector() * -50.0f;

			constexpr float NavDist = 100.0f;

			if (IsOpen())
			{
				if (IsOpen_Forward())
				{
					Back += GetActorForwardVector() * -NavDist;
				}
				else
				{
					Front += GetActorForwardVector() * NavDist;
				}
			}

			FTransform Transform = Doorway->GetComponentTransform();
			Transform.SetScale3D(FVector::OneVector);
			
			FNavLocation RelativeStart(Front);
			FNavLocation RelativeEnd(Back);

			// Making the extent thin in depth, so that it doesn't get projected too close to the door and giving incorrect path information when door is partially open but navmesh doesn't actually connect.
			// In this case, we move our point to project further into the room to find the navmesh.
			FVector Extent = FVector(20.0f, 100.0f, 100.0f);
			Extent = Doorway->GetComponentRotation().RotateVector(Extent).GetAbs();

			bool bProjectionSuccess = false;
			bProjectionSuccess = NavSys->ProjectPointToNavigation(Front, RelativeStart, Extent, NavData, QueryFilter);
			if (!bProjectionSuccess)
			{
				Front += GetActorForwardVector() * (NavDist * 2);
				NavSys->ProjectPointToNavigation(Front, RelativeStart, FVector(100.f), NavData, QueryFilter);
			}
			
			bProjectionSuccess = NavSys->ProjectPointToNavigation(Back, RelativeEnd, Extent, NavData, QueryFilter);
			if (!bProjectionSuccess)
			{
				Back += GetActorForwardVector() * -(NavDist * 2);
				NavSys->ProjectPointToNavigation(Back, RelativeEnd, FVector(100.0f), NavData, QueryFilter);
			}

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			
			if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
			{
				QueryParams.AddIgnoredActors((TArray<AActor*>)GS->AllDoors);
				QueryParams.AddIgnoredActors((TArray<AActor*>)GS->AllReadyOrNotCharacters);
				QueryParams.AddIgnoredActors((TArray<AActor*>)GS->AllItems);
				//QueryParams.AddIgnoredActors((TArray<AActor*>)GetWorld()->GetGameState<AReadyOrNotGameState>()->AllEvidenceActors);
			}
			
			bool bHit = GetWorld()->LineTraceTestByChannel(FVector(Front.X, Front.Y, GetDoorMidLocation().Z), FVector(Back.X, Back.Y, GetDoorMidLocation().Z), ECC_Visibility, QueryParams);
			if (bHit)
			{
				DisableNavLink();
				return;
			}

			// These are set here to be used in EnableNavLink to avoid having to do these calculations again
			FrontDoorPoint = Transform.InverseTransformPosition(RelativeStart.Location);
			BackDoorPoint = Transform.InverseTransformPosition(RelativeEnd.Location);

			#if !UE_BUILD_SHIPPING
			if (CVarRonDrawDoorPathTestPoints.GetValueOnAnyThread() > 0)
			{
				DrawDebugPoint(GetWorld(), RelativeStart, 10.0f, FColor::White, false, 1.0f, 0);
				DrawDebugPoint(GetWorld(), RelativeEnd, 10.0f, FColor::White, false, 1.0f, 0);
			}
			#endif

			FNavPathQueryDelegate NavDelegate;
			NavDelegate.BindUObject(this, &ADoor::OnTestCanPathBothSidesOfDoor);

			FPathFindingQuery PathFindingQuery;
			PathFindingQuery.QueryFilter = QueryFilter;
			PathFindingQuery.NavData = NavData;
			PathFindingQuery.StartLocation = RelativeStart.Location;
			PathFindingQuery.EndLocation = RelativeEnd.Location;
			PathFindingQuery.SetAllowPartialPaths(false);
			TimeUntilNextPathTest = 0.25f;
			bSearchingPath = true;
			NavSys->FindPathAsync(AReadyOrNotCharacter::StaticClass()->GetDefaultObject<AReadyOrNotCharacter>()->GetNavAgentPropertiesRef(), PathFindingQuery, NavDelegate, EPathFindingMode::Regular);
		}
	}
}

void ADoor::OnTestCanPathBothSidesOfDoor(uint32 PathId, const ENavigationQueryResult::Type ResultType, const FNavPathSharedPtr NavPath)
{
	bSearchingPath = false;
	
	if (ResultType == ENavigationQueryResult::Success)
	{
		if (NavPath.Get()->ContainsAnyCustomLink() || NavPath.Get()->GetLength() > 500.0f)
		{
			EnableNavLink();
		}
		else
		{
			DisableNavLink();
		}
	}
	else
	{
		bForceClosedDoorNavArea = false;
		EnableNavLink();
	}
}

void ADoor::DisableNavLink()
{
	if (IsDoorwayOnly())
		return;
	
	if (NavLinkProxy)
	{
		if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			if (NavLinkProxy->GetSmartLinkComp()->IsEnabled())
			{
				NavLinkProxy->GetSmartLinkComp()->SetDisabledArea(UNavArea_Default::StaticClass());
				NavLinkProxy->GetSmartLinkComp()->SetNavigationRelevancy(false);
				NavLinkProxy->GetSmartLinkComp()->SetEnabled(false);
				
				NavSys->UpdateActorInNavOctree(*NavLinkProxy);
				NavSys->UpdateComponentInNavOctree(*NavLinkProxy->GetSmartLinkComp());
			}
		}
	}
}

void ADoor::UpdateNavLinkLocations()
{
	if (!NavLinkProxy)
		return;
	
	if (const UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(AReadyOrNotCharacter::StaticClass()->GetDefaultObject<AReadyOrNotCharacter>()->GetNavAgentPropertiesRef()))
		{
			const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();

			float NavDist = (IsOpen() ? 140.0f : 70.0f);
			const FVector FrontMirrorLocation = Doorway->GetComponentLocation() + GetActorForwardVector() * NavDist + FVector(0.0f, 0.0f, -80.0f);
			const FVector BackMirrorLocation = Doorway->GetComponentLocation() + GetActorForwardVector() * -NavDist + FVector(0.0f, 0.0f, -80.0f);
			FNavLocation RelativeStart(FrontMirrorLocation);
			FNavLocation RelativeEnd(BackMirrorLocation);

			const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass);
			const FVector Extent = FVector(100.0f, 100.0f, 50.0f);

			NavSys->ProjectPointToNavigation(FrontMirrorLocation, RelativeStart, Extent, NavData, QueryFilter);
			NavSys->ProjectPointToNavigation(BackMirrorLocation, RelativeEnd, Extent, NavData, QueryFilter);

			#if !UE_BUILD_SHIPPING
			if (CVarRonDrawDoorPathTestPoints.GetValueOnAnyThread() > 0)
			{
				DrawDebugPoint(GetWorld(), RelativeStart.Location, 10.0f, FColor::Red, false, 1.0f, 0);
				DrawDebugPoint(GetWorld(), RelativeEnd.Location, 10.0f, FColor::Red, false, 1.0f, 0);
			}
			#endif

			NavLinkProxy->GetSmartLinkComp()->SetLinkData(RelativeStart, RelativeEnd, ENavLinkDirection::BothWays);
			NavSys->UpdateActorInNavOctree(*NavLinkProxy);
			NavSys->UpdateComponentInNavOctree(*NavLinkProxy->GetSmartLinkComp());
		}
	}
}

void ADoor::DestroyNavLink()
{
	if (NavLinkProxy)
	{
		NavLinkProxy->Destroy();
		NavLinkProxy = nullptr;
	}
}

void ADoor::DisableAllInteractables()
{
	TInlineComponentArray<UInteractableComponent*> InteractableComponents(this, true);
	GetComponents(InteractableComponents, true);

	for (UInteractableComponent* InteractableComponent : InteractableComponents)
	{
		InteractableComponent->DisableInteractable();
	}
}

void ADoor::Multicast_DisableDoorInteraction_Implementation(bool bSetClosed)
{
	DisableAllInteractables();
	bCanPlayerInteract = false;

	if (!bSetClosed)
		return;

	StopAllTimelines(true);
	OpenCloseAmount = 0.0f;
}

bool ADoor::IsOutlineEnabled(EActorOutlineType OutlineType) const
{
	if (IsDestructible())
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (Chunk)
			{
				if (Chunk->bRenderCustomDepth && Chunk->CustomDepthStencilValue == int32(OutlineType))
					return true;
			}
		}
	}

	return DoorStatic->bRenderCustomDepth && DoorStatic->CustomDepthStencilValue == int32(OutlineType);
}

bool ADoor::IsOutlineDisabled() const
{
	if (IsDestructible())
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (Chunk)
			{
				if (Chunk->bRenderCustomDepth)
					return true;
			}
		}
	}
	
	return DoorStatic->bRenderCustomDepth;
}

void ADoor::OpenDoorFullyInstantly(AReadyOrNotCharacter* DoorOpenCharacter)
{
	if (!DoorOpenCharacter)
		return;

	if (IsLocked())
		return;
	
	OpenDoorFullyInstantly(IsActorInFrontOfDoor(DoorOpenCharacter));

	LastDoorUser = DoorOpenCharacter;

	if (LastDoorUser->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}
}

void ADoor::OpenDoorFullyInstantly(const bool bForward)
{
	UnlockDoor();

	RemoveWedges();
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	NumSuccessfulKicksToBreakDown = 0;

	LastDoorUser = nullptr;

	OpenDoor_SpecificAngle(nullptr, bForward ? MaxOpenClose : -MaxOpenClose, true, false);
}

void ADoor::SetDoorLockKnowledge(const bool bSuspectTeam, const bool bKnowledge)
{
	if (bSuspectTeam)
	{
		bSuspectKnowsLockState = bKnowledge;

		if (DriveSubDoor)
		{
			DriveSubDoor->bSuspectKnowsLockState = bKnowledge;
		}
	}
	else
	{
		bSWATKnowsLockState = bKnowledge;

		if (DriveSubDoor)
		{
			DriveSubDoor->bSWATKnowsLockState = bKnowledge;
		}
	}
}

void ADoor::SetDoorTrapKnowledge(const bool bSuspectTeam, const bool bKnowledge)
{
	if (bSuspectTeam)
	{
		bSuspectKnowsTrapState = bKnowledge;
	}
	else
	{
		bSWATKnowsTrapState = bKnowledge;
	}
}

void ADoor::ResetDoorLockKnowledge()
{
	bSWATKnowsLockState = false;
	bSuspectKnowsLockState = false;

	if (DriveSubDoor)
	{
		DriveSubDoor->bSWATKnowsLockState = false;
		DriveSubDoor->bSuspectKnowsLockState = false;
	}
}

void ADoor::ResetDoorTrapKnowledge()
{
	bSWATKnowsTrapState = false;
	bSuspectKnowsTrapState = false;

	if (DriveSubDoor)
	{
		DriveSubDoor->bSWATKnowsTrapState = false;
		DriveSubDoor->bSuspectKnowsTrapState = false;
	}
}

bool ADoor::TeamKnowsDoorLockState(const bool bSuspectTeam) const
{
	if (bSuspectTeam)
	{
		return bSuspectKnowsLockState;
	}

	return bSWATKnowsLockState;
}

bool ADoor::TeamKnowsDoorTrapState(const bool bSuspectTeam) const
{
	if (bSuspectTeam)
	{
		return bSuspectKnowsTrapState;
	}

	return bSWATKnowsTrapState;	
}

bool ADoor::IsLocationSameSideAsTrap(FVector InLocation) const
{
	return bTrapInFront == IsPointInFrontOfDoorway(InLocation);
}

bool ADoor::IsActorSameSideAsTrap(AActor* InActor) const
{
	if (!InActor)
		return false;

	return bTrapInFront == IsPointInFrontOfDoorway(InActor->GetActorLocation());
}

bool ADoor::IsTrapLive() const
{
	return AttachedTrap && AttachedTrap->TrapStatus == ETrapState::TS_Live;
}

bool ADoor::CanPushDoorWhileBroken() const
{
	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return false;
	
	return bDoorBroken && !AllMajorDoorChunksDestroyed();
}

bool ADoor::IsTooFarForKick() const
{
	return DoorKickInteractableComp->GetDistanceFromPlayer() > DoorKickInteractableComp->ShowPromptAtDistance - 25.0f;
}

float ADoor::PushDoor(AReadyOrNotCharacter* DoorPusherCharacter, const float InIncrementAngle, const bool bAnimateDoorHandle, const bool bPlaySound)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return LastTargetAngle;

	if (!DoorPusherCharacter)
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;

		const float IncrementAngle_Abs = FMath::Abs(InIncrementAngle);
		const float Difference = FMath::GridSnap(FMath::Abs(LastStartAngle), IncrementAngle_Abs) - FMath::Abs(LastStartAngle);
		const float TargetYaw = IsActorInFrontOfDoor(DoorPusherCharacter) ? LastStartAngle + (IncrementAngle_Abs - Difference) : LastStartAngle - (IncrementAngle_Abs + Difference);

		LastTargetAngle = FMath::Clamp(TargetYaw, -MaxOpenClose, MaxOpenClose);
		TargetHandleAngle = IncrementAngle_Abs;

		if (bAnimateDoorHandle)
			PlayTimelines({&TL_DoorOpenClose, &TL_DoorHandleOpen});
		else
			PlayTimeline(TL_DoorOpenClose);
		
		return LastTargetAngle;
	}

	if (InIncrementAngle == 0.0f)
		return LastTargetAngle;

	if ((IsLocked() || IsJammed()) && !TL_DoorLocked.IsPlaying())
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		PlayLockedSound();
		PlayTimelines({&TL_DoorLocked, &TL_DoorHandleLocked});
	}
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);

	if ((!CanOpenDoor(DoorPusherCharacter) && !AnyChunksDestroyed()) || IsLocked())
		return LastTargetAngle;

	if (DoorPusherCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;

	const float IncrementAngle_Abs = FMath::Abs(InIncrementAngle);
	const float Difference = FMath::GridSnap(FMath::Abs(LastStartAngle), IncrementAngle_Abs) - FMath::Abs(LastStartAngle);
	const float TargetYaw = IsActorInFrontOfDoor(DoorPusherCharacter) ? LastStartAngle + (IncrementAngle_Abs - Difference) : LastStartAngle - (IncrementAngle_Abs + Difference);
	
	LastTargetAngle = FMath::Clamp(TargetYaw, -MaxOpenClose, MaxOpenClose);
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward)
			LastTargetAngle = FMath::Clamp(TargetYaw, -MaxOpenClose, 0.0f);
		else
			LastTargetAngle = FMath::Clamp(TargetYaw, 0.0f, MaxOpenClose);
	}
	
	TargetHandleAngle = IncrementAngle_Abs;
	LastDoorUser = DoorPusherCharacter;

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorPush, true, true);

	if (bAnimateDoorHandle)
	{
		PlayTimeline(TL_DoorHandlePush, false, false);
	}

	if (bPlaySound)
		Multicast_PlayDoorSound(EDoorInteraction::Push, DoorPusherCharacter, {MakeOcclusionParam()});

	return LastTargetAngle;
}

void ADoor::PushDoor_SpecificAngle(AReadyOrNotCharacter* DoorPusherCharacter, const float CustomTargetAngle, const bool bAnimateDoorHandle)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (!DoorPusherCharacter)
		return;
	
	if (!bDoorBroken && (IsLocked() || IsJammed()) && !TL_DoorLocked.IsPlaying())
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		PlayLockedSound();
		PlayTimelines({&TL_DoorHandleLocked, &TL_DoorLocked});
	}
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);

	if ((!CanOpenDoor(DoorPusherCharacter) && !AnyChunksDestroyed()) || IsLocked())
		return;
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;

	LastTargetAngle = bOneWay ? FMath::Abs(CustomTargetAngle) : FMath::Clamp(CustomTargetAngle, -MaxOpenClose, MaxOpenClose);
	TargetHandleAngle = FMath::Abs(CustomTargetAngle);
	LastDoorUser = DoorPusherCharacter;

	if (DoorPusherCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorPush, true, true);

	if (bAnimateDoorHandle)
	{
		PlayTimeline(TL_DoorHandlePush, false, false);
	}

	Multicast_PlayDoorSound(EDoorInteraction::Push, DoorPusherCharacter, {MakeOcclusionParam()});
}

float ADoor::PeekDoor(AReadyOrNotCharacter* DoorPeekerCharacter, const float InIncrementAngle, const bool bAnimateDoorHandle)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return LastTargetAngle;

	if (!DoorPeekerCharacter || InIncrementAngle == 0.0f)
		return LastTargetAngle;

	CheckKeycards(DoorPeekerCharacter);

	if (!bDoorBroken && (IsLocked() || IsJammed()) && !TL_DoorLocked.IsPlaying())
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		PlayLockedSound();
		PlayTimelines({&TL_DoorHandleLocked, &TL_DoorLocked});
	}
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);
	
	if (!CanOpenDoor(DoorPeekerCharacter) || IsLocked())
		return LastTargetAngle;

	if (IsOpen() && !IsOpenBy_Angle(IncrementAngle))
		return LastTargetAngle;

	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);
	
	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	
	const float IncrementAngle_Abs = FMath::Abs(InIncrementAngle);

	if (FMath::Abs(LastTargetAngle) >= IncrementAngle_Abs)
	{
		LastTargetAngle = 0.0f;
	}
	else
	{
		float OpenAmount;
		
		if (IsActorInFrontOfDoor(DoorPeekerCharacter))
		{
			OpenAmount = IncrementAngle_Abs;
			if (bOneWay)
			{
				if (OneWayDirection == EDoorOneWayDirection::Forward)
					OpenAmount = -IncrementAngle_Abs;
			}
		}
		else
		{
			OpenAmount = -IncrementAngle_Abs;
			if (bOneWay)
			{
				if (OneWayDirection == EDoorOneWayDirection::Backward)
					OpenAmount = IncrementAngle_Abs;
			}
		}
		
		LastTargetAngle = OpenAmount;
	}
	
	TargetHandleAngle = IncrementAngle_Abs;

	LastDoorUser = DoorPeekerCharacter;

	if (DoorPeekerCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	NumSuccessfulKicksToBreakDown = 0;
	
	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorPush, true, true);

	if (bAnimateDoorHandle)
	{
		PlayTimeline(TL_DoorHandlePush, false, false);
	}

	Multicast_PlayDoorSound(EDoorInteraction::Push, DoorPeekerCharacter, {MakeOcclusionParam()});

	return LastTargetAngle;
}

void ADoor::CloseDoor(AReadyOrNotCharacter* DoorCloserCharacter, const bool bInstant, const bool bAnimateDoorHandle)
{
	OpenDoor_SpecificAngle(DoorCloserCharacter, 0.0f, bInstant, bAnimateDoorHandle);
}

float ADoor::OpenDoor(AReadyOrNotCharacter* DoorOpenCharacter, const bool bInstant, const bool bAnimateDoorHandle, const bool bNoCloseThreshold)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return LastTargetAngle;

	if (!DoorOpenCharacter)
		return LastTargetAngle;

	if (DoorStatic->GetAttachParent() == nullptr || DoorStatic->GetAttachParent() != RootComponent)
		return LastStartAngle;

	CheckKeycards(DoorOpenCharacter);

	const bool bBypassLock = DoorOpenCharacter->IsSuspect() && bSuspectAlwaysUnlocks;
	
	if (!bDoorBroken && ((IsLocked() && !bBypassLock) || IsJammed()) && !TL_DoorLocked.IsPlaying())
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		PlayLockedSound();
		PlayTimelines({&TL_DoorHandleLocked, &TL_DoorLocked});
	}
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);

	if (!CanOpenDoor(DoorOpenCharacter))
		return LastTargetAngle;

	if (IsLocked() && !bBypassLock)
		return LastTargetAngle;
	
	if (LastTargetAngle == 0.0f && IsActorBehindDoor_Relative(DoorOpenCharacter))
		return LastTargetAngle;

	UnlockDoor();
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	
	if (IsActorInFrontOfDoor(DoorOpenCharacter))
	{
		if (IsOpen_Forward())
		{
			IsOpenBeyond_Angle(CloseThreshold) && !bNoCloseThreshold ? LastTargetAngle = 0.0f : LastTargetAngle = MaxOpenClose;
		}
		else
		{
			float OpenAmount = MaxOpenClose;
			if (bOneWay)
			{
				if (OneWayDirection == EDoorOneWayDirection::Backward)
					OpenAmount = MaxOpenClose;
				else
					OpenAmount = -MaxOpenClose;
			}

			const bool bClose = bOneWay ? IsOpenBeyond(0.5f) : IsOpen();
			LastTargetAngle = bClose ? 0.0f : OpenAmount;
		}
	}
	else
	{
		if (IsOpen_Backward())
		{
			IsOpenBeyond_Angle(CloseThreshold) && !bNoCloseThreshold ? LastTargetAngle = 0.0f : LastTargetAngle = -MaxOpenClose;
		}
		else
		{
			float OpenAmount = -MaxOpenClose;
			if (bOneWay)
			{
				if (OneWayDirection == EDoorOneWayDirection::Backward)
					OpenAmount = MaxOpenClose;
				else
					OpenAmount = -MaxOpenClose;
			}
			
			const bool bClose = bOneWay ? IsOpenBeyond(0.5f) : IsOpen();
			LastTargetAngle = bClose ? 0.0f : OpenAmount;
		}
	}

	TargetHandleAngle = 90.0f;

	NumSuccessfulKicksToBreakDown = 0;

	if (DriveSubDoor)
	{
		DriveSubDoor->NumSuccessfulKicksToBreakDown = 0;
	}

	LastDoorUser = DoorOpenCharacter;

	if (DoorOpenCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	Multicast_PlayDoorSound(EDoorInteraction::Open, DoorOpenCharacter, {MakeOcclusionParam()});

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	if (bInstant)
	{
		OpenCloseAmount = LastTargetAngle;
	}
	else
	{
		PlayTimeline(TL_DoorOpenClose, true, true);

		if (bAnimateDoorHandle)
		{
			PlayTimeline(TL_DoorHandleOpen, false, false);
		}
	}

	return LastTargetAngle;
}

void ADoor::OpenDoor_SpecificAngle(AReadyOrNotCharacter* DoorOpenCharacter, const float CustomTargetAngle, const bool bInstant, const bool bAnimateDoorHandle)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (!DoorOpenCharacter)
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		LastTargetAngle = CustomTargetAngle;
		TargetHandleAngle = 90.0f;

		UnlockDoor();
		
		if (bInstant)
		{
			OpenCloseAmount = CustomTargetAngle;
		}
		else
		{
			if (bAnimateDoorHandle)
				PlayTimelines({&TL_DoorOpenClose, &TL_DoorHandleOpen});
			else
				PlayTimeline(TL_DoorOpenClose);

			Multicast_PlayDoorSound(EDoorInteraction::Open, DoorOpenCharacter, {MakeOcclusionParam()});
		}
		
		return;
	}
	
	const bool bBypassLock = DoorOpenCharacter->IsSuspect() && bSuspectAlwaysUnlocks;

	if (!bDoorBroken && ((IsLocked() && !bBypassLock) || IsJammed()) && !TL_DoorLocked.IsPlaying())
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		PlayLockedSound();
		PlayTimelines({&TL_DoorHandleLocked, &TL_DoorLocked});
	}
	
	SetDoorLockKnowledge(true, true);
	SetDoorLockKnowledge(false, true);

	if (!CanOpenDoor(DoorOpenCharacter) || (IsLocked() && !bBypassLock) || (LastTargetAngle == 0.0f && TL_DoorOpenClose.IsPlaying() && IsActorBehindDoor_Relative(DoorOpenCharacter)))
		return;
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	//LastTargetAngle = bOneWay ? FMath::Abs(CustomTargetAngle) : CustomTargetAngle;
	LastTargetAngle = CustomTargetAngle;
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward)
			LastTargetAngle = FMath::Clamp(CustomTargetAngle, -MaxOpenClose, 0.0f);
		else
			LastTargetAngle = FMath::Clamp(CustomTargetAngle, 0.0f, MaxOpenClose);
	}
	
	TargetHandleAngle = 90.0f;
	LastDoorUser = DoorOpenCharacter;

	if (DoorOpenCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	Multicast_PlayDoorSound(EDoorInteraction::Open, DoorOpenCharacter, {MakeOcclusionParam()});

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	UnlockDoor();
	
	if (bInstant)
	{
		OpenCloseAmount = CustomTargetAngle;
	}
	else
	{
		PlayTimeline(TL_DoorOpenClose, true, true);

		if (bAnimateDoorHandle)
		{
			PlayTimeline(TL_DoorHandleOpen, false, false);
		}
		else
		{
			DoorHandlePitchAmount = 0.0f;
		}
	}
}

void ADoor::OpenDoor_Debug()
{
	StopAllTimelines();

	OpenCloseAmount = MaxOpenClose;
}

void ADoor::CloseDoor_Debug()
{
	StopAllTimelines();

	OpenCloseAmount = 0.0f;
}

void ADoor::Restore()
{
	Reset();
}

float ADoor::RamDoor(AReadyOrNotCharacter* DoorRamCharacter, const bool bPlayRamSound)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return LastTargetAngle;

	if (!DoorRamCharacter)
		return LastTargetAngle;

	if (!AnyHingesLeft())
		return LastStartAngle;

	if (TL_DoorRam.IsPlaying())
		return LastTargetAngle;

	if (!bCanUseRam)
		return LastTargetAngle;

	UnlockDoor();

	RemoveWedges();
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);
	
	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = IsActorInFrontOfDoor(DoorRamCharacter) ? MaxOpenClose : -MaxOpenClose;
	DoorHandlePitchAmount = 0.0f;
	LastDoorUser = DoorRamCharacter;

	const bool bCauserInFront = IsActorInFrontOfDoor(DoorRamCharacter);
	
	// break off one way door?
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
			OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront)
		{
			if (DoorData.bCanBreakOffOneWayDoorWithKick)
			{
				BreakAndDetachDoor(IsDestructible(), DoorRamCharacter, 15500.0f, 20.0f);
			}
		}
	}

	if (DoorRamCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorRam);

	if (bPlayRamSound)
	{
		Multicast_PlayDoorDamageSound(EDoorDamageType::DDT_Ramming, {});

		const float Damage = AI_CONFIG_GET_FLOAT("BatteringRamMorale.Damage");
		const float InnerRadius = AI_CONFIG_GET_FLOAT("BatteringRamMorale.DamageInnerRadius");
		const float OuterRadius = AI_CONFIG_GET_FLOAT("BatteringRamMorale.DamageOuterRadius");
		const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("BatteringRamMorale.DamageFalloffCurve"));

		float RepeatBreachFactor = FMath::Pow(AI_CONFIG_GET_FLOAT("RepeatBreachDamageFactor", 0.25f), FMath::Max(TimesBreached, 0));
		if (TimesBreached >= AI_CONFIG_GET_INT("MaximumDoorBreachesImpactingMorale", 3))
			RepeatBreachFactor = 0.0f;
		
		const float DamageMultiplier = FMath::Max(URosterManager::GetSquadTraitValue("Breacher", GetWorld()), 1.0f) * RepeatBreachFactor;
		const float FinalDamage = Damage * DamageMultiplier;
		
		UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, GetDoorMidLocation(), FinalDamage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_SUSPECT, ETeamType::TT_CIVILIAN}, Curve, "Battering Ram");
		TimesBreached++;
	}
	
	return LastTargetAngle;
}

float ADoor::BodyRamDoor(AReadyOrNotCharacter* DoorRamCharacter)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return LastTargetAngle;

	if (!DoorRamCharacter)
		return LastTargetAngle;

	if (!AnyHingesLeft())
		return LastStartAngle;

	if (TL_DoorRam.IsPlaying())
		return LastTargetAngle;

	UnlockDoor();

	RemoveWedges();
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);
	
	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = IsActorInFrontOfDoor(DoorRamCharacter) ? MaxOpenClose : -MaxOpenClose;
	DoorHandlePitchAmount = 0.0f;
	LastDoorUser = DoorRamCharacter;

	const bool bCauserInFront = IsActorInFrontOfDoor(DoorRamCharacter);
	
	// break off one way door?
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
			OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront)
		{
			if (DoorData.bCanBreakOffOneWayDoorWithKick)
			{
				BreakAndDetachDoor(IsDestructible(), DoorRamCharacter, 15500.0f, 20.0f);
			}
		}
	}

	if (DoorRamCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorRam);

	Multicast_PlayDoorDamageSound(EDoorDamageType::DDT_Bash, {});

	return LastTargetAngle;
}

void ADoor::BreachDoor(AReadyOrNotCharacter* DoorBreacherCharacter, const float InIncrementAngle)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (!DoorBreacherCharacter)
		return;
	
	if (!bCanUseBSG)
		return;

	BreachDoorFromPoint(DoorBreacherCharacter, DoorBreacherCharacter->GetActorLocation(), InIncrementAngle);
}

void ADoor::BreachDoorFromPoint(AReadyOrNotCharacter* DoorBreacherCharacter, const FVector BreachPoint, const float InIncrementAngle)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (InIncrementAngle <= 0.0f)
		return;

	TriggerAlarm(DoorBreacherCharacter);
	UnlockDoor();

	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	
	const float IncrementAngle_Abs = FMath::Abs(InIncrementAngle);
	const float Difference = FMath::GridSnap(FMath::Abs(LastStartAngle), IncrementAngle_Abs) - FMath::Abs(LastStartAngle);
	const float TargetYaw = IsPointInFrontOfDoor(BreachPoint) || bOneWay ? LastStartAngle + (IncrementAngle_Abs - Difference) : LastStartAngle - (IncrementAngle_Abs + Difference);

	LastTargetAngle = FMath::Clamp(TargetYaw, bOneWay ? 0.0f : -MaxOpenClose, MaxOpenClose);
	
	DoorHandlePitchAmount = 0.0f;

	if (bOneWay && LastTargetAngle < 0.0f)
	{
		BreakDoor(true, DoorBreacherCharacter);
	}
	
	if (DoorBreacherCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	LastDoorUser = DoorBreacherCharacter;

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	PlayTimeline(TL_DoorBreach);
	
	const float Damage = AI_CONFIG_GET_FLOAT("BreachingShotgunMorale.Damage");
	const float InnerRadius = AI_CONFIG_GET_FLOAT("BreachingShotgunMorale.DamageInnerRadius");
	const float OuterRadius = AI_CONFIG_GET_FLOAT("BreachingShotgunMorale.DamageOuterRadius");
	const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("BreachingShotgunMorale.DamageFalloffCurve"));
	
	float RepeatBreachFactor = FMath::Pow(AI_CONFIG_GET_FLOAT("RepeatBreachDamageFactor"), FMath::Max(TimesBreached, 0));
	if (TimesBreached >= AI_CONFIG_GET_INT("MaximumDoorBreachesImpactingMorale"))
		RepeatBreachFactor = 0.0f;
	
	const float DamageMultiplier = FMath::Max(URosterManager::GetSquadTraitValue("Breacher", GetWorld()), 1.0f) * RepeatBreachFactor;
	const float FinalDamage = Damage * DamageMultiplier;
		
	UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, BreachPoint, FinalDamage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_CIVILIAN, ETeamType::TT_SUSPECT}, Curve, "Breaching Shotgun");
	TimesBreached++;
}

void ADoor::ExplodeDoor(AReadyOrNotCharacter* DoorBreacherCharacter, AActor* ExplosionCauser, bool bKeepHinges)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	Multicast_ExplodeDoor(DoorBreacherCharacter, ExplosionCauser, bKeepHinges);
}

void ADoor::Multicast_ExplodeDoor_Implementation(AReadyOrNotCharacter* DoorBreacherCharacter, AActor* ExplosionCauser, bool bKeepHinges)
{
	if (IsDoorwayOnly())
		return;
		
	if (!ExplosionCauser)
		return;

	if (bDoorBroken)
		return;

	if (TL_DoorExplode.IsPlaying())
		return;

	LastDoorUser = DoorBreacherCharacter;

	BreakDoor(false, DoorBreacherCharacter);

	const bool bCauserInFront = IsActorInFrontOfDoor(ExplosionCauser);

	if (IsDestructible())
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			if (!bKeepHinges || (bKeepHinges && Chunk != DoorChunk0 && Chunk != DoorChunk3 && Chunk != DoorChunk6))
			{
				Chunk->SetWorldLocation(Chunk->GetComponentLocation() + GetActorForwardVector() * (bCauserInFront ? -20.0f : 20.0f));
			}
		}

		DestroyAllChunks(bCauserInFront ? -GetActorForwardVector() : GetActorForwardVector(), 2500.0f, bKeepHinges);
	}
	else
	{
		FVector ForwardVector = GetActorForwardVector();
		if (IsNonMainSubdoor() && GetSubDoor())
		{
			ForwardVector = -GetActorForwardVector();
		}
		DoorStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		DoorStatic->SetWorldLocation(DoorStatic->GetComponentLocation() + ForwardVector * (bCauserInFront ? -70.0f : 70.0f));
		DoorStatic->SetSimulatePhysics(true);
		DoorStatic->SetMassOverrideInKg(NAME_None, 100.0f);
		DoorStatic->SetLinearDamping(0.05f);
		DoorStatic->SetCollisionObjectType(ECC_PhysicsBody);
		DoorStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DoorStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		DoorStatic->AddImpulse((bCauserInFront ? -ForwardVector : ForwardVector) * 15500.0f);
		//DoorStatic->AddImpulseAtLocation(bC2InFront ? DoorStatic->GetForwardVector().GetSafeNormal2D() * 1000.0f : DoorStatic->GetForwardVector().GetSafeNormal2D() * -1000.0f, Doorway->GetComponentLocation() + GetActorForwardVector() * (bC2InFront ? 20.0f : -20.0f), NAME_None);
		DoorStatic->SetCanEverAffectNavigation(false);
		// quick hack to scale the door down when we blow it up so it doesn't get stuck or if the designer has it clipping slightly!!
		DoorStatic->SetWorldScale3D(FVector(0.92f));
		
		if (C2ExplosionDecalComponent)
		{
			C2ExplosionDecalComponent->Activate();
			C2ExplosionDecalComponent->SetVisibility(true);
			C2ExplosionDecalComponent->AttachToComponent(GetLockpickHighlight(), FAttachmentTransformRules::KeepRelativeTransform);
			C2ExplosionDecalComponent->SetRelativeLocationAndRotation(FVector(-7.0f, -90.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f));
			C2ExplosionDecalComponent->SetRelativeScale3D(FVector(0.92f));
		}
		
		//UGameplayStatics::SpawnDecalAttached(C2ExplosionDecal, FVector(5.0f, 96.0f, 96.0f), GetLockpickHighlight());
	}

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = bCauserInFront ? MaxOpenClose : -MaxOpenClose;
	OpenCloseAmount = LastTargetAngle;

	LastDoorDamage = EDoorDamageType::DDT_Blasting;

	PlayDoorDamageSound(EDoorDamageType::DDT_Blasting, {});

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;

	OnDoorExploded.Broadcast(this, DoorBreacherCharacter);

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetDoorMidLocation(), 2.0f, this, 1500.0f, EXPLODE_DOOR_NOISE_TAG);

	//PlayTimeline(TL_DoorExplode);
}

void ADoor::CollapseDoor(AReadyOrNotCharacter* DoorBreacherCharacter, FVector BreachLocation)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;
	
	if (bDoorBroken)
		return;

	if (BreachLocation == FVector::ZeroVector)
		return;

	if (TL_DoorExplode.IsPlaying())
		return;

	BreakDoor(false, DoorBreacherCharacter);

	const bool bPointInFront = IsPointInFrontOfDoor(BreachLocation);

	if (IsDestructible())
	{
		for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
		{
			Chunk->SetWorldLocation(Chunk->GetComponentLocation() + GetActorForwardVector() * (bPointInFront ? -20.0f : 20.0f));
		}

		DestroyAllChunks(bPointInFront ? -GetActorForwardVector() : GetActorForwardVector(), 2500.0f);
	}
	else
	{
		DoorStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		//DoorStatic->SetWorldLocation(DoorStatic->GetComponentLocation() + GetActorForwardVector() * (bPointInFront ? -20.0f : 20.0f));
		DoorStatic->SetSimulatePhysics(true);
		DoorStatic->SetMassOverrideInKg(NAME_None, 50.0f);
		DoorStatic->SetLinearDamping(0.05f);
		DoorStatic->SetCollisionObjectType(ECC_PhysicsBody);
		DoorStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DoorStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		DoorStatic->AddImpulseAtLocation(bPointInFront ? DoorStatic->GetForwardVector().GetSafeNormal2D() * 1000.0f : DoorStatic->GetForwardVector().GetSafeNormal2D() * -1000.0f, BreachLocation, NAME_None);
		DoorStatic->SetCanEverAffectNavigation(false);
		// quick hack to scale the door down when we blow it up so it doesn't get stuck or if the designer has it clipping slightly!!
		DoorStatic->SetWorldScale3D(FVector(0.98f));
	}

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = bPointInFront ? MaxOpenClose : -MaxOpenClose;
	OpenCloseAmount = LastStartAngle;

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;
}

void ADoor::KickDoor(AReadyOrNotCharacter* DoorKickCharacter, bool bKickSubDoor, bool bForce)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (!DoorKickCharacter)
		return;

	if (IsDoorwayOnly())
		return;
	
	LastDoorUser = DoorKickCharacter;
	
	float InstaKickChance = URosterManager::GetSquadTraitValue("Kicker", GetWorld());
	if (DoorKickCharacter->IsOnSWATTeam() && InstaKickChance > 0.0f)
	{
		bForce |= UKismetMathLibrary::RandomBoolWithWeight(InstaKickChance);
	}
	
	float KickChanceOffset = 0.0f;
	if (Cast<APlayerCharacter>(DoorKickCharacter)) // Only player characters allowed to mess with kick chance
	{
		if (DoorKickCharacter->IsLimbHit(ELimbType::LT_RightLeg))
		{
			const int32 MaxLegTickets = DoorKickCharacter->GetLimbHealth(ELimbType::LT_RightLeg).GetMaxTickets();
			const int32 LegTicketsRemaining = DoorKickCharacter->GetLimbHealth(ELimbType::LT_RightLeg).GetRemainingTickets();
			KickChanceOffset = 1-(LegTicketsRemaining/MaxLegTickets);
				
			DoorKickCharacter->GetHealthComponent()->DecreaseLimbTickets(ELimbType::LT_RightLeg, 1);
			DoorKickCharacter->OnBodyPartDamaged.Broadcast(false, false, false, false, false, true, false, false);
		}
	}
	
	const bool bCauserInFront = IsActorInFrontOfDoor(DoorKickCharacter);

	bool bShouldOpenDoor = false;
	bool bShouldBreakLock = false;
	DecreaseNumKicksToBreakDown(DoorKickCharacter, bShouldOpenDoor, bShouldBreakLock, KickChanceOffset);

	const bool bCantKick = bOneWay && (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
										OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront &&
										!DoorData.bCanBreakOffOneWayDoorWithKick);

	if (!bForce && (!CanKickDoor(DoorKickCharacter) || bCantKick || ((!bShouldOpenDoor && !IsOpen()) || (bKickAlwaysFails && !IsOpen()))))
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		LastTargetAngle = LastStartAngle + 1;

 		PlayTimeline(TL_DoorKickFail);

		Multicast_PlayDoorSound(EDoorInteraction::Kick_Fail, DoorKickCharacter, {MakeOcclusionParam()});
		Multicast_PlayDoorKickEffects(false, bCauserInFront);
		
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);

		OnDoorKicked.Broadcast(this, DoorKickCharacter, false);
		return;
	}

	if (TL_DoorKickSuccess.IsPlaying())
		return;

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);

	NumSuccessfulKicksToBreakDown = 0;

	if (DriveSubDoor)
	{
		DriveSubDoor->NumSuccessfulKicksToBreakDown = 0;
	}
	
	bool bWasLocked = bLocked;
	Multicast_PlayDoorKickEffects(bWasLocked, bCauserInFront);
	
	UnlockDoor();
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);
	
	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = bCauserInFront ? MaxOpenClose : -MaxOpenClose;

	// break off one way door?
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
			OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront)
		{
			if (DoorData.bCanBreakOffOneWayDoorWithKick)
			{
				BreakAndDetachDoor(false, DoorKickCharacter, 15500.0f, 20.0f);
			}
		}
	}
	else
	{
		// TODO: doesnt work great in all environments, disabled for now
		/*
		if (DoorData.bCanBreakOffWithKick)
		{
			BreakAndDetachDoor(false, DoorKickCharacter, 15500.0f, 20.0f);
		}
		*/
	}

	if (DoorKickCharacter->IsOnSWATTeam())
	{
		SetDoorHasEverBeenOpenedBySwat();
	}

	LastDoorDamage = EDoorDamageType::DDT_Kicking;

	PlayDoorKickSound(DoorKickCharacter, 1.0f);

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;
	
	PlayTimeline(TL_DoorKickSuccess);

	// fix player control rotation if door kick worked out
	const FRotator CurrentControlRotation = DoorKickCharacter->GetControlRotation();
	DoorKickCharacter->Client_SetControlRotation(FRotator(0.0f, CurrentControlRotation.Yaw, CurrentControlRotation.Roll));

	if (bKickSubDoor && DriveSubDoor)
	{
		DriveSubDoor->KickSubDoor(DoorKickCharacter);
	}

	OnDoorKicked.Broadcast(this, DoorKickCharacter, true);
}

void ADoor::KickSubDoor(AReadyOrNotCharacter* DoorKickCharacter)
{
	// Only allowed as server
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (!DoorKickCharacter)
		return;
	
	if (TL_DoorKickSuccess.IsPlaying())
		return;
	
	const bool bCauserInFront = IsActorInFrontOfDoor(DoorKickCharacter);
	
	
	const bool bCantKick = bOneWay && (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
										OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront &&
										!DoorData.bCanBreakOffOneWayDoorWithKick);

	if (!CanKickDoor(DoorKickCharacter) || bCantKick || ((NumSuccessfulKicksToBreakDown > 0 && !IsOpen()) || (bKickAlwaysFails && !IsOpen())))
	{
		LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
		LastTargetAngle = LastStartAngle + 1;

		PlayTimeline(TL_DoorKickFail);

		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, this, AI_CONFIG_GET_FLOAT("DoorKickHearingRange", 1000.0f), KICK_DOOR_NOISE_TAG);

		return;
	}
	
	NumSuccessfulKicksToBreakDown = 0;

	UnlockDoor();
	
	SetDoorTrapKnowledge(true, true);
	SetDoorTrapKnowledge(false, true);

	LastStartAngle = DoorStatic->GetRelativeRotation().Yaw;
	LastTargetAngle = IsActorInFrontOfDoor(DoorKickCharacter) ? MaxOpenClose : -MaxOpenClose;

	// break off one way door?
	if (bOneWay)
	{
		if (OneWayDirection == EDoorOneWayDirection::Forward && bCauserInFront ||
			OneWayDirection == EDoorOneWayDirection::Backward && !bCauserInFront)
		{
			if (DoorData.bCanBreakOffOneWayDoorWithKick)
			{
				BreakAndDetachDoor(false, DoorKickCharacter, 15500.0f, 20.0f);
			}
		}
	}
	else
	{
		if (DoorData.bCanBreakOffWithKick)
		{
			BreakAndDetachDoor(false, DoorKickCharacter, 15500.0f, 20.0f);
		}
	}
	
	LastDoorUser = DoorKickCharacter;

	LastDoorDamage = EDoorDamageType::DDT_Kicking;

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;
	
	PlayTimeline(TL_DoorKickSuccess);

	bPendingSubDoorKick = false;
}

void ADoor::BreakDoor(const bool bDestroyAllChunks, AReadyOrNotCharacter* DoorBreakerCharacter)
{
	if (bDoorBroken)
		return;

	TriggerAlarm(DoorBreakerCharacter);
	UnlockDoor();

	RemoveWedges();

	ResetDoorLockKnowledge();
	ResetDoorTrapKnowledge();
	SetDoorHasEverBeenOpenedBySwat();

	if (DoorBreakerCharacter)
		LastDoorUser = DoorBreakerCharacter;

	if (AttachedTrap && AttachedTrap->TrapStatus == ETrapState::TS_Live)
	{
		AttachedTrap->OnTrapTriggered(DoorBreakerCharacter);
	}
	
	if (DoorData.bIsDestructible)
	{
		if (bDestroyAllChunks)
		{
			DestroyAllChunks(GetActorForwardVector());
		}
	}

	DisableDoorChunkNavigation();
	bDoorBroken = true;

	OnDoorBroken.Broadcast();
}

void ADoor::BreakAndDetachDoor(bool bDestroyAllChunks, AReadyOrNotCharacter* DoorBreakerCharacter, float Impulse, float ForwardOffset)
{
	BreakDoor(bDestroyAllChunks, DoorBreakerCharacter);
	
	const bool bCauserInFront = IsActorInFrontOfDoor(DoorBreakerCharacter);
	
	DoorStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	DoorStatic->SetSimulatePhysics(true);
	DoorStatic->SetMassOverrideInKg(NAME_None, 100.0f);
	DoorStatic->SetLinearDamping(0.05f);
	DoorStatic->SetCollisionObjectType(ECC_PhysicsBody);
	DoorStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	DoorStatic->AddImpulse((bCauserInFront ? -GetActorForwardVector() : GetActorForwardVector()) * Impulse);
	//DoorStatic->AddImpulseAtLocation(bC2InFront ? DoorStatic->GetForwardVector().GetSafeNormal2D() * 1000.0f : DoorStatic->GetForwardVector().GetSafeNormal2D() * -1000.0f, Doorway->GetComponentLocation() + GetActorForwardVector() * (bC2InFront ? 20.0f : -20.0f), NAME_None);
	DoorStatic->SetCanEverAffectNavigation(false);
	DoorStatic->SetWorldLocation(DoorStatic->GetComponentLocation() + GetActorForwardVector() * (bCauserInFront ? -ForwardOffset : ForwardOffset) + GetActorRightVector() * 5.0f, false, nullptr, ETeleportType::TeleportPhysics);
	// quick hack to scale the door down when we blow it up so it doesn't get stuck or if the designer has it clipping slightly!!
	DoorStatic->SetWorldScale3D(FVector(0.98f));
}

void ADoor::Tick_DoorOpenClose()
{
	if (!DoorOpenCurve || !DoorCloseCurve)
		return;
	
	const float CurveValue = LastTargetAngle == 0.0f ? DoorCloseCurve->GetFloatValue(TL_DoorOpenClose.GetPlaybackPosition()) : DoorOpenCurve->GetFloatValue(TL_DoorOpenClose.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, FMath::Abs(CurveValue));

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorOpenClose.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;

	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorKick_Success()
{
	if (!DoorKickSuccessCurve)
		return;
	
	const float CurveValue = DoorKickSuccessCurve->GetFloatValue(TL_DoorKickSuccess.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorKickSuccess.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;

	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorKick_Fail()
{
	if (!DoorKickFailCurve)
		return;
	
	const float CurveValue = DoorKickFailCurve->GetFloatValue(TL_DoorKickFail.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorKickFail.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;
	
	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorPush()
{
	if (!DoorPushCurve)
		return;
	
	const float CurveValue = DoorPushCurve->GetFloatValue(TL_DoorPush.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorPush.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;
	
	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorHandle_Open()
{
	if (!DoorHandleOpenCurve)
		return;
	
	const float CurveValue = DoorHandleOpenCurve->GetFloatValue(TL_DoorHandleOpen.GetPlaybackPosition());
	const float PitchValue = FMath::Lerp(0.0f, TargetHandleAngle, CurveValue);

	DoorHandlePitchAmount = PitchValue;
}

void ADoor::Tick_DoorHandle_Push()
{
	if (!DoorHandlePushCurve)
		return;
	
	const float CurveValue = DoorHandlePushCurve->GetFloatValue(TL_DoorHandlePush.GetPlaybackPosition());
	const float PitchValue = FMath::Lerp(0.0f, TargetHandleAngle, CurveValue);

	DoorHandlePitchAmount = PitchValue;
}

void ADoor::Tick_DoorHandleLocked()
{
	if (!DoorHandleLockedCurve)
		return;
	
	const float CurveValue = DoorHandleLockedCurve->GetFloatValue(TL_DoorHandleLocked.GetPlaybackPosition());

	DoorHandlePitchAmount = CurveValue;
}

void ADoor::Tick_DoorLocked()
{
	if (!DoorLockedCurve)
		return;
	
	const float YawValue = DoorLockedCurve->GetFloatValue(TL_DoorLocked.GetPlaybackPosition());

	DistanceMovedThisFrame = 0.0f;
	AccumulatedDistance = 0.0f;
	TimeMovedThisFrame = 0.0f;
	AccumulatedTime = 0.0f;
	
	OpenCloseAmount = LastStartAngle + YawValue;
}

void ADoor::Tick_DoorRam()
{
	if (!DoorRamCurve)
		return;
	
	const float CurveValue = DoorRamCurve->GetFloatValue(TL_DoorRam.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorRam.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;
	
	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorExplode()
{
	if (!DoorExplodeCurve)
		return;
	
	const float CurveValue = DoorExplodeCurve->GetFloatValue(TL_DoorExplode.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorExplode.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;
	
	OpenCloseAmount = YawValue;
}

void ADoor::Tick_DoorBreach()
{
	if (!DoorBreachCurve)
		return;
	
	const float CurveValue = DoorBreachCurve->GetFloatValue(TL_DoorBreach.GetPlaybackPosition());
	const float YawValue = FMath::Lerp(LastStartAngle, LastTargetAngle, CurveValue);

	DistanceMovedThisFrame = CurveValue - AccumulatedDistance;
	AccumulatedDistance += DistanceMovedThisFrame;
	TimeMovedThisFrame = TL_DoorBreach.GetPlaybackPosition() - AccumulatedTime;
	AccumulatedTime += TimeMovedThisFrame;
	
	OpenCloseAmount = YawValue;
}

void ADoor::Finished_DoorRam()
{
	LastDoorDamage = EDoorDamageType::DDT_None;
}

void ADoor::Finished_DoorExplode()
{
	LastDoorDamage = EDoorDamageType::DDT_None;
}

void ADoor::Finished_DoorKick_Success()
{
	LastDoorDamage = EDoorDamageType::DDT_None;
}

#if WITH_EDITOR
void ADoor::DrawKillStunDistances(const float DeltaTime)
{
	if (bDoorBroken)
		return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	
	// Kill distances
	{
		float CurrentSpacing = 0.0f;
		for (auto KillDistance : DoorKillDistance)
		{
			const float Spacing = 40.0f;
			
			if (KillDistance.Value > 0.0f)
			{
				FVector Start = (DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * CurrentSpacing) + DoorStatic->GetUpVector() * 10.0f;
				FVector End_Forward = Start + DoorStatic->GetForwardVector() * KillDistance.Value;
				FVector End_Backward = Start + DoorStatic->GetForwardVector() * -KillDistance.Value;

				FString DoorDamageTypeString = ENUM_TO_STRING(EDoorDamageType, KillDistance.Key, false);

				if (Player)
				{
					if (FVector::Distance(End_Forward, Player->GetActorLocation()) < 200.0f)
						DrawDebugString(GetWorld(), End_Forward + DoorStatic->GetForwardVector() * 10.0f, FString("Door Kill Distance [" + DoorDamageTypeString + "]"), nullptr, FColor::White, DeltaTime);

					if (FVector::Distance(End_Backward, Player->GetActorLocation()) < 200.0f)
						DrawDebugString(GetWorld(), End_Backward + DoorStatic->GetForwardVector() * 10.0f, FString("Door Kill Distance [" + DoorDamageTypeString + "]"), nullptr, FColor::White, DeltaTime);
				}

				DrawDebugLine(GetWorld(), Start, End_Forward, FColor::Red, false, -1, 0, 2);
				DrawDebugLine(GetWorld(), Start, End_Backward, FColor::Red, false, -1, 0, 2);
			}

			CurrentSpacing += Spacing;
		}
	}
	
	// Stun distances
	{
		float CurrentSpacing = 0.0f;
		for (auto StunDistance : DoorStunDistance)
		{
			const float Spacing = 40.0f;
			if (StunDistance.Value > 0.0f)
			{
				const float ActualDistance = StunDistance.Value - DoorKillDistance[StunDistance.Key];
				
				FVector Start_Forward = ((DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * CurrentSpacing) + DoorStatic->GetForwardVector() * DoorKillDistance[StunDistance.Key]) + DoorStatic->GetUpVector() * 10.0f;
				FVector Start_Backward = ((DoorStatic->GetComponentLocation() + DoorStatic->GetRightVector() * CurrentSpacing) + DoorStatic->GetForwardVector() * -DoorKillDistance[StunDistance.Key]) + DoorStatic->GetUpVector() * 10.0f;
				FVector End_Forward = Start_Forward + DoorStatic->GetForwardVector() * ActualDistance;
				FVector End_Backward = Start_Backward + DoorStatic->GetForwardVector() * -ActualDistance;

				FString DoorDamageTypeString = ENUM_TO_STRING(EDoorDamageType, StunDistance.Key, false);

				if (Player)
				{
					if (FVector::Distance(End_Forward, Player->GetActorLocation()) < 200.0f)
						DrawDebugString(GetWorld(), End_Forward, FString("Door Stun Distance [" + DoorDamageTypeString + "]"), nullptr, FColor::White, DeltaTime);
					
					if (FVector::Distance(End_Backward, Player->GetActorLocation()) < 200.0f)
						DrawDebugString(GetWorld(), End_Backward, FString("Door Stun Distance [" + DoorDamageTypeString + "]"), nullptr, FColor::White, DeltaTime);
				}

				DrawDebugLine(GetWorld(), Start_Forward, End_Forward, FColor::Yellow, false, -1, 0, 2);
				DrawDebugLine(GetWorld(), Start_Backward, End_Backward, FColor::Yellow, false, -1, 0, 2);
			}

			CurrentSpacing += Spacing;
		}
	}
}
#endif

void ADoor::GenerateClearPoints()
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GetWorld()))
	{
		WorldData->GenerateDoorClearPointsV2(this);
	}
}

bool ADoor::AnyChunksDestroyed() const
{
	if (!IsDestructible())
		return false;
	
	for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
	{
		if (IsValid(Chunk) && (Chunk->IsDestroyed() || !Chunk->IsVisible()))
		{
			return true;
		}
	}

	return false;
}

bool ADoor::IsMiddleChunkBroken() const
{
	if (!IsDestructible())
		return false;

	return DoorChunk4->IsDestroyed() || !DoorChunk4->IsVisible();
}

bool ADoor::AllMajorDoorChunksDestroyed() const
{
	if (!IsDestructible())
		return false;

	return IsDoorChunkDestroyed(DoorChunk1) && IsDoorChunkDestroyed(DoorChunk4) &&
			IsDoorChunkDestroyed(DoorChunk5) && IsDoorChunkDestroyed(DoorChunk7) &&
			IsDoorChunkDestroyed(DoorChunk8);
	
	/*
	return (DoorChunk1->IsDestroyed() && DoorChunk2->IsDestroyed() && DoorChunk4->IsDestroyed() && DoorChunk5->IsDestroyed() && DoorChunk7->IsDestroyed() && DoorChunk8->IsDestroyed()) ||
			(!DoorChunk1->IsVisible() && !DoorChunk2->IsVisible() && !DoorChunk4->IsVisible() && !DoorChunk5->IsVisible() && !DoorChunk7->IsVisible() && !DoorChunk8->IsVisible());
	*/
}

bool ADoor::IsDoorChunkDestroyed(UDestructibleDoorChunkComponent* InChunkComponent) const
{
	if (!IsValid(InChunkComponent))
		return false;
	
	return InChunkComponent->IsDestroyed() || !InChunkComponent->IsVisible();
}

bool ADoor::AllBottomDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk0->IsDestroyed() && DoorChunk1->IsDestroyed() && DoorChunk2->IsDestroyed()) || (!DoorChunk0->IsVisible() && !DoorChunk1->IsVisible() && !DoorChunk2->IsVisible());
}

bool ADoor::AnyBottomDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk0->IsDestroyed() || DoorChunk1->IsDestroyed() || DoorChunk2->IsDestroyed()) || (!DoorChunk0->IsVisible() || !DoorChunk1->IsVisible() || !DoorChunk2->IsVisible());
}

bool ADoor::AllMiddleDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk3->IsDestroyed() && DoorChunk4->IsDestroyed() && DoorChunk5->IsDestroyed()) || (!DoorChunk3->IsVisible() && !DoorChunk4->IsVisible() && !DoorChunk5->IsVisible());
}

bool ADoor::AnyMiddleDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk3->IsDestroyed() || DoorChunk4->IsDestroyed() || DoorChunk5->IsDestroyed()) || (!DoorChunk3->IsVisible() || !DoorChunk4->IsVisible() || !DoorChunk5->IsVisible());
}

bool ADoor::AllTopDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk6->IsDestroyed() && DoorChunk7->IsDestroyed() && DoorChunk8->IsDestroyed()) || (!DoorChunk6->IsVisible() && !DoorChunk7->IsVisible() && !DoorChunk8->IsVisible());
}

bool ADoor::AnyTopDoorChunksBroken() const
{
	if (!IsDestructible())
		return false;
	
	return (DoorChunk6->IsDestroyed() || DoorChunk7->IsDestroyed() || DoorChunk8->IsDestroyed()) || (!DoorChunk6->IsVisible() || !DoorChunk7->IsVisible() || !DoorChunk8->IsVisible());
}

bool ADoor::AnyHingesLeft() const
{
	if (!IsDestructible())
		return true;
	
	for (UDestructibleDoorChunkComponent* Chunk : ChunkComponents)
	{
		if (Chunk)
		{
			if (!Chunk->IsDestroyed() && Chunk->IsHinge())
			{
				return true;
			}
		}
	}

	return false;
}

bool ADoor::CanSpawnTrap() const
{
	return !bNoSpawnTrap;
}

void ADoor::Multicast_PlayElectronicDoorSound_Implementation(UFMODEvent* Event)
{
	if (!DoorHandleFront)
		return;

	USoundSource* DoorSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), Event, FTransform(FRotator(), GetActorLocation(), FVector()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
	if(DoorSoundSource)
	{
		DoorSoundSource->SetObstructionOcclusionAdditive(0.0f);
		DoorSoundSource->Attach(DoorHandleFront, NAME_None);
		DoorSoundSource->Play();
	}
}

void ADoor::Multicast_PlayDoorKickEffects_Implementation(bool bBreakLock, bool bInFront)
{
	if (!GetWorld() || !IsValid(DoorStatic))
		return;

	FTransform ParticleTransform = !bInFront ? FTransform::Identity : FTransform(FRotator(0.0f, 180.0f, 0.0f));
	ParticleTransform *= DoorData.KickParticleTransform;
	ParticleTransform *= DoorStatic->GetComponentTransform();

	FVector TransformStart = ParticleTransform.TransformPosition(FVector(0.0f, 0.0f, 100.0f));
	FVector TransformEnd = TransformStart + ParticleTransform.TransformVector(FVector::ForwardVector) * 15.0f;
	DrawDebugDirectionalArrow(GetWorld(), TransformStart, TransformEnd, 10.0f, FColor::Red, false, 10.0f);
	
	if (KickedParticleSystem)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), KickedParticleSystem, ParticleTransform);
	}
	
	if (bBreakLock && LockBrokenParticleSystem)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), LockBrokenParticleSystem, ParticleTransform);
	}
}

void ADoor::OnRep_DoorHandlesBroken()
{
	DoorHandleFront->SetSimulatePhysics(true);
	DoorHandleBack->SetSimulatePhysics(true);

	DoorHandleFront->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	DoorHandleBack->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void ADoor::ForceDoorReset_Implementation()
{
	bClientReset = true;
	OnRep_ClientResetDoor();
}

bool ADoor::IsActorBehindDoor_Relative(AActor* Actor) const
{
	if (IsActorInFrontOfDoor(Actor))
	{
		if (!IsOpen())
		{
			return false;
		}
		
		if (IsOpen_Forward())
		{
			return false;
		}

		return true;
	}
	
	if (!IsOpen())
	{
		return false;
	}
	
	if (IsOpen_Backward())
	{
		return false;
	}

	return true;
}

bool ADoor::IsActorInFrontOfDoor(AActor* Actor) const
{
	if (!Actor)
		return false;
	
	return IsPointInFrontOfDoor(Actor->GetActorLocation());
}

bool ADoor::IsActorInFrontOfDoorway(AActor* Actor) const
{
	if (!Actor)
		return false;
	
	return IsPointInFrontOfDoorway(Actor->GetActorLocation());
}

bool ADoor::IsActorRightOfDoorway(AActor* Actor) const
{
	if (!Actor)
		return false;

	return IsPointRightOfDoorway(Actor->GetActorLocation());
}

bool ADoor::IsPointsOnOppositeSideOfDoor(FVector A, FVector B) const
{
	return (IsPointInFrontOfDoorway(A) && !IsPointInFrontOfDoorway(B)) || (!IsPointInFrontOfDoorway(A) && IsPointInFrontOfDoorway(B));
}

#if WITH_EDITOR
void ADoor::ClearCrossLevelReferences()
{
	Super::ClearCrossLevelReferences();
	
	// These MUST not be saved into the level
	if (NavLinkProxy)
	{
		NavLinkProxy->Destroy();
		NavLinkProxy = nullptr;
	}
}
#endif

bool ADoor::IsPointInFrontOfDoor(const FVector Vector) const
{
	const float DotProduct = FVector::DotProduct(DoorStatic->GetForwardVector().GetSafeNormal2D(), (DoorStatic->GetComponentLocation() - Vector).GetSafeNormal2D());
	return DotProduct < 0.0f;
}

bool ADoor::IsPointRightOfDoorway(const FVector Vector) const
{
	const FVector DirectionToDoor = (Vector - Doorway->GetComponentLocation()).GetSafeNormal2D();
	const float RightDotProduct = FVector::DotProduct(DirectionToDoor, GetActorRightVector());
	
	return RightDotProduct > 0.0f;
}

bool ADoor::IsComponentRelevantForNavigation(UActorComponent* Component) const
{
	return Component && Component->IsNavigationRelevant();
}

// ICanIssueCommandOn implementation
bool ADoor::CanIssueCommand_Implementation() const
{
	return true;
}

AActor* ADoor::GetCommandActor_Implementation() const
{
	return const_cast<ADoor*>(this);
}

// ICanPlaceC2On implementation
void ADoor::C2StartPlacement_Implementation(class AC2Explosive* C2)
{
	PlacedC2 = nullptr;
	bC2Placed = true;
}

void ADoor::C2StopPlacement_Implementation(class AC2Explosive* C2)
{
	PlacedC2 = C2->LastPlacedC2Explosive;
	bC2Placed = true;
}

void ADoor::OnC2Detonated_Implementation(class APlacedC2Explosive* C2)
{
	bC2Placed = false;
}

void ADoor::OnC2Removed_Implementation(class APlacedC2Explosive* C2)
{
	bC2Placed = false;
}

bool ADoor::CanPlaceC2OnNow_Implementation(class APlayerCharacter* C2Owner, class AC2Explosive* C2, FHitResult Hit)
{
	if (PlacedC2)
	{
		return false;
	}

	if (bDoorBroken || bC2Placed || IsOpen())
	{
		return false;
	}

	return true;
}

FVector ADoor::GetPlacementLocation_Implementation(const FHitResult TraceHit)
{
	if (IsPointInFrontOfDoor(TraceHit.TraceStart))
	{
		return DoorStatic->GetComponentLocation() + DoorData.C2PlacementPoint_Front.RotateAngleAxis(GetActorRotation().Yaw + DoorStatic->GetRelativeRotation().Yaw, FVector::UpVector);
	}
	
	return DoorStatic->GetComponentLocation() + DoorData.C2PlacementPoint_Back.RotateAngleAxis(GetActorRotation().Yaw + DoorStatic->GetRelativeRotation().Yaw, FVector::UpVector);
}

FRotator ADoor::GetPlacementRotation_Implementation(const FHitResult TraceHit)
{
	FRotator ReturnRotator = DoorStatic->GetComponentRotation();
	
	if (!IsPointInFrontOfDoor(TraceHit.TraceStart))
	{
		ReturnRotator += FRotator(0.0f, 180.0f, 0.0f);
	}
	
	return ReturnRotator -= FRotator(0.0f, 90.0f, 0.0f);
}

void ADoor::GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData)
{
#if !UE_BUILD_SHIPPING
	OutDebugData.Empty(40);
	OutDebugData.Add(FDebugData("Type", FText::FromName(TypeOfDoor.RowName)));
	OutDebugData.Add(FDebugData("One Way", FText::FromString(bOneWay ? "True" : "False")));
	OutDebugData.Add(FDebugData("Max Open Close Angle", FText::AsNumber(MaxOpenClose)));
	OutDebugData.Add(FDebugData("Last Door User", FText::FromString(LastDoorUser ? LastDoorUser->GetName() : "None")));
	OutDebugData.Add(FDebugData("Open Amount (Percentage)", FText::AsNumber(GetOpenAmountAsPercentage())));
	OutDebugData.Add(FDebugData("Open Amount (Angle)", FText::AsNumber(GetOpenAmount())));
	OutDebugData.Add(FDebugData("Lockable", FText::FromString(IsLockable() ? "True" : "False")));
	OutDebugData.Add(FDebugData("Locked Chance", FText::AsNumber(LockedChance)));
	OutDebugData.Add(FDebugData("Locked", FText::FromString(IsLocked() ? "True" : "False")));
	OutDebugData.Add(FDebugData("Jammed", FText::FromString(IsJammed() ? "True" : "False")));
	OutDebugData.Add(FDebugData("Broken", FText::FromString(bDoorBroken ? "True" : "False")));
	OutDebugData.Add(FDebugData("Destructible", FText::FromString(DoorData.bIsDestructible ? "True" : "False")));
	OutDebugData.Add(FDebugData("Door Attached To Root", FText::FromString(DoorStatic->GetAttachParent() == RootComponent ? "True" : "False")));
	OutDebugData.Add(FDebugData("Any Hinges Left", FText::FromString(AnyHingesLeft() ? "True" : "False")));
	OutDebugData.Add(FDebugData("Has Ever Been Opened", FText::FromString(bHasEverBeenOpenedBySwat ? "True" : "False")));
	OutDebugData.Add(FDebugData("Double Door", FText::FromString(DriveSubDoor ? "True" : "False")));
	OutDebugData.Add(FDebugData("Main Double Door", FText::FromString(bMainSubDoor ? "True" : "False")));
	OutDebugData.Add(FDebugData("Kick Always Fails", FText::FromString(bKickAlwaysFails ? "True" : "False")));
	OutDebugData.Add(FDebugData("Kick Success Chance", FText::AsNumber(DoorKickSuccessChance)));
	OutDebugData.Add(FDebugData("Kicks to Break Down", FText::AsNumber(NumSuccessfulKicksToBreakDown)));
	OutDebugData.Add(FDebugData("C2 Placed", FText::FromString(PlacedC2 ? PlacedC2->GetName() : "None")));
	OutDebugData.Add(FDebugData("Wedge Placed", FText::FromString(AttachedWedge ? AttachedWedge->GetName() : "None")));
	OutDebugData.Add(FDebugData("Trap Attached", FText::FromString(AttachedTrap ? AttachedTrap->GetName() : "None")));
	OutDebugData.Add(FDebugData("Trap Status", FText::FromString(AttachedTrap ? ENUM_TO_STRING(ETrapState, AttachedTrap->TrapStatus, false) : "None")));
	OutDebugData.Add(FDebugData("Suspects Knows Trap State", FText::FromString(bSuspectKnowsTrapState ? "True" : "False")));
	OutDebugData.Add(FDebugData("SWAT Knows Trap State", FText::FromString(bSWATKnowsTrapState ? "True" : "False")));
	OutDebugData.Add(FDebugData("Suspects Knows Lock State", FText::FromString(bSuspectKnowsLockState ? "True" : "False")));
	OutDebugData.Add(FDebugData("SWAT Knows Lock State", FText::FromString(bSWATKnowsLockState ? "True" : "False")));
#endif
}

EMultitoolFunctions ADoor::GetMultitoolUseType_Implementation()
{
	return EMultitoolFunctions::MF_Lockpick;
}

float ADoor::GetMultitoolUseTime_Implementation()
{
	return 4.0f;
}

UInteractableComponent* ADoor::GetInteractableComponent_Implementation() const
{
	return DoorOpenInteractableComp;
}

void ADoor::Server_FinishedUsingMultitool_Implementation(AReadyOrNotCharacter* ToolOwner)
{
	OperatingStates.Remove("Player");
	
	if (ToolOwner)
	{
		UnlockDoor();
		SetDoorLockKnowledge(ToolOwner->IsSuspect(), true);
	}
}

void ADoor::Client_FinishedUsingMultitool_Implementation(AReadyOrNotCharacter* ToolOwner)
{
	OperatingStates.Remove("Player");
	
	if (ToolOwner)
	{
		UnlockDoor();
		SetDoorLockKnowledge(ToolOwner->IsSuspect(), true);
	}
}

void ADoor::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (!InSenseController)
		return;
	
	if (GetLastDoorUser() && GetLastDoorUser()->IsOnSWATTeam())
	{
		InSenseController->AddExposedToStimulusTag("DoorUse", GetActorLocation(), false, GetLastDoorUser());

		if (InSenseController->GetCharacter())
		{
			const float MaxZ = FMath::Max(InSenseController->GetCharacter()->GetActorLocation().Z, GetActorLocation().Z);
			const float MinZ = FMath::Min(InSenseController->GetCharacter()->GetActorLocation().Z, GetActorLocation().Z);
			
			const float ZHeightDifference = MaxZ - MinZ;
			
			if ((InSenseController->GetCharacter()->GetActorLocation() - GetActorLocation()).Size() < 500.0f && ZHeightDifference < 150.0f)
			{
				InSenseController->SpottedEnemy(GetLastDoorUser());
				
				InSenseController->GetRONPerceptionComp()->RegisterStimulus(GetLastDoorUser(), FAIStimulus(*UReadyOrNotAISense_Sight::StaticClass()->GetDefaultObject<UReadyOrNotAISense_Sight>(), 1.0f, GetActorLocation(), InSenseController->GetCharacter()->GetActorLocation()));
			}
		}
					
		if (USuspectsAndCivilianManager* SusCivManager = USuspectsAndCivilianManager::Get(this))
		{
			if (SusCivManager->CanInvestigate())
			{
				//InSenseController->InvestigateStimulus(GetFrontMirrorPoint()->GetComponentLocation() + GetFrontMirrorPoint()->GetForwardVector() * 300.0f); 
				InSenseController->InvestigateStimulus(Stimulus); 
				SusCivManager->StartedInvestigating();
			}
			else
			{
				OutOverrideSensedActor = GetLastDoorUser();
			}
		}
	}
}

void ADoor::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (!InSenseController)
		return;
	
	if (GetLastDoorUser() && GetLastDoorUser()->IsOnSWATTeam())
	{
		FActorSense DoorSoundSense;
		DoorSoundSense.Actor = this;
		DoorSoundSense.Tag = Stimulus.Tag;
		DoorSoundSense.Stimulus = Stimulus;
		DoorSoundSense.SenseReactionTime = InSenseController->GetReactionTime(EActorSenseType::Sound);
		DoorSoundSense.SenseForgetTime = Stimulus.Tag == EXPLODE_DOOR_NOISE_TAG ? 5.0f : 30.0f;
			
		InSenseController->AddActorSoundSense(DoorSoundSense);
	}
}