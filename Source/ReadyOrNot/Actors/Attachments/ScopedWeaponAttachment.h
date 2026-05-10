// Copyright Void Interactive, 2017

#pragma once

#include "WeaponAttachment.h"
#include "ScopedWeaponAttachment.generated.h"

USTRUCT(BlueprintType)
struct FScopeModifications
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Scope)
	UClass* WeaponClass;

	UPROPERTY(EditAnywhere, Category = Scope)
	float VerticalOffset;

	UPROPERTY(EditAnywhere, Category = Scope)
	float HorizontalOffset;

	UPROPERTY(EditAnywhere, Category = Scope)
	float DistanceOffset;

	UPROPERTY(EditAnywhere, Category = Scope)
	USkeletalMesh* CustomWeaponMesh;
	
	UPROPERTY(EditAnywhere, Category = Scope)
	FName HideBone;

	// addition for flipping sights!
	UPROPERTY(EditAnywhere, Category = Scope)
	bool bSupportsSecondarySights;

	UPROPERTY(EditAnywhere, Category = Scope)
	FVector MeshSpace_ADS_SecondaryPos;

	UPROPERTY(EditAnywhere, Category = Scope)
	FRotator MeshSpace_ADS_SecondaryRot;

	FScopeModifications()
	{
		WeaponClass = nullptr;
		VerticalOffset = 0.0f;
		HorizontalOffset = 0.0f;
		DistanceOffset = 0.0f;
		CustomWeaponMesh = nullptr;

		//
		bSupportsSecondarySights = false;
		MeshSpace_ADS_SecondaryPos = FVector(0, 0, 0);
		MeshSpace_ADS_SecondaryRot = FRotator(0, 0, 0);
	}

	bool operator==(const FScopeModifications& OtherItem) const
	{
		if (WeaponClass == OtherItem.WeaponClass && VerticalOffset == OtherItem.VerticalOffset)
		{
			return true;
		}
		return false;
	}

};

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class READYORNOT_API UScopedWeaponAttachment : public UWeaponAttachment
{
	GENERATED_BODY()

public:

	UScopedWeaponAttachment();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = Scope)
		TArray<FScopeModifications> ScopeMods;

	UFUNCTION(BlueprintCallable, Category = Scope)
	float GetMeshspaceOffsetVertical(class ABaseWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = Scope)
		float GetMeshspaceOffsetHorizontal(class ABaseWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = Scope)
		float GetMeshspaceOffsetDistance(class ABaseWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = Scope)
		FScopeModifications GetScopeMods(class ABaseWeapon* Weapon);

	UPROPERTY(EditAnywhere, Category = Scope)
		float ZoomFOVAddition = 0.0f;

	UPROPERTY(EditAnywhere, Category = Scope)
		float ZoomInSpeed = 20.0f;

	UPROPERTY(EditAnywhere, Category = Scope)
		float ZoomOutSpeed = 20.0f;	

	UPROPERTY(EditAnywhere, Category = Scope)
		bool bSupportsCowitness = false;

	UPROPERTY(EditAnywhere, Category = Scope)
		bool bUseScopeEffect = false;

	UPROPERTY(EditAnywhere, Category = Scope)
		TSoftObjectPtr<UTexture2D> ScopeReticle = nullptr;

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		bool bUsePipRendering = false;

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		TEnumAsByte<ESceneCaptureSource> CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		bool bOverridePostProcessSettings = false;

	UPROPERTY(BlueprintReadOnly, Category = "Scope|Picture In Picture")
		bool bNeedInventoryUpdate = true; // if true, we need to update the list of inventory items that are hidden

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture", meta = (ShowPostProcessCategories))
		FPostProcessSettings OverridePostProcessSettings;
	
	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		int32 PipMaterialIdx;

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		float PipFOV = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Scope|FOV")
	float PlayerCameraFOVMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "PipRender")
		UMaterialInstanceDynamic* PipRenderDynamicMaterial;

	UPROPERTY(VisibleAnywhere, Category = "PipRender")
		USceneCaptureComponent2D* PipSceneCapture;

	UPROPERTY(VisibleAnywhere, Category = "PipRender")
		UTextureRenderTarget2D* PipRenderTarget;

	UPROPERTY(EditAnywhere, Category = "Scope|Picture In Picture")
		FVector2D PipResolution = FVector2D(512, 512);

	float PipFadeIn  = 0.0f;
	
	// so we can register the state on the attachment itself!
	bool bIsSecondarySightActive;
};
