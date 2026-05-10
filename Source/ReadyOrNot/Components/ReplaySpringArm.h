// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReplaySpringArm.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UReplaySpringArm : public USpringArmComponent
{
	GENERATED_BODY()

private:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;
protected:
public:
};
