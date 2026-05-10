// Copyright Void Interactive, 2023

#pragma once

#include "Actors/BaseMagazineWeapon.h"
#include "Components/ShellRackShellComponent.h"
#include "Shotgun.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FShotgunVisuals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<UStaticMesh*> ShellVisuals;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class READYORNOT_API AShotgun : public ABaseMagazineWeapon
{
	GENERATED_BODY()

public:
	AShotgun();
	
	virtual void Tick(float DeltaSeconds) override;

	virtual void AddMagazineCountFromAttachments(float AddAmount) override {}
	virtual void GivenAmmoFromAmmoBag() override {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun")
	uint8 bIsSawnOff : 1;

	// Maximum amount of shells (total) when held by suspects
	UPROPERTY(BlueprintReadOnly, Category = "Shotgun|Ammo")
	int32 MaxShells = 24;

	UPROPERTY(ReplicatedUsing = OnRep_ShellsReplicated)
	TArray<int32> Shells;

	UPROPERTY()
	TArray<int32> OriginalShellCounts;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shotgun|Ammo")
	int32 MaxShellsInWeapon = 8;

	// Defines how many shells should be allocated per ammo slot for this weapon
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shotgun|Ammo")
	int32 ShellsPerSlot = 8;

	// How much each shell weighs
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shotgun|Ammo")
	float ShellWeight;

	virtual float GetWeight() override;

	virtual void SetMagazineCount(int32 Count, TArray<FName> AmmoTypes) override;

	virtual float GetMagazineAmmoPercentage(int32 MagazineIndex) const override;

	virtual float GetCurrentAmmoPercentage() const override;

	void LoadShellsFromMagazine();

	UPROPERTY()
	UStaticMesh* CurrentShellMesh;
	void UpdateShellMesh();

	UFUNCTION()
	void OnRep_ShellsReplicated();

	// If true, you need to press reload every time to load in the shell
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shotgun|Ammo")
	bool bTapReload = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shotgun|Ammo")
	FShotgunVisuals ShotgunVisuals;

	UFUNCTION(BlueprintCallable, Category = "Reloading")
	void CheckReloadSettings();

	// If true, the firing animation is considered a blocking animation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shotgun")
	bool bBlockingFireAnimation = false;

	virtual void BeginPlay() override;
	virtual bool HasAnyAmmo() const override;
	virtual float GetAmmo() const override;
	virtual float RemoveAmmo(float Value) override;
	virtual void ReplenishAmmo() override;
	virtual bool CanReload() override;
	virtual void OnWeaponTacticalReload() override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;
	virtual void OnWeaponReload(bool bForce = false) override;
	virtual float GetAmmoWeight(int32 Count) override;
	virtual void AttachStatic() override;
	virtual void DetachStatic() override;
	virtual void OnDrawComplete() override;
	
	virtual void Server_NextMagazine_Implementation() override;

	virtual void LocallySimulateFire(FRotator Direction, FVector SpawnLoc, int32 Seed) override;
	
	UFUNCTION(BlueprintCallable, Category = Shotgun)
	void PlayReloadLoop();

	virtual void OnRep_AttachmentRep() override;

	/*
	* TODO(killo): needs reimpl
	* keep these around to preserve blueprints
	*/
	UPROPERTY(BlueprintReadWrite, Category = Ammo)
	int32 ShellsInRack = 7;

	UPROPERTY(BlueprintReadOnly, Category = "Shotgun|Ammo|Shell Rack")
	TArray<UShellRackShellComponent*> ShellMeshComponents;

	UFUNCTION(BlueprintCallable, Category = "Shotgun|Ammo|Shell Rack")
	void LoadNextShellInRack() {}

	UFUNCTION(BlueprintCallable, Category = "Shotgun|Ammo|Shell Rack")
	void RefreshEntireShellRack() {}

	UFUNCTION(BlueprintCallable, Category = "Shotgun|Ammo|Shell Rack")
	void FinishedLoadingShellFromRack() {}

#if 0
	// We have slightly different behavior from base weapons when we use an ammo bag
	virtual void GivenAmmoFromAmmoBag() override { TotalShells += MaxShellsInWeapon; }

	// Whether this weapon uses a shell rack
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shotgun|Ammo")
	bool bUseShellRack = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shotgun|Ammo")
	int32 MaxShellsInRack = 7;

	// Whether we are currently refreshing the shell rack
	UPROPERTY(BlueprintReadOnly, Category = "Shotgun|Ammo")
	bool bRefreshingShellRack = false;

	// Whether you can reload the shell rack after finishing reloading the weapon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, "Shotgun|Ammo")
	bool bReloadRack = false;

	// Whether the shell rack is velcro'd onto the weapon (that is, use tacreload to replace reload rack animations)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, "Shotgun|Ammo")
	bool bVelcroRack = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, "Shotgun|Ammo|Shell Rack")
	TArray<FName> ShellRackSockets;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UpdateMagazineCount(int32 NewShellRackCount, int32 NewTotalShellCount);
	virtual void Multicast_UpdateMagazineCount_Implementation(int32 NewShellRackCount, int32 NewTotalShellCount);

	UFUNCTION(Server, Reliable, WithValidation, Category = "Shotgun|Ammo|Shell Rack")
	void Server_ReloadShellInRack();
	virtual void Server_ReloadShellInRack_Implementation();
	virtual bool Server_ReloadShellInRack_Validate() { return true; }

	UFUNCTION(Server, Reliable, WithValidation, Category = "Shotgun|Ammo|Shell Rack")
	void Server_ReloadShellRack();
	virtual void Server_ReloadShellRack_Implementation();
	virtual bool Server_ReloadShellRack_Validate() { return true; }
#endif
};
