// Copyright Void Interactive, 2023

#include "AnimatedIconWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/InteractableComponent.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"

#include "Kismet/KismetMaterialLibrary.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Animated Icon Widget ~ Reset Anim"), STAT_ResetAnim, STATGROUP_AnimatedIconWidget);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Animated Icon Widget ~ Set Icon Size"), STAT_SetIconSize, STATGROUP_AnimatedIconWidget);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Animated Icon Widget ~ Set Active Icon"), STAT_SetActiveIcon, STATGROUP_AnimatedIconWidget);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Animated Icon Widget ~ Set Current Progress"), STAT_SetCurrentProgress, STATGROUP_AnimatedIconWidget);

bool UAnimatedIconWidget::Initialize()
{
    const bool bResult = Super::Initialize();
    
    if (MID_ProgressCircle)
    {
        MID_ProgressCircle->ConditionalBeginDestroy();
        MID_ProgressCircle = nullptr;
    }
    
    MID_ProgressCircle = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, ProgressCircleMaterial);
    
    FrameImages[0] = Frame_1;
    FrameImages[1] = Frame_2;
    FrameImages[2] = Frame_3;
    FrameImages[3] = Frame_4;
    FrameImages[4] = Frame_5;
    FrameImages[5] = Frame_6;
    FrameImages[6] = Frame_7;
    FrameImages[7] = Frame_8;
    
    return bResult;
}

void UAnimatedIconWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    MID_ProgressCircle->SetScalarParameterValue(ProgressParamName, 0.0f);
    ProgressCircle_Image->SetBrushFromMaterial(MID_ProgressCircle);
}

void UAnimatedIconWidget::OnAnimationStarted_Implementation(const UWidgetAnimation* Animation)
{
    if (ParentComponent)
    {
        ParentComponent->bCanHide = false;
    }
}

void UAnimatedIconWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
    if (Animation == Anim_Interact)
    {
        if (ParentComponent)
        {
            ParentComponent->bCanHide = true;
        }
    }
}

void UAnimatedIconWidget::ResetAnim()
{
    SCOPE_CYCLE_COUNTER(STAT_ResetAnim);
    
    CurrentIndex = 0;
    ElapsedTime = 0.0f;
    bPaused = false;

    //UReadyOrNotFunctionLibrary::RemoveFromParentAndClear(IconImages);
    IconImages.Reset();
    
    for (uint8 i = 0; i < 8; i++)
    {
        FrameImages[i]->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (ParentComponent)
    {
        const uint8 Num = FMath::Min(8, ParentComponent->AnimatedIcon.Icons.Num());
        for (uint8 i = 0; i < Num; i++)
        {
            UImage* Frame = FrameImages[i];

            FSlateBrush IconBrush;
            IconBrush.SetResourceObject(ParentComponent->AnimatedIcon.Icons[i]);
            
            Frame->SetBrush(IconBrush);
            Frame->SetVisibility(ESlateVisibility::HitTestInvisible);

            IconImages.Add(Frame);
        }
        
        //IconSwitcher->ClearChildren();

        /*
        for (UTexture2D* Icon : ParentComponent->AnimatedIcon.Icons)
        {
            if (UImage* IconImage = NewObject<UImage>(this))
            {
                FSlateBrush IconBrush;
                IconBrush.SetResourceObject(Icon);
                
                IconImage->SetBrush(IconBrush);
                IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

                IconSwitcher->AddChild(IconImage);
            
                //IconImages.Add(IconImage);
            }
        }
        */
    }
}

void UAnimatedIconWidget::SetInteractIconSize(const float InInteractCircleSize, const float InInteractIconSize)
{
    SCOPE_CYCLE_COUNTER(STAT_SetIconSize);
    
    if (UCanvasPanelSlot* CanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InteractCircle_Overlay))
        CanvasPanelSlot->SetSize({InInteractCircleSize, InInteractCircleSize});

    if (UCanvasPanelSlot* CanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InteractIcon_SizeBox))
        CanvasPanelSlot->SetSize({InInteractIconSize, InInteractIconSize});
    
    //InteractIcon_SizeBox->SetWidthOverride(InInteractIconSize);
    //InteractIcon_SizeBox->SetHeightOverride(InInteractIconSize);
}

void UAnimatedIconWidget::PlayInteractAnim()
{
    PlayAnimation(Anim_Interact, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f, true);
}

void UAnimatedIconWidget::StopInteractAnim()
{
    StopAnimation(Anim_Interact);
}

void UAnimatedIconWidget::PlayFocusAnim(const bool bReverse)
{
    PlayAnimation(Anim_Focus, 0.0f, 1, bReverse ? EUMGSequencePlayMode::Reverse : EUMGSequencePlayMode::Forward);
}

void UAnimatedIconWidget::StopFocusAnim()
{
    StopAnimation(Anim_Focus);
}

void UAnimatedIconWidget::PauseIconAnim()
{
    bPaused = true;
}

void UAnimatedIconWidget::UnPauseIconAnim()
{
    bPaused = false;
}

void UAnimatedIconWidget::SetActiveIcon(const int32 Index)
{
    SCOPE_CYCLE_COUNTER(STAT_SetActiveIcon);
    
    if (IconSwitcher && IconSwitcher->HasAnyChildren())
        IconSwitcher->SetActiveWidgetIndex(Index);
}

void UAnimatedIconWidget::SetCurrentProgress(const float Percent)
{
    SCOPE_CYCLE_COUNTER(STAT_SetCurrentProgress);
    
    if (MID_ProgressCircle)
        MID_ProgressCircle->SetScalarParameterValue(ProgressParamName, FMath::Clamp(Percent, 0.0f, 1.0f));
}
