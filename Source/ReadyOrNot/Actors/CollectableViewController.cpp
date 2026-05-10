// Copyright Void Interactive, 2023

#include "CollectableViewController.h"

#include "Collectable.h"
#include "CollectableViewer.h"
#include "HUD/Widgets/CollectableWidget.h"

ACollectableViewController::ACollectableViewController()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_View.T_View'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(RootComponent);
	}
#endif
}

void ACollectableViewController::BeginPlay()
{
	Super::BeginPlay();

	InputComponent = NewObject<UInputComponent>(this);
	InputComponent->RegisterComponent();
	
	InputComponent->Priority = 1000;
	InputComponent->BindAxis("MoveRight", this, &ACollectableViewController::HorizontalLook);
	InputComponent->BindAxis("MoveForward", this, &ACollectableViewController::VerticalLook);
	InputComponent->BindAxis("LookUpRate", this, &ACollectableViewController::Zoom);
	
	UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport();
	if (GameViewportClient && GameViewportClient->Viewport)
		GameViewportClient->Viewport->ViewportResizedEvent.AddUObject(this, &ACollectableViewController::OnViewportResized);
}

void ACollectableViewController::AddRotationInput(FVector2D Delta)
{
	if (!IsValid(CollectableActor))
		return;

	Delta.X *= -RotationSpeed;
	Delta.Y *= -RotationSpeed;
	CollectableActor->AddActorWorldRotation(FRotator(Delta.Y, Delta.X, 0.0f));
}

void ACollectableViewController::AddZoomInput(float Delta, bool bAnalogue)
{
	if (!IsValid(CollectableActor))
		return;

	if (bAnalogue)
	{
		CurrentScale = FMath::Clamp(CurrentScale + Delta, MinimumScale, MaximumScale);
	}
	else
	{
		float Step = (MaximumScale - MinimumScale) / ScaleLevels;
		float StepDelta = FMath::Sign(Delta) * Step;
		CurrentScale = FMath::Clamp(CurrentScale + StepDelta, MinimumScale, MaximumScale);
		
		if (FMath::IsNearlyEqual(CurrentScale, MinimumScale, Step / 2.0f))
			CurrentScale = MinimumScale;
		if (FMath::IsNearlyEqual(CurrentScale, MaximumScale, Step / 2.0f))
			CurrentScale = MaximumScale;
	}
	
	CollectableActor->SetActorScale3D(FVector(CurrentScale));
}

void ACollectableViewController::OpenViewer(ACollectableViewer* Collectable, AReadyOrNotPlayerController* Controller)
{
	TSubclassOf<ACollectable> CollectableActorClass = Collectable->CollectableClass;
	CurrentController = Controller;
	
	if (!ensure(CurrentController))
		return;

	if (!CameraActor || !SpawnPointActor || !CollectableActorClass)
		return;
		
	CollectableWidget = Cast<UCollectableWidget>(CurrentController->CreateWidgetForPlayer("CollectableWidget"));
	if (!CollectableWidget)
		return;

	HideWorldEffects();
	
	CurrentController->HideHUD();
	CurrentController->SetViewTarget(CameraActor);

	FInputModeGameAndUI InputMode;
	CurrentController->SetInputMode(InputMode);
	CurrentController->bShowMouseCursor = true;

	UGameplayStatics::SetViewportMouseCaptureMode(GetWorld(), EMouseCaptureMode::NoCapture);
	
	if (CurrentController->GetPawn())	
		CurrentController->GetPawn()->DisableInput(CurrentController);
	EnableInput(CurrentController);
	
	CurrentScale = 1.0f;
	float HorizontalFov = CalculateHorizontalFov();
	
	if (IsValid(CameraActor) && CameraActor->GetCameraComponent())
		CameraActor->GetCameraComponent()->SetFieldOfView(HorizontalFov);
	
	if (IsValid(CollectableActor))
		CollectableActor->Destroy();

	FVector SpawnLocation = SpawnPointActor->GetActorLocation();
	CollectableActor = GetWorld()->SpawnActor<ACollectable>(CollectableActorClass, SpawnLocation, FRotator::ZeroRotator);
	
	CollectableWidget->ParentController = this;
	CollectableWidget->SetItem(CollectableActor);
}

void ACollectableViewController::CloseViewer()
{
	if (!CurrentController)
		return;

	RestoreWorldEffects();
	
	CurrentController->RemoveWidgetFromStack("CollectableWidget");
	CurrentController->ShowHUD();
	
	CurrentController->SetViewTarget(CurrentController->GetPawn());

	if (CurrentController->GetPawn())
		CurrentController->GetPawn()->EnableInput(CurrentController);
	DisableInput(CurrentController);
	
	CurrentController = nullptr;
	
	if (IsValid(CollectableActor))
		CollectableActor->Destroy();
}

void ACollectableViewController::HideWorldEffects()
{
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		ADirectionalLight* DirectionalLight = *It;
		if (!DirectionalLight->GetLightComponent()->IsVisible())
			return;
		
		DirectionalLight->SetEnabled(false);
		HiddenDirectionalLights.Add(DirectionalLight);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding DirectionalLight '%s' for CollectableView"), *GetNameSafe(DirectionalLight));
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		ASkyLight* SkyLight = *It;
		if (!SkyLight->GetLightComponent()->IsVisible())
			return;
		
		SkyLight->GetLightComponent()->SetVisibility(false);
		HiddenSkyLights.Add(SkyLight);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding SkyLight '%s' for CollectableView"), *GetNameSafe(SkyLight));
	}
	
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		APostProcessVolume* PostProcessVolume = *It;
		if (!PostProcessVolume->bEnabled)
			return;
		
		PostProcessVolume->bEnabled = false;
		HiddenPostProcessVolumes.Add(PostProcessVolume);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding PostProcessVolume '%s' for CollectableView"), *GetNameSafe(PostProcessVolume));
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		AExponentialHeightFog* ExponentialHeightFog = *It;
		if (!ExponentialHeightFog->GetComponent()->IsVisible())
			return;
		
		ExponentialHeightFog->GetComponent()->SetVisibility(false);
		HiddenExponentialHeightFogs.Add(ExponentialHeightFog);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding ExponentialHeightFog '%s' for CollectableView"), *GetNameSafe(ExponentialHeightFog));
	}
}

void ACollectableViewController::RestoreWorldEffects()
{
	for (ADirectionalLight* DirectionalLight : HiddenDirectionalLights)
	{
		if (!IsValid(DirectionalLight))
			continue;
		
		DirectionalLight->SetEnabled(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored DirectionalLight '%s' for CollectableView"), *GetNameSafe(DirectionalLight));
	}
	HiddenDirectionalLights.Empty();

	for (ASkyLight* SkyLight : HiddenSkyLights)
	{
		if (!IsValid(SkyLight))
			continue;

		SkyLight->GetLightComponent()->SetVisibility(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored SkyLight '%s' for CollectableView"), *GetNameSafe(SkyLight));
	}
	HiddenSkyLights.Empty();
	
	for (APostProcessVolume* PostProcessVolume : HiddenPostProcessVolumes)
	{
		if (!IsValid(PostProcessVolume))
			continue;
		
		PostProcessVolume->bEnabled = true;
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored PostProcessVolume '%s' for CollectableView"), *GetNameSafe(PostProcessVolume));
	}
	HiddenPostProcessVolumes.Empty();

	for (AExponentialHeightFog* ExponentialHeightFog : HiddenExponentialHeightFogs)
	{
		if (!IsValid(ExponentialHeightFog))
			continue;
		
		ExponentialHeightFog->GetComponent()->SetVisibility(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored ExponentialHeightFog '%s' for CollectableView"), *GetNameSafe(ExponentialHeightFog));
	}
	HiddenExponentialHeightFogs.Empty();
}

float ACollectableViewController::CalculateHorizontalFov() const
{
	if (!GetWorld() || !GetWorld()->GetGameViewport() || !CameraActor || !CameraActor->GetCameraComponent())
		return 90.0f;
	
	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	float CurrentFovRadians = FMath::DegreesToRadians(VerticalFov);
	float HorizontalFov = 2.0f * FMath::Atan(FMath::Tan(CurrentFovRadians / 2.0f) * (ViewportSize.X / ViewportSize.Y));
	return FMath::RadiansToDegrees(HorizontalFov);
}

void ACollectableViewController::OnViewportResized(FViewport* Viewport, uint32 Unused)
{
	if (CameraActor && CameraActor->GetCameraComponent())
		CameraActor->GetCameraComponent()->SetFieldOfView(CalculateHorizontalFov());
}