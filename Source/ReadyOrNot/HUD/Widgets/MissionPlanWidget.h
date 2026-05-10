// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "SlateMaterialBrush.h"
#include "Blueprint/UserWidget.h"
#include "Data/LevelData.h"
#include "Info/MissionPlanManager.h"
#include "MissionPlanWidget.generated.h"

class SMissionPlanLinesWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMissionPlanLinesWidget)
			: _LineMaterial(nullptr)
		{}
		SLATE_ARGUMENT(TWeakObjectPtr<class AMissionPlanManager>, MissionPlanManager)
		SLATE_ARGUMENT(UMaterialInterface*, LineMaterial)
		SLATE_ARGUMENT(float, LineWidth)
		SLATE_ARGUMENT(float, FirstNodeRadius)
		SLATE_ARGUMENT(float, NodeRadius)
		SLATE_ARGUMENT(bool, bOnlyDrawPreviewLine)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(32.0f, 32.0f); }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
	void SetCurrentFloor(int32 Floor) { CurrentFloor = Floor; }
	void SetPreviewLine(const FPlanningLine& Line) { PreviewLine = Line; }
	
private:
	void DrawPlanningLine(const FPlanningLine& Line, const FGeometry& AllottedGeometry, TArray<FSlateVertex>& Vertices, TArray<SlateIndex>& Indices) const;
	
	TWeakObjectPtr<AMissionPlanManager> MissionPlanManager = nullptr;
	
	TSharedPtr<FSlateMaterialBrush> Brush;
	FSlateResourceHandle LineBrushRenderProxy;
	
	float LineWidth = 3.0f;
	float FirstNodeRadius = 20.0f;
	float NodeRadius = 12.0f;

	int32 CurrentFloor = 0;
	FPlanningLine PreviewLine;
	bool bOnlyDrawPreviewLine = false;
};

UCLASS()
class READYORNOT_API UMissionPlanLinesWidget : public UWidget
{
	GENERATED_BODY()
	
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UPROPERTY(EditAnywhere)
	float LineWidth = 3.0f;

	UPROPERTY(EditAnywhere)
	float FirstNodeRadius = 20.0f;

	UPROPERTY(EditAnywhere)
	float NodeRadius = 12.0f;
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* LineMaterial;

	UPROPERTY(EditAnywhere)
	bool bOnlyDrawPreviewLine = false;
	
	UFUNCTION(BlueprintCallable)
	void SetCurrentFloor(int32 Floor);

	UFUNCTION(BlueprintCallable)
	void SetPreviewLine(const FPlanningLine& Line);
	
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<class SMissionPlanLinesWidget> SlateWidget;
	
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UMissionPlanWidget : public UUserWidget
{
	GENERATED_BODY()

	FString CurrentURL = "";
	FName CurrentEntryPoint;
	
	bool bIsDrawing = false;

	TSharedPtr<FSlateMaterialBrush> Brush;
	FSlateResourceHandle LineBrushRenderProxy;

	UPROPERTY()
	AMissionPlanManager* MissionPlanManager;
	
	UPROPERTY()
	class UFMODAudioComponent* DrawingAudioComponent;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	void FinishDrawing();
	void UpdateURL();
	
public:
	UPROPERTY(EditAnywhere, Category="Free Drawing")
	float LineThickness = 3.0f;
	
	UPROPERTY(EditAnywhere, Category="Free Drawing")
	float FadeTime = 10.0f;

	UPROPERTY(EditAnywhere, Category="Free Drawing")
	FLinearColor LineColor = FLinearColor::Gray;
	
	UPROPERTY(EditAnywhere, Category="Free Drawing")
	FLinearColor ActiveLineColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category="Lines")
	float PlanLineWidth = 3.0f;

	UPROPERTY(EditAnywhere, Category="Lines")
	float FirstNodeRadius = 20.0f;

	UPROPERTY(EditAnywhere, Category="Lines")
	float NodeRadius = 12.0f;
	
	UPROPERTY(EditAnywhere, Category="Lines")
	class UMaterialInterface* PlanLineMaterial;
	
	UPROPERTY(EditAnywhere)
	class UFMODEvent* DrawingSoundEvent;

	UPROPERTY(BlueprintReadWrite)
	int32 DrawingFloor = 0;

	UPROPERTY(BlueprintReadWrite)
	UWidget* DrawingTargetWidget;

	UPROPERTY(BlueprintReadWrite)
	FPlanningLine PreviewLine;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetEntryPoint(FName EntryPoint);

	UFUNCTION(BlueprintImplementableEvent)
	void OnMissionChanged(const FString& URL, const FLevelDataLookupTable& LevelData);

	UFUNCTION(BlueprintImplementableEvent)
	void OnEntryPointChanged(FEntryPoint NewEntryPoint);
	
	UFUNCTION(BlueprintCallable)
	void AddMarker(const FPlanningMarker& Marker);

	UFUNCTION(BlueprintCallable)
	void RemoveMarker(int32 ID);

	UFUNCTION(BlueprintCallable)
	void AddLine(const FPlanningLine& Line);

	UFUNCTION(BlueprintCallable)
	void RemoveLine(int32 ID);

	UFUNCTION(BlueprintImplementableEvent)
	void OnMarkerAdded(int32 ID, const FPlanningMarker& Marker);

	UFUNCTION(BlueprintImplementableEvent)
	void OnMarkerRemoved(int32 ID);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLineAdded(int32 ID, const FPlanningLine& Line);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLineRemoved(int32 ID);

	void HandleMarkerAdded(const FPlanningMarker& Marker) { OnMarkerAdded(Marker.ReplicationID, Marker); }
	void HandleMarkerRemoved(int32 ID) { OnMarkerRemoved(ID); }

	void HandleLineAdded(const FPlanningLine& Line) { OnLineAdded(Line.ReplicationID, Line); }
	void HandleLineRemoved(int32 ID) { OnLineRemoved(ID); }
};
