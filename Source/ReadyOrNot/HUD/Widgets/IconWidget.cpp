// Void Interactive, 2020

#include "IconWidget.h"

#include "Components/InteractableComponent.h"
#include "Components/Image.h"

void UIconWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    if (!IsValid(ParentComponent) || ParentComponent->IsUnreachable())
    {
        if (GetParent())
            RemoveFromParent();
    }

    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UIconWidget::SetIcon(const FSlateBrush& NewIconBrush)
{
    MainImage->SetBrush(NewIconBrush);
}

FString UIconWidget::GetAttachedObjectName() const
{
    if (ParentComponent)
    {
        IGetFriendlyName* FriendlyNameInterface = Cast<IGetFriendlyName>(ParentComponent->GetAttachmentRootActor());
        if (FriendlyNameInterface)
        {
            return FriendlyNameInterface->Execute_GetFriendlyName(ParentComponent->GetAttachmentRootActor());
        }
    }
    
    return "";
}

UTexture2D* UIconWidget::GetAttachedObjectIcon() const
{
    if (ParentComponent)
    {
        IGetFriendlyName* FriendlyNameInterface = Cast<IGetFriendlyName>(ParentComponent->GetAttachmentRootActor());
        if (FriendlyNameInterface)
        {
            return FriendlyNameInterface->Execute_GetFriendlyIcon(ParentComponent->GetAttachmentRootActor());
        }
    }
    
    return nullptr;
}
