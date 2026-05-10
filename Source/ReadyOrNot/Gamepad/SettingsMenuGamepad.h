// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "HUD/Widgets/MenuWidget.h"
#include "InputRemappingNodes.h"
#include "IllegalUnbindModal.h"
#include "CommonButtonBase.h"
#include "SettingsMenuGamepad.generated.h"

class UControlsBind;

UENUM()
enum class EInputKeyCategoryV2 : uint8
{
	KE_Shared	 UMETA(DisplayName = "Shared"),
	KE_Character UMETA(DisplayName = "Character"),
	KE_Drone	 UMETA(DisplayName = "Drone")
};

USTRUCT(BlueprintType)
struct FKeyBinding
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Enabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Axis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BindingName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText FriendlyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AxisScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInputKeyCategoryV2 InputKeyCategory = EInputKeyCategoryV2::KE_Character;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CannotBeUnbindable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsReadOnly = false;
};

FORCEINLINE uint32 GetTypeHash(const FActionMappingStruct& ActionMapping)
{
	uint32 Hash = FCrc::MemCrc32(&ActionMapping, sizeof(FActionMappingStruct));
	return Hash;
}

FORCEINLINE uint32 GetTypeHash(const FAxisMappingStruct& AxisMapping)
{
	uint32 Hash = FCrc::MemCrc32(&AxisMapping, sizeof(FAxisMappingStruct));
	return Hash;
}

FORCEINLINE uint32 GetTypeHash(const FKeyBinding& KeyBinding)
{
	uint32 Hash = FCrc::MemCrc32(&KeyBinding, sizeof(FKeyBinding));
	return Hash;
}

UCLASS()
class READYORNOT_API USettingsMenuGamepad : public UMenuWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SettingsMenuGamepad")
	TArray<FKeyBinding> CharacterControls;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SettingsMenuGamepad")
	TArray<FKeyBinding> DroneControls;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SettingsMenuGamepad")
	TArray<FKeyBinding> SharedControls;

	UPROPERTY(BlueprintReadWrite, Category = "SettingsMenuGamepad")
	TMap<UControlsBind*, FAxisMappingStruct> AxesControlBinds;

	UPROPERTY(BlueprintReadWrite, Category = "SettingsMenuGamepad")
	TMap<UControlsBind*, FActionMappingStruct> ActionsControlBinds;

	UPROPERTY(BlueprintReadWrite, Category = "SettingsMenuGamepad")
	TArray<FKeyBinding> UnbindableControls;

	UPROPERTY(BlueprintReadWrite, Category = "SettingsMenuGamepad")
	class UIllegalUnbindModal* IllegalKeyConflictDetectedPopUp;

	UPROPERTY(BlueprintReadWrite, Category = "SettingsMenuGamepad")
	class UCommonActivatableWidgetStack* WidgetStack;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SettingsMenuGamepad")
	void EndButtonsDisabledForBinding();

	UFUNCTION(BlueprintCallable, Category = "SettingsMenuGamepad")
	void SelectNewTab(TArray<UCommonButtonBase*> TabButtonsArray, int WidgetIndex, bool GoingToPreviousWidget);
};
