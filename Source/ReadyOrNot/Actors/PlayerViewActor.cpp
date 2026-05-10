// Void Interactive, 2020

#include "PlayerViewActor.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

APlayerViewActor::APlayerViewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	PrimaryActorTick.TickInterval = 0.0f;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> PostProcess_Greyscale_TeamView(TEXT("MaterialInstanceConstant'/Game/ReadyOrNot/Assets/Advanced/Postprocess/MI_Greyscale_Postprocess_TeamView.MI_Greyscale_Postprocess_TeamView'"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> PostProcess_Bump(TEXT("MaterialInstanceConstant'/Game/ReadyOrNot/Assets/Advanced/Postprocess/inst_post_bump_teamview.inst_post_bump_teamview'"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> PostProcess_Glitch(TEXT("MaterialInstanceConstant'/Game/ThirdParty/SciFiGlitchPostPro/PostProcess/Instance/inst_post_glitch.inst_post_glitch'"));

	MI_PostProcess_Greyscale = PostProcess_Greyscale_TeamView.Get();
	MI_PostProcess_Bump = PostProcess_Bump.Get();
	MI_PostProcess_Glitch = PostProcess_Glitch.Get();
	
	// Setup camera capture for FP camera
	CameraCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Camera Capture Component"));
	CameraCaptureComponent->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;
	CameraCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CameraCaptureComponent->CaptureSource = SCS_FinalToneCurveHDR;
	CameraCaptureComponent->bCaptureEveryFrame = false;
	CameraCaptureComponent->bCaptureOnMovement = false;
	CameraCaptureComponent->bAlwaysPersistRenderingState = true;
	CameraCaptureComponent->FOVAngle = 100.0f;
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Greyscale, 1.0f);
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Bump, 1.0f);
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Glitch, 0.0f);
	//CameraCaptureComponent->PostProcessSettings.bOverride_ScreenPercentage = true;
	//CameraCaptureComponent->PostProcessSettings.ScreenPercentage = 25.0f;
	CameraCaptureComponent->PostProcessSettings.bOverride_FilmSlope = true;
	CameraCaptureComponent->PostProcessSettings.FilmSlope = 0.67f;
	CameraCaptureComponent->PostProcessSettings.bOverride_FilmToe = true;
	CameraCaptureComponent->PostProcessSettings.FilmToe = 0.607f;
	CameraCaptureComponent->PostProcessSettings.bOverride_FilmShoulder = true;
	CameraCaptureComponent->PostProcessSettings.FilmShoulder = 0.3838f;
	CameraCaptureComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	CameraCaptureComponent->PostProcessSettings.VignetteIntensity = 0.9f;
	//CameraCaptureComponent->PostProcessSettings.bOverride_GrainJitter = true;
	//CameraCaptureComponent->PostProcessSettings.GrainJitter = 0.1f;
	//CameraCaptureComponent->PostProcessSettings.bOverride_GrainIntensity = true;
	//CameraCaptureComponent->PostProcessSettings.GrainIntensity = 0.28f;
	CameraCaptureComponent->PostProcessSettings.bOverride_ChromaticAberrationStartOffset = true;
	CameraCaptureComponent->PostProcessSettings.ChromaticAberrationStartOffset = 3.714f;
	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	//CameraCaptureComponent->PostProcessSettings.AutoExposureMinBrightness = 0.12f;
	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	//CameraCaptureComponent->PostProcessSettings.AutoExposureMaxBrightness = 0.16f;
	CameraCaptureComponent->SetupAttachment(SceneComponent);

	bDeathEffectsApplied = false;
	bSwitchViewEffectsApplied = false;

	bReplicates = true;
	SetCanBeDamaged(false);
	
	AActor::SetReplicateMovement(true);
}

void APlayerViewActor::SetOwningPlayer(APlayerCharacter* NewOwnerCharacter)
{
	if (!NewOwnerCharacter)
		return;
	
	OwningPlayerCharacter = NewOwnerCharacter;

	ResetDeathEffects();
}

void APlayerViewActor::SetViewPlayer(AReadyOrNotCharacter* NewViewCharacter)
{
	ViewCharacter = NewViewCharacter;

	ResetDeathEffects();
}

bool APlayerViewActor::IsViewInuse(FTransform& OutViewPoint)
{
	if (bShouldCaptureScene)
	{
		OutViewPoint.SetRotation(TargetRotation.Quaternion());
		OutViewPoint.SetLocation(TargetLocation);
		return true;
	}
	return false;
}

void APlayerViewActor::BeginPlay()
{
	Super::BeginPlay();
	
	CameraRenderTarget = NewObject<UTextureRenderTarget2D>();
	CameraRenderTarget->AddressX = TA_Wrap;
	CameraRenderTarget->AddressY = TA_Wrap;
	CameraRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	CameraRenderTarget->ClearColor = FColor::White;
	#if WITH_EDITOR
	CameraRenderTarget->MipGenSettings = TMGS_FromTextureGroup;
	#endif
	CameraRenderTarget->TargetGamma = 1.0f;
	CameraRenderTarget->InitAutoFormat(512, 288);

	CameraCaptureComponent->TextureTarget = CameraRenderTarget;
	CameraCaptureComponent->CompositeMode = SCCM_Overwrite;
	CameraRenderTarget->bAutoGenerateMips = false;

	SetupSwitchViewEffects();
	SetupDeathEffects();
	
	bDeathEffectsApplied = false;
	bSwitchViewEffectsApplied = false;
}

void APlayerViewActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	PrimaryActorTick.TickInterval = 0.0f;

	// Refesh scene
	if (bShouldCaptureScene)
	{		
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		if (PlayerController)
		{
			APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
			if (LocalPlayerCharacter)
			{
				CameraCaptureComponent->HiddenActors.Empty();
				for (ABaseItem* Item : LocalPlayerCharacter->GetInventoryComponent()->GetInventoryItems())
				{
					CameraCaptureComponent->HiddenActors.Add(Item);
				}
			}
		}

		bool bEnabled = false;
		UBpGameplayHelperLib::LoadTeamViewFPSSetting(bEnabled, RefreshRate);
		
		const float RefreshInterval = 1.0f/static_cast<float>(RefreshRate);

		SetActorTickInterval(RefreshInterval);
		
		if (ElapsedTime_CameraRefresh > RefreshInterval || !bEnabled)
		{
			CameraCaptureComponent->UpdateContent();

			ElapsedTime_CameraRefresh = 0.0f;
		} else
		{
			ElapsedTime_CameraRefresh += DeltaTime;
		}

		if (ViewCharacter && ViewCharacter->IsDeadOrUnconscious() && !bDeathEffectsApplied)
		{			
			ApplyDeathEffects();

			if (!IsSwitchingView())
				UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_TryNextView, this, FTimerDelegate::CreateUObject(this, &APlayerViewActor::TryNextView, false, false), DeathViewTime);
		}
	}
	else
	{
		ElapsedTime_CameraRefresh = 0.0f;
	}

	if (bSwitchViewEffectsApplied)
	{
		bSwitchViewEffectsApplied = UReadyOrNotFunctionLibrary::ProcessPostProcessEffect(this, CameraCaptureComponent->PostProcessSettings, SwitchViewEffects, DeltaTime);
	}
}

void APlayerViewActor::Destroyed()
{
	if (CameraRenderTarget)
	{
		CameraRenderTarget = nullptr;

		CameraCaptureComponent->TextureTarget = nullptr;
	}
	
	Super::Destroyed();
}

void APlayerViewActor::SetupSwitchViewEffects()
{
	UReadyOrNotFunctionLibrary::SetupPostProcessEffect(this, SwitchViewEffects);
}

void APlayerViewActor::SetupDeathEffects()
{	
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Glitch, 0.0f);
}

void APlayerViewActor::ApplyDeathEffects()
{
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Glitch, 1.0f);

	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = false;
	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = false;

	DeathViewEventInstance = UFMODBlueprintStatics::PlayEvent2D(this, DeathViewEvent, true);

	bDeathEffectsApplied = true;
}

void APlayerViewActor::ResetDeathEffects()
{
	CameraCaptureComponent->PostProcessSettings.AddBlendable(MI_PostProcess_Glitch, 0.0f);

	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	//CameraCaptureComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;

	UFMODBlueprintStatics::EventInstanceStop(DeathViewEventInstance, true);

	bDeathEffectsApplied = false;
}

void APlayerViewActor::TryNextView(const bool bRequestClose, const bool bIncludeDeadViews)
{
	if (IsSwitchingView())
		return;

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_TryNextView);
	
	if (!bSwitchViewEffectsApplied)
	{
		UReadyOrNotFunctionLibrary::StartPostProcessEffect(this, CameraCaptureComponent->PostProcessSettings, SwitchViewEffects);
		bSwitchViewEffectsApplied = true;

		// Get the maximum time that this effect will play for, basically the lifetime of the entire SwitchViewEffects
		float TimeRemaining = 0.0f;
		for (FPostProcessEffectPlayer& EffectPlayer : SwitchViewEffects.PostProcesses)
		{
			if (EffectPlayer.PostProcess_Data)
			{
				const float EffectTimeRemaining = EffectPlayer.PostProcess_Data->GetGlobalTimeRemaining();
				
				if (EffectTimeRemaining > TimeRemaining)
					TimeRemaining = EffectTimeRemaining;
			}
		}

		if (TimeRemaining > 0.0f)
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &APlayerViewActor::NextView, bRequestClose, bIncludeDeadViews), TimeRemaining/2.0f);
		}
		else
		{
			NextView();
		}

		if (bRequestClose)
		{
			UFMODBlueprintStatics::PlayEvent2D(this, CloseViewEvent, true);
		}
		else
		{
			UFMODBlueprintStatics::PlayEvent2D(this, SwitchViewEvent, true);
		}
	}
}

void APlayerViewActor::NextView(const bool bRequestClose, const bool bIncludeDeadViews)
{
	if (OwningPlayerCharacter)
	{
		OwningPlayerCharacter->NextPlayerView(bRequestClose, bIncludeDeadViews);
	}

	ResetDeathEffects();
}

void APlayerViewActor::UpdateViewTarget(const FVector& NewLocation, const FRotator& NewRotation)
{
	TargetLocation = NewLocation;
	TargetRotation = NewRotation;
}

void APlayerViewActor::HideComponent(UPrimitiveComponent* ComponentToHide) const
{
	CameraCaptureComponent->HideComponent(ComponentToHide);
}

void APlayerViewActor::ClearHiddenComponents() const
{
	CameraCaptureComponent->ClearHiddenComponents();
}

void APlayerViewActor::HideActor(AActor* ActorToHide, const bool bIncludeChildActors) const
{
	CameraCaptureComponent->HideActorComponents(ActorToHide, bIncludeChildActors);
}
