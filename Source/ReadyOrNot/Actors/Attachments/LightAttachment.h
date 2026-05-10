// Copyright Void Interactive, 2022

#pragma once

#include "Actors/Attachments/WeaponAttachment.h"
#include "LightAttachment.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULightAttachment : public UWeaponAttachment
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, Category = Light)
	USpotLightComponent* SpotLightComponent;

	UPROPERTY(VisibleAnywhere, Category = Light)
	UPointLightComponent* PointLightComponent;

	UPROPERTY(ReplicatedUsing=OnRep_On)
	bool bRepOn;

	UFUNCTION()
	void OnRep_On();

	float AvgDistance = 0.0f;

	// Spotlight intensity, editable in Ready or Not Level Script
	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	float Intensity = 400000.0f;

	// Fake bounced light intensity
	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	float BouncedIntensity = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	float Attenuation = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	float InnerConeAngle = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	float OuterConeAngle = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	class UMaterialInterface* LightFunctionMaterial;

	UPROPERTY(EditAnywhere, Category = "Exposed Light Data")
	FVector LightFunctionScale = FVector(1024, 1024, 1024);

	UPROPERTY(EditAnywhere, Category = "Lens Flare")
	TSubclassOf<class ALensFlare> LensFlareClass;
	UPROPERTY(EditAnywhere, Category = "Lens Flare")
	class ALensFlare* SpawnedLensFlare;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flash Light Tracking", meta = (ClampMin = "2", ClampMax = "10", UIMin = "1", UIMax = "10"))
	int32 NumOfFlashLightTrackingPoints = 4;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flash Light Tracking")
	TArray<class AFlashLightTrackingPoint*> FlashLightTrackingPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flash Light Tracking")
	class AFlashLightTrackingPoint* PrimaryTrackingPoint = nullptr;

	FCollisionQueryParams FlashLightQueryParams;

	UFUNCTION(BlueprintCallable, Category = Light)
	void ToggleLight(bool bOn);
	UFUNCTION(BlueprintPure, Category = Light)
	bool IsLightOn();

	void AttachLight();
	void DetachLight();
	
	void InitialiseFlashLightTrackingPoints();
	void DestroyFlashLightTrackingPoints();

	void AttachTrackingPoint();
	void DetachTrackingPoint();
	
	bool bIsFlashLightTrackable = false;
	
	virtual void DestroyComponent(bool bPromoteChildren = false) override;

private:
	/**
	*  @param  Start           Start location of the ray
	*  @param  End             End location of the ray
	*  @param  Position			Position in line 0 - 1 , E.g. 1/3 will give 1/3 point
	*/
	FVector CalculatePointInLine(FVector Start, FVector End, float Position);
};
