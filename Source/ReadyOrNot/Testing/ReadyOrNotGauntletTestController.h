// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GauntletTestController.h"
#include "ReadyOrNotGauntletTestController.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotGauntletTestController : public UGauntletTestController
{
	GENERATED_BODY()
	
	 
private:
    // Time to wait after game start before doing anything.
    const float SpinUpTime = 3.f;

    float TotalTestTime = 0.0f;

    bool bStartedTesting = false;
    // Time to run the profiler for.
    const float ProfilingTime = 16.f;
 
    UFUNCTION()
    void StartTesting();
 
    void StartProfiling();
 
    UFUNCTION()
    void StopProfiling();
 
    void StopTesting();
 
protected:
    virtual void OnInit() override;
    virtual void OnTick(float DeltaTime) override;
    
};
