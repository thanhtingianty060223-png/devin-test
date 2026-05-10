// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "Loadout_InformationBox.generated.h"

USTRUCT(BlueprintType)
struct FAttachmentEffects
{
	GENERATED_BODY()

	FAttachmentEffects() {}
	FAttachmentEffects(FText Text, bool EffectBool)
	{
		EffectText = Text;
		bEffectIsPos = EffectBool;
	}

	UPROPERTY(BlueprintReadOnly)
	FText EffectText;

	UPROPERTY(BlueprintReadOnly)
	bool bEffectIsPos = false;
};

USTRUCT()
struct FEffectsMultiplierStruct
{
	GENERATED_BODY()

	FEffectsMultiplierStruct() {}
	FEffectsMultiplierStruct(bool ReverseBool, float MultiplierFloat)
	{
		bShouldReverse = ReverseBool;
		Multiplier = MultiplierFloat;
	}

	UPROPERTY()
	bool bShouldReverse = false;

	UPROPERTY()
	float Multiplier = 0.0f;
};

UCLASS()
class READYORNOT_API ULoadout_InformationBox : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* txt_Category = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* txt_Class = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* txt_ItemName = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* txt_Description = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* VB_Attachments = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* VB_Effects = nullptr;

	UFUNCTION(BlueprintCallable)
	void UpdateInfoBox(ABaseItem* Item, FText CurrentItemCategory, bool IsItemWeapon);

	UFUNCTION(BlueprintCallable)
	void UpdateMaterialInfo(UArmourMaterial* ArmorMaterial, FText CurrentItemCategory);

	UFUNCTION(BlueprintCallable)
	void UpdateEffectsInfo(UWeaponAttachment* Attachment, FText CurrentItemCategory);

	UFUNCTION()
	void SetCategory(FText CurrentItemCategory);

	UFUNCTION(BlueprintCallable)
	void SetAttachments(TSubclassOf<ABaseWeapon> BaseWeapon, bool IsSecondary);

	UFUNCTION()
	TArray<FAttachmentEffects> SetEffects(UWeaponAttachment* Attachment);

	UFUNCTION(BlueprintImplementableEvent)
	void CreateAttachmentElement(const TArray<UWeaponAttachment*>& Attachments, const TArray<EWeaponAttachmentType>& AttachmentTypes);

	UFUNCTION(BlueprintImplementableEvent)
	void CreateEffectsElement(const TArray<FAttachmentEffects>& AttachmentEffects);

private:
	UPROPERTY()
	class AReadyOrNotGameState* gs;

	UPROPERTY()
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;
};
