// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Components/ResourceComponent.h"
#include "ArmourResourceComponent.generated.h"

/**
* A reusable component that can be placed on any actor that requires the concept of armour without having to define and implement one yourself.
* It is also network replicated.
* @author Ali
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta = (BlueprintSpawnableComponent))
class READYORNOT_API UArmourResourceComponent : public UResourceComponent
{
	GENERATED_BODY()

public:
    UArmourResourceComponent();

	// Retreives the resistance percentage
	UFUNCTION(BlueprintPure, Category = "Armour")
    FORCEINLINE float GetResistancePercentage() const { return Resistance; }

	// Retreives the remaining tickets
	UFUNCTION(BlueprintPure, Category = "Armour")
    FORCEINLINE int32 GetRemainingTickets() const { return RemainingTickets; }
	
	// Retreives the max tickets
	UFUNCTION(BlueprintPure, Category = "Armour")
    FORCEINLINE int32 GetMaxTickets() const { return MaxTickets; }

	UFUNCTION(BlueprintCallable, Category = "Armour")
	void SetMaxTickets(int32 NewMax);
	
	UFUNCTION(BlueprintCallable, Category = "Armour")
	void SetResistance(float NewResistancePercentage);

protected:
	void BeginPlay() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	FORCEINLINE float DecreaseResource_Expression_Implementation(const float Amount) const override { return Resource - (Amount - Amount * (Resistance/100)); }

	void Server_IncreaseResource_Implementation(float Amount) override;
	void Server_ResetResource_Implementation() override;
	void Server_DecreaseResource_Implementation(float Amount) override;
	
	// The maximum amount of hits this armour can recieve before damaging the player
    UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Armour", meta = (ClampMin = 0, UIMin = 0))
	int32 MaxTickets = 1;
	
	// The amount of resistance given (in percent) to this armour (e.g. a bullet does +120 damage - 30% resistance = +84 damage to player)
    UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Armour", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 100.0f, UIMax = 100.0f))
    float Resistance = 30.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Armour")
	int32 RemainingTickets = 1;
};
