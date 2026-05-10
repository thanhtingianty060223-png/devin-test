// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseItem.h"
#include "Data/InteractionsData.h"
#include "Actors/Gameplay/PlacedZipcuffs.h"
#include "Zipcuffs.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AZipcuffs : public ABaseItem
{
	GENERATED_BODY()

	AZipcuffs();
	
	virtual void OnItemSecondaryUsed() override;
public:
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;
	
	UFUNCTION(Server, BlueprintCallable, Reliable, WithValidation)
	void Server_ArrestStart(AReadyOrNotCharacter* TargetedChar);
	virtual void Server_ArrestStart_Implementation(AReadyOrNotCharacter* TargetedChar);
	virtual bool Server_ArrestStart_Validate(AReadyOrNotCharacter* TargetedChar) { return true; }
	
	UFUNCTION(Server, BlueprintCallable, Reliable, WithValidation)
	void Server_ArrestComplete();
	virtual void Server_ArrestComplete_Implementation();
	virtual bool Server_ArrestComplete_Validate() { return true; }

	UFUNCTION(Server, BlueprintCallable, Reliable, WithValidation)
	void Server_ArrestCancelled();
	virtual void Server_ArrestCancelled_Implementation();
	virtual bool Server_ArrestCancelled_Validate() { return true; }

	UFUNCTION(Client, BlueprintCallable, Reliable)
	void Client_ArrestComplete();
	virtual void Client_ArrestComplete_Implementation();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnRagdollArrestStart(AReadyOrNotCharacter* ArrestTarget);
	void Multicast_OnRagdollArrestStart_Implementation(AReadyOrNotCharacter* ArrestTarget);

	virtual void StunnedWhileEquipped_Implementation();

	virtual bool PlayDraw(bool bDrawFirst) override;

	UFUNCTION()
	void OnRagdollArrestInteractionStarted();
	UFUNCTION()
	void OnRagdollArrestComplete_Driver(AActor* Driver);
	UFUNCTION()
	void OnRagdollArrestComplete_Slave(AActor* Slave);
	
	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true) override;

	UPROPERTY()
	class APlayerCharacter* PendingArrestCharacter = nullptr;
	UPROPERTY()
	class AReadyOrNotCharacter* TargetedCharacter = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Zipcuffs)
	bool bArresting = false;

	float ArrestTimer = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = Zipcuffs)
		float ArrestMaxDistance = 200.0f;

	UPROPERTY(EditAnywhere, Category = Zipcuffs)
		TArray<TEnumAsByte<ECollisionChannel>> ArrestCollisionChannels;

	UPROPERTY(EditAnywhere, Category = Zipcuffs)
	UAnimMontage* UseZipcuffs_1P;

	UPROPERTY(EditAnywhere, Category = Zipcuffs)
		UAnimMontage* UseZipcuffs_3P;

	UPROPERTY(EditAnywhere, Category = Zipcuffs)
		float ArrestTime = 2.0f;

	float ArrestCurrentTime;
	float DoingArrestTime;
	bool bDoingArrest;

	// specified from the arrest activity so we know the offset in advance
	UPROPERTY()
	UInteractionsData* ForcedInteractionData = nullptr;


	//Standing Arrest Interaction for Suspects
	UPROPERTY(EditAnywhere, Category = "Zipcuffs|Interactions")
	TArray<UInteractionsData*> StandingArrestInteractionSuspects;
	
	//Standing Arrest Interaction for Civilians
	UPROPERTY(EditAnywhere, Category = "Zipcuffs|Interactions")
	TArray<UInteractionsData*> StandingArrestInteractionCivilians;
	
	// standing arrest interaction, pvp
	UPROPERTY(EditAnywhere, Category = "Zipcuffs|Interactions")
	TArray<UInteractionsData*> PvPArrestInteraction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zipcuffs|Interactions")
	class UInteractionsData* ArrestRagdoll_Up = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zipcuffs|Interactions")
	class UInteractionsData* ArrestRagdoll_Down = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zipcuffs|Interactions")
	class UInteractionsData* ArrestRagdoll_Left = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zipcuffs|Interactions")
	class UInteractionsData* ArrestRagdoll_Right = nullptr;
	

	// The zipcuffs that spawn on the person.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnedCuffs)
	TSubclassOf<APlacedZipcuffs> SpawnedZipcuffsClass;

	// Which bone on the person to spawn the zipcuffs
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = SpawnedCuffs)
	FName ZipcuffBone;

	// Transform to use on the spawned zipcuffs
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = SpawnedCuffs)
	FTransform SpawnCuffsTransform;

	// Rotation to use on the spawned zipcuffs (relative)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = SpawnedCuffs)
		FRotator SpawnCuffsRelativeRotation;

	// Translation to use on the spawned zipcuffs (relative)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = SpawnedCuffs)
		FVector SpawnCuffsRelativeTranslation;

	//UFMODAudioComponent* FMODArrestAudio;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Audio)
	UFMODEvent* FMODZipcuffsArrest;

	// Raise fear for the arrested guy when near a dead body by this amount
	//UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Cybernetics)
	//TMap<ECyberneticsLevel, float> CivilianCorpseFearAmount;
//
	//// Raise fear for the arrested guy when this close to the dead guy
	//UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Cybernetics)
	//TMap<ECyberneticsLevel, float> CivilianCorpseFearRadius;
//
	//// Raise fear for the arrested guy when near a dead body by this amount
	//UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Cybernetics)
	//TMap<ECyberneticsLevel, float> SuspectCorpseFearAmount;
//
	//// Raise fear for the arrested guy when this close to the dead guy
	//UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Cybernetics)
	//TMap<ECyberneticsLevel, float> SuspectCorpseFearRadius;
};
