// Copyright Void Interactive, 2023

#pragma once

#include "Camera/CameraModifier_CameraShake.h"
#include "RONCameraModifier_CameraShake.generated.h"

UCLASS()
class READYORNOT_API URONCameraModifier_CameraShake : public UCameraModifier_CameraShake
{
	GENERATED_BODY()

public:
	UCameraShakeBase* AddCameraShake2(UCameraShakeBase* NewShake, const FAddCameraShakeParams& Params);
};
