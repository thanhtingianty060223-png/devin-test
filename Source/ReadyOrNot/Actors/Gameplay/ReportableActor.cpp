// Copyright Void Interactive, 2023


#include "ReportableActor.h"

#include "Objectives/ReportReportableByTag.h"


// Sets default values
AReportableActor::AReportableActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));
	RootComponent = SceneComponent;
	
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.5f;
	InteractableComponent->ShowPromptAtDistance = 150;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->AnimatedIconName = "Reveal Clue";
	InteractableComponent->InteractIconSize = 200;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->bHideUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Report"));
	InteractableComponent->SetupAttachment(SceneComponent);

	// Default to only needing to overlap the component
	InteractableComponent->bDistanceChecksEnabled = false;
	InteractableComponent->bMustBeLookingAt = false;
	InteractableComponent->bMustBeOverlapping = true;
	InteractableComponent->bOverrideActionPromptUserSettings = true;
	
	ShapeComponent = CreateDefaultSubobject<UShapeComponent>(TEXT("Shape Component"));

	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->ScoreGroupName = "SoftObjectives";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;
	//Default scoring of these to be disabled, as most of the time, only want the objective score showing
	ScoringComponent->bEnabled = false;

#if WITH_EDITORONLY_DATA
	bRunConstructionScriptOnDrag = false;
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_Reportable(TEXT("Texture2D'/Game/ReadyOrNot/UI/Icons/icn_alert_32.icn_alert_32'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Door Open Billboard Component (Front)"));
	BillboardComponent->SetSprite(T_Reportable.Object);
	BillboardComponent->SetWorldScale3D(FVector(1.f));
	BillboardComponent->bIsEditorOnly = true;
	BillboardComponent->SetupAttachment(InteractableComponent);
#endif

	bNetLoadOnClient = true;
	bReplicates = true;
}

void AReportableActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// Bind to this delegate so we know that the objectives have actually been spawned in before we try to decide whether one matches this reportable
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->OnMissionObjectivesUpdated.AddDynamic(this, &AReportableActor::DisableReportableIfNoMatchingObjective);
	}
}

void AReportableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AReportableActor, bReportableEnabled);
}

void AReportableActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (!bReportableEnabled)
	{
		InteractableComponent->bEnabled = false;
	}
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllReportableActors.AddUnique(this);
	}
}

void AReportableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllReportableActors.Remove(this);
	}
}

void AReportableActor::DisableReportableIfNoMatchingObjective()
{
	if (!HasAuthority())
		return;
	
	if (!bDisableOnNoMatchingObjective)
		return;

	for (TActorIterator<AReportReportableByTag> It(GetWorld()); It; ++It)
	{
		AReportReportableByTag* Objective = *It;
		if (Tags.Contains(Objective->ReportTag))
		{
			if ((Objective->bHiddenObjective && bDisableWhileMatchingObjectiveHidden) || bHasBeenReported)
			{
				SetReportableEnabled(false);
			}
			else
			{
				SetReportableEnabled(true);
			}
			
			return;
		}
	}

	SetReportableEnabled(false);
}


void AReportableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AReportableActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!HasAuthority())
		return;
	
	if (!bHasBeenReported && InteractableComponent->bEnabled)
	{
		InteractInstigator->Server_ReportToTOC(this, true, bTocResponseOnReport);
	}
}

bool AReportableActor::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	return false;
}

void AReportableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	switch(Shape)
	{
	case Box:
	{
		UBoxComponent* BoxComponent = Cast<UBoxComponent>(AddComponentByClass(UBoxComponent::StaticClass(), false, FTransform::Identity, false));
		BoxComponent->SetBoxExtent(BoxExtents, true);
		ShapeComponent = BoxComponent;
	}
	break;

	case Sphere:
	{
		USphereComponent* SphereComponent = Cast<USphereComponent>(AddComponentByClass(USphereComponent::StaticClass(), false, FTransform::Identity, false));
		SphereComponent->SetSphereRadius(Radius);
		ShapeComponent = SphereComponent;
	}
	break;

	case Capsule:
	{
		UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(AddComponentByClass(UCapsuleComponent::StaticClass(), false, FTransform::Identity, false));
		CapsuleComponent->SetCapsuleSize(Radius, ZHeight);
		ShapeComponent = CapsuleComponent;
	}
	break;

	default:
		break;
	}
	
	ShapeComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

	ShapeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ShapeComponent->SetCollisionObjectType(ECC_WorldStatic);
	ShapeComponent->SetCollisionProfileName("Trigger", true);
	ShapeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ShapeComponent->SetCanEverAffectNavigation(false);

	ShapeComponent->SetVisibility(true);
	ShapeComponent->SetHiddenInGame(true);
}

void AReportableActor::OnRep_bReportableEnabled()
{
	if (bReportableEnabled)
	{
		InteractableComponent->EnableInteractable();
	}
	else
	{
		InteractableComponent->DisableInteractable();
	}
}

void AReportableActor::SetReportableEnabled(bool bEnable)
{
	if (bEnable)
	{
		bReportableEnabled = true;
		InteractableComponent->EnableInteractable();
	}
	else
	{
		bReportableEnabled = false;
		InteractableComponent->DisableInteractable();
	}
}

bool AReportableActor::CanReportNow_Implementation()
{
	return !bHasBeenReported;
}

FString AReportableActor::GetSpeechTypeForReport_Implementation()
{
	return ReportVoiceLine;
}

void AReportableActor::ReportToTOC_Implementation(AReadyOrNotCharacter* Reporter, bool bPlayAnimation)
{
	FText ScoreText = FText::Format(FText::FromStringTable("ScoringTable", "Reported"), ReportableName);
	ScoringComponent->GiveAllScores(true, true, ScoreText);
	OnReported.Broadcast(this);
	SetReportableEnabled(false);
	bHasBeenReported = true;
}

bool AReportableActor::CanBeSecured_Implementation() const
{
	return !bHasBeenReported;
}

bool AReportableActor::CanBeSecuredByTrailers_Implementation() const
{
	return false;
}

bool AReportableActor::IsSecured_Implementation() const
{
	return bHasBeenReported;
}

void AReportableActor::Secure_Implementation(AReadyOrNotCharacter* InInstigator)
{
	InteractableComponent->bEnabled = true;
	Interact_Implementation(InInstigator, InteractableComponent);
}

FVector AReportableActor::GetLocation_Implementation() const
{
	return GetActorLocation();
}

