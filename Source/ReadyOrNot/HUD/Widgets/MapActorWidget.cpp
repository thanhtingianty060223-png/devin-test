// Void Interactive, 2020

#include "MapActorWidget.h"

#include "Components/TextBlock.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

void UMapActorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateMapProperties();
	UpdateMapActorTranslation();
}

void UMapActorWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateMapActorTranslation();
}

void UMapActorWidget::InitializeWidget(AActor* InActorToTrack, const bool bInUseActorRotation, const bool bInUseLocation, const FVector InLocationToTrack, const float InRotationOffset)
{
	ActorToTrack = InActorToTrack;
	bUseActorRotation = bInUseActorRotation;
	bUseLocation = bInUseLocation;
	LocationToTrack = InLocationToTrack;
	RotationOffset = InRotationOffset;
}

void UMapActorWidget::SetMapActorText(const FText InText)
{
	if (MapActor_Text)
		MapActor_Text->SetText(InText);
}

void UMapActorWidget::SetMapActorTextColor(const FLinearColor InTextColor)
{
	if (MapActor_Text)
		MapActor_Text->SetColorAndOpacity(InTextColor);
}

void UMapActorWidget::UpdateMapActorTranslation()
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
				SetRenderTransformAngle(ActorToTrack->GetActorRotation().Yaw + RotationOffset);
		}
	}
}

void UMapActorWidget::UpdateMapProperties()
{
	const FName& MapName = UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(this);
	if (FLevelDataLookupTable* LookupRow = UBpGameplayHelperLib::GetLevelLookupDataTable()->FindRow<FLevelDataLookupTable>(MapName,"Level Lookup"))
	{
		MapSize = LookupRow->MapLayout.MapSize;
		MapOrigin = LookupRow->MapLayout.MapOrigin;
		
		if (UTexture2D* MapTexture = LookupRow->MapLayout.MapOverviewTexture.LoadSynchronous())
		{
			MapTextureSize = FMath::Max(MapTexture->GetSizeX(), MapTexture->GetSizeY());
		}
	}
}
