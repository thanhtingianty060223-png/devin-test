// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "FileMediaSource.h"
#include "GameFramework/Actor.h"
#include "HUD/Widgets/TutorialWidget.h"
#include "WorldTutorial.generated.h"

UENUM()
enum ETooltipActivationType
{
	TAT_EnterArea,
	TAT_DirectLook,
	TAT_LineOfSight
};

UENUM()
enum EScreenspaceMarkerType
{
	SMT_InActivationArea,
	SMT_TutorialClosed
};


UCLASS(Blueprintable)
class READYORNOT_API AWorldTutorial : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldTutorial();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

public:
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* AttachedObject;

	// Tooltip content
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTutorialWidgetData WidgetData;

	// If this tooltip is attached to an object, then put a screenspace marker over it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableScreenspaceMarker = false;

	// Whether to use tooltip actor as location for screenspace marker instead of parent location.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseTutorialLocationForScreenspaceMarker = false;

	// Whether the widget should wait for completion to be called to be removed, rather than a conditional removal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWaitForCompletionClose = false;

	// Whether the widget should be ready to be visible again after the completion is called.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bResetAfterCompletion = false;

	// The number of logins the player must have before this tooltip is not longer active. 0 for always, anything above for the number of logins.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int LoginsUntilInvalid = 0;

	// How the widget should be activated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETooltipActivationType> TutorialActivationType = TAT_EnterArea;

	// When the screenspace marker should be visible or not.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EScreenspaceMarkerType> ScreenspaceMarkerType = SMT_InActivationArea;

	// The location of the tooltip widget on the screen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETutorialWidgetLocation> TutorialWidgetLocation = TWL_Right;

	// How far the actual trace for TAT_LineOfSight or TAT_DirectLook should go.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TutorialActivationDistance = 250;

	UPROPERTY(BlueprintReadOnly)
	bool bIsPlayerInRange = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsTutorialVisible = false;

	UPROPERTY()
	UTutorialWidget* TutorialWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* ActivationComponent;

	// Callable function for "completing" the widget.
	UFUNCTION(BlueprintCallable)
	void CompletionEvent();

	FTraceDelegate TraceDelegate;
	void CreateWidget();
	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	float GetAngleBetween(FVector A, FVector B);
	void OnLeaveActivationArea();
	void OnEnterActivationArea();

	void EnableTutorialVisibility();
	void DisableTutorialVisibility();
};
