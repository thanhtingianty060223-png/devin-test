// Copyright Void Interactive, 2023

#include "Tablet.h"

#include "Actors/Environment/MissionPortal.h"
#include "Components/WidgetComponent.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"

ATablet::ATablet()
{
	bDisableTickWhenNotEquipped = true;
	
	bShouldTickAnimBPWhenNotEquipped = true;
	ItemMesh->bEnableUpdateRateOptimizations = false;
	ItemMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
}

void ATablet::BeginPlay()
{
	Super::BeginPlay();

	InputComponent = NewObject<UInputComponent>(this);
	InputComponent->RegisterComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("OpenTablet", IE_Pressed, this, &ATablet::OpenTabletPressed);
		InputComponent->BindAction("OpenTablet", IE_Released, this, &ATablet::OpenTabletReleased);
		
		InputComponent->BindAction("TeamView", IE_Pressed, this, &ATablet::TryNextPlayerView_Pressed);
		InputComponent->BindAction("TeamView", IE_Released, this, &ATablet::TryNextPlayerView_Released);
	}

	// Notifications
	if (UReadyOrNotFunctionLibrary::IsInLobby())
	{
		for (const TActorIterator<AMissionPortal> It(GetWorld()); It;)
		{
			It->OnMissionSelected_Delegate.RemoveAll(this);
			It->OnMissionSelected_Delegate.AddDynamic(this, &ATablet::OnMissionSelected);
			break;
		}
	}
}

void ATablet::OnMissionSelected()
{
	if (UReadyOrNotFunctionLibrary::IsInLobby())
	{
		APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter();
		if (OwnerPlayerCharacter && OwnerPlayerCharacter->HumanCharacterWidget_V2)
			OwnerPlayerCharacter->HumanCharacterWidget_V2->OnTabletNotificationEvent();
		
		PlayNotificationEvent();
		PlayVibrationEvent();
	}
}

void ATablet::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ItemMesh->SetOnlyOwnerSee(false);
	
	APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (GetOwner() == LocalPlayer)
	{
		if (IsPlayingDraw())
		{
			if (bStartedPlayingDraw)
			{
				ScaleTime += DeltaSeconds * 4.0f;
				MoveTime += DeltaSeconds * 2.9f;
				
				FVector CameraLocation = GetOwnerPlayerCharacter()->GetFirstPersonCameraComponent()->GetComponentLocation();
				FRotator CameraRotation = GetOwnerPlayerCharacter()->GetFirstPersonCameraComponent()->GetComponentRotation();

				FVector Start = CameraLocation - CameraRotation.Vector() * 50.0f;
				Start.Z -= 20.0f;
				SetActorLocation(FMath::Lerp(Start, GetOwnerPlayerCharacter()->GetMesh1P()->GetSocketLocation(HandsSocket), FMath::Clamp(MoveTime, 0.0f, 1.0f)));
				
				SetActorRotation(GetOwnerPlayerCharacter()->GetMesh1P()->GetSocketRotation(HandsSocket));
				SetActorRelativeScale3D(FMath::Lerp(FVector::ZeroVector, FVector::OneVector, FMath::Clamp(ScaleTime, 0.0f, 1.0f)));
			}
		}
		else if (IsPlayingHolster())
		{
			if (GetWorldTimerManager().GetTimerElapsed(TH_HolsterComplete) > 0.42f)
			{
				if (bStartedPlayingHolster)
				{
					ScaleTime += DeltaSeconds * 4;
					SetActorRelativeScale3D(FMath::Lerp(FVector::OneVector, FVector(0.25f), FMath::Clamp(ScaleTime, 0.0f, 1.0f)));
				}
			}
		}
	}
	
	// Ensure that we release any slate resources if we're not owned by the local player
	if (WidgetComponent)
	{
		if (!GetOwnerPlayerCharacter() || !GetOwnerPlayerCharacter()->IsLocalPlayer())
		{
			// NOTE(killo): can't do this as the widget component will never recreate
			// the widget renderer. easy fix post 1.0
			// WidgetComponent->ReleaseResources();
			
			WidgetComponent->SetVisibility(false);
			WidgetComponent->SetComponentTickEnabled(false);
		}
		else if (IsEquipped())
		{
			WidgetComponent->SetComponentTickEnabled(true);
			WidgetComponent->SetVisibility(true);
			WidgetComponent->InitWidget();

			// Correct potential annoying engine bug where the slate window doesn't have a parent breaking fast path
			// Could also just disable fast path with Slate.EnableFastWidgetPath 0
			if (WidgetComponent->GetSlateWindow().IsValid())
			{
				TSharedPtr<SWindow> Window = WidgetComponent->GetSlateWindow();
				if (!Window->GetParentWidget().IsValid())
				{
					UE_LOG(LogReadyOrNot, Warning, TEXT("Recreating tablet slate resources, slate window parent was invalid"));
					WidgetComponent->ReleaseResources();
					WidgetComponent->UpdateWidget();
				}
			}
		}
	}
	
	if (bTabletButtonHeld)
		TabletFocusTimer += DeltaSeconds;

	if (TabletFocusTimer >= 0.5f)
	{
		bTabletButtonHeld = false;
		TabletFocusTimer = 0.0f;

		// Equip the last equipped item if we're focused and held down the tablet key
		APlayerCharacter* OwningPlayer = GetOwnerPlayerCharacter();
		if (OwningPlayer)
		{
			OwningPlayer->SetTabletFocused(false);
			
			if (OwningPlayer->GetInventoryComponent())
				OwningPlayer->GetInventoryComponent()->EquipLastEquippedItem();
		}
	}
}

bool ATablet::ShouldAttachToOwner() const
{
	LOCAL_PLAYER;
	if (!HasAuthority() && GetOwner() != LocalPlayer)
		return true;
	
	if (bStartedPlayingDraw)
		return false;
		
	return Super::ShouldAttachToOwner();
}

bool ATablet::PlayDraw(bool bDrawFirst)
{
	bStartedPlayingHolster = false;
	bStartedPlayingDraw = true;
	ScaleTime = 0.0f;
	MoveTime = 0.0f;

	SetActorRelativeScale3D(FVector::ZeroVector);
	
	return Super::PlayDraw(bDrawFirst);
}

bool ATablet::PlayHolster()
{
	if (GetOwnerCharacter() && GetOwnerCharacter()->IsLocalPlayer())
	{
		SleepScreen();
		bIsTabletAwake = false;
	}

	bStartedPlayingDraw = false;
	bStartedPlayingHolster = true;
	ScaleTime = 0.0f;
	MoveTime = 0.0f;
	
	return Super::PlayHolster();
}

void ATablet::OnDrawComplete()
{
	Super::OnDrawComplete();

	bStartedPlayingDraw = false;
	ScaleTime = 0.0f;
	MoveTime = 0.0f;

	bTabletDrawn = true;
	
	if (!GetOwnerCharacter() || !GetOwnerCharacter()->IsLocalPlayer())
		return;

	// UInventoryComponent* InventoryComponent = GetOwnerCharacter()->GetInventoryComponent();
	// if (InventoryComponent)
	// {
	// 	InventoryComponent->EquipLastEquippedItem();
	// }
	
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayerController());
	if (PlayerController)
	{
		WakeScreen();
		bIsTabletAwake = true;
	}
}

void ATablet::OnHolsterComplete()
{
	Super::OnHolsterComplete();

	bTabletDrawn = false;
	
	bStartedPlayingHolster = false;
	ScaleTime = 0.0f;
	MoveTime = 0.0f;
}

void ATablet::SetTabletFocusBlend(float Blend)
{
	Blend = FMath::Clamp(Blend, 0.0f, 1.0f);

	DesiredDynamicWeaponFoVBlendEffectAmount = Blend;
	if (bEnabledWeaponFovShader && CurrentDynamicWeaponFoVBlendEffectAmount != DesiredDynamicWeaponFoVBlendEffectAmount)
	{
		CurrentDynamicWeaponFoVBlendEffectAmount = Blend;
		
		for (int32 i = 0; i < DynamicWeaponFovMats.Num(); i++)
		{
			if (UMaterialInstanceDynamic* DynMat = DynamicWeaponFovMats[i])
			{
				DynMat->SetScalarParameterValue("Blend", DesiredDynamicWeaponFoVBlendEffectAmount);
				DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
			}
		}
	}
	
	if (WidgetComponent && WidgetComponent->GetMaterialInstance())
	{
		WidgetComponent->GetMaterialInstance()->SetScalarParameterValue("Blend", Blend);
	}
}

float ATablet::CalculateTabletFov() const
{
	const float MinimumHorizontalFov = FMath::DegreesToRadians(FocusedMinimumHorizontalFov);
	const float TargetVerticalFov = FMath::DegreesToRadians(FocusedTargetVerticalFov);
	
	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	float VerticalFov = 2.0f * FMath::Atan(FMath::Tan(TargetVerticalFov / 2.0f) * (ViewportSize.X / ViewportSize.Y));
	float DesiredFov = FMath::Max(VerticalFov, MinimumHorizontalFov);

	return FMath::RadiansToDegrees(DesiredFov);
}

void ATablet::UpdateLocationAndScale()
{
}

void ATablet::OpenTabletPressed()
{
	bTabletButtonHeld = true;
}

void ATablet::OpenTabletReleased()
{
	APlayerCharacter* OwningPlayer = GetOwnerPlayerCharacter();
	if (bTabletButtonHeld && OwningPlayer)
		OwningPlayer->SetTabletFocused(false);
		
	bTabletButtonHeld = false;
	TabletFocusTimer = 0.0f;
}

void ATablet::TryNextPlayerView_Pressed()
{
	if (APlayerCharacter* OwningPlayer = GetOwnerPlayerCharacter())
		OwningPlayer->TryNextPlayerView_Pressed();
}

void ATablet::TryNextPlayerView_Released()
{
	if (APlayerCharacter* OwningPlayer = GetOwnerPlayerCharacter())
		OwningPlayer->TryNextPlayerView_Released();
}
