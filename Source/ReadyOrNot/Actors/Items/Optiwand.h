// Copyright Void Interactive, 2021

#pragma once

#include "Actors/BaseItem.h"
#include "Optiwand.generated.h"

DECLARE_STATS_GROUP(TEXT("Optiwand"), STATGROUP_Optiwand, STATCAT_Advanced);

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API AOptiwand : public ABaseItem
{
	GENERATED_BODY()
	
public:
	AOptiwand();

	virtual void OnItemPrimaryUse() override;
	virtual void OnItemPrimaryUseEnd() override;
	virtual void OnItemSecondaryUsed() override;

	virtual bool ConsumeMouseMovement(FRotator RotateVector) override;

	UFUNCTION(BlueprintPure, Category = "Preference")
	EOptiwandViewMode GetViewMode() const;
	
	UFUNCTION(BlueprintPure, Category = Optiwand)
	bool IsCameraBlocked() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", DisplayName = "Optiwand Start ADS")
	class UAnimMontage* Montage_StartOptiwandADS;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", DisplayName = "Optiwand End ADS")
	class UAnimMontage* Montage_EndOptiwandADS;

	UPROPERTY(EditDefaultsOnly, Category = "Optiwand")
	float CollisionTraceDistance = 25.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	UFMODEvent* FMODOptiwandMove;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	UFMODEvent* FMODOptiwandEnterView;
	
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	UFMODEvent* FMODOptiwandExitView;

	FHitResult DoHitFromCamera();

	UPROPERTY()
	UMaterialInstance* MI_AIOutline;

	bool IsMirroring() { return bRepMirroring; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual bool PlayDraw(bool bDrawFirst) override;
	
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const override;

	float TimeUntilNextUpdate = 0.0f;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneCaptureComponent2D* SceneCapture2D = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UTextureRenderTarget2D* CameraRenderTarget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	ACameraActor* CameraActor;

	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	FVector LookAtPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	FRotator LookAtRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	FRotator OptiwandCaptureRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	uint8 bMirorring : 1;

	// notify mirroring
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Optiwand|Data")
	bool bRepMirroring = false;

	UPROPERTY(BlueprintReadOnly, Category = "Optiwand|Data")
	uint8 bInUse : 1;
	
private:
	void MirrorOn(AActor* Actor);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void Server_NotifyMirroring(bool bIsMirroring);
	
	void OnStartADS();
	void OnStartADSFinished();
	void OnEndADSFinished();

	UPROPERTY()
	ADoor* LastUsedDoor = nullptr;
	
	UPROPERTY()
	UFMODAudioComponent* FMODOptiwandMoveAudioComp;

	UPROPERTY()
	UFMODAudioComponent* FMODOptiwandEnterViewComp;
	
	UPROPERTY()
	UFMODAudioComponent* FMODOptiwandExitViewComp;

	float DoorRotationOnMirrorStart = 0.0f;

	float MinCameraPitch = -60.0f;
	float MaxCameraPitch = 60.0f;

	float TimeSinceLastSceneUpdate = 0.0f;

	float TimeMirroring = 0.0f;
	float InitialMirrorIdleConvoDelay = 0.0f;

	FRotator ConsumedMovementAccumulation = FRotator::ZeroRotator;
	FRotator OriginalSceneCameraRelativeRotation = FRotator::ZeroRotator;
	float OriginalFOV = 90.0f;

	FTimerHandle TH_StartADS;
	FTimerHandle TH_StartADSFinished;
	FTimerHandle TH_EndADS;
};
