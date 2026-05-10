// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "../PreMissionPlanning.h"
#include "LoadoutCategory.h"
#include "LoadoutWidget.generated.h"

class ULoadoutUnitSelectWidget;
class ULoadoutOverviewWidget;
class ULoadoutVerticalItemListWidget;
class ULoadoutInformationTableWidget;

/**
 * Controller class for the Loadout widget.
 */
UCLASS()
class READYORNOT_API ULoadoutWidget : public UPreMissionPlanning
{
	GENERATED_BODY()

public:
	ULoadoutWidget();

	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeOnHandleBackAction() override;

	
	UFUNCTION(BlueprintPure)
	static class ULoadoutWidget* GetLoadoutWidget();

	UPROPERTY(meta=(BindWidget))
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(meta=(BindWidget))
	ULoadoutUnitSelectWidget *WLoadoutCharacterSelect;

	UPROPERTY(meta=(BindWidget))
	ULoadoutOverviewWidget *WLoadoutOverview;

	UPROPERTY(meta=(BindWidget))
	ULoadoutVerticalItemListWidget *VerticalItemList;

	UPROPERTY(meta=(BindWidget))
	ULoadoutInformationTableWidget *InfoPanel;

protected:
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	static ULoadoutWidget* ActiveLoadoutWidget;

	UPROPERTY(EditAnywhere)
	TMap<EItemCategory, FName> CategoryCameraTags;
	
	UPROPERTY(EditAnywhere)
	TMap<EItemCategory, FString> CategoryPoses;
	
	UPROPERTY(EditAnywhere)
	FString PrimaryDrawAnim;

	UPROPERTY(EditAnywhere)
	FString SidearmDrawAnim;
	
	UPROPERTY(EditAnywhere)
	FString PrimaryHolsterAnim;

	UPROPERTY(EditAnywhere)
	FString SidearmHolsterAnim;

	UPROPERTY(EditAnywhere)
	UFMODEvent *WeaponHolsteredSound;
	
	FName CurrentCameraTag;
	FString CurrentPreviewAnimation;
	FString AnimationForSetPreMissionCamera;

	UFUNCTION(Category=Initialization)
	void SetupBindings();

	UFUNCTION(Category=Initialization)
	void InitializeOverview();

	UFUNCTION(Category=Initialization)
	void InitializeItemSelectionPanel(ULoadoutSlotWidget *TriggeringSlot);

	UFUNCTION(Category=Initialization)
	void InitializeItemSelectionRemote(ULoadoutSlotWidget *TriggeringSlot);

	UFUNCTION(Category=Initialization)
	void InitializeAttachmentSelectionPanel(ULoadoutSlotWidget *TriggeringSlot);

	UFUNCTION(Category=Loadout)
	void SetActiveLoadout(FString LoadoutName, AReadyOrNotPlayerState *RonPlayerState, EEquippingSwat EquippingUnit);

	UFUNCTION(Category=Loadout)
	void EquipItem(ABaseItem *ItemData);

	UFUNCTION(Category=Loadout)
	void PrimaryChanged();

	UFUNCTION(Category=Loadout)
	void SidearmChanged();

	UFUNCTION(Category=Loadout)
	void EquipArmor(ABaseItem *ItemData);

	UFUNCTION(Category=Loadout)
	void EquipHeadwear(ABaseItem *ItemData);

	UFUNCTION(Category=Loadout)
	void EquipLongTactical(ABaseItem *ItemData);

	UFUNCTION(Category=Loadout)
	void EquipGrenades(const TArray<ABaseItem *> ItemData);

	UFUNCTION(Category=Loadout)
	void EquipTactical(const TArray<ABaseItem *> ItemData);

	UFUNCTION(Category=Loadout)
	void EquipAttachment(ABaseItem *AttachingWeapon, UWeaponAttachment *AttachmentData);
	
	UFUNCTION(Category=Loadout)
	void EquipPrimaryAttachments(EWeaponAttachmentType AttachmentType, UWeaponAttachment *AttachmentData, ABaseItem *AttachingWeapon);

	UFUNCTION(Category=Loadout)
	void EquipSidearmAttachments(EWeaponAttachmentType AttachmentType, UWeaponAttachment *AttachmentData, ABaseItem *AttachingWeapon);

	UFUNCTION(Category=Loadout)
	void SavePrimaryAmmoSlotCount(int PrimaryAmmoSlotCount);
	
	UFUNCTION(Category=Loadout)
	void SaveSidearmAmmoSlotCount(int SidearmAmmoSlotCount);

	UFUNCTION(Category=Loadout)
	void SaveGrenadeSlotCount(int GrenadeSlotsCount);

	UFUNCTION(Category=Loadout)
	void SaveTacticalSlotcount(int TacticalSlotsCount);

	UFUNCTION(Category=Loadout)
	void EquipPrimaryAmmo(const TArray<FName> &PrimaryAmmo);

	UFUNCTION(Category=Loadout)
	void EquipSidearmAmmo(const TArray<FName> &SidearmAmmo);
	
	UFUNCTION(Category=Loadout)
	void SaveArmorCoverage(EArmourCoverage ArmourCoverage);

	UFUNCTION(Category=Loadout)
	void EquipArmourMaterial(UArmourMaterial *ArmourMaterial);

	UFUNCTION(Category=Quartermaster)
	void OpenWeaponQuartermaster(ABaseItem *ItemToModify);

	UFUNCTION(Category=Quartermaster)
	void CleanCurrentWeapon();

	UFUNCTION(Category=Quartermaster)
	void LookAtAttachmentSlot(EWeaponAttachmentType AttachmentSlot);

	UFUNCTION(Category=Quartermaster)
	void SetupWorkbenchCamera();
	
	UFUNCTION(Category=Quartermaster)
	void UpdateWorkbenchCameraRotation(float DeltaTime);
	
	UFUNCTION(Category=Quartermaster)
	void OpenArmorQuartermaster(ABaseItem *ItemToModify);

	UFUNCTION(Category=Quartermaster)
	void InitializeQuartermaster();

	UFUNCTION(Category=HUD)
	void HideHUD();

	UFUNCTION(Category=HUD)
	void ShowHUD();

	UFUNCTION(Category=HUD)
	void UpdateHUDStates();

	UFUNCTION(Category=Presets)
	void SaveWeaponPreset(UUserWidget *TriggeringModal, FText TextEntry);

	UFUNCTION(Category=Presets)
	void SaveLoadoutPreset(UUserWidget *TriggeringModal, FText TextEntry);

	UFUNCTION(Category=Presets)
	void ApplyLoadoutPreset(FLoadoutPreset LoadoutPreset);

	UFUNCTION(Category=Presets)
	void OpenLoadoutPresetModal(FString CurrentPreset, bool bDelete);

	UFUNCTION(Category=Presets)
	void DeleteLoadoutPreset(UUserWidget *TriggeringModal, FText TextEntry);

	UFUNCTION(Category=Presets)
	void CheckPreset();

	UFUNCTION(Category=Presets)
	void SetPresetModified(bool bPresetDirty);

	UFUNCTION(Category=Presets)
	void ClearPreset();

	UFUNCTION(Category=InfoPanel)
	void ShowItemInfoPanel(ULoadoutSlotWidget *LoadoutSlot);

	UFUNCTION(Category=InfoPanel)
	void HideItemInfoPanel();

	UFUNCTION(Category=InfoPanel)
	void RefreshItemInfoPanel(ULoadoutSlotWidget *LoadoutSlot);

	UFUNCTION(Category=InfoPanel)
	void RefreshAttachmentInfoPanel(ULoadoutSlotWidget *LoadoutSlot);
	
	UFUNCTION()
	void ShowToolTip(FLevelDataLookupTable MissionDetails);

	UFUNCTION()
	void GoBack();

	UFUNCTION()
	void DeselectLoadoutSlot();

	UFUNCTION()
	void SetPreMissionCameraBySlot(EItemCategory LoadoutSlot, float BlendTime);

	UFUNCTION()
	void UpdatePreview(EItemCategory Selection);

	UFUNCTION()
	void UpdateAllPreviewWeaponAttachments(bool bIsWorkbench, TSubclassOf<ABaseItem> Weapon, bool bIsSidearm);
	
	UFUNCTION()
	void InitializeWeaponAttachmentMap();

	UFUNCTION()
	bool HolsterPreviewCharacterWeapon();

	UFUNCTION()
	void SaveActiveLoadoutOld();

	UFUNCTION()
	void CloseLoadout();

	UFUNCTION()
	void DeselectAttachmentSlot();

	UFUNCTION()
	void UpdateCurrentItem();
	
	UFUNCTION()
	void HideItemList();

	UFUNCTION()
	bool IsNullAttachment(TSubclassOf<UWeaponAttachment> Attachment);

	UFUNCTION()
	void OpenAmmoList(bool bVerticalList, FName ExcludedAmmoType, TSubclassOf<ABaseItem> Weapon);

	UFUNCTION()
	void OpenArmourMaterialList(bool bVerticalList);

	UFUNCTION()
	void OpenDeployableList(bool bVerticalList, EItemCategory LoadoutSlot, const TArray<FLoadoutCategory> &GearCategoryClasses, const TArray<TSubclassOf<ABaseItem>> &ExcludedItems);

	UFUNCTION()
	void OpenItemList(bool bVerticalList, EItemCategory LoadoutSlot, const TArray<FLoadoutCategory> &GearCategoryClasses);

	UFUNCTION()
	void OpenAttachmentList(bool bVerticalList, TSubclassOf<ABaseItem> ItemData, EWeaponAttachmentType AttachmentType);

	UFUNCTION()
	void LoadDefaultLoadout();

	UFUNCTION()
	void SetActiveQuartermasterSlot();
	
	UFUNCTION(Category=EventHandlers)
	void SetPreMissionCamera(FName Tag, FString Animation, float BlendTime);

	UFUNCTION()
	void PlaySetPreMissionCameraAnimation();

	UFUNCTION(Category=EventHandlers)
	void ItemClicked(ULoadoutSlotWidget *TriggeringSlot);

	UFUNCTION(Category=EventHandlers)
	void ItemHovered(ULoadoutSlotWidget *TriggeringSlot);
	
	UFUNCTION(Category=EventHandlers)
	void ItemUnhovered(ULoadoutSlotWidget *TriggeringSlot);

	UFUNCTION(Category=EventHandlers)
	void AttachmentClicked(ULoadoutSlotAttachmentWidget *AttachmentSlot);

	UFUNCTION(Category=EventHandlers)
	void AttachmentHovered(ULoadoutSlotAttachmentWidget *AttachmentSlot);
	
	UFUNCTION(Category=EventHandlers)
	void AttachmentUnhovered(ULoadoutSlotAttachmentWidget *AttachmentSlot);

	UFUNCTION(Category=EventHandlers)
	void DoItemUnhover();

	AReadyOrNotCharacter *Character = nullptr;
	UInventoryComponent *InventoryComp = nullptr;
	ULoadoutSlotWidget *CurrentActiveSlot = nullptr;
	float HolsterTimeRemaining = 0.0f;
	bool bUsingPreset = false;
	FString CurrentLoadoutPreset;
	bool bWorkbenchCameraRotating = false;
	FRotator CurrentWorkbenchCameraRotation;
	FRotator TargetWorkbenchCameraRotation;
	float WorkbenchCameraLookSpeed = 0.0f;
	ACameraActor *WorkbenchCamera;
	bool bRemotePlayer = false;
	ULoadoutSlotWidget *CurrentHoveredSlot = nullptr;
	FTimerHandle UnhoverTimer;
	float InfoPanelHideDelay = 0.3f;
	bool bQuartermasterOpen = false;
	ULoadoutSlotWidget *ActiveQuartermasterSlot = nullptr;
	FName CurrentAmmoType;
	TArray<TSubclassOf<ABaseItem>> ListExclusionItems;
};
