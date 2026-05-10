// Copyright Void Interactive, 2022

#pragma once

#include "Headwear.h"
#include "NightvisionGoggles.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API ANightvisionGoggles : public AHeadwear
{
	GENERATED_BODY()

		ANightvisionGoggles();
protected:
	UPROPERTY()
	UUserWidget * SpawnedWidget;

public:

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "NVG")
	UTexture2D* Green_LUT = nullptr;
	UPROPERTY(EditAnywhere, Category = "NVG")
	UTexture2D* White_LUT = nullptr;
	
	// The material parameter collection where our global NVG material parameter belongs
	UPROPERTY(EditAnywhere, Category = "NVG")
	UMaterialParameterCollection* GlobalMaterialParameters;

	// The name of the material parameter to set when NVGs are enabled/disabled
	UPROPERTY(EditAnywhere, Category = "NVG")
	FName NVGGlobalParameterName = "NightvisionEnabled";

	UPROPERTY(EditAnywhere, Category = "NVG")
	TSubclassOf<UUserWidget> NightVisionFirstPersonWidget;

	void UpdateNVGPostProcess();

	void SetNightvisionGlobalMaterialParameters(bool bEnabled);

	UFUNCTION()
	void SpawnNightvisionWidget();

	UFUNCTION()
	void DestroyNightvisionWidget();

	UFUNCTION(BlueprintImplementableEvent)
	void OnNightvisionActivated();

	UFUNCTION(BlueprintImplementableEvent)
	void OnNightvisionDeactivated();

	UPROPERTY(EditDefaultsOnly, Category = "NVG|Postprocess")
	FPostProcessSettings NightVisionPostProcess;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = NVG)
	bool bNVGOn = false;

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_SetTogglingNVG(bool bNewTogglingNVG);
	virtual void Server_SetTogglingNVG_Implementation(bool bNewTogglingNVG) { bTogglingNVG = bNewTogglingNVG; }
	virtual bool Server_SetTogglingNVG_Validate(bool bNewTogglingNVG) { return true; }

	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = NVG)
	bool bTogglingNVG = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = NVG)
	TArray<TSubclassOf<UDamageType>> BlockDamageTypesWhileActive;
};
