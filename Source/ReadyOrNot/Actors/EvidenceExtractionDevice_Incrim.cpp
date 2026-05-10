// Void Interactive, 2020

#include "EvidenceExtractionDevice_Incrim.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/MapActorComponent.h"

#include "Actors/Gameplay/EvidenceActor.h"

AEvidenceExtractionDevice_Incrim::AEvidenceExtractionDevice_Incrim()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinderOptional<UTexture2D> T_Marker(TEXT("Texture2D'/Game/Blueprints/3rdParty/SmartPingSystem/Textures/Icons/GeneralTag2_Icon.GeneralTag2_Icon'"));
	FSlateBrush MarkerIconBrush;
	MarkerIconBrush.SetResourceObject(T_Marker.Get());
	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component"));
	ObjectiveMarkerComponent->InitMarkerSettings(MarkerIconBrush);
	ObjectiveMarkerComponent->SetRelativeLocation(FVector::ZeroVector);
	ObjectiveMarkerComponent->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinderOptional<UTexture2D> T_WayPointMarker(TEXT("Texture2D'/Game/Blueprints/3rdParty/SmartPingSystem/Textures/Icons/T_SPS_Icon_Marker.T_SPS_Icon_Marker'"));
	FSlateBrush WayPointMarkerIconBrush;
	WayPointMarkerIconBrush.SetResourceObject(T_WayPointMarker.Get());
	ObjectiveMarkerComponent_WayPoint = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component Way Point"));
	ObjectiveMarkerComponent_WayPoint->InitMarkerSettings(WayPointMarkerIconBrush);
	ObjectiveMarkerComponent_WayPoint->SetRelativeLocation(FVector::ZeroVector);
	ObjectiveMarkerComponent_WayPoint->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinderOptional<UTexture2D> T_MapMarker(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD_Revised/Matt/BaggingEvidence_Frame1.BaggingEvidence_Frame1'"));
	FSlateBrush MapMarkerIconBrush;
	MapMarkerIconBrush.SetResourceObject(T_MapMarker.Get());
	MapActorComponent = CreateDefaultSubobject<UMapActorComponent>(TEXT("Map Actor Component"));
	MapActorComponent->InitializeMapActorSettings(MapMarkerIconBrush, FColor::White, FText::FromString("Extract"));
	ObjectiveMarkerComponent_WayPoint->SetupAttachment(RootComponent);
}

void AEvidenceExtractionDevice_Incrim::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}

bool AEvidenceExtractionDevice_Incrim::CanStartExtraction() const
{
	if (GetWorld())
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			if (IncrimGS->ChosenEvidenceActor)
				return IncrimGS->IntelState == EEvidenceActorState::Collected && !IncrimGS->bIntelExtracted && IncrimGS->ChosenEvidenceActor->GetPickupInstigator() == UGameplayStatics::GetPlayerCharacter(this, 0);

			return false;
		}
	}

	return Super::CanStartExtraction();
}

bool AEvidenceExtractionDevice_Incrim::CanCollectEvidence() const
{
	if (GetWorld())
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			return IncrimGS->IntelState == EEvidenceActorState::Extraction && IncrimGS->bIntelExtracted && IncrimGS->CurrentExtractionDevice == this;
		}
	}

	return Super::CanCollectEvidence();
}

bool AEvidenceExtractionDevice_Incrim::IsExtracting() const
{
	if (GetWorld())
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			return IncrimGS->IntelState == EEvidenceActorState::Extraction && !IncrimGS->bIntelExtracted && IncrimGS->CurrentExtractionDevice == this;
		}
	}
	
	return Super::IsExtracting();
}

bool AEvidenceExtractionDevice_Incrim::HasEvidenceToExtract() const
{
	if (GetWorld())
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			if (IncrimGS->ChosenEvidenceActor)
				return IncrimGS->IntelState != EEvidenceActorState::Extraction && !IncrimGS->bIntelExtracted && IncrimGS->ChosenEvidenceActor->GetPickupInstigator() != UGameplayStatics::GetPlayerCharacter(this, 0);

			return false;
		}
	}
	
	return Super::HasEvidenceToExtract();
}
