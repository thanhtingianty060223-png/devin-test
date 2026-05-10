// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "MunitionsSlot.generated.h"

UCLASS()
class READYORNOT_API UMunitionsSlot : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void UpdateSlotCount();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateSlotText(int CurrentSlots, int MaxSlots);
	UFUNCTION(BlueprintCallable)
	void UpdateElementContainer();
	UFUNCTION()
	void SetPrimaryAmmoSlot();
	UFUNCTION()
	void SetSecondaryAmmoSlot();
	UFUNCTION()
	void SetGrenadeSlot();
	UFUNCTION()
	void SetTacticalSlot();

	UFUNCTION(BlueprintImplementableEvent)
	void CreateElement(UTexture2D* Icon, int Amount, bool bIsAmmoElement = false, const FText& AmmoType = FText::GetEmpty());

	UFUNCTION(BlueprintImplementableEvent)
	void CreateSeparator(bool bShouldCreateSeparator);

	UPROPERTY(BlueprintReadOnly)
	int SlotCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UHorizontalBox* ElementContainer;

	UPROPERTY(BlueprintReadOnly)
	class AReadyOrNotGameState* gs;

	UPROPERTY(BlueprintReadOnly)
	bool bHasElementsPrevious = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHasElementsNext = false;

public:
	UPROPERTY()
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;

	UPROPERTY(EditAnywhere)
	UTexture2D* PrimaryAmmoIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* SecondaryAmmoIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* GrenadeIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* C2ChargeIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* WedgeIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* PeppersprayIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* LockpickGunIcon;

	UPROPERTY(EditAnywhere)
	UTexture2D* TaserIcon;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<ABaseItem>> SlotItems;
};
