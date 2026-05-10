// Copyright Void Interactive, 2023

#pragma once

#include "ItemWheelElement.generated.h"

UCLASS()

class READYORNOT_API UItemWheelElement : public UUserWidget
{
    GENERATED_BODY()

    APlayerCharacter* PlayerCharacter = nullptr;

public:
    void Update();
    UFUNCTION(BlueprintCallable)
    virtual bool IsSelectable();
    void SetBrush(FSlateBrush Brush);

protected:
    void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void Selected(bool Selected);
    UFUNCTION(BlueprintCallable)
    bool ShowCounter();

    UPROPERTY(EditAnywhere)
    EItemCategory ElementCategory = EItemCategory::IC_None;
    
    UPROPERTY(meta=(BindWidget))
    UImage* ElementImage;

    UPROPERTY(EditAnywhere)
    FSlateBrush ElementBrush;

    UPROPERTY(meta=(BindWidget))
    class UTextBlock* CounterText;

private:
    FText CounterFormat = FText::FromString("x{0}");
};
