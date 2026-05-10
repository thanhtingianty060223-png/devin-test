// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseItem.h"
#include "Chemlight.generated.h"

/**
 * Illuminates an area when thrown
 */
UCLASS(Abstract)
class READYORNOT_API AChemlight : public ABaseItem
{
	GENERATED_BODY()

public:
	AChemlight();

	UFUNCTION(Server, Reliable, WithValidation, Category = "Chemlight")
	void Server_SpawnThrownChemlight();

	virtual void SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation = FVector::ZeroVector) override;

	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void SetFPMeshHidden(bool bFPMeshHidden);

	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void NormalThrow();

	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void QuickThrow();
	
	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void CancelThrow();

	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void OnChemlightThrown();
	
	UFUNCTION(BlueprintPure, Category = "Chemlight")
	bool CanThrow() const;
	
	UFUNCTION(BlueprintPure, Category = "Chemlight")
	bool IsPlayingChemlightThrowAnimations() const;
	
	UFUNCTION(BlueprintPure, Category = "Chemlight")
	int32 GetRemainingAmmo() const;

	float TimeHeld = 0.0f;
	
	FTimerHandle TH_AIDropChem;

	virtual bool IsDepleted() const override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const override;

	UAmmoComponent* GetAmmoComponent() const { return Ammo; }

protected:
    virtual void Tick(float DeltaTime) override;
	virtual void OnItemPrimaryUse() override;

	virtual bool PlayDraw(bool bDrawFirst) override;

	virtual void Server_SpawnThrownChemlight_Implementation();
	virtual bool Server_SpawnThrownChemlight_Validate() { return true; }

	UFUNCTION(NetMulticast, Reliable, Category = "Chemlight")
			void Multicast_SpawnThrownChemlight();
	virtual void Multicast_SpawnThrownChemlight_Implementation();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UAmmoComponent* Ammo = nullptr;

	// Where on the TP mesh to spawn the chemlight
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chemlight")
	FName SocketSpawnName = NAME_None;

	void PlayChemlightThrowAnimation();
	
private:
	void SpawnThrownChemlight();

	void OnChemlightThrownComplete();

	void ClearTimers();

	bool AnyTimersActive() const;

	FTimerHandle TH_ChemQuickThrowAnimExpiry;
	FTimerHandle TH_ChemThrowCompleteExpiry;

	uint8 bIsBeingQuickThrown : 1;
};
