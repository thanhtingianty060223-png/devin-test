// Void Interactive, 2020

#include "HUD/Widgets/VOIPTalkerEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

UVOIPTalkerEntryWidget::UVOIPTalkerEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VoiceTypeColorMap.Empty(6);
	VoiceTypeColorMap.Add(ETeamType::TT_NONE, FSlateColor(FColor::White));
	VoiceTypeColorMap.Add(ETeamType::TT_SQUAD, FSlateColor(FColor::Yellow));
	VoiceTypeColorMap.Add(ETeamType::TT_SERT_RED, FSlateColor(FColor::Red));
	VoiceTypeColorMap.Add(ETeamType::TT_SUSPECT, FSlateColor(FColor::Red));
	VoiceTypeColorMap.Add(ETeamType::TT_SERT_BLUE, FSlateColor(FColor::Blue));
	VoiceTypeColorMap.Add(ETeamType::TT_CIVILIAN, FSlateColor(FColor::Blue));
}

void UVOIPTalkerEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsValid(PlayerState))
	{
		PlayerName->SetText(FText::FromString(PlayerState->GetPlayerName()));
	}
}

void UVOIPTalkerEntryWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsValid(PlayerState))
	{
		if (PlayerState->IsTalking())
		{
			VoiceImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			SetRenderOpacity(FMath::FInterpTo(GetRenderOpacity(), 1.0f, InDeltaTime, 6.0f));
		}
		else
		{
			SetRenderOpacity(FMath::FInterpTo(GetRenderOpacity(), 0.0f, InDeltaTime, 3.0f));
			
			if (GetRenderOpacity() <= 0.0f)
				VoiceImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		RemoveFromParent();
	}
}

FSlateColor UVOIPTalkerEntryWidget::GetVoiceTypeColor() const
{
	if (IsValid(PlayerState))
	{
		switch (PlayerState->GetVoiceType())
		{
			case EVoiceType::VT_Local:	return FSlateColor(FColor::White);
			case EVoiceType::VT_Team:	return VoiceTypeColorMap[PlayerState->GetTeam()];
		}
	}

	return FSlateColor::UseForeground();
}

FText UVOIPTalkerEntryWidget::GetVoiceTypeText() const
{
	if (IsValid(PlayerState))
	{
		switch (PlayerState->GetVoiceType())
		{
			case EVoiceType::VT_Local:	return FText::FromString("Local");
			case EVoiceType::VT_Team:	return FText::FromString("Team");
		}
	}

	return FText::FromString("");
}
