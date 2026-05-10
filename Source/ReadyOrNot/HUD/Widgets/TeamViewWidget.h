// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "TeamViewWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UTeamViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	explicit UTeamViewWidget(const FObjectInitializer& ObjectInitializer);
	
	void TickTeamView(float DeltaTime);

	void OnViewSwitched();

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent)
	void Tick_TeamViewOn();
	
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UInvalidationBox* InvalidationBox = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* PlayerViewImage = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* PlayerNameText = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* PlayerHealthStatusText = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UImage* PlayerTeamIndicatorImage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* SwitchViewText = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* CloseViewText = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget))
	class UTextBlock* CurrentViewIndexText = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	TMap<ETeamType, FSlateColor> TeamColorMap;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Design")
	TMap<EPlayerHealthStatus, FSlateColor> HealthStatusColorMap;

private:
	void UpdateInputText();
};
