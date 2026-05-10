// Copyright Void Interactive, 2022

#pragma once

#include "ConsoleMagSelectionItem.generated.h"

UCLASS()
class READYORNOT_API UConsoleMagSelectionItem : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UMaterial* MagazineMaterial;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta=(BindWidget))
	UImage* MagazineIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta=(BindWidget))
	UImage* SelectedIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta=(BindWidget))
	class UTextBlock* MagazineText;

	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* MagazineMaterialDynamic;

	UFUNCTION()
	void UpdateMagPercentage(float Percentage);

	UFUNCTION()
	void SetSelected(bool bCond);

	UFUNCTION()
	void SetMagIndex(int Index);
	UFUNCTION()
	int GetMagIndex();

protected:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
	int MagIndex;
};
