// Copyright Void Interactive, 2023

#pragma once

#include "ItemWheelMagazineElement.h"
#include "ItemWheelElement.h"
#include "ItemWheel.generated.h"

#define ITEM_WHEEL_OUTER_RADIUS 150.0

UCLASS()
class READYORNOT_API UItemWheel : public UUserWidget
{
	GENERATED_BODY()

public:
	void Enable();
	void Disable();
	void Reset();
	void CancelSelection();
	void Update(FVector& Input);
	void SetSelection(FVector& Input);
	void ConfirmSelection();
	void SetOuterSelection(FVector& Input);
	UItemWheelElement*  SetGrenadeSelection(const int segmentIndex);
	UItemWheelElement* SetTacticalSelection(const int segmentIndex);
	UItemWheelElement*  SetMiscSelection(const int segmentIndex);
	UItemWheelElement* SetMagazineSelection(const int segmentIndex);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeIn;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeOut;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* OuterWheelFadeIn;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* OuterWheelFadeOut;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* InnerWheelFadeIn;

	UPROPERTY(BlueprintReadOnly, Category = "Required Animations", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* InnerWheelFadeOut;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UUserWidget* OuterWheel = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UImage* TacticalCategory = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UImage* MiscCategory = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UImage* GrenadeCategory = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UImage* MagazineCategory = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* GrenadeFlashbang = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* GrenadeCSGas = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* GrenadeStinger = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* TacticalC2 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* TacticalSpray = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* TacticalWedge = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* TacticalTaser = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* TacticalLockpickGun = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* MiscDetonator = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* MiscMultitool = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelElement* MiscZipcuffs = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelMagazineElement* MagazineSlot1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UItemWheelMagazineElement* MagazineSlot2 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UCanvasPanel* GrenadeWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UCanvasPanel* TacticalWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UCanvasPanel* MiscWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UCanvasPanel* MagazineWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UUserWidget* InnerWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UCanvasPanel* InnerWheelCanvasPanel	 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UCanvasPanel* OuterWheelCanvasPanel	 = nullptr;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnNumberOfSegmentsChange(int NumSegments);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnMagazineIconChange(UTexture2D* Icon);

	UFUNCTION(BlueprintImplementableEvent)
	void SetInnerWheelOpacity(float opacity);
	
	UFUNCTION(BlueprintCallable)
	FVector GetInnerWheelPosition();

	UFUNCTION(BlueprintCallable)
	FVector GetOuterWheelPosition();
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	bool ShowOuterWheelMenu = false;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	UTextBlock* HeaderText;
	
private:
	FLinearColor ItemWheelDefaultColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
	FLinearColor ItemWheelDisabledColor =  FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
	FLinearColor ItemWheelSelectedColor = FLinearColor(0, 0, 0, 0.9f);

	FVector InnerWheelPosition = FVector::ZeroVector;
	FVector OuterWheelPosition = FVector::ZeroVector;

	int NumOuterWheelOptions = -1;
	bool CurrentWeaponHasMagazines = false;
	ABaseMagazineWeapon* CurrentWeapon;
	bool OuterMagazineIconsUpdated = false;

	bool HasGrenades = false;

	int GetSegmentIndex(const FVector& InputVector, const int NumSegments);
	FVector2D GetSegmentPosition(int SlotIndex);

	bool SelectionCompleted = true;
	UImage *PreviouslySelectedInnerItem = nullptr;
	UItemWheelElement *PreviouslySelectedOuterItem = nullptr;
	
	UTexture2D* NoMagazineIcon = nullptr;
	
	enum EItemWheelSelection
	{
		NoneSelected,
		GrenadeSelected,
		TacticalSelected,
		MiscSelected,
		MagazineSelected,
		GrenadeFlashbangSelected,
		GrenadeCSGasSelected,
		GrenadeStingerSelected,
		TacticalC2Selected,
		TacticalSpraySelected,
		TacticalWedgeSelected,
		TacticalLockpickGunSelected,
		TacticalTaserSelected,
		MiscMultitoolSelected,
		MiscDetonatorSelected,
		MiscZipcuffsSelected,
		MagazineSlot1Selected,
		MagazineSlot2Selected,
		MagazineSlot3Selected,
	};

	EItemWheelSelection CategoryPreSelectionState = NoneSelected;
	EItemWheelSelection CategorySelectionState = NoneSelected;
	EItemWheelSelection OuterCategorySelectionState = NoneSelected;

	APlayerCharacter* PlayerCharacter;
	
	FTimerHandle AnimationTimer;
	
	void ResetSelection();
	void ResetOuterSelection();
	void ResetUItemWheelElement(UItemWheelElement *Element);
	void SetSelectedUItemWheelElement(UItemWheelElement *Element);
	void UpdateMagazineStatus(bool ForceIconRedraw = false);
	void UpdateInnerMagazineIcon();
	void UpdateOuterMagazineIcon();
};
