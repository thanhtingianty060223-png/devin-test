// Void Interactive, 2020

#pragma once

#include "MenuWidget.h"
#include "Info/ReadyOrNotBackend.h"
#include "MainMenu.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, BlueprintType)
class READYORNOT_API UMainMenu : public UMenuWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UFUNCTION(BlueprintPure, Category = "Main Menu")
	FText GetVersion() const;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TMap<ELoginState, FText> BackendConnectionStatus;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TMap<ELoginState, FSlateColor> BackendConnectionStatusColor;

	UFUNCTION(BlueprintCallable)
	void OpenModMenu();

	UFUNCTION(BlueprintCallable)
	void CloseModMenu();
	
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	class UCanvasPanel* BackgroundCanvas = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	UUserWidget* PlayBtn = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	UUserWidget* FindCOOPSessionBtn = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	UUserWidget* FindPVPSessionBtn = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	UUserWidget* PlayPublicLobbyBtn = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	UUserWidget* PlayFriendsOnlyBtn = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	class UTextBlock* Txt_BackEndConnection = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu | Required Widgets", meta = (BindWidget))
	class UTextBlock* Txt_PublicLobbyCooldown = nullptr;
	
	UPROPERTY(BlueprintReadWrite, Category = "Main Menu")
	uint8 bEnableFindSessionCOOPButton : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Main Menu")
	uint8 bEnableFindSessionPVPButton : 1;
};
