// Copyright Void Interactive, 2022

#pragma once

#include "MenuWidget.h"
#include "Data/ItemData.h"
#include "lib/BpGameplayHelperLib.h"
#include "PreMissionPlanning.generated.h"

UENUM(BlueprintType)
enum class EPreMissionSubCategory : uint8
{
	None,
	Primary,
	Secondary,
	Tactical,
	Appearance,
	Protection,
	ItemSelection,
	Grenades,
	Clean
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UPreMissionPlanning : public UMenuWidget
{
	GENERATED_BODY()

public:
	UPreMissionPlanning();

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void Init(bool bReadOnly, FSavedLoadout PreviewLoadout);

	UFUNCTION(BlueprintPure)
	bool IsInLobby();

	UPROPERTY(BlueprintReadWrite)
	bool bIsWeaponCustomization = false;

	UPROPERTY(BlueprintReadWrite)
	bool bOpenInQuartermaster = false;

	UPROPERTY(BlueprintReadWrite)
	bool bIsCustomizingPrimary = false;

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<ABaseItem> CustomizeItemClass;

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotGameState* gs;

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotPlayerController* pc;

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotPlayerState* ps;

	UFUNCTION(BlueprintPure)
	static class UPreMissionPlanning* GetPremissionPlanning();
	
		UPROPERTY()
    	ULevelStreaming* PreMissionStreamedLevel;

	UPROPERTY()
	UFMODEvent* LoadoutMusic;

	FFMODEventInstance LoadoutEventInst;

	void PlayLoadoutMusic();
	void StopLoadoutMusic();

	// delete anything we have spawned in here
	void CleanUpCustomizationWorld();

	void GetSubPremissionPlanningActors(TArray<AActor*>& OutActors);

	// total seconds  the menu has  been opened
	float TotalSeconds = 0.0f;

	// our preview playerstate (our player controllers playerstate normally)
	UPROPERTY()
	class AReadyOrNotPlayerState* PreviewPlayerState = nullptr;

	bool bHasCameraEverBeenInStreamingLevel = false;
	bool bSetInitialLoadout = false;

	UPROPERTY(BlueprintReadOnly)
	EEquippingSwat EquippingSwatMember;

	UFUNCTION(BlueprintPure)
	void GetEquippingSwatMember(EEquippingSwat& EquippingSwat) { EquippingSwat = EquippingSwatMember; }

	UFUNCTION(BlueprintCallable)
	void SetEquippingSwatMember(EEquippingSwat NewEquippingSwat, AReadyOrNotPlayerState* NewEquippingPlayerState);

	UFUNCTION(BlueprintCallable)
	void SaveActiveLoadout();
	UPROPERTY()
	TMap<EEquippingSwat, FSavedLoadout> LastSavedLoadout;
	TMap<EEquippingSwat, FSavedLoadout> LastEquippedPreviewLoadout;

	// Timer so multiple calls to save active loadout don't call it multiple times
	FTimerHandle TH_DoSaveLoadout;
	UFUNCTION()
	void DoSaveLoadout(EEquippingSwat SwatMember, FSavedLoadout Loadout);

	UPROPERTY(BlueprintReadOnly)
	bool bLoadedLoadout = false;
	
	bool bWasVIP = false;

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

	// Active loaded saved in memory (and passed to  server when updated)
	UPROPERTY(BlueprintReadWrite)
	FSavedLoadout ActiveLoadout;

	UPROPERTY(BlueprintReadWrite)
	TMap<FName, FLoadoutPreset> LoadoutPresetMap;

	UPROPERTY(BlueprintReadWrite)
	AReadyOrNotPlayerState* EquippingPlayerState;

	UPROPERTY(BlueprintReadWrite)
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponPreset> WeaponToWeaponPresetsMap;
	
	UPROPERTY(BlueprintReadWrite)
	TMap<TSubclassOf<ABaseItem>, FSavedWeaponAttachmentData> WeaponToAttachmentsMap;

	UPROPERTY(BlueprintReadWrite)
	TMap<TSubclassOf<ABaseWeapon>, EFireMode> WeaponClassToDefaultFireModeMap;

	bool bDestructing = false;

	// Try set our players camera to the customization menu
	void TrySetPlayerCamera();

	UPROPERTY(BlueprintReadWrite)
	FName ActiveCameraTag = NAME_None;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestRefresh);
	// Binding to rrequest any child widgets etc to refresh (could be due to team change etc)
	UPROPERTY(BlueprintAssignable)
	FOnRequestRefresh OnRequestRefresh;

	bool bPlayInitialCameraFade = false;

	// Get's a preview character by tag (coudl be our player, or any of our teams players)
	class AReadyOrNotCharacter* GetPreviewCharacter(FName Tag);
	// gets our players preview character
	UFUNCTION(BlueprintCallable)
	class AReadyOrNotCharacter* GetDefaultPreviewCharacter() { return GetPreviewCharacter("PreviewCharacter"); }
	// updates the previe character with a loadout from a playerstate (or our active loadout if local)
	UFUNCTION(BlueprintCallable)
	void UpdatePreviewCharacter(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag = "PreviewCharacter");
	// Update anything we need to when our team changes (team side armor, unique skins etc)
	void UpdateTeamVisuals(class AReadyOrNotPlayerState* InPreviewPlayerState, FName Tag = "PreviewCharacter");
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> RedTeamClass;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> BlueTeamClass;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<APlayerCharacter> VIPClass;
	
	// tracks our last spawned teamtypes and does any updates if this differs from our playerstate
	ETeamType OurSpawnedTeamType;
	
	// update all our friendly loadouts
	void UpdateFriendlyLoadouts();
	void UpdateSwatTeamLoadouts();

	void RemoveDuplicates();
	
	// store a map of customization characters to preview playerstates
	UPROPERTY()
	TMap<AReadyOrNotCharacter*, AReadyOrNotPlayerState*> PlayerStatePreviewMap;

	// gets the quarter  master character in the customization menu
	AReadyOrNotCharacter* GetQuartermaster();

	FTimerHandle UnlockInput_Handle;
	bool bIsInputLocked;

	UFUNCTION()
	void SetLockInput(bool bShouldLockInput);
	UFUNCTION(BlueprintPure)
	bool GetInputLocked();

	UFUNCTION(BlueprintCallable)
	void PlayAnimationOnQuartermaster(FString Animation);
	
	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void PlayAnimationOnPreviewCharacter(FString Animation);

	FString GetPistolNonPistolVariation(FString InAnimation);

	FTimerHandle HidePistolHandle, HideRifleHandle;
	bool bRifleDrawn = false;
	bool bPistolDrawn = false;

	// sets our active camera by tag
	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetActiveCameraByTag(FName Tag, float BlendTime = 0.75f);

	TMap<FName, float> CameraTagOriginalFovMap;

	// sets any light colors by tag
	UFUNCTION(BlueprintCallable)
	void SetLightColorByTag(FName Tag, FLinearColor Color);

	TMap<FName, FLinearColor> LightTag;
	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void EquipPrimary();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void EquipSecondary();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void HideWeapons(bool bHidePrimary, bool bHideSecondary, float Delay = 0.0f);

	FTimerHandle HidePrimary_Handle, HideSecondary_Handle;

	UFUNCTION(BlueprintCallable)
	void HidePrimary(bool bIsHidden);

	UFUNCTION(BlueprintCallable)
	void HideSecondary(bool bIsHidden);

	UFUNCTION(BlueprintCallable)
	void AttachPrimaryToSocket(FName Socket);

	UFUNCTION(BlueprintCallable)
	void AttachSecondaryToSocket(FName Socket);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	bool IsAnyWeaponVisible();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
		void SetSubcategory(EPreMissionSubCategory NewSubCategory) { SubCategory = NewSubCategory; };

	void UpdatePrimaryWeaponAttachmentData();
	void UpdateSecondaryWeaponAttachmentData();

	void UpdateItemAttachmentData(TSubclassOf<ABaseItem> Weapon, FSavedWeaponAttachmentData AttachmentData);

	UFUNCTION(BlueprintCallable)
	void UpdateWeaponPresets(TSubclassOf<ABaseItem> Weapon, FSavedWeaponPreset Presets);

	UFUNCTION(BlueprintCallable)
	void UpdateWeaponPreset(TSubclassOf<ABaseItem> Weapon, FWeaponPreset PresetData, int32 Index);

	UFUNCTION(BlueprintCallable)
	void SaveItemClassAsSlot(EItemType ItemType, TSubclassOf<ABaseItem> Class);
	
	UFUNCTION(BlueprintCallable)
	void UpdateWeaponDefaultFireMode(TSubclassOf<ABaseWeapon> Weapon, EFireMode NewDefaultFireMode);
	
	UFUNCTION(BlueprintCallable)
	void SaveWeaponDefaultFireMode();
	
	UFUNCTION(BlueprintCallable)
	void LoadWeaponDefaultFireModes();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponDefaultFireModesLoaded();

	UFUNCTION(BlueprintCallable)
	TSubclassOf<ABaseItem> GetLastItemInSlot(EItemType ItemType);

	UFUNCTION(BlueprintPure)
	FSavedWeaponAttachmentData GetItemAttachmentData(TSubclassOf<ABaseItem> Weapon);

	UFUNCTION(BlueprintPure)
	FSavedWeaponPreset GetWeaponPresetsData(TSubclassOf<ABaseItem> Weapon);
	
	UFUNCTION(BlueprintPure)
	FWeaponPreset GetWeaponPresetData(TSubclassOf<ABaseItem> Weapon, int32 Index);

	// The item displayed on the bench when modifying
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, ABaseItem*> WorkBenchItemPtrMap;

	void UpdateOrbitalCameraLocation(FVector Location);

	UFUNCTION(BlueprintCallable)
	void SetWorkbenchItemClass(TSubclassOf<class ABaseItem> Item, FName Tag = "BenchPlacement");

	UFUNCTION(BlueprintCallable)
	void UpdateWorkbenchItemAttachments();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryWeapon(FWeaponData WeaponData);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewCharacterPrimary();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void DoPrimaryWeaponPreviewBlend();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
		void SetSecondaryWeapon(FWeaponData WeaponData);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewCharacterSecondary();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetHeadwear(TSubclassOf<ABaseItem> Headwear);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewCharacterHeadwear();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetBodyArmour(TSubclassOf<ABaseItem> BodyArmour);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewCharacterArmour();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetLongTactical(TSubclassOf<ABaseItem> LongTactical);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewCharacterLongTactical();

	UPROPERTY(BlueprintReadOnly)
	EItemType LastSetItemType;
	
	UPROPERTY(BlueprintReadOnly)
	EItemClass LastSetItemClass;
	
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<ABaseItem> LastSetItemObjectClass;
	
	UFUNCTION(BlueprintCallable, Category = "PreMissingPlanning")
	void SetItem(EItemType ItemType, TSubclassOf<ABaseItem> ItemClass);

	UFUNCTION(BlueprintCallable, Category = "PreMissingPlanning")
    void SetItem_V2(EItemClass ItemClass, TSubclassOf<ABaseItem> ItemObjectClass);

	UFUNCTION(BlueprintCallable, Category = "PreMissingPlanning")
	void SetPlayerSkin(TSubclassOf<USkinComponent> SkinCompClass);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	TArray<TSubclassOf<USkinComponent>> GetAvailablePlayerSkins();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewWeaponAttachments(bool IsSecondary, TSubclassOf<class UWeaponAttachment> Attachment);
	
	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void UpdatePreviewWeaponSkin(bool IsSecondary, TSubclassOf<class USkinComponent> SkinAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void ClearPreviewWeaponSkin(bool IsSecondary);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryScopeAttachment(TSubclassOf<class UWeaponAttachment> ScopeAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryMuzzleAttachment(TSubclassOf<class UWeaponAttachment> MuzzleAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryOverbarrelAttachment(TSubclassOf<class UWeaponAttachment> OverbarrelAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryUnderbarrelAttachment(TSubclassOf<class UWeaponAttachment> UnderbarrelAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
    void SetPrimaryStockAttachment(TSubclassOf<class UWeaponAttachment> StockAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
    void SetPrimaryGripAttachment(TSubclassOf<class UWeaponAttachment> GripAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
    void SetPrimaryIlluminatorAttachment(TSubclassOf<class UWeaponAttachment> IlluminatorAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimaryAmmunitionAttachment(TSubclassOf<class UWeaponAttachment> AmmunitionAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetPrimarySkinAttachment(TSubclassOf<class USkinComponent> SkinAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void CleanPrimaryGun();

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryScopeAttachment(TSubclassOf<class UWeaponAttachment> ScopeAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryMuzzleAttachment(TSubclassOf<class UWeaponAttachment> MuzzleAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryOverbarrelAttachment(TSubclassOf<class UWeaponAttachment> OverbarrelAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryUnderbarrelAttachment(TSubclassOf<class UWeaponAttachment> UnderbarrelAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryStockAttachment(TSubclassOf<class UWeaponAttachment> StockAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
    void SetSecondaryGripAttachment(TSubclassOf<class UWeaponAttachment> GripAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
    void SetSecondaryIlluminatorAttachment(TSubclassOf<class UWeaponAttachment> IlluminatorAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondaryAmmunitionAttachment(TSubclassOf<class UWeaponAttachment> AmmunitionAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void SetSecondarySkinAttachment(TSubclassOf<class USkinComponent> SkinAttachment);

	UFUNCTION(BlueprintCallable, Category = "PreMissionPlanning")
	void CleanSecondaryGun();
	
	EPreMissionSubCategory SubCategory;
	UFUNCTION(BlueprintPure, Category = "PreMissionPlanning")
	EPreMissionSubCategory GetSubcategory() { return SubCategory;}

	UFUNCTION(BlueprintPure, Category = "PreMissionPlanning")
	EItemType ItemClassToItemType(EItemClass InItemClass) const;

	UPROPERTY(BlueprintReadWrite)
	bool bCanUpdateWithUI = true;

	bool bCanEquipLoadout = false;
	void AttemptEquipLoadoutInGame();

	UFUNCTION(BlueprintCallable)
	void LoadLoadoutPresets();

	UFUNCTION(BlueprintCallable)
	void SaveLoadoutPresets();

	UFUNCTION(BlueprintCallable)
	void LoadWeaponPresets();
	
	UFUNCTION(BlueprintCallable)
	void SaveWeaponPresets();
	
	UFUNCTION(BlueprintCallable)
	void LoadWeaponAttachments();
	
	UFUNCTION(BlueprintCallable)
	void SaveWeaponAttachments();
	
	UReadyOrNotSaveGame* LoadGame();
	void SaveGame();

};
