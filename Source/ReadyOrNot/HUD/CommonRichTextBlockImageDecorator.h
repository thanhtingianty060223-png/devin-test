// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "CommonRichTextBlockImageDecorator.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCommonRichTextBlockImageDecorator : public URichTextBlockImageDecorator {
	GENERATED_BODY()

public:
	virtual const FSlateBrush* FindImageBrush(FName TagOrId, bool bWarnIfMissing) override;

private:
	TMap<FName, FSlateBrush> SlateBrushes;
};
