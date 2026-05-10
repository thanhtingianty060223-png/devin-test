// Copyright Void Interactive, 2022 

#include "HUD/Widgets/Console/ConsoleMagSelection.h"

#include "ConsoleMagSelectionItem.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

PRAGMA_DISABLE_OPTIMIZATION

bool UConsoleMagSelection::Initialize()
{
	const bool Initialized = Super::Initialize();

	PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(
		GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return false;
	}
	// PlayerCharacter->OnWeaponFired.AddDynamic(this, &UConsoleMagSelection::OnWeaponFired);
	return Initialized;
}

void UConsoleMagSelection::OnMagCheck(ABaseMagazineWeapon* MagazineWeapon)
{
	UpdateMagazines(MagazineWeapon);
}

void UConsoleMagSelection::OnWeaponFired(ABaseWeapon* Weapon)
{
	ABaseMagazineWeapon* MagazineWeapon = Cast<ABaseMagazineWeapon>(Weapon);
	if (MagazineWeapon != nullptr)
	{
		UpdateMagazines(MagazineWeapon);
	}
}

void UConsoleMagSelection::UpdateMagazines(ABaseMagazineWeapon* MagazineWeapon)
{
	Container->ClearChildren();
	int i = 0;
	for (FMagazine& Magazine : MagazineWeapon->Magazines)
	{
		const FWidgetLookupData WidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData(
			"ConsoleMagSelectionItem");
		UConsoleMagSelectionItem* NewConsoleWidget = CreateWidget<UConsoleMagSelectionItem>(
			GetWorld(), WidgetData.WidgetClass);
		NewConsoleWidget->MagazineText->SetText(MagazineWeapon->GetMagazineScreenName(Magazine));
		NewConsoleWidget->UpdateMagPercentage(MagazineWeapon->GetMagazineAmmoPercentage(i));
		NewConsoleWidget->SetMagIndex(i);
		NewConsoleWidget->SetSelected(i == MagazineWeapon->MagIndex);
		Container->AddChild(NewConsoleWidget);
		i++;
	}
}

PRAGMA_ENABLE_OPTIMIZATION
