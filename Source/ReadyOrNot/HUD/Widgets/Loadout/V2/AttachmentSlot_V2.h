// Copyright Void Interactive, 2023

#pragma once

#include "CommonUserWidget.h"
#include "AttachmentSlot_V2.generated.h"

UCLASS()
class READYORNOT_API UAttachmentSlot_V2 : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SetEquipped(bool IsEquipped);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetEquipped();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquipped();

	UPROPERTY(EditAnywhere)
	EWeaponAttachmentType AttachmentType;

	UFUNCTION(BlueprintCallable)
	void SetAttachment(UWeaponAttachment* WeaponAttachment);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSubclassOf<UWeaponAttachment> GetAttachment();

	UFUNCTION(BlueprintCallable)
	void SetAttachmentType(EWeaponAttachmentType Type);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	EWeaponAttachmentType GetAttachmentType();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RefreshAttachmentInfo();

	UPROPERTY(BlueprintReadWrite)
	class UTexture2D* ItemImage;

	UPROPERTY(EditAnywhere)
	class UTexture2D* EmptyImage;

	UPROPERTY(BlueprintReadWrite)
	FText ItemName;

	UPROPERTY(BlueprintReadWrite)
	class UTextBlock* ItemType;

private:
	UPROPERTY()
	UWeaponAttachment* Attachment;
	
	UPROPERTY()
	bool Equipped = false;
};
