// Copyright Void Interactive, 2023

#include "Actors/Triggers/LoadoutPortal.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/InteractableComponent.h"
#include "HUD/Widgets/PreMissionPlanning.h"
#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"
#include "lib/ReadyOrNotLoadoutManager.h"

ALoadoutPortal::FOnLoadoutOpened ALoadoutPortal::OnLoadoutOpened;
ALoadoutPortal::FOnLoadoutClosed ALoadoutPortal::OnLoadoutClosed;

ALoadoutPortal::ALoadoutPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::AsCultureInvariant("Open Loadout"));
	InteractableComponent->ActionSlot1.bCondition = true;
	InteractableComponent->bClientInteract = true;
	SetRootComponent(InteractableComponent);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_Loadout.T_Loadout'"));
	
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

void ALoadoutPortal::BeginPlay()
{
	Super::BeginPlay();

	const FText ActionText = bOpenCustomization ?
		NSLOCTEXT("Loadout", "OpenCustomization", "Customize Character") : NSLOCTEXT("Loadout", "OpenLoadout", "Modify Loadout");
	
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, ActionText);
	InteractableComponent->ActionSlot1.bCondition = true;
	
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), LightActorsOfTag, OutActors);
	for (AActor* a : OutActors)
	{
		TArray<UStaticMeshComponent*> OutStatics;
		a->GetComponents(OutStatics);
		CompsToOutline.Append(OutStatics);
		TArray<ULightComponent*> OutLights;
		a->GetComponents(OutLights);
		LightsToEnable.Append(OutLights);
	}
}

void ALoadoutPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InteractableComponent->ActionSlot1.bCondition)
	{
		InteractableComponent->IsBeingLookedAt(UReadyOrNotStatics::GetReadyOrNotPlayerController(),
		                                       InteractableComponent->ShowPromptAtDistance,
		                                       InteractableComponent->RequiredLookAtPercentage)
			? DrawOutline()
			: DisableOutline();
	}
}

void ALoadoutPortal::LoadLoadout()
{
	if (!GetWorld())
		return;

	AReadyOrNotPlayerController* PlayerController = GetGameInstance() ? Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()) : nullptr;
	// If we don't have a valid playercameramanager we shouldn't proceed as while it just plays camera fade here, it likely means we don't have a valid controller for this
	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerCameraManager))
		return;
	
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	PlayerController->HideHUD();
	PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 2.0f, 1.0f, FLinearColor::Black, false, true);
	PlayerController->ChangeInputMode(false, true);

	AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	gs->LoadoutFunctionLibrary = NewObject<UReadyOrNotLoadoutManager>();
	gs->LoadoutFunctionLibrary->Initialize(GetWorld());
	gs->LoadoutFunctionLibrary->SetActiveLoadoutByName("default");
	gs->LoadoutFunctionLibrary->SanitizeActiveLoadout();
	// FStringAssetReference SequenceName("/Game/ReadyOrNot/Level/RoN_Station/Loadout/Loadout.Loadout");
	// LevelSequence = Cast<ULevelSequence>(SequenceName.TryLoad());
	// FMovieSceneSequencePlaybackSettings Settings = FMovieSceneSequencePlaybackSettings();
	// LevelSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequence, Settings, LevelSequenceActor);

	if (!GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel)
	{
		// Remove any existing sub premission planning levels as we now load it directly instead of via the levels window
		for (int32 i = 0; i < GetWorld()->GetStreamingLevels().Num(); i++)
		{
			ULevelStreaming* Level = GetWorld()->GetStreamingLevels()[i];
			if (Level->GetWorldAssetPackageName().Contains("SubPreMissionPlanning"))
			{
				Level->SetShouldBeVisible(false);
				GetWorld()->RemoveStreamingLevel(Level);
			}
		}
	}

	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel = NewObject<ULevelStreaming>(
		GetWorld(), ULevelStreamingDynamic::StaticClass(),
		NAME_None,
		RF_NoFlags);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->
	            SetWorldAsset(GetWorld()->GetGameState<AReadyOrNotGameState>()->SubPreMissionPlanningLevel);

	GetWorld()->AddStreamingLevel(GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelLoaded.Clear();
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelLoaded.AddUniqueDynamic(
		this, &ALoadoutPortal::OnLoadoutLoaded);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelShown.Clear();
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelShown.AddUniqueDynamic(
		this, &ALoadoutPortal::OnLoadoutShown);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelHidden.Clear();
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelHidden.AddUniqueDynamic(
		this, &ALoadoutPortal::OnLoadoutHidden);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelUnloaded.Clear();
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->OnLevelUnloaded.AddUniqueDynamic(
		this, &ALoadoutPortal::OnLoadoutUnloaded);
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->SetShouldBeLoaded(true);
}

void ALoadoutPortal::OnLoadoutLoaded()
{
	const AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	GetWorldTimerManager().ClearTimer(FadeTimerHandle);
	// const float FadeTimeRemaining = PlayerController->PlayerCameraManager->FadeTimeRemaining;
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->SetShouldBeVisible(true);
}

void ALoadoutPortal::OnLoadoutFadeIn()
{
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->SetShouldBeVisible(true);
}

void ALoadoutPortal::OnLoadoutShown()
{
	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	if (!PlayerController)
		return;
	
	// If we have a loading screen in the viewport, we shouldn't proceed as it means we're loading a level
	if (PlayerController->HasLoadingScreenInViewport())
		return;
	
	const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("Loadout", false);
	if (WidgetData.WidgetClass)
	{
		LoadoutWidget = CreateWidget<ULoadout_V2>(GetWorld(), WidgetData.WidgetClass);
		GetWorld()->GetGameState<AReadyOrNotGameState>()->Loadout_V2 = LoadoutWidget;
		LoadoutWidget->AddToViewport(1000);
		
		if (bOpenCustomization)
		{
			LoadoutWidget->OpenCustomization();
		}
		
		OnLoadoutOpened.Broadcast();
	}
	
	PlayerController->PlayerCameraManager->StopCameraFade();
	PlayerController->PlayerCameraManager->StartCameraFade(2.0f, 0.0f, 1.0f, FLinearColor::Black, false);
}

void ALoadoutPortal::OnLoadoutHidden()
{
	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	GetWorld()->GetGameState<AReadyOrNotGameState>()->PreMissionStreamedLevel->SetShouldBeLoaded(false);
}

void ALoadoutPortal::OnLoadoutUnloaded()
{
	OnLoadoutClosed.Broadcast();

	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	GameState->LoadoutFunctionLibrary = nullptr;

	GameState->PreMissionStreamedLevel->OnLevelLoaded.Clear();
	GameState->PreMissionStreamedLevel->OnLevelShown.Clear();
	GameState->PreMissionStreamedLevel->OnLevelHidden.Clear();
	GameState->PreMissionStreamedLevel->OnLevelUnloaded.Clear();
	GameState->PreMissionStreamedLevel = nullptr;
	GEngine->ForceGarbageCollection(true);

	GetWorldTimerManager().ClearTimer(FadeTimerHandle);
	GetWorldTimerManager().SetTimer(FadeTimerHandle, this, &ALoadoutPortal::OnLoadoutFadeOut, 2.0f,
	                                false);
}

void ALoadoutPortal::OnLoadoutFadeOut()
{
	if (LoadoutWidget)
	{
		LoadoutWidget->RemoveFromViewport();
		LoadoutWidget = nullptr;
	}
	
	AReadyOrNotPlayerController* PlayerController = UBpGameplayHelperLib::GetLocalRoNPlayerController(GetWorld());
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	IsActive = false;

	PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.0f, FLinearColor::Black, false, false);
	PlayerController->ChangeInputMode(true, false);
	PlayerController->ShowHUD();
}

void ALoadoutPortal::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator,
                                             UInteractableComponent* InInteractableComponent)
{
	if (InteractInstigator->IsLocalPlayer() && !IsActive)
	{
		IsActive = true;
		LoadLoadout();
	}
}

void ALoadoutPortal::DrawOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(true);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnable)
	{
		if (s)
		{
			s->SetVisibility(true);
		}
	}
}

void ALoadoutPortal::DisableOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(false);
			s->SetCustomDepthStencilValue(2);
		}
	}

	for (ULightComponent* s : LightsToEnable)
	{
		if (s)
		{
			s->SetVisibility(false);
		}
	}
}
