// Copyright Void Interactive, 2022


#include "HUD/Widgets/Console/ConsoleFireModes.h"

PRAGMA_DISABLE_OPTIMIZATION

bool UConsoleFireModes::Initialize()
{
	const bool Initialized = Super::Initialize();
	APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(UGameplayStatics::GetPlayerCharacter(
		GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return Initialized;
	}
	OnEquipped(PlayerCharacter->GetEquippedItem());
	PlayerCharacter->GetInventoryComponent()->OnItemEquipped.AddDynamic(this, &UConsoleFireModes::OnEquipped);
	PlayerCharacter->OnWeaponFireModeChanged.AddDynamic(this, &UConsoleFireModes::OnFireModeChanged);
	return Initialized;
}

void UConsoleFireModes::OnEquipped(ABaseItem* Item)
{
	ABaseWeapon* Weapon = Cast<ABaseWeapon>(Item);
	if (Weapon != nullptr)
	{
		UpdateAvailableFireModes(Weapon);
		APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (!PlayerCharacter)
			return;
		OnFireModeChanged(PlayerCharacter, Weapon->CurrentFireMode, Weapon->CurrentFireMode);
		this->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		this->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UConsoleFireModes::OnFireModeChanged(APlayerCharacter* PlayerCharacter, EFireMode NewFireMode, EFireMode /*LastFireMode*/)
{
	UpdateSelectedFireMode(FireModeSafe, EFireMode::FM_Safe, NewFireMode);
	UpdateSelectedFireMode(FireModeSingle, EFireMode::FM_Single, NewFireMode);
	UpdateSelectedFireMode(FireModeBurst, EFireMode::FM_Burst, NewFireMode);
	UpdateSelectedFireMode(FireModeAuto, EFireMode::FM_Auto, NewFireMode);
}

void UConsoleFireModes::UpdateSelectedFireMode(UUserWidget* FireMode, EFireMode WidgetFireMode, EFireMode NewFireMode)
{
	if (WidgetFireMode == NewFireMode)
	{
		FireMode->SetColorAndOpacity(FLinearColor::Red);
	}
	else
	{
		FireMode->SetColorAndOpacity(FLinearColor::White);
	} 
}

void UConsoleFireModes::UpdateAvailableFireModes(ABaseWeapon* Weapon)
{
	if (Weapon->AvailableFireModes.Contains(EFireMode::FM_Single))
	{
		
		FireModeSingle->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		FireModeSingle->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Weapon->AvailableFireModes.Contains(EFireMode::FM_Burst))
	{
		FireModeBurst->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		FireModeBurst->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Weapon->AvailableFireModes.Contains(EFireMode::FM_Auto) || Weapon->AvailableFireModes.Contains(
		EFireMode::FM_Continuous))
	{
		FireModeAuto->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		FireModeAuto->SetVisibility(ESlateVisibility::Collapsed);
	}
}


PRAGMA_ENABLE_OPTIMIZATION
