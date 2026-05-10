// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/CanUseMultitoolOn.h"
#include "Actors/Sound/SoundSource.h"
#include "BombActor.generated.h"

UENUM(BlueprintType)
enum class EBombState : uint8
{
	BS_None,
	BS_Active,
	BS_Disabled,
	BS_Exploded,
	BS_HiddenAndFullyDisabled
};

UCLASS()
class READYORNOT_API ABombActor : public AActor, public IUseabilityInterface, public ICanUseMultitoolOn
{
	GENERATED_BODY()

public:
	ABombActor();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBombDefusedSignature, ABombActor*, DefusedBomb);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBombDefusedSignature OnBombDefused;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystemComponent* ExplosionParticleComponent;

	UPROPERTY(EditAnywhere)
	float MultitoolUseTime = 15.0f;

	UPROPERTY(EditAnywhere)
	float ExplosionRadius = 100000.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	EBombState BombState = EBombState::BS_None;

	UFUNCTION(BlueprintPure)
	EBombState GetBombState() { return BombState; }

	UFUNCTION(BlueprintPure)
	float GetTimeUntilExplodes() { return TimeUntilExplodes; }

	UPROPERTY(VisibleAnywhere, Replicated)
	float TimeUntilExplodes = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPVPBombOnly = false;

	UPROPERTY(EditAnywhere)
	UFMODEvent* BombTickEvent;

	UPROPERTY(EditAnywhere)
	UFMODEvent* BombExplodeEvent;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayBombExplodeSFX();
	
	FFMODEventInstance BombTickEventInst;

	UPROPERTY()
	USoundSource* BombSoundSource;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// ICanUseMultitoolOn
	///////////////////////////////////
	virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual void Client_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual float GetMultitoolUseTime_Implementation() override;
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	///////////////////////////////////

	// IUseablilityInterface
	///////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteract_Implementation() const override;
	///////////////////////////////////

	UFUNCTION(CallInEditor)
	void Explode();
};
