// Void Interactive, 2020

#include "EvidenceExtractionDevice.h"

#include "Components/InteractableComponent.h"

AEvidenceExtractionDevice::AEvidenceExtractionDevice()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> SM_Cube(TEXT("StaticMesh'/Game/Geometry/Meshes/1M_Cube.1M_Cube'"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
	StaticMeshComponent->SetStaticMesh(SM_Cube.Get());
	SetRootComponent(StaticMeshComponent);
	
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->AnimatedIconName = "Empty";
	InteractableComponent->ShowPromptAtDistance = 200.0f;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Start Extraction"));
	InteractableComponent->ActionSlot2.Init("Use", IE_Pressed, FText::FromString("Collect Extracted Evidence"));
	InteractableComponent->ActionSlot3.bUseCustomActionText = true;
	InteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromString("<Red>Extracting...</>");
	InteractableComponent->ActionSlot3.bLoopAnimation = true;
	InteractableComponent->ActionSlot4.bUseCustomActionText = true;
	InteractableComponent->ActionSlot4.CustomActionPromptText = FText::FromString("<Red>Intel</> required to <Red>Extract</>");
	InteractableComponent->SetupAttachment(RootComponent);
}

void AEvidenceExtractionDevice::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	InteractableComponent->ActionSlot1.bCondition = CanStartExtraction();
	InteractableComponent->ActionSlot2.bCondition = CanCollectEvidence();
	InteractableComponent->ActionSlot3.bCondition = IsExtracting();
	InteractableComponent->ActionSlot4.bCondition = HasEvidenceToExtract();
}

void AEvidenceExtractionDevice::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	TryExtractEvidence(Cast<APlayerCharacter>(InteractInstigator));
}

void AEvidenceExtractionDevice::TryExtractEvidence_Implementation(APlayerCharacter* EvidencePossessor)
{
}

bool AEvidenceExtractionDevice::CanStartExtraction() const
{
	return false;
}

bool AEvidenceExtractionDevice::CanCollectEvidence() const
{
	return false;
}

bool AEvidenceExtractionDevice::IsExtracting() const
{
	return false;
}

bool AEvidenceExtractionDevice::HasEvidenceToExtract() const
{
	return false;
}
