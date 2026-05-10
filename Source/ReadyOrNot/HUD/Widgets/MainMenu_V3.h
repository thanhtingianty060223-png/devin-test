// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/IInputProcessor.h"
#ifdef WITH_MODIO
#include "ModioUISubsystem.h"
#include "Core/Input/ModioUIInputProcessor.h"
#endif
#include "MainMenu_V3.generated.h"

UCLASS()
class READYORNOT_API UMainMenu_V3 : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void EnableMainMenuInputPreprocessor();
	UFUNCTION(BlueprintCallable)
	void DisableMainMenuInputPreprocessor();
	// virtual void NativeOnActivated() override;
	// virtual void NativeOnDeactivated() override;
	UFUNCTION(BlueprintCallable)
	bool OpenModMenu(APlayerController* PlayerController);
	UFUNCTION(BlueprintCallable)
	void HideModMenu();
	UFUNCTION(BlueprintCallable)
	void CloseModMenu();
	UFUNCTION(BlueprintImplementableEvent)
	void OnModMenuClosed();
	UFUNCTION(BlueprintImplementableEvent)
	void OnModMenuClosedDuringUpdate();

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	bool IsLoggedIn() const;

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	bool CanFindSession(bool bCOOP) const;

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	FText GetBackEndConnectionStatus(ELoginState LoginState) const;

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	FText GetPublicLobbyCooldown() const;

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	bool CanPlayPublicLobby() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool ShouldShowModsButton();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsModUpdating();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool ShouldShowRestartDialog();	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TMap<ELoginState, FText> BackendConnectionStatus;

	UPROPERTY(BlueprintReadWrite, Category = "Main Menu")
	uint8 bEnableFindSessionCOOPButton : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Main Menu")
	uint8 bEnableFindSessionPVPButton : 1;

private:
	TSharedPtr<IInputProcessor> MainMenuProcessor;
#ifdef WITH_MODIO
	TSharedPtr<FModioUIInputProcessor> Processor;
	FOnModBrowserClosed BrowserClosedDelegate;
	class UModioMenu* ModioMenu;
#endif
};

class MainMenuInputPreProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual int32 GetOwnerUserIndex() const { return 0; };

private:
	class UMainMenu_BaseButton* FocusTarget = nullptr;
};
