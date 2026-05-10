// Copyright Void Interactive, 2023

#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "Enums.h"
#include "WeaponAttachment.generated.h"

USTRUCT(BlueprintType)
struct FAttachmentUIElements
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Scope)
	TSoftObjectPtr<UTexture2D> ImageOfScope;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Scope)
	TSoftObjectPtr<UTexture2D> SightPicture;

	// icon as seen on the customization screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
	TSoftObjectPtr<UTexture2D> AttachmentIcon;
};

UENUM(BlueprintType)
enum class EWeaponAttachmentType : uint8
{
	Null,
	Optics,
	Muzzle,
	Underbarrel,
	Overbarrel,
	Stock,
	Grip,
	Illuminators,
	Ammunition,
};

UENUM(BlueprintType)
enum class EWeaponUnderbarrelAnimationType : uint8
{
	WU_None,
	WU_VFG,
	WU_AFG,
	WU_Handstop,
};

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class READYORNOT_API UWeaponAttachment : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UWeaponAttachment();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = Environmental)
	void ApplyWetness(float Wetness, float RainAmount);

	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TArray<UMaterialInstanceDynamic*> SkinMaterials;

	UFUNCTION(BlueprintCallable)
	void PlayToggleSound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	EWeaponAttachmentType WeaponAttachmentType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachment")
	TArray<EWeaponAttachmentType> RemovesWeaponAttachmentTypes;

	// For underbarrel attachments, this is what animation to use
	UPROPERTY(EditAnywhere, Category = "Attachment|Underbarrel")
		EWeaponUnderbarrelAnimationType UnderbarrelAnimationType;

	// If true, the weapon attachment doesn't need to physically appear in order to apply its effects.
	UPROPERTY(EditAnywhere, Category = Attachment)
		bool bNeedNotAttach = false;

	// is a null attachment (designed to remove any added attachment and delete itself)
	UPROPERTY(EditAnywhere, Category = Attachment)
	bool bNullAttachmentOnly = false;

	UPROPERTY(EditAnywhere, Category = Attachment)
	FName SocketAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
	int32 PointCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
	FAttachmentUIElements UIElements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
	TArray<EItemAttachment> AdditionalAttachments;

	// In kilograms.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
		float AttachmentWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float VerticalRecoilMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float HorizontalRecoilMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float SpreadMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float DeadzoneMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float MuzzleVelocityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		float LowReadyLengthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		bool bShouldSupressWeapon = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		bool bHidesMuzzleFlash = false;

	// overrides muzzle flash and muzzle smoke particles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		bool bOverrideMuzzleFlash = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		UParticleSystem* MuzzleFlashParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		UParticleSystem* MuzzleSmokeParticle;

	bool bAddedMagazineAmmo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		int32 MagazineAmmoIncrease = 0;

	// if noit null will hide the bone with this name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
		FName HideBoneOnMesh = NAME_None;

	FORCEINLINE class ABaseWeapon* GetWeapon() const { return GetOwner<ABaseWeapon>(); }

	virtual void DestroyComponent(bool bPromoteChildren = false) override;

	/* gets summed up then multiplied against base weapon bob factor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Bob")
	float CameraBobAdditionFactor = 0.0f;

	/* modifies the total ADS speed*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment)
	float ADS_Speed_Multiplier = 1.0f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toggle")
	class UFMODEvent* ToggleSound;
};
