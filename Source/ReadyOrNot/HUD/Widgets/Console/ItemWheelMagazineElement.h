// Copyright Void Interactive, 2023

#pragma once
#include "ItemWheelElement.h"
#include "ItemWheelMagazineElement.generated.h"

UCLASS()
class READYORNOT_API UItemWheelMagazineElement : public UItemWheelElement
{
	GENERATED_BODY()
	UItemWheelMagazineElement(const FObjectInitializer& ObjectInitializer);
private: 
    APlayerCharacter* PlayerCharacter = nullptr;
	UDataTable* DataTable = nullptr;

public:
	bool IsSelectable() override;
	void SetAmmoType(const FName &Label);
	FName GetAmmoType();
	FString GetDisplayName();

protected:

	virtual void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UTextBlock* MagazineType;

private:
	FName AmmoFName = "";
	FString DisplayName;
};
