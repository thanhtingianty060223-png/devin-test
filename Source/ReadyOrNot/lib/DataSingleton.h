// Copyright Void Interactive, 2022

#pragma once

#include "Templates/SubclassOf.h"
#include "Engine/StreamableManager.h"
#include "Components/SkinComponent.h"
#include "Actors/Attachments/WeaponAttachment.h"
#include "Actors/BaseItem.h"
#include "Actors/SWATArmour.h"
#include "DataSingleton.generated.h"

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	MS_None,
	MS_Warmup,
	MS_Playing,
	MS_RoundEnded,
	MS_MatchEnded,
	MS_GoingToNextLevel
};

UENUM(BlueprintType)
enum class ETacticalAuthorityVoice : uint8
{
	TAV_None					UMETA(DisplayName="None"),

	// Tell suspect/civ to surrender
	TAV_Surrender				UMETA(DisplayName="Surrender"),

	// Tell suspect/civ to pick up an item
    TAV_PickUpItem				UMETA(DisplayName="Pick Up Item"),
	
	// Tell suspect/civ to drop the gun
	TAV_DropTheGun				UMETA(DisplayName="Drop The Gun"),
	
	// Tell suspect/civ to get on da floor
	TAV_GetOnTheFloor			UMETA(DisplayName="Get On The floor"),

	// Tell suspect/civ to come here
	TAV_ComeHere				UMETA(DisplayName="Come Here"),
	
	// Tell suspect/civ to wait
	TAV_Wait					UMETA(DisplayName="Wait"),
	
	// Tell suspect/civ to put their hands up
	TAV_PutHandsUp				UMETA(DisplayName="Put Hands Up"),
	
	TAV_MoveOverThere			UMETA(DisplayName="Move Over There"), 
	TAV_ReportDead				UMETA(DisplayName="Report Dead"),
	TAV_ReportArrested			UMETA(DisplayName="Report Arrested"),
	TAV_ReportIncapacitated		UMETA(DisplayName="Report Incapacitated"),
	TAV_ReportEvidence			UMETA(DisplayName="Report Evidence"),
};


USTRUCT(BlueprintType)
struct FSpawnedGear
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid Guid;

	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* Primary;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* Secondary;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* Armor;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* Helmet;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* RandomGear;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class UPlayableCharacterData* Character;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		class ABaseItem* LongTactical;
	//UPROPERTY(BlueprintReadWrite, Category = Inventory)
	//	TMap<class ABaseGrenade*, int32> GrenadeSlots;
	//UPROPERTY(BlueprintReadWrite, Category = Inventory)
	//	TMap<class ABaseItem*, int32> TacticalSlots;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		TArray<class ABaseItem*> Grenades;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		TArray<class ABaseItem*> TacticalDevices;
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
		TArray<class ABaseItem*> Miscelaneous;

	FSpawnedGear()
	{
		Guid = FGuid::NewGuid();
		Primary = nullptr;
		Secondary = nullptr;
		Armor = nullptr;
		Helmet = nullptr;
		RandomGear = nullptr;
		Grenades = {};
		TacticalDevices = {};
		LongTactical = nullptr;
		Character = nullptr;
		Miscelaneous = TArray<class ABaseItem*>();
	}

	void Remove(class ABaseItem* Item)
	{
		if (Item == Primary)
			Primary = nullptr;

		if (Item == Secondary)
			Secondary = nullptr;

		if (Item == LongTactical)
			LongTactical = nullptr;

		if (Item == Armor)
			Armor = nullptr;

		if (Item == Helmet)
			Helmet = nullptr;

		if (Item == RandomGear)
			RandomGear = nullptr;

		for (int32 i = 0; i < Miscelaneous.Num(); i++)
		{
			if (Miscelaneous[i] == Item)
				Miscelaneous[i] = nullptr;
		}
	}

};

USTRUCT(BlueprintType)
struct FSavedLoadout
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActiveLoadoutPreset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class ABaseItem> Primary;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryScope;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryMuzzle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryUnderbarrel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryOverbarrel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryStock;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryGrip;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryIlluminator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> PrimaryAmmunition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TArray<FName> PrimaryAmmoSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int32 PrimaryAmmoSlotsCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class USkinComponent> PrimarySkin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<ABaseItem> Secondary;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryScope;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryMuzzle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryUnderbarrel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryOverbarrel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryStock;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryGrip;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryIlluminator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class UWeaponAttachment> SecondaryAmmunition;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TArray<FName> SecondaryAmmoSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int32 SecondaryAmmoSlotsCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class USkinComponent> SecondarySkin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	FName CharacterType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class ABaseItem> LongTactical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TArray<TSubclassOf<ABaseItem>> GrenadeSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int32 GrenadeSlotsCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TArray<TSubclassOf<ABaseItem>> TacticalSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int32 TacticalSlotsCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class ABaseItem> Armor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class ABaseItem> Helmet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class ABaseItem> RandomGear;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TArray<TSubclassOf<class ABaseItem>> Miscelaneous;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TSubclassOf<class USkinComponent> PlayerSkin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	FString CharacterLookOverride = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	EArmourCoverage ArmourCoverage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	class UArmourMaterial* ArmourMaterial;

	FSavedLoadout()
	{
		Name = "";
		Primary = nullptr;
		PrimaryScope = nullptr;
		PrimaryMuzzle = nullptr;
		PrimaryUnderbarrel = nullptr;
		PrimaryOverbarrel = nullptr;
		PrimaryStock = nullptr;
		PrimaryGrip = nullptr;
		PrimaryIlluminator = nullptr;
		PrimaryAmmunition = nullptr;
		Secondary = nullptr;
		SecondaryScope = nullptr;
		SecondaryMuzzle = nullptr;
		SecondaryUnderbarrel = nullptr;
		SecondaryOverbarrel = nullptr;
		SecondaryStock = nullptr;
		SecondaryGrip = nullptr;
		SecondaryIlluminator = nullptr;
		SecondaryAmmunition = nullptr;
		LongTactical = nullptr;
		CharacterType = FName();
		Armor = nullptr;
		Helmet = nullptr;
		RandomGear = nullptr;
		Miscelaneous = TArray<TSubclassOf<class ABaseItem>>();
		ArmourCoverage = EArmourCoverage::AC_FrontBack;
		ArmourMaterial = nullptr;
		PrimaryAmmoSlotsCount = -1;
		SecondaryAmmoSlotsCount = -1;
		GrenadeSlotsCount = -1;
		TacticalSlotsCount = -1;
	}

	bool IsValid()
	{
		return !Name.IsEmpty() || Primary || Secondary || Armor || Helmet || LongTactical || !CharacterType.IsNone();
	}

	bool operator==(const FSavedLoadout& OtherItem) const
	{
		return Name == OtherItem.Name
			&& Primary == OtherItem.Primary
			&& Secondary == OtherItem.Secondary
			&& Armor == OtherItem.Armor
			&& Helmet == OtherItem.Helmet
			&& RandomGear == OtherItem.RandomGear
			&& LongTactical == OtherItem.LongTactical
			&& CharacterType == OtherItem.CharacterType
			&& ArmourCoverage == OtherItem.ArmourCoverage
			&& ArmourMaterial == OtherItem.ArmourMaterial
			&& PrimaryScope == OtherItem.PrimaryScope
			&& PrimaryMuzzle == OtherItem.PrimaryMuzzle
			&& PrimaryUnderbarrel == OtherItem.PrimaryUnderbarrel
			&& PrimaryOverbarrel == OtherItem.PrimaryOverbarrel
			&& PrimaryStock == OtherItem.PrimaryStock
			&& PrimaryGrip == OtherItem.PrimaryGrip
			&& PrimaryIlluminator == OtherItem.PrimaryIlluminator
			&& PrimaryAmmunition == OtherItem.PrimaryAmmunition
			&& PrimaryAmmoSlots == OtherItem.PrimaryAmmoSlots
			&& PrimaryAmmoSlotsCount == OtherItem.PrimaryAmmoSlotsCount
			&& SecondaryScope == OtherItem.SecondaryScope
			&& SecondaryMuzzle == OtherItem.SecondaryMuzzle
			&& SecondaryUnderbarrel == OtherItem.SecondaryUnderbarrel
			&& SecondaryOverbarrel == OtherItem.SecondaryOverbarrel
			&& SecondaryStock == OtherItem.SecondaryStock
			&& SecondaryGrip == OtherItem.SecondaryGrip
			&& SecondaryIlluminator == OtherItem.SecondaryIlluminator
			&& SecondaryAmmunition == OtherItem.SecondaryAmmunition
			&& SecondaryAmmoSlots == OtherItem.SecondaryAmmoSlots
			&& SecondaryAmmoSlotsCount == OtherItem.SecondaryAmmoSlotsCount
			&& PrimarySkin == OtherItem.PrimarySkin
			&& SecondarySkin == OtherItem.SecondarySkin
			&& PlayerSkin == OtherItem.PlayerSkin;
	}

	bool operator!=(const FSavedLoadout& OtherItem) const
	{
		return !(*this == OtherItem);
	}
};

USTRUCT(BlueprintType)
struct FMapList
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FString GameMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FString LoadURL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FText Author;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FText AuthorContact;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	FText RecommendedPlayerCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	class USoundCue* LoadingScreenMusic;

	
	// Supports PvP / cooperative against bots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	bool bSupportsPvP;

	// Supports CO-OP against suspects/civilians
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
	bool bSupportsCoop;

	// allows you to toggle visibility of this map in the selection without removing it...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Maps")
		bool bVisible = true;
};

USTRUCT(BlueprintType)
struct FItemID
{

	GENERATED_USTRUCT_BODY();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Items")
		int32 id;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Items")
		TSubclassOf<class ABaseItem> Item;
};

USTRUCT(BlueprintType)
struct FGameModeSettings : public FTableRowBase
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxPlayers = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RoundStartTime = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ReinforcementTimer = 30.0f;

	// used if reinfrocements isn't used
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RespawnTimer = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Timelimit = 1200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RoundsPerMap = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TDMScoreLimit = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText MatchStartInformationSwat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText MatchStartInformationMlo;
};


/**
 * 
 */
UCLASS(Blueprintable)
class READYORNOT_API UDataSingleton : public UObject
{
	GENERATED_BODY()

public:

	static UDataSingleton& Get();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(CallInEditor, Category = "Input")
	void RefreshInputKeyTable();

	UFUNCTION(BlueprintCallable, Category = "Maps & Modes")
	void UnloadLevels();

	UFUNCTION(BlueprintCallable, Category = "Maps & Modes")
	void LoadLevels();

	UPROPERTY(EditAnywhere, Category =  "Lookup Data")
	class ULookupData* LookupData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawBulletDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawNoNameplates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "loading")
	TArray<FText> LoadingScreen_Tips;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	FText YouKilledFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	TArray<FText> KillfeedFormatRandom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	TArray<FText> ArrestfeedFormatRandom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	TArray<FText> FreefeedFormatRandom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	TArray<FText> TeamkillfeedFormatRandom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Defaults")
	FText KillfeedWithA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowGrenadeDamage = false;

	UPROPERTY(EditAnywhere, Category = Data)
	class UItemData* ItemData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Data)
	class UWidgetsData* WidgetData;

	UPROPERTY(EditAnywhere, Category = Data)
	class UPenetrationData* PenetrationData;

	UPROPERTY(EditAnywhere, Category = Data)
	class UCampaignData* CampaignData;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* ItemDataLookupTable;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* AmmoDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* AIDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* LevelDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* AnimationDataLookupTable;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* AnimatedIconLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* DoorDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* TrapDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* ConversationLookupTable;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* GameSettingsLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* CharacterLookOverrideTable;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* RonInputKeyTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* InputKeyGamePadIconTable;

	UPROPERTY(EditAnywhere, Category = Data)
	TMap<FString, UDataTable*> SpeechDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* WidgetDataLookupTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* SuspectArmourDataTable;

	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* PairedInteractionDataTable;
	
	UPROPERTY(EditAnywhere, Category = Data)
	class UDataTable* MoveStyleDataTable;

	UPROPERTY(EditAnywhere, Category = LevelStreaming)
	FName CustomizationCharacterLevel;

	UPROPERTY(EditAnywhere, Category = LevelStreaming)
	FName CustomizationMenuLevel;

	UPROPERTY(EditAnywhere, Category = GeneratedWorldActivities)
	TArray<TSubclassOf<class UWorldBuildingActivity>> RandomWorldBuildingActivities;

	FStreamableManager AssetLoader;
};
