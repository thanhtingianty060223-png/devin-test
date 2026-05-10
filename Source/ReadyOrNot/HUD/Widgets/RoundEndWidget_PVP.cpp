// Copyright Void Interactive, 2021

#include "RoundEndWidget_PVP.h"

void URoundEndWidget_PVP::OnGameModeRoundEnded_Implementation()
{
	ShowWidget();

	FadeIn();
}

void URoundEndWidget_PVP::FadeIn()
{
	PlayWidgetAnimation_Internal(Anim_FadeIn);
}
