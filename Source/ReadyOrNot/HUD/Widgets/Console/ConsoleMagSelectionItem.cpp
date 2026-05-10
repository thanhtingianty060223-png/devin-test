// Copyright Void Interactive, 2022

#include "HUD/Widgets/Console/ConsoleMagSelectionItem.h"

PRAGMA_DISABLE_OPTIMIZATION

bool UConsoleMagSelectionItem::Initialize()
{
	const bool Initialized = Super::Initialize();
	if (MagazineMaterial != nullptr)
	{
		MagazineMaterialDynamic = UMaterialInstanceDynamic::Create(MagazineMaterial, this);
		MagazineIcon->Brush.SetResourceObject(MagazineMaterialDynamic);
	}
	return Initialized;
}

void UConsoleMagSelectionItem::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	const APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	const ABaseMagazineWeapon* MagazineWeapon = Cast<ABaseMagazineWeapon>(PlayerCharacter->GetEquippedItem());
	if (MagazineWeapon)
	{
		const float Percentage = MagazineWeapon->GetMagazineAmmoPercentage(MagIndex);
		UpdateMagPercentage(Percentage);
	}
}

void UConsoleMagSelectionItem::UpdateMagPercentage(float Percentage)
{
	MagazineMaterialDynamic->SetScalarParameterValue("Percentage", Percentage);
}

void UConsoleMagSelectionItem::SetSelected(bool bCond)
{
	if (bCond)
	{
		SetRenderOpacity(1.0f);
		SelectedIcon->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetRenderOpacity(0.5f);
		SelectedIcon->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UConsoleMagSelectionItem::SetMagIndex(int Index)
{
	MagIndex = Index;
}

int UConsoleMagSelectionItem::GetMagIndex()
{
	return MagIndex;
}

PRAGMA_ENABLE_OPTIMIZATION
