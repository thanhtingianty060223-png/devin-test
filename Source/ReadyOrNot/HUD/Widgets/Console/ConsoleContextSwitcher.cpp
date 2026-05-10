// Copyright Void Interactive, 2022


#include "HUD/Widgets/Console/ConsoleContextSwitcher.h"

PRAGMA_DISABLE_OPTIMIZATION

bool UConsoleContextSwitcher::Initialize()
{
	const bool Initialized = Super::Initialize();
	APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(UGameplayStatics::GetPlayerCharacter(
		GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return Initialized;
	}
	this->SetRenderOpacity(0.0f);
	PlayerCharacter->InputComponent->BindAction("GamepadGrenadeCycle", IE_Pressed, this,
	                                            &UConsoleContextSwitcher::SwapToGrenades);
	PlayerCharacter->InputComponent->BindAction("GamepadTacticalCycle", IE_Pressed, this,
	                                            &UConsoleContextSwitcher::SwapToTactical);
	PlayerCharacter->InputComponent->BindAction("GamepadAttachmentCycle", IE_Pressed, this,
	                                            &UConsoleContextSwitcher::SwapToFireModes);
	// PlayerCharacter->InputComponent->BindAction("GamepadReload", IE_Pressed, this,
	//                                             &UConsoleContextSwitcher::SwapToMag);
	PlayerCharacter->OnWeaponMagCheck.AddDynamic(this, &UConsoleContextSwitcher::SwapToMag);

	PlayerCharacter->InputComponent->BindAction("GamepadGrenadeCycle", IE_Released, this,
	                                            &UConsoleContextSwitcher::QueueFadeOut);
	PlayerCharacter->InputComponent->BindAction("GamepadTacticalCycle", IE_Released, this,
	                                            &UConsoleContextSwitcher::QueueFadeOut);
	PlayerCharacter->InputComponent->BindAction("GamepadAttachmentCycle", IE_Released, this,
	                                            &UConsoleContextSwitcher::QueueFadeOut);
	// PlayerCharacter->InputComponent->BindAction("GamepadReload", IE_Released, this,
	//                                             &UConsoleContextSwitcher::QueueFadeOut);
	return Initialized;
}

void UConsoleContextSwitcher::SwapToGrenades()
{
	PlayFadeIn();
	ContextSwitcher->SetActiveWidgetIndex(0);
}

void UConsoleContextSwitcher::SwapToTactical()
{
	PlayFadeIn();
	ContextSwitcher->SetActiveWidgetIndex(1);
}

void UConsoleContextSwitcher::SwapToFireModes()
{
	const APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	if (Cast<ABaseWeapon>(PlayerCharacter->GetEquippedItem()) != nullptr)
	{
		PlayFadeIn();
		ContextSwitcher->SetActiveWidgetIndex(2);
	}
}

void UConsoleContextSwitcher::SwapToMag(ABaseMagazineWeapon* MagazineWeapon)
{
	PlayFadeIn();
	ContextSwitcher->SetActiveWidgetIndex(3);
	MagSelection->UpdateMagazines(MagazineWeapon);
	QueueFadeOut();
}

void UConsoleContextSwitcher::PlayFadeIn()
{
	const APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	PlayerCharacter->GetWorldTimerManager().ClearTimer(AnimationTimer);
	StopWidgetAnimation_Internal(FadeOut);
	const float StartAtTime = GetRenderOpacity() * FadeIn->GetEndTime();
	PlayAnimation(FadeIn, StartAtTime);
}

void UConsoleContextSwitcher::QueueFadeOut()
{
	const APlayerCharacter* PlayerCharacter = static_cast<APlayerCharacter*>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	float AnimationDelay = 2.0f;
	if (IsAnimationPlaying(FadeIn)) // Add some extra time if the fade in animation is still playing.
	{
		const float CurrentTime = GetAnimationCurrentTime(FadeIn);
		const float EndTime = FadeIn->GetEndTime();
		AnimationDelay += EndTime - CurrentTime;
	}
	if (!IsAnimationPlaying(FadeOut))
	{
		PlayerCharacter->GetWorldTimerManager().SetTimer(AnimationTimer, this, &UConsoleContextSwitcher::PlayFadeOut,
		                                                 AnimationDelay, false);
	}
}

void UConsoleContextSwitcher::PlayFadeOut()
{
	if (!IsAnimationPlaying(FadeOut))
	{
		PlayAnimation(FadeOut);
	}
}

PRAGMA_ENABLE_OPTIMIZATION
