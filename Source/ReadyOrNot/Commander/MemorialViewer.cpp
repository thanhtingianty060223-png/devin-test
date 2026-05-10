// Copyright Void Interactive, 2023

#include "MemorialViewer.h"

#include "CommanderGM.h"
#include "GameModes/LobbyGM.h"

AMemorialViewer::AMemorialViewer()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);
	RootSceneComponent = RootComponent;
	
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>("InteractableComponent");
	InteractableComponent->bClientInteract = true;
	InteractableComponent->bExecuteOnServer = false;
	InteractableComponent->ShowPromptAtDistance = 250.0f;
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->InteractCircleSize = 80.0f;
	InteractableComponent->InteractIconSize = 76.0f;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, NSLOCTEXT("Memorial", "ViewMemorial", "View Memorial"));
	InteractableComponent->ActionSlot1.bAnimate = true;
	InteractableComponent->ActionSlot1.bLoopAnimation = true;
	InteractableComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_View_Alt.T_View_Alt'"));
	
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

void AMemorialViewer::BeginPlay()
{
	Super::BeginPlay();

	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM || !LobbyGM->CommanderProfile)
	{
		InteractableComponent->DisableInteractable();
	}
}

void AMemorialViewer::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!IsValid(InteractInstigator))
		return;

	AReadyOrNotPlayerController* PlayerController = InteractInstigator->GetRONPlayerController();
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
		return;
	
	Widget = PlayerController->CreateWidgetForPlayer("MemorialWidget");

	if (Widget)
		PlayerController->HideHUD();
}

