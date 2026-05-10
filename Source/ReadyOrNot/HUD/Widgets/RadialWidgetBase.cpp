// Copyright Void Interactive, 2021

#include "RadialWidgetBase.h"

#include "ReadyOrNot.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanelSlot.h"

#include "RadialSectorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"

#include "Data/RadialWidgetThemeData.h"

#include "lib/ReadyOrNotMathLibrary.h"

#include "TimerManager.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#if !UE_BUILD_SHIPPING
#include "Log.h"
#endif

// Shortcut coordinates
FVector2D URadialWidgetBase::Coordinate_TopLeft = {0.0f, 0.0f};
FVector2D URadialWidgetBase::Coordinate_TopRight = {1.0f, 0.0f};
FVector2D URadialWidgetBase::Coordinate_BottomLeft = {0.0f, 1.0f};
FVector2D URadialWidgetBase::Coordinate_BottomRight = {1.0f, 1.0f};
FVector2D URadialWidgetBase::Coordinate_Middle = {0.5f, 0.5f};
FVector2D URadialWidgetBase::Coordinate_TopMiddle = {0.5f, 0.0f};
FVector2D URadialWidgetBase::Coordinate_BottomMiddle = {0.5f, 1.0f};
FVector2D URadialWidgetBase::Coordinate_LeftMiddle = {0.0f, 0.5f};
FVector2D URadialWidgetBase::Coordinate_RightMiddle = {1.0f, 0.5f};

URadialWidgetBase::URadialWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IFMODStudioModule::Get();

	if (RadialSectorWidgetClass == nullptr) 
	{
		ConstructorHelpers::FClassFinder<URadialSectorWidget> W_RadialSectorBaseClass(TEXT("WidgetBlueprint'/Game/Blueprints/Widgets/Radial/Base/W_RadialSectorBase.W_RadialSectorBase_C'"));
		RadialSectorWidgetClass = W_RadialSectorBaseClass.Class;
	}

	if (SelectionSound == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> RM_Select(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Select.RM_Select'"));
		SelectionSound = RM_Select.Object;
	}
	if (MenuOpenSound == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> RM_Open(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Open.RM_Open'"));
		MenuOpenSound = RM_Open.Object;
	}
	if (MenuCloseSound == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> RM_Close(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Close.RM_Close'"));
		MenuCloseSound = RM_Close.Object;
	}
	if (MenuCloseSound_NoSelection == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> RM_CloseNoSelection(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Collapse.RM_Collapse'"));
		MenuCloseSound_NoSelection = RM_CloseNoSelection.Object;
	}

#if !UE_SERVER
	static ConstructorHelpers::FObjectFinder<UFont> LouisGeorgeBold(TEXT("Font'/Game/ReadyOrNot/Assets/Font/Louis_George_Cafe_Bold_Font.Louis_George_Cafe_Bold_Font'"));
	Font = LouisGeorgeBold.Object;
#endif

	bHideRadialWheelCursorOnMenuOpened = true;
	bFirstTickRun = true;
}

void URadialWidgetBase::Setup(const FRadialWidgetSpawnProperties& RadialWidgetSpawnProperties)
{
	StartingSectorIndex = RadialWidgetSpawnProperties.StartingSectorIndex;
	StartingSectorAngle = RadialWidgetSpawnProperties.StartingSectorAngle;

	IconSize = RadialWidgetSpawnProperties.IconSize;
	IconPadding = RadialWidgetSpawnProperties.IconPadding;
	
	SectorInnerRadius = RadialWidgetSpawnProperties.SectorInnerRadius;
	SectorOuterRadius = RadialWidgetSpawnProperties.SectorOuterRadius;

	GapSize = RadialWidgetSpawnProperties.GapSize;
	WheelSize = RadialWidgetSpawnProperties.WheelSize;

	WheelCursorDistanceFromCenterWheel = RadialWidgetSpawnProperties.WheelCursorDistanceFromCenterWheel;

	bHideRadialWheelCursorOnMenuOpened = RadialWidgetSpawnProperties.bHideRadialWheelCursorOnMenuOpened;

	SelectedColor = RadialWidgetSpawnProperties.SelectedColor;
	UnselectedColor = RadialWidgetSpawnProperties.UnselectedColor;
	UnselectableColor = RadialWidgetSpawnProperties.UnselectableColor;

	if (RadialWidgetSpawnProperties.Font)
		Font = RadialWidgetSpawnProperties.Font;
}

void URadialWidgetBase::Setup(URadialWidgetThemeData* InThemeData)
{
	if (!InThemeData)
		return;
	
	StartingSectorIndex = InThemeData->StartingSectorIndex;
	StartingSectorAngle = InThemeData->StartingSectorAngle;
	
	IconSize = InThemeData->IconSize;
	IconPadding = InThemeData->IconPadding;

	SectorInnerRadius = InThemeData->SectorInnerRadius;
	SectorOuterRadius = InThemeData->SectorOuterRadius;

	GapSize = InThemeData->GapSize;
	WheelSize = InThemeData->WheelSize;

	WheelCursorDistanceFromCenterWheel = InThemeData->WheelCursorDistanceFromCenterWheel;

	bHideRadialWheelCursorOnMenuOpened = InThemeData->bHideRadialWheelCursorOnMenuOpened;

	SelectedColor = InThemeData->SelectedColor;
	UnselectedColor = InThemeData->UnselectedColor;
	UnselectableColor = InThemeData->UnselectableColor;
	
	Font = InThemeData->Font;

	MenuOpenSound = InThemeData->MenuOpenSound;
	MenuCloseSound = InThemeData->MenuCloseSound;
	MenuCloseSound_NoSelection = InThemeData->MenuCloseSound_NoSelection;
}

void URadialWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	OwningPlayer = GetOwningPlayer();
	OwningPawn = GetOwningPlayerPawn();
	OwningPlayerCharacter = Cast<APlayerCharacter>(OwningPawn);
}

void URadialWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	OwningPlayer = GetOwningPlayer();
	OwningPawn = GetOwningPlayerPawn();
	OwningPlayerCharacter = Cast<APlayerCharacter>(OwningPawn);

	Setup(RadialWidgetTheme);

	InitializeMenuProperties();

	if (bShowMouseCursor)
		ShowMouseCursor();
	else
		HideMouseCursor();
}

void URadialWidgetBase::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsWheelCreated)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Warning(CUR_CLASS_FUNC + " | " + GetName() + ": Radial wheel has not been created. Aborting...");
		#endif

		CloseWheel(false, true, true, ERadialMenuCloseReason::MCR_ForceClosed);
		
		return;
	}

	GlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);

	MouseAxisDelta = GetMouseDelta();
	MouseAngle = GetDirectionToMouse(GetCenterScreenPosition());
	GamepadAngle = GetDirectionToGamepadAxis();
	RadialCursorPosition = UReadyOrNotMathLibrary::CalculatePositionOnCircle(FVector2D(0.0f), WheelCursorDistanceFromCenterWheel, GetCorrectAngle());
	
	SaveMousePosition();
	
	DetermineInputDevice();

	if (HasMouseMoved())
	{
		UpdateMouseSelectionLogic(RadialWheelCursor);
	}
	else
	{
		UpdateGamepadSelectionLogic(RadialWheelCursor);
	}
}

bool URadialWidgetBase::InitializeMenuProperties_Implementation()
{
	Angle = (MaxCursorAngle-MinCursorAngle)/NumOfSectors;
	AngleSpread = Angle/2;

	PercentageWithoutGap = 1/360.0f * Angle;
	PercentageWithGap = PercentageWithoutGap - GapSize;

	return true;
}

bool URadialWidgetBase::OpenWheel_Implementation(bool bForceRefresh)
{
	BeginOpenWheel();
	
	if (IsMenuClosing())
	{
		OwningPlayer->GetWorldTimerManager().ClearTimer(TH_CloseDelayExpiry);
	}
	
	if (OwningPlayer && (OpenDelay > 0.0f || IsMenuOpening()))
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback_Args(TH_OpenDelayExpiry, this, "OpenWheel_Internal", OpenDelay /** GlobalTimeDilation*/, false, true, -1.0f, bForceRefresh);
		return false;
	}
	
	OpenWheel_Internal(bForceRefresh);

	return true;
}

bool URadialWidgetBase::CloseWheel_Implementation(const bool bExecuteRadial, const bool bRemoveFromParent, const bool bHideMouseCursor, const ERadialMenuCloseReason CloseReason)
{
	BeginCloseWheel();
	
	#if WITH_EDITOR
	ULog::Info("Close Reason: " + RON_ENUM_TO_STRING(ERadialMenuCloseReason, CloseReason));
	#endif
	
	if (IsMenuOpening())
	{
		OwningPlayer->GetWorldTimerManager().ClearTimer(TH_OpenDelayExpiry);
		return false;
	}

	if (IsWheelClosed())
	{
		LastCloseReason = ERadialMenuCloseReason::MCR_UserClosed;
		return false;
	}

	if (OwningPlayer && (CloseDelay > 0.0f && !IsMenuClosing()))
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback_Args(TH_CloseDelayExpiry, this, "CloseWheel_Internal", CloseDelay /** GlobalTimeDilation*/, false, true, -1.0f, bExecuteRadial, bRemoveFromParent, bHideMouseCursor, CloseReason);
		return false;
	}

	CloseWheel_Internal(bExecuteRadial, bRemoveFromParent, bHideMouseCursor, CloseReason);
		
	return true;
}

void URadialWidgetBase::OpenWheel_Internal(const bool bForceRefresh)
{
	if (!GetParent())
		AddToViewport();
	
	PlaySoundEffect(MenuOpenSound);

	if (bIsWheelCreated)
	{
		SetMousePositionToCenterScreen();
	}
	else
	{
		CreateWheel();
	}

	if (bForceRefresh)
		RefreshWheel(StartingSectorIndex);

	if (OwningPawn)
	{
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OwningPawn);
		if (PlayerCharacter)
		{
			if (bCanAimWhileMenuIsOpened)
				PlayerCharacter->Server_UnlockAim();
			else
				PlayerCharacter->Server_LockAim();
			
			if (bCanPerformActionsWhileMenuIsOpened)
				PlayerCharacter->Server_UnlockAllActions();
			else
				PlayerCharacter->Server_LockAllActions();
				
			if (bCanMoveWhileMenuIsOpened)
				PlayerCharacter->Server_UnlockMovement();
			else
				PlayerCharacter->Server_LockMovement();
		}
	}

	OnRadialMenuOpened();

	OnRadialMenuOpened_Delegate.Broadcast();
}

void URadialWidgetBase::CloseWheel_Internal(const bool bExecuteRadial, const bool bRemoveFromParent, const bool bHideMouseCursor, const ERadialMenuCloseReason CloseReason)
{
	SaveMousePosition();

	if (!IsBlockingAnimationPlaying())
	{
		if (CurrentSelectionIndex > -1)
			PlaySoundEffect(MenuCloseSound);
		else
			PlaySoundEffect(MenuCloseSound_NoSelection);			

		if (bExecuteRadial)
			ExecuteRadial();
	}

	if (bRemoveFromParent)
		RemoveFromParent();

	if (bHideMouseCursor)
	{
		HideMouseCursor();
		
		if (OwningPlayer)
			OwningPlayer->SetInputMode(FInputModeGameOnly());
	}

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OwningPawn);
	if (PlayerCharacter)
	{
		PlayerCharacter->Server_UnlockMovementAndAim();
		PlayerCharacter->Server_UnlockAllActions();
	}

	LastCloseReason = CloseReason;

	OnRadialMenuClosed();

	OnRadialMenuClosed_Delegate.Broadcast();
}

bool URadialWidgetBase::IsMenuOpening()
{
	return OwningPlayer ? OwningPlayer->GetWorldTimerManager().IsTimerActive(TH_OpenDelayExpiry) : false;
}

bool URadialWidgetBase::IsMenuClosing()
{
	return OwningPlayer ? OwningPlayer->GetWorldTimerManager().IsTimerActive(TH_CloseDelayExpiry) : false;
}

bool URadialWidgetBase::ShowWheel_Implementation()
{
	SetVisibility(ESlateVisibility::Visible);

	return true;
}

bool URadialWidgetBase::HideWheel_Implementation()
{
	SetVisibility(ESlateVisibility::Hidden);

	return true;
}

bool URadialWidgetBase::IsWheelOpen_Implementation()
{
	return IsVisible() || IsInViewport();
}

bool URadialWidgetBase::IsWheelClosed_Implementation()
{
	return !IsVisible() || !IsInViewport();
}

void URadialWidgetBase::SetMouseWheelDelta(const float InDelta)
{
	MouseWheelDelta = InDelta;
}

void URadialWidgetBase::SetGamepadXYAxis(const float InGamepadXAxis, const float InGamepadYAxis)
{
	GamepadXAxis = InGamepadXAxis;
	GamepadYAxis = InGamepadYAxis;
}

void URadialWidgetBase::SetGamepadXAxis(const float InGamepadXAxis)
{
	GamepadXAxis = InGamepadXAxis;
}

void URadialWidgetBase::SetGamepadYAxis(const float InGamepadYAxis)
{
	GamepadYAxis = InGamepadYAxis;
}

void URadialWidgetBase::SetCloseDelay(const float NewDelay)
{
	CloseDelay = NewDelay;
}

bool URadialWidgetBase::CreateWheel_Implementation()
{
	if (bIsWheelCreated)
		return true;

	float CurrentAngle = (MaxCursorAngle-MinCursorAngle) - StartingSectorAngle - AngleSpread;
	float RadialSectorAngle = StartingSectorAngle;

	UCanvasPanelSlot* RadialWheelPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(RadialWheel);

	if (RadialWheelPanelSlot)
		RadialWheelPanelSlot->SetSize(FVector2D(WheelSize));

	Sectors.Empty();
	Angles.Empty();

	Angles.Reserve(NumOfSectors);
	Sectors.Reserve(NumOfSectors);
	for (int32 i = 0; i < NumOfSectors; i++)
	{
		Angles.Add(RadialSectorAngle + GapSize - StartingSectorAngle);
		
		CreateWheelSector(RadialWheel, RadialSectorAngle + GapSize, SectorInnerRadius, SectorOuterRadius);

		OnRadialSectorCreated(i, CurrentAngle);

		CurrentAngle -= Angle;
		RadialSectorAngle += Angle;
	}

	InitializeMenu(StartingSectorIndex);

	OnRadialMenuInitialized();

	bIsWheelCreated = true;
	bIsWheelRefreshed = true;

	OnRadialMenuCreated();

	return true;
}

void URadialWidgetBase::BeginOpenWheel_Implementation()
{
}

void URadialWidgetBase::BeginCloseWheel_Implementation()
{
}

bool URadialWidgetBase::RefreshWheel_Implementation(const int32 InStartingSectorIndex)
{
	bIsWheelRefreshed = false;

	InitializeMenuProperties();

	float CurrentAngle = (MaxCursorAngle-MinCursorAngle) - StartingSectorAngle - AngleSpread;
	float RadialSectorAngle = StartingSectorAngle;

	UCanvasPanelSlot* RadialWheelPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(RadialWheel);

	if (RadialWheelPanelSlot)
		RadialWheelPanelSlot->SetSize(FVector2D(WheelSize));

	for (auto Sector : Sectors)
	{
		Sector->RemoveFromParent();
	}

	Sectors.Empty();
	Angles.Empty();

	Angles.Reserve(NumOfSectors);
	Sectors.Reserve(NumOfSectors);
	for (int32 i = 0; i < NumOfSectors; i++)
	{
		Angles.Add(RadialSectorAngle + GapSize - StartingSectorAngle);
		
		CreateWheelSector(RadialWheel, RadialSectorAngle + GapSize, SectorInnerRadius, SectorOuterRadius);

		OnRadialSectorCreated(i, CurrentAngle);

		CurrentAngle -= Angle;
		RadialSectorAngle += Angle;
	}

	InitializeMenu(InStartingSectorIndex);

	OnRadialMenuInitialized();

	bIsWheelRefreshed = true;

	return true;
}

bool URadialWidgetBase::CreateWheelSector_Implementation(UPanelWidget* PanelWidget, const float InAngle, const float InSectorInnerRadius, const float InSectorOuterRadius, UMaterialInterface* InSectorMaterial, const bool bCreateGap)
{
	URadialSectorWidget* SectorImageWidget = CreateWidget<URadialSectorWidget>(GetOwningPlayer(), RadialSectorWidgetClass);
	
	if (!SectorImageWidget)
	{
#if !UE_BUILD_SHIPPING
		ULog::Error(CUR_CLASS_FUNC_WITH_LINE + " | SectorImageWidget is null. Aborting...");
#endif

		return false;
	}

	PanelWidget->AddChild(SectorImageWidget);

	SectorImageWidget->InitializeSectorWidget(InAngle, bCreateGap ? PercentageWithGap : PercentageWithoutGap, InSectorInnerRadius, InSectorOuterRadius, InSectorMaterial, UnselectableColor);

	Sectors.Add(SectorImageWidget);

	return true;
}

bool URadialWidgetBase::ExecuteRadial_Implementation()
{
	return true;
}

bool URadialWidgetBase::DetermineSelectedSector_Implementation(const float InAngle)
{
	uint16 LocalIndex = 0;
	const uint16 NumOfAngles = Angles.Num();

	for (float LocalAngle : Angles)
	{
		const bool bIsAtLastIndex = NumOfAngles - 1 == LocalIndex;
		const float AngleToCompare = bIsAtLastIndex ? (MaxCursorAngle-MinCursorAngle) : Angles[LocalIndex+1];

		if (InAngle >= UReadyOrNotMathLibrary::KeepAngleBelow360(LocalAngle) && InAngle <= AngleToCompare)
		{
			Select(LocalIndex);

			break;
		}

		LocalIndex += 1;
	}

	return true;
}

bool URadialWidgetBase::InitializeMenu_Implementation(const int32 Index)
{
	DeselectAll();

	if (Index < 0)
		return false;

	for (int32 i = 0; i < Sectors.Num(); i++)
	{
		if (i == Index)
		{
			CurrentSelectionIndex = i;
			UpdateSectorColor(i, GetCorrectSelectionColor());
			OnSectorSelected(i);
			break;
		}
	}

	return true;
}

void URadialWidgetBase::OnRadialMenuOpened_Implementation()
{
	SetMousePositionToCenterScreen();

	if (bHideRadialWheelCursorOnMenuOpened)
		RadialWheelCursor->SetVisibility(ESlateVisibility::Hidden);
}

void URadialWidgetBase::OnRadialMenuClosed_Implementation()
{
}

bool URadialWidgetBase::Select_Implementation(const int32 Index)
{
	if (CurrentSelectionIndex != Index)
	{
		Deselect(CurrentSelectionIndex);

		for (int32 i = 0; i < Sectors.Num(); i++)
		{
			if (i == Index)
			{
				CurrentSelectionIndex = i;
				UpdateSectorColor(i, GetCorrectSelectionColor());
				PlaySoundEffect(SelectionSound);
				OnSectorSelected(i);
				break;
			}
		}
	}
	else
	{
		UpdateSectorColor(CurrentSelectionIndex, GetCorrectSelectionColor());
	}

	return true;
}

bool URadialWidgetBase::Deselect_Implementation(const int32 Index)
{
	for (int32 i = 0; i < Sectors.Num(); i++)
	{
		if (i == Index)
		{
			PreviousSelectionIndex = i;
			UpdateSectorColor(i, UnselectedColor);
			OnSectorDeselected(i);
			break;
		}
	}

	return true;
}

bool URadialWidgetBase::DeselectAll_Implementation()
{
	CurrentSelectionIndex = -1;

	for (int32 i = 0; i < Sectors.Num(); i++)
	{
		Deselect(i);
	}

	return true;
}

bool URadialWidgetBase::Next_Implementation()
{
	return true;
}

bool URadialWidgetBase::Previous_Implementation()
{
	return true;
}

bool URadialWidgetBase::IsWheelCursorVisible_Implementation()
{
	return true;
}

void URadialWidgetBase::OnSectorSelected_Implementation(int32 SelectedIndex)
{
}

void URadialWidgetBase::OnSectorDeselected_Implementation(int32 DeselectedIndex)
{
}

void URadialWidgetBase::OnRadialMenuCreated_Implementation()
{
	if (CurrentSelectionIndex > 0)
	{
		GamepadAngle = (MaxCursorAngle-MinCursorAngle) - Angles[CurrentSelectionIndex] - AngleSpread;
		RadialCursorPosition = UReadyOrNotMathLibrary::CalculatePositionOnCircle(FVector2D(0.0f), WheelCursorDistanceFromCenterWheel, GamepadAngle);
	}

	//FVector2D NewPosition = GetCenterScreenPosition();
	//
	//NewPosition.X -= WheelCursorDistanceFromCenterWheel;
	//
	//SetMousePosition(FVector2D(NewPosition.X - WheelCursorDistanceFromCenterWheel, NewPosition.Y));
	//
	//MousePosition = NewPosition;
}

bool URadialWidgetBase::OnRadialMenuInitialized_Implementation()
{
	return true;
}

bool URadialWidgetBase::OnRadialSectorCreated_Implementation(int32 Index, float InAngle)
{
	return true;
}

float URadialWidgetBase::GetDirectionToMouse_Implementation(const FVector2D MidWidgetCoordinates)
{
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(FVector(MidWidgetCoordinates.X, MidWidgetCoordinates.Y, 0.0f), 
																		  FVector(MousePosition.X, MousePosition.Y, 0.0f));

	const float DirectionInDegrees = LookAtRotation.Yaw;

	if (DirectionInDegrees < 0.0f)
	{
		switch (RadialCursorBehaviour)
		{
			case ERadialCursorBehaviour::RCB_Clamped:
			if (360.0f + DirectionInDegrees <= MinCursorAngle + 10)
				return MinCursorAngle + 10;
			
			return FMath::Clamp(360.0f + DirectionInDegrees, MinCursorAngle + 10, MaxCursorAngle - 10);

			default:
			return 360.0f + DirectionInDegrees;
		}
	}

	switch (RadialCursorBehaviour)
	{
		case ERadialCursorBehaviour::RCB_Clamped:
		if (DirectionInDegrees <= MinCursorAngle + 10 && DirectionInDegrees >= MinCursorAngle/2)
			return MinCursorAngle + 10;
		if (DirectionInDegrees <= MinCursorAngle + 10 && DirectionInDegrees >= 0.0f)
			return MaxCursorAngle - 10;

		return FMath::Clamp(DirectionInDegrees, MinCursorAngle + 10, MaxCursorAngle - 10);

		default:
		return DirectionInDegrees;
	}
}

float URadialWidgetBase::GetDirectionToGamepadAxis_Implementation()
{
	const float ModifiedGamepadYAxis = GamepadYAxis + (GamepadXAxis != 0.0f ? 0.0001 : 0.0f);
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(FVector(0.0f), FVector(GamepadXAxis, ModifiedGamepadYAxis, 0.0f)); 

	const float DirectionInDegrees = LookAtRotation.Yaw;

	if (DirectionInDegrees < 0.0f)
		return 360.0f + DirectionInDegrees;

	return DirectionInDegrees;	
}

void URadialWidgetBase::UpdateSectorColor(const int32 SectorIndex, const FLinearColor SectorColor)
{
	const int32 Index = FMath::Clamp(SectorIndex, 0, NumOfSectors-1);

	if (Sectors.IsValidIndex(Index))
	{
		Sectors[Index]->SetSectorColor(SectorColor);
	}
	else
	{
		#if !UE_BUILD_SHIPPING
		if (bShowDebugMessages)
			ULog::Error(CUR_CLASS_FUNC_WITH_LINE + " | Sector index " + FString::FromInt(Index) + " does not exist!", LO_Both, true);
		#endif
	}
}

void URadialWidgetBase::UpdateMouseSelectionLogic(UWidget* RadialCursorWidget)
{
	if (!RadialCursorWidget)
	{
		#if !UE_BUILD_SHIPPING
		if (bShowDebugMessages)
			ULog::Warning(CUR_CLASS_FUNC_WITH_LINE + " | RadialCursorWidget is null!", LO_Console, true);
		#endif

		return;
	}

	if (!bNavigatingWithGamepad && !RadialCursorWidget->IsVisible() && IsMouseAxisBeyondThreshold(MouseAxisDelta))
	{
		DetermineSelectedSector_Internal(RadialCursorWidget);
	}
	else
	{
		if (RadialCursorWidget->IsVisible())
		{
			DetermineSelectedSector_Internal(RadialCursorWidget);
		}
	}

	if (bAlwaysHideRadialWheelCursor)
		RadialCursorWidget->SetVisibility(ESlateVisibility::Hidden);
}

void URadialWidgetBase::UpdateGamepadSelectionLogic(UWidget* RadialCursorWidget)
{
	if (!RadialCursorWidget)
	{
		#if !UE_BUILD_SHIPPING
		if (bShowDebugMessages)
			ULog::Warning(CUR_CLASS_FUNC_WITH_LINE + " | RadialCursorWidget is null!", LO_Console, true);
		#endif

		return;
	}

	if (bNavigatingWithGamepad && IsGamepadAxisBeyondThreshold({GamepadXAxis, GamepadYAxis}))
	{
		DetermineSelectedSector_Internal(RadialCursorWidget, ESlateVisibility::Hidden);
	}
}

FLinearColor URadialWidgetBase::GetCorrectSelectionColor()
{
	return IsBlockingAnimationPlaying() ? UnselectableColor : SelectedColor;
}

FVector2D URadialWidgetBase::CalculatePositionOnCircleFromWidget(UPanelWidget* PanelWidget, const FVector2D Origin, const FVector2D InPadding, const float InAngle)
{
	UCanvasPanelSlot* PanelWidgetSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(PanelWidget);
	FVector2D WidgetSize(WheelSize);
	
	if (PanelWidgetSlot)
		WidgetSize = PanelWidgetSlot->GetSize() / 2;

	return FVector2D(Origin.X + (WidgetSize.X - InPadding.X) * FMath::Cos(FMath::DegreesToRadians(UReadyOrNotMathLibrary::KeepAngleBelow360(InAngle))), Origin.Y + (WidgetSize.Y - InPadding.Y) * FMath::Sin(FMath::DegreesToRadians(UReadyOrNotMathLibrary::KeepAngleBelow360(InAngle))));
}

FVector2D URadialWidgetBase::GetViewportPositionOfWidget(UWidget* InWidget, const FVector2D InCoordinates)
{
	if (InWidget)
	{
		FVector2D ViewportPosition;
		FVector2D PixelPosition;
		USlateBlueprintLibrary::LocalToViewport(this, GetTickSpaceGeometry(), InWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(InCoordinates), PixelPosition, ViewportPosition);
		
		return ViewportPosition;
	}

	return FVector2D::ZeroVector;
}

FVector2D URadialWidgetBase::GetPixelPositionOfWidget(UWidget* InWidget, const FVector2D InCoordinates)
{
	if (InWidget)
	{
		FVector2D ViewportPosition;
		FVector2D PixelPosition;
		USlateBlueprintLibrary::LocalToViewport(this, GetTickSpaceGeometry(), InWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(InCoordinates), PixelPosition, ViewportPosition);
		
		return PixelPosition;
	}

	return FVector2D::ZeroVector;
}

FVector2D URadialWidgetBase::GetViewportPositionOfWidgetCenter(UWidget* InWidget)
{
	return GetViewportPositionOfWidget(InWidget, Coordinate_Middle);
}

FVector2D URadialWidgetBase::GetPixelPositionOfWidgetCenter(UWidget* InWidget)
{
	return GetPixelPositionOfWidget(InWidget, Coordinate_Middle);
}

float URadialWidgetBase::GetCorrectAngle()
{
	return bNavigatingWithGamepad ? GamepadAngle : MouseAngle;
}

void URadialWidgetBase::ShowMouseCursor()
{
	if (OwningPlayer)
		OwningPlayer->bShowMouseCursor = true;
}

void URadialWidgetBase::HideMouseCursor()
{
	if (OwningPlayer)
		OwningPlayer->bShowMouseCursor = false;
}

void URadialWidgetBase::SetMousePositionToCenterScreen()
{
	SetMousePosition(GetCenterScreenPosition());
}

void URadialWidgetBase::SetMousePosition(const FVector2D& NewMousePosition)
{
	if (OwningPlayer)
		OwningPlayer->SetMouseLocation(NewMousePosition.X, NewMousePosition.Y);
}

void URadialWidgetBase::SaveMousePosition()
{
	MousePosition = GetMousePosition();
}

void URadialWidgetBase::RestoreMousePosition()
{
	SetMousePosition(MousePosition);
}

void URadialWidgetBase::UseMouseControl()
{
	bNavigatingWithGamepad = false;
}

void URadialWidgetBase::UseGamepadControl()
{
	bNavigatingWithGamepad = true;

	SetMousePosition(GetCenterScreenPosition() - -RadialCursorPosition);
}

void URadialWidgetBase::DetermineInputDevice()
{
	if (GamepadXAxis != 0.0f || GamepadYAxis != 0.0f)
		UseGamepadControl();
	else
		UseMouseControl();
}

void URadialWidgetBase::DetermineSelectedSector_Internal(UWidget* RadialCursorWidget, const ESlateVisibility& RadialCursorVisibility)
{
	RadialCursorWidget->SetRenderTranslation(RadialCursorPosition);
	RadialCursorWidget->SetVisibility(RadialCursorVisibility);
	
	DetermineSelectedSector(UReadyOrNotMathLibrary::KeepAngleBelow360(360.0f - GetCorrectAngle() + (360.0f - StartingSectorAngle)));
}
