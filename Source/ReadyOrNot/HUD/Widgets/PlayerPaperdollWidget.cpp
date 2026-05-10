// Copyright Void Interactive, 2022

#include "HUD/Widgets/PlayerPaperdollWidget.h"

#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/WidgetSwitcher.h"

#include "Actors/SWATArmour.h"
#include "Actors/Items/Headwear.h"

//void UPlayerPaperdollWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
//{
//	Super::NativeTick(MyGeometry, InDeltaTime);
//	//InvalidationBox_0->InvalidateLayoutAndVolatility();
//	
//	//return;
//	
//}

void UPlayerPaperdollWidget::UpdateGearImage(UImage* Image, UTexture2D* InTexture)
{
	if (!Image->IsVisible() || Image->Brush.GetResourceObject() != InTexture ||
		(Image->IsVisible() && InTexture == nullptr))
	{
		Image->SetBrushFromTexture(InTexture);
		Image->SetVisibility(InTexture ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPlayerPaperdollWidget::ToggleHeadwearVisibility(const bool bVisible)
{
	Headwear_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	Headwear_Crouch_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UPlayerPaperdollWidget::ToggleBodyArmorVisibility(const bool bVisible)
{
	BodyArmor_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	BodyArmor_Crouch_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	
	Carry_BodyArmor_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	Carry_BodyArmor_Crouch_Image->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UPlayerPaperdollWidget::UpdateHealth(ABaseItem* Item)
{
	if (const APlayerCharacter* OwningPlayer = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
	{
		StanceSwitcher->SetActiveWidget(OwningPlayer->IsCrouching() ? Crouch_Overlay : Stand_Overlay);
		Stand_CarrySwitcher->SetActiveWidget(OwningPlayer->IsCarrying() ? StandCarry_Overlay : StandNoCarry_Overlay);
		Crouch_CarrySwitcher->SetActiveWidget(OwningPlayer->IsCarrying() ? CrouchCarry_Overlay : CrouchNoCarry_Overlay);

		// Display current body armour, if available
		const ASWATArmour* Armor = Cast<ASWATArmour>(OwningPlayer->GetInventoryComponent()->GetArmour());
		bool bHasRemainingProtection = Armor ? Armor->HasRemainingProtection() : false;
		if (bHasRemainingProtection)
		{
			UpdateGearImage(BodyArmor_Image, Armor->PaperdollTexture);
			UpdateGearImage(BodyArmor_Crouch_Image, Armor->PaperdollTexture_Crouch);
			UpdateGearImage(Carry_BodyArmor_Image, Armor->PaperdollTexture_Carry);
			UpdateGearImage(Carry_BodyArmor_Crouch_Image, Armor->PaperdollTexture_Carry_Crouch);
		}
		else
		{
			UpdateGearImage(BodyArmor_Image, nullptr);
			UpdateGearImage(BodyArmor_Crouch_Image, nullptr);
			UpdateGearImage(Carry_BodyArmor_Image, nullptr);
			UpdateGearImage(Carry_BodyArmor_Crouch_Image, nullptr);
		}

		if (OwningPlayer->bOverrideHeadwearPaperdollTexture)
		{
			UpdateGearImage(Headwear_Image, OwningPlayer->HeadwearPaperdollTexture_Override);
			UpdateGearImage(Headwear_Crouch_Image, OwningPlayer->HeadwearPaperdollTexture_Crouch_Override);
		}
		else
		{
			// Display current headwear, if available
			const AHeadwear* Headwear = OwningPlayer->GetInventoryComponent()->GetHeadwear();
			bHasRemainingProtection = Headwear ? Headwear->HasRemainingProtection() : false;

			if (bHasRemainingProtection)
			{
				UpdateGearImage(Headwear_Image, Headwear->PaperdollTexture);
				UpdateGearImage(Headwear_Crouch_Image, Headwear->PaperdollTexture_Crouch);
			}
			else
			{
				UpdateGearImage(Headwear_Image, nullptr);
				UpdateGearImage(Headwear_Crouch_Image, nullptr);
			}
		}
	}
	
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}