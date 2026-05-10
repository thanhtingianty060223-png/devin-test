// Copyright Void Interactive, 2024

#pragma once

#include "CoreMinimal.h"
#include "CustomizationData.generated.h"

USTRUCT()
struct READYORNOT_API FSavedCustomization
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Character = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Voice = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Helmet = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Shirt = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Pants = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Shoes = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Eyewear = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Belt = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Gloves = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Watch = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* Tattoo = nullptr;
	
	/* Skins */
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* PrimarySkin = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* SecondarySkin = nullptr;

	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* ArmorSkin = nullptr;
	
	UPROPERTY(EditAnywhere)
	UCustomizationDataBase* HeadwearSkin = nullptr;

	// Annoying special case for helmets
	UPROPERTY(EditAnywhere)
	bool bHasHelmet = true;
	
	UPROPERTY(NotReplicated)
	TMap<TSoftClassPtr<class ABaseItem>, UCustomizationDataBase*> PreviousSkinsMap;
	
	void SetCustomizationItem(UCustomizationDataBase* Item);
	UCustomizationDataBase* GetCustomizationItem(ECustomizationType Type);
	
	void ApplyCustomization(class AReadyOrNotCharacter* Target);
	void ApplyCustomizationSkins(class AReadyOrNotCharacter* Target);
	
	static void ApplyItemCustomization(class ABaseItem* Target, UCustomizationSkin* Skin);
	static FSavedCustomization GetSavedCustomization(EEquippingSwat EquippingSwat);

	void ClearCustomization(class AReadyOrNotCharacter* Target);
	
	void Sanitize();
	void SanitizeServer(class AReadyOrNotCharacter* Target);
};

UENUM(BlueprintType)
enum class ECustomizationType : uint8
{
	None,
	Character,
	Voice,
	Helmet,
	Shirt,
	Pants,
	Shoes,
	Eyewear,
	Belt,
	Gloves,
	Watch,
	Tattoo,
	PrimarySkin,
	SecondarySkin,
	ArmorSkin,
	HeadwearSkin,
	Any UMETA(Hidden),
};

UENUM()
enum class ECustomizationAssetCookRule : uint8
{
	EditorOnly UMETA(ToolTip="Asset is never cooked"),
	DevelopmentOnly	UMETA(ToolTip="Asset is only cooked for development builds (i.e. nightly)"),
	AlwaysCook UMETA(ToolTip="Asset is always cooked, including shipping builds")
};

USTRUCT()
struct FCustomizationMaterialSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Slot = 0;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterialInterface> Material;
};

UCLASS(Abstract)
class READYORNOT_API UCustomizationDataBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// The type of customization this asset represents
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECustomizationType Type;

	// Parent customization asset, for defining families of customization
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCustomizationDataBase* Parent;
	
	// Friendly name for this asset used in the customization screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Parent == nullptr"))
	FText Name;

	// What variant this item represents, usually the color or pattern
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Variant;
	
	// The description for this asset in the customization screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;
	
	// The icon for this asset in the customization screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	// The variant icon to use for this item when shown as part of a family
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UMaterialInterface> VariantIcon;
	
	// The priority of this asset in menus, higher values show up last
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DisplayPriority = 1;
	
	// Tags needed in player's save for this asset to be considered unlocked
	UPROPERTY(EditAnywhere)
	TArray<FName> RequiredTags;
	
	// Text that describes how to acquire this item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText RequirementsText;

	// Categories of customization this item will hide
	UPROPERTY(EditAnywhere)
	TSet<ECustomizationType> TypesToHide;
	
	// True if this asset should be shown in the customization screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowInLoadout = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<EGameVersionRestriction> LockedToDLC;

#if WITH_EDITORONLY_DATA
	// Determines if this asset is included in a build
	UPROPERTY(EditAnywhere)
	ECustomizationAssetCookRule CookRule = ECustomizationAssetCookRule::AlwaysCook;
#endif
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("CustomizationAsset", GetFName()); }
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

#if WITH_EDITOR
	virtual bool IsEditorOnly() const override;
#endif
};

UCLASS()
class READYORNOT_API UCustomizationEmpty : public UCustomizationDataBase
{
	GENERATED_BODY()
};

UCLASS()
class READYORNOT_API UCustomizationSkeletalMesh : public UCustomizationDataBase
{
	GENERATED_BODY()
	
public:
	// The skeletal mesh to attach when this customization is equipped
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// The socket on the master skeleton to attach to, leave empty to not use any socket
	UPROPERTY(EditAnywhere)
	FName Socket;

	// True to set the master pose to base character skeleton
	UPROPERTY(EditAnywhere)
	bool bUseMasterPose = true;

	// Optional material overrides to apply to the base item, useful for items that share a common skeletal mesh
	UPROPERTY(EditAnywhere, meta=(TitleProperty="Material"))
	TArray<FCustomizationMaterialSlot> MaterialOverrides;

	// Socket overrides to apply to all items when this customization is equipped
	UPROPERTY(EditAnywhere)
	TMap<FName, FName> SocketOverridesMap;
};

UCLASS()
class READYORNOT_API UCustomizationStaticMesh : public UCustomizationDataBase
{
	GENERATED_BODY()
	
public:
	// The static mesh to attach when this customization is equipped
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	// The socket on the master skeleton to attach to, leave empty to not use any socket
	UPROPERTY(EditAnywhere)
	FName Socket; 
};

UCLASS()
class READYORNOT_API UCustomizationBlueprint : public UCustomizationDataBase
{
	GENERATED_BODY()

public:
	// The blueprint to attach when this customization is equipped
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AActor> BlueprintClass;

	// If true, will not force an actor to disable tick in third person
	UPROPERTY(EditAnywhere)
	bool bTickInThirdPerson = false;
	
	// The socket on the master skeleton to attach to, leave empty to not use any socket
	UPROPERTY(EditAnywhere)
	FName Socket;
};

UCLASS()
class READYORNOT_API UCustomizationSkin : public UCustomizationDataBase
{
	GENERATED_BODY()

public:
	// Items that have any of these tags will be considered compatible with this skin
	UPROPERTY(EditAnywhere)
	TArray<FName> CompatibleItemTags;

	// Optional mesh override to use for this skin, will have material slots applied to it
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> MeshOverride;
	
	// The material slots to override on the items this is compatible with
	UPROPERTY(EditAnywhere, meta=(TitleProperty="Material"))
	TArray<FCustomizationMaterialSlot> MaterialSlots;

	// Socket to use for this skin override
	UPROPERTY(EditAnywhere)
	bool bUseSocketOverride;
	
	// Socket to use for this skin override
	UPROPERTY(EditAnywhere, meta=(EditCondition="bUseSocketOverride"))
	FName SocketOverride;
};

UCLASS()
class READYORNOT_API UCustomizationCharacter : public UCustomizationDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> HeadMesh;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterialInterface> ArmsMaterial;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class UPoseAsset> FaceROM;

	UPROPERTY(EditAnywhere)
	int32 HairMaterialIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ProfileImage;
};

UCLASS()
class READYORNOT_API UCustomizationVoice : public UCustomizationDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName VoiceHandle;
};

UCLASS()
class READYORNOT_API UCustomizationTattoo : public UCustomizationDataBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> TattooTexture;
	
	UPROPERTY(EditAnywhere)
	FCustomizationMaterialSlot ArmSlotOverride;
};

