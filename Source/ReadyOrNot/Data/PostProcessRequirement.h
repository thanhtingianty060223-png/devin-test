// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "PostProcessRequirement.generated.h"

/**
 * A base class for implementing a requirement to be used by the PlayerPostProcessing component
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UPostProcessRequirement : public UObject
{
	GENERATED_BODY()

public:
	UPostProcessRequirement();

	UFUNCTION(BlueprintCallable, Category = "Post Process Requirement")
	void Initialize(class APlayerCharacter* InPlayerCharacter, AActor* InDamageCauser);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Post Process Requirement")
	bool EnablePostProcessEffect();
	
	UFUNCTION(BlueprintPure, Category = "Post Process Requirement")
	FORCEINLINE class APlayerCharacter* GetPlayerCharacter() const { return PlayerCharacter; }
	
	UFUNCTION(BlueprintPure, Category = "Post Process Requirement")
	FORCEINLINE AActor* GetDamageCauser() const { return DamageCauser; }

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Post Process Requirement")
	class APlayerCharacter* PlayerCharacter;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Post Process Requirement")
	AActor* DamageCauser;

	virtual bool EnablePostProcessEffect_Implementation();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
};

/**
 * 
 */
UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UPPR_IsDamageCauserOnScreen final : public UPostProcessRequirement
{
	GENERATED_BODY()

protected:
	bool EnablePostProcessEffect_Implementation() override;
};