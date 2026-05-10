// Copyright Void Interactive, 2021

#pragma once

#include "Components/ActorComponent.h"
#include "ResourceComponent.generated.h"

/**
* A reusable component that can be placed on any actor that requires the concept of resources without having to define and implement one yourself.
* It is also network replicated.
* @author Ali
*/
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class READYORNOT_API UResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceComponent();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFullResourceSignature);
	// When the actor has reached full Resource, only broadcasted once
	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnFullResourceSignature OnFullResource;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLowResourceSignature, float, Resource);
	// When the actor has reached low Resource, only broadcasted once
	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnLowResourceSignature OnLowResource;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDepletedResourceSignature);
	// When the actor has depleted this Resource, only broadcasted once
	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnDepletedResourceSignature OnDepletedResource;

	// Return this component's resource name
	UFUNCTION(BlueprintPure, Category = "Resource")
    FORCEINLINE FName GetResourceName() const { return ResourceName; }

	// Return the actor's current default Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetMaxResource() const { return MaxResource; }

	// Return the actor's original default Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetOriginalMaxResource() const { return OriginalMaxResource; }

	// Return the actor's half Resource value (MaxResource / 2)
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetHalfResource() const { return MaxResource/2; }

	// Return the actor's current Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetCurrentResource() const { return Resource; }

	// Return the actor's low Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
    FORCEINLINE float GetLowResource() const { return LowResource; }
	
	// Return the actor's normalized Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetNormalizedResource() const { return Resource / MaxResource; }

	// Return the actor's previous Resource value
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetPreviousResource() const { return PreviousResource; }

	// Return the actor's low Resource threshold value as percentage
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE float GetLowResourceThreshold() const { return LowResourceThreshold; }

	// Returns true if current Resource is at MaxResource
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsFullResource() const { return Resource >= MaxResource; }
	
	// Returns true if current Resource is at half of MaxResource
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsHalfResource() const { return Resource <= (MaxResource/2); }

	// Returns true if we have any Resources
	UFUNCTION(BlueprintPure, Category = "Resource")
    FORCEINLINE bool HasResource() const { return Resource > 0.0f; }

	// Returns true if current Resource is at or below 0
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsDepleted() const { return Resource <= 0.0f; }

	// Returns true if the actor's Resource is less than or equal to the low Resource threshold
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsLowResource() const { return Resource <= MaxResource * LowResourceThreshold; }

	// Compares the actor's current Resource with the given ResourceValue parameter and returns true if they are equal
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsResourceAt(const float ResourceValue) const { return Resource == ResourceValue; }

	// Returns true if current Resource is less than the given ResourceValue parameter
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsResourceBelow(const float ResourceValue) const { return Resource < ResourceValue; }

	// Returns true if current Resource is greater than the given ResourceValue parameter
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsResourceAbove(const float ResourceValue) const { return Resource > ResourceValue; }

	// Returns true if current Resource is less than or equal to the given ResourceValue parameter
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsResourceAtOrBelow(const float ResourceValue) const { return Resource <= ResourceValue; }

	// Returns true if current Resource is greater than or equal to the given ResourceValue parameter
	UFUNCTION(BlueprintPure, Category = "Resource")
	FORCEINLINE bool IsResourceAtOrAbove(const float ResourceValue) const { return Resource >= ResourceValue; }

	// Returns true if the unlimited resource setting is enabled
	UFUNCTION(BlueprintPure, Category = "Resource")
    FORCEINLINE bool IsUnlimitedResourceEnabled() const { return bUnlimited; }

	// Enables unlimited resources setting
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void EnableUnlimitedResource() { Server_EnableUnlimitedResource_Implementation(); }

	// Disables unlimited resources setting
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void DisableUnlimitedResource() { Server_DisableUnlimitedResource_Implementation(); }
	
	// Toggles unlimited resources setting
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void ToggleUnlimitedResource() { Server_ToggleUnlimitedResource_Implementation(); }
	
	// Set a new default Resource value
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void SetMaxResource(const float NewMaxResource) { Server_SetMaxResource_Implementation(NewMaxResource); }

	// Set a new current Resource value
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void SetResource(const float NewResourceAmount) { Server_SetResource_Implementation(NewResourceAmount); }

	// Set the current Resource value to max Resource
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void SetCurrentResourceToMax() { Server_SetCurrentResourceToMax_Implementation(); }

	// Add Resource by an amount
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void IncreaseResource(const float Amount) { Server_IncreaseResource_Implementation(Amount); }
	
	// Add Resource by Rate * DeltaTime
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void IncreaseResourceByRate(const float Rate) { Server_IncreaseResource_Implementation(Rate * (GetWorld() ? GetWorld()->DeltaTimeSeconds : 1.0f)); }

	// Subtract Resource by an amount
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void DecreaseResource(const float Amount) { Server_DecreaseResource_Implementation(Amount); }
	
	// Subtract Resource by Rate * DeltaTime
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void DecreaseResourceByRate(const float Rate) { Server_DecreaseResource_Implementation(Rate * (GetWorld() ? GetWorld()->DeltaTimeSeconds : 1.0f)); }

	// Resets Resource back to default
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void ResetResource() { Server_ResetResource_Implementation(); }
	
	// Depletes current Resource to 0
	UFUNCTION(BlueprintCallable, Category = "Resource")
    void DepleteResource() { Server_DepleteResource_Implementation(); }
	
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SetUnlimitedResource(const bool bEnabled);

	// Enables unlimited resources setting
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_EnableUnlimitedResource();

	// Disables unlimited resources setting
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_DisableUnlimitedResource();
	
	// Toggles unlimited resources setting
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_ToggleUnlimitedResource();

	// Sets unlimited resources setting to the given parameter value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_SetUnlimitedResource(bool bEnabled);
	
	// Set a new default Resource value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_SetMaxResource(float NewMaxResource);

	// Set a new current Resource value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_SetResource(float NewResourceAmount);

	// Set the current Resource value to max Resource
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_SetCurrentResourceToMax();

	// Update our previous Resource value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_UpdatePreviousResource();

	// Add Resource by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_IncreaseResource(float Amount);

	// Subtract Resource by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_DecreaseResource(float Amount);

	// Resets Resource back to default
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_ResetResource();

	// Depletes current Resource to 0
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
    void Server_DepleteResource();

protected:
	void BeginPlay() override;

	void OnComponentCreated() override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Resource")
	float IncreaseResource_Expression(float Amount) const;
	FORCEINLINE virtual float IncreaseResource_Expression_Implementation(const float Amount) const { return Resource + Amount; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Resource")
    float DecreaseResource_Expression(float Amount) const;
	FORCEINLINE virtual float DecreaseResource_Expression_Implementation(const float Amount) const { return Resource - Amount; }
	
	UFUNCTION(BlueprintCallable, Category = "Resource")
	void UpdatePreviousResource() { Server_UpdatePreviousResource_Implementation(); }
	
	// Initialize the actor's resource values
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Resource")
	void Server_InitResource();
	virtual void Server_InitResource_Implementation();
	virtual bool Server_InitResource_Validate() { return true; }
	
	virtual void Server_EnableUnlimitedResource_Implementation();
	virtual bool Server_EnableUnlimitedResource_Validate() { return true; }
	
	virtual void Server_DisableUnlimitedResource_Implementation();
	virtual bool Server_DisableUnlimitedResource_Validate() { return true; }

	virtual void Server_ToggleUnlimitedResource_Implementation();
	virtual bool Server_ToggleUnlimitedResource_Validate() { return true; }

	virtual void Server_SetUnlimitedResource_Implementation(bool bEnabled);
	virtual bool Server_SetUnlimitedResource_Validate(bool bEnabled) { return true; }

	virtual void Server_SetMaxResource_Implementation(float NewMaxResource);
	virtual bool Server_SetMaxResource_Validate(const float NewMaxResource) { return NewMaxResource <= MaxResourceLimit; }

	virtual void Server_SetResource_Implementation(float NewResourceAmount);
	virtual bool Server_SetResource_Validate(const float NewResourceAmount) { return NewResourceAmount <= MaxResourceLimit; }

	virtual void Server_SetCurrentResourceToMax_Implementation();
	virtual bool Server_SetCurrentResourceToMax_Validate() { return true; }

	virtual void Server_UpdatePreviousResource_Implementation();
	virtual bool Server_UpdatePreviousResource_Validate() { return true; }

	virtual void Server_IncreaseResource_Implementation(float Amount);
	virtual bool Server_IncreaseResource_Validate(const float Amount) { return Amount <= MaxResourceLimit; }

	virtual void Server_DecreaseResource_Implementation(float Amount);
	virtual bool Server_DecreaseResource_Validate(const float Amount) { return true; }

	virtual void Server_ResetResource_Implementation();
	virtual bool Server_ResetResource_Validate() { return true; }

	virtual void Server_DepleteResource_Implementation();
	virtual bool Server_DepleteResource_Validate() { return true; }

	void BroadcastEvents_IncreaseResource();
	void BroadcastEvents_DecreaseResource();
	void BroadcastEvents_SetResource();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Resource")
	FName ResourceName = "Resource-Generic";
	
	// The actor's current resource
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float Resource = 100.0f;

	// The actor's default resource
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxResource = 100.0f;

	// The absolute maximum resource that this actor can change to. (Also to prevent cheating)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxResourceLimit = 1000.0f;

	// The actor's low resource threshold as percentage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
	float LowResourceThreshold = 0.25f;

	// The actor's original default resource
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Limb Resource")
    float OriginalMaxResource = 100.0f;
	
	// The actor's low resource value
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Resource")
	float LowResource = 10.0f;

	// The actor's previous resource before being damaged
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Resource")
	float PreviousResource = 100.0f;

	// Should this resource not be consumed?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	uint8 bUnlimited : 1;

private:
	uint8 bFullResourceEventBroadcasted : 1;
	uint8 bLowResourceEventBroadcasted : 1;
	uint8 bNoResourceEventBroadcasted : 1;
};
