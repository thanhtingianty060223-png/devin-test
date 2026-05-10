// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "MapActorWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API UMapActorWidget : public UBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Map Actor Widget")
	void InitializeWidget(AActor* InActorToTrack, bool bInUseActorRotation = true, bool bInUseLocation = false, FVector InLocationToTrack = FVector::ZeroVector, float InRotationOffset = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor Widget")
	void SetMapActorText(FText InText);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor Widget")
	void SetMapActorTextColor(FLinearColor InTextColor);

protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void UpdateMapActorTranslation();
	
	UPROPERTY(BlueprintReadOnly, Category = "Optional Widgets", meta = (BindWidgetOptional))
	class UTextBlock* MapActor_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data", meta = (ExposeOnSpawn = true))
	AActor* ActorToTrack = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data", meta = (ExposeOnSpawn = true))
	bool bUseActorRotation = true;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data", meta = (ExposeOnSpawn = true))
	bool bUseLocation = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data", meta = (ExposeOnSpawn = true))
	FVector LocationToTrack = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data", meta = (ExposeOnSpawn = true))
	float RotationOffset = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data")
	float MapSize = 5000.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data")
	float MapTextureSize = 1024.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Map Actor Widget|Data")
	FVector2D MapOrigin = FVector2D::ZeroVector;

private:
	void UpdateMapProperties();
};
