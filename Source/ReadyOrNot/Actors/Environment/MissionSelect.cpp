// Copyright Void Interactive, 2023

#include "MissionSelect.h"

#include "MissionPortal.h"
#include "HUD/Widgets/MissionSelectWidget.h"

AMissionSelect::AMissionSelect()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_MissionSelect.T_MissionSelect'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>("BillboardComponent");
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

void AMissionSelect::BeginPlay()
{
	Super::BeginPlay();

	PersistentLightingLevel = UGameplayStatics::GetStreamingLevel(GetWorld(), PersistentLightingLevelName);
	RestorePersistentLighting(); // New clients get hidden sublevels replicated to them, ensure level is visible for them
}

void AMissionSelect::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(TH_RemoveCurrentLevel);
	GetWorldTimerManager().ClearTimer(TH_ShowCurrentLevel);
	
	RemoveInFlightLevel();
	RemoveCurrentLevel();
}

void AMissionSelect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCamera(DeltaSeconds);

	FadeTimeRemaining = FMath::Max(FadeTimeRemaining - DeltaSeconds, 0.0f);
}

void AMissionSelect::OpenMissionSelect()
{
	if (bIsOpen)
		return;

	if (!GetWorld())
		return;
	
	PlayerController = Cast<AReadyOrNotPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	if (!PlayerController)
		return;

	PlayerCameraManager = PlayerController->PlayerCameraManager;
	if (!PlayerCameraManager)
		return;

	bIsOpen = true;
	
	PlayerController->HideHUD();
	PlayerController->ChangeInputMode(false, true);
	
	LoadLevel(LosSuenosLevel);
	
	SetActorTickEnabled(true);
}

void AMissionSelect::CloseMissionSelect()
{
	if (!bIsOpen)
		return;
	
	bIsOpen = false;

	
	GetWorldTimerManager().ClearTimer(TH_RemoveCurrentLevel);
	GetWorldTimerManager().ClearTimer(TH_ShowCurrentLevel);
	
	RemoveInFlightLevel();
	RemoveCurrentLevel();

	FadeTimeRemaining = 0.0f;
	if (PlayerCameraManager)
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInTime, FadeColor);

	ViewTargetActor = nullptr;
	if (PlayerController)
	{
		// PlayerController->RemoveWidgetFromStack("MissionSelect");
		PlayerController->ShowHUD();
		PlayerController->ChangeInputMode(true, false);
		
		if (PlayerController->GetPawn())
		{
			PlayerController->SetViewTarget(PlayerController->GetPawn());
		}
	}
	
	RestoreWorldEffects();
	RestorePersistentLighting();

	SetActorTickEnabled(false);
	
	MissionSelectWidget->DeactivateWidget();
	MissionSelectWidget = nullptr; 
}

void AMissionSelect::PreviewMission(ULevelData* LevelData)
{
	if (!LevelData)
		return;

	LoadLevel(LevelData->LevelPreview);
}

void AMissionSelect::SelectMission(ULevelData* LevelData)
{
	if (!LevelData)
		return;
	
	AMissionPortal::SetSelectedMission(LevelData->LevelName.ToString());
	AMissionPortal::SetSelectedMode("BS_COOP");
	
	for (TActorIterator<AMissionPortal> It(GetWorld()); It; ++It)
	{
		AMissionPortal* MissionPortal = *It;
		if (IsValid(MissionPortal))
		{
			MissionPortal->OnMissionSelected();
			break;
		}
	}
}

void AMissionSelect::LoadLevel(TSoftObjectPtr<UWorld> Level)
{
	if (PlayerCameraManager)
	{
		float Start = PlayerCameraManager->FadeAmount;
		PlayerCameraManager->StartCameraFade(Start, 1.0f, FadeOutTime, FadeColor, false, true);
		FadeTimeRemaining = FadeOutTime + FadeHoldTime;
		
		GetWorldTimerManager().SetTimer(TH_RemoveCurrentLevel, FTimerDelegate::CreateUObject(this, &AMissionSelect::RemoveCurrentLevel),
			FadeOutTime, false);
	}

	if (TH_ShowCurrentLevel.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TH_ShowCurrentLevel);
	}
	
	RemoveInFlightLevel();
	
	bool bSuccess;
	InFlightLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(GetWorld(), Level, LevelOffset, FRotator::ZeroRotator, bSuccess);

	if (!InFlightLevel)
	{
		UE_LOG(LogReadyOrNot, Warning, TEXT("MissionSelect failed to load streaming level %s"), *Level.GetAssetName());
		return;
	}

	InFlightLevel->bInitiallyVisible = false;
	InFlightLevel->bInitiallyLoaded = false;

	InFlightLevel->OnLevelLoaded.AddDynamic(this, &AMissionSelect::OnLevelLoaded);
	InFlightLevel->OnLevelShown.AddDynamic(this, &AMissionSelect::OnLevelShown);
	
	InFlightLevel->bShouldBlockOnLoad = bUseBlockingLoad;
	InFlightLevel->SetShouldBeLoaded(true);
	InFlightLevel->SetShouldBeVisible(false);
}

void AMissionSelect::OnLevelLoaded()
{
	if (!ensure(InFlightLevel))
		return;
	
	UE_LOG(LogReadyOrNot, Display, TEXT("MissionSelect level streaming loaded map %s"), *InFlightLevel->GetWorldAssetPackageName());

	// Try to get ahead on texture streaming
	if (InFlightLevel->GetLoadedLevel())
	{
		TArray<AActor*> Actors = InFlightLevel->GetLoadedLevel()->Actors;
		for (AActor* Actor : Actors)
		{
			if (!IsValid(Actor))
				continue;
			
			//Actor->PrestreamTextures(5.0f, false);
		}
	}
	
	// wait until ready
	if (TH_ShowCurrentLevel.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TH_ShowCurrentLevel);
	}
	if (PlayerCameraManager && PlayerCameraManager->FadeTimeRemaining > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TH_ShowCurrentLevel, FTimerDelegate::CreateUObject(this, &AMissionSelect::OnLevelLoaded), FadeTimeRemaining, false);
		return;
	}
	
	InFlightLevel->SetShouldBeVisible(true);
}

void AMissionSelect::OnLevelShown()
{
	if (!ensure(InFlightLevel))
		return;
	
	UE_LOG(LogReadyOrNot, Display, TEXT("MissionSelect level streaming map made visible %s"), *InFlightLevel->GetWorldAssetPackageName());
	
	RemoveCurrentLevel();
	if (TH_RemoveCurrentLevel.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TH_RemoveCurrentLevel);
	}
	
	InFlightLevel->OnLevelShown.RemoveAll(this);
	InFlightLevel->OnLevelLoaded.RemoveAll(this);
	
	CurrentLevel = InFlightLevel;
	InFlightLevel = nullptr;

	/* 
	if (!UBpGameplayHelperLib::HasWidgetInViewport("MissionSelect"))
		PlayerController->CreateWidgetForPlayer("MissionSelect");
		*/

	// CreateWidget<UCommonActivatableWidget>
	const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("MissionSelect", false);
	if (WidgetData.WidgetClass && MissionSelectWidget == nullptr)
	{
		MissionSelectWidget = CreateWidget<UMissionSelectWidget>(GetWorld(), WidgetData.WidgetClass);
		MissionSelectWidget->AddToViewport(100);
	}
	 
	HideWorldEffects();
	HidePersistentLighting();
	FindAndSetViewTarget();

	// wait until ready
	if (TH_ShowCurrentLevel.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TH_ShowCurrentLevel);
	}
	// if (PlayerCameraManager && FadeTimeRemaining > 0.0f)
	// {
	// 	GetWorldTimerManager().SetTimer(TH_ShowCurrentLevel, FTimerDelegate::CreateUObject(this, &AMissionSelect::OnLevelShown), FadeTimeRemaining, false);
	// 	return;
	// }
	
	if (PlayerCameraManager && ViewTargetActor)
	{
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInTime, FadeColor, false, true);
	}
}

void AMissionSelect::RemoveCurrentLevel()
{
	RemoveLevel(CurrentLevel);
	CurrentLevel = nullptr;
}

void AMissionSelect::RemoveInFlightLevel()
{
	RemoveLevel(InFlightLevel);
	InFlightLevel = nullptr;
}

void AMissionSelect::RemoveLevel(ULevelStreamingDynamic* LevelStreaming)
{
	if (!LevelStreaming)
		return;

	LevelStreaming->OnLevelLoaded.RemoveAll(this);
	LevelStreaming->OnLevelShown.RemoveAll(this);
		
	LevelStreaming->SetShouldBeLoaded(false);
	LevelStreaming->SetShouldBeVisible(false);
	LevelStreaming->SetIsRequestingUnloadAndRemoval(true);
}

void AMissionSelect::UpdateCamera(float DeltaSeconds)
{
	if (!ViewTargetActor || !PlayerController)
		return;

	bool bIsUsingGamepad = false;//UReadyOrNotFunctionLibrary::IsUsingGamepad(PlayerController);
	
	int32 ViewPortX, ViewPortY;

	PlayerController->GetViewportSize(ViewPortX, ViewPortY);

	float CameraRangeWidth = 30.0f;
	float CameraRangeHeight = (ViewPortY / static_cast<float>(ViewPortX)) * CameraRangeWidth;
	
	float RightAlpha = 0.0f;
	float UpAlpha = 0.0f;
	
	if (!bIsUsingGamepad)
	{
		float MouseX, MouseY;
		PlayerController->GetMousePosition(MouseX, MouseY);
		
		RightAlpha = FMath::Clamp(MouseX / ViewPortX, 0.0f, 1.0f);
		UpAlpha = FMath::Clamp(MouseY / ViewPortY, 0.0f, 1.0f);
	}
	else 
	{
		float ControlX, ControlY;
		PlayerController->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, ControlX, ControlY);

		RightAlpha = FMath::Clamp((ControlX + 1.0f) / 2.0f, 0.0f, 1.0f);
		UpAlpha = FMath::Clamp((ControlY + 1.0f) / 2.0f, 0.0f, 1.0f);
	}

	FVector Position = OriginalCameraPosition;
	Position += ViewTargetActor->GetActorRightVector() * FMath::Lerp(-CameraRangeWidth, CameraRangeWidth, RightAlpha);
	Position -= ViewTargetActor->GetActorUpVector() * FMath::Lerp(-CameraRangeHeight, CameraRangeHeight, UpAlpha);

	FVector MoveTorwards = FMath::VInterpTo(ViewTargetActor->GetActorLocation(), Position, DeltaSeconds, CameraInterpSpeed);
	ViewTargetActor->SetActorLocation(MoveTorwards);
}

void AMissionSelect::FindAndSetViewTarget()
{
	ViewTargetActor = nullptr;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		ACameraActor* CameraActor = *It;
		if (IsValid(CameraActor) && CameraActor->ActorHasTag(CameraTag))
		{
			ViewTargetActor = CameraActor;
			OriginalCameraPosition = CameraActor->GetActorLocation();
			
			break;
		}
	}
	PlayerController->SetViewTarget(ViewTargetActor);
}

void AMissionSelect::HideWorldEffects()
{
	if (bWorldEffectsHidden)
		return;
	
	bWorldEffectsHidden = true;

	const TArray<AActor*> LevelActors = CurrentLevel->GetLoadedLevel()->Actors;
	
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		ADirectionalLight* DirectionalLight = *It;

		if (LevelActors.Contains(DirectionalLight))
			continue;
		
		// if (DirectionalLight->ActorHasTag(EffectsTag))
		//	continue;

		if (!DirectionalLight->GetLightComponent()->IsVisible())
			continue;
		
		DirectionalLight->SetEnabled(false);
		HiddenDirectionalLights.Add(DirectionalLight);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding DirectionalLight '%s' for MissionSelect"), *GetNameSafe(DirectionalLight));
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		ASkyLight* SkyLight = *It;

		if (LevelActors.Contains(SkyLight))
			continue;
		
		// if (SkyLight->ActorHasTag(EffectsTag))
		// 	continue;
		
		if (!SkyLight->GetLightComponent()->IsVisible())
			continue;
		
		SkyLight->GetLightComponent()->SetVisibility(false);
		HiddenSkyLights.Add(SkyLight);
		
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding SkyLight '%s' for MissionSelect"), *GetNameSafe(SkyLight));
	}
	
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		APostProcessVolume* PostProcessVolume = *It;

		if (LevelActors.Contains(PostProcessVolume))
			continue;
		
		// if (PostProcessVolume->ActorHasTag(EffectsTag))
		// 	continue;

		if (!PostProcessVolume->bEnabled)
			continue;
		
		PostProcessVolume->bEnabled = false;
		HiddenPostProcessVolumes.Add(PostProcessVolume);

		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding PostProcessVolume '%s' for MissionSelect"), *GetNameSafe(PostProcessVolume));
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		AExponentialHeightFog* ExponentialHeightFog = *It;
		
		if (LevelActors.Contains(ExponentialHeightFog))
			continue;
		
		// if (ExponentialHeightFog->ActorHasTag(EffectsTag))
		// 	continue;

		if (!ExponentialHeightFog->GetComponent()->IsVisible())
			continue;
		
		ExponentialHeightFog->GetComponent()->SetVisibility(false);
		HiddenExponentialHeightFogs.Add(ExponentialHeightFog);

		UE_LOG(LogReadyOrNot, Verbose, TEXT("Temporarily hiding ExponentialHeightFog '%s' for MissionSelect"), *GetNameSafe(ExponentialHeightFog));
	}
}

void AMissionSelect::RestoreWorldEffects()
{
	bWorldEffectsHidden = false;
	
	for (ADirectionalLight* DirectionalLight : HiddenDirectionalLights)
	{
		if (!IsValid(DirectionalLight))
			continue;
		
		DirectionalLight->SetEnabled(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored DirectionalLight '%s' for MissionSelect"), *GetNameSafe(DirectionalLight));
	}
	HiddenDirectionalLights.Empty();

	for (ASkyLight* SkyLight : HiddenSkyLights)
	{
		if (!IsValid(SkyLight))
			continue;

		SkyLight->GetLightComponent()->SetVisibility(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored SkyLight '%s' for MissionSelect"), *GetNameSafe(SkyLight));
	}
	HiddenSkyLights.Empty();
	
	for (APostProcessVolume* PostProcessVolume : HiddenPostProcessVolumes)
	{
		if (!IsValid(PostProcessVolume))
			continue;
		
		PostProcessVolume->bEnabled = true;
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored PostProcessVolume '%s' for MissionSelect"), *GetNameSafe(PostProcessVolume));
	}
	HiddenPostProcessVolumes.Empty();

	for (AExponentialHeightFog* ExponentialHeightFog : HiddenExponentialHeightFogs)
	{
		if (!IsValid(ExponentialHeightFog))
			continue;
		
		ExponentialHeightFog->GetComponent()->SetVisibility(true);
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Restored ExponentialHeightFog '%s' for MissionSelect"), *GetNameSafe(ExponentialHeightFog));
	}
	HiddenExponentialHeightFogs.Empty();
}

void AMissionSelect::HidePersistentLighting()
{
	if (!PersistentLightingLevel)
		return;
	
	PersistentLightingLevel->bShouldBlockOnUnload = true;
	PersistentLightingLevel->SetShouldBeVisible(false);
	PersistentLightingLevel->SetShouldBeLoaded(false);
}

void AMissionSelect::RestorePersistentLighting()
{
	if (!PersistentLightingLevel)
		return;

	PersistentLightingLevel->bShouldBlockOnLoad = true;
	PersistentLightingLevel->SetShouldBeLoaded(true);
	PersistentLightingLevel->SetShouldBeVisible(true);
}
