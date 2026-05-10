// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CollectableViewController.generated.h"

class ACollectable;
class ACollectableViewer;
class AReadyOrNotPlayerController;
class UCollectableWidget;

UCLASS(HideCategories=("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ACollectableViewController : public AActor
{
	GENERATED_BODY()

public:
	ACollectableViewController();

	virtual void BeginPlay() override;
	
	void AddRotationInput(FVector2D Delta);
	void AddZoomInput(float Delta, bool bAnalogue);

	void OpenViewer(ACollectableViewer* Collectable, AReadyOrNotPlayerController* Controller);
	void CloseViewer();

	UPROPERTY(EditAnywhere)
	float RotationSpeed = 0.1f;

	UPROPERTY(EditAnywhere)
	int32 ScaleLevels = 6;
	
	UPROPERTY(EditAnywhere)
	float ScaleSpeed = 0.0625f;
	
	UPROPERTY(EditAnywhere)
	float MinimumScale = 0.5f;

	UPROPERTY(EditAnywhere)
	float MaximumScale = 2.0f;
	
	UPROPERTY(EditAnywhere)
	float VerticalFov = 59.0f;
	
	UPROPERTY(EditInstanceOnly)
	ACameraActor* CameraActor;

	UPROPERTY(EditInstanceOnly)
	AActor* SpawnPointActor;

	UPROPERTY(Transient)
	ACollectable* CollectableActor;

	UPROPERTY(Transient)
	AReadyOrNotPlayerController* CurrentController;
	
	UPROPERTY(Transient)
	UCollectableWidget* CollectableWidget;
	
private:
	void HorizontalLook(float Input) { AddRotationInput(FVector2D(Input * 8.0f, 0.0f)); }
	void VerticalLook(float Input) { AddRotationInput(FVector2D(0.0f, -Input * 8.0f)); }
	void Zoom(float Input) { AddZoomInput(-Input * ScaleSpeed, true); }
	
	void HideWorldEffects();
	void RestoreWorldEffects();
	
	float CalculateHorizontalFov() const;
	void OnViewportResized(FViewport* Viewport, uint32 Unused);
	
	float CurrentScale = 1.0f;
	
	UPROPERTY(Transient)
	TArray<ADirectionalLight*> HiddenDirectionalLights;

	UPROPERTY(Transient)
	TArray<ASkyLight*> HiddenSkyLights;

	UPROPERTY(Transient)
	TArray<APostProcessVolume*> HiddenPostProcessVolumes;

	UPROPERTY(Transient)
	TArray<AExponentialHeightFog*> HiddenExponentialHeightFogs;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif
};