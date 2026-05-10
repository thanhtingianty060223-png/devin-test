// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Layout/SBox.h"

/**
 * 
 */
class READYORNOT_API SAspectRatioConstraintBox : public SBox
{
public:
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;

	void SetDesiredAspectRatio(TAttribute<FOptionalSize> InDesiredAspectRatio);
	
private:
	TAttribute<FOptionalSize> DesiredAspectRatio;
};
