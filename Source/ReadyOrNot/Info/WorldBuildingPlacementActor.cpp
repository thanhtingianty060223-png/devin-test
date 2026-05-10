// Copyright Void Interactive, 2021

#include "WorldBuildingPlacementActor.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Activities/MoveActivity.h"
#include "Activities/PickupItemActivity.h"
#include "Activities/WorldBuildingActivity.h"

TAutoConsoleVariable<int32> CVarRonDrawWorldBuildingActors(TEXT("a.RonDrawWorldBuildingActors"), 1, TEXT("0 = No draw world building actors. 1 = Draw all world building actors"));

AWorldBuildingPlacementActor::AWorldBuildingPlacementActor()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
	#endif

	DefaultScene = CreateDefaultSubobject<USceneComponent>("DefaultScene");
	SetRootComponent(DefaultScene);

	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> WorldBuildActivityIcon(TEXT("Texture2D'/Game/Blueprints/Widgets/Editor/MapIcons/Hotel.Hotel'"));
	
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->ScreenSize = 0.0025f;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Movable); // To get rid of PIE warnings, if mobility was set to Static
	BillboardComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 15.0f));
	BillboardComponent->SetRelativeScale3D(FVector(0.5f));
	BillboardComponent->SetSprite(WorldBuildActivityIcon.Object);
	BillboardComponent->SetupAttachment(GetRootComponent());
	
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	ArrowComponent->SetupAttachment(GetRootComponent());
	#endif
}

void AWorldBuildingPlacementActor::GenerateActivities()
{
	if (!ActivityInstance)
	{
		ActivityInstance = UActivityManager::CreateActivity<UWorldBuildingActivity>(this, Activity);
		ActivityInstance->OnFinishActivity.AddDynamic(this, &AWorldBuildingPlacementActor::OnWorldBuildingActivityFinished);
	}

	if (!MoveToActivityInstance)
	{
		MoveToActivityInstance = UActivityManager::CreateActivity<UMoveActivity>(this);
	}
}

void AWorldBuildingPlacementActor::BeginPlay()
{
	Super::BeginPlay();

	GenerateActivities();
}

void AWorldBuildingPlacementActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
	#endif
	
	SetActorTickEnabled(false);
	SetActorTickInterval(1.0f);

	PrimaryActorTick.UnRegisterTickFunction();
}

void AWorldBuildingPlacementActor::OnWorldBuildingActivityFinished(class UBaseActivity* InActivity, ACyberneticController* CyberneticController)
{
	InUseByController = nullptr;

	if (CyberneticController)
		CyberneticController->NextActivityOnRoute();
}

#if WITH_EDITOR
void AWorldBuildingPlacementActor::EditorTick(float DeltaTime)
{
	if (Activity)
	{
		TInlineComponentArray<UBillboardComponent*> BillboardComponents(this, true);
		GetComponents(BillboardComponents, true);
		for (UBillboardComponent* Billboard : BillboardComponents)
		{
			Billboard->SetVisibility(CVarRonDrawWorldBuildingActors.GetValueOnAnyThread() > 0);
		}

		TInlineComponentArray<UTextRenderComponent*> TextRenderComponents(this, true);
		GetComponents(TextRenderComponents, true);
		for (UTextRenderComponent* TextRender : TextRenderComponents)
		{
			TextRender->SetVisibility(CVarRonDrawWorldBuildingActors.GetValueOnAnyThread() > 0);
		}
	}
}
#endif

bool AWorldBuildingPlacementActor::GiveNextActivityForController(ACyberneticController* Controller, const FActivityRoute& Route)
{
	if (!Controller || !Controller->GetCharacter())
		return false;
	
	if (!Route.bAllowFemale && Controller->GetCharacter()->bFemale)
		return false;
	
	ActivityInstance->ResetData();
	ActivityInstance->SetLocation(GetActorLocation());
	ActivityInstance->SetRotation(GetActorRotation());
	ActivityInstance->WorldBuildingTime = Route.TimeDoingActivity;
	ActivityInstance->bMoveOnly = Route.bMoveOnly;

	UActivityManager::GiveActivityTo(ActivityInstance, Controller->GetCharacter(), false, false);
	
	InUseByController = Controller;

	return true;
}

void AWorldBuildingPlacementActor::GiveActivityForControllerWithoutRoute(ACyberneticController* Controller, const float TimeDoingActivity)
{
	if (!Controller || !Controller->GetCharacter())
		return;

	ActivityInstance->ResetData();
	ActivityInstance->SetLocation(GetActorLocation());
	ActivityInstance->SetRotation(GetActorRotation());
	ActivityInstance->WorldBuildingTime = TimeDoingActivity;
	UActivityManager::GiveActivityTo(ActivityInstance, Controller->GetCharacter(), false, false);

	// move back to original spot
	MoveToActivityInstance->ResetData();
	MoveToActivityInstance->SetLocation(Controller->GetPawn()->GetActorLocation());
	UActivityManager::GiveActivityTo(MoveToActivityInstance, Controller->GetCharacter(), false, false);
}
