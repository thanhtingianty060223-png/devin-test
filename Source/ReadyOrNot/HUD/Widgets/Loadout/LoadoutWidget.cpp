// Copyright Void Interactive, 2023


#include "LoadoutWidget.h"

#include "LoadoutInformationTableWidget.h"
#include "LoadoutSlotWidget.h"
#include "LoadoutOverviewWidget.h"
#include "LoadoutVerticalItemListWidget.h"
#include "LoadoutUnitSelectWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

ULoadoutWidget* ULoadoutWidget::ActiveLoadoutWidget = nullptr;

ULoadoutWidget::ULoadoutWidget()
{
	ActiveLoadoutWidget = this;
}

ULoadoutWidget* ULoadoutWidget::GetLoadoutWidget()
{
	return ActiveLoadoutWidget;
}

void ULoadoutWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetupBindings();
}

void ULoadoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WLoadoutCharacterSelect->Init();
	InitializeOverview();
	bUsingPreset = !ActiveLoadout.PresetName.IsEmpty();
	if (bUsingPreset)
		CurrentLoadoutPreset = ActiveLoadout.PresetName;
}

void ULoadoutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	gs->bInPlanningMenu = true;

	HideHUD();
	LoadWeaponPresets();
	LoadWeaponAttachments();
	LoadLoadoutPresets();
	LoadWeaponDefaultFireModes();
	Character = Cast<AReadyOrNotCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	InventoryComp = Character->GetInventoryComponent();
	SetPreMissionCameraBySlot(EItemCategory::IC_None, 0.75); 
}

void ULoadoutWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateWorkbenchCameraRotation(InDeltaTime);
}

bool ULoadoutWidget::NativeOnHandleBackAction()
{
	GoBack();
	
	return true;
}

FReply ULoadoutWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton) {
		GoBack();
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape) {
		GoBack();
		return FReply::Handled();
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULoadoutWidget::SetupBindings()
{
	//InfoPanel = WLoadoutInformationTableC1; Should be bound already.
	InfoPanel->OnModifyWeaponButtonClicked.AddDynamic(this, &ULoadoutWidget::OpenWeaponQuartermaster);

	WLoadoutOverview->OnOverviewItemClicked.AddDynamic(this, &ULoadoutWidget::ItemClicked);
	WLoadoutOverview->OnOverviewItemHovered.AddDynamic(this, &ULoadoutWidget::ItemHovered);
	WLoadoutOverview->OnOverviewItemUnhovered.AddDynamic(this, &ULoadoutWidget::ItemUnhovered);
	WLoadoutOverview->OnAttachmentSlotClicked.AddDynamic(this, &ULoadoutWidget::AttachmentClicked);
	WLoadoutOverview->OnAttachmentSlotHovered.AddDynamic(this, &ULoadoutWidget::AttachmentHovered);
	WLoadoutOverview->OnAttachmentSlotUnhovered.AddDynamic(this, &ULoadoutWidget::AttachmentUnhovered);

	VerticalItemList->OnOverviewItemHovered.AddDynamic(this, &ULoadoutWidget::ItemHovered);
	VerticalItemList->OnOverviewItemUnhovered.AddDynamic(this, &ULoadoutWidget::ItemUnhovered);
	VerticalItemList->OnAttachmentSlotHovered.AddDynamic(this, &ULoadoutWidget::AttachmentHovered);
	VerticalItemList->OnAttachmentSlotUnhovered.AddDynamic(this, &ULoadoutWidget::AttachmentUnhovered);
}

void ULoadoutWidget::InitializeOverview()
{
	SetPreMissionCameraBySlot(CurrentActiveSlot != nullptr ? CurrentActiveSlot->LoadoutSlot : EItemCategory::IC_None, 0.75f);
	WLoadoutOverview->InitializeOverviewList(bRemotePlayer);
}

void ULoadoutWidget::InitializeItemSelectionPanel(ULoadoutSlotWidget *TriggeringSlot)
{
	if (CurrentActiveSlot != TriggeringSlot) {
		if (CurrentActiveSlot) {
			CurrentActiveSlot->SetActive(false);
			HideItemList();
		}
	}

	if (CurrentActiveSlot != nullptr) {
		//UpdateCurrentLoadoutUISlot(); LoadoutUI interface
		CurrentActiveSlot->SetActive(true);
		if (CurrentActiveSlot->bIsAmmunition) {
			OpenAmmoList(true, CurrentAmmoType, CurrentActiveSlot->bIsPrimary ? ActiveLoadout.Primary : ActiveLoadout.Secondary);
		} else if (CurrentActiveSlot->bIsArmourMaterial) {
			OpenArmourMaterialList(true);
		} else if (CurrentActiveSlot->LoadoutSlot == EItemCategory::IC_TacticalDevice || CurrentActiveSlot->LoadoutSlot == EItemCategory::IC_Grenade) {
			OpenDeployableList(true, CurrentActiveSlot->LoadoutSlot, CurrentActiveSlot->GearCategoryClasses, ListExclusionItems);
		} else {
			OpenItemList(true, CurrentActiveSlot->LoadoutSlot, CurrentActiveSlot->GearCategoryClasses);
		}
	}
}

void ULoadoutWidget::InitializeItemSelectionRemote(ULoadoutSlotWidget *TriggeringSlot)
{
	if (CurrentActiveSlot != TriggeringSlot) {
		if (CurrentActiveSlot != nullptr)
			CurrentActiveSlot->SetActive(false);
		//UpdateCurrentLoadoutUISlot(); LoadoutUI interface
		CurrentActiveSlot->SetActive(true);
	}
}

void ULoadoutWidget::InitializeAttachmentSelectionPanel(ULoadoutSlotWidget *TriggeringSlot)
{
	if (CurrentActiveSlot != TriggeringSlot) {
		if (CurrentActiveSlot != nullptr)
			CurrentActiveSlot->SetActive(false);
		CurrentActiveSlot = TriggeringSlot;
		CurrentActiveSlot->SetActive(true);
		OpenAttachmentList(true, ActiveQuartermasterSlot->ItemData, TriggeringSlot->AttachmentSlot);
	}
}

void ULoadoutWidget::SetActiveLoadout(FString LoadoutName, AReadyOrNotPlayerState *RonPlayerState, EEquippingSwat EquippingUnit)
{
}

void ULoadoutWidget::EquipItem(ABaseItem *ItemData)
{
}

void ULoadoutWidget::PrimaryChanged()
{
}

void ULoadoutWidget::SidearmChanged()
{
}

void ULoadoutWidget::EquipArmor(ABaseItem *ItemData)
{
}

void ULoadoutWidget::EquipHeadwear(ABaseItem *ItemData)
{
}

void ULoadoutWidget::EquipLongTactical(ABaseItem *ItemData)
{
}

void ULoadoutWidget::EquipGrenades(const TArray<ABaseItem *> ItemData)
{
}

void ULoadoutWidget::EquipTactical(const TArray<ABaseItem *> ItemData)
{
}

void ULoadoutWidget::EquipAttachment(ABaseItem *AttachingWeapon, UWeaponAttachment *AttachmentData)
{
}
	
void ULoadoutWidget::EquipPrimaryAttachments(EWeaponAttachmentType AttachmentType, UWeaponAttachment *AttachmentData, ABaseItem *AttachingWeapon)
{
}

void ULoadoutWidget::EquipSidearmAttachments(EWeaponAttachmentType AttachmentType, UWeaponAttachment *AttachmentData, ABaseItem *AttachingWeapon)
{
}

void ULoadoutWidget::SavePrimaryAmmoSlotCount(int PrimaryAmmoSlotCount)
{
}
	
void ULoadoutWidget::SaveSidearmAmmoSlotCount(int SidearmAmmoSlotCount)
{
}

void ULoadoutWidget::SaveGrenadeSlotCount(int GrenadeSlotsCount)
{
}

void ULoadoutWidget::SaveTacticalSlotcount(int TacticalSlotsCount)
{
}

void ULoadoutWidget::EquipPrimaryAmmo(const TArray<FName> &PrimaryAmmo)
{
}

void ULoadoutWidget::EquipSidearmAmmo(const TArray<FName> &SidearmAmmo)
{
}
	
void ULoadoutWidget::SaveArmorCoverage(EArmourCoverage ArmourCoverage)
{
}

void ULoadoutWidget::EquipArmourMaterial(UArmourMaterial *ArmourMaterial)
{
}

void ULoadoutWidget::OpenWeaponQuartermaster(ABaseItem *ItemToModify)
{
}

void ULoadoutWidget::CleanCurrentWeapon()
{
}

void ULoadoutWidget::LookAtAttachmentSlot(EWeaponAttachmentType AttachmentSlot)
{
}

void ULoadoutWidget::SetupWorkbenchCamera()
{
}

void ULoadoutWidget::UpdateWorkbenchCameraRotation(float DeltaTime)
{
	if (bWorkbenchCameraRotating) {
		CurrentWorkbenchCameraRotation =
			UKismetMathLibrary::RInterpTo(
				CurrentWorkbenchCameraRotation,
				TargetWorkbenchCameraRotation,
				DeltaTime,
				WorkbenchCameraLookSpeed);
		WorkbenchCamera->SetActorRotation(CurrentWorkbenchCameraRotation);
		if (CurrentWorkbenchCameraRotation.Equals(TargetWorkbenchCameraRotation, 0.0001f))
			bWorkbenchCameraRotating = false;
	}
}

void ULoadoutWidget::OpenArmorQuartermaster(ABaseItem *ItemToModify)
{
}

void ULoadoutWidget::InitializeQuartermaster()
{
}

void ULoadoutWidget::HideHUD()
{
	TArray<UUserWidget *> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, HUDWidgetClass);
	for (int i = 0; i < FoundWidgets.Num(); ++i) {
		UBaseWidget *BaseWidget = Cast<UBaseWidget>(FoundWidgets[i]);
		if (BaseWidget != nullptr)
			BaseWidget->HideWidget();
	}
}

void ULoadoutWidget::ShowHUD()
{
}

void ULoadoutWidget::UpdateHUDStates()
{
}

void ULoadoutWidget::SaveWeaponPreset(UUserWidget *TriggeringModal, FText TextEntry)
{
}

void ULoadoutWidget::SaveLoadoutPreset(UUserWidget *TriggeringModal, FText TextEntry)
{
}

void ULoadoutWidget::ApplyLoadoutPreset(FLoadoutPreset LoadoutPreset)
{
}

void ULoadoutWidget::OpenLoadoutPresetModal(FString CurrentPreset, bool bDelete)
{
}

void ULoadoutWidget::DeleteLoadoutPreset(UUserWidget *TriggeringModal, FText TextEntry)
{
}

void ULoadoutWidget::CheckPreset()
{
}

void ULoadoutWidget::SetPresetModified(bool bPresetDirty)
{
}

void ULoadoutWidget::ClearPreset()
{
}

void ULoadoutWidget::ShowItemInfoPanel(ULoadoutSlotWidget *LoadoutSlot)
{
	InfoPanel->RefreshPanelItemInfo(ActiveLoadout, LoadoutSlot->ItemData, LoadoutSlot->LoadoutSlot);
}

void ULoadoutWidget::HideItemInfoPanel()
{
	//if (InfoPanel != nullptr)
	//InfoPanel->Hide(); RON UI interface
}

void ULoadoutWidget::RefreshItemInfoPanel(ULoadoutSlotWidget *LoadoutSlot)
{
	if (LoadoutSlot != nullptr) {
		if (LoadoutSlot->bIsAmmunition) {
			InfoPanel->RefreshPanelAmmoInfo(LoadoutSlot->AmmoType, LoadoutSlot->ItemData);
		} else if (LoadoutSlot->bIsArmourMaterial) {
			InfoPanel->RefreshPanelArmourMaterial(LoadoutSlot->ArmourMaterialData);
		} else {
			InfoPanel->RefreshPanelItemInfo(ActiveLoadout, LoadoutSlot->ItemData, LoadoutSlot->LoadoutSlot, bRemotePlayer || bQuartermasterOpen);
		}
	}
}

void ULoadoutWidget::RefreshAttachmentInfoPanel(ULoadoutSlotWidget *LoadoutSlot)
{
}
	
void ULoadoutWidget::ShowToolTip(FLevelDataLookupTable MissionDetails)
{
}

void ULoadoutWidget::GoBack()
{
}

void ULoadoutWidget::DeselectLoadoutSlot()
{
}

void ULoadoutWidget::SetPreMissionCameraBySlot(EItemCategory LoadoutSlot, float BlendTime)
{
	SetPreMissionCamera(*CategoryCameraTags.Find(LoadoutSlot), *CategoryPoses.Find(LoadoutSlot), BlendTime);
}

void ULoadoutWidget::UpdatePreview(EItemCategory Selection)
{
}

void ULoadoutWidget::UpdateAllPreviewWeaponAttachments(bool bIsWorkbench, TSubclassOf<ABaseItem> Weapon, bool bIsSidearm)
{
}

void ULoadoutWidget::InitializeWeaponAttachmentMap()
{
}

bool ULoadoutWidget::HolsterPreviewCharacterWeapon()
{
	bool bPrimaryDrawIsCurrent = CurrentPreviewAnimation == PrimaryDrawAnim;
	bool bSidearmDrawIsCurrent = CurrentPreviewAnimation == SidearmDrawAnim;
	
	if (!(bPrimaryDrawIsCurrent || bSidearmDrawIsCurrent))
		return false;

	if (bPrimaryDrawIsCurrent)
		PlayAnimationOnPreviewCharacter(PrimaryHolsterAnim);
	else if (bSidearmDrawIsCurrent)
		PlayAnimationOnPreviewCharacter(SidearmHolsterAnim);
	else
		PlayAnimationOnPreviewCharacter(FString{});

	Character = GetDefaultPreviewCharacter();
	UFMODBlueprintStatics::PlayEventAttached(
		WeaponHolsteredSound,
		Character->GetFMODVoiceAudioComp(),
		FName{},
		FVector{},
		EAttachLocation::KeepRelativeOffset,
		false,
		true,
		true);
	Character->IsTableMontagePlayingWithTimeRemaining(CurrentPreviewAnimation, HolsterTimeRemaining);

	return true;
}

void ULoadoutWidget::SaveActiveLoadoutOld()
{
}

void ULoadoutWidget::CloseLoadout()
{
}

void ULoadoutWidget::DeselectAttachmentSlot()
{
}

void ULoadoutWidget::UpdateCurrentItem()
{
}
	
void ULoadoutWidget::HideItemList()
{
}

bool ULoadoutWidget::IsNullAttachment(TSubclassOf<UWeaponAttachment> Attachment)
{
	AReadyOrNotPlayerController *RONPC = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	UWorld *World = UBpGameplayHelperLib::GetWorldBP(RONPC);
	UItemData *ItemData = UBpGameplayHelperLib::GetItemData(World);

	return Attachment == ItemData->NullPrimaryScopeAttachment
		|| Attachment == ItemData->NullMuzzleAttachment
		|| Attachment == ItemData->NullUnderbarrelAttachment
		|| Attachment == ItemData->NullOverbarrelAttachment
		|| Attachment == ItemData->NullStockAttachment
		|| Attachment == ItemData->NullGripAttachment
		|| Attachment == ItemData->NullIlluminatorAttachment;
}

void ULoadoutWidget::OpenAmmoList(bool bVerticalList, FName ExcludedAmmoType, TSubclassOf<ABaseItem> Weapon)
{
}

void ULoadoutWidget::OpenArmourMaterialList(bool bVerticalList)
{
}

void ULoadoutWidget::OpenDeployableList(bool bVerticalList, EItemCategory LoadoutSlot, const TArray<FLoadoutCategory> &GearCategoryClasses, const TArray<TSubclassOf<ABaseItem>> &ExcludedItems)
{
}

void ULoadoutWidget::OpenItemList(bool bVerticalList, EItemCategory LoadoutSlot, const TArray<FLoadoutCategory> &GearCategoryClasses)
{
}

void ULoadoutWidget::OpenAttachmentList(bool bVerticalList, TSubclassOf<ABaseItem> ItemData, EWeaponAttachmentType AttachmentType)
{
}

void ULoadoutWidget::LoadDefaultLoadout()
{
}

void ULoadoutWidget::SetActiveQuartermasterSlot()
{
}

void ULoadoutWidget::SetPreMissionCamera(FName Tag, FString Animation, float BlendTime)
{
	SetActiveCameraByTag(Tag.IsNone() ? CurrentCameraTag : Tag, BlendTime);
	AnimationForSetPreMissionCamera = Animation;
	if (CurrentPreviewAnimation.IsEmpty()) {
		PlaySetPreMissionCameraAnimation();
	} else {
		if (HolsterPreviewCharacterWeapon()) {
			FTimerHandle UnusedHandle;
			GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &ULoadoutWidget::PlaySetPreMissionCameraAnimation, BlendTime, false);
		} else {
			PlaySetPreMissionCameraAnimation();
		}
	}
}


void ULoadoutWidget::PlaySetPreMissionCameraAnimation()
{
	if (!AnimationForSetPreMissionCamera.IsEmpty())
		CurrentPreviewAnimation = AnimationForSetPreMissionCamera;
	PlayAnimationOnPreviewCharacter(CurrentPreviewAnimation);
}

void ULoadoutWidget::ItemClicked(ULoadoutSlotWidget *TriggeringSlot)
{
	if (bRemotePlayer)
		InitializeItemSelectionRemote(TriggeringSlot);
	else
		InitializeItemSelectionPanel(TriggeringSlot);
	UpdateAllPreviewWeaponAttachments(false, TriggeringSlot->ItemData, TriggeringSlot->LoadoutSlot == EItemCategory::IC_Secondary);
	SetPreMissionCameraBySlot(CurrentActiveSlot->LoadoutSlot, 0.8f);
}

void ULoadoutWidget::ItemHovered(ULoadoutSlotWidget *TriggeringSlot)
{
	CurrentHoveredSlot = TriggeringSlot;
	if (UnhoverTimer.IsValid()) {
		UnhoverTimer.Invalidate();
	}
	RefreshItemInfoPanel(TriggeringSlot);
}
	
void ULoadoutWidget::ItemUnhovered(ULoadoutSlotWidget *TriggeringSlot)
{
	GetWorld()->GetTimerManager().SetTimer(UnhoverTimer, this, &ULoadoutWidget::DoItemUnhover, InfoPanelHideDelay);
}

void ULoadoutWidget::AttachmentClicked(ULoadoutSlotAttachmentWidget *AttachmentSlot)
{
}

void ULoadoutWidget::AttachmentHovered(ULoadoutSlotAttachmentWidget *AttachmentSlot)
{
}
	
void ULoadoutWidget::AttachmentUnhovered(ULoadoutSlotAttachmentWidget *AttachmentSlot)
{
}

void ULoadoutWidget::DoItemUnhover()
{
	if (CurrentActiveSlot != nullptr) {
		if (CurrentHoveredSlot != CurrentActiveSlot || bQuartermasterOpen)
			RefreshItemInfoPanel(CurrentActiveSlot);
	} else {
		HideItemInfoPanel();
	}
}
