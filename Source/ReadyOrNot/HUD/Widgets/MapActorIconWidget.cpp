// Void Interactive, 2020

#include "MapActorIconWidget.h"

#include "Components/Image.h"

void UMapActorIconWidget::SetIconBrushStyle(const FSlateBrush InIconBrush, const FLinearColor InIconColor)
{
	Icon_Image->SetBrush(InIconBrush);
	Icon_Image->SetColorAndOpacity(InIconColor);

	Icon_Image_BG->SetBrush(InIconBrush);
	Icon_Image_BG->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
}

void UMapActorIconWidget::SetIconColor(const FLinearColor InIconColor)
{
	Icon_Image->SetColorAndOpacity(InIconColor);
}

void UMapActorIconWidget::UpdateMapActorTranslation()
{
	if (bUseLocation)
	{
		SetRenderTranslation({(-(MapOrigin.Y - LocationToTrack.Y) / MapSize) * MapTextureSize, -(MapOrigin.X - LocationToTrack.X) / -MapSize * MapTextureSize});
	}
	else
	{
		if (ActorToTrack)
		{
			SetRenderTranslation({(-(MapOrigin.Y - ActorToTrack->GetActorLocation().Y) / MapSize) * MapTextureSize, -(MapOrigin.X - ActorToTrack->GetActorLocation().X) / -MapSize * MapTextureSize});

			if (bUseActorRotation)
			{
				Icon_Image->SetRenderTransformAngle(ActorToTrack->GetActorRotation().Yaw + RotationOffset);
				Icon_Image_BG->SetRenderTransformAngle(ActorToTrack->GetActorRotation().Yaw + RotationOffset);
			}
		}
	}
}
