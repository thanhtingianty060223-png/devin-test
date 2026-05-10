// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "SwatCommandStatusWidget.generated.h"

DECLARE_STATS_GROUP(TEXT("Swat Command Status Widget"), STATGROUP_SwatCommandStatusWidget, STATCAT_Advanced);

UCLASS()
class READYORNOT_API USwatCommandStatusWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	void Refresh(bool bUpdateColor = false);
	
	void Shrink(bool bInstant);
	void Grow(bool bInstant);

	void StartHeightChange(float NewHeight);
	void StartHealthWidthChange(float NewWidth);

	void UpdateSquadData();

	void SetPlayerHealthStatus(EPlayerHealthStatus HealthStatus);

	void SetCurrentCommand(FText CommandText, FText ProgressText, bool bWaiting, bool bInProgress);

	void SetCommandNameColorFromTeam(ETeamType Team);
	void SetCommandIconBrush(const FSlateBrush& NewBrush);

	void SetPlayerName(FText NewPlayerName);

	void PlayCommandIssuedAnim();
	
	void HideCommandStatus(bool bShrinkHeight);
	void HideCommandInfo();
	
	void SetCommandStatusText(FText NewText);
	void SetHotkeyText(FText NewText);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bIsLead = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	ESquadPosition SquadPosition = ESquadPosition::SP_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	EPlayerHealthStatus PlayerHealthStatus = EPlayerHealthStatus::HS_NotAvailable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	float MinHeight = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	float MaxHeight = 32.0f;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UHorizontalBox* CurrentCommand_Box = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* CommandStatus_Box = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* CurrentCommand_Text = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* CurrentCommand_Pulse_Text = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* CurrentCommand_Status_Text = nullptr;

protected:
	void TickDesiredHeight();
	void TickDesiredWidth();

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
	
	void PlayCommandCompleteAnim();
	
	void ShowCommandSubText();
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USizeBox* SizeBox = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* SwatInfo_Box = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UHorizontalBox* SwatStats_Box = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* SwatName_Text = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USizeBox* TeamIndicator_Box = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* TeamIndicator_Image = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USizeBox* HealthStatus_SizeBox = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextWidget* PlayerHealth_Text = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class URichTextBlock* IssueCommand_Hotkey_RichText = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UImage* IssueCommand_Hotkey_Icon = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* Anim_CommandCompleted = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* Anim_CommandIssued = nullptr;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* Anim_CommandCompletedWhileAnotherQueued = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsProgress = false;
	
	UPROPERTY(BlueprintReadOnly)
	FLinearColor RedTeamColor = FLinearColor(0.9375f, 0.096029f, 0.078125f, 0.92f);
	UPROPERTY(BlueprintReadOnly)
	FLinearColor BlueTeamColor = FLinearColor(0.109212f, 0.217737f, 0.953125f, 0.92f);
	UPROPERTY(BlueprintReadOnly)
	FLinearColor ElementTeamColor = FLinearColor(0.890625f, 0.883343f, 0.282959f, 0.92f);
	UPROPERTY(BlueprintReadOnly)
	FLinearColor NormalColor = FLinearColor(0.875f, 0.875f, 0.875f, 0.75f);
	UPROPERTY(BlueprintReadOnly)
	FLinearColor InjuredColor = FLinearColor(0.854993f, 0.827451f, 0.43195f, 0.7f);
	UPROPERTY(BlueprintReadOnly)
	FLinearColor DeadColor = FLinearColor(0.880208f, 0.281847f, 0.247559f, 0.7f);

	float TargetHealthWidth = 37.0f;
	float TargetHeight = 37.0f;

	FTimerHandle HeightChangeTimerHandle;
	FTimerHandle HealthWidthChangeTimerHandle;
};
