// Void Interactive, 2020

#pragma once

#include "ActivatableBaseWidget.h"
#include "Actors/Triggers/BuildingTrigger.h"
#include "HumanCharacterHUD_V2.generated.h"

class UScreenspaceMarkerWidget;
class UTutorialWidget;

UENUM(BlueprintType)
enum class EHUDStyle : uint8
{
	Default,
	Minimal,
	Detail,
	Immersive
};

/**
 * The main HUD for a player controlled character
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API UHumanCharacterHUD_V2 : public UActivatableBaseWidget
{
	GENERATED_BODY()

public:
	UHumanCharacterHUD_V2();

	/*DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAsdf, bool, asdf);
	UPROPERTY()
	FOnAsdf OnAsdf;*/

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSlot1Updated,FText, InText, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlot1Updated OnSlot1Updated;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSlot2Updated, FText, InText, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlot2Updated OnSlot2Updated;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSlot3Updated, FText, InText, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlot3Updated OnSlot3Updated;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSlot4Updated, FText, InText, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlot4Updated OnSlot4Updated;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSlotReserved1Updated, FText, InText, FText, InTextGamepad, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlotReserved1Updated OnSlotReserved1Updated;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSlotReserved2Updated, FText, InText, FText, InTextGamepad, bool, clearText, bool, bAnimate, bool, bLoopAnimation);
	UPROPERTY(BlueprintAssignable)
	FOnSlotReserved2Updated OnSlotReserved2Updated;


	UFUNCTION(BlueprintImplementableEvent)
	void OnInventoryItemsChanged();

	UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowHUD();
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideHUD();

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void ChatPressed();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void TeamChatPressed();

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void AddScorePopup(const FText& ScoreText, int32 ScoreValue, bool bGive = true);

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	void AddObjectivePopup(const FText& PopupText);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void ShowTutorialPrompt(const FString& MessageID, bool bFirstShowing, const FText& MessageTitle, const TArray<FText>& MessageContent);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void HideTutorialPrompt(const FString& MessageID);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void ShowTutorialOverview(const FString& MessageID, const FText& MessageTitle, const TArray<FText>& MessageContent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void HideTutorialOverview(const FString& MessageID);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map", meta = (AutoCreateRefTerm = "IconBrush"))
	class UMapActorWidget* AddMapActor(class UMapActorComponent* MapActorComponent, TSubclassOf<class UMapActorWidget> MapActorIconWidgetClass, const FSlateBrush& IconBrush, const FLinearColor& IconColor = FLinearColor::White, const FLinearColor& IconTextColor = FLinearColor::White, float RotationOffset = 0.0f);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
	void RemoveMapActor(class UMapActorComponent* MapActorComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
	void UpdateMapFloors(const TArray<FBuildingFloor>& Floors);

	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void AssignPlayerActionPromptToFreeSlot(FKey InputKey, EInputEvent InputEvent, FText InActionText, FString InColorLabel, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void AssignPlayerActionPrompt(int32 InSlot, FKey InputKey, EInputEvent InputEvent, FText InActionText, FString InColorLabel, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	int32 AssignPlayerActionPromptToFreeSlot_Reserved(FKey InputKey, EInputEvent InputEvent, FText InActionText, FString InColorLabel, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void AssignPlayerActionPrompt_Reserved(int32 InSlot, EInputEvent InputEvent, FText InActionText, FString InColorLabel, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void AssignPlayerActionPrompt_Custom(int32 InSlot, FText InCustomPromptText, bool bAnimate = true, bool bLoopAnimation = false);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void RemovePlayerActionPrompt(int32 InSlot);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void RemovePlayerActionPrompt_Reserved(int32 InSlot);
	
	UFUNCTION(BlueprintCallable, Category = "Action Prompts")
	void ClearAllPlayerActionPrompts();
	
	UFUNCTION(BlueprintPure, Category = "Action Prompts")
	bool IsActionSlotInUse(int32 InSlot) const;
	
	UFUNCTION(BlueprintPure, Category = "Action Prompts")
	bool IsActionSlotInUse_Reserved(int32 InSlot) const;
	
	FORCEINLINE TArray<FName> GetWidgetFadeZoneNames() const { return ObjectiveMarker_WidgetFadeZones; }
	
	FORCEINLINE class UCanvasPanel* GetMainCanvas() const { return CanvasPanel_Main; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnTabletNotificationEvent();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnTabletOpen();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnTabletClose();

	UFUNCTION(BlueprintCallable)
	UItemWheel* GetItemWheel();

	UFUNCTION(BlueprintCallable)
	UCommandWheel* GetCommandWheel();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	UTutorialWidget* GetTutorialWidget();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	UScreenspaceMarkerWidget* GetScreenspaceMarkerWidget();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	UScreenspaceMarkerWidget* GetRoundupWidget();

	UFUNCTION(BlueprintNativeEvent)
	void LocalReadyStateChanged(bool bReady);

	UFUNCTION(BlueprintNativeEvent)
	void ReadiedPlayersChanged();

	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim))
	UWidgetAnimation* Anim_FadeInHUD;
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
			void ReflectHUDSettings();
	virtual void ReflectHUDSettings_Implementation();

	virtual void ChatPressed_Implementation();
	virtual void TeamChatPressed_Implementation();
	virtual void AddScorePopup_Implementation(const FText& ScoreText, int32 ScoreValue, bool bGive);
	class UMapActorWidget* AddMapActor_Implementation(class UMapActorComponent* MapActorComponent, TSubclassOf<class UMapActorWidget> MapActorIconWidgetClass, const FSlateBrush& IconBrush, const FLinearColor& IconColor = FColor::White, const FLinearColor& IconTextColor = FLinearColor::White, float RotationOffset = 0.0f);
	virtual void RemoveMapActor_Implementation(class UMapActorComponent* MapActorComponent);
	virtual	void UpdateMapFloors_Implementation(const TArray<FBuildingFloor>& Floors);

	#pragma region Required Widgets
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UCanvasPanel* CanvasPanel_Main = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UCanvasPanel* CanvasPanel_Gamepad = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class URetainerBox* RetainerBox_SwayableElements = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UPlayerActionPromptWidget* PlayerActionText_Slot_1 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
    class UPlayerActionPromptWidget* PlayerActionText_Slot_2 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
    class UPlayerActionPromptWidget* PlayerActionText_Slot_3 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
    class UPlayerActionPromptWidget* PlayerActionText_Slot_4 = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
    class UPlayerActionPromptWidget* PlayerActionText_Slot_Reserved_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UPlayerActionPromptWidget* PlayerActionText_Slot_Reserved_2 = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UItemWheel* ItemWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UCommandWheel* CommandWheel = nullptr;
	#pragma endregion

	#pragma region Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EHUDStyle HUDStyle = EHUDStyle::Default;

	// Fade widgets when overlapping with the widgets listed in this array
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> ObjectiveMarker_WidgetFadeZones;
	
	#pragma region HUD Sway
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|HUD Sway")
	uint8 bEnableHUDSway : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|HUD Sway", meta = (EditCondition = "bEnableHUDSway"))
	FVector2D MaxHUDSwayMovement = FVector2D::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|HUD Sway", meta = (EditCondition = "bEnableHUDSway"))
	FVector2D SwayStrength = FVector2D::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|HUD Sway", meta = (EditCondition = "bEnableHUDSway"))
	FVector2D SwaySpeed = FVector2D::ZeroVector;
	#pragma endregion

	#pragma region Curved HUD
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Curved HUD")
	uint8 bEnableCurvedHUD : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Curved HUD")
	float CurvedHUDStrength = 0.2f;
	#pragma endregion
	#pragma endregion

	#pragma region Data
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	class APlayerCharacter* PlayerCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	class AReadyOrNotPlayerController* PlayerController = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FVector2D AccumulatedMouseDelta = FVector2D::ZeroVector;
	#pragma endregion

private:
	void ProcessHUDSway(float InDeltaTime);

	bool ActionSlot1InUse = false;
	bool ActionSlot2InUse = false;
	bool ActionSlot3InUse = false;
	bool ActionSlot4InUse = false;
	bool ActionSlotReserved1InUse = false;
	bool ActionSlotReserved2InUse = false;

};
