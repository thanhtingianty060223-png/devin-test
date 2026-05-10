// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Widgets/WeaponWheelWidget.h"
#include "WidgetsData.generated.h"

UENUM(BlueprintType)
enum class EInterfaceSoundType : uint8
{
	IST_None,
	IST_Checkmark,
	IST_Button,
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UWidgetsData : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// TODO: Move All Widgets into here so they can be easily swapped out...

	UPROPERTY(EditAnywhere, Category = UI)
	TSubclassOf<UUserWidget> MagCheckUI;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UUserWidget> FireModeUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> StartupMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> AuthenticationMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> MainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> OptionsMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> CustomizationMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> ChatBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<TSoftClassPtr<UUserWidget>> Overlays;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> Scoreboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> LoadingScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	TSoftClassPtr<class UMessageDisplayBox> MessageDisplayBoxClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> Leaderboards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> CrossHairOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<USoundBase*> UISoundClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UUserWidget> EscapeMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UWeaponWheelWidget> WeaponWheelWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UBaseWidget> ItemSelectionWidget;
};
