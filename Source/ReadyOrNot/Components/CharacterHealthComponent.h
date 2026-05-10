// Copyright Void Interactive, 2021

#pragma once

#include "Components/HealthComponent.h"
#include "CharacterHealthComponent.generated.h"

UENUM(BlueprintType)
enum class EPlayerHealthStatus : uint8
{
	HS_Healthy,
	HS_Injured,
	HS_Downed,
	HS_Incapacitated,
	HS_Dead,
	HS_Arrested,
	HS_NotAvailable,
};
static FText PlayerHealthStatusToString(const EPlayerHealthStatus& InPlayerHealthStatus)
{
	switch (InPlayerHealthStatus)
	{
		case EPlayerHealthStatus::HS_Healthy:			return FText::FromStringTable("SwatCommandTable", "Healthy");
		case EPlayerHealthStatus::HS_Injured:			return FText::FromStringTable("SwatCommandTable", "Injured");
		case EPlayerHealthStatus::HS_Downed:			return FText::FromStringTable("SwatCommandTable", "Downed");
		case EPlayerHealthStatus::HS_Incapacitated:		return FText::FromStringTable("SwatCommandTable", "Incapacitated");
		case EPlayerHealthStatus::HS_Dead:				return FText::FromStringTable("SwatCommandTable", "Dead");
		case EPlayerHealthStatus::HS_Arrested:			return FText::FromStringTable("SwatCommandTable", "Arrested");
		case EPlayerHealthStatus::HS_NotAvailable:		return FText::FromStringTable("SwatCommandTable", "Unavailable");
		default:										return FText::FromStringTable("SwatCommandTable", "None");
	}
}

USTRUCT(BlueprintType)
struct FLimbHealthData
{
	GENERATED_BODY()

	FLimbHealthData();

	FORCEINLINE float GetCurrentHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetMaxHealthLimit() const { return MaxHealthLimit; }
	FORCEINLINE float GetOriginalMaxHealth() const { return OriginalMaxHealth; }
	FORCEINLINE float GetHalfHealth() const { return MaxHealth/2; }
	FORCEINLINE float GetLowHealth() const { return LowHealth; }
	FORCEINLINE float GetLowHealthThreshold() const { return LowHealthThreshold; }
	FORCEINLINE float GetPreviousHealth() const { return PreviousHealth; }
	FORCEINLINE float GetLimbDamageMultiplier() const { return LimbDamageMultiplier; }
	FORCEINLINE int32 GetMaxLimbHealthHalving() const { return MaxLimbHealthHalving; }
	FORCEINLINE int32 GetMaxTickets() const { return MaxTickets; }
	FORCEINLINE int32 GetRemainingTickets() const { return Tickets; }

	// Returns true if current health is at maximum health
    FORCEINLINE bool IsFullHealth() const { return Health >= MaxHealth; }

	// Returns true if current health is at or below 0
    FORCEINLINE bool IsHealthDepleted() const { return Health <= 0.0f; }

	// Returns true if current health is less than or equal to the low health threshold
    FORCEINLINE bool IsLowHealth() const { return Health <= MaxHealth * LowHealthThreshold; }

	// Compares the current health with the given HealthValue parameter and returns true if they are equal
    FORCEINLINE bool IsHealthAt(const float HealthValue) const { return Health == HealthValue; }
	
	// Returns true if current health is less than the given HealthValue parameter
    FORCEINLINE bool IsHealthBelow(const float HealthValue) const { return Health < HealthValue; }
	
	// Returns true if current health is greater than the given HealthValue parameter
    FORCEINLINE bool IsHealthAbove(const float HealthValue) const { return Health > HealthValue; }

	// Returns true if current health is less than or equal to the given HealthValue parameter
    FORCEINLINE bool IsHealthAtOrBelow(const float HealthValue) const { return Health <= HealthValue; }
	
	// Returns true if current health is greater than or equal to the given HealthValue parameter
    FORCEINLINE bool IsHealthAtOrAbove(const float HealthValue) const { return Health >= HealthValue; }

	// Returns true if we have no remaining tickets
	FORCEINLINE bool HasNoTickets() const { return Tickets <= 0; }
	
	// Returns true if we have not lost any tickets
	FORCEINLINE bool HasMaxTickets() const { return Tickets == MaxTickets; }

	// Returns true if the this limb is equal an invalid one
	bool IsInvalid() const;
	
	// Returns true if the other limb is equal to ours
	bool Equals(const FLimbHealthData& OtherLimb) const;

	void SetHealthToMax();
	void SetHealth(float Amount);
	void IncreaseHealth(float Amount);
	void DecreaseHealth(float Amount);
	void ResetHealth();

	void SetMaxHealth(float NewMaxHealth);
	void UpdatePreviousHealth();

	void IncreaseTickets(int32 Amount);
	void DecreaseTickets(int32 Amount);
	void EmptyRemainingTickets();
	void ResetTickets();

	int32 CurrentLimbHealthHalvings = 0;

	uint8 bHasFullHealthEventBeenBroadcasted : 1;
	uint8 bHasLowHealthEventBeenBroadcasted : 1;
	uint8 bHasNoHealthEventBeenBroadcasted : 1;
	uint8 bHasOnBrokenEventBeenBroadcasted : 1;
	
	uint8 bHasOnFullTicketsEventBeenBroadcasted : 1;
	uint8 bHasOnNoTicketsRemainingEventBeenBroadcasted : 1;

	static FLimbHealthData Invalid; 

protected:
	// The limb's current health
	UPROPERTY(BlueprintReadOnly, Category = "Limb Health")
	float Health = 100.0f;

	// The limb's default health
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHealth = 100.0f;

	// The absolute maximum health that this limb can change to. (Also to prevent cheating)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 0.0f, UIMin = 0.0f))
    float MaxHealthLimit = 1000.0f;

	// The limb's low health threshold as percentage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
    float LowHealthThreshold = 0.25f;

	// When decreasing health on this limb, multiply the amount to decrease by this value.
	// (Basically, the strength of this limb, how much damage can it withstand. lower values = less damage to apply, higher values = more damage to apply)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 10.0f))
    float LimbDamageMultiplier = 1.25f;

	// The amount of times this limb can get shot before being broken
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 1, UIMin = 1))
    int32 MaxTickets = 4;
	
	// The amount of times this limb can get shot before being broken
	UPROPERTY(BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 1, UIMin = 1))
    int32 Tickets = MaxTickets;
	
	// When halving a limb's max health and setting it as the new max health, how many times should the halving be done? Default is 3 times. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Limb Health", meta = (ClampMin = 1, UIMin = 1))
	int32 MaxLimbHealthHalving = 3;

	// The limb's original default health
	UPROPERTY(BlueprintReadOnly, Category = "Limb Health")
    float OriginalMaxHealth = MaxHealth;
    
	// The limb's low health value
	UPROPERTY(BlueprintReadOnly, Category = "Limb Health")
    float LowHealth = 0.0f;

	// The limb's previous health before being damaged
	UPROPERTY(BlueprintReadOnly, Category = "Limb Health")
	float PreviousHealth = 0.0f;
};

UENUM(BlueprintType)
enum class ELimbType : uint8
{
	LT_None			UMETA(DisplayName="None"),
	LT_RightLeg		UMETA(DisplayName="Right Leg"),
	LT_LeftLeg		UMETA(DisplayName="Left Leg"),
	LT_RightArm		UMETA(DisplayName="Right Arm"),
	LT_LeftArm		UMETA(DisplayName="Left Arm"),
	LT_Head			UMETA(DisplayName="Head"),
};

static FString LimbTypeToString(const ELimbType& Limb)
{
	switch (Limb)
	{
		case ELimbType::LT_None:
		return FString("None");

		case ELimbType::LT_RightLeg:
		return FString("Right Leg");

		case ELimbType::LT_LeftLeg:
		return FString("Left Leg");

		case ELimbType::LT_RightArm:
		return FString("Right Arm");

		case ELimbType::LT_LeftArm:
		return FString("Left Arm");

		case ELimbType::LT_Head:
		return FString("Head");
		
		default:
		return FString("None");
	}
}

/**
 * A health component that can be added to a character that is controlled in-game
 * @author Ali
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UCharacterHealthComponent : public UHealthComponent
{
	GENERATED_BODY()

public:
	UCharacterHealthComponent();

	void OnPawnLeavingGame();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimbFullHealthSignature, ELimbType, Limb);
	// Called when a limb has reached full health (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbFullHealthSignature OnLimbFullHealth;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLimbLowHealthSignature, ELimbType, AffectedLimb, float, LimbHealth);
	// Called when a limb has reached low health (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbLowHealthSignature OnLimbLowHealth;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimbNoHealthSignature, ELimbType, Limb);
	// Called when a limb has reached no health (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbNoHealthSignature OnLimbNoHealth;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimbBrokenSignature, ELimbType, Limb);
	// Called when a limb has broken (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbBrokenSignature OnLimbBroken;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimbFullTicketsSignature, ELimbType, Limb);
	// Called when a limb has regained all their tickets (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbFullTicketsSignature OnLimbFullTickets;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLimbNoTicketsRemainingSignature, ELimbType, Limb);
	// Called when a limb has 0 remaining tickets (Only broadcasted once)
	UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnLimbNoTicketsRemainingSignature OnLimbNoTickets;
	
	UFUNCTION(BlueprintCallable, Category = "Character Health")
	FORCEINLINE EPlayerHealthStatus GetHealthStatus() const { return HealthStatus; }

	// Retrieves the right leg limb data
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Right Leg Health"))
	FORCEINLINE FLimbHealthData GetRightLegHealth_Copy() const { return RightLeg; }

	// Retrieves the left leg limb data
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Left Leg Health"))
    FORCEINLINE FLimbHealthData GetLeftLegHealth_Copy() const { return LeftLeg; }

	// Retrieves the left arm limb data
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Left Arm Health"))
	FORCEINLINE FLimbHealthData GetLeftArmHealth_Copy() const { return LeftArm; }

	// Retrieves the right arm limb data
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Right Arm Health"))
    FORCEINLINE FLimbHealthData GetRightArmHealth_Copy() const { return RightArm; }

	// Retrieves the head limb data
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Head Health"))
    FORCEINLINE FLimbHealthData GetHeadHealth_Copy() const { return Head; }

	// Retrieves the right leg limb data (read-only)
	FORCEINLINE const FLimbHealthData& GetRightLegHealth_Const() const { return RightLeg; }

	// Retrieves the left leg limb data (read-only)
	FORCEINLINE const FLimbHealthData& GetLeftLegHealth_Const() { return LeftLeg; }

	// Retrieves the left arm limb data (read-only)
	FORCEINLINE const FLimbHealthData& GetLeftArmHealth_Const() { return LeftArm; }

	// Retrieves the right arm limb data (read-only)
	FORCEINLINE const FLimbHealthData& GetRightArmHealth_Const() { return RightArm; }

	// Retrieves the head limb data (read-only)
	FORCEINLINE const FLimbHealthData& GetHeadHealth_Const() { return Head; }

	// Retrieves the right leg limb data
    FORCEINLINE FLimbHealthData& GetRightLegHealth() { return RightLeg; }

	// Retrieves the left leg limb data
    FORCEINLINE FLimbHealthData& GetLeftLegHealth() { return LeftLeg; }

	// Retrieves the left arm limb data
    FORCEINLINE FLimbHealthData& GetLeftArmHealth() { return LeftArm; }

	// Retrieves the right arm limb data
    FORCEINLINE FLimbHealthData& GetRightArmHealth() { return RightArm; }

	// Retrieves the head limb data
    FORCEINLINE FLimbHealthData& GetHeadHealth() { return Head; }

	// Retrieves the specified limb data (read-only)
    const FLimbHealthData& GetLimb_Const(const ELimbType& Limb) const;
	
	// Retrieves the specified limb data (returns a reference)
    FLimbHealthData& GetLimb(const ELimbType& Limb);

	// Retrieves the specified limb data (returns a copy)
	UFUNCTION(BlueprintPure, Category = "Character Health | Limbs", meta = (DisplayName = "Get Limb"))
    FLimbHealthData GetLimb_Copy(const ELimbType& Limb) const;

	// Returns true if a limb data is equal to another
	UFUNCTION(BlueprintPure, Category = "Health")
	static FORCEINLINE bool IsLimbEqualTo(const FLimbHealthData& InLimbHealthData, const FLimbHealthData& OtherLimbHealthData) { return InLimbHealthData.Equals(OtherLimbHealthData); }

	// Returns true if a limb's remaining tickets is 0
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbBroken(const ELimbType& Limb) const;

	// Returns all limb's whos tickets are depleted
	UFUNCTION(BlueprintPure, Category = "Health")
    TArray<ELimbType> GetBrokenLimbs() const;

	// Returns true if a limb's current health is at maximum health
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbFullHealth(const ELimbType& Limb) const;

	// Returns true if a limb's current health is at or below 0
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbNoHealth(const ELimbType& Limb) const;

	// Returns true if a limb's health is less than or equal to the low health threshold
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbLowHealth(const ELimbType& Limb) const;

	// Compares a limb's current health with the given HealthValue parameter and returns true if they are equal
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbHealthAt(const ELimbType& Limb, float HealthValue) const;
	
	// Returns true if a limb's current health is less than the given HealthValue
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbHealthBelow(const ELimbType& Limb, float HealthValue) const;
	
	// Returns true if a limb's current health is greater than the given HealthValue
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbHealthAbove(const ELimbType& Limb, float HealthValue) const;

	// Returns true if a limb's current health is less than or equal to the given HealthValue
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbHealthAtOrBelow(const ELimbType& Limb, float HealthValue) const;
	
	// Returns true if a limb's current health is greater than or equal to the given HealthValue
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsLimbHealthAtOrAbove(const ELimbType& Limb, float HealthValue) const;

	// Returns true if any limb's health is below full health. Also returns the first limb found which is below full health
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAnyLimbBelowFullHealth(ELimbType& OutLimbType) const;

	// Returns true if any limb's health is at 0. Also returns the first limb found which is at no health
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAnyLimbAtNoHealth(ELimbType& OutLimbType) const;

	// Returns true if any limb's remaining tickets is at 0. Also returns the first limb found which does not have any tickets left.
	UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAnyLimbBroken(ELimbType& OutLimbType) const;

	// Set a new default limb health value
	UFUNCTION(BlueprintCallable, Category = "Health")
    void SetMaxLimbHealth(const ELimbType& Limb, const float NewMaxHealth) { Server_SetMaxLimbHealth_Implementation(Limb, NewMaxHealth); }

	// Set a new default limb health value
	UFUNCTION(BlueprintCallable, Category = "Health")
    bool HalfMaxLimbHealth(const ELimbType& Limb);
	
	// Set a new current limb health value
	UFUNCTION(BlueprintCallable, Category = "Health")
    void SetLimbHealth(const ELimbType& Limb, const float NewHealthAmount) { Server_SetLimbHealth_Implementation(Limb, NewHealthAmount); }
	
	// Sets the current limb health value to the limb's max health
	UFUNCTION(BlueprintCallable, Category = "Health")
    void SetCurrentLimbHealthToMax(const ELimbType& Limb) { Server_SetCurrentLimbHealthToMax_Implementation(Limb); }

	// Update the limb's previous health value
	UFUNCTION(BlueprintCallable, Category = "Health")
    void UpdatePreviousLimbHealth(const ELimbType& Limb) { Server_UpdatePreviousLimbHealth_Implementation(Limb); }

	// Add health to a limb by an amount
	UFUNCTION(BlueprintCallable, Category = "Health")
    void IncreaseLimbHealth(const ELimbType& Limb, const float Amount) { Server_IncreaseLimbHealth_Implementation(Limb, Amount); }

	// Subtract health to a limb by an amount
	UFUNCTION(BlueprintCallable, Category = "Health")
    void DecreaseLimbHealth(const ELimbType& Limb, const float Amount) { Server_DecreaseLimbHealth_Implementation(Limb, Amount); }

	// Resets a limb's health back to default
	UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetLimbHealth(const ELimbType& Limb) { Server_ResetLimbHealth_Implementation(Limb); }

	// Add tickets to a limb by an amount
	UFUNCTION(BlueprintCallable, Category = "Health")
    void IncreaseLimbTickets(const ELimbType& Limb, const int32 Amount) { Server_IncreaseLimbTickets_Implementation(Limb, Amount); }
	
	// Subtract tickets to a limb by an amount
	UFUNCTION(BlueprintCallable, Category = "Health")
    void DecreaseLimbTickets(const ELimbType& Limb, const int32 Amount) { Server_DecreaseLimbTickets_Implementation(Limb, Amount); }

	// Subtract all tickets to a limb
	UFUNCTION(BlueprintCallable, Category = "Health")
    void UseAllLimbTickets(const ELimbType& Limb) { Server_UseAllRemainingLimbTickets_Implementation(Limb); }

	// Resets limb tickets to maximum
	UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetLimbTickets(const ELimbType& Limb) { Server_ResetLimbTickets_Implementation(Limb); }
	
	UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetAllLimbHealth() { Server_ResetAllLimbHealth_Implementation(); }

	UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetAllLimbTickets() { Server_ResetAllLimbTickets_Implementation(); }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void IncreaseRevive() { Server_IncreaseRevive_Implementation(); }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void DecreaseRevive() { Server_DecreaseRevive_Implementation(); }

	UFUNCTION(BlueprintCallable, Category = "Health")
    void SetRemainingRevives(const int32 NewRemainingRevives) { Server_SetRemainingRevives_Implementation(NewRemainingRevives); }
	
	UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetRevives() { Server_ResetRevives_Implementation(); }
    
    UFUNCTION(BlueprintCallable, Category = "Health")
    void IncreaseReviveHealth(const float Amount) { Server_IncreaseReviveHealth_Implementation(Amount); }

    UFUNCTION(BlueprintCallable, Category = "Health")
    void DecreaseReviveHealth(const float Amount) { Server_DecreaseReviveHealth_Implementation(Amount); }

    UFUNCTION(BlueprintCallable, Category = "Health")
    void SetReviveHealth(const float NewReviveHealth) { Server_SetReviveHealth_Implementation(NewReviveHealth); }
    
    UFUNCTION(BlueprintCallable, Category = "Health")
    void ResetReviveHealth() { Server_ResetReviveHealth_Implementation(); }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthStatus(EPlayerHealthStatus NewHealthStatus);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Health")
	void Server_SetHealthStatus(EPlayerHealthStatus NewHealthStatus);

	// Set a new default limb health value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_SetMaxLimbHealth(const ELimbType& Limb, float NewMaxHealth);
	
	// Sets a limb's max health value to half of it's max health
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_HalfMaxLimbHealth(const ELimbType& Limb);

	// Set a new current limb health value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_SetLimbHealth(const ELimbType& Limb, float NewHealthAmount);
    
	// Sets the current limb health value to the limb's max health
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_SetCurrentLimbHealthToMax(const ELimbType& Limb);

	// Update the limb's previous health value
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_UpdatePreviousLimbHealth(const ELimbType& Limb);

	// Add health to a limb by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_IncreaseLimbHealth(const ELimbType& Limb, float Amount);

	// Subtract health to a limb by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_DecreaseLimbHealth(const ELimbType& Limb, float Amount);

	// Resets a limb's health back to default
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_ResetLimbHealth(const ELimbType& Limb);

	// Add tickets to a limb by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_IncreaseLimbTickets(const ELimbType& Limb, int32 Amount);

	// Subtract tickets to a limb by an amount
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_DecreaseLimbTickets(const ELimbType& Limb, int32 Amount);

	// Subtract all tickets to a limb
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_UseAllRemainingLimbTickets(const ELimbType& Limb);

	// Resets limb tickets to maximum
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
    void Server_ResetLimbTickets(const ELimbType& Limb);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
	void Server_ResetAllLimbHealth();
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health")
	void Server_ResetAllLimbTickets();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
	void Server_IncreaseRevive();
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
	void Server_DecreaseRevive();
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
	void Server_SetRemainingRevives(int32 NewRemainingRevives);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
    void Server_ResetRevives();
    
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
	void Server_IncreaseReviveHealth(float Amount);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
    void Server_DecreaseReviveHealth(float Amount);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
    void Server_SetReviveHealth(float NewReviveHealth);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Health | Revive System")
    void Server_ResetReviveHealth();

	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
	bool CanUseReviveSystem() const;

	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
    FORCEINLINE bool IsUsingUnlimitedRevives() const { return bUnlimitedRevives; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
	FORCEINLINE bool IsReviveHealthDepleted() const { return RemainingReviveHealth <= 0.0f; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
	FORCEINLINE int32 GetMaxRevives() const { return MaxRevives; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
	FORCEINLINE int32 GetRemainingRevives() const { return RemainingRevives; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
    FORCEINLINE float GetMaxReviveHealth() const { return MaxReviveHealth; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
	FORCEINLINE float GetRemainingReviveHealth() const { return RemainingReviveHealth; }

	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
    FORCEINLINE float GetRemainingReviveTime() const { return RemainingReviveTime; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Revive System")
    FORCEINLINE float GetReviveOperatingTime() const { return ReviveOperatingTime; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Incapacitation System")
	FORCEINLINE bool IsIncapacitated() const { return HealthStatus == EPlayerHealthStatus::HS_Incapacitated; }

	UFUNCTION(BlueprintPure, Category = "Health | Incapacitation System")
	FORCEINLINE bool IsIncapacitationEnabled() const { return bEnableIncapacitation; }
	
	UFUNCTION(BlueprintPure, Category = "Health | Incapacitation System")
	FORCEINLINE float GetIncapacitationHealthMultiplier() const { return IncapacitationHealthMultiplier; }

protected:
	virtual void Server_InitResource_Implementation() override;
	virtual void Server_SetMaxResource_Implementation(const float NewMaxResource) override;
	
	virtual void OnComponentCreated() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Server_DecreaseResource_Implementation(float Amount) override;

	virtual void Server_SetMaxLimbHealth_Implementation(const ELimbType& Limb, float NewMaxHealth);
	virtual bool Server_SetMaxLimbHealth_Validate(const ELimbType& Limb, const float NewMaxHealth) { return true; }
	
	virtual void Server_HalfMaxLimbHealth_Implementation(const ELimbType& Limb);
	virtual bool Server_HalfMaxLimbHealth_Validate(const ELimbType& Limb) { return true; }

	virtual void Server_SetLimbHealth_Implementation(const ELimbType& Limb, float NewHealthAmount);
	virtual bool Server_SetLimbHealth_Validate(const ELimbType& Limb, const float NewHealthAmount) { return true; }
	
	virtual void Server_SetCurrentLimbHealthToMax_Implementation(const ELimbType& Limb);
	virtual bool Server_SetCurrentLimbHealthToMax_Validate(const ELimbType& Limb) { return true; }

	virtual void Server_UpdatePreviousLimbHealth_Implementation(const ELimbType& Limb);
	virtual bool Server_UpdatePreviousLimbHealth_Validate(const ELimbType& Limb) { return true; }

	virtual void Server_IncreaseLimbHealth_Implementation(const ELimbType& Limb, float Amount);
	virtual bool Server_IncreaseLimbHealth_Validate(const ELimbType& Limb, const float Amount) { return true; }

	virtual void Server_DecreaseLimbHealth_Implementation(const ELimbType& Limb, float Amount);
	virtual bool Server_DecreaseLimbHealth_Validate(const ELimbType& Limb, const float Amount) { return true; }

	virtual void Server_ResetLimbHealth_Implementation(const ELimbType& Limb);
	virtual bool Server_ResetLimbHealth_Validate(const ELimbType& Limb) { return true; }

	virtual void Server_IncreaseLimbTickets_Implementation(const ELimbType& Limb, int32 Amount);
	virtual bool Server_IncreaseLimbTickets_Validate(const ELimbType& Limb, int32 Amount) { return true; }

	virtual void Server_DecreaseLimbTickets_Implementation(const ELimbType& Limb, int32 Amount);
	virtual bool Server_DecreaseLimbTickets_Validate(const ELimbType& Limb, int32 Amount) { return true; }

	virtual void Server_UseAllRemainingLimbTickets_Implementation(const ELimbType& Limb);
	virtual bool Server_UseAllRemainingLimbTickets_Validate(const ELimbType& Limb) { return true; }

	virtual void Server_ResetLimbTickets_Implementation(const ELimbType& Limb);
	virtual bool Server_ResetLimbTickets_Validate(const ELimbType& Limb) { return true; }
	
	virtual void Server_ResetAllLimbHealth_Implementation();
	virtual bool Server_ResetAllLimbHealth_Validate() { return true; }
	
	virtual void Server_ResetAllLimbTickets_Implementation();
	virtual bool Server_ResetAllLimbTickets_Validate() { return true; }
	
	virtual void Server_IncreaseRevive_Implementation();
	virtual bool Server_IncreaseRevive_Validate() { return true; }
	
	virtual void Server_DecreaseRevive_Implementation();
	virtual bool Server_DecreaseRevive_Validate() { return true; }

	virtual void Server_SetRemainingRevives_Implementation(int32 NewRemainingRevives);
	virtual bool Server_SetRemainingRevives_Validate(int32 NewRemainingRevives) { return true; }

	virtual void Server_ResetRevives_Implementation();
	virtual bool Server_ResetRevives_Validate() { return true; }

	virtual void Server_IncreaseReviveHealth_Implementation(float Amount);
	virtual bool Server_IncreaseReviveHealth_Validate(float Amount) { return true; }
	
	virtual void Server_DecreaseReviveHealth_Implementation(float Amount);
	virtual bool Server_DecreaseReviveHealth_Validate(float Amount) { return true; }

	virtual void Server_SetReviveHealth_Implementation(float NewReviveHealth);
	virtual bool Server_SetReviveHealth_Validate(float NewReviveHealth) { return true; }

	virtual void Server_ResetReviveHealth_Implementation();
	virtual bool Server_ResetReviveHealth_Validate() { return true; }
		
	virtual void Server_SetHealthStatus_Implementation(const EPlayerHealthStatus NewHealthStatus) { SetHealthStatus(NewHealthStatus); }
	virtual bool Server_SetHealthStatus_Validate(EPlayerHealthStatus NewHealthStatus) { return true; }

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Character Health")
	EPlayerHealthStatus HealthStatus = EPlayerHealthStatus::HS_Healthy;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Health | Incapacitation")
	uint8 bEnableIncapacitation : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Health | Incapacitation", meta = (ClampMin = 0.01))
	float IncapacitationHealthMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Limbs")
	FLimbHealthData RightLeg;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Limbs")
	FLimbHealthData LeftLeg;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Limbs")
    FLimbHealthData RightArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Limbs")
    FLimbHealthData LeftArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Limbs")
    FLimbHealthData Head;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
    uint8 bUnlimitedRevives : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System", meta = (EditCondition = "!bUnlimitedRevives"))
	int32 MaxRevives = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
    float ReviveTime = 20.0f;

	// Upon revive, how many seconds should we subtract from the current revive time?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System", meta = (EditCondition = "!bUnlimitedRevives"))
    float ReviveTimeDecrement = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
    float ReviveOperatingTime = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
    float MaxReviveHealth = 150.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
    int32 RemainingRevives = 4;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
	float RemainingReviveTime = 20.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Character Health | Revive System")
	float RemainingReviveHealth = 50.0f;

private:
	void BroadcastOnFullLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType);
	void BroadcastOnLowLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType, const float& InCurrentLimbHealth);
	void BroadcastOnNoLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType);
	void BroadcastOnLimbBroken(FLimbHealthData& InLimbData, const ELimbType& LimbType);
	
	void BroadcastOnFullTickets(FLimbHealthData& InLimbData, const ELimbType& LimbType);
	void BroadcastOnNoTickets(FLimbHealthData& InLimbData, const ELimbType& LimbType);
};
