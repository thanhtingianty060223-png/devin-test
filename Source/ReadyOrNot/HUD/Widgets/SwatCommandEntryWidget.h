// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "SwatCommandEntryWidget.generated.h"

UCLASS()
class READYORNOT_API USwatCommandEntryWidget final : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateCommandEntry(const FSwatCommand& InSwatCommand, ETeamType Team);
	
	void SetBorderColor();
	void SetExtendedImageColor();
	
	void SetText(const FText& NewText);
	void SetTextColor();

	void PlayFlashAnimation();
	
	void UpdateExtendedImageVisibility();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FSwatCommand SwatCommand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	ETeamType ActiveTeamType = ETeamType::TT_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FLinearColor RedTeamColor = FColor::FromHex("FF5938E6");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FLinearColor BlueTeamColor = FColor::FromHex("8CC4FFE6");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FLinearColor GoldTeamColor = FColor::FromHex("FFF455E6");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bLast = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bBack = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bExtended = false;
	
protected:
	virtual void NativePreConstruct() override;

	FLinearColor GetTeamColor() const;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* CommandText = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* KeybindText = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UBorder* EntryBorder = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* ExtendedImage = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* BackIcon = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* Flash = nullptr;
};
