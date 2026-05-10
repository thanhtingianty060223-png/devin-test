// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "Data/PostProcessEffectData.h"
#include "PlayerViewActor.generated.h"

UCLASS()
class READYORNOT_API APlayerViewActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APlayerViewActor();

	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
	void TryNextView(bool bRequestClose = false, const bool bIncludeDeadViews = true);

	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
	void SetOwningPlayer(APlayerCharacter* NewOwnerCharacter);
	
	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
    void SetViewPlayer(AReadyOrNotCharacter* NewViewCharacter);
	
	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
	void UpdateViewTarget(const FVector& NewLocation, const FRotator& NewRotation);
	
	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
	void HideComponent(UPrimitiveComponent* ComponentToHide) const;

	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
	void ClearHiddenComponents() const;
	
	UFUNCTION(BlueprintCallable, Category = "Player View Actor")
    void HideActor(AActor* ActorToHide, const bool bIncludeChildActors = true) const;
	
	UFUNCTION(BlueprintPure, Category = "Player View Actor")
	FORCEINLINE bool IsSwitchingView() const { return bSwitchViewEffectsApplied; }

	UPROPERTY(BlueprintReadOnly, Category = "Player View Actor|Data")
	uint8 bShouldCaptureScene : 1;

	bool IsViewInuse(FTransform& OutViewPoint);

	FORCEINLINE USceneCaptureComponent2D* GetSceneCaptureComponent() const { return CameraCaptureComponent; }

protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	void Destroyed() override;

	void NextView(bool bRequestClose = false, const bool bIncludeDeadViews = true);

	int32 RefreshRate = 10;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneCaptureComponent2D* CameraCaptureComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Switch View Effect")
	FPostProcessEffect SwitchViewEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Switch View Effect")
	UFMODEvent* SwitchViewEvent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death View Effect")
	float DeathViewTime = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death View Effect")
	UFMODEvent* DeathViewEvent = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Close View Effect")
	UFMODEvent* CloseViewEvent = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UTextureRenderTarget2D* CameraRenderTarget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	APlayerCharacter* OwningPlayerCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	AReadyOrNotCharacter* ViewCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	FRotator TargetRotation = FRotator::ZeroRotator;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	UMaterialInstance* MI_PostProcess_Greyscale = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	UMaterialInstance* MI_PostProcess_Bump = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	UMaterialInstance* MI_PostProcess_Glitch = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	uint8 bDeathEffectsApplied : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player View|Data")
	uint8 bSwitchViewEffectsApplied : 1;
	
private:
	void SetupSwitchViewEffects();
	
	void SetupDeathEffects();
	void ApplyDeathEffects();
	void ResetDeathEffects();

	float ElapsedTime_CameraRefresh = 0.0f;

	FTimerHandle TH_TryNextView;

	FFMODEventInstance DeathViewEventInstance;
};
