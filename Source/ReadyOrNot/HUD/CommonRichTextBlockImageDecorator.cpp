// Copyright Void Interactive, 2023


#include "CommonRichTextBlockImageDecorator.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"

const FSlateBrush* UCommonRichTextBlockImageDecorator::FindImageBrush(FName TagOrId, bool bWarnIfMissing)
{
	if (const FSlateBrush* FoundSlateBrush = SlateBrushes.Find(TagOrId)) {
		return FoundSlateBrush;
	}

	if (AReadyOrNotPlayerController* RONPC = UReadyOrNotStatics::GetReadyOrNotPlayerController())
	{
		if (!RONPC->GetLocalPlayer())
			return Super::FindImageBrush(TagOrId, bWarnIfMissing);

		UCommonInputSubsystem* CommonInputSubsystem = RONPC->GetLocalPlayer()->GetSubsystem<UCommonInputSubsystem>();
		TArray<FKey> Keys;
		Keys.Add(FKey{TagOrId});
		FSlateBrush SlateBrush;

		// UE5UPGRADE: Input
		// if (FCommonInputBase::GetCurrentBasePlatformData().TryGetInputBrush(SlateBrush, Keys, CommonInputSubsystem->GetCurrentInputType(), CommonInputSubsystem->GetCurrentGamepadName()))
		// {
		SlateBrushes.Add(TagOrId, SlateBrush);
		return SlateBrushes.Find(TagOrId);
		// }
	}

	return Super::FindImageBrush(TagOrId, bWarnIfMissing);
}
