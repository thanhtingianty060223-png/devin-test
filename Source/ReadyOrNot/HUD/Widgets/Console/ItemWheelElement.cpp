// Copyright Void Interactive, 2023


#include "HUD/Widgets/Console/ItemWheelElement.h"

#include <Components/TextBlock.h>

void UItemWheelElement::NativeConstruct()
{
    Super::NativeConstruct();
    PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
    SetBrush(ElementBrush);
    Update();
}

void UItemWheelElement::Update()
{
    if (ShowCounter() && ElementCategory != EItemCategory::IC_None && PlayerCharacter)
    {
        CounterText->SetVisibility(ESlateVisibility::Visible);
        const int num = PlayerCharacter->GetInventoryComponent()->GetInventoryItemsOfType(ElementCategory).Num();
        CounterText->SetText(FText::Format(CounterFormat, num));
    }
    else
    {
        CounterText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UItemWheelElement::SetBrush(FSlateBrush Brush)
{
    if (ElementImage != nullptr)
    {
        ElementImage->SetBrush(Brush);
    }
}

void UItemWheelElement::Selected(bool Selected)
{
    if (Selected)
    {
        SetRenderOpacity(1.0f);
    }
    else
    {
        SetRenderOpacity(0.5f);
    }
}

bool UItemWheelElement::IsSelectable()
{
    bool IsSelectable = false;
    if (PlayerCharacter->GetInventoryComponent()->HasAnyInventoryItemsOfType(ElementCategory))
    {
        IsSelectable = true;
    }
    return IsSelectable;
}

bool UItemWheelElement::ShowCounter()
{
    switch (ElementCategory)
    {
    case EItemCategory::IC_LockpickGun:
        return false;
    case EItemCategory::IC_Multitool:
        return false;
    case EItemCategory::IC_Detonator:
        return false;
    case EItemCategory::IC_Zipcuffs:
        return false;
    case EItemCategory::IC_OCSpray:
        return false;
    default:
        return true;
    }
}
