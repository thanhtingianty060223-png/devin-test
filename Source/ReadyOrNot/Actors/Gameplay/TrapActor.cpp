// Copyright Void Interactive, 2021

#include "TrapActor.h"

#include "CableComponent.h"
#include "Actors/Items/Multitool.h"

#include "Characters/CyberneticController.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Characters/AI/SWATCharacter.h"

#include "Components/DestructibleDoorChunkComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/ScoringComponent.h"
#include "Components/SplineComponent.h"
#include "Info/ScoringManager.h"
#include "Info/SuspectsAndCivilianManager.h"
#include "Info/Activities/PlaceTrapActivity.h"

#include "Info/Activities/Team/ArrestTargetActivity.h"
#include "Info/Activities/Team/DisarmStandaloneTrapActivity.h"
#include "Info/Activities/Team/DisarmDoorTrapActivity.h"
#include "Info/Activities/Team/TeamBreachAndClearActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"

#include "NavigationSystem/Public/NavAreas/NavArea_Obstacle.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "Perception/AISense_Hearing.h"

// TODO: profile scopes

ATrapActor::ATrapActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0f;

	bFindCameraComponentWhenViewTarget = false;
	
	TrapRoot = CreateDefaultSubobject<USceneComponent>("Trap Root");
	TrapRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(TrapRoot);

	TrapMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trap Mesh Component"));
	TrapMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	TrapMeshComponent->SetEnableGravity(false);
	TrapMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrapMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TrapMeshComponent->SetGenerateOverlapEvents(false);
	TrapMeshComponent->bApplyImpulseOnDamage = false;
	TrapMeshComponent->bReplicatePhysicsToAutonomousProxy = false;
	TrapMeshComponent->SetupAttachment(GetRootComponent());
	TrapMeshComponent->bNavigationRelevant = false;
	TrapMeshComponent->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_Cable(TEXT("StaticMesh'/Game/ReadyOrNot/Assets/Trap/SM_TrapWire_2.SM_TrapWire_2'"));
	CableMesh = SM_Cable.Object;

	TripWireTriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Trip Wire Trigger Component"));
	TripWireTriggerComponent->SetRelativeLocation(FVector(70.0f, 0.0f, -10.0f));
	TripWireTriggerComponent->SetBoxExtent(FVector(58.0f, 5.0f, 15.0f));
	TripWireTriggerComponent->SetEnableGravity(false);
	TripWireTriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TripWireTriggerComponent->SetCollisionObjectType(ECC_DOORWAY);
	TripWireTriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TripWireTriggerComponent->SetCollisionResponseToChannel(ECC_DOORWAY, ECR_Block);
	TripWireTriggerComponent->SetGenerateOverlapEvents(false);
	TripWireTriggerComponent->AreaClass = UNavArea_Obstacle::StaticClass();
	TripWireTriggerComponent->bApplyImpulseOnDamage = false;
	TripWireTriggerComponent->bReplicatePhysicsToAutonomousProxy = false;
	TripWireTriggerComponent->SetupAttachment(GetRootComponent());
	TripWireTriggerComponent->bNavigationRelevant = false;
	TripWireTriggerComponent->SetCanEverAffectNavigation(false);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline Component"));
	SplineComponent->SetClosedLoop(false);
	SplineComponent->SetRelativeLocation(FVector::ZeroVector);
	SplineComponent->SetRelativeRotation(FRotator::ZeroRotator);
	SplineComponent->SetupAttachment(GetRootComponent());
	SplineComponent->bNavigationRelevant = false;
	SplineComponent->SetCanEverAffectNavigation(false);
	
	#if WITH_EDITOR
	SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 0.023497f, 0.203665f, 1.0f);
	SplineComponent->bShouldVisualizeScale = true;
	SplineComponent->ScaleVisualizationWidth = 10.0f;
	#endif

	CutCableComponent1 = CreateDefaultSubobject<UCableComponent>(TEXT("Cut Cable Component 1"));
	CutCableComponent1->bAttachStart = true;
	CutCableComponent1->bAttachEnd = true;
	CutCableComponent1->EndLocation = FVector(146.0f, 0.0f, 0.0f);
	CutCableComponent1->CableLength = 128.0f;
	CutCableComponent1->NumSegments = 10;
	CutCableComponent1->SolverIterations = 7;
	CutCableComponent1->SubstepTime = 0.01f;
	CutCableComponent1->bEnableStiffness = true;
	CutCableComponent1->bUseSubstepping = true;
	CutCableComponent1->bSkipCableUpdateWhenNotVisible = false;
	CutCableComponent1->bSkipCableUpdateWhenNotOwnerRecentlyRendered = false;
	CutCableComponent1->bEnableCollision = true;
	CutCableComponent1->CollisionFriction = 1.0f;
	CutCableComponent1->CableGravityScale = 2.0f;
	CutCableComponent1->CableWidth = 0.45f;
	CutCableComponent1->NumSides = 4;
	CutCableComponent1->TileMaterial = 8.0f;
	CutCableComponent1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CutCableComponent1->SetCollisionObjectType(ECC_PhysicsBody);
	CutCableComponent1->SetCollisionResponseToChannels(ECR_Ignore);
	CutCableComponent1->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	CutCableComponent1->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CutCableComponent1->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CutCableComponent1->SetGenerateOverlapEvents(false);
	CutCableComponent1->SetCastShadow(false);
	CutCableComponent1->SetRelativeLocation(FVector::ZeroVector);
	CutCableComponent1->SetupAttachment(SplineComponent);
	CutCableComponent1->bNavigationRelevant = false;
	CutCableComponent1->SetCanEverAffectNavigation(false);

	CutCableComponent2 = CreateDefaultSubobject<UCableComponent>(TEXT("Cut Cable Component 2"));
	CutCableComponent2->bAttachStart = true;
	CutCableComponent2->bAttachEnd = true;
	CutCableComponent2->EndLocation = FVector(146.0f, 0.0f, 0.0f);
	CutCableComponent2->CableLength = 128.0f;
	CutCableComponent2->NumSegments = 10;
	CutCableComponent2->SolverIterations = 7;
	CutCableComponent2->SubstepTime = 0.01f;
	CutCableComponent2->bEnableStiffness = true;
	CutCableComponent2->bUseSubstepping = true;
	CutCableComponent2->bSkipCableUpdateWhenNotVisible = false;
	CutCableComponent2->bSkipCableUpdateWhenNotOwnerRecentlyRendered = false;
	CutCableComponent2->bEnableCollision = true;
	CutCableComponent2->CollisionFriction = 1.0f;
	CutCableComponent2->CableGravityScale = 2.0f;
	CutCableComponent2->CableWidth = 0.45f;
	CutCableComponent2->NumSides = 4;
	CutCableComponent2->TileMaterial = 8.0f;
	CutCableComponent2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CutCableComponent2->SetCollisionObjectType(ECC_PhysicsBody);
	CutCableComponent2->SetCollisionResponseToChannels(ECR_Ignore);
	CutCableComponent2->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	CutCableComponent2->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CutCableComponent2->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CutCableComponent2->SetGenerateOverlapEvents(false);
	CutCableComponent2->SetCastShadow(false);
	CutCableComponent2->SetRelativeLocation(FVector::ZeroVector);
	CutCableComponent2->SetupAttachment(SplineComponent);
	CutCableComponent2->bNavigationRelevant = false;
	CutCableComponent2->SetCanEverAffectNavigation(false);
	
	TrapActivateAudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("Trap Activate Audio Component"));
	TrapActivateAudioComponent->SetRelativeLocation(FVector::ZeroVector);
	TrapActivateAudioComponent->bAutoActivate = false;
	TrapActivateAudioComponent->SetupAttachment(TrapMeshComponent);
	TrapActivateAudioComponent->bNavigationRelevant = false;
	TrapActivateAudioComponent->SetCanEverAffectNavigation(false);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->ShowPromptAtDistance = 150.0f;
	InteractableComponent->RequiredLookAtPercentage = 0.98f;
	InteractableComponent->bShowIconWhenActionsLocked = true;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bHideUponPlayerMovement = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipWireCutters"));
	InteractableComponent->ActionSlot1.DisallowedItems.Empty();
	InteractableComponent->ActionSlot2.Init("Fire", IE_Repeat, FText::FromStringTable("ActionPromptTable", "DisarmTrap"));
	InteractableComponent->ActionSlot3.bUseCustomActionText = true;
	InteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "Disarming");
	InteractableComponent->ActionSlot3.bAnimate = true;
	InteractableComponent->ActionSlot3.bLoopAnimation = true;
	InteractableComponent->SetupAttachment(SplineComponent);
	
	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->ScoreGroupName = "TrapsDisarmed";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;
	ScoringComponent->bAutoAddToScorePool = false;
	ScoringComponent->bEnabled = false;

	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));
	
	bReplicates = true;
	bAlwaysRelevant = true;
}

bool ATrapActor::CanDisarmTrap() const
{
	return TrapStatus == ETrapState::TS_Live || (TrapStatus == ETrapState::TS_Activated && bCanUseMultitoolWhenActivated);
}

void ATrapActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATrapActor, TrapStatus);
}

void ATrapActor::BeginPlay()
{
	Super::BeginPlay();

	PerceptionStimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuliComp->RegisterWithPerceptionSystem();

	if (bInitializeTrapOnBeginPlay)
	{
		TrapInit();
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ATrapActor::UpdateTickRate, 1.0f, true, true);

	InitCable(CutCableComponent1);
	InitCable(CutCableComponent2);

	OriginalEndPositionX = CutCableComponent1->EndLocation.X;
}

void ATrapActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ScoringComponent)
		ScoringComponent->RemoveFromScorePool();
}

void ATrapActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	InteractableComponent->bEnabled = CanDisarmTrap();
	InteractableComponent->UseActor = this;

	TrapMeshComponent->bNavigationRelevant = false;
	TrapMeshComponent->SetCanEverAffectNavigation(false);

	if (LOCAL_PLAYER)
	{
		if (InteractableComponent->IsInteractionEnabledFor(LocalPlayer))
		{
			InteractableComponent->ShowPromptAtDistance = FMath::Lerp(InteractableComponent->ShowPromptAtDistance, LocalPlayer->bIsCrouched ? 150.0f : 200.0f, 5*DeltaSeconds);

			// Disarming progress
			if (const AMultitool* Multitool = LocalPlayer->GetEquippedItem<AMultitool>())
			{
				if (Multitool->CurrentToolKit == EMultitoolFunctions::MF_Wirecutter)
				{
					InteractableComponent->SetAnimatedIconName(Multitool->GetCurrentOperatingTime() > 0.0f ? "Empty" : "Disarm Trap");
					InteractableComponent->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Multitool->GetMaxOperatingTime()), FVector2D(0.0f, 1.0f), Multitool->GetCurrentOperatingTime());
					InteractableComponent->ActionSlot3.bCondition = InteractableComponent->CurrentProgress > 0.0f;
				}
			}
			else
			{
				InteractableComponent->SetAnimatedIconName(CanDisarmTrap() ? "Disarm Trap" : "Empty");
				InteractableComponent->CurrentProgress = 0.0f;
				InteractableComponent->ActionSlot3.bCondition = false;
			}

			InteractableComponent->ActionSlot1.bCondition = CanEquipMultitool(LocalPlayer);
			InteractableComponent->ActionSlot2.bCondition = CanDisarmTrap(LocalPlayer) && InteractableComponent->CurrentProgress <= 0.0f;
		}
	}

	if (!CanDisarmTrap())
	{
		TimeSinceTrapTriggered += DeltaSeconds;

		if (TimeSinceTrapTriggered > 5.0f)
		{
			SetActorTickInterval(1.0f);

			DisableCable(CutCableComponent1);
			DisableCable(CutCableComponent2);
		}

		return;
	}

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (AReadyOrNotCharacter* Character : GS->AllReadyOrNotCharacters)
		{
			if (!Character)
				continue;

			if (Character->GetMesh())
			{
				if (const ASWATCharacter* Swat = Cast<ASWATCharacter>(Character))
				{
					continue;
				}

				if (const ASuspectCharacter* Suspect = Cast<ASuspectCharacter>(Character))
				{
					if (!Suspect->GetCyberneticsController())
						continue;

					if (Suspect->GetCyberneticsController()->GetCurrentActivity<UPlaceTrapActivity>())
						continue;
				}

				const FVector PlayerFootLocation_L = Character->GetMesh()->GetBoneLocation("foot_RI");
				const FVector PlayerFootLocation_R = Character->GetMesh()->GetBoneLocation("foot_LE");

				if (ShouldConsiderTrapCollisionFor(Character))
				{
					//DrawDebugSphere(GetWorld(), PlayerFootLocation_L, 5.0f, 12.0f, FColor::Green, false, DeltaSeconds + 0.02f);
					//DrawDebugSphere(GetWorld(), PlayerFootLocation_R, 5.0f, 12.0f, FColor::Green, false, DeltaSeconds + 0.02f);

					if (UKismetMathLibrary::IsPointInBoxWithTransform(PlayerFootLocation_L, TripWireTriggerComponent->GetComponentTransform(), TripWireTriggerComponent->GetUnscaledBoxExtent()) ||
						UKismetMathLibrary::IsPointInBoxWithTransform(PlayerFootLocation_R, TripWireTriggerComponent->GetComponentTransform(), TripWireTriggerComponent->GetUnscaledBoxExtent()))
					{
						Server_OnTrapTriggered(Character);
						OnTrapTriggered(Character);
					}
				}
			}
		}
	}

	if (TrapStatus == ETrapState::TS_Live)
	{
		if (LOCAL_PLAYER)
		{
			if (CanCutWire() && InteractableComponent->IsInteractionEnabledFor(LocalPlayer) && GEngine->GameViewport)
			{
				InteractableComponent->bEnabled = true;

				// Only update the interactable if we're almost in range, do not do this beyond this distance threshold
				if (FVector::Distance(LocalPlayer->GetActorLocation(), GetActorLocation()) < InteractableComponent->ShowPromptAtDistance * 2.0f)
				{
					FVector2D ViewportSize;
					GEngine->GameViewport->GetViewportSize(ViewportSize);
					
					FVector CameraLocation;
					FVector CameraDirection;
					UGameplayStatics::DeprojectScreenToWorld(LocalPlayer->GetController<APlayerController>(), ViewportSize/2, CameraLocation, CameraDirection);

					const FVector EndLocation = CameraLocation + CameraDirection * FVector::Distance(InteractableComponent->GetComponentLocation(), CameraLocation);
					//const FVector SmoothedLocationOnSpline = UKismetMathLibrary::VLerp(InteractableComponent->GetComponentLocation(), SplineComponent->GetLocationAtSplineInputKey(SplineComponent->FindInputKeyClosestToWorldLocation(EndLocation), ESplineCoordinateSpace::World), 15.0f * DeltaSeconds);

					//DrawDebugLine(GetWorld(), CameraLocation, EndLocation, FColor::Yellow, false, DeltaSeconds + 0.02f);
					
					InteractableComponent->SetWorldLocation(SplineComponent->GetLocationAtSplineInputKey(SplineComponent->FindInputKeyClosestToWorldLocation(EndLocation), ESplineCoordinateSpace::World));
				}
			}
			else
			{
				InteractableComponent->bEnabled = false;
			}
		}
	}
	else
	{
		InteractableComponent->ResetToOriginalLocation();
	}

	CutCable(InteractableComponent->GetRelativeLocation().X/OriginalEndPositionX);
}

void ATrapActor::PostLoad()
{
	Super::PostLoad();
	
	CutCableComponent1->SetMaterial(0, CableMaterial);
	CutCableComponent2->SetMaterial(0, CableMaterial);
}

#if WITH_EDITOR
void ATrapActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "CableMaterial")
	{
		CutCableComponent1->SetMaterial(0, CableMaterial);
		CutCableComponent2->SetMaterial(0, CableMaterial);
	}
	else if (PropertyChangedEvent.GetPropertyName() == "bSimulateCable")
	{
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			if (bSimulateCable)
			{
				CutCableComponent1->bAttachStart = true;
				CutCableComponent1->bAttachEnd = false;
				CutCableComponent1->SubstepTime = 0.01f;
				CutCableComponent1->bUseSubstepping = true;
				CutCableComponent1->bEnableCollision = true;
				
				CutCableComponent2->bAttachStart = false;
				CutCableComponent2->bAttachEnd = true;
				CutCableComponent2->SubstepTime = 0.01f;
				CutCableComponent2->bUseSubstepping = true;
				CutCableComponent2->bEnableCollision = true;
			}
			else
			{
				CutCableComponent1->bAttachStart = true;
				CutCableComponent1->bAttachEnd = true;
				CutCableComponent1->bEnableCollision = false;
				CutCableComponent1->bEnableCollision = false;
				
				CutCableComponent2->bAttachStart = true;
				CutCableComponent2->bAttachEnd = true;
				CutCableComponent2->bEnableCollision = false;
				CutCableComponent2->bEnableCollision = false;
			}
		}
		else
		{
			bSimulateCable = true;
		}
	}
}
#endif

bool ATrapActor::ShouldConsiderTrapCollisionFor(AReadyOrNotCharacter* InCharacter)
{
	if (!InCharacter)
		return false;
	
	// note(killo): temp fix for suspects triggering own traps
	//if (Cast<ACyberneticCharacter>(InCharacter))
	//	return false;

	return FVector::Distance(InCharacter->GetActorLocation(), GetActorLocation()) < 500.0f;
}

// ICanIssueCommandOn implementation
bool ATrapActor::CanIssueCommand_Implementation() const
{
	return TrapStatus == ETrapState::TS_Live;
}

AActor* ATrapActor::GetCommandActor_Implementation() const
{
	return const_cast<ATrapActor*>(this);
}

bool ATrapActor::CanUseMultitoolNow_Implementation(class AReadyOrNotCharacter* ToolOwner, class AMultitool* Tool, FHitResult TraceHit)
{
	switch (TrapStatus)
	{
		case ETrapState::TS_Activated:
		return bCanUseMultitoolWhenActivated;

		case ETrapState::TS_Disabled:
		return false;

		case ETrapState::TS_Live:
		return true;
	}
	
	return false;
}

EMultitoolFunctions ATrapActor::GetMultitoolUseType_Implementation()
{
	return EMultitoolFunctions::MF_Wirecutter;
}

float ATrapActor::GetMultitoolUseTime_Implementation()
{
	return 3.0f;
}

void ATrapActor::Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner)
{
	if (CanDisarmTrap())
	{
		Multicast_OnTrapDisarmed();
	}
}

void ATrapActor::Client_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner)
{
	if (CanDisarmTrap())
	{
		OnTrapDisarmed();
	}
}

void ATrapActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->Server_EquipMultitool_Implementation(EMultitoolFunctions::MF_Wirecutter);
	}
}

void ATrapActor::Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (AMultitool* Multitool = Cast<AMultitool>(InteractCharacter->GetEquippedItem()))
		{
			if (Multitool->CurrentToolKit == EMultitoolFunctions::MF_Wirecutter)
			{
				Multitool->StartUsingTool(this);
				InteractCharacter->PlayRawVO(VO_SWAT_GENERAL::CALL_DISARM_TRAP);
			}
		}
	}
}

void ATrapActor::EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->StopUsingMultitool(this);
	}
}

void ATrapActor::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	//EndFire_Implementation(InteractInstigator, InInteractableComponent);
}

bool ATrapActor::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (Hit.GetComponent())
	{
		if (UDestructibleDoorChunkComponent* DoorChunk = Cast<UDestructibleDoorChunkComponent>(Hit.GetComponent()))
		{
			return DoorChunk->IsDestroyed() || !DoorChunk->IsVisible();
		}
	}

	return false;
}

UInteractableComponent* ATrapActor::GetInteractableComponent_Implementation() const
{
	return InteractableComponent;
}

UScoringComponent* ATrapActor::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

void ATrapActor::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (!InSenseController)
		return;
	
	if (USuspectsAndCivilianManager* SusCivManager = USuspectsAndCivilianManager::Get(this))
	{
		if (SusCivManager->CanInvestigateTrap(this))
		{
			if (InSenseController->GetCharacter())
				InSenseController->GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::BARK_TRIGGER_ACTIVATED, true);

			SusCivManager->StartedInvestigatingTrap(this);
			InSenseController->InvestigateStimulus(Stimulus);
		}
	}

	InSenseController->AddExposedToStimulusTag(*(TrapTypeToString(TrapType) + "Trap"), GetActorLocation(), false);
}

bool ATrapActor::CanCutWire() const
{
	return true;
}

bool ATrapActor::CanEquipMultitool(APlayerCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return CanDisarmTrap();

	if (const AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
	{
		return Multitool->CurrentToolKit != EMultitoolFunctions::MF_Wirecutter && CanDisarmTrap();
	}

	return (UReadyOrNotFunctionLibrary::IsItemInInventory(PlayerCharacter, EItemCategory::IC_Multitool) && !UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) && CanDisarmTrap());
}

bool ATrapActor::CanDisarmTrap(APlayerCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return CanDisarmTrap();
	
	if (AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
	{
		return Multitool->CurrentToolKit == EMultitoolFunctions::MF_Wirecutter && CanDisarmTrap();
	}

	return UReadyOrNotFunctionLibrary::IsItemEquipped(PlayerCharacter, EItemCategory::IC_Multitool) && CanDisarmTrap();
}

void ATrapActor::Server_OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy)
{
	Multicast_OnTrapTriggered(TriggeredBy);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, this, 1500.0f, "Trap");
}

void ATrapActor::Multicast_OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy)
{
	OnTrapTriggered(TriggeredBy);
}

void ATrapActor::Server_OnTrapDisarmed_Implementation()
{
	Multicast_OnTrapDisarmed();
}

void ATrapActor::Multicast_OnTrapDisarmed_Implementation()
{
	OnTrapDisarmed();
}

void ATrapActor::UpdateTickRate()
{
	if (!CanDisarmTrap())
	{
		SetActorTickInterval(1.0f);
		return;
	}
	
	if (LOCAL_PLAYER)
	{
		const float DistanceToLocalPlayer = FVector::Distance(LocalPlayer->GetActorLocation(), GetActorLocation());
		const float NewTickRate = FMath::GetMappedRangeValueClamped(FVector2D(500.0f, 5000.0f), FVector2D(0.0167f, 0.25f), DistanceToLocalPlayer);
		SetActorTickInterval(NewTickRate);
		CutCableComponent1->SetComponentTickInterval(NewTickRate);
		CutCableComponent2->SetComponentTickInterval(NewTickRate);
		//CutCableComponent1->SetComponentTickEnabled(InteractableComponent->IsFocused());
		//CutCableComponent2->SetComponentTickInterval(InteractableComponent->IsFocused());
	}
}

void ATrapActor::InitCable(UCableComponent* InCableComponent)
{
	if (InCableComponent)
	{
		InCableComponent->SetVisibility(false);
		InCableComponent->SetHiddenInGame(true);
		InCableComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//InCableComponent->SetComponentTickEnabled(false);
		InCableComponent->SetComponentTickInterval(0.0167f);
		InCableComponent->bAttachStart = true;
		InCableComponent->bAttachEnd = true;
		InCableComponent->bEnableCollision = false;
		//InCableComponent->SubstepTime = 1.0f;
	}
}

void ATrapActor::EnableCable(UCableComponent* InCableComponent)
{
	if (InCableComponent)
	{
		// Show dynamic cable mesh
		InCableComponent->bSkipCableUpdateWhenNotVisible = false;
		InCableComponent->bSkipCableUpdateWhenNotOwnerRecentlyRendered = false;
		InCableComponent->bAttachStart = InCableComponent == CutCableComponent1;
		InCableComponent->bAttachEnd = InCableComponent == CutCableComponent2;
		InCableComponent->SubstepTime = 0.01f;
		InCableComponent->bUseSubstepping = true;
		InCableComponent->bEnableCollision = true;
		
		InCableComponent->SetVisibility(true);
		InCableComponent->SetHiddenInGame(false);
		InCableComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		InCableComponent->SetComponentTickEnabled(true);
	}
}

void ATrapActor::DisableCable(UCableComponent* InCableComponent)
{
	if (InCableComponent)
	{
		InCableComponent->bSkipCableUpdateWhenNotVisible = true;
		InCableComponent->bSkipCableUpdateWhenNotOwnerRecentlyRendered = true;
		InCableComponent->bEnableCollision = false;
		InCableComponent->SubstepTime = 1.0f;
		InCableComponent->SetComponentTickEnabled(false);
	}
}

void ATrapActor::CutCable(float Alpha)
{
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	const float CutLocationX = OriginalEndPositionX * Alpha;
	CutCableComponent1->EndLocation.X = CutLocationX;
	CutCableComponent2->SetRelativeLocation(FVector(CutLocationX, CutCableComponent2->GetRelativeLocation().Y, CutCableComponent2->GetRelativeLocation().Z));

	CutCableComponent1->CableLength = CutLocationX;
	CutCableComponent2->CableLength = CutCableComponent2->EndLocation.X * (1.0f-Alpha);
}

void ATrapActor::CutCable(const FVector& InRelativeLocation)
{
	const float Alpha = InRelativeLocation.X/OriginalEndPositionX;
	CutCable(Alpha);

	//ULog::Number(Alpha, "Cut Alpha: ");
	//ULog::Vector(InteractableComponent->GetRelativeLocation(), false, "Relative loc: ");
}

void ATrapActor::TrapInit_Implementation()
{
	TrapStatus = ETrapState::TS_Live;

	if (ScoringComponent)
	{
		//ScoringComponent->RemoveFromScorePool();
		ScoringComponent->AddToScorePool();
	}
}

void ATrapActor::TrapDeInit_Implementation()
{
	if (ScoringComponent)
	{
		ScoringComponent->RemoveFromScorePool();
	}
}

void ATrapActor::OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy)
{
	if (TriggeredBy)
	{
		FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "TrapTriggeredWithName"), FText::FromString(TrapName));
		ScoringComponent->GivePenalty(AScoringManager::PENALTY_TRAP_TRIGGERED, true, ScoreText, 0.3f);

		if (TrapStatus == ETrapState::TS_Live)
		{
			if (!TrapActivateAudioComponent->IsPlaying())
				TrapActivateAudioComponent->Play();
		}

		EnableCable(CutCableComponent1);
		EnableCable(CutCableComponent2);
		
		TripWireTriggerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TripWireTriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	TrapTriggered.Broadcast(this, TriggeredBy);
}

void ATrapActor::OnTrapDisarmed_Implementation(AReadyOrNotCharacter* DisarmedBy)
{
	TrapStatus = ETrapState::TS_Disabled;

	ScoringComponent->GiveAllScores(true, true, FText::FromStringTable("ScoringTable", "TrapDisarmed"));
	
	EnableCable(CutCableComponent1);
	EnableCable(CutCableComponent2);

	TripWireTriggerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TripWireTriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
}
