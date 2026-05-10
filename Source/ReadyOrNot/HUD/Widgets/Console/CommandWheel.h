// Copyright Void Interactive, 2023

#pragma once

#include "Enums.h"
#include "Blueprint/UserWidget.h"
#include "lib/ReadyOrNotCommandFunctionLibrary.h"
#include "CommandWheel.generated.h"

UCLASS()
class READYORNOT_API UCommandWheel : public UUserWidget
{
	GENERATED_BODY()

public:
	void InputCommandVector(const FVector& InputVector);
	void Enable();
	void Disable();
	void Cancel();
	void ToggleQueueing() const;
	void ConfirmCommand();
	void ConfirmCommand(int CurrentIndex);
	bool IsInFreeView() const { return bIsInFreeView;}
	
	void FreeViewConfirm();

	UPROPERTY(EditAnywhere)
	UDataTable* CommandWheelIcons;
	
	void CycleSwatElement(bool Next);

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> Flashbang;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> Stinger;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABaseGrenade> CSGas;

	UPROPERTY(BlueprintReadWrite)
	int CurrentIndex = -1;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UOverlay* QueueTextContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class URichTextBlock* QueueText = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UUserWidget* InnerWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UUserWidget* OuterWheel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UImage* ThumbstickImage;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* HeaderText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UOverlay* HeaderOverlay;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UCanvasPanel* FreeViewInstructionsCanvasPanel;
	
	UPROPERTY()
	class UReadyOrNotCommandFunctionLibrary* CommandLibrary;
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetInnerSegments(int num);

	UFUNCTION(BlueprintImplementableEvent)
	void SetOuterSegments(int num);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetInnerCommands(const TArray<FSwatCommand> &Options);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateOuterWheel(const FVector direction, const TArray<FSwatCommand> &Options, const int SelectedIndex);
	
	UFUNCTION(BlueprintCallable)
	FSlateBrush& GetIconForCommand(FSwatCommand Command);
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetInnerWheelDirection(FVector direction);
	
	
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void UpdateQueueText() const;
	
private:
	TMap<ESwatCommand, FSlateBrush> CommandIconMap;
	
	FLinearColor CurrentWheelColor = FLinearColor::White;
	const FLinearColor GoldColor = FLinearColor (1, 0.95686274509f,0.33333333333f, 0.9f);
	const FLinearColor BlueColor = FLinearColor (0.30980392156f, 0.56078431372f,1, 0.9f);
	const FLinearColor RedColor = FLinearColor (1, 0.2862745098f,0.20784313725f, 0.9f);
	const FLinearColor WhiteColor = FLinearColor::White;
	
	void ResetWheel();
	void UpdateWheelColor();
	void SetHeaderText(FText text) const;
	
	FVector PreviousInputVector;
	TArray<TTuple<FVector,int>> WheelInputHistory;

	bool CommandWheelActive = false;
	TArray<FSwatCommand> PreviousCommandOptions;

	bool bIsInFreeView = false;
	const TArray<ESwatCommand> FreeViewCommands = {
		ESwatCommand::SC_MoveTo_Individual,
		ESwatCommand::SC_MoveToAndBack_Individual,
	};

	FSwatCommand SelectedFreeViewCommand;
};