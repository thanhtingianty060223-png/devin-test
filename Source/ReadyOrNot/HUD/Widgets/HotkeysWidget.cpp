// Void Interactive, 2020

#include "HotkeysWidget.h"
#include "HotkeyWidget.h"
#include "Actors/BaseGrenade.h"
#include "Actors/Items/BallisticsShield.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"

#include "Blueprint/WidgetLayoutLibrary.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

void UHotkeysWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		SetHotkeyVisibility(Hotkey_Grenade, true);
		SetHotkeyVisibility(Hotkey_Chemlight, true);
		SetHotkeyVisibility(Hotkey_Scope, true);
		SetHotkeyVisibility(Hotkey_NVG, true);
		SetHotkeyVisibility(IlluminatorAttachment_Switcher, true);
		SetHotkeyVisibility(Hotkey_Laser, true);
		SetHotkeyVisibility(Hotkey_Flashlight, true);
	}
}

void UHotkeysWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UHotkeysWidget::RefreshHotkeyInput, 0.5f, true, true);
}

void UHotkeysWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCharacter)
	{
		if (Hotkey_Grenade)
		{
			if (PlayerCharacter->QuickThrowItem)
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(PlayerCharacter->QuickThrowItem->Visuals.ItemIcon);
				Hotkey_Grenade->SetHotkeyImage(Brush);
			}

			Hotkey_Grenade->SetHotkeyRemainingUses(PlayerCharacter->GetQuickthrowGrenadeAmmo());
		}

		if (Hotkey_Chemlight)
		{
			Hotkey_Chemlight->SetHotkeyRemainingUses(PlayerCharacter->GetChemlightAmmo());
		}
		
		SetHotkeyVisibility(Hotkey_Grenade, PlayerCharacter->HasGrenadesInInventory() && PlayerCharacter->CanQuickThrow());
		SetHotkeyVisibility(Hotkey_Chemlight, PlayerCharacter->CanThrowChemlight());
		SetHotkeyVisibility(Hotkey_Scope, PlayerCharacter->EquippedWeaponHasSecondarySight());
		SetHotkeyVisibility(Hotkey_NVG, PlayerCharacter->HasNVG() && !PlayerCharacter->IsAnimationBlocking() && !PlayerCharacter->IsStunned());

		const bool bHasLaserAttachment = PlayerCharacter->EquippedWeaponHasLaserAttachment();
		const bool bHasLightAttachment = PlayerCharacter->EquippedWeaponHasLightAttachment();
		
		SetHotkeyVisibility(IlluminatorAttachment_Switcher, bHasLaserAttachment || bHasLightAttachment);
		SetHotkeyVisibility(Hotkey_Laser, bHasLaserAttachment);
		SetHotkeyVisibility(Hotkey_Flashlight, bHasLightAttachment);

		if (IlluminatorAttachment_Switcher)
		{
			IlluminatorAttachment_Switcher->SetActiveWidgetIndex(bHasLaserAttachment ? 0 : bHasLightAttachment ? 1 : 0);
		}
	}
}

void UHotkeysWidget::SetHotkeyVisibility(UWidget* Widget, const bool bCondition)
{
	if (!Widget)
		return;

	const bool CanPerformActions = PlayerCharacter ? !PlayerCharacter->IsStunned() && !PlayerCharacter->IsArrested() && !PlayerCharacter->IsActionsLocked() && !PlayerCharacter->IsCarried() && !PlayerCharacter->IsCarrying() : true;
	
	if (!CanPerformActions)
	{
		if (GetWorld())
			Widget->SetRenderOpacity(FMath::FInterpConstantTo(Widget->GetRenderOpacity(), 0.0f, GetWorld()->DeltaTimeSeconds, 20.0f));
		else
			Widget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		if (GetWorld())
		{
			Widget->SetRenderOpacity(FMath::FInterpConstantTo(Widget->GetRenderOpacity(), bCondition ? 1.0f : 0.0f, GetWorld()->DeltaTimeSeconds, 20.0f));
		}

		Widget->SetVisibility(bCondition ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (UHorizontalBoxSlot* HBS = UWidgetLayoutLibrary::SlotAsHorizontalBoxSlot(Widget))
	{
		// Add some right padding
		//{
		//	const TArray<UWidget*> UncollapsedHotkeys = GetAllUncollapsedWidgets();
		//	const bool bIsLastHotkey = UncollapsedHotkeys.Find(Widget) == UncollapsedHotkeys.Num() - 1; 
		//
		//	HBS->SetPadding(bIsLastHotkey ? FMargin(0.0f) : FMargin(0.0f, 0.0f, SpacingBetweenHotkeys, 0.0f));
		//}

		// Show/Hide the divider image, when appropriate
		{
			if (UHotkeyWidget* HotkeyWidget = Cast<UHotkeyWidget>(Widget))
			{
				SetHotkeyVisibility(HotkeyWidget, Widget);
			}
			// If Widget is a UWidgetSwitcher, set the active widget visibility 
			else if (UWidgetSwitcher* WidgetSwitcher = Cast<UWidgetSwitcher>(Widget))
			{
				if (UHotkeyWidget* HotkeyWidgetInSwitcher = Cast<UHotkeyWidget>(WidgetSwitcher->GetActiveWidget()))
					SetHotkeyVisibility(HotkeyWidgetInSwitcher, Widget);
			}
		}
	}
}

void UHotkeysWidget::SetHotkeyVisibility(UHotkeyWidget* HotkeyWidget, UWidget* ParentWidget)
{
	if (!HotkeyWidget)
		return;

	//const TArray<UWidget*> UncollapsedHotkeys = GetAllUncollapsedWidgets();
	//const bool bIsLastHotkey = UncollapsedHotkeys.Find(ParentWidget) == UncollapsedHotkeys.Num() - 1; 

	// Set the hotkey divider image visibility
	HotkeyWidget->GetDividerImage()->SetVisibility(/*bIsLastHotkey ? ESlateVisibility::Collapsed : ESlateVisibility::Collapsed*/ESlateVisibility::Collapsed);

	// Add some left padding
	//if (UHorizontalBoxSlot* HBS_Divider = UWidgetLayoutLibrary::SlotAsHorizontalBoxSlot(HotkeyWidget->GetDividerImage()))
	//{
	//	HBS_Divider->SetPadding(bIsLastHotkey ? FMargin(0.0f) : FMargin(SpacingBetweenHotkeys, 2.0f, 0.0f, 2.0f));
	//}
}

TArray<UWidget*> UHotkeysWidget::GetAllUncollapsedWidgets() const
{
	TArray<UWidget*> Children = Hotkeys_Container->GetAllChildren();

	if (Children.Num() == 0)
		return TArray<UWidget*>();

	TArray<UWidget*> UncollapsedChildren;
	UncollapsedChildren.Reserve(Children.Num());
	
	for (UWidget* HotkeyWidget : Children)
	{
		if (HotkeyWidget->GetVisibility() != ESlateVisibility::Collapsed)
		{
			UncollapsedChildren.Add(HotkeyWidget);
		}
	}

	return UncollapsedChildren;
}

void UHotkeysWidget::RefreshHotkeyInput_Implementation()
{
}
