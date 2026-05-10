// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Pawn.h"
#include "Interfaces/ControllableByTablet.h"
#include "UnmannedVehicle.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AUnmannedVehicle : public APawn, public IControllableByTablet
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Vehicle)
	class APlayerCharacter* Pilot;

	UPROPERTY(BlueprintReadWrite, Category = Vehicle)
	float Health;

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	virtual void Die(AController* EventInstigator, AActor* DamageCauser);

	UPROPERTY(BlueprintReadOnly, Category = Vehicle)
	bool bDead = false;
public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Vehicle)
		float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Vehicle)
		FText VehicleTabletName;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Vehicle)
	class APlayerCharacter* GetPilot() { return Pilot; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Vehicle)
	float GetHealth() { return Health; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Vehicle)
	bool IsAlive() { return !bDead; }

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Vehicle)
	void Server_StopPiloting(class AReadyOrNotPlayerController* CallingController);
	virtual void Server_StopPiloting_Implementation(class AReadyOrNotPlayerController* CallingController);
	virtual bool Server_StopPiloting_Validate(class AReadyOrNotPlayerController* CallingController) { return true; }

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Vehicle)
	void Server_StartPiloting(class AReadyOrNotPlayerController* NewController);
	virtual void Server_StartPiloting_Implementation(class AReadyOrNotPlayerController* NewController);
	virtual bool Server_StartPiloting_Validate(class AReadyOrNotPlayerController* NewController) { return true; }

	UFUNCTION(BlueprintCallable, Category = Vehicle)
	float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnDeath(AController* EventInstigator, AActor* DamageCauser);

	UPROPERTY(BlueprintReadOnly, Category = Vehicle)
		TSubclassOf<AHUD> PreviousHUD;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Vehicle)
		TSubclassOf<AHUD> VehicleHUD;

	// IControllableByTablet implementation
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		bool CanControlWithTablet(class APlayerCharacter* TabletOwner);
	virtual bool CanControlWithTablet_Implementation(class APlayerCharacter* TabletOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		void AssumeTabletControl(class APlayerCharacter* TabletOwner);
	virtual void AssumeTabletControl_Implementation(class APlayerCharacter* TabletOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		bool CanTabletViewMe(class APlayerCharacter* TabletOwner, class AReadyOrNotGameState* GameState);
	virtual bool CanTabletViewMe_Implementation(class APlayerCharacter* TabletOwner, class AReadyOrNotGameState* GameState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		USceneComponent* GetTabletViewComponent();
	virtual USceneComponent* GetTabletViewComponent_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		ETeamType GetTabletViewTeamColor();
	virtual ETeamType GetTabletViewTeamColor_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		void HideActorsForTabletView(class USceneCaptureComponent2D* Component);
	virtual void HideActorsForTabletView_Implementation(class USceneCaptureComponent2D* Component);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Tablet)
		FText GetTabletNameText();
	virtual FText GetTabletNameText_Implementation() { return VehicleTabletName; };
};
