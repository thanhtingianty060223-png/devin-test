// Copyright Void Interactive, 2023

#include "SubtitlesWidget.h"

#include "CommonInputSubsystem.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void USubtitlesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SubtitleBlocks.Empty();
	for (int i = 0; i < MaxSubtitles; i++)
	{
		USubtitleBlock* SubtitleBlock = CreateWidget<USubtitleBlock>(this, SubtitleBlockClass);
		if (!SubtitleBlock)
			continue;

		SubtitleBlock->SetText(FText());
		SubtitleBlock->SetBackgroundColor(FLinearColor::Transparent);
		
		SubtitleBlocks.Add(SubtitleBlock);
	}

	LastAudioTime = GetPlayerContext().GetWorld() ? GetPlayerContext().GetWorld()->AudioTimeSeconds : 0.0f;
	
	USubtitlesSubsystem* SubtitlesSubsystem = GetGameInstance()->GetSubsystem<USubtitlesSubsystem>();
	if (ensure(SubtitlesSubsystem))
	{
		SubtitlesSubsystem->PlaySubtitlesDelegate.AddUObject(this, &USubtitlesWidget::OnSubtitleAdded);
	}
	
	UReadyOrNotGameUserSettings* GameUserSettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (GameUserSettings)
	{
		GameUserSettings->OnSettingsSaved.AddUObject(this, &USubtitlesWidget::OnSettingsUpdated);
	}

	OnSettingsUpdated();
}

void USubtitlesWidget::NativeDestruct()
{
	Super::NativeDestruct();

	USubtitlesSubsystem* SubtitlesSubsystem = GetGameInstance()->GetSubsystem<USubtitlesSubsystem>();
	if (ensure(SubtitlesSubsystem))
	{
		SubtitlesSubsystem->PlaySubtitlesDelegate.RemoveAll(this);
	}

	UReadyOrNotGameUserSettings* GameUserSettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (GameUserSettings)
	{
		GameUserSettings->OnSettingsSaved.RemoveAll(this);
	}
}

void USubtitlesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	float AudioTime = GetPlayerContext().GetWorld() ? GetPlayerContext().GetWorld()->AudioTimeSeconds : 0.0f;
	float AudioDeltaTime = AudioTime - LastAudioTime;
	
	for (USubtitleBlock* SubtitleBlock : SubtitleBlocks)
	{
		if (!IsValid(SubtitleBlock) || !IsValid(SubtitleBlock->VerticalBoxSlot))
			continue;

		SubtitleBlock->CurrentTime += AudioDeltaTime * CurrentSpeed;
		SubtitleBlock->CurrentTime = FMath::Clamp(SubtitleBlock->CurrentTime, 0.0f, SubtitleBlock->TotalTime);
		
		float CurrentTime = SubtitleBlock->CurrentTime;
		float TotalTime = SubtitleBlock->TotalTime;

		float FadeInOpacity = FMath::Clamp(CurrentTime / FadeInTime, 0.0f, 1.0f);
		float FadeOutOpacity = FMath::Clamp((TotalTime - CurrentTime) / FadeOutTime, 0.0f, 1.0f);

		float FinalOpacity = FMath::Clamp(FadeInOpacity * FadeOutOpacity, 0.0f, 1.0f);
		SubtitleBlock->SetRenderOpacity(FinalOpacity);
		
		UVerticalBoxSlot* VerticalBoxSlot = SubtitleBlock->VerticalBoxSlot;
		FVector2D DesiredSize = SubtitleBlock->GetDesiredSize();
		
		float OffsetAlpha = FMath::Clamp(CurrentTime / PadInTime, 0.0f, 1.0f);

		float BottomPadding = FMath::Lerp(-DesiredSize.Y, 0.0f, OffsetAlpha);
		VerticalBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	LastAudioTime = AudioTime;
}

FText USubtitlesWidget::FormatSubtitle(const FSubtitleData& SubtitleData) const
{
	FString Formatted = FString::Printf(TEXT("<%s>%s:</> %s"),
		*SubtitleData.SpeakerTag.ToString(), *SubtitleData.Speaker, *SubtitleData.Dialogue);

	return FText::FromString(Formatted);
}

void USubtitlesWidget::AddSubtitle(USubtitleBlock* SubtitleBlock)
{
	if (!IsValid(SubtitlesVerticalBox) || !IsValid(SubtitleBlock))
		return;

	UVerticalBoxSlot* VerticalBoxSlot = SubtitlesVerticalBox->AddChildToVerticalBox(SubtitleBlock);
	if (!VerticalBoxSlot)
		return;

	SubtitleBlock->ForceLayoutPrepass();
	FVector2D DesiredSize = SubtitleBlock->GetDesiredSize();

	VerticalBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, -DesiredSize.Y));
	VerticalBoxSlot->SetHorizontalAlignment(HAlign_Center);
	VerticalBoxSlot->SetVerticalAlignment(VAlign_Fill);
	
	SubtitleBlock->VerticalBoxSlot = VerticalBoxSlot;
}

void USubtitlesWidget::OnSubtitleAdded(const FSubtitleData& SubtitleData)
{
	if (SubtitleBlocks.Num() <= 0)
		return;
	
	CurrentSubtitle++;
	if (CurrentSubtitle >= SubtitleBlocks.Num())
		CurrentSubtitle = 0;
	
	USubtitleBlock* SubtitleBlock = SubtitleBlocks[CurrentSubtitle];
	if (!IsValid(SubtitleBlock))
		return;

	float PreviousTime = SubtitleBlock->TotalTime - SubtitleBlock->CurrentTime;
	for (USubtitleBlock* Subtitle : SubtitleBlocks)
	{
		float MinimumTime = FMath::Min(Subtitle->CurrentTime + FadeOutTime, Subtitle->TotalTime - Subtitle->CurrentTime);
		Subtitle->TotalTime = FMath::Max(MinimumTime, Subtitle->TotalTime - PreviousTime);
	}
	
	float DesiredTime = SubtitleData.Length + LingerTime;
	if (PreviousSubtitle)
		DesiredTime = FMath::Max(DesiredTime, PreviousSubtitle->TotalTime - PreviousSubtitle->CurrentTime);

	SubtitleBlock->CurrentTime = 0.0f;
	SubtitleBlock->TotalTime = DesiredTime;
	SubtitleBlock->SetText(FormatSubtitle(SubtitleData));

	AddSubtitle(SubtitleBlock);
	PreviousSubtitle = SubtitleBlock;
}

void USubtitlesWidget::OnSettingsUpdated()
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (!Settings)
		return;

	SetVisibility(Settings->bEnableSubtitles ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetSubtitlesSize(Settings->SubtitlesSize);
	
	CurrentSpeed = FMath::IsNearlyEqual(1.0f, Settings->SubtitlesSpeed, 0.01f) ? 1.0f : Settings->SubtitlesSpeed;
	CurrentBackgroundOpacity = Settings->SubtitlesBackgroundOpacity;

	FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, CurrentBackgroundOpacity);
	for (USubtitleBlock* SubtitleBlock : SubtitleBlocks)
	{
		if (IsValid(SubtitleBlock))
			SubtitleBlock->SetBackgroundColor(BackgroundColor);
	}
}
