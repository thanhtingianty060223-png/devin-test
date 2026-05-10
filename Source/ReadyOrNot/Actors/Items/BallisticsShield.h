// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseDeployableGear.h"
#include "BallisticsShield.generated.h"

UCLASS(Abstract)
class READYORNOT_API ABallisticsShield final : public ABaseDeployableGear
{
	GENERATED_BODY()

	ABallisticsShield();
public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY()
	UMaterialInstanceDynamic* GlassMaterialInstance;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentRep, BlueprintReadOnly)
	ABaseMagazineWeapon* PistolEquippedWithShield;

	void SetPistol(ABaseItem* newPistol);

	UFUNCTION(Client, Reliable)
	void Client_SetPistol(ABaseItem* newPistol);

	virtual bool IsCollidesWhileNotEquipped() const override { return true; }

	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;

	float RefireDelay = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = Shield)
	UFMODEvent* ShieldHitEvent;
	
	UPROPERTY(BlueprintReadOnly)
	int32 Damage = 0;
	
	UPROPERTY(Replicated)
	float GlassPhaseParam = 0.0f;

	void DamageShieldGlass();
	
	bool bTryingToAim = false;
	bool bTryingToStopAiming = false;

	virtual void OnItemPrimaryUse() override;
	virtual void OnItemSecondaryUsed() override;
	virtual void OnItemEndSecondaryUse() override;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = Shield)
	bool bLowered = false;

	UFUNCTION(Client, Unreliable)
	void Client_PlayShieldHitSound();
	void Client_PlayShieldHitSound_Implementation();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLowered(bool bShouldLower);
	void Server_SetLowered_Implementation(bool bShouldLower);
	bool Server_SetLowered_Validate(bool bShouldLower) { return true; }

	virtual bool PlayDraw(bool bDrawFirst) override;
	virtual bool PlayHolster();

	void OnItemReload();
	void OnItemReloadComplete();
	
	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true) override;
	
	virtual void OnRep_AttachmentRep() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ULegacyCameraShake> ShieldHitCameraShake;

	UFUNCTION()
	virtual void OnTPShieldHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
