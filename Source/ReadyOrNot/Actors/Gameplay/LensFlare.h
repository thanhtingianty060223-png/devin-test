// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LensFlare.generated.h"

UCLASS()
class READYORNOT_API ALensFlare : public AActor
{
	GENERATED_BODY()

		UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Scene, meta = (AllowPrivateAccess = "true"))
	USceneComponent* Scene;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Scene, meta = (AllowPrivateAccess = "true"))
		UMaterialBillboardComponent* LensFlare;
	
	virtual void OnConstruction(const FTransform& Transform) override;
public:	
	// Sets default values for this actor's properties
	ALensFlare();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void InitFlare();

	UPROPERTY()
	AReadyOrNotCharacter* OwningCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dev-Only")
	UMaterialInterface* FlareMat;
	UPROPERTY(EditAnywhere, Category = "Dev-Only")
	FVector2D BaseSize;
	// basically how close to the camera it has to be shining to be seen
	// values closer to 1.0 are near the camera
	UPROPERTY(EditAnywhere, Category = "Dev-Only")
	float MinimumRotationDeltaToBeVisible = 0.2f;

	UPROPERTY()
	UMaterialInstanceDynamic* FlareMatInstance;
	
	float Luminosity = 0.0f;

	// global components
	UPROPERTY(EditAnywhere, Category = "Global")
	float GlobalBrightness = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	float GhostsBrightness = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	float FlickerIntensity = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	float FlickerTime = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	FLinearColor GlobalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Global")
	float FadeDistance = 2048.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	bool bUseFadeDistance = false;
	UPROPERTY(EditAnywhere, Category = "Global")
	float IncreasingBloomByDistance = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Global")
	bool bUseDistanceBloom = false;

	// halo
	UPROPERTY(EditAnywhere, Category = "Halo")
	float HaloOpacity = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Halo")
	float HaloInnerRadius = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Halo")
	float HaloBrightness = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Halo")
	FLinearColor HaloOuterColor = FLinearColor(0.035516f, 0.55179f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Halo")
	FLinearColor HaloInnerColor = FLinearColor(1.0f, 0.19647f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Halo")
	UTexture* HaloTexture = nullptr;
	UPROPERTY(EditAnywhere, Category = "Halo")
	float HaloTextureSize = 1.4285721f;
	UPROPERTY(EditAnywhere, Category = "Halo")
	float HaloContrast = 2.0f;

	// centre flare
	UPROPERTY(EditAnywhere, Category = "Centre Flare")
	float CentreFlareBrightness = 40.2380943f;
	UPROPERTY(EditAnywhere, Category = "Centre Flare")
	UTexture* CentreFlareTexture = nullptr;
	UPROPERTY(EditAnywhere, Category = "Centre Flare")
	FLinearColor CentreFlareColor = FLinearColor(1.0f, 0.19647f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Centre Flare")
	float CentreFlareSize = 1.4285721f;
	UPROPERTY(EditAnywhere, Category = "Centre Flare")
	float CentreFlareContrast = 2.0f;

	// main flare
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	float MainFlareBrightness = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	UTexture* MainFlare = nullptr;
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	float MainFlareInnerRadius = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	FLinearColor MainOuterColor = FLinearColor(1.0f, 0.19647f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	FLinearColor MainLensInnerColor = FLinearColor(1.0f, 0.19647f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	float MainFlareSize = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Main Flare")
	float MainFlareContrast = 5.0f;

	// reflection
	UPROPERTY(EditAnywhere, Category = "Reflection Centre")
	float ReflectionSize = 2.3809531f;
	UPROPERTY(EditAnywhere, Category = "Reflection Centre")
	UTexture* ReflectionCentreTexture = nullptr;
	UPROPERTY(EditAnywhere, Category = "Reflection Centre")
	FLinearColor ReflectionColor = FLinearColor(1.0f, 0.19647f, 0.0f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Reflection Centre")
	float ReflectionBrightness = 10.0f;
	UPROPERTY(EditAnywhere, Category = "Reflection Centre")
	float ReflectionContrast = 4.0f;

	// opposite flare
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float OppositeFlaresAxisProjection = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeSize01 = 44.7619095f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeSize02 = 16.1904774;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	FLinearColor FlareOppositeColor01 = FLinearColor(1.0f, 0.224955f, 0.693261f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	FLinearColor FlareOppositeColor02 = FLinearColor(1.0f, 0.0f, 0.83229f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeBrightness01 = 10.0f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeBrightness02 = 10.0f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeContrast01 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	float FlareOppositeContrast02 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	UTexture* FlareOpposite01 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Opposite Flare")
	UTexture* FlareOpposite02 = nullptr;

	// front flare
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FrontFlaresAxisProjection = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontSize01 = 17.4761906f;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontSize02 = 7.9523821;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	FLinearColor FlareFrontColor01 = FLinearColor(0.572916f, 0.12888f, 0.397181f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	FLinearColor FlareFrontColor02 = FLinearColor(0.421875f, 0.094903f, 0.292469f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontProximityCentre01 = 0.619048;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontProximityCentre02 = 0.8;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	UTexture* FlareFront01 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	UTexture* FlareFront02 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontContrast01 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontContrast02 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontBrightness01 = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Front Flare")
	float FlareFrontBrightness02 = 1.0f;

	// reflected flares
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareSize01 = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareSize02 = 6.0f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareProximityCentre01 = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareProximityCentre02 = 0.738095f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	UTexture* ReflectedFlare01 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	UTexture* ReflectedFlare02 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	FLinearColor ReflectedFlareColor01 = FLinearColor(1.0f, 0.008445f, 0.068168f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	FLinearColor ReflectedFlareColor02 = FLinearColor(1.0f, 0.0f, 0.110346f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareContrast01 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareContrast02 = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareBrightness01 = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Reflected Flares")
	float ReflectedFlareBrightness02 = 13.3333368f;

	// minor flare
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareProximityCentre01 = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareSize01 = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareSizeRandom01 = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	UTexture* MinorFlare01 = nullptr;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	FLinearColor MinorFlareColor01 = FLinearColor(0.244792f, 0.055067f, 0.169704f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareContrast01 = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareBrightness01 = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	FLinearColor MinorFlareColor02 = FLinearColor(0.286458f, 0.06444f, 0.19859f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareContrast02 = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareBrightness02 = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareProximityCentre02 = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareSize02 = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	float MinorFlareSizeRandom02 = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Minor Flares")
	UTexture* MinorFlare02 = nullptr;

	// iris reflection
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	UTexture* Iris = nullptr;
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	float IrisSize = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	float IrisProximityCentre = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	float IrisBrightness = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	float IrisContrast = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Iris Reflection")
	FLinearColor IrisColor = FLinearColor(1.0f, 0.224955f, 0.693261f, 1.0f);

	// additional flare
	UPROPERTY(EditAnywhere, Category = "Additional Flare")
	UTexture* AdditionalFlare = nullptr;
	UPROPERTY(EditAnywhere, Category = "Additional Flare")
	float AdditionalFlareSize = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Additional Flare")
	float AdditionalFlareContrast = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Additional Flare")
	float AdditionalFlareBrightness = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Additional Flare")
	FLinearColor AdditionalFlareColor = FLinearColor(1.0f, 0.224955f, 0.693261f, 1.0f);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetOwningCharacter(AReadyOrNotCharacter* NewOwningCharacter) { OwningCharacter = NewOwningCharacter; }

};
