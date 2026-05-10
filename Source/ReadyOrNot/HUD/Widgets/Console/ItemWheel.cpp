// Copyright Void Interactive, 2023

#include "HUD/Widgets/Console/ItemWheel.h"
#include "Animation/UMGSequencePlayer.h"
#include "Components/ContentWidget.h"
#include "Components/InvalidationBox.h"
PRAGMA_DISABLE_OPTIMIZATION
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "ItemWheelMagazineElement.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

void UItemWheel::Enable()
{
	StopAnimation(InnerWheelFadeIn);
	StopAnimation(InnerWheelFadeOut);
	StopAnimation(OuterWheelFadeIn);
	StopAnimation(OuterWheelFadeOut);
	
	ShowOuterWheelMenu = false;
	OuterWheelCanvasPanel->SetRenderOpacity(0);
	if(!IsValid(NoMagazineIcon))
	{
		NoMagazineIcon = (UTexture2D*)(MagazineCategory->Brush.GetResourceObject());
	}

	HasGrenades = GrenadeStinger->IsSelectable() || GrenadeFlashbang->IsSelectable() || GrenadeCSGas->IsSelectable();
	
	Reset();
	SelectionCompleted = false;
	StopAnimation(FadeOut);
	if (!IsAnimationPlaying(FadeIn))
	{
		PlayAnimation(FadeIn);
	}
	UpdateMagazineStatus(true);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	InvalidateLayoutAndVolatility();

	GrenadeWheel->SetVisibility(ESlateVisibility::Collapsed);
	TacticalWheel->SetVisibility(ESlateVisibility::Collapsed);
	MiscWheel->SetVisibility(ESlateVisibility::Collapsed);
	MagazineWheel->SetVisibility(ESlateVisibility::Collapsed);
}

void UItemWheel::Disable()
{
	StopAnimation(FadeIn);
	SelectionCompleted = true;
	if (!IsAnimationPlaying(FadeOut))
	{
		PlayAnimation(FadeOut, (1.0 - GetRenderOpacity()));
	}
}

void UItemWheel::Reset()
{
	HeaderText->SetText(FText::FromString(""));
	PreviouslySelectedInnerItem = nullptr;
	InnerWheelPosition = FVector::ZeroVector;
	OuterWheelPosition = FVector::ZeroVector;
	ResetOuterSelection();
	ResetSelection();
	CategoryPreSelectionState = NoneSelected;
	OuterCategorySelectionState = NoneSelected;
	CategorySelectionState = NoneSelected;
}

void UItemWheel::CancelSelection()
{
	if (!SelectionCompleted)
	{
		Reset();
	}
}

void UItemWheel::SetSelection(FVector& Vector)
{
	UImage* SelectedCategory = PreviouslySelectedInnerItem;
	
	if (Vector.Size() > 0.9f)
	{
		auto previousNumOuterWheelOptions = NumOuterWheelOptions;
		auto previousSelectedCategory = SelectedCategory;
		
		InnerWheelPosition = FVector::ZeroVector;
		HeaderText->SetText(FText::FromString(""));
		SelectedCategory = nullptr;
		ResetSelection();
		if (HasGrenades && (Vector.X >= 0.0f && Vector.Y <= UE_INV_SQRT_2 && Vector.Y >= -UE_INV_SQRT_2))
		{
			CategoryPreSelectionState = GrenadeSelected;
			SelectedCategory = GrenadeCategory;
			HeaderText->SetText(FText::FromString("GRENADES"));
			NumOuterWheelOptions = 3;
		}
		else if (Vector.Y <= 0.0f && Vector.X <= UE_INV_SQRT_2 && Vector.X >= -UE_INV_SQRT_2)
		{
			CategoryPreSelectionState = TacticalSelected;
			SelectedCategory = TacticalCategory;
			HeaderText->SetText(FText::FromString("TACTICAL"));
			NumOuterWheelOptions = 5;
		}
		else if (Vector.X <= 0.0f && Vector.Y <= UE_INV_SQRT_2 && Vector.Y >= -UE_INV_SQRT_2)
		{
			CategoryPreSelectionState = MiscSelected;
			SelectedCategory = MiscCategory;
			HeaderText->SetText(FText::FromString("MISCELLANEOUS"));
			NumOuterWheelOptions = 3;
		}
		else if (CurrentWeaponHasMagazines)
		{
			if(!OuterMagazineIconsUpdated)
			{
				UpdateOuterMagazineIcon();
			}
			CategoryPreSelectionState = MagazineSelected;
			SelectedCategory = MagazineCategory;
			HeaderText->SetText(FText::FromString("AMMO TYPE"));
			NumOuterWheelOptions = 2;
		}
		if(IsValid(SelectedCategory))
		{
			InnerWheelPosition = Vector;
		}

		if(previousSelectedCategory != SelectedCategory)
		{
			InnerWheel->InvalidateLayoutAndVolatility();
			HeaderText->InvalidateLayoutAndVolatility();
		}
		
		if(NumOuterWheelOptions != previousNumOuterWheelOptions)
		{
			OnNumberOfSegmentsChange(NumOuterWheelOptions);
		}
	}
	
	if (CategoryPreSelectionState != NoneSelected && Vector.Size() < 0.1f)
	{
		CategorySelectionState = CategoryPreSelectionState;
		GrenadeCategory->SetColorAndOpacity(ItemWheelDisabledColor);
		TacticalCategory->SetColorAndOpacity(ItemWheelDisabledColor);
		MiscCategory->SetColorAndOpacity(ItemWheelDisabledColor);
		MagazineCategory->SetColorAndOpacity(ItemWheelDisabledColor);
	}
	
	if(IsValid(SelectedCategory))
	{
		SelectedCategory->SetColorAndOpacity(ItemWheelSelectedColor);
		PreviouslySelectedInnerItem = SelectedCategory;
	}
}

void UItemWheel::ConfirmSelection()
{
	switch (OuterCategorySelectionState)
	{
		case GrenadeStingerSelected:
			PlayerCharacter->EquipStinger();
			SetSelectedUItemWheelElement(GrenadeStinger);
			break;
		case GrenadeFlashbangSelected:
			PlayerCharacter->EquipFlashbang();
			SetSelectedUItemWheelElement(GrenadeFlashbang);
			break;
		case GrenadeCSGasSelected:
			PlayerCharacter->EquipCSGas();
			SetSelectedUItemWheelElement(GrenadeCSGas);
			break;
		case TacticalC2Selected:
			PlayerCharacter->EquipC2();
			SetSelectedUItemWheelElement(TacticalC2);
			break;
		case TacticalSpraySelected:
			PlayerCharacter->EquipPepperspray();
			SetSelectedUItemWheelElement(TacticalSpray);
			break;
		case TacticalWedgeSelected:
			PlayerCharacter->EquipDoorJam();
			SetSelectedUItemWheelElement(TacticalWedge);
			break;
		case TacticalTaserSelected:
			PlayerCharacter->EquipItemOfType(EItemCategory::IC_Taser);
			SetSelectedUItemWheelElement(TacticalTaser);
			break;
		case TacticalLockpickGunSelected:
			PlayerCharacter->EquipItemOfType(EItemCategory::IC_LockpickGun);
			SetSelectedUItemWheelElement(TacticalLockpickGun);
			break;
		case MiscDetonatorSelected:
			PlayerCharacter->EquipDetonator();
			SetSelectedUItemWheelElement(MiscDetonator);
			break;
		case MiscZipcuffsSelected:
			PlayerCharacter->EquipZipcuffs();
			SetSelectedUItemWheelElement(MiscZipcuffs);
			break;
		case MiscMultitoolSelected:
			PlayerCharacter->EquipMultitool();
			SetSelectedUItemWheelElement(MiscMultitool);
			break;
		case MagazineSlot1Selected:
			PlayerCharacter->SwitchAmmoType(MagazineSlot1->GetAmmoType());
			SetSelectedUItemWheelElement(MagazineSlot1);
			break;
		case MagazineSlot2Selected:
			PlayerCharacter->SwitchAmmoType(MagazineSlot2->GetAmmoType());
			SetSelectedUItemWheelElement(MagazineSlot2);
		default:
			break;
	}
	
	SelectionCompleted = true;
	Disable();
}

void UItemWheel::SetOuterSelection(FVector& Vector)
{
	EItemWheelSelection previousSelection = OuterCategorySelectionState;
	OuterCategorySelectionState = NoneSelected;

	auto segmentIndex = GetSegmentIndex(Vector, NumOuterWheelOptions);
	UItemWheelElement* SelectedElement = nullptr;

	UCanvasPanel* currentWheel = nullptr;
	
	if (CategorySelectionState == GrenadeSelected)
	{
		currentWheel = GrenadeWheel;
		GrenadeWheel->SetVisibility(ESlateVisibility::Visible);
		SelectedElement = SetGrenadeSelection(segmentIndex);
	}
	else if (CategorySelectionState == TacticalSelected)
	{
		currentWheel = TacticalWheel;
		TacticalWheel->SetVisibility(ESlateVisibility::Visible);
		SelectedElement = SetTacticalSelection(segmentIndex);
	}
	else if (CategorySelectionState == MiscSelected)
	{
		currentWheel = MiscWheel;
		MiscWheel->SetVisibility(ESlateVisibility::Visible);
		SelectedElement = SetMiscSelection(segmentIndex);
	}
	else if (CategorySelectionState == MagazineSelected)
	{
		currentWheel = MagazineWheel;
		MagazineWheel->SetVisibility(ESlateVisibility::Visible); 
		SelectedElement = SetMagazineSelection(segmentIndex);
	}

	if(IsValid(currentWheel))
	{
		GrenadeWheel->SetVisibility(currentWheel == GrenadeWheel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		TacticalWheel->SetVisibility(currentWheel == TacticalWheel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		MiscWheel->SetVisibility(currentWheel == MiscWheel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		MagazineWheel->SetVisibility(currentWheel == MagazineWheel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	if(OuterCategorySelectionState == NoneSelected || !IsValid(SelectedElement) || !SelectedElement->IsSelectable())
	{
		OuterCategorySelectionState = previousSelection;
		return;
	}
	
	SetSelectedUItemWheelElement(SelectedElement);
	auto Vector2D = GetSegmentPosition(segmentIndex);
	OuterWheelPosition = FVector(Vector2D.X, Vector2D.Y, 0);;
}

UItemWheelElement* UItemWheel::SetGrenadeSelection(const int segmentIndex)
{	
	GrenadeFlashbang->Update();
	GrenadeCSGas->Update();
	GrenadeStinger->Update();

	if(segmentIndex < 0)
	{
		return nullptr;
	}
	
	switch(segmentIndex)
	{
		case 0:
			OuterCategorySelectionState = GrenadeFlashbangSelected;
			return GrenadeFlashbang;
		case 1:
			OuterCategorySelectionState = GrenadeCSGasSelected;
			return GrenadeCSGas;
		case 2:
			OuterCategorySelectionState = GrenadeStingerSelected;
			return GrenadeStinger;
	}

	return nullptr;
}

UItemWheelElement* UItemWheel::SetTacticalSelection(const int index)
{
	TacticalC2->Update();
	TacticalSpray->Update();
	TacticalWedge->Update();
	TacticalTaser->Update();
	TacticalLockpickGun->Update();	

	if(index < 0)
	{
		return nullptr;
	}
	
	switch(index)
	{
	case 0:
		OuterCategorySelectionState = TacticalC2Selected;
		return TacticalC2;
	case 1:
		OuterCategorySelectionState = TacticalSpraySelected;
		return TacticalSpray;
	case 2:
		OuterCategorySelectionState = TacticalTaserSelected;
		return TacticalTaser;
	case 3:
		OuterCategorySelectionState = TacticalWedgeSelected;
		return TacticalWedge;
	case 4:
		OuterCategorySelectionState = TacticalLockpickGunSelected;
		return TacticalLockpickGun;
	}
	
	return nullptr;
}

UItemWheelElement* UItemWheel::SetMiscSelection(const int segmentIndex)
{
	MiscDetonator->Update();
	MiscZipcuffs->Update();
	MiscMultitool->Update();
	
	if(segmentIndex < 0)
	{
		return nullptr;
	}
	
	switch(segmentIndex)
	{
		case 0:
			OuterCategorySelectionState = MiscDetonatorSelected;
			return MiscDetonator;
		case 1:
			OuterCategorySelectionState = MiscZipcuffsSelected;
			return MiscZipcuffs;
		case 2:
			OuterCategorySelectionState = MiscMultitoolSelected;
			return MiscMultitool;
	}

	return nullptr;
}

UItemWheelElement* UItemWheel::SetMagazineSelection(const int segmentIndex)
{
	if(segmentIndex < 0)
	{
		return nullptr;
	}

	switch(segmentIndex)
	{
	case 0:
		OuterCategorySelectionState = MagazineSlot1Selected;
		return MagazineSlot1;
	case 1:
		OuterCategorySelectionState = MagazineSlot2Selected;
		return MagazineSlot2;
	}

	return nullptr;
}

FVector2D UItemWheel::GetSegmentPosition(int SlotIndex)
{
	const float PolarAngleRad = (2 * PI / NumOuterWheelOptions);
	float x, y;
	// ##UE5UPGRADE## FMath
	FMath::PolarToCartesian<float>(150, PolarAngleRad * SlotIndex - HALF_PI, x,y);
	return FVector2D(x,y);
}

FVector UItemWheel::GetInnerWheelPosition()
{
	return InnerWheelPosition.GetSafeNormal();
}

FVector UItemWheel::GetOuterWheelPosition()
{
	return OuterWheelPosition.GetSafeNormal();
}

int UItemWheel::GetSegmentIndex(const FVector& InputVector, const int NumSegments)
{
	if(InputVector.Size() < 0.9f)
	{
		return -1;
	}
	
	const float HeadingAngle = InputVector.HeadingAngle() + PI;
	const float SegmentSize = 2 * PI / NumSegments;
	return FMath::FloorToInt(HeadingAngle / SegmentSize - 0.5f);
}

void UItemWheel::ResetSelection()
{
	if(!IsValid(TacticalCategory))
	{
		return;
	}
	TacticalCategory->SetColorAndOpacity(ItemWheelDefaultColor);
	MiscCategory->SetColorAndOpacity(ItemWheelDefaultColor);
	GrenadeCategory->SetColorAndOpacity(HasGrenades ? ItemWheelDefaultColor : ItemWheelDisabledColor);
	UpdateMagazineStatus();
	MagazineCategory->SetColorAndOpacity(CurrentWeaponHasMagazines ? ItemWheelDefaultColor : ItemWheelDisabledColor);
}

void UItemWheel::ResetOuterSelection()
{
	OuterWheelPosition = FVector::ZeroVector;
	OuterCategorySelectionState = NoneSelected;
	OuterMagazineIconsUpdated = false;
	ResetUItemWheelElement(GrenadeFlashbang);
	ResetUItemWheelElement(GrenadeCSGas);
	ResetUItemWheelElement(GrenadeStinger);
	ResetUItemWheelElement(TacticalC2);
	ResetUItemWheelElement(TacticalSpray);
	ResetUItemWheelElement(TacticalWedge);
	ResetUItemWheelElement(TacticalTaser);
	ResetUItemWheelElement(TacticalLockpickGun);
	ResetUItemWheelElement(MiscMultitool);
	ResetUItemWheelElement(MiscZipcuffs);
	ResetUItemWheelElement(MiscDetonator);
	ResetUItemWheelElement(MagazineSlot1);
	ResetUItemWheelElement(MagazineSlot2);
}

void UItemWheel::ResetUItemWheelElement(UItemWheelElement* Element)
{
	if(!IsValid(Element))
	{
		return;
	}
	
	Element->SetColorAndOpacity(Element->IsSelectable() ? ItemWheelDefaultColor : ItemWheelDisabledColor);
}

void UItemWheel::SetSelectedUItemWheelElement(UItemWheelElement* Element)
{
	InnerWheel->InvalidateLayoutAndVolatility();
	HeaderText->InvalidateLayoutAndVolatility();
	
	if(!IsValid(Element))
	{
		return;
	}

	FString selectedLabel;
	switch(OuterCategorySelectionState)
	{
		case GrenadeStingerSelected:
			selectedLabel = "STINGER";
			break;
		case GrenadeFlashbangSelected:
			selectedLabel = "FLASHBANG";
			break;
		case GrenadeCSGasSelected:
			selectedLabel = "CS GAS";
			break;
		case TacticalC2Selected:
			selectedLabel = "C2 CHARGER";
			break;
		case TacticalSpraySelected:
			selectedLabel = "PEPPER SPRAY";
			break;
		case TacticalWedgeSelected:
			selectedLabel = "DOOR WEDGE";
			break;
		case TacticalTaserSelected:
			selectedLabel = "TASER 7";
			break;
		case TacticalLockpickGunSelected:
			selectedLabel = "LOCKPICK GUN";
			break;
		case MiscDetonatorSelected:
			selectedLabel = "DETONATOR";
			break;
		case MiscZipcuffsSelected:
			selectedLabel = "ZIPCUFFS";
			break;
		case MiscMultitoolSelected:
			selectedLabel = "MULTITOOL";
			break;
		case MagazineSlot1Selected:
			selectedLabel = MagazineSlot1->GetDisplayName();
			break;
		case MagazineSlot2Selected:
			selectedLabel = MagazineSlot2->GetDisplayName();
			break;
		default:
			selectedLabel = "Unknown selection";
		break;
	}
	
	HeaderText->SetText(FText::FromString(selectedLabel));
	
	if(IsValid(PreviouslySelectedOuterItem))
	{
		ResetUItemWheelElement(PreviouslySelectedOuterItem);
	}
	
	if(IsValid(Element) && Element->IsSelectable()) {
		Element->SetColorAndOpacity(ItemWheelSelectedColor);
		PreviouslySelectedOuterItem = Element;
	}
}

void UItemWheel::UpdateMagazineStatus(bool ForceIconRedraw)
{
	ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(PlayerCharacter->GetEquippedWeapon());

	auto newWeapon = Weapon != CurrentWeapon;
	if(!ForceIconRedraw && !newWeapon)
	{
		return;
	}
	
	CurrentWeapon = Weapon;
	CurrentWeaponHasMagazines = IsValid(CurrentWeapon) && (CurrentWeapon->GetAmmunitionTypes()).Num() > 1;
	
	if(!newWeapon)
	{
		return;
	}

	UpdateInnerMagazineIcon();
	
	if(CategorySelectionState == MagazineSelected || PreviouslySelectedInnerItem == MagazineCategory)
	{
		Reset();
	}

	OuterMagazineIconsUpdated = false;
}

void UItemWheel::UpdateInnerMagazineIcon()
{
	MagazineCategory->SetColorAndOpacity(CurrentWeaponHasMagazines ?  ItemWheelDefaultColor : ItemWheelDisabledColor);
	auto magazine = CurrentWeaponHasMagazines ? CurrentWeapon->MagCheckIcon_Full : NoMagazineIcon;
	auto brush = UWidgetBlueprintLibrary::MakeBrushFromTexture(magazine);
	MagazineCategory->SetBrush(brush);
}

void UItemWheel::UpdateOuterMagazineIcon()
{
	auto magazine = CurrentWeaponHasMagazines ? CurrentWeapon->MagCheckIcon_Full : NoMagazineIcon;
	if(IsValid(magazine))
	{
		auto brush = UWidgetBlueprintLibrary::MakeBrushFromTexture(magazine);
		MagazineSlot1->SetBrush(brush);
		MagazineSlot2->SetBrush(brush);
	}
	
	if(IsValid(CurrentWeapon) && CurrentWeaponHasMagazines)
	{
		TArray<FName> AmmoTypes = CurrentWeapon->GetAmmunitionTypes();
		MagazineSlot1->SetVisibility(ESlateVisibility::Visible);
		MagazineSlot1->SetAmmoType(AmmoTypes[0]);
		MagazineSlot2->SetVisibility(ESlateVisibility::Visible);
		MagazineSlot2->SetAmmoType(AmmoTypes[1]);
	}
	
	MagazineSlot1->Update();
	MagazineSlot2->Update();	
	ResetUItemWheelElement(MagazineSlot1);
	ResetUItemWheelElement(MagazineSlot2);

	OuterMagazineIconsUpdated = true;
}

void UItemWheel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (SelectionCompleted)
		return;

	const AReadyOrNotPlayerController* ReadyOrNotPlayerController = Cast<AReadyOrNotPlayerController>(
		GetOwningPlayerPawn()->GetController());
	float x = 0.0f, y = 0.0f;
	ReadyOrNotPlayerController->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, x, y);
	FVector Vector = FVector(x, y, 0);
	Update(Vector);
}

void UItemWheel::Update(FVector& Vector)
{
	if (CategorySelectionState == NoneSelected)
	{
		SetSelection(Vector);
	}
	else if (!SelectionCompleted)
	{
		SetOuterSelection(Vector);
	}

	// Recalculate the number of magazine slots, since weapon can change while the wheel is open
	UpdateMagazineStatus();

	auto wasShowningOuterWheel = ShowOuterWheelMenu;
	ShowOuterWheelMenu = CategorySelectionState != NoneSelected;

	if(wasShowningOuterWheel != ShowOuterWheelMenu)
	{
		// Inner wheel has opacity 0.5 when faded out
		auto innerWheelFadedInPercentage = (InnerWheelCanvasPanel->GetRenderOpacity()-0.5f) * 2;
		
		if(ShowOuterWheelMenu)
		{
			StopAnimation(InnerWheelFadeIn);
			PlayAnimation(InnerWheelFadeOut,innerWheelFadedInPercentage);
			
			StopAnimation(OuterWheelFadeOut);
			PlayAnimation(OuterWheelFadeIn, (OuterWheelCanvasPanel->GetRenderOpacity()-1));
		}
		else
		{
			StopAnimation(InnerWheelFadeOut);
			PlayAnimation(InnerWheelFadeIn, 1 - innerWheelFadedInPercentage);
			
			StopAnimation(OuterWheelFadeIn);
			PlayAnimation(OuterWheelFadeOut, (1-OuterWheelCanvasPanel->GetRenderOpacity()));
		}
	}
}

void UItemWheel::NativeConstruct()
{
	Super::NativeConstruct();
	SetRenderOpacity(0.0f);
	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
}
PRAGMA_ENABLE_OPTIMIZATION