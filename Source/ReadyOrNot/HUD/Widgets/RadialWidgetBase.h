// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "BaseWidget.h"
#include "RadialWidgetBase.generated.h"

class UFMODEvent;

UENUM(BlueprintType)
enum class ERadialCursorBehaviour : uint8
{
	// When clamped, the radial wheel cursor can only move between MinCursorAngle and MaxCursorAngle (Limited freedom)
	RCB_Clamped		UMETA(DisplayName="Clamped"),

	// When Continuous, the radial wheel cursor can move freely (360 freedom), even if MinCursorAngle and MaxCursorAngle are set to non-default values
	RCB_Continuous	UMETA(DisplayName="Continuous")
};

UENUM(BlueprintType)
enum class ERadialMenuCloseReason : uint8
{
	// The user has closed the menu
	MCR_UserClosed		UMETA(DisplayName="User Closed"),
	
	// The menu has force closed itself
	MCR_ForceClosed		UMETA(DisplayName="Closed Closed"),
};

USTRUCT(BlueprintType)
struct FRadialWidgetSpawnProperties
{
	GENERATED_BODY()

	FRadialWidgetSpawnProperties()
	{
		bHideRadialWheelCursorOnMenuOpened = true;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 StartingSectorIndex = -1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float IconSize = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float IconPadding = 70.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StartingSectorAngle = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float SectorInnerRadius = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float SectorOuterRadius = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float GapSize = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WheelSize = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WheelCursorDistanceFromCenterWheel = 160.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bHideRadialWheelCursorOnMenuOpened : 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor SelectedColor = FLinearColor(0.765625f, 0.0f, 0.0f, 0.7f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor UnselectedColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor UnselectableColor = FLinearColor(0.385417f, 0.0f, 0.0f, 0.611f);
	
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    UFont* Font = nullptr;
};

/**
 * Base class for radial-style menus
 * You must derive from this class when instantiating/creating this widget.
 * @author Ali
 */
UCLASS(Abstract, BlueprintType)
class READYORNOT_API URadialWidgetBase : public UBaseWidget
{
	GENERATED_BODY()

public:
	URadialWidgetBase(const FObjectInitializer& ObjectInitializer);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRadialMenuOpened);
	UPROPERTY(BlueprintAssignable, Category = "Radial Menu")
	FOnRadialMenuOpened OnRadialMenuOpened_Delegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRadialMenuClosed);
	UPROPERTY(BlueprintAssignable, Category = "Radial Menu")
    FOnRadialMenuClosed OnRadialMenuClosed_Delegate;
	
	// Setup the properties of the radial widget using FRadialWidgetSpawnProperties
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void Setup(const FRadialWidgetSpawnProperties& RadialWidgetSpawnProperties);
	
	// Setup the properties of the radial widget using URadialWidgetThemeData
	void Setup(class URadialWidgetThemeData* InThemeData);

	// Create and open the wheel, if it has not already been created. If already created, just open it
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool OpenWheel(bool bForceRefresh = false);

	// Close the wheel and remove it from the viewport, you need to call OpenWheel to bring up the menu again
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool CloseWheel(bool bExecuteRadial = true, bool bRemoveFromParent = true, bool bHideMouseCursor = true, ERadialMenuCloseReason CloseReason = ERadialMenuCloseReason::MCR_UserClosed);

	// Make the wheel visible on screen
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool ShowWheel();

	// Make the wheel invisible from the screen
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool HideWheel();

	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	ERadialMenuCloseReason GetLastClosedReason() const { return LastCloseReason; }

	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	bool WasForceClosed() const { return LastCloseReason == ERadialMenuCloseReason::MCR_ForceClosed; }

	// Returns true if the OpenDelay timer is active
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	bool IsMenuOpening();

	// Returns true if the CloseDelay timer is active
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
    bool IsMenuClosing();
	
	// Is the wheel open? (i.e is it visible on screen?)
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Radial Menu")
	bool IsWheelOpen();
	
	// Is the wheel closed? (i.e is it not visible on screen?)
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Radial Menu")
	bool IsWheelClosed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool Next();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool Previous();

	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetMouseWheelDelta(float InDelta);

	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetGamepadXYAxis(float InGamepadXAxis, float InGamepadYAxis);

	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetGamepadXAxis(float InGamepadXAxis);

	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetGamepadYAxis(float InGamepadYAxis);
	
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
    void SetCloseDelay(float NewDelay);
	
protected:
	void NativeOnInitialized() override;
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	virtual bool OpenWheel_Implementation(bool bForceRefresh = false);

	virtual bool CloseWheel_Implementation(bool bExecuteRadial = true, bool bRemoveFromParent = true, bool bHideMouseCursor = true, ERadialMenuCloseReason CloseReason = ERadialMenuCloseReason::MCR_UserClosed);

	virtual bool ShowWheel_Implementation();

	virtual bool HideWheel_Implementation();

	virtual bool IsWheelOpen_Implementation();

	virtual bool IsWheelClosed_Implementation();

	virtual bool Next_Implementation();

	virtual bool Previous_Implementation();

	// Initializes the radial menu's propeties
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool InitializeMenuProperties();
	virtual bool InitializeMenuProperties_Implementation();

	// Creates the radial menu and populates it with data
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool CreateWheel();
	virtual bool CreateWheel_Implementation();

	// Called immediately when the user requests to open this radial menu
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
    void BeginOpenWheel();
    virtual void BeginOpenWheel_Implementation();

	// Called immediately when the user requests to close this radial menu
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
    void BeginCloseWheel();
	virtual void BeginCloseWheel_Implementation();

	// Refreshes the radial menu and populates it with data
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool RefreshWheel(int32 InStartingSectorIndex = 0);
	virtual bool RefreshWheel_Implementation(int32 InStartingSectorIndex = 0);

	// Creates a radial sector widget to be used for this menu
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool CreateWheelSector(class UPanelWidget* PanelWidget, float InAngle, float InSectorInnerRadius, float InSectorOuterRadius, class UMaterialInterface* InSectorMaterial = nullptr, bool bCreateGap = true);
	virtual bool CreateWheelSector_Implementation(class UPanelWidget* PanelWidget, float InAngle, float InSectorInnerRadius, float InSectorOuterRadius, class UMaterialInterface* InSectorMaterial = nullptr, bool bCreateGap = true);

	// Executes code based on current selection
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool ExecuteRadial();
	virtual bool ExecuteRadial_Implementation();

	// Determines the selected sector based on the given angle value (i.e. Maps the given angle to a sector)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool DetermineSelectedSector(float InAngle);
	virtual bool DetermineSelectedSector_Implementation(float InAngle);

	// Intializes the radial menu before it's ready for use
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool InitializeMenu(int32 Index);
	virtual bool InitializeMenu_Implementation(int32 Index);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	void OnRadialMenuOpened();
	virtual void OnRadialMenuOpened_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	void OnRadialMenuClosed();
	virtual void OnRadialMenuClosed_Implementation();
	
	// Select a category, given an index.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool Select(int32 Index);
	virtual bool Select_Implementation(int32 Index);

	// Deselect a category, given an index.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool Deselect(int32 Index);
	virtual bool Deselect_Implementation(int32 Index);

	// Deselect all categories on the wheel.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool DeselectAll();
	virtual bool DeselectAll_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool IsWheelCursorVisible();
	virtual bool IsWheelCursorVisible_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	void OnSectorSelected(int32 SelectedIndex);
	virtual void OnSectorSelected_Implementation(int32 SelectedIndex);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	void OnSectorDeselected(int32 DeselectedIndex);
	virtual void OnSectorDeselected_Implementation(int32 DeselectedIndex);

	// Called after the radial menu has been created
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	void OnRadialMenuCreated();
	virtual void OnRadialMenuCreated_Implementation();

	// Called after the radial menu has been initialized
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool OnRadialMenuInitialized();
	virtual bool OnRadialMenuInitialized_Implementation();

	// Called after creating a radial sector image
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu")
	bool OnRadialSectorCreated(int32 Index, float InAngle);
	virtual bool OnRadialSectorCreated_Implementation(int32 Index, float InAngle);

	// Retrieves the angle of the mouse from the center radial wheel
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Radial Menu")
	float GetDirectionToMouse(FVector2D MidWidgetCoordinates);
	virtual float GetDirectionToMouse_Implementation(FVector2D MidWidgetCoordinates);
	
	// Retrieves the angle of the gamepad axis from the center radial wheel
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Radial Menu")
	float GetDirectionToGamepadAxis();
	virtual float GetDirectionToGamepadAxis_Implementation();

	// Updates a specific sector color, given the sector index and color value
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void UpdateSectorColor(int32 SectorIndex, FLinearColor SectorColor);

	// Determines which sector in the radial wheel to select, based on mouse inputs (RadialCursorWidget is optional and can be left null)
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void UpdateMouseSelectionLogic(UWidget* RadialCursorWidget = nullptr);

	// Determines which sector in the radial wheel to select, based on gamepad input (RadialCursorWidget is optional and can be left null)
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void UpdateGamepadSelectionLogic(UWidget* RadialCursorWidget = nullptr);

	// Retrieves the correct radial selection color based on whether any blocking animation is playing or not
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	FLinearColor GetCorrectSelectionColor();

	// Standard parametric equation of a circle but with a PanelWidget used as the radius, to make things easier
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	FVector2D CalculatePositionOnCircleFromWidget(class UPanelWidget* PanelWidget, FVector2D Origin, FVector2D InPadding, float InAngle);

	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	FVector2D GetViewportPositionOfWidget(UWidget* InWidget, FVector2D InCoordinates = FVector2D(0.0f, 0.0f));
	
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	FVector2D GetPixelPositionOfWidget(UWidget* InWidget, FVector2D InCoordinates = FVector2D(0.0f, 0.0f));
	
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	FVector2D GetViewportPositionOfWidgetCenter(UWidget* InWidget);
	
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
    FVector2D GetPixelPositionOfWidgetCenter(UWidget* InWidget);

	// Retrieves the correct angle based on current device input. Used interally for determining which sector of the radial wheel to select.
	UFUNCTION(BlueprintPure, Category = "Radial Menu")
	float GetCorrectAngle();

	// Sets the actual mouse cursor to be visible on screen
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void ShowMouseCursor();

	// Sets the actual mouse cursor to be hidden from the screen
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void HideMouseCursor();

	// Sets the actual mouse position to the center of the screen based on the current viewport size
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetMousePositionToCenterScreen();

	// Sets the actual mouse position to the given new mouse position
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SetMousePosition(const FVector2D& NewMousePosition);

	// Captures the current mouse's position and saves it for later use
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void SaveMousePosition();

	// Sets the actual mouse postion to the previously saved mouse position
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void RestoreMousePosition();

	// Recognize only mouse inputs
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void UseMouseControl();

	// Recognize only gamepad inputs
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void UseGamepadControl();

	// Determines whether we should use mouse or gamepad as input
	UFUNCTION(BlueprintCallable, Category = "Radial Menu")
	void DetermineInputDevice();
	
	// Currently selected category/sector index
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	int32 CurrentSelectionIndex = 0;

	// Previously selected category/sector index
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	int32 PreviousSelectionIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	float Angle = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	float AngleSpread = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data")
	float PercentageWithoutGap = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data")
	float PercentageWithGap = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	uint8 bIsWheelCreated : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	uint8 bIsWheelRefreshed : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	uint8 bNavigatingWithGamepad : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	FVector2D RadialCursorPosition = FVector2D(0.0f);

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	TArray<float> Angles;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Data")
	TArray<class URadialSectorWidget*> Sectors;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data", meta = (BindWidget))
	class UPanelWidget* RadialWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data", meta = (BindWidgetOptional))
	class UImage* RadialWheelCursor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data")
    APlayerController* OwningPlayer;
	
	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data")
    APawn* OwningPawn;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Data")
    class APlayerCharacter* OwningPlayerCharacter;
	
	// The number of slices we should create for this radial menu
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 2, UIMin = 2))
	int32 NumOfSectors = 4;
	
	// The amount of time (in seconds) to wait before opening this menu. If OpenDelay is <= 0, then open instantly. Otherwise, wait the specified time before opening the menu.
	// Can be canceled by calling CloseWheel
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 10.0f, UIMax = 10.0f))
    float OpenDelay = 0.0f;

	// The amount of time (in seconds) to wait before closing this menu. If CloseDelay is <= 0, then close instantly. Otherwise, wait the specified time before closing the menu.
	// Can be canceled by calling OpenWheel
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 10.0f, UIMax = 10.0f))
    float CloseDelay = 0.0f;

	// The absolute minimum size that this radial menu can scale down to, without severly impacting user experience
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f))
	float MinWheelSize = 400.0f;

	// The absolute maximum size that this radial menu can scale up to, without severly impacting user experience
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f))
	float MaxWheelSize = 800.0f;
	
	// The minimum angle the radial wheel cursor can move to. Also it is the minimum angle of the wheel itself
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = "-360.0", UIMin = "-360.0", ClampMax = "360.0", UIMax = "360.0"))
	float MinCursorAngle = 0.0f;

	// The maximum angle the radial wheel cursor can move to. Also it is the maximum angle of the wheel itself
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 360.0f, UIMax = 360.0f))
	float MaxCursorAngle = 360.0f;

	// The preferred behaviour of the radial wheel cursor. Hover over the dropdown for more info.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Radial Menu | Settings")
	ERadialCursorBehaviour RadialCursorBehaviour = ERadialCursorBehaviour::RCB_Continuous;

	// The sector widget to use when creating sectors for this radial menu. W_RadialSectorBaseClass is the default setting.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Settings")
	TSubclassOf<class URadialSectorWidget> RadialSectorWidgetClass;

	// Should we always hide the radial wheel cursor whenever we are navigating the radial wheel?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Settings")
	uint8 bAlwaysHideRadialWheelCursor : 1;

	// Can the player controller's owned pawn still move while this radial menu is opened? False = Lock movement when this menu is opened
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Settings")
    uint8 bCanMoveWhileMenuIsOpened : 1;
    
	// Can the player controller's owned pawn still perform its gameplay actions while this radial menu is opened? False = Lock player actions when this menu is opened
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Settings")
    uint8 bCanPerformActionsWhileMenuIsOpened : 1;

	// Can the player controller's owned pawn still aim the camera while this radial menu is opened? False = Lock camera aim when this menu is opened
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Settings")
    uint8 bCanAimWhileMenuIsOpened : 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Debug")
	uint8 bShowDebugMessages : 1;

	// Should we show the actual mouse cursor on screen whenever we open this radial menu?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu | Debug")
	uint8 bShowMouseCursor : 1;
	
	// The sector index of the radial wheel we should automatically select, when we first create this widget. -1 = select nothing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = -1, UIMin = -1, EditCondition = "RadialWidgetTheme == nullptr"))
	int32 StartingSectorIndex = -1;

	// The angle to start at when creating sectors on the radial wheel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 360.0f, UIMax = 360.0f, EditCondition = "RadialWidgetTheme == nullptr"))
    float StartingSectorAngle = 0.0f;

	// The size of each image widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, EditCondition = "RadialWidgetTheme == nullptr"))
    float IconSize = 60.0f;

	// The offset from the circumference of the radial wheel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, EditCondition = "RadialWidgetTheme == nullptr"))
	float IconPadding = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 0.5f, UIMax = 0.5f, EditCondition = "RadialWidgetTheme == nullptr"))
	float SectorInnerRadius = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 0.5f, UIMax = 0.5f, EditCondition = "RadialWidgetTheme == nullptr"))
	float SectorOuterRadius = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, EditCondition = "RadialWidgetTheme == nullptr"))
	float GapSize = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f, EditCondition = "RadialWidgetTheme == nullptr"))
	float WheelSize = 800.0f;

	// The distance from the center of the radial wheel widget to place the radial wheel cursor at
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f, EditCondition = "RadialWidgetTheme == nullptr"))
	float WheelCursorDistanceFromCenterWheel = 160.0f;

	// Upon opening the radial menu, should we hide the radial wheel cursor widget?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
    uint8 bHideRadialWheelCursorOnMenuOpened : 1;

	// The color to use when a sector of the radial wheel is currently selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	FLinearColor SelectedColor = FLinearColor(0.765625f, 0.0f, 0.0f, 0.7f);

	// The color to use when a sector of the radial wheel is not currently selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	FLinearColor UnselectedColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);

	// The color to use when a sector of the radial wheel cannot be selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	FLinearColor UnselectableColor = FLinearColor(0.385417f, 0.0f, 0.0f, 0.611f);

	// The font to use to represent text on this radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
    UFont* Font = nullptr;

	// The sound to play when selecting a sector
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	UFMODEvent* SelectionSound = nullptr;

	// The sound to play when opening this radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	UFMODEvent* MenuOpenSound = nullptr;
	
	// The sound to play when closing this radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	UFMODEvent* MenuCloseSound = nullptr;

	// The sound to play when closing this radial menu, if there are no active selections on this menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true, EditCondition = "RadialWidgetTheme == nullptr"))
	UFMODEvent* MenuCloseSound_NoSelection = nullptr;

	// Use this theme data to override all settings previously provided (Optional)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Menu | Spawn Properties", meta = (ExposeOnSpawn = true))
    class URadialWidgetThemeData* RadialWidgetTheme = nullptr;

	// The current position of the mouse cursor on screen
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Mouse Input")
	FVector2D MousePosition = FVector2D(0.0f);

	// The amount the mouse has moved by during a frame
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Mouse Input")
	FVector2D MouseAxisDelta = FVector2D(0.0f);

	// The amount the mouse wheel has moved by during a frame
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Mouse Input")
	float MouseWheelDelta = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Mouse Input")
	float MouseAngle = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Gamepad Input")
	float GamepadXAxis = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Gamepad Input")
	float GamepadYAxis = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Radial Menu | Gamepad Input")
	float GamepadAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu | Time")
	float GlobalTimeDilation = 1.0f;

	static FVector2D Coordinate_TopLeft;
	static FVector2D Coordinate_TopRight;
	static FVector2D Coordinate_BottomLeft;
	static FVector2D Coordinate_BottomRight;
	static FVector2D Coordinate_Middle;
	static FVector2D Coordinate_TopMiddle;
	static FVector2D Coordinate_BottomMiddle;
	static FVector2D Coordinate_LeftMiddle;
	static FVector2D Coordinate_RightMiddle;

	uint8 bFirstTickRun : 1;

private:
	UFUNCTION() // <- Required for timer delegate that takes in parameters
	void OpenWheel_Internal(bool bForceRefresh = false);

	UFUNCTION() // <- Required for timer delegate that takes in parameters
	void CloseWheel_Internal(bool bExecuteRadial = true, bool bRemoveFromParent = true, bool bHideMouseCursor = true, ERadialMenuCloseReason CloseReason = ERadialMenuCloseReason::MCR_UserClosed);

	void DetermineSelectedSector_Internal(UWidget* RadialCursorWidget, const ESlateVisibility& RadialCursorVisibility = ESlateVisibility::Visible);

	ERadialMenuCloseReason LastCloseReason = ERadialMenuCloseReason::MCR_UserClosed;
	
	FTimerHandle TH_OpenDelayExpiry;
	FTimerHandle TH_CloseDelayExpiry;
};
