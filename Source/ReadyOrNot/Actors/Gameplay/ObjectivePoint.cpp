// Void Interactive, 2020

#include "ObjectivePoint.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/MapActorComponent.h"

AObjectivePoint::AObjectivePoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.033f;

	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultMarkerIcon(TEXT("Texture2D'/Game/Blueprints/3rdParty/SmartPingSystem/Textures/Icons/T_SPS_Icon_Marker.T_SPS_Icon_Marker'"));
	
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(DefaultMarkerIcon.Object);
	
	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component"));
	ObjectiveMarkerComponent->bEnabled = false;
	ObjectiveMarkerComponent->bDynamic = true;
	ObjectiveMarkerComponent->InitMarkerSettings(IconBrush);
	SetRootComponent(ObjectiveMarkerComponent);

	MapActorComponent = CreateDefaultSubobject<UMapActorComponent>(TEXT("Map Actor Component"));
	MapActorComponent->InitializeMapActorSettings(IconBrush);
}

void AObjectivePoint::InitSettings(const FSlateBrush Icon, const FText Text, const float ShowMarkerAtDistance)
{
	ObjectiveMarkerComponent->InitMarkerSettings(Icon);
	ObjectiveMarkerComponent->SetNewFadeDistance(ShowMarkerAtDistance);
	
	MapActorComponent->InitializeMapActorSettings(Icon, FColor::White, Text, FColor::White);
}

void AObjectivePoint::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(10.0f);

	ObjectiveMarkerComponent->bEnabled = true;
	ObjectiveMarkerComponent->bDynamic = true;
	ObjectiveMarkerComponent->CreateObjectiveMarkerWidget();
}

void AObjectivePoint::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AObjectivePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AObjectivePoint, ObjectiveMarkerComponent);
	DOREPLIFETIME(AObjectivePoint, MapActorComponent);
}

void AObjectivePoint::ToggleObjectiveMarkerVisibility()
{
	ObjectiveMarkerComponent->ToggleObjectiveMarkerVisibility();
}

void AObjectivePoint::ShowObjectiveMarker()
{
	ObjectiveMarkerComponent->ShowObjectiveMarker();
}

void AObjectivePoint::HideObjectiveMarker()
{
	ObjectiveMarkerComponent->HideObjectiveMarker();
}
