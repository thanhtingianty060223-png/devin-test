// Copyright Void Interactive, 2022

#include "PlayerHealthStatusWidget.h"

#include "HUD/Widgets/HealthStatusWidget.h"

#include "Components/Image.h"

#include "Components/CharacterHealthComponent.h"
#include "Components/ArmourResourceComponent.h"
#include "Components/InventoryComponent.h"

void UPlayerHealthStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());

	if (PlayerCharacter)
	{
		Helmet->SetVisibility(PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Helmet) != nullptr ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Armor->SetVisibility(PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Armor) != nullptr ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPlayerHealthStatusWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCharacter)
	{
		if (ABaseArmour* HelmetItem = Cast<ABaseArmour>(PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Helmet)))
		{
			Helmet->UpdateIconColor(HelmetItem->GetDurabilityPercentage(), 0.0f, 1.0f);
			Helmet->UpdateHealthPercentage(HelmetItem->GetDurabilityPercentage(), 1.0f);
			Helmet->AutoDetermineIconImage();

			Helmet->SetVisibility(ESlateVisibility::Visible);
		}
		
		if (ABaseArmour* ArmourItem = Cast<ABaseArmour>(PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Armor)))
		{
			Armor->UpdateIconColor(ArmourItem->GetDurabilityPercentage(), 0.0f, 1.0f);
			Armor->UpdateHealthPercentage(ArmourItem->GetDurabilityPercentage(), 1.0f);
			Armor->AutoDetermineIconImage();

			Armor->SetVisibility(ESlateVisibility::Visible);
		}

		Health->UpdateIconColor(PlayerCharacter->GetHealthComponent()->GetCurrentResource(), 0.0f, PlayerCharacter->GetHealthComponent()->GetMaxResource());
		Health->UpdateHealthPercentage(PlayerCharacter->GetHealthComponent()->GetCurrentResource(), PlayerCharacter->GetHealthComponent()->GetMaxResource());
		Health->AutoDetermineIconImage();
	}
}
