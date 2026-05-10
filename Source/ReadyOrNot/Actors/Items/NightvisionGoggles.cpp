// Copyright Void Interactive, 2022

#include "NightvisionGoggles.h"
#include "Components/PlayerPostProcessing.h"

void ANightvisionGoggles::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANightvisionGoggles, bNVGOn);
	DOREPLIFETIME(ANightvisionGoggles, bTogglingNVG);
}

ANightvisionGoggles::ANightvisionGoggles()
{
	bShowStaticMeshOnBody = false;
	bShouldTickAnimBPWhenNotEquipped = true;
	bDisableTickWhenNotEquipped = false;

	// TODO: remove this, use array instead
	static ConstructorHelpers::FObjectFinder<UTexture2D> GreenLUT(TEXT("Texture2D'/Game/ReadyOrNot/Assets/Lighting/LUTs/NVG/LUT_NVG_Green_004.LUT_NVG_Green_004'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> WhiteLUT(TEXT("Texture2D'/Game/ReadyOrNot/Assets/Lighting/LUTs/NVG/LUT_NVG_White_002.LUT_NVG_White_002'"));

	Green_LUT = GreenLUT.Object;
	White_LUT = WhiteLUT.Object;
}

void ANightvisionGoggles::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (AReadyOrNotCharacter* Character = GetOwnerCharacter())
	{
		GetItemMesh()->VisibilityBasedAnimTickOption = bTogglingNVG ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		bNVGOn = Character->bNVGOn;

		// FIXME: do something smarter with this
		for (int32 i = 0; i < BlockDamageTypesWhileActive.Num(); i++)
		{
			if (bNVGOn)
			{
				BlockAnyDamageFrom.AddUnique(BlockDamageTypesWhileActive[i]);
			}
			else
			{
				BlockAnyDamageFrom.Remove(BlockDamageTypesWhileActive[i]);
			}
		}

		// Update nvg color style
		if (APlayerCharacter* pc = GetOwnerPlayerCharacter())
		{
			if (bNVGOn)
			{
				ENVGStyle LoadedStyle = ENVGStyle::GreenPhosphor;
				UBpGameplayHelperLib::LoadNVGStyle(LoadedStyle);

				switch (LoadedStyle)
				{
					case ENVGStyle::GreenPhosphor:
						pc->GetPlayerPostProcessing()->Settings.ColorGradingLUT = Green_LUT;
					break;

					case ENVGStyle::WhitePhosphor:
						pc->GetPlayerPostProcessing()->Settings.ColorGradingLUT = White_LUT;
					break;
				}
			}
		}
	}
}

void ANightvisionGoggles::UpdateNVGPostProcess()
{
	if (const APlayerCharacter* pc = GetOwnerPlayerCharacter())
	{
		const bool bIsOn = pc->bNVGOn;
		
		NightVisionPostProcess.bOverride_ColorGradingLUT = bIsOn;
		NightVisionPostProcess.bOverride_ColorGradingIntensity = bIsOn;
		NightVisionPostProcess.ColorGradingIntensity = bIsOn ? 1.0f : 0.0f;

		if (bIsOn)
		{
			ENVGStyle LoadedStyle = ENVGStyle::GreenPhosphor;
			UBpGameplayHelperLib::LoadNVGStyle(LoadedStyle);

			switch (LoadedStyle)
			{
				case ENVGStyle::GreenPhosphor:
					NightVisionPostProcess.ColorGradingLUT = Green_LUT;
				break;

				case ENVGStyle::WhitePhosphor:
					NightVisionPostProcess.ColorGradingLUT = White_LUT;
				break;
			}
		}
	}
}

void ANightvisionGoggles::SetNightvisionGlobalMaterialParameters(bool bEnabled)
{
	if (!GetWorld() || !GlobalMaterialParameters)
		return;

	UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(GlobalMaterialParameters);
	if (!Instance)
		return;

	Instance->SetScalarParameterValue(NVGGlobalParameterName, bEnabled ? 1.0f : 0.0f);
}

void ANightvisionGoggles::SpawnNightvisionWidget()
{
	DestroyNightvisionWidget();

	SpawnedWidget = CreateWidget<UUserWidget>(UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()), NightVisionFirstPersonWidget);
	if (SpawnedWidget)
	{
		SpawnedWidget->AddToViewport(-1);
	}
	OnNightvisionActivated();
}

void ANightvisionGoggles::DestroyNightvisionWidget()
{
	if (SpawnedWidget)
	{
		if (SpawnedWidget->IsValidLowLevel())
		{
			SpawnedWidget->RemoveFromParent();
		}
		SpawnedWidget = nullptr;
	}
	OnNightvisionDeactivated();
}

bool ANightvisionGoggles::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (bTogglingNVG)
	{
		return true;
	}

	return Super::IsBlockingAnimationPlaying(Exclusions);
}