// Copyright Void Interactive, 2023

#include "RosterPortal.h"

#include "Commander/CommanderProfile.h"
#include "Components/InteractableComponent.h"
#include "GameModes/LobbyGM.h"

ARosterPortal::ARosterPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.8f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;

	InteractableComponent->bClientInteract = true;
	SetRootComponent(InteractableComponent);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_Portal.T_Portal'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(InteractableComponent);
	}
#endif
}

void ARosterPortal::BeginPlay()
{
	Super::BeginPlay();

	Destroy(); // cut for 1.0 -killo
	
	if (InteractableComponent)
	{
		FText InteractionText = NSLOCTEXT("RosterPortal", "ViewTherapist", "View Therapy");
		
		InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, InteractionText);
		InteractableComponent->ActionSlot1.bCondition = IsRosterAvailable();
	}
}

void ARosterPortal::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator,
	UInteractableComponent* InInteractableComponent)
{
	if (!GetWorld())
		return;
	
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM || !LobbyGM->CommanderProfile)
		return;

	AReadyOrNotPlayerController* PlayerController = GetWorld()->GetFirstPlayerController<AReadyOrNotPlayerController>();
	if (!PlayerController)
		return;

	switch (PortalType)
	{
	case ERosterPortalType::Therapist: PlayerController->Client_CreateWidget("TherapistWidget"); break;
	default: break;
	}
}

bool ARosterPortal::IsRosterAvailable() const
{
	if (!GetWorld() || !HasAuthority())
		return false;
	
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM)
		return false;

	return IsValid(LobbyGM->CommanderProfile);
}

