// Void Interactive, 2020


#include "TutorialMenu.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

void UTutorialMenu::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTutorialMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	if (!pc)
		return;

	if (!pc->PlayerCameraManager)
	{
		pc->SpawnPlayerCameraManager();
	}
}

void UTutorialMenu::NativeDestruct()
{
	Super::NativeDestruct();
}



