// Copyright Void Interactive, 2021

#include "LensFlare.h"
#include "ReadyOrNot.h"

void ALensFlare::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitFlare();
}

// Sets default values
ALensFlare::ALensFlare()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);

	LensFlare = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("LensFlare"));
	LensFlare->AddElement(nullptr, nullptr, true, 1.0f, 1.0f, nullptr);
	LensFlare->SetupAttachment(Scene);
}

void ALensFlare::BeginPlay()
{
	Super::BeginPlay();
	InitFlare();
}

void ALensFlare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetGameTimeSinceCreation() < 1.0f)
		return;
	
	if (!FlareMatInstance)
		return;

	APlayerController* LocalController = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	
	#if WITH_EDITOR
	LensFlare->Elements[0].BaseSizeX = BaseSize.X;
	LensFlare->Elements[0].BaseSizeY = BaseSize.Y;
	
	// global
	FlareMatInstance->SetScalarParameterValue("Global_Brightness", GlobalBrightness);
	FlareMatInstance->SetScalarParameterValue("SmallFlares_Brightness", GhostsBrightness);
	FlareMatInstance->SetScalarParameterValue("StaticState", FlickerIntensity);
	FlareMatInstance->SetScalarParameterValue("TimeVatiable", FlickerTime);
	FlareMatInstance->SetVectorParameterValue("Global_Color", GlobalColor);
	FlareMatInstance->SetScalarParameterValue("FadeDistance", FadeDistance);
	FlareMatInstance->SetScalarParameterValue("UseFadeDistance", bUseFadeDistance);
	FlareMatInstance->SetScalarParameterValue("IncreasingBloomByDistance", IncreasingBloomByDistance);
	FlareMatInstance->SetScalarParameterValue("UseDistanceBloom", bUseDistanceBloom);

	// halo
	FlareMatInstance->SetScalarParameterValue("Halo_Opacity", HaloOpacity);
	FlareMatInstance->SetScalarParameterValue("Halo_InnerRadius", HaloInnerRadius);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Brightness", HaloBrightness);
	FlareMatInstance->SetVectorParameterValue("Halo_Outer_Color", HaloOuterColor);
	FlareMatInstance->SetVectorParameterValue("Halo_Inner_Color", HaloInnerColor);
	FlareMatInstance->SetTextureParameterValue("T_HaloTexture", HaloTexture);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Size_Reduction", HaloTextureSize);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Contrast", HaloContrast);

	//centre flare
	FlareMatInstance->SetScalarParameterValue("Centre_Brightness", CentreFlareBrightness);
	FlareMatInstance->SetTextureParameterValue("T_Centre", CentreFlareTexture);
	FlareMatInstance->SetVectorParameterValue("Centre_Color", CentreFlareColor);
	FlareMatInstance->SetScalarParameterValue("Centre_Size_Reduction", CentreFlareSize);
	FlareMatInstance->SetScalarParameterValue("Centre_Contrast", CentreFlareContrast);


	// main flare
	FlareMatInstance->SetScalarParameterValue("MainFlare_Brightness", MainFlareBrightness);
	FlareMatInstance->SetTextureParameterValue("T_Main_Flare", MainFlare);
	FlareMatInstance->SetScalarParameterValue("MainFlare_InnerRadius", MainFlareInnerRadius);
	FlareMatInstance->SetVectorParameterValue("MainLens_Outer_Color", MainOuterColor);
	FlareMatInstance->SetVectorParameterValue("MainLens_Inner_Color", MainLensInnerColor);
	FlareMatInstance->SetScalarParameterValue("Main_Flare_Size_Reduction", MainFlareSize);
	FlareMatInstance->SetScalarParameterValue("MainFlare_Contrast", MainFlareContrast);
	
	// reflection centre
	FlareMatInstance->SetScalarParameterValue("EyeReflection_SizeReduction", ReflectionSize);
	FlareMatInstance->SetTextureParameterValue("T_EyeReflectionCentre", ReflectionCentreTexture);
	FlareMatInstance->SetVectorParameterValue("EyeReflection_Color", ReflectionColor);
	FlareMatInstance->SetScalarParameterValue("ReflectionCentre_Brightness", ReflectionBrightness);
	FlareMatInstance->SetScalarParameterValue("ReflectionContrast", ReflectionContrast);
	
	// opposite flare
	FlareMatInstance->SetScalarParameterValue("FlaresOpposite_ProjectionOnAxis", OppositeFlaresAxisProjection);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite_1_SizeReduction", FlareOppositeSize01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite_2_SizeReduction", FlareOppositeSize02);
	FlareMatInstance->SetVectorParameterValue("FlareOpposite_1_Color", FlareOppositeColor01);
	FlareMatInstance->SetVectorParameterValue("FlareOpposite_2_Color", FlareOppositeColor02);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite1_Brightness", FlareOppositeBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite2_Brightness", FlareOppositeBrightness02);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite1_Contrast", FlareOppositeContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite2_Contrast", FlareOppositeContrast02);
	FlareMatInstance->SetTextureParameterValue("T_FlareOpposite_1", FlareOpposite01);
	FlareMatInstance->SetTextureParameterValue("T_FlareOpposite_2", FlareOpposite02);
	
	// front flare
	FlareMatInstance->SetScalarParameterValue("FlaresFront_ProjectionOnAxis", FrontFlaresAxisProjection);
	FlareMatInstance->SetScalarParameterValue("FlareFront_1_SizeReduction", FlareFrontSize01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_2_SizeReduction", FlareFrontSize02);
	FlareMatInstance->SetVectorParameterValue("Flare_1_Color", FlareFrontColor01);
	FlareMatInstance->SetVectorParameterValue("Flare_2_Color", FlareFrontColor02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_1_ProximityToCentre", FlareFrontProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_2_ProximityToCentre", FlareFrontProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_FlareFront_1", FlareFront01);
	FlareMatInstance->SetTextureParameterValue("T_FlareFront_2", FlareFront02);
	FlareMatInstance->SetScalarParameterValue("FlareFront1_Contrast", FlareFrontContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareFront2_Contrast", FlareFrontContrast02);
	FlareMatInstance->SetScalarParameterValue("FlareFront1_Brightness", FlareFrontBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareFront2_Brightness", FlareFrontBrightness02);

	// reflectedflares
	FlareMatInstance->SetScalarParameterValue("FlareMirror_1_SizeReduction", ReflectedFlareSize01);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_1_ProximityToCentre", ReflectedFlareProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_2_SizeReduction", ReflectedFlareSize02);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_2_ProximityToCentre", ReflectedFlareProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Mirror_1", ReflectedFlare01);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Mirror_2", ReflectedFlare02);
	FlareMatInstance->SetVectorParameterValue("FlareMirror_1_Color", ReflectedFlareColor01);
	FlareMatInstance->SetVectorParameterValue("FlareMirror_2_Color", ReflectedFlareColor02);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare01Contrast", ReflectedFlareContrast01);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare02Contrast", ReflectedFlareContrast02);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare01Brightness", ReflectedFlareBrightness01);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare02Brightness", ReflectedFlareBrightness02);
	
	//minor flare
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_ProximityToCentre", MinorFlareProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_SizeReduction", MinorFlareSize01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_Size_Random", MinorFlareSizeRandom01);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Minor_1", MinorFlare01);
	FlareMatInstance->SetVectorParameterValue("FlareMinor_1_Color", MinorFlareColor01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorContrast01", MinorFlareContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorBrightness01", MinorFlareBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorContrast02", MinorFlareContrast02);
	FlareMatInstance->SetScalarParameterValue("FlareMinorBrightness02", MinorFlareBrightness02);
	FlareMatInstance->SetVectorParameterValue("FlareMinor_2_Color", MinorFlareColor02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_SizeReduction", MinorFlareSize02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_Size_Random", MinorFlareSizeRandom02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_ProximityToCentre", MinorFlareProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Minor_2", MinorFlare02);
	
	//iris reflection
	FlareMatInstance->SetTextureParameterValue("T_Iris", Iris);
	FlareMatInstance->SetScalarParameterValue("Iris_SizeReduction", IrisSize);
	FlareMatInstance->SetScalarParameterValue("Iris_ProximityToCentre", IrisProximityCentre);
	FlareMatInstance->SetScalarParameterValue("Iris_intensity", IrisBrightness);
	FlareMatInstance->SetScalarParameterValue("Iris_Contrast", IrisContrast);
	FlareMatInstance->SetVectorParameterValue("IrisColor", IrisColor);

	// additional flare
	FlareMatInstance->SetTextureParameterValue("T_AdditionalFlare", AdditionalFlare);
	FlareMatInstance->SetScalarParameterValue("AdditionalFlare_SizeReduction", AdditionalFlareSize);
	FlareMatInstance->SetScalarParameterValue("Additional Flare Contrast", AdditionalFlareContrast);
	FlareMatInstance->SetScalarParameterValue("Additional Flare Brightness", AdditionalFlareBrightness);
	FlareMatInstance->SetVectorParameterValue("AdditionalFlareColor", AdditionalFlareColor);
	#endif
	
	AReadyOrNotCharacter* LocalPlayer = Cast<AReadyOrNotCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!LocalPlayer)
	{
		if (LocalController)
		{
			if (ASpectatePawn* SpectatorPawn = Cast<ASpectatePawn>(LocalController->GetPawn()))
			{
				LocalPlayer = Cast<AReadyOrNotCharacter>(SpectatorPawn->CurrentViewTarget);
			}
		}
	}

	APlayerCameraManager* CameraManager = LocalController ? LocalController->PlayerCameraManager : nullptr;
	if (LocalController && LocalPlayer && CameraManager)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		//CameraLocation.Z = LocalPlayer->GetActorLocation().Z + 70.0f;
		//FRotator CameraRotation = CameraManager->GetCameraRotation();
		FVector Direction = (GetActorLocation() - CameraLocation).GetSafeNormal();
		
		FVector StartTrace = GetActorLocation();
		FVector EndTrace = CameraLocation + Direction * 10.0f;

		FCollisionQueryParams CollisionParams;
		if (IsValid(OwningCharacter))
		{
			CollisionParams = OwningCharacter->GetCollisionQueryParameters();
			CollisionParams.AddIgnoredActor(OwningCharacter);
		}
		CollisionParams.AddIgnoredActor(this);
		CollisionParams.AddIgnoredActor(LocalPlayer);
		
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_PROJECTILE, CollisionParams);
		//DrawDebugLine(GetWorld(), StartTrace, EndTrace, HitResult.bBlockingHit ? FColor::Red : FColor::Green, false, DeltaTime + 0.001f);
		
		if (!HitResult.bBlockingHit)
		{
			//FVector CameraLocation = LocalPlayer->GetMesh()->GetBoneLocation("head");
			FVector v1 = GetActorForwardVector();
			FVector v2 = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CameraLocation).Vector();
			//float FlareRotationDelta = FVector::DotProduct(v1, -CameraRotation.Vector());
			float FlareRotationDelta = FVector::DotProduct(v1, v2);
			//LOG_NUMBER(FlareRotationDelta);
			FlareMatInstance->SetScalarParameterValue("Main_Flare_Size_Reduction", FMath::Clamp(10.0f - (FMath::Clamp(FlareRotationDelta, 0.0f, 1.0f) * 10.0f), 1.0f, 10.0f));
			float DistanceScale = (CameraLocation - GetActorLocation()).Size();
			DistanceScale = FMath::Clamp(1.0f - UKismetMathLibrary::NormalizeToRange(DistanceScale, 0.0f, 1000.0f), 0.0f, 1.0f);
			
			LensFlare->Elements[0].BaseSizeX = DistanceScale * 4.0f * FMath::Clamp(FlareRotationDelta, 0.0f, 1.0f);
			LensFlare->Elements[0].BaseSizeY = DistanceScale * 4.0f * FMath::Clamp(FlareRotationDelta, 0.0f, 1.0f);
			LensFlare->Elements[0].bSizeIsInScreenSpace = true;

			ABaseArmour* ba = Cast<ABaseArmour>(LocalPlayer->GetInventoryComponent()->GetSpawnedGear().Helmet);
			if (ba)
			{
				LensFlare->Elements[0].BaseSizeX *= ba->ScaleLensFlare;
				LensFlare->Elements[0].BaseSizeY *= ba->ScaleLensFlare;
			}
			
			LensFlare->SetVisibility(true);

			MarkComponentsRenderStateDirty();

			if (FlareRotationDelta > MinimumRotationDeltaToBeVisible)
			{
				Luminosity = UKismetMathLibrary::FInterpTo_Constant(Luminosity, FMath::Clamp(FlareRotationDelta, 0.0f, 1.0f), DeltaTime, 0.1f);
			}
			else
			{
				Luminosity = UKismetMathLibrary::FInterpTo_Constant(Luminosity, 0.0f, DeltaTime, 2.0f);
			}
		}
		else
		{
			//ULog::Info("laser blocked by: " + GetNameSafe(HitResult.GetActor()) + " | " + GetNameSafe(HitResult.GetComponent()));
			Luminosity = UKismetMathLibrary::FInterpTo_Constant(Luminosity, 0.0f, DeltaTime, 2.0f);
		}
	}

	if (LocalController)
	{
		FVector2D ScreenPosition;
		LocalController->ProjectWorldLocationToScreen(LensFlare->GetComponentLocation(), ScreenPosition);
		FlareMatInstance->SetScalarParameterValue("X", ScreenPosition.X);
		FlareMatInstance->SetScalarParameterValue("Y", ScreenPosition.Y);

		int32 X, Y;
		LocalController->GetViewportSize(X, Y);
		FVector AngleVector;
		AngleVector.X = ScreenPosition.X / X;
		AngleVector.Y = ScreenPosition.Y / Y;
		FVector ScreenCentre;
		ScreenCentre.X = (X / 2.0f) / X;
		ScreenCentre.Y = (Y / 2.0f) / Y;
		  
		FlareMatInstance->SetScalarParameterValue("Angle", UKismetMathLibrary::FindLookAtRotation(AngleVector, ScreenCentre).Yaw);
	}

	FlareMatInstance->SetScalarParameterValue("Brightness", Luminosity);
}

void ALensFlare::InitFlare()
{
	FlareMatInstance = UMaterialInstanceDynamic::Create(FlareMat, this);
	if (!FlareMatInstance)
		return;

	LensFlare->SetMaterial(0, FlareMatInstance);
	LensFlare->Elements[0].BaseSizeX = BaseSize.X;
	LensFlare->Elements[0].BaseSizeY = BaseSize.Y;
	LensFlare->Elements[0].bSizeIsInScreenSpace = false;
	LensFlare->SetVisibility(false);

	// global
	FlareMatInstance->SetScalarParameterValue("Global_Brightness", GlobalBrightness);
	FlareMatInstance->SetScalarParameterValue("SmallFlares_Brightness", GhostsBrightness);
	FlareMatInstance->SetScalarParameterValue("StaticState", FlickerIntensity);
	FlareMatInstance->SetScalarParameterValue("TimeVatiable", FlickerTime);
	FlareMatInstance->SetVectorParameterValue("Global_Color", GlobalColor);
	FlareMatInstance->SetScalarParameterValue("FadeDistance", FadeDistance);
	FlareMatInstance->SetScalarParameterValue("UseFadeDistance", bUseFadeDistance);
	FlareMatInstance->SetScalarParameterValue("IncreasingBloomByDistance", IncreasingBloomByDistance);
	FlareMatInstance->SetScalarParameterValue("UseDistanceBloom", bUseDistanceBloom);

	// halo
	FlareMatInstance->SetScalarParameterValue("Halo_Opacity", HaloOpacity);
	FlareMatInstance->SetScalarParameterValue("Halo_InnerRadius", HaloInnerRadius);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Brightness", HaloBrightness);
	FlareMatInstance->SetVectorParameterValue("Halo_Outer_Color", HaloOuterColor);
	FlareMatInstance->SetVectorParameterValue("Halo_Inner_Color", HaloInnerColor);
	FlareMatInstance->SetTextureParameterValue("T_HaloTexture", HaloTexture);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Size_Reduction", HaloTextureSize);
	FlareMatInstance->SetScalarParameterValue("HaloTexture_Contrast", HaloContrast);

	//centre flare
	FlareMatInstance->SetScalarParameterValue("Centre_Brightness", CentreFlareBrightness);
	FlareMatInstance->SetTextureParameterValue("T_Centre", CentreFlareTexture);
	FlareMatInstance->SetVectorParameterValue("Centre_Color", CentreFlareColor);
	FlareMatInstance->SetScalarParameterValue("Centre_Size_Reduction", CentreFlareSize);
	FlareMatInstance->SetScalarParameterValue("Centre_Contrast", CentreFlareContrast);


	// main flare
	FlareMatInstance->SetScalarParameterValue("MainFlare_Brightness", MainFlareBrightness);
	FlareMatInstance->SetTextureParameterValue("T_Main_Flare", MainFlare);
	FlareMatInstance->SetScalarParameterValue("MainFlare_InnerRadius", MainFlareInnerRadius);
	FlareMatInstance->SetVectorParameterValue("MainLens_Outer_Color", MainOuterColor);
	FlareMatInstance->SetVectorParameterValue("MainLens_Inner_Color", MainLensInnerColor);
	FlareMatInstance->SetScalarParameterValue("Main_Flare_Size_Reduction", MainFlareSize);
	FlareMatInstance->SetScalarParameterValue("MainFlare_Contrast", MainFlareContrast);
	
	// reflection centre
	FlareMatInstance->SetScalarParameterValue("EyeReflection_SizeReduction", ReflectionSize);
	FlareMatInstance->SetTextureParameterValue("T_EyeReflectionCentre", ReflectionCentreTexture);
	FlareMatInstance->SetVectorParameterValue("EyeReflection_Color", ReflectionColor);
	FlareMatInstance->SetScalarParameterValue("ReflectionCentre_Brightness", ReflectionBrightness);
	FlareMatInstance->SetScalarParameterValue("ReflectionContrast", ReflectionContrast);
	
	// opposite flare
	FlareMatInstance->SetScalarParameterValue("FlaresOpposite_ProjectionOnAxis", OppositeFlaresAxisProjection);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite_1_SizeReduction", FlareOppositeSize01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite_2_SizeReduction", FlareOppositeSize02);
	FlareMatInstance->SetVectorParameterValue("FlareOpposite_1_Color", FlareOppositeColor01);
	FlareMatInstance->SetVectorParameterValue("FlareOpposite_2_Color", FlareOppositeColor02);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite1_Brightness", FlareOppositeBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite2_Brightness", FlareOppositeBrightness02);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite1_Contrast", FlareOppositeContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareOpposite2_Contrast", FlareOppositeContrast02);
	FlareMatInstance->SetTextureParameterValue("T_FlareOpposite_1", FlareOpposite01);
	FlareMatInstance->SetTextureParameterValue("T_FlareOpposite_2", FlareOpposite02);
	
	// front flare
	FlareMatInstance->SetScalarParameterValue("FlaresFront_ProjectionOnAxis", FrontFlaresAxisProjection);
	FlareMatInstance->SetScalarParameterValue("FlareFront_1_SizeReduction", FlareFrontSize01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_2_SizeReduction", FlareFrontSize02);
	FlareMatInstance->SetVectorParameterValue("Flare_1_Color", FlareFrontColor01);
	FlareMatInstance->SetVectorParameterValue("Flare_2_Color", FlareFrontColor02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_1_ProximityToCentre", FlareFrontProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_2_ProximityToCentre", FlareFrontProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_FlareFront_1", FlareFront01);
	FlareMatInstance->SetTextureParameterValue("T_FlareFront_2", FlareFront02);
	FlareMatInstance->SetScalarParameterValue("FlareFront1_Contrast", FlareFrontContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareFront2_Contrast", FlareFrontContrast02);
	FlareMatInstance->SetScalarParameterValue("FlareFront1_Brightness", FlareFrontBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareFront2_Brightness", FlareFrontBrightness02);

	// reflectedflares
	FlareMatInstance->SetScalarParameterValue("FlareMirror_1_SizeReduction", ReflectedFlareSize01);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_1_ProximityToCentre", ReflectedFlareProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_2_SizeReduction", ReflectedFlareSize02);
	FlareMatInstance->SetScalarParameterValue("FlareMirror_2_ProximityToCentre", ReflectedFlareProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Mirror_1", ReflectedFlare01);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Mirror_2", ReflectedFlare02);
	FlareMatInstance->SetVectorParameterValue("FlareMirror_1_Color", ReflectedFlareColor01);
	FlareMatInstance->SetVectorParameterValue("FlareMirror_2_Color", ReflectedFlareColor02);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare01Contrast", ReflectedFlareContrast01);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare02Contrast", ReflectedFlareContrast02);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare01Brightness", ReflectedFlareBrightness01);
	FlareMatInstance->SetScalarParameterValue("ReflectedFlare02Brightness", ReflectedFlareBrightness02);
	
	//minor flare
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_ProximityToCentre", MinorFlareProximityCentre01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_SizeReduction", MinorFlareSize01);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_1_Size_Random", MinorFlareSizeRandom01);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Minor_1", MinorFlare01);
	FlareMatInstance->SetVectorParameterValue("FlareMinor_1_Color", MinorFlareColor01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorContrast01", MinorFlareContrast01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorBrightness01", MinorFlareBrightness01);
	FlareMatInstance->SetScalarParameterValue("FlareMinorContrast02", MinorFlareContrast02);
	FlareMatInstance->SetScalarParameterValue("FlareMinorBrightness02", MinorFlareBrightness02);
	FlareMatInstance->SetVectorParameterValue("FlareMinor_2_Color", MinorFlareColor02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_SizeReduction", MinorFlareSize02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_Size_Random", MinorFlareSizeRandom02);
	FlareMatInstance->SetScalarParameterValue("FlareFront_Minor_2_ProximityToCentre", MinorFlareProximityCentre02);
	FlareMatInstance->SetTextureParameterValue("T_Flare_Minor_2", MinorFlare02);
	
	//iris reflection
	FlareMatInstance->SetTextureParameterValue("T_Iris", Iris);
	FlareMatInstance->SetScalarParameterValue("Iris_SizeReduction", IrisSize);
	FlareMatInstance->SetScalarParameterValue("Iris_ProximityToCentre", IrisProximityCentre);
	FlareMatInstance->SetScalarParameterValue("Iris_intensity", IrisBrightness);
	FlareMatInstance->SetScalarParameterValue("Iris_Contrast", IrisContrast);
	FlareMatInstance->SetVectorParameterValue("IrisColor", IrisColor);

	// additional flare
	FlareMatInstance->SetTextureParameterValue("T_AdditionalFlare", AdditionalFlare);
	FlareMatInstance->SetScalarParameterValue("AdditionalFlare_SizeReduction", AdditionalFlareSize);
	FlareMatInstance->SetScalarParameterValue("Additional Flare Contrast", AdditionalFlareContrast);
	FlareMatInstance->SetScalarParameterValue("Additional Flare Brightness", AdditionalFlareBrightness);
	FlareMatInstance->SetVectorParameterValue("AdditionalFlareColor", AdditionalFlareColor);
}

