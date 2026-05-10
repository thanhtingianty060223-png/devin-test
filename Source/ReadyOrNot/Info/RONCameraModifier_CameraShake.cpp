// Copyright Void Interactive, 2023

#include "Info/RONCameraModifier_CameraShake.h"

#include "Camera/CameraShakeSourceComponent.h"

UCameraShakeBase* URONCameraModifier_CameraShake::AddCameraShake2(UCameraShakeBase* NewShake, const FAddCameraShakeParams& Params)
{
	if (NewShake != nullptr)
	{
		float Scale = Params.Scale;
		const UCameraShakeSourceComponent* SourceComponent = Params.SourceComponent;
		const bool bIsCustomInitialized = Params.Initializer.IsBound();

		// Adjust for splitscreen
		if (CameraOwner != nullptr && GEngine->IsSplitScreen(CameraOwner->GetWorld()))
		{
			Scale *= SplitScreenShakeScale;
		}

		UClass* ShakeClass = NewShake->GetClass();

		UCameraShakeBase const* const ShakeCDO = GetDefault<UCameraShakeBase>(ShakeClass);
		const bool bIsSingleInstance = ShakeCDO && ShakeCDO->bSingleInstance;
		if (bIsSingleInstance)
		{
			// Look for existing instance of same class
			for (FActiveCameraShakeInfo& ShakeInfo : ActiveShakes)
			{
				UCameraShakeBase* ShakeInst = ShakeInfo.ShakeInstance;
				if (ShakeInst && (ShakeClass == ShakeInst->GetClass()))
				{
					if (!ShakeInfo.bIsCustomInitialized && !bIsCustomInitialized)
					{
						// Just restart the existing shake, possibly at the new location.
						// Warning: if the shake source changes, this would "teleport" the shake, which might create a visual
						// artifact, if the user didn't intend to do this.
						ShakeInfo.ShakeSource = SourceComponent;
						ShakeInst->StartShake(CameraOwner, Scale, Params.PlaySpace, Params.UserPlaySpaceRot);
						return ShakeInst;
					}
					else
					{
						// If either the old or new shake are custom initialized, we can't
						// reliably restart the existing shake and expect it to be the same as what the caller wants. 
						// So we forcibly stop the existing shake immediately and will create a brand new one.
						ShakeInst->StopShake(true);
						// Discard it right away so the spot is free in the active shakes array.
						ShakeInfo.ShakeInstance = nullptr;
					}
				}
			}
		}

		// Try to find a shake in the expired pool
		//UCameraShakeBase* NewInst = ReclaimShakeFromExpiredPool(ShakeClass);
		UCameraShakeBase* NewInst = NewShake;
		
		// Custom initialization if necessary.
		if (bIsCustomInitialized)
		{
			Params.Initializer.Execute(NewInst);
		}

		// Initialize new shake and add it to the list of active shakes
		NewInst->StartShake(CameraOwner, Scale, Params.PlaySpace, Params.UserPlaySpaceRot);

		// Look for nulls in the array to replace first -- keeps the array compact
		bool bReplacedNull = false;
		for (int32 Idx = 0; Idx < ActiveShakes.Num(); ++Idx)
		{
			FActiveCameraShakeInfo& ShakeInfo = ActiveShakes[Idx];
			if (ShakeInfo.ShakeInstance == nullptr)
			{
				ShakeInfo.ShakeInstance = NewInst;
				ShakeInfo.ShakeSource = SourceComponent;
				ShakeInfo.bIsCustomInitialized = bIsCustomInitialized;
				bReplacedNull = true;
			}
		}

		// no holes, extend the array
		if (bReplacedNull == false)
		{
			FActiveCameraShakeInfo ShakeInfo;
			ShakeInfo.ShakeInstance = NewInst;
			ShakeInfo.ShakeSource = SourceComponent;
			ShakeInfo.bIsCustomInitialized = bIsCustomInitialized;
			ActiveShakes.Emplace(ShakeInfo);
		}

		return NewInst;
	}

	return nullptr;
}
