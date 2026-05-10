// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/SubtitlesSubsystem.h"
#include "SubtitlesWidget.generated.h"

// TODO(killo): slate version could probably avoid a lot of unnecessary allocations. ron 2

/*
 *	Subtitle block, serves as an interface for blueprint design
 */
UCLASS()
class READYORNOT_API USubtitleBlock : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	float CurrentTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float TotalTime = 0.0f;

	UPROPERTY(Transient)
	class UVerticalBoxSlot* VerticalBoxSlot;
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetText(const FText& Text);

	UFUNCTION(BlueprintImplementableEvent)
	void SetBackgroundColor(FLinearColor Color);
};

/**
 *	Subtitles widget, manages incoming subtitles and timing logic
 */
UCLASS()
class READYORNOT_API USubtitlesWidget : public UUserWidget
{
	GENERATED_BODY()
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<USubtitleBlock> SubtitleBlockClass;
	
	UPROPERTY(EditAnywhere)
	int32 MaxSubtitles = 3;

	UPROPERTY(EditAnywhere)
	float LingerTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float FadeInTime = 0.8f;

	UPROPERTY(EditAnywhere)
	float FadeOutTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float PadInTime = 0.5f;
	
	UPROPERTY(meta=(BindWidgetOptional))
	class UVerticalBox* SubtitlesVerticalBox;

	UFUNCTION(BlueprintImplementableEvent)
	void SetSubtitlesSize(ESubtitlesSize Size);
	
private:
	UPROPERTY(Transient)
	TArray<USubtitleBlock*> SubtitleBlocks;

	UPROPERTY(Transient)
	USubtitleBlock* PreviousSubtitle;

	float LastAudioTime = 0.0f;
	int32 CurrentSubtitle = INDEX_NONE;
	
	float CurrentSpeed = 1.0f;
	float CurrentBackgroundOpacity = 0.0f;
	
	FText FormatSubtitle(const FSubtitleData& SubtitleData) const;

	void AddSubtitle(USubtitleBlock* SubtitleBlock);
	
	void OnSubtitleAdded(const FSubtitleData& SubtitleData);
	void OnSettingsUpdated();
};
