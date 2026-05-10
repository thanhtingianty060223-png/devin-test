// Void Interactive, 2020

#pragma once

#include "Components/WidgetComponent.h"
#include "ObjectiveMarkerComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("Objective Marker Component"), STATGROUP_ObjectiveMarkerComp, STATCAT_Advanced);

/*
 * A scene component that can be added to any actor to display objectives on the player's HUD
 */
UCLASS(ClassGroup=("Objective Markers"), HideCategories=("Activation", "Rendering", "Cooking", "Physics", "LOD", "Assest User Data", "Collision"), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UObjectiveMarkerComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:	
	UObjectiveMarkerComponent();

	void InitMarkerSettings(const FSlateBrush& InBrush, const FLinearColor& InIconColorAndOpacity = FColor::White);

	void CreateObjectiveMarkerWidget();

	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetIconBrush(FSlateBrush NewIconBrush);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetIconColor(FLinearColor InIconColorAndOpacity);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetMarkerTextColor(FLinearColor InIconColorAndOpacity);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetIconSize(FVector2D NewIconSize);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void ShowIcon();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void HideIcon();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void ShowMarkerText();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void HideMarkerText();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void EnableObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
    void DisableObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
    void ToggleObjectiveMarkerVisibility();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void ShowObjectiveMarker();

	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
    void HideObjectiveMarker(bool bFadeOut = false);

	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetNewFadeDistance(float NewDistance);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetMarkerText(FText NewMarkerText);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker Component")
	void SetMarkerTextFontSize(int32 NewFontSize);

	FORCEINLINE class UObjectiveMarkerWidget* GetOffscreenMarkerWidget() const { return ObjectiveMarkerWidget_Offscreen; }

	UFUNCTION(BlueprintPure, Category = "Objective Marker Component")
	FORCEINLINE bool IsObjectiveMarkerOffscreen() const { return bIsOffscreen; }

	UFUNCTION(BlueprintPure, Category = "Objective Marker Component")
	bool CanShowObjectiveMarker() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective Marker|Settings")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bStartHidden : 1;
	
	uint8 bDynamic : 1;

	// Use this component's location instead of its owner?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bCustomLocation : 1;

	// Reduce the opacity of the objective marker when offscreen?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bFadeOffscreen : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bDistanceScaleIcon : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bHideIconOffscreen : 1;
	
	// Fade out the objective marker when it's overlapping other widgets on the viewport
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bCompletelyFadeWhenOverlappingOtherWidgets : 1;
	
	// Fade out the objective marker when the local player is close
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bCompletelyFadeWhenClose : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled && bCompletelyFadeWhenClose"))
	float FadeAtDistance_Close = 100.0f;
	
	// Fade out the objective marker when the local player is far
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bCompletelyFadeWhenFar : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled && bCompletelyFadeWhenFar"))
	float FadeAtDistance_Far = 1500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bHideDistanceInfo : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled && bHideDistanceInfo"))
	float HideDistanceInfoAtDistance = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bHideDirectionalArrow : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	uint8 bDisplayMarkerText : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled && bDisplayMarkerText"))
	FText MarkerText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	TSubclassOf<class UObjectiveMarkerWidget> MarkerWidgetClass = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	FSlateBrush IconBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	FLinearColor IconColorAndOpacity = FColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Marker|Settings", meta = (EditCondition="bEnabled"))
	FVector2D IconSize = FVector2D(44.0f, 44.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective Marker|Settings")
	uint8 bDebug : 1;
	
	uint8 bOverrideIconVisibility : 1;

	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker|Data")
	class UObjectiveMarkerWidget* ObjectiveMarkerWidget_Offscreen = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker|Data")
	class UObjectiveMarkerWidget* ObjectiveMarkerWidget_Onscreen = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker|Data")
	bool bIsOffscreen = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker|Data")
	bool bRequestingFadeOut = false;

private:
	void CheckHUDOnScreen();
	void DestroyObjectiveMarkerWidget();

	FVector CurrentMarkerLocation = FVector::ZeroVector;

	static float FadeSpeed;

	FTimerHandle TH_CheckHUD;
	
	uint8 bHUDVisible : 1;
	
	uint8 bFirstTick : 1;
};
