// Copyright Void Interactive, 2023

#pragma once
#include "CommonActivatableWidget.h"
#include "Actors/Triggers/LoadoutPortal.h"
#include "HUD/Widgets/PreMissionPlanning.h"

#include "Loadout_v2.generated.h"

UCLASS()
class READYORNOT_API ULoadout_V2 : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	bool IsInLobby();
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OpenCustomization();

protected:
	void AttemptEquipLoadoutInGame();

	UFUNCTION(BlueprintCallable)
	void SetDefaultCamera(float BlendTime = 0.0f);

	UPROPERTY(BlueprintReadWrite)
	class UOverview_V2* OverviewWidget;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UCommonActivatableWidgetStack* WidgetStack;

	UPROPERTY(BlueprintReadOnly)
	class AReadyOrNotGameState* gs;

	UPROPERTY(BlueprintReadOnly)
	class AReadyOrNotPlayerController* pc;

	UPROPERTY(BlueprintReadOnly)
	class AReadyOrNotPlayerState* ps;

	ETeamType OurSpawnedTeamType;

	UPROPERTY()
	class AReadyOrNotPlayerState* PreviewPlayerState = nullptr;
	UPROPERTY()
	TMap<AReadyOrNotCharacter*, AReadyOrNotPlayerState*> PlayerStatePreviewMap;

	void UpdateTeamVisuals(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag = "PreviewCharacter");
	void DoPrimaryWeaponPreviewBlend();

	UFUNCTION(BlueprintCallable)
	void SetPrimaryWeapon(FWeaponData WeaponData);
	UFUNCTION(BlueprintCallable)
	void SetSecondaryWeapon(FWeaponData WeaponData);
	FSavedWeaponAttachmentData GetItemAttachmentData(TSubclassOf<ABaseItem> Weapon);
	FString GetPistolNonPistolVariation(FString InAnimation);
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> RedTeamClass;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> BlueTeamClass;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> VIPClass;

	UFUNCTION(BlueprintCallable)
	void PlayAnimationOnPreviewCharacter(FString Animation);

	UFUNCTION(BlueprintCallable)
	void HidePrimary(bool bIsHidden);
	UFUNCTION(BlueprintCallable)
	void LoadWeaponPresets();

	UPROPERTY(BlueprintReadWrite)
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponPreset> WeaponToWeaponPresetsMap;

	UFUNCTION(BlueprintCallable)
	void UpdatePreviewWeaponAttachments(bool IsSecondary, TSubclassOf<UWeaponAttachment> Attachment);

	UFUNCTION()
	void LoadAddAttachment(ABaseWeapon* BaseWeapon, TSubclassOf<UWeaponAttachment> Attachment, bool bReplicateAttachment = true);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutLoaded();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutSaved();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSwatCharacterChanged();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutPresetsLoaded();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutPresetsSaved();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutItemPresetsLoaded();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutItemPresetsSaved();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutItemAttachmentsSaved();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLoadoutItemAttachmentsLoaded();

	UFUNCTION(BlueprintCallable)
	void ApplyLoadoutPreset(FLoadoutPreset LoadoutPreset);

	UPROPERTY(BlueprintReadWrite)
	AReadyOrNotPlayerState* EquippingPlayerState;

	FTimerHandle TH_DoSaveLoadout;
	void EquipSecondary();
	UFUNCTION(BlueprintCallable)
	void SetActiveCameraByTagWithFade(FName Tag, float BlendTime, float FadeTime);
	UFUNCTION(BlueprintCallable)
	void SetActiveCameraByTag(FName Tag, float BlendTime);

	UPROPERTY()
	TMap<EEquippingSwat, FSavedLoadout> LastSavedLoadout;
	UPROPERTY()
	TMap<EEquippingSwat, FSavedLoadout> LastEquippedPreviewLoadout;

	UPROPERTY(BlueprintReadWrite)
	FString PrimaryDrawAnim;

	UPROPERTY(BlueprintReadWrite)
	FString PrimaryHolsterAnim;

	UPROPERTY(BlueprintReadWrite)
	FString SidearmDrawAnim;

	UPROPERTY(BlueprintReadWrite)
	FString SidearmHolsterAnim;

	UPROPERTY(BlueprintReadWrite)
	FString CurrentPreviewAnimation;

	UPROPERTY(BlueprintReadWrite)
	FName CurrentCameraTag;

	UPROPERTY(BlueprintReadWrite)
	float HolsterTimeRemaining;

	UPROPERTY(BlueprintReadWrite)
	TMap<EItemCategory, FName> CategoryCameraTags;

	UPROPERTY(BlueprintReadWrite)
	TMap<EItemCategory, FString> CategoryPoses;

	UPROPERTY(BlueprintReadWrite)
	bool ApplyingPresets = false;

	UPROPERTY(BlueprintReadWrite)
	bool UsingPreset = false;

	UPROPERTY(BlueprintReadWrite)
	bool PresetDirty = false;

	UPROPERTY(BlueprintReadWrite)
	bool VerticalListOpen;

	UPROPERTY(BlueprintReadWrite)
	bool ListVisible;

	UPROPERTY(BlueprintReadWrite)
	UWidgetAnimation* AnimExtendListSlide;

	// UPROPERTY(BlueprintReadWrite)
	// class FMODEvent* WeaponHolsteredSound;
	UFUNCTION(BlueprintCallable)
	class AReadyOrNotCharacter* GetDefaultPreviewCharacter() { return GetPreviewCharacter("PreviewCharacter"); }

	UFUNCTION()
	void SetLockInput(bool bShouldLockInput);
	UFUNCTION(BlueprintPure)
	bool GetInputLocked();
	// UFUNCTION(BlueprintCallable)
	// void SaveActiveLoadout();
	UFUNCTION(BlueprintCallable)
	void SetItem(EItemType ItemType, TSubclassOf<ABaseItem> ItemClass);
	UFUNCTION(BlueprintCallable)
	void SetLongTactical(TSubclassOf<ABaseItem> LongTactical);
	UFUNCTION()
	void UpdatePreviewCharacterLongTactical();
	UFUNCTION(BlueprintCallable)
	void SetHeadwear(TSubclassOf<ABaseItem> Headwear);
	UFUNCTION()
	void UpdatePreviewCharacterHeadwear();
	UFUNCTION(BlueprintCallable)
	void SetBodyArmour(TSubclassOf<ABaseItem> BodyArmour);
	UFUNCTION(BlueprintCallable)
	void UpdatePreviewCharacterArmour();
	UFUNCTION(BlueprintCallable)
	void UpdateWorkbenchItemAttachments(FSavedLoadout Loadout, bool IsSecondary);
	UFUNCTION(BlueprintCallable)
	void SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag, FSavedLoadout Loadout);
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, ABaseItem*> WorkBenchItemPtrMap;
	UPROPERTY(BlueprintReadWrite)
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponAttachmentData> WeaponToAttachmentsMap;
	UFUNCTION()
	void SaveStoredWeaponAttachments();
	UFUNCTION()
	void LoadStoredWeaponAttachments();

	UFUNCTION(BlueprintCallable)
	void HidePrimaryAndSecondary();
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsCustomizingPrimary = false;

	FTimerHandle UnlockInput_Handle;
	bool bIsInputLocked;

	UPROPERTY(BlueprintReadWrite)
	FName ActiveCameraTag = NAME_None;

	TMap<FName, float> CameraTagOriginalFovMap;

	bool bDestructing = false;

	UPROPERTY(BlueprintReadWrite)
	class ULoadoutSlotWidget* CurrentActiveSlot;

	UFUNCTION(BlueprintCallable)
	void UpdateDefaultPreviewCharacter();

	UFUNCTION(BlueprintCallable)
	void UpdatePreviewCharacter(AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag);

	UPROPERTY(Transient, meta=(BindWidgetAnim))
	UWidgetAnimation* FadeOut;

private:
	AReadyOrNotCharacter* GetPreviewCharacter(FName Tag);
	UFUNCTION(BlueprintCallable)
	void UpdatePreviewCharacterPrimary();
	UFUNCTION(BlueprintCallable)
	void UpdatePreviewCharacterSecondary();
	void AttachSecondaryToSocket(FName Socket);
	void EquipPrimary();
	FTimerHandle HidePistolHandle, HideRifleHandle;
	bool bRifleDrawn = false;
	bool bPistolDrawn = false;
	bool IsExiting = false;

public:
	UPROPERTY()
	class UReadyOrNotLoadoutManager* LoadoutFunctionLibrary;
	UFUNCTION(BlueprintCallable)
	void ExitLoadout();
};
