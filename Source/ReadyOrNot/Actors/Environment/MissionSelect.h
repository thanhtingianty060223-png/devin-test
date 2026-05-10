// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionSelect.generated.h"

UCLASS(Config=Game, HideCategories=("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AMissionSelect : public AActor
{
	GENERATED_BODY()

public:
	AMissionSelect();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> LosSuenosLevel;

	UPROPERTY(EditAnywhere)
	FName PersistentLightingLevelName;
	
	UPROPERTY(EditAnywhere)
	FVector LevelOffset = FVector(0.0f, 0.0f, -25000.0f);
	
	UPROPERTY(EditAnywhere)
	FName CameraTag = "LosSuenosCamera";
	
	UPROPERTY()
	FName EffectsTag = "LosSuenosEffect";

	UPROPERTY()
	FName PreloadTag = "LosSuenosMesh";

	UPROPERTY(EditAnywhere)
	float CameraInterpSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="MissionSelect|Fade")
	float FadeOutTime = 0.5f;

	UPROPERTY(EditAnywhere, Category="MissionSelect|Fade")
	float FadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, Category="MissionSelect|Fade")
	float FadeHoldTime = 1.0f;

	UPROPERTY(EditAnywhere, Category="MissionSelect|Fade")
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY(Config)
	bool bUseBlockingLoad = true;
	
	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	TArray<ADirectionalLight*> HiddenDirectionalLights;

	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	TArray<ASkyLight*> HiddenSkyLights;
	
	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	TArray<APostProcessVolume*> HiddenPostProcessVolumes;
	
	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	TArray<AExponentialHeightFog*> HiddenExponentialHeightFogs;

	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	AActor* ViewTargetActor;
	
	UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay)
	FVector OriginalCameraPosition;
	
	UFUNCTION()
	void OpenMissionSelect();

	UFUNCTION()
	void CloseMissionSelect();

	UFUNCTION()
	bool IsOpen() const { return bIsOpen; }

	UFUNCTION()
	void PreviewMission(class ULevelData* LevelData);
	
	UFUNCTION()
	void SelectMission(class ULevelData* LevelData);
	
private:
	bool bIsOpen = false;
	bool bWorldEffectsHidden = false;
	float FadeTimeRemaining = 0.0f;
	
	FTimerHandle TH_RemoveCurrentLevel;
	FTimerHandle TH_ShowCurrentLevel;
	
	UPROPERTY(Transient)
	ULevelStreamingDynamic* CurrentLevel;
	
	UPROPERTY(Transient)
	ULevelStreamingDynamic* InFlightLevel;

	UPROPERTY(Transient)
	ULevelStreaming* PersistentLightingLevel;
	
	UPROPERTY(Transient)
	AReadyOrNotPlayerController* PlayerController;

	UPROPERTY(Transient)
	APlayerCameraManager* PlayerCameraManager;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif
	
	UFUNCTION()
	void LoadLevel(TSoftObjectPtr<UWorld> Level);
	
	UFUNCTION()
	void OnLevelLoaded();

	UFUNCTION()
	void OnLevelShown();

	void RemoveCurrentLevel();
	void RemoveInFlightLevel();
	void RemoveLevel(ULevelStreamingDynamic* LevelStreaming);
	
	void UpdateCamera(float DeltaSeconds);
	void FindAndSetViewTarget();
	
	void HideWorldEffects();
	void RestoreWorldEffects();
	void HidePersistentLighting();
	void RestorePersistentLighting();

	UCommonActivatableWidget* MissionSelectWidget = nullptr;
};

