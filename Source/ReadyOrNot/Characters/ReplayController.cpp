// Copyright Void Interactive, 2022

#include "Characters/ReplayController.h"

#include "ReplayCameraPawn.h"
#include "Actors/Door.h"
#include "Actors/PairedInteractionDriver.h"
#include "AI/CivilianCharacter.h"
#include "AI/SuspectCharacter.h"
#include "AI/SWATCharacter.h"
#include "Components/DestructibleDoorChunkComponent.h"
#include "Engine/DemoNetDriver.h"	
#include "Engine/InputDelegateBinding.h"
#include "HUD/Widgets/ReplayControls.h"

#include "HUD/Widgets/PauseMenu_Wrapper.h"

AReplayController::AReplayController()
{
	bShouldPerformFullTickWhenPaused = true;
	bIsReplaySpectator = true;
	bDisableCameraShakes = true;
	SetTickableWhenPaused(true);

	CurrentCameraState = Freecam;
}

ASpectatorPawn* AReplayController::SpawnSpectatorPawn()
{
	ASpectatorPawn* SpawnedSpectator = nullptr;

	// Only spawned for the local player
	if ((GetSpectatorPawn() == nullptr) && IsLocalController())
	{
		UWorld* World = GetWorld();
		if (AGameStateBase const* const GameState = World->GetGameState())
		{
			if (true)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				SpawnParams.ObjectFlags |= RF_Transient;	// We never want to save spectator pawns into a map
				SpawnedSpectator = World->SpawnActor<ASpectatorPawn>(AReplayCameraPawn::StaticClass(), GetSpawnLocation(), GetControlRotation(), SpawnParams);
				if (SpawnedSpectator)
				{
					SpawnedSpectator->SetReplicates(false); // Client-side only
					SpawnedSpectator->PossessedBy(this);
					SpawnedSpectator->PawnClientRestart();
					if (SpawnedSpectator->PrimaryActorTick.bStartWithTickEnabled)
					{
						SpawnedSpectator->SetActorTickEnabled(true);
					}

					UE_LOG(LogPlayerController, Verbose, TEXT("Spawned spectator %s [server:%d]"), *GetNameSafe(SpawnedSpectator), GetNetMode() < NM_Client);
				}
				else
				{
					UE_LOG(LogPlayerController, Warning, TEXT("Failed to spawn spectator with class %s"), *GetNameSafe(AReplayCameraPawn::StaticClass()));
				}
			}
		}
		else
		{
			// This normally happens on clients if the Player is replicated but the GameState has not yet.
			UE_LOG(LogPlayerController, Verbose, TEXT("NULL GameState when trying to spawn spectator!"));
		}
	}

	return SpawnedSpectator;
}

void AReplayController::OnFirstDynamicLoad()
{
	ReplayControls->AddToViewport();
	SetInputMode(FInputModeGameAndUI());

	ReplayControls->SetMinimumReplayBarTime(GetCurrentReplayCurrentTimeInSeconds()/GetCurrentReplayTotalTimeInSeconds());

	V_LOGM(LogReadyOrNot, "Tracking %d dynamic replay actors.", SelectableActors.Num());
	NumberTrackedDynamics = SelectableActors.Num();
	
	AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
	if(ReplayCameraPawn && SelectedActor)
	{
		FHitResult Hit;
		FVector ReverseLookVector = ReplayCameraPawn->GetActorForwardVector()*-1;
		FVector ActorLocation = SelectedActor->GetActorLocation();

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(SelectedActor);
		GetWorld()->LineTraceSingleByChannel(Hit, ActorLocation, ActorLocation + 300*ReverseLookVector, ECollisionChannel::ECC_Visibility, QueryParams);

		if(Hit.IsValidBlockingHit())
		{
			ReplayCameraPawn->SetActorLocation( (ActorLocation + Hit.Distance*ReverseLookVector));
		}
		else
		{
			ReplayCameraPawn->SetActorLocation(ActorLocation + 300*ReverseLookVector);
		}
	}

	FString DoFCommand = "r.DepthOfFieldQuality 2";
	if(GEngine)
	{
		GEngine->Exec(GetWorld(), *DoFCommand);
	}
}

void AReplayController::OnDynamicLoad()
{
	RefreshSelectableActors();

	// Apply new selectable actor.
	if(SelectableActors.Num() > SelectedActorIndex)
	{
		SelectedActor = SelectableActors[SelectedActorIndex];
	}
	else if(SelectableActors.Num() > 1)
	{
		SelectedActor = SelectableActors[0];
		SelectedActorIndex = 0;
	}

	// Remove loading screen and set minimum replay bar time.
	UReadyOrNotGameInstance* GameInstance = Cast<UReadyOrNotGameInstance>(GetGameInstance());
	if(GameInstance)
	{
		if(GameInstance->ReplayLoadingScreen)
		{
			GameInstance->RemoveReplayLoadingScreen();
			OnFirstDynamicLoad();
		}
	}

	// Revert to cached states.
	if(CurrentCameraState == Orbit)
	{
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
		if(ReplayCameraPawn)
		{
			ReplayCameraPawn->SpringArm->TargetArmLength = CachedArmLength;
		}
	}
	else if(CurrentCameraState == Mounted)
	{
		FReplaySubMesh ReplaySubMesh = MountedSubMeshes[CachedFirstMountedIndex];
		MountedMesh = ReplaySubMesh.Mesh;
		MountedBoneName = ReplaySubMesh.ReplaySockets[CachedSecondMountedIndex].SocketName;
	}
	bShouldCallPostDynamicLoad = true;
}

void AReplayController::OnPostDynamicLoad()
{
	bShouldCallPostDynamicLoad = false;
	SetPaused(bShouldPauseAfterScrub);	
}

void AReplayController::OnScrubInitiated()
{
	bShouldPauseAfterScrub = bIsPaused;
	
	// Cache data
	if(CurrentCameraState == Mounted)
	{
		for(int i = 0; i < MountedSubMeshes.Num(); i++)
		{
			FReplaySubMesh SubMesh = MountedSubMeshes[i];

			if(SubMesh.Mesh == MountedMesh)
			{
				CachedFirstMountedIndex = i;
				for(int j = 0; j < SubMesh.ReplaySockets.Num(); j ++)
				{
					FReplaySocket ReplaySocket = SubMesh.ReplaySockets[j];
					if(ReplaySocket.SocketName == MountedBoneName)
					{
						CachedSecondMountedIndex = j;
						break;
					}
				}
					
				break;
			}
		}
	}
	// Cache arm length;
	else if(CurrentCameraState == Orbit)
	{
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
		if(ReplayCameraPawn)
		{
			CachedArmLength = ReplayCameraPawn->SpringArm->TargetArmLength;
		}
	}

	// Prevents occasional crash during scrub
	AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
	if(ReplayCameraPawn)
	{
		ReplayCameraPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	
}

void AReplayController::OnChangeSelectedActor()
{
	ApplyNewCameraState();
}

void AReplayController::OnPlayerChangeCameraState(TEnumAsByte<ECameraState> NewState)
{
	if(!bIsFollowingSpline && !(CurrentCameraState == NewState))
	{
		RevertPreviousCameraState();
	}
	CurrentCameraState = NewState;
	ApplyNewCameraState();
}

void AReplayController::RevertPreviousCameraState()
{
	if(CurrentCameraState == Orbit)
	{
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
		if(ReplayCameraPawn)
		{
			ReplayCameraPawn->SpringArm->TargetArmLength = 0;
		}
	}
}


void AReplayController::ApplyCameraState()
{
	// Orbit needs to set the target arm length
	if(CurrentCameraState == Mounted)
	{
		CreateMountData();
	}
}

void AReplayController::ApplyNewCameraState()
{
	// Orbit needs to set the target arm length
	if(CurrentCameraState == Freecam)
	{
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
		if(ReplayCameraPawn && SelectedActor)
		{
			FHitResult Hit;
			FVector ReverseLookVector = ReplayCameraPawn->GetActorForwardVector()*-1;
			FVector ActorLocation = SelectedActor->GetActorLocation() + FVector(0,0,100);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(SelectedActor);
			GetWorld()->LineTraceSingleByChannel(Hit, ActorLocation, ActorLocation + 300*ReverseLookVector, ECollisionChannel::ECC_Visibility, QueryParams);

			if(Hit.IsValidBlockingHit())
			{
				ReplayCameraPawn->SetActorLocation( (ActorLocation + Hit.Distance*ReverseLookVector));
			}
			else
			{
				ReplayCameraPawn->SetActorLocation(ActorLocation + 300*ReverseLookVector);
			}
		}
	}
	else if(CurrentCameraState == Orbit)
	{
		// Snap replay camera to actor and set the spring arm length.
		AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetPawnOrSpectator());
		if(ReplayCameraPawn)
		{
			AddTickPrerequisiteActor(SelectedActor);
			ReplayCameraPawn->SpringArm->TargetArmLength = CachedArmLength;
		}
	}
	else if(CurrentCameraState == Mounted)
	{
		CreateMountData();
		FReplaySubMesh ReplaySubMesh = MountedSubMeshes[0];
		MountedMesh = ReplaySubMesh.Mesh;
		MountedBoneName = ReplaySubMesh.ReplaySockets[0].SocketName;
		MountedLocationOffset = FVector();
		MountedRotationOffset = FRotator();
		AddTickPrerequisiteActor(SelectedActor);
	}
}

void AReplayController::SetupInputComponent()
{
	if (!InputComponent)
	{
		InputComponent = NewObject<UInputComponent>(this, UInputSettings::GetDefaultInputComponentClass(), TEXT("PC_InputComponent0"));
		InputComponent->RegisterComponent();
	}
	if (UInputDelegateBinding::SupportsInputDelegate(GetClass()))
	{
		InputComponent->bBlockInput = bBlockInput;
		UInputDelegateBinding::BindInputDelegates(GetClass(), InputComponent);
	}
	
	InputComponent->BindAction("EscapeMenu", IE_Pressed, this, &AReplayController::EscapeMenu);

	InputComponent->BindAction("ReplayPause", IE_Pressed, this, &AReplayController::PauseReplay);
	
	InputComponent->BindAction("ReplaySkipForward", IE_Pressed, this, &AReplayController::SkipReplayForward);
	InputComponent->BindAction("ReplaySkipBackward", IE_Pressed, this, &AReplayController::SkipReplayBackward);
	
	InputComponent->BindAction("ReplayNextActor", IE_Pressed, this, &AReplayController::NextActor);
	InputComponent->BindAction("ReplayPreviousActor", IE_Pressed, this, &AReplayController::PreviousActor);
	
	InputComponent->BindAction("ReplayToggleHUD", IE_Pressed, this, &AReplayController::ToggleHUD);
}

void AReplayController::PauseReplay()
{
	ReplayControls->PauseReplay();
}

void AReplayController::SkipReplayForward()
{
	ReplayControls->SkipReplayForward();
}

void AReplayController::SkipReplayBackward()
{
	ReplayControls->SkipReplayBackward();
}

void AReplayController::NextActor()
{
	ReplayControls->NextActor();
}

void AReplayController::PreviousActor()
{
	ReplayControls->PreviousActor();
}

void AReplayController::ToggleHUD()
{
	ReplayControls->ToggleHUD();
}

void AReplayController::UpdateRotation(float DeltaTime)
{
		if(GetSpectatorPawn())
		{

			FRotator DeltaRot(RotationInput);
			FRotator ViewRotation = GetSpectatorPawn()->GetActorRotation();//GetControlRotation();
	
			if (PlayerCameraManager)
			{
				PlayerCameraManager->ProcessViewRotation(DeltaTime, ViewRotation, DeltaRot);
			}
			
			SetControlRotation(ViewRotation);
			GetSpectatorPawn()->FaceRotation(ViewRotation, DeltaTime);
		}
}

void AReplayController::EscapeMenu()
{
#ifdef WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.0f, FColor::Magenta, "AReplayController::EscapeMenu()");
#endif

	const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("Escape", false);
	if (WidgetData.WidgetClass)
	{
		PauseMenu = CreateWidget<UPauseMenu_Wrapper>(GetWorld(), WidgetData.WidgetClass);
		PauseMenu->AddToViewport();
		PauseMenu->OnPauseMenuClosed.AddDynamic(this, &AReplayController::OnPauseMenuClosed);
		PauseMenu->OpenPauseMenu();
	}
	return;
	/*
	//If there is something to exit out of.
	if (WidgetStack.Num() > 0)
	{
		UBpGameplayHelperLib::SaveKeybinds();
		const FString RemoveWidget = WidgetStack.Pop();

		if (RemoveWidget == "Escape")
		{
			if(!bIsPausedOnMenuPause)
			{
				SetPaused(false);
			}
			UReadyOrNotFunctionLibrary::PauseFMOD(false);
			
			UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
			
			if(bIsReplayMenuOpen)
			{
				SetInputMode(FInputModeGameAndUI());
				bShowMouseCursor = true;
				gi->bForceShowMouseCursor = true;
			}
			else
			{
				SetInputMode(FInputModeGameOnly());
				bShowMouseCursor = false;
				gi->bForceShowMouseCursor = false;
			}
			
			UBpGameplayHelperLib::RemoveWidgetFromViewport(RemoveWidget);
			return;
		}

		UBpGameplayHelperLib::RemoveWidgetFromViewport(RemoveWidget);

		// If there is a next widget in the stack, we need to focus it.
		if (WidgetStack.Num() > 0)
		{
			// Note: every time we pop and have a widget in the stack, just focus on it. No need to recreate it
			if (UUserWidget* LastWidget = UBpGameplayHelperLib::GetFirstWidgetFromViewport(WidgetStack.Last()))
			{
				const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData(WidgetStack.Last());
				
				if (WidgetData.bMouseUIOnly)
					SetInputMode(FInputModeUIOnly().SetWidgetToFocus(LastWidget->TakeWidget()));
				else if (WidgetData.bShowMouseCursor)
					SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(LastWidget->TakeWidget()));
				else
					SetInputMode(FInputModeGameOnly());
			}
		}
		// If there is no next widget.
		else
		{
			UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
			gi->bForceShowMouseCursor = false;
			bShowMouseCursor = false;
			SetInputMode(FInputModeGameOnly());
			
			if (GetPawn())
			{
				GetPawn()->bBlockInput = false;
			}
		}
	}
	else if (!GetWorld()->GetMapName().Contains("MainMenu"))
	{
		if (UBpGameplayHelperLib::HasWidgetInViewport("Escape"))
		{
			UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
			if (bPreEscapeMenuMouseEnabled)
			{
				gi->bForceShowMouseCursor = true;
				bShowMouseCursor = true;
				SetInputMode(FInputModeGameAndUI());
			}
			else
			{
				gi->bForceShowMouseCursor = false;
				bShowMouseCursor = false;
				SetInputMode(FInputModeGameOnly());
			}
			
			if (GetPawn())
			{
				GetPawn()->bBlockInput = false;
			}
			
			UBpGameplayHelperLib::RemoveWidgetFromViewport("Escape");
			if(!bIsPausedOnMenuPause)
			{
				SetPaused(false);	
			}
			UReadyOrNotFunctionLibrary::PauseFMOD(false);
		}
		else
		{
			Client_CreateWidget("Escape", false, true);
			
			if (GetPawn())
			{
				GetPawn()->bBlockInput = true;
			}

			AWorldSettings* WorldSettings = GetWorldSettings();
			if(WorldSettings && WorldSettings->GetPauserPlayerState())
			{
				bIsPausedOnMenuPause = true;
			}
			else
			{
				bIsPausedOnMenuPause = false;
				SetPaused(true);	
			}
			UReadyOrNotFunctionLibrary::PauseFMOD(true);

		}
	}
	*/
}

bool AReplayController::SetPaused(bool bDoPause)
{
	return SetPausedState(bDoPause, true);
}

bool AReplayController::SetPausedState(bool bDoPause, bool bMuteAudio)
{
	AWorldSettings* WorldSettings = GetWorldSettings();

	// Set MotionBlur off and Anti Aliasing to FXAA in order to bypass the pause-bug of both
	static const auto CVarAA = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AntiAliasing"));

	static const auto CVarMB = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.MotionBlur"));

	if (bDoPause)
	{
		PreviousAASetting = CVarAA->GetInt();
		PreviousMBSetting = CVarMB->GetInt();

		// Set MotionBlur to OFF, Anti-Aliasing to FXAA
		CVarAA->Set(1);
		CVarMB->Set(0);

		WorldSettings->SetPauserPlayerState(PlayerState);
		if(bMuteAudio)
		{
			UReadyOrNotFunctionLibrary::PauseFMOD(true);
		}
		bIsPaused = true;
		return true;
	}
	else
	{
		// Reset MotionBlur and AA but only if we were previously paused
		if (bIsPaused)
		{
			CVarAA->Set(PreviousAASetting);
			CVarMB->Set(PreviousMBSetting);
		}

		WorldSettings->SetPauserPlayerState(nullptr);
		if(bMuteAudio)
		{
			UReadyOrNotFunctionLibrary::PauseFMOD(false);
		}
		bIsPaused = false;
		return false;
	}
}

float AReplayController::GetCurrentReplayTotalTimeInSeconds() const
{
	if (GetWorld())
	{
		if (GetWorld()->GetDemoNetDriver())
		{
			return GetWorld()->GetDemoNetDriver()->GetDemoTotalTime();
		}
	}

	return 0.f;
}

float AReplayController::GetCurrentReplayCurrentTimeInSeconds() const
{
	if (GetWorld())
	{
	
		if (GetWorld()->GetDemoNetDriver())
		{
			return GetWorld()->GetDemoNetDriver()->GetDemoCurrentTime();
		}
	}
	return 0.f;
}

void AReplayController::SetCurrentReplayTimeToSeconds(float Seconds)
{
	if (GetWorld())
	{
		OnScrubInitiated();
		
		if (GetWorld()->GetDemoNetDriver())
		{
			GetWorld()->GetDemoNetDriver()->GotoTimeInSeconds(Seconds);
		}
	}
}

void AReplayController::SetCurrentReplayPlayRate(float PlayRate /*= 1.f*/)
{
	// Ensure > 0 replay rate, otherwise we get a hard freeze.
	if(PlayRate <= 0)
	{
		return;
	}
	
	if (GetWorld())
	{
		if (GetWorld()->GetDemoNetDriver())
		{
			GetWorld()->GetWorldSettings()->DemoPlayTimeDilation = PlayRate;
		}
	}
}

void AReplayController::SetViewOverride()
{
	if (GetWorld())
	{
		if (GetWorld()->GetDemoNetDriver())
		{
			GetWorld()->GetDemoNetDriver()->SetViewerOverride(this);
		}
	}
}

void AReplayController::BeginPlay()
{
	Super::BeginPlay();

	SetViewOverride();
	
	if(!GetWorld()->IsRecordingReplay() && !GetWorld()->IsRecordingClientReplay())
	{

		// Add replay widget to viewport.
		// ##UE5UPGRADE##
		FSoftClassPath WidgetReference(TEXT("/Game/Blueprints/Logic/DataStructures/ReadyOrNotWidget/Replay/W_ReplayControls.W_ReplayControls_C"));
		if (UClass* WidgetClass = WidgetReference.TryLoadClass<UUserWidget>())
		{
			ReplayControls = Cast<UReplayControls>(CreateWidget<UUserWidget>(this, WidgetClass));
		}

		// Create a new replay spline actor.
		ReplaySplineActor = Cast<AReplaySplineActor>(GetWorld()->SpawnActor(AReplaySplineActor::StaticClass()));

		// Bind so that we can detect when a scrubbing attempt is almost complete.
		if(!FNetworkReplayDelegates::OnReplayScrubComplete.IsBoundToObject(this))
		{
			FNetworkReplayDelegates::OnReplayScrubComplete.AddUObject(this, &AReplayController::OnScrubComplete);
		}
	}
}

void AReplayController::Tick(float DeltaSeconds)
{
	// Called here so that it can be called the tick after DynamicLoad
	if(bShouldCallPostDynamicLoad)
	{
		OnPostDynamicLoad();
	}

	// A fast forward/backward just completed, check if we need to preserve attachment state.
	if(bTryVerifyDynamicLoaded)
	{
		if(VerifyScrubComplete())
		{
			bTryVerifyDynamicLoaded = false;
			OnDynamicLoad();
		}
	}
	else
	{
		//Snap camera location to target. Instead of AttachToActor, we do this to keep the rotation separate for Third Person.
		if(CurrentCameraState == Orbit && SelectableActors.Num() > 0 && !bTryVerifyDynamicLoaded && SelectedActor)
		{
			UpdateOrbitTransform();
		}
		else if(CurrentCameraState == Mounted && SelectableActors.Num() > 0 && MountedMesh && MountedBoneName != "")
		{
			UpdateMountedTransform();
		}

		// Replay controls uses tick to detect playback speed UI presses
		if(ReplayControls)
		{
			ReplayControls->CustomTick();
		}
	}
}

void AReplayController::UpdateOrbitTransform()
{
	if(SelectedActor)
	{
		if(AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetSpectatorPawn()))
		{
			if(AReadyOrNotCharacter* Character1 = Cast<AReadyOrNotCharacter>(SelectedActor))
			{
				ReplayCameraPawn->SetActorLocation(Character1->GetMesh()->GetComponentLocation() + FVector(0,0, AdjustableVerticalOffset + 100));
			}
			else
			{
				ReplayCameraPawn->SetActorLocation(SelectedActor->GetActorLocation() + FVector(0,0, AdjustableVerticalOffset));
			}
		}
	}
}

void AReplayController::UpdateMountedTransform()
{
	if(SelectedActor && MountedMesh && MountedBoneName != "")
	{
		if(AReplayCameraPawn* ReplayCameraPawn = Cast<AReplayCameraPawn>(GetSpectatorPawn()))
		{
			ReplayCameraPawn->SetActorLocation(MountedMesh->GetSocketLocation(FName(MountedBoneName)) + MountedLocationOffset);

			FRotator SocketRotation = MountedMesh->GetSocketRotation(FName(MountedBoneName));
			ReplayCameraPawn->SetActorRotation(SocketRotation + MountedRotationOffset);
		}
	}
}

bool AReplayController::VerifyScrubComplete()
{
	UReadyOrNotGameInstance* GameInstance = Cast<UReadyOrNotGameInstance>(GetGameInstance());
	// First time loaded, this method is not reliable, but seems to be so for only the first load of the replay.
	if(GameInstance->ReplayLoadingScreen)
	{
		bool bHasFoundDynamic = false;
		for(TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
		{
			bHasFoundDynamic = true;
			break;
		}
		return bHasFoundDynamic;
	}
	else
	{
		int32 DynamicsFound = 0;

		FixRagdolls(GetWorld());
	
		DynamicsFound += GetAllPlayers().Num();
		DynamicsFound += GetAllSwatAI().Num();
		DynamicsFound += GetAllCivilianAI().Num();
		DynamicsFound += GetAllSuspectAI().Num();
		return DynamicsFound >= NumberTrackedDynamics;
	}
}

/**
 * Attachment
 */
TArray<APlayerCharacter*> AReplayController::GetAllPlayers()
{
	TArray<APlayerCharacter*> Players;
	for(TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* Actor = *It;
		Players.Add(Actor);
	}
	return Players;
}

TArray<ASWATCharacter*> AReplayController::GetAllSwatAI()
{
	TArray<ASWATCharacter*> SwatAI;
	for(TActorIterator<ASWATCharacter> It(GetWorld()); It; ++It)
	{
		ASWATCharacter* Actor = *It;
		SwatAI.Add(Actor);
	}
	return SwatAI;
}

TArray<ACivilianCharacter*> AReplayController::GetAllCivilianAI()
{
	TArray<ACivilianCharacter*> CivilianAI;
	for(TActorIterator<ACivilianCharacter> It(GetWorld()); It; ++It)
	{
		ACivilianCharacter* Actor = *It;
		CivilianAI.Add(Actor);
	}
	return CivilianAI;
}

TArray<ASuspectCharacter*> AReplayController::GetAllSuspectAI()
{
	TArray<ASuspectCharacter*> SuspectAI;
	for(TActorIterator<ASuspectCharacter> It(GetWorld()); It; ++It)
	{
		ASuspectCharacter* Actor = *It;
		SuspectAI.Add(Actor);
	}
	return SuspectAI;
}

void AReplayController::NextSelectableActor()
{
	// This should never happen unless this function is used improperly, but just in case.
	if(SelectableActors.Num() == 0)
	{
		SelectedActor = nullptr;
		return;
	}

	// Increase actor index, but loop around if out of bounds.
	SelectedActorIndex++;
	if(SelectedActorIndex >= SelectableActors.Num())
	{
		SelectedActorIndex = 0;
	}
	SelectedActor = SelectableActors[SelectedActorIndex];
	
	OnChangeSelectedActor();
}

void AReplayController::PreviousSelectableActor()
{
	// This should never happen unless this function is used improperly, but just in case.
	if(SelectableActors.Num() == 0)
	{
		SelectedActor = nullptr;
		return;
	}

	// Decrease actor index, but loop around if out of bounds.
	SelectedActorIndex--;
	if(SelectedActorIndex < 0)
	{
		SelectedActorIndex = SelectableActors.Num()-1;
	}
	SelectedActor = SelectableActors[SelectedActorIndex];
	
	OnChangeSelectedActor();
}

void AReplayController::CreateMountData()
{
	TArray<FReplaySubMesh> ReplaySubMeshes;
	if(AReadyOrNotCharacter* RoNCharacter = Cast<AReadyOrNotCharacter>(SelectedActor))
	{
		//Body
		FReplaySocket LineTraceLE = FReplaySocket("LineTraceLE", "LineTraceLE");
		FReplaySubMesh BodySubMesh = FReplaySubMesh(RoNCharacter->GetMesh(), "Body", {LineTraceLE});

		//Head
		FReplaySocket HeadCamSocket = FReplaySocket("HeadCamSocket", "Head Camera");
		FReplaySocket FirstPersonSocket = FReplaySocket("fp_camera", "First Person Camera");
		FReplaySubMesh HeadSubMesh = FReplaySubMesh(RoNCharacter->GetFaceMesh(), "Head", {HeadCamSocket,FirstPersonSocket});

		
		UInventoryComponent* InvComponent = RoNCharacter->GetInventoryComponent();
		//Primary
		FReplaySocket MuzzleSocket = FReplaySocket("tag_muzzle", "Muzzle");
		FReplaySocket SightSocket = FReplaySocket("tag_sight", "Sight");
		FReplaySubMesh PrimarySubMesh = FReplaySubMesh(InvComponent->GetSpawnedGear().Primary->ItemMesh, InvComponent->GetSpawnedGear().Primary->ItemName.ToString(), {SightSocket, MuzzleSocket});

		//Secondary	
		FReplaySubMesh SecondarySubMesh = FReplaySubMesh(InvComponent->GetSpawnedGear().Secondary->ItemMesh, InvComponent->GetSpawnedGear().Secondary->ItemName.ToString(), {SightSocket, MuzzleSocket});

		ReplaySubMeshes = {HeadSubMesh, BodySubMesh, PrimarySubMesh, SecondarySubMesh};
		MountedSubMeshes = ReplaySubMeshes;
	}
	
	ReplayControls->UpdateMountedSocketSelections();
}

void AReplayController::RefreshSelectableActors()
{
	SelectableActors.Empty();
	SelectableActors.Append(GetAllPlayers());
	SelectableActors.Append(GetAllSwatAI());
	SelectableActors.Append(GetAllSuspectAI());
	SelectableActors.Append(GetAllCivilianAI());
}

FString AReplayController::GetActorName(AActor* Actor)
{
	if(!Actor)
	{
		return FString("None");
	}
	
	// The actors name.
	FString Name;
	if(APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Actor))
	{

		if(PlayerCharacter->GetPlayerState())
		{
			Name = PlayerCharacter->GetPlayerState()->GetPlayerName();
		}
		else
		{
			Name = FString("Player");
		}
	}
	else if(ASWATCharacter* SwatCharacter = Cast<ASWATCharacter>(Actor))
	{
		if(SwatCharacter->GetInventoryComponent() && SwatCharacter->GetInventoryComponent()->GetSpawnedGear().Character)
		{
			Name = SwatCharacter->GetInventoryComponent()->GetSpawnedGear().Character->CharacterName.ToString();
		}
		else
		{
			Name = FString("SWAT");
		}
	}
	else if(ASuspectCharacter* SuspectCharacter = Cast<ASuspectCharacter>(Actor))
	{
		if(SuspectCharacter->GetInventoryComponent() && SuspectCharacter->GetInventoryComponent()->GetSpawnedGear().Character)
		{
			Name = SuspectCharacter->GetInventoryComponent()->GetSpawnedGear().Character->CharacterName.ToString();
		}
		else
		{
			Name = FString("Suspect");
		}
	}
	else if(ACivilianCharacter* CivilianCharacter = Cast<ACivilianCharacter>(Actor))
	{
		if(CivilianCharacter->GetInventoryComponent() && CivilianCharacter->GetInventoryComponent()->GetSpawnedGear().Character)
		{
			Name = CivilianCharacter->GetInventoryComponent()->GetSpawnedGear().Character->CharacterName.ToString();
		}
		else
		{
			Name = FString("Civilian");
		}
	}
	else if(ABaseItem* BaseItem = Cast<ABaseItem>(Actor))
	{
		Name = BaseItem->ItemName.ToString();
	}
	
	return Name;
}


/**
 * Splines
 */
void AReplayController::AddSplinePoint(FVector Location, FRotator Rotation)
{
	if(!ReplaySplineActor)
	{
		return;
	}

	ReplaySplineActor->SplineComponent->AddSplineWorldPoint(Location);
	ReplaySplineActor->SplinePointRotations.Add(Rotation);
}

void AReplayController::RemoveSplinePoint(int32 Index)
{
	if(!ReplaySplineActor)
	{
		return;
	}

	if(! (ReplaySplineActor->SplineComponent->GetNumberOfSplinePoints() > 0) )
	{
		return;
	}

	ReplaySplineActor->SplineComponent->RemoveSplinePoint(Index, true);
	ReplaySplineActor->SplinePointRotations.RemoveAt(Index);
}

void AReplayController::BeginFollowingSpline()
{
	if(!ReplaySplineActor || ReplaySplineActor->SplineComponent->GetNumberOfSplinePoints() < 2)
	{
		return;
	}
	
	bIsFollowingSpline = true;
	V_LOGM(LogReadyOrNot, "Replay camera started following spline.")
}

void AReplayController::StopFollowingSpline()
{
	bIsFollowingSpline = false;
	DeltaSplineTime = 0;
}

TArray<FSplinePoint> AReplayController::GetSplinePoints()
{
	TArray<FSplinePoint> SplinePoints;
	
	if(!ReplaySplineActor)
	{
		return SplinePoints;
	}

	for(int i = 0; i < ReplaySplineActor->SplineComponent->GetNumberOfSplinePoints(); i++)
	{
		FSplinePoint SplinePoint;
		SplinePoint.Position = ReplaySplineActor->SplineComponent->GetTransformAtSplinePoint(i, ESplineCoordinateSpace::World).GetLocation();
		SplinePoint.Rotation = ReplaySplineActor->SplineComponent->GetTransformAtSplinePoint(i, ESplineCoordinateSpace::World).GetRotation().Rotator();
		SplinePoint.Type = ReplaySplineActor->SplineComponent->GetSplinePointType(i);
		SplinePoint.ArriveTangent = ReplaySplineActor->SplineComponent->GetArriveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		SplinePoint.InputKey = i;
		SplinePoint.LeaveTangent = ReplaySplineActor->SplineComponent->GetLeaveTangentAtSplinePoint(i, ESplineCoordinateSpace::World);

		SplinePoints.Add(SplinePoint);
	}

	return SplinePoints;
}

void AReplayController::ClearSplinePoints()
{
	if(!ReplaySplineActor)
	{
		return;
	}
	ReplaySplineActor->SplineComponent->ClearSplinePoints();
	ReplaySplineActor->SplinePointRotations.Empty();
}

/**	
 * Door Fix
 */
void AReplayController::FixDoors(UWorld* World)
{
	for(TActorIterator<ADoor> It(World); It; ++It)
	{
		ADoor* Door = *It;

		// Door chunks
		if(!Door->IsDestructible())
		{
			Door->DestroyAllChunkComponents();
		}
		

		// Door mesh
		if(!Door->IsDoorBroken())
		{
			if(Door->DoorData.DoorMesh){
				Door->GetDoorMesh()->SetStaticMesh(Door->DoorData.DoorMesh);
			}
		}
	}
}

/**	
 * Door Fix
 */
void AReplayController::FixRagdolls(UWorld* World)
{
	for(TActorIterator<AReadyOrNotCharacter> It(World); It; ++It)
	{
		AReadyOrNotCharacter* ReadyOrNotCharacter = *It;

		if ((ReadyOrNotCharacter->IsDeadOrUnconscious() || ReadyOrNotCharacter->IsIncapacitated()) && !ReadyOrNotCharacter->GetMesh()->IsSimulatingPhysics("pelvis"))
		{
			ReadyOrNotCharacter->EnableRagdoll();
		}
	}
}

void AReplayController::OnScrubComplete(UWorld* World)
{
	// The world pointer passed here will remain the same, but non-static objects will have their addresses changed.
	// Anything here is only useful for modifying the state of the static world objects before it is finalized.
	bTryVerifyDynamicLoaded = true;
	bHasPausedAfterScrub = false;
	FixDoors(World);
}