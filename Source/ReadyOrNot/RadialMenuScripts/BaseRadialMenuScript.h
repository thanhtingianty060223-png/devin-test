// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "BaseRadialMenuScript.generated.h"

/**
 * A base class for executing code on a radial menu selection
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UBaseRadialMenuScript : public UObject
{
	GENERATED_BODY()

public:
	UBaseRadialMenuScript(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Radial Menu Script")
	void Initialize(class URadialWidgetBase* InRadialMenuOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Menu Script")
	void ExecuteScript();

protected:
	virtual void ExecuteScript_Implementation();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Menu Script")
	TSoftObjectPtr<UTexture2D> RadialMenuIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu Script")
	class URadialWidgetBase* RadialMenuOwner;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu Script")
	class AActor* Actor;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Menu Script")
	class UWorld* World;
};
