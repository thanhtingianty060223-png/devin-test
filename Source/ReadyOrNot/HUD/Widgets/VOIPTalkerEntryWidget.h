// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "VOIPTalkerEntryWidget.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API UVOIPTalkerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	explicit UVOIPTalkerEntryWidget(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintPure)
	FSlateColor GetVoiceTypeColor() const;
	UFUNCTION(BlueprintPure)
	FText GetVoiceTypeText() const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VOIP Design")
	TMap<ETeamType, FSlateColor> VoiceTypeColorMap;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* PlayerName = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* PlayerChannel = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* VoiceImage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Data", meta = (ExposeOnSpawn = true))
	class AReadyOrNotPlayerState* PlayerState = nullptr;
};
