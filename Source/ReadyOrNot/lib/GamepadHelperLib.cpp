#include "GamepadHelperLib.h"

#include "CommonInputSubsystem.h"
#include "CommonUISubsystemBase.h"

int UGamepadHelperLib::GetActiveButton(int currentButtonIndex, int navigationDirection, TArray<bool> buttonAvailability)
{
	if (navigationDirection != 1 && navigationDirection != -1)
	{
		return currentButtonIndex;
	}

	auto newButtonIndex = currentButtonIndex;

	while (true)
	{
		newButtonIndex += navigationDirection;
		if (newButtonIndex < 0 || newButtonIndex > buttonAvailability.Num() - 1)
		{
			return currentButtonIndex;
		}
		if (buttonAvailability[newButtonIndex])
		{
			break;
		}
	}

	return newButtonIndex;
}

bool UGamepadHelperLib::EnableCommonInputPreprocessing(UWorld* World, bool Enable)
{
	if (World)
	{
		if (const APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UCommonInputSubsystem* InputSubsystem = LocalPlayer->
					GetSubsystem<
						UCommonInputSubsystem>())
				{
					// ##UE5UPGRADE## CommonUI
					// UCommonUISubsystemBase::bDisableVirtualAccept = !Enable; // UE5UPGRADE: Input
					// InputSubsystem->EnableInputPreprocessing(Enable);  // UE5UPGRADE: Input
					return true;
				}
			}
		}
	}
	return false;
}
