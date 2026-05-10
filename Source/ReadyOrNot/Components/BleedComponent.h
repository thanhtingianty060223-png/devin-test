// Void Interactive, 2020

#pragma once

#include "Components/ActorComponent.h"
#include "BleedComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UBleedComponent : public UActorComponent
{
	GENERATED_BODY()

	UBleedComponent();

	virtual void DestroyComponent(bool bPromoteChildren) override;

public:	
	// Called from notify
	void CompleteHeal();

	UFUNCTION(BlueprintPure)
	bool CanHeal() const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetHealCount() const { return HealCount; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetBleedTime() const { return BleedTime; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsBleeding() const { return bIsBleeding; }
	
	UFUNCTION(BlueprintCallable)
	void StartBleeding();
	
	UFUNCTION(BlueprintCallable)
	void StopBleeding();

	void PrepareHeal();

	UPROPERTY(EditAnywhere, Category="Collection")
	UFMODEvent* BleedEvent;

	bool HasStartedHealing() const { return bHasStartedHealing; }
	
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class APlayerCharacter* GetOwningCharacter() const;

	void SetBreathingCompBleedEvent(bool bBleed);

private:
	UPROPERTY(Replicated)
	bool bIsBleeding = false;

	float BleedTime = 0.0f;

	// temporarily stop bleeding while starting heal
	UPROPERTY(Replicated)
	bool bTempStopBleeding = false;

	bool bHasStartedHealing = false;
	bool bHasStartedPostProcess = false;
	bool bShownActionPrompt = false;

	// Heal count 0 = stop bleeding return to 100% health
	// Heal count 1 = stop bleeding return to 50% health
	// Heal count 1+ = stop bleeding
	UPROPERTY(Replicated)
	int32 HealCount = 3;
	
	UFUNCTION()
	void DoHeal();

	FTimerHandle TH_DoHeal;
};
