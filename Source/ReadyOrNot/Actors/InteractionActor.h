// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interfaces/CanUse.h"
#include "InteractionActor.generated.h"

// TODO: Delete?
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AInteractionActor : public AActor, public ICanUse
{
	GENERATED_BODY()

public:
	AInteractionActor();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Use)
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Use)
	USphereComponent* UseIconRadius = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Use)
	UStaticMeshComponent* Mesh_Static = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Use)
	USkeletalMeshComponent* Mesh_Skeletal = nullptr;

	// Whether or not this thing can be used now (if false, icon will not show it as usable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
	bool bCanUseNow = true;

	// Whether or not the thing should be considered for tracing (if false, the icon will not show up -at all-)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
	bool bAvailableForUse = true;

	// Whether or not using this thing causes the icon to completely disappear when used (if false, it will blip instead)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
	bool bCompleteIcon = true;

	// Whether or not using this thing causes the player to use the button push animation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Use)
	bool bButtonPushAnimation = false;

	// Blueprint event that fires off when this thing starts being used
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Use)
	void OnActorUsed(AActor* User);

	// Blueprint event that fires off when this thing stops being used.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Use)
	void OnActorUsedEnd(AActor* User);

	// Whether we should use a hold dialogue over a press dialogue
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
	bool bHoldButtonPrompt = false;

	// Whether we should override the button prompt text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
	bool bOverrideButtonPrompt = false;

	// The text that we should override the button prompt with
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Use)
		FText OverrideButtonPromptText;

	// Check to see if this thing can be used
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = Use)
		bool CanBeUsedNow(AActor* PotentialUser);
	virtual bool CanBeUsedNow_Implementation(AActor* PotentialUser) { return true; }

	// Call this when we want to use the thing.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Use)
		void Server_TryUse(AActor* User);
	virtual void Server_TryUse_Implementation(AActor* User);
	virtual bool Server_TryUse_Validate(AActor* User) { return true; };

	// Call this when we want to stop using the thing.
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Use)
		void Server_EndUse(AActor* User);
	virtual void Server_EndUse_Implementation(AActor* User);
	virtual bool Server_EndUse_Validate(AActor* User) { return true; }

	///////////////////////////////////////////////////////
	//
	//	ICanUse implementation

	UPROPERTY()
	TArray<USceneComponent*> CachedUseComponents;

	// Returns true if we can use this thing now
	virtual bool CanUse_Implementation(class APlayerCharacter* User) override;
	virtual bool IsAvailableForUse_Implementation() override;
	virtual bool StartUse_Implementation(class APlayerCharacter* User) override;
	virtual void EndUse_Implementation(class APlayerCharacter* User) override;
	virtual bool OverridesUseButtonPromptText_Implementation() override;
	virtual FText GetUseButtonPromptText_Implementation() override;
	virtual bool PlaysUseIconComplete_Implementation() override;
	virtual USceneComponent* GetUseIconBoltComponent_Implementation() override;
	virtual TArray<USceneComponent*> GetUseViewComponents_Implementation() override;
	virtual bool UsesHoldButtonPrompt_Implementation() override { return bHoldButtonPrompt; }
};
