// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseTriggerable.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class READYORNOT_API ABaseTriggerable : public AActor
{
	GENERATED_BODY()

public:
	ABaseTriggerable();

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Triggerable")
	USceneComponent* SceneComponent;

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Triggerable")
	void Activate();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Triggerable")
	void Deactivate();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Triggerable")
	void Stop();

	/** Should this actor be activated or deactivated on triggered? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "On Triggered")
	bool bActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "On Triggered")
	float ActivateDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "On Triggered")
	float DeactivateDelay = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bIsActive = false;
};
