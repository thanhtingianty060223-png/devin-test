// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "LoadoutPortal.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ALoadoutPortal : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

	ALoadoutPortal();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent; 
#endif
	
	UPROPERTY()
	TArray<UStaticMeshComponent*> CompsToOutline;

	UPROPERTY()
	TArray<ULightComponent*> LightsToEnable;

	UPROPERTY()
	class ULevelSequence* LevelSequence;
	UPROPERTY()
	class ULevelSequencePlayer* LevelSequencePlayer;
	UPROPERTY()
	class ALevelSequenceActor* LevelSequenceActor;

	UPROPERTY(EditAnywhere)
	bool bOpenCustomization = false;
	
	UPROPERTY(EditAnywhere)
	FName LightActorsOfTag = "";

	UPROPERTY()
	bool IsActive = false;

	// UPROPERTY(EditAnywhere)
	// bool bIsWeaponCustomizationOnly = false;

	// UPROPERTY(EditAnywhere, meta = (EditCondition = "bIsWeaponCustomizationOnly"))
	// ACameraActor* WeaponCustomizationCamera;

	// TArray<EItemCategory> RequipItemCategories;

	// bool bInWeaponCustomization = false;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	class ULoadout_V2* LoadoutWidget;
	
	FTimerHandle FadeTimerHandle;

	UFUNCTION()
	void OnLoadoutFadeIn();
	UFUNCTION() void OnLoadoutFadeOut();
	
	UFUNCTION()
	void OnLoadoutLoaded();
	UFUNCTION()
	void OnLoadoutShown();

	UFUNCTION()
	void OnLoadoutHidden();
	UFUNCTION()
	void OnLoadoutUnloaded();

	// UPROPERTY()
	// class ULoadout_V2* Loadout_V2;
	
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;

	// bool bWantsLoadout = false;
	// bool bMustExitZoneBeforeRetrigger = false;

	// UPROPERTY()
	// ULevelStreaming* PreMissionStreamedLevel;

	void DrawOutline();
	void DisableOutline();
	

	// void TryLoadPreMissionPlanning();

	// bool bPreMissionPlanningLevelLoaded = false;
	// void OnPreMissionPlanningLoaded();


	// void OnStartPreMissionWeaponCustomization();
public:
	UFUNCTION()
	void LoadLoadout();
	// UFUNCTION()
	// void UnloadLoadout();
	
	DECLARE_MULTICAST_DELEGATE(FOnLoadoutOpened);
	static FOnLoadoutOpened OnLoadoutOpened;
	
	DECLARE_MULTICAST_DELEGATE(FOnLoadoutClosed);
	static FOnLoadoutClosed OnLoadoutClosed;
};
