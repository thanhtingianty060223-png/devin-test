// Copyright Void Interactive, 2021

#include "PostProcessRequirement.h"

#include "Characters/PlayerCharacter.h"

UPostProcessRequirement::UPostProcessRequirement()
{
}

void UPostProcessRequirement::Initialize(APlayerCharacter* InPlayerCharacter, AActor* InDamageCauser)
{
	PlayerCharacter = InPlayerCharacter;
	DamageCauser = InDamageCauser;
}

bool UPostProcessRequirement::EnablePostProcessEffect_Implementation()
{
	return true;
}

void UPostProcessRequirement::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPostProcessRequirement, PlayerCharacter);
	DOREPLIFETIME(UPostProcessRequirement, DamageCauser);
}

bool UPPR_IsDamageCauserOnScreen::EnablePostProcessEffect_Implementation()
{
	if (!DamageCauser)
		return false;
	
	FVector2D ScreenPosition;
	if (DamageCauser->IsA(ABaseItem::StaticClass()))
	{
		PlayerCharacter->GetRONPlayerController()->ProjectWorldLocationToScreen(Cast<ABaseItem>(DamageCauser)->GetItemLocation(), ScreenPosition);
	}
	else
	{
		PlayerCharacter->GetRONPlayerController()->ProjectWorldLocationToScreen(DamageCauser->GetActorLocation(), ScreenPosition);
	}

	if (GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		return ScreenPosition.X > 0 && ScreenPosition.X < ViewportSize.X &&
                ScreenPosition.Y > 0 && ScreenPosition.Y < ViewportSize.Y;
	}
	
	return false;
}
