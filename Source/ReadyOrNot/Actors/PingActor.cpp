// Void Interactive, 2021

#include "PingActor.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/MapActorComponent.h"

APingActor::APingActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f;

	bReplicates = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultMarkerIcon(TEXT("Texture2D'/Game/Blueprints/3rdParty/SmartPingSystem/Textures/Icons/T_SPS_Icon_Marker.T_SPS_Icon_Marker'"));
	
	IconBrush.SetResourceObject(DefaultMarkerIcon.Object);
	
	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component"));
	ObjectiveMarkerComponent->bEnabled = false;
	ObjectiveMarkerComponent->bDynamic = true;
	ObjectiveMarkerComponent->SetNewFadeDistance(200.0f);
	ObjectiveMarkerComponent->InitMarkerSettings(IconBrush);
	SetRootComponent(ObjectiveMarkerComponent);

	MapActorComponent = CreateDefaultSubobject<UMapActorComponent>(TEXT("Map Actor Component"));
	MapActorComponent->DisableMapActor();
	MapActorComponent->InitializeMapActorSettings(IconBrush);
}

void APingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APingActor, PingedActor);
	DOREPLIFETIME(APingActor, IconBrush);
	DOREPLIFETIME(APingActor, PingText);
}

void APingActor::BeginPlay()
{
	Super::BeginPlay();

	ObjectiveMarkerComponent->bEnabled = true;
	ObjectiveMarkerComponent->bDynamic = true;
	ObjectiveMarkerComponent->SetNewFadeDistance(200.0f);
	ObjectiveMarkerComponent->CreateObjectiveMarkerWidget();

	SetLifeSpan(10.0f);
}

void APingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (PingedActor)
	{
		if (IPingInterface* ActorAsPingInterface = Cast<IPingInterface>(PingedActor))
		{
			ActorAsPingInterface->bPinged = false;
		}
	}
}

void APingActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PingedActor)
	{
		SetActorLocation(IPingInterface::Execute_GetPingLocation(PingedActor));
	}
}

void APingActor::Setup(AActor* InActor)
{
	if (!InActor)
		return;
	
	PingedActor = InActor;
	PingText = IPingInterface::Execute_GetPingText(InActor);
	
	const FSlateBrush NewBrush = IPingInterface::Execute_GetPingIcon(InActor);
	if (NewBrush.GetResourceObject())
	{
		IconBrush = NewBrush;
	}
	
	OnRep_SetIconBrush();
	OnRep_SetPingText();

	SetLifeSpan(IPingInterface::Execute_GetPingDuration(InActor));
}

void APingActor::ToggleObjectiveMarkerVisibility() const
{
	ObjectiveMarkerComponent->ToggleObjectiveMarkerVisibility();
}

void APingActor::ShowObjectiveMarker() const
{
	ObjectiveMarkerComponent->ShowObjectiveMarker();
}

void APingActor::HideObjectiveMarker() const
{
	ObjectiveMarkerComponent->HideObjectiveMarker();
}

void APingActor::OnRep_SetIconBrush()
{
	ObjectiveMarkerComponent->InitMarkerSettings(IconBrush);
	ObjectiveMarkerComponent->SetNewFadeDistance(200.0f);

	MapActorComponent->InitializeMapActorSettings(IconBrush, FColor::White, PingText, FColor::White);
	MapActorComponent->EnableMapActor();
}

void APingActor::OnRep_SetPingText()
{
	MapActorComponent->SetIconText(PingText);
}