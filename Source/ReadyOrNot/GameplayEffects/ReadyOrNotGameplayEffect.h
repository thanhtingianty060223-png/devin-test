// Copyright Void Interactive, 2021

#pragma once

#include "UObject/NoExportTypes.h"
#include "ReadyOrNotGameplayEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoNGameplayEffectExpired, TSubclassOf<class UReadyOrNotGameplayEffect>, InGameplayEffectClass);

/**
 * A base class for applying a gameplay effect to a specific actor.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UReadyOrNotGameplayEffect : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RON Gameplay Effect")
	void Initialize(class AActor* InActor);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RON Gameplay Effect")
	void ApplyEffect();

	UFUNCTION(BlueprintCallable, Category = "RON Gameplay Effect")
	void ApplyEffectFor(float Seconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RON Gameplay Effect")
	void ResetEffect();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RON Gameplay Effect")
	void Server_Initialize(class AActor* InActor, bool bMulticast = false);
	virtual void Server_Initialize_Implementation(class AActor* InActor, const bool bMulticast = false) { bMulticast ? Multicast_Initialize(InActor) : Multicast_Initialize_Implementation(InActor); }
	virtual bool Server_Initialize_Validate(class AActor* InActor, bool bMulticast = false) { return true; }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "RON Gameplay Effect")
	void Multicast_Initialize(class AActor* InActor);
	virtual void Multicast_Initialize_Implementation(class AActor* InActor);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RON Gameplay Effect")
	void Server_ApplyEffect(bool bMulticast = false);
	virtual void Server_ApplyEffect_Implementation(const bool bMulticast = false) { bMulticast ? Multicast_ApplyEffect() : Multicast_ApplyEffect_Implementation(); }
	virtual bool Server_ApplyEffect_Validate(bool bMulticast = false) { return true; }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "RON Gameplay Effect")
	void Multicast_ApplyEffect();
	virtual void Multicast_ApplyEffect_Implementation();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RON Gameplay Effect")
	void Server_ApplyEffectFor(float Seconds, bool bMulticast = false);
	virtual void Server_ApplyEffectFor_Implementation(const float Seconds, const bool bMulticast = false) { bMulticast ? Multicast_ApplyEffectFor(Seconds) : Multicast_ApplyEffectFor_Implementation(Seconds); }
	virtual bool Server_ApplyEffectFor_Validate(float Seconds, bool bMulticast = false) { return true; }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "RON Gameplay Effect")
	void Multicast_ApplyEffectFor(float Seconds);
	virtual void Multicast_ApplyEffectFor_Implementation(float Seconds);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RON Gameplay Effect")
	void Server_ResetEffect(bool bMulticast = false);
	virtual void Server_ResetEffect_Implementation(const bool bMulticast = false) { bMulticast ? Multicast_ResetEffect() : Multicast_ResetEffect_Implementation(); }
	virtual bool Server_ResetEffect_Validate(bool bMulticast = false) { return true; }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "RON Gameplay Effect")
	void Multicast_ResetEffect();
	virtual void Multicast_ResetEffect_Implementation();

	UPROPERTY(BlueprintAssignable, Category = "RON Gameplay Effect")
	FOnRoNGameplayEffectExpired OnGameplayEffectExpired;
	
protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RON Gameplay Effect")
	void OnEffectExpired();

	virtual UWorld* GetWorld() const override;

	virtual void Initialize_Implementation(class AActor* InActor);
	virtual void ApplyEffect_Implementation();
	virtual void ResetEffect_Implementation();
	virtual void OnEffectExpired_Implementation();

	UPROPERTY(BlueprintReadOnly, Category = "RON Gameplay Effect")
	class AActor* Actor;

	UPROPERTY(BlueprintReadOnly, Category = "RON Gameplay Effect")
	class UWorld* World;
	
	FTimerHandle TH_EffectExpiry;
	FTimerManager* TimerManager;
};
