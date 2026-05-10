// Copyright Void Interactive, 2021

#include "Optiwand.h"

#include "Animation/RoNWeaponAnimInstance.h"

#include "Components/MirrorPortalComponent.h"
#include "Components/InteractableComponent.h"

#include "Actors/Door.h"
#include "Actors/ThreatAwarenessActor.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"
#include "Characters/CyberneticController.h"
#include "Info/SuspectsAndCivilianManager.h"
#include "Subsystems/AchievementSubsystem.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Optiwand Tick"), STAT_RoNOptiwandTick, STATGROUP_Optiwand);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Check Camera Blocked"), STAT_CheckCameraBlocked, STATGROUP_Optiwand);

AOptiwand::AOptiwand()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	CameraRenderTarget = NewObject<UTextureRenderTarget2D>();
	CameraRenderTarget->AddressX = TA_Wrap;
	CameraRenderTarget->AddressY = TA_Wrap;
	CameraRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	CameraRenderTarget->ClearColor = FColor::White;
	#if WITH_EDITOR
	CameraRenderTarget->MipGenSettings = TMGS_FromTextureGroup;
	#endif
	CameraRenderTarget->TargetGamma = 1.0f;
	CameraRenderTarget->InitAutoFormat(1024, 1024);
	CameraRenderTarget->bAutoGenerateMips = false;
	
	SceneCapture2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture2D"));
	SceneCapture2D->SetupAttachment(GetItemMesh(), "tag_camera");
	SceneCapture2D->FOVAngle = 90.0f;
	SceneCapture2D->CompositeMode = SCCM_Overwrite;
		
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> ObjOutlineMaterial(TEXT("MaterialInstanceConstant'/Game/ReadyOrNot/Assets/Advanced/Postprocess/MI_AIOutline.MI_AIOutline'"));
	MI_AIOutline = ObjOutlineMaterial.Object;
}

void AOptiwand::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOptiwand, bRepMirroring);
}

FHitResult AOptiwand::DoHitFromCamera()
{
	FHitResult Hit;
	GetWorld()->LineTraceSingleByObjectType(Hit, CameraActor->GetActorLocation(), CameraActor->GetActorLocation() + CameraActor->GetActorRotation().Vector() * 5000.0f, FCollisionObjectQueryParams(ECC_Pawn));
	return Hit;
}

void AOptiwand::BeginPlay()
{
	Super::BeginPlay();

	CameraRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	CameraRenderTarget->AddressX = TA_Wrap;
	CameraRenderTarget->AddressY = TA_Wrap;
	CameraRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	CameraRenderTarget->ClearColor = FColor::White;
	CameraRenderTarget->bAutoGenerateMips = false;

#if WITH_EDITOR
	CameraRenderTarget->MipGenSettings = TMGS_FromTextureGroup;
#endif

	CameraRenderTarget->TargetGamma = 1.0f;
	CameraRenderTarget->InitAutoFormat(512, 512);

	CameraActor = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorLocation(), GetActorRotation());

	OriginalSceneCameraRelativeRotation = SceneCapture2D->GetRelativeRotation();
	OriginalFOV = SceneCapture2D->FOVAngle;
}

void AOptiwand::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_RoNOptiwandTick);

	if (!GetOwnerCharacter())
		return;

	if (GetOwnerCharacter()->IsDeadOrUnconscious())
	{
		OnItemPrimaryUseEnd();
		return;
	}

	bMirorring ? TimeMirroring += DeltaSeconds : TimeMirroring = 0.0f;
	
	if (bMirorring && LastUsedDoor)
	{
		if (LastUsedDoor->GetOpenAmount() != DoorRotationOnMirrorStart)
		{
			OnItemPrimaryUseEnd();
		}
	}

	if (!IsEquipped() && bInUse && bMirorring)
		OnItemPrimaryUseEnd();
	
	if (CameraActor)
	{
		CameraActor->SetActorRotation(FRotator(LookAtRotation.Pitch + OptiwandCaptureRotation.Pitch, LookAtRotation.Yaw + OptiwandCaptureRotation.Yaw, 0.0f));
		CameraActor->GetCameraComponent()->AddOrUpdateBlendable(MI_AIOutline, 1.0f);
	}

	if (FMODOptiwandMoveAudioComp)
	{
		if (bMirorring)
		{
			ConsumedMovementAccumulation = UKismetMathLibrary::RInterpTo(ConsumedMovementAccumulation, FRotator::ZeroRotator, DeltaSeconds, 30.0f);
			float MoveAmount = FMath::Abs(ConsumedMovementAccumulation.Yaw) + FMath::Abs(ConsumedMovementAccumulation.Pitch) + FMath::Abs(ConsumedMovementAccumulation.Roll);
			MoveAmount = UKismetMathLibrary::NormalizeToRange(MoveAmount, 0.0f, 10.0f);
		
			//FMODOptiwandMoveAudioComp->SetVolume(FMath::Max(MoveAmount, 1.0f));
			FMODOptiwandMoveAudioComp->SetParameter("OptiwandMove", MoveAmount);
		}
		else
		{
			FMODOptiwandMoveAudioComp->Stop();
			FMODOptiwandMoveAudioComp->DestroyComponent();
			FMODOptiwandMoveAudioComp = nullptr;
		}
	}
	else
	{
		if (bMirorring)
		{
			FMODOptiwandMoveAudioComp = UFMODBlueprintStatics::PlayEventAttached(FMODOptiwandMove, GetItemMesh(), "tag_camera", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
		}
	}

	if (IsEquipped() && GetOwnerPlayerCharacter() && IsLocallyControlled())
	{
		SceneCapture2D->bCaptureEveryFrame = false;
		// match framerate
		if (TimeUntilNextUpdate <= 0.0f)
		{
			float FPS;
			bool bEnabled;
			UBpGameplayHelperLib::LoadPiPFPS(bEnabled, FPS);
			TimeUntilNextUpdate = 1.0f/FPS;
			if (!bEnabled)
			{
				TimeUntilNextUpdate = 0.0f;
			}
			const bool bCameraBlocked = IsCameraBlocked();

			if (!bMirorring)
			{
				if (!UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TH_EndADS))
				{
					SceneCapture2D->PostProcessSettings.bOverride_ColorContrast = true;
					SceneCapture2D->PostProcessSettings.ColorContrast = bCameraBlocked ? FVector::ZeroVector : FVector(1.3f);
				}
			}

			if (bMirorring && LastUsedDoor)
			{
				const UMirrorPortalComponent* MirrorPortalComponent = LastUsedDoor->IsActorInFrontOfDoor(GetOwnerCharacter()) ? LastUsedDoor->GetBackMirrorPoint() : LastUsedDoor->GetFrontMirrorPoint();

				SceneCapture2D->SetRelativeRotation(FRotator::ZeroRotator);
				SceneCapture2D->SetWorldRotation(MirrorPortalComponent->GetComponentRotation() + FRotator(15.0f, -20.0, 0.0f) + OptiwandCaptureRotation);

				//DrawDebugLine(GetWorld(), SceneCapture2D->GetComponentLocation(), SceneCapture2D->GetComponentLocation() + SceneCapture2D->GetComponentRotation().Vector() * 100.0f, FColor::Orange, false, 0.05f, 0, 2.0f);
			}
			else
			{
				SceneCapture2D->SetRelativeRotation(OriginalSceneCameraRelativeRotation + OptiwandCaptureRotation);
			}
			SceneCapture2D->TextureTarget = CameraRenderTarget;
			SceneCapture2D->HideActorComponents(this);
			SceneCapture2D->HideActorComponents(GetOwnerCharacter());
			SceneCapture2D->CaptureSceneDeferred();
			
			if (UMaterialInstanceDynamic* MID_Viewfinder = Cast<UMaterialInstanceDynamic>(ItemMesh->GetMaterial(1)))
			{
				MID_Viewfinder->SetTextureParameterValue("ScreenTexture", bCameraBlocked ? nullptr : SceneCapture2D->TextureTarget);
			}

			//DrawDebugLine(GetWorld(), SceneCapture2D->GetComponentLocation(), SceneCapture2D->GetComponentLocation() + GetOwnerPlayerCharacter()->GetControlRotation().Vector() * 1000.0f, FColor::Purple, false, 0.05f, 0, 0.5f);
			//DrawDebugLine(GetWorld(), SceneCapture2D->GetComponentLocation(), SceneCapture2D->GetComponentLocation() + SceneCapture2D->GetComponentRotation().Vector() * 100.0f, FColor::Purple, false, 0.05f, 0, 2.0f);
			//DrawDebugLine(GetWorld(), ItemMesh->GetComponentLocation(), ItemMesh->GetComponentLocation() + GetOwnerCharacter()->GetControlRotation().Vector() * 1000.0f, FColor::Red, false, 0.05f);
			//DrawDebugSphere(GetWorld(), SceneCapture2D->GetComponentLocation(), 15.0f, 6, FColor::Green, false, 0.05f);

			//ULog::Rotator(OptiwandCaptureRotation, false, "OptiwandCaptureRotation: ");
		}
		else
		{
			TimeUntilNextUpdate -= DeltaSeconds;
		}

		if (bMirorring)
		{
			if (LastUsedDoor)
			{
				if (LastUsedDoor->GetAttachedTrap())
				{
					if (!LastUsedDoor->TeamKnowsDoorTrapState(GetOwnerCharacter()->IsSuspect()) && LastUsedDoor->IsLocationSameSideAsTrap(LookAtPosition))
					{
						const FVector TrapMeshLocation = LastUsedDoor->GetAttachedTrap()->TrapMeshComponent->GetComponentLocation();
						const FVector DirectionToTrapMesh = (TrapMeshLocation - LookAtPosition).GetSafeNormal();

						const float DotProduct = FVector::DotProduct(SceneCapture2D->GetComponentRotation().Vector(), DirectionToTrapMesh);
					
						if (DotProduct > 0.4f)
						{
							LastUsedDoor->SetDoorTrapKnowledge(GetOwnerCharacter()->IsSuspect(), true);
							GetOwnerCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_NO_MIRRORGUN);
						}
					}
				}
				else
				{
					LastUsedDoor->SetDoorTrapKnowledge(GetOwnerCharacter()->IsSuspect(), true);
				}

				if (USuspectsAndCivilianManager::Get(this)->TimeSinceIdleVO_MirrorGun < 0.0f &&
					TimeMirroring > InitialMirrorIdleConvoDelay)
				{
					const bool bFront = LastUsedDoor->IsActorInFrontOfDoorway(GetOwnerCharacter());

					if (LastUsedDoor->BackThreat && LastUsedDoor->FrontThreat)
					{
						if (const FRoom* MirrorRoom = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(bFront ? LastUsedDoor->BackThreat->OwningRoom : LastUsedDoor->FrontThreat->OwningRoom))
						{
							// get all ai in the mirrored room
							TArray<ACyberneticCharacter*, TFixedAllocator<64>> AIInRoom;
							for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
							{
								if (AI->IsActive() && !AI->IsStunned() && AI->GetCyberneticsController()->GetAwarenessState() <= EAIAwarenessState::Suspicious)
								{
									if (const AThreatAwarenessActor* TAA = AI->GetCyberneticsController()->GetTargetingComp()->GetNearestThreat())
									{
										if (TAA->OwningRoom == MirrorRoom->Name)
										{
											AIInRoom.AddUnique(AI);
										}
									}
								}
							}

							// pick a random one to start a conversation
							if (AIInRoom.Num() > 0)
							{
								if (ACyberneticCharacter* RandomAI = AIInRoom[FMath::RandRange(0, AIInRoom.Num() - 1)])
								{
									USuspectsAndCivilianManager::Get(this)->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::IDLE, RandomAI);
									USuspectsAndCivilianManager::Get(this)->TimeSinceIdleVO_MirrorGun = FMath::FRandRange(5.0f, 10.0f);
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		SceneCapture2D->bCaptureEveryFrame = false;
	}
}

void AOptiwand::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (CameraActor)
	{
		CameraActor->Destroy();
	}
}

bool AOptiwand::PlayDraw(bool bDrawFirst)
{
	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();
	if (PlayerCharacter)
	{
		PlayerCharacter->bAiming = false;
	}
	return Super::PlayDraw(bDrawFirst);
}

bool AOptiwand::IsCameraBlocked() const
{
	SCOPE_CYCLE_COUNTER(STAT_CheckCameraBlocked);

	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();

	if (!PlayerCharacter)
		return true;

	if (PlayerCharacter->IsLowReady() || PlayerCharacter->GetEquippedItem() != this || PlayerCharacter->IsDeadOrUnconscious())
		return true;

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, SceneCapture2D->GetComponentLocation() + GetOwnerPlayerCharacter()->GetActorForwardVector() * -100.0f, SceneCapture2D->GetComponentLocation() + GetOwnerPlayerCharacter()->GetControlRotation().Vector() * (CollisionTraceDistance/2.0f), ECC_Visibility, GetOwnerPlayerCharacter()->GetCollisionQueryParameters());
	return Hit.bBlockingHit;
}

void AOptiwand::OnItemPrimaryUse()
{
	bInUse = true;
	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();

	if (!PlayerCharacter)
		return;

	if (IsBlockingAnimationPlaying({}) || UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TH_EndADS))
	{
		return;
	}

	// Mirroring at an interactable object
	if (PlayerCharacter->LastInteractableComponent && PlayerCharacter->LastInteractableComponent->CanInteract())
	{
		AActor* RootActor = PlayerCharacter->LastInteractableComponent->GetUseActor();
		ADoor* Door = Cast<ADoor>(RootActor);

		if (Door && Door->GetOptiwandInteractableComponent() == PlayerCharacter->LastInteractableComponent)
		{	
			if (!Door->CanMirrorUnderDoor(PlayerCharacter))
			{
				if (!IsCameraBlocked() && !Door->GetOptiwandInteractableComponent()->IsFocused())
				{
					MirrorOn(nullptr);
				}
				
				return;
			}

			if (IsCameraBlocked() && !Door->GetOptiwandInteractableComponent()->IsFocused())
				return;

			MirrorOn(Door);
		}
		else
		{
			if (!IsCameraBlocked())
			{
				MirrorOn(nullptr);
			}
		}
	}
	else
	{
		if (!IsCameraBlocked())
		{
			MirrorOn(nullptr);
		}
	}
	Super::OnItemPrimaryUse();
}

void AOptiwand::OnItemPrimaryUseEnd()
{
	bInUse = false;
	
	if (LastUsedDoor)
	{
		LastUsedDoor->OperatingStates.Remove("Optiwand");
	}
	
	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();
	if (!PlayerCharacter)
		return;

	APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);

	if (UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TH_StartADS))
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_StartADS);
		
	if (UReadyOrNotFunctionLibrary::IsCallbackTimerActive(this, TH_StartADSFinished))
	{
		UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_StartADSFinished);

		if (Montage_StartOptiwandADS)
		{
			PlayerCharacter->StopFPAnimMontage(Montage_StartOptiwandADS);
		}

		if (PlayerCameraManager)
		{
			PlayerCameraManager->StopCameraFade();
		}

		PlayerCharacter->bMirroring = false;
		Server_NotifyMirroring(false);
		PlayerCharacter->GetMesh1P()->SetHiddenInGame(false);
		SetItemVisibility(true);

		PlayerCharacter->UnlockMovementAndActions();
		PlayerCharacter->Server_UnlockMovementAndActions();
		
		bMirorring = false;

		return;
	}
	
	if (bMirorring)
	{
		OnItemSecondaryUsed();
	
		PlayerCharacter->UnlockMovementAndActions();
		PlayerCharacter->Server_UnlockMovementAndActions();

		SceneCapture2D->PostProcessSettings.bOverride_ColorContrast = true;
		SceneCapture2D->PostProcessSettings.ColorContrast = FVector(0.0f);

		if (GetViewMode() == EOptiwandViewMode::Fullscreen)
		{
			if (PlayerCameraManager)
			{
				PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.2f, FColor::Black, false, true);
			}

			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_EndADS, this, &AOptiwand::OnEndADSFinished, 0.2f);
		}
		else
		{
			if (Montage_EndOptiwandADS)
			{
				PlayerCharacter->Play1PMontage(Montage_EndOptiwandADS);
				
				UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_EndADS, this, &AOptiwand::OnEndADSFinished, Montage_EndOptiwandADS->GetPlayLength() - 0.2f);
			}
		}
	}

	SceneCapture2D->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "tag_camera");

	PlayerCharacter->bMirroring = false;
	Server_NotifyMirroring(false);
	bMirorring = false;
}

void AOptiwand::OnItemSecondaryUsed()
{
	if (APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter())
	{
		PlayerCharacter->bAiming = false;
		Super::OnItemSecondaryUsed();
	}
}

bool AOptiwand::ConsumeMouseMovement(FRotator RotateVector)
{
	if (bMirorring)
	{
		const float MaxYaw = LastUsedDoor ? 90.0f : 75.0f;

		bool bInvertVertical,  bInvertHorizontal;
		UBpGameplayHelperLib::GetMouseInverted(bInvertVertical, bInvertHorizontal);
		RotateVector.Pitch = -RotateVector.Pitch * (bInvertVertical ? -1.0f : 1.0f);
		RotateVector.Yaw = RotateVector.Yaw * (bInvertHorizontal ? -1.0f : 1.0f);
		RotateVector.Roll = 0.0f;
		
		OptiwandCaptureRotation += RotateVector;

		OptiwandCaptureRotation.Yaw = FMath::Clamp(OptiwandCaptureRotation.Yaw , -MaxYaw, MaxYaw);
		OptiwandCaptureRotation.Pitch = FMath::Clamp(OptiwandCaptureRotation.Pitch , MinCameraPitch, MaxCameraPitch);
	
		ConsumedMovementAccumulation += RotateVector;

		OptiwandCaptureRotation.Normalize();
		
		return true;
	}
	
	return false;
}

EOptiwandViewMode AOptiwand::GetViewMode() const
{
	EOptiwandViewMode ViewMode;
	UBpGameplayHelperLib::LoadOptiwandViewMode(ViewMode);

	return ViewMode;
}

bool AOptiwand::IsBlockingAnimationPlaying(const TArray<EBlockingAnimationExclusion> Exclusions) const
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	
	if (!pc)
		return false;

	if (pc->Is1PMontagePlaying(Montage_StartOptiwandADS) || pc->Is1PMontagePlaying(Montage_EndOptiwandADS))
	{
		return true;
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void AOptiwand::MirrorOn(AActor* Actor)
{
	APlayerCharacter* PlayerCharacter = GetOwnerPlayerCharacter();
	if (!PlayerCharacter)
		return;

	if (GetViewMode() == EOptiwandViewMode::PiP)
	{
		SceneCapture2D->SetRelativeLocation(FVector::ZeroVector);
		SceneCapture2D->SetRelativeRotation(OriginalSceneCameraRelativeRotation);
		
		SceneCapture2D->HideActorComponents(this);
	}
	else
	{
		SceneCapture2D->SetRelativeRotation(FRotator::ZeroRotator);
	}

	// Mirroring in mid-air
	if (!Actor)
	{
		LookAtPosition = SceneCapture2D->GetComponentLocation();
		LookAtRotation = GetOwnerCharacter()->GetControlRotation();
		
		MinCameraPitch = -60.0f;
		MaxCameraPitch = 60.0f;

		LastUsedDoor = nullptr;
	}

	if (ADoor* Door = Cast<ADoor>(Actor))
	{
		const bool bCanMirrorOnDoor = Door && PlayerCharacter->LastInteractableComponent == Door->GetOptiwandInteractableComponent();
		
		if (bCanMirrorOnDoor)
		{
			UMirrorPortalComponent* MirrorPortalComponent = Door->IsActorInFrontOfDoor(GetOwnerCharacter()) ? Door->GetBackMirrorPoint() : Door->GetFrontMirrorPoint();
			if (MirrorPortalComponent)
			{
				LookAtPosition = MirrorPortalComponent->GetComponentLocation();
				LookAtRotation = MirrorPortalComponent->GetComponentRotation();
			}
			
			MinCameraPitch = -10.0f;

			LastUsedDoor = Door;
			DoorRotationOnMirrorStart = Door->GetOpenAmount();
		}
		else
		{
			LookAtPosition = SceneCapture2D->GetComponentLocation();
			LookAtRotation = SceneCapture2D->GetComponentRotation();
		
			MinCameraPitch = -60.0f;

			LastUsedDoor = nullptr;
		}

		if (LastUsedDoor)
		{
			LastUsedDoor->OperatingStates.AddUnique("Optiwand");
		}
		
		if (!LastUsedDoor && IsCameraBlocked())
			return;
	}

	PlayerCharacter->LockMovementAndActions();
	PlayerCharacter->Server_LockMovementAndActions();

	OnItemSecondaryUsed();
   
	CameraActor->SetActorLocation(LookAtPosition);
	CameraActor->SetActorRotation(LookAtRotation);
    
	OptiwandCaptureRotation = FRotator::ZeroRotator;	

	SceneCapture2D->PostProcessSettings.bOverride_ColorContrast = true;
	SceneCapture2D->PostProcessSettings.ColorContrast = FVector(0.0f);

	if (!bMirorring)
	{
		Server_NotifyMirroring(true);
	}

	bMirorring = true;
	
	if (Montage_StartOptiwandADS)
	{
		InitialMirrorIdleConvoDelay = FMath::FRandRange(2.0f, 6.0f);
		
		PlayerCharacter->StopAnimMontage();
		PlayerCharacter->StopFPAnimMontage();
		PlayerCharacter->Play1PMontage(Montage_StartOptiwandADS);

		if (GetViewMode() == EOptiwandViewMode::Fullscreen)
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_StartADS, this, &AOptiwand::OnStartADS, Montage_StartOptiwandADS->GetPlayLength()/2.0f);
		
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_StartADSFinished, this, &AOptiwand::OnStartADSFinished, Montage_StartOptiwandADS->GetPlayLength());
	}

	// Count number of uses
	// Achievement PEEPING_TOM
	if (GetOwnerCharacter() && GetOwnerCharacter()->IsLocalPlayer())
	{
		UAchievementStatics::IncreaseAchievementStat(GetWorld(), EAchievementStats::PROGRESS_MIRRORGUN, 1);
	}
}

void AOptiwand::Server_NotifyMirroring_Implementation(bool bIsMirroring)
{
	bRepMirroring = bIsMirroring;
}

bool AOptiwand::Server_NotifyMirroring_Validate(bool bIsMirroring)
{
	return true;
}

void AOptiwand::OnStartADS()
{
	if (APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PlayerCameraManager->StartCameraFade(0.0f, 1.3f, Montage_StartOptiwandADS->GetPlayLength()/2.0f, FColor::Black, false, true);
	}
}

void AOptiwand::OnStartADSFinished()
{
	if (!GetOwnerCharacter())
		return;
	
	FMODOptiwandEnterViewComp = UFMODBlueprintStatics::PlayEventAttached(FMODOptiwandEnterView, GetItemMesh(), "tag_camera", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);

	if (GetOwnerPlayerCharacter())
	{
		if (UItemVisualizationComponent* DeployableVisComp = GetOwnerPlayerCharacter()->GetLongTacticalVisualizationComponent())
			DeployableVisComp->SetVisibility(false);
	}
	
	if (GetViewMode() == EOptiwandViewMode::Fullscreen)
	{
		// Turn off scanlines
		if (SceneCapture2D->PostProcessSettings.WeightedBlendables.Array.IsValidIndex(1))
		{
			SceneCapture2D->PostProcessSettings.WeightedBlendables.Array[1].Weight = 0.0f;
		}

		//SceneCapture2D->PostProcessSettings.bOverride_ScreenPercentage = true;
		//SceneCapture2D->PostProcessSettings.ScreenPercentage = 60.0f;

		SceneCapture2D->FOVAngle = OriginalFOV/2;

		if (APlayerController* PlayerController = GetOwningPlayerController())
		{
			PlayerController->SetViewTarget(CameraActor);
		}

		if (APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.5f, FColor::Black, false, true);
		}

		if (GetOwnerPlayerCharacter())
		{
			GetOwnerPlayerCharacter()->bMirroring = true;
			GetOwnerPlayerCharacter()->GetMesh1P()->SetHiddenInGame(true);
			SetItemVisibility(false);
		}
	}
	else
	{
		//SceneCapture2D->PostProcessSettings.bOverride_ScreenPercentage = true;
		//SceneCapture2D->PostProcessSettings.ScreenPercentage = 90.0f;

		SceneCapture2D->FOVAngle = OriginalFOV;
	}

	SceneCapture2D->PostProcessSettings.bOverride_ColorContrast = true;
	SceneCapture2D->PostProcessSettings.ColorContrast = FVector(1.3f);
	
	SceneCapture2D->HideActorComponents(GetOwnerCharacter(), true);

	CameraActor->GetCameraComponent()->PostProcessSettings = SceneCapture2D->PostProcessSettings;
	CameraActor->GetCameraComponent()->SetFieldOfView(SceneCapture2D->FOVAngle);
	
	if (LastUsedDoor)
	{
		SceneCapture2D->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		SceneCapture2D->SetWorldLocation(LookAtPosition);
	}
}

void AOptiwand::OnEndADSFinished()
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
		return;

	if (GetOwnerPlayerCharacter())
	{
		if (UItemVisualizationComponent* DeployableVisComp = GetOwnerPlayerCharacter()->GetLongTacticalVisualizationComponent())
			DeployableVisComp->SetVisibility(true);
	}

	// Turn on scanlines
	if (SceneCapture2D->PostProcessSettings.WeightedBlendables.Array.IsValidIndex(1))
	{
		SceneCapture2D->PostProcessSettings.WeightedBlendables.Array[1].Weight = 1.0f;
	}

	SceneCapture2D->PostProcessSettings.bOverride_ColorContrast = true;
	SceneCapture2D->PostProcessSettings.ColorContrast = FVector(1.3f);
	
	//SceneCapture2D->PostProcessSettings.bOverride_ScreenPercentage = true;
	//SceneCapture2D->PostProcessSettings.ScreenPercentage = 90.0f;
	
	SceneCapture2D->FOVAngle = OriginalFOV;

	SceneCapture2D->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "tag_camera");

	LastUsedDoor = nullptr;
	OptiwandCaptureRotation = FRotator::ZeroRotator;

	FMODOptiwandExitViewComp = UFMODBlueprintStatics::PlayEventAttached(FMODOptiwandExitView, GetItemMesh(), "tag_camera", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);

	if (GetViewMode() == EOptiwandViewMode::Fullscreen)
	{
		if (Montage_EndOptiwandADS)
			PlayerCharacter->Play1PMontage(Montage_EndOptiwandADS);

		if (APlayerController* PlayerController = GetOwningPlayerController())
		{
			PlayerController->SetViewTarget(PlayerCharacter);
		}

		if (APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.5f, FColor::Black, false, true);
		}
	}

	if (GetOwnerPlayerCharacter())
	{
		GetOwnerPlayerCharacter()->bMirroring = false;
		Server_NotifyMirroring(false);
		GetOwnerPlayerCharacter()->GetMesh1P()->SetHiddenInGame(false);
		SetItemVisibility(true);
	}
}
