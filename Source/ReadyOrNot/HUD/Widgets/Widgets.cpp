// Copyright Void Interactive, 2017

#include "Widgets.h"

void USpectatorCharacterHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ASpectatePawn* sp = Cast<ASpectatePawn>(GetOwningPlayerPawn());
	if (sp)
	{
		sp->SetSpectatorCharacterWidget(this);
	}
}
