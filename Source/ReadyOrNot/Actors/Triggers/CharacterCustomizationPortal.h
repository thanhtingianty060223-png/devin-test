// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "CharacterCustomizationPortal.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ACharacterCustomizationPortal : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

	ACharacterCustomizationPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	AReadyOrNotCharacter* CustomizationCharacter;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UBillboardComponent* BillboardComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* CharacterSpawnPoint;

	FName LastSetHead;
	FName LastSetBody;

	UFUNCTION(BlueprintCallable)
	void UpdateCharacterLookOverride( FName Head, FName Body);
	
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;

	void SpawnCustomizationCharacter(AReadyOrNotCharacter* InteractInstigator);

	UFUNCTION(BlueprintCallable)
	static bool SaveCharacterLookOverride(FName InHead, FName InBody);

	UFUNCTION(BlueprintPure)
	static bool GetCurrentCharacterLookOverride(FName& OutHead, FName& OutBody);

	UFUNCTION(BlueprintPure)
	static void GetCustomizationEntries(TArray<FCharacterPersonalizationData>& OutHeads, TArray<FCharacterPersonalizationData>& OutBodys);

	UFUNCTION(BlueprintPure)
	static bool IsDLCLocked(FCharacterPersonalizationData Data);

	UFUNCTION(BlueprintPure)
	static bool GetCharacterLookOverride(FName Head, FName Body, FCharacterLookOverride& OutCharacterLookOverride);

	UFUNCTION(BlueprintPure)
	static void GetAllCompatibleBodies(FName InHead, TArray<FName>& OutBodies);
	
	UFUNCTION(BlueprintPure)
	static void GetAllCompatibleHeads(FName InBody, TArray<FName>& OutHeads);

	void DrawOutline();
	void DisableOutline();

	UPROPERTY()
	TArray<UStaticMeshComponent*> CompsToOutline;

	UPROPERTY()
	TArray<ULightComponent*> LightsToEnable;

	UPROPERTY(EditAnywhere)
	FName LightActorsOfTag = "";
	
	// NOTE(killo): fixes issue where optiwand movement should be locked but was being unlocked while in lobby letting you move around
	// Keep track of whether or not we've already locked/unlocked to prevent other player movement lock logic from breaking
	UPROPERTY()
	bool bHasLocked = false;
};
