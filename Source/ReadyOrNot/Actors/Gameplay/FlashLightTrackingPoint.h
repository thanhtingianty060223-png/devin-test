// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "FlashLightTrackingPoint.generated.h"

UCLASS()
class READYORNOT_API AFlashLightTrackingPoint : public AActor, public IReceiveAISenseUpdates
{
	GENERATED_BODY()
	
public:	
	AFlashLightTrackingPoint();

	void ToggleTrackingPoint(bool bOn);

	FORCEINLINE bool IsPrimary() const { return bIsPrimary; }
	FORCEINLINE bool IsActive() const { return bIsActive; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Status")
	uint8 bIsPrimary : 1;

	AReadyOrNotCharacter* GetOwnerCharacter() const;

	void SetupTrackingPoint();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Component")
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	TArray<class ACyberneticController*> SensedByControllers;

private:
	bool bIsActive = false;
};
