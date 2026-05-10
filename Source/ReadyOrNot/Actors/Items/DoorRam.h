// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseDeployableGear.h"
#include "DoorRam.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API ADoorRam : public ABaseDeployableGear
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Ram")
	void Server_StrikeDoor(ADoor* TargetDoor);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Ram")
	void Server_StrikePlayer(APlayerCharacter* TargetPlayer);
	
	UFUNCTION(BlueprintPure, Category = "Ram")
	FHitResult TryGetHitPosition() const;
	
	UFUNCTION(BlueprintPure, Category = "Ram")
	bool CanHitActor(const FHitResult& TestHit) const;
	
	// Things we are allowed to hit
	UPROPERTY(EditDefaultsOnly, Category = "Ram")
	TArray<TSubclassOf<AActor>> AcceptableHitWhitelist;

	// How much to deduct from a door's lock integrity, minimum. When lock integrity reaches 0, the door becomes unlocked.
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitIntegrityMin = 0.2;

	// How much to deduct from a door's lock integrity, maximum. When lock integrity reaches 0, the door becomes unlocked.
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitIntegrityMax = 1.0;

	// Trace distance.
	UPROPERTY(EditAnywhere, Category = "Ram")
	float MaxHitDistance = 50.0f;

	// Minimum amount of strength to open doors with, when it's closed
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthClosedMin = 50.0f;

	// Maximum amount of strength to open doors with, when it's closed
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthClosedMax = 50.0f;

	// Minimum amount of strength to hit doors with, when it's open
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthOpenMin = 50.0f;

	// Maximum amount of strength to hit doors with, when it's open
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthOpenMax = 50.0f;

	// Minimum amount of strength to open doors with, when it's locked
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthLockedMin = 50.0f;

	// Maximum amount of strength to open doors with, when it's locked
	//UPROPERTY(EditAnywhere, Category = "Ram")
	//float HitStrengthLockedMax = 50.0f;
		
	UPROPERTY(EditAnywhere, Category = "Ram")
	TSubclassOf<UDamageType> RamDamageTypeDefault;
	
	UPROPERTY(EditAnywhere, Category = "Ram")
	TSubclassOf<UDamageType> RamDamageTypeCrumble;
	
	UPROPERTY(EditAnywhere, Category = "Ram")
	TSubclassOf<UDamageType> RamDamageTypePlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ram")
	float StrikePlayerDamage = 200.0f;
	
protected:
	virtual void OnItemPrimaryUse() override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	void StartRamming();

	// Called when the battering ram hits its intended target
	UFUNCTION(BlueprintCallable, Category = "Ram")
	virtual void OnBatteringRamHit();

	virtual void Server_StrikeDoor_Implementation(ADoor* TargetDoor);
	virtual bool Server_StrikeDoor_Validate(ADoor* TargetDoor) { return true; }

	virtual void Server_StrikePlayer_Implementation(APlayerCharacter* TargetCharacter);
	virtual bool Server_StrikePlayer_Validate(APlayerCharacter* TargetPlayer) { return true; }
	
	UPROPERTY(BlueprintReadOnly, Category = "Ram")
	FHitResult LastGoodHit;
};
