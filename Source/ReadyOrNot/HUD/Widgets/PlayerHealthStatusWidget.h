// Copyright Void Interactive, 2022

#pragma once

#include "Blueprint/UserWidget.h"
#include "PlayerHealthStatusWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UPlayerHealthStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Player Health Status Widget|Required Widgets", meta = (BindWidget))
	class UHealthStatusWidget* Health = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Health Status Widget|Required Widgets", meta = (BindWidget))
	class UHealthStatusWidget* Armor = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Health Status Widget|Required Widgets", meta = (BindWidget))
	class UHealthStatusWidget* Helmet = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Player Health Status Widget|Data")
	class APlayerCharacter* PlayerCharacter = nullptr;
};
