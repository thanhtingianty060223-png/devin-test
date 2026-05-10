// Copyright Void Interactive, 2023

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "ReadyOrNotPlayerCameraManager.generated.h"

UCLASS()
class READYORNOT_API AReadyOrNotPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AReadyOrNotPlayerCameraManager();
	
	UCameraShakeBase* StartCameraShake2(UCameraShakeBase* Shake, float Scale=1.f, ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	
	static void PlayWorldCameraShake(UWorld* InWorld, class UCameraShakeBase* Shake, FVector Epicenter, float InnerRadius, float OuterRadius, float Falloff = 1.f, bool bOrientShakeTowardsEpicenter = false);
	
protected:
	virtual void PostInitializeComponents() override;
};
