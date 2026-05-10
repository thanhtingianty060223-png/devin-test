// Copyright Void Interactive, 2017

#include "PlayerHUD.h"
#include "lib/BpGameplayHelperLib.h"

APlayerHUD::APlayerHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.033f;
}

void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void APlayerHUD::DrawHUD()
{
	Super::DrawHUD();
}

void APlayerHUD::SetWidgetTranslationByMouseDelta(APlayerController* Controller, UUserWidget* Widget, float DeltaSeconds, float InterpSpeed, float InputScale, float ClampAt)
{
	if (!Controller || !Widget)
	{
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(Controller->GetPawn());
	if (!pc)
	{
		return;
	}

	bool bDontCare, bSwayHUD;
	float fDontCare;
	int32 iDontCare;
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	UBpGameplayHelperLib::LoadHUDSettings(bDontCare, bDontCare, bDontCare, bDontCare, bDontCare, bDontCare, bSwayHUD, bDontCare, fDontCare, fDontCare, iDontCare, bDontCare, bDontCare);

	if (!pc->bInCommandMenu && !pc->bInDevicesMenu && bSwayHUD && !pc->IsAimLocked())
	{
		Controller->GetInputMouseDelta(deltaX, deltaY);
	}

	FVector2D newVector = UKismetMathLibrary::Vector2DInterpTo(Widget->RenderTransform.Translation, FVector2D(deltaX, deltaY) * InputScale, DeltaSeconds, InterpSpeed);

	if (ClampAt != 0.0f)
	{
		newVector.X = FMath::Clamp(newVector.X, -ClampAt, ClampAt);
		newVector.Y = FMath::Clamp(newVector.Y, -ClampAt, ClampAt);
	}

	Widget->SetRenderTranslation(newVector);
}

void APlayerHUD::SetCanvasTranslationByMouseDelta(APlayerController* Controller, UCanvasPanel* Widget, float DeltaSeconds, float InterpSpeed /*= 1.0f*/, float InputScale /*= 1.0f*/, float ClampAt /*= 0.0f*/)
{
	if (!Controller || !Widget)
	{
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(Controller->GetPawn());
	if (!pc)
	{
		return;
	}

	bool bDontCare, bSwayHUD;
	float fDontCare;
	int32 iDontCare;
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	UBpGameplayHelperLib::LoadHUDSettings(bDontCare, bDontCare, bDontCare, bDontCare, bDontCare, bDontCare, bSwayHUD, bDontCare, fDontCare, fDontCare, iDontCare, bDontCare, bDontCare);

	if (!pc->bInCommandMenu && !pc->bInDevicesMenu && bSwayHUD && !pc->IsAimLocked())
	{
		Controller->GetInputMouseDelta(deltaX, deltaY);
	}

	FVector2D newVector = UKismetMathLibrary::Vector2DInterpTo(Widget->RenderTransform.Translation, FVector2D(deltaX, deltaY) * InputScale, DeltaSeconds, InterpSpeed);

	if (ClampAt != 0.0f)
	{
		newVector.X = FMath::Clamp(newVector.X, -ClampAt, ClampAt);
		newVector.Y = FMath::Clamp(newVector.Y, -ClampAt, ClampAt);
	}

	Widget->SetRenderTranslation(newVector);
}
