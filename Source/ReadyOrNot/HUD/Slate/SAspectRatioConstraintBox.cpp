// Copyright Void Interactive, 2023

#include "SAspectRatioConstraintBox.h"

void SAspectRatioConstraintBox::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const EVisibility ChildVisibility = ChildSlot.GetWidget()->GetVisibility();
	if ( ArrangedChildren.Accepts(ChildVisibility) )
	{
		const FOptionalSize CurrentMinAspectRatio = 0;
		const FOptionalSize CurrentMaxAspectRatio = DesiredAspectRatio.Get();

		// UE5UPGRADE : TODO General build error: 'SlotPadding': is not a member of 'FSlotBase'
		// const FMargin SlotPadding(ChildSlot.SlotPadding.Get());
		const FMargin SlotPadding;
		bool bAlignChildren = true;

		AlignmentArrangeResult XAlignmentResult(0, 0);
		AlignmentArrangeResult YAlignmentResult(0, 0);

		if (CurrentMaxAspectRatio.IsSet() || CurrentMinAspectRatio.IsSet())
		{
			float CurrentWidth = FMath::Min(AllottedGeometry.Size.X, ChildSlot.GetWidget()->GetDesiredSize().X);
			float CurrentHeight = FMath::Min(AllottedGeometry.Size.Y, ChildSlot.GetWidget()->GetDesiredSize().Y);

			float MinAspectRatioWidth = CurrentMinAspectRatio.IsSet() ? CurrentMinAspectRatio.Get() : 0;
			float MaxAspectRatioWidth = CurrentMaxAspectRatio.IsSet() ? CurrentMaxAspectRatio.Get() : 0;
			if (CurrentHeight > 0 && CurrentWidth > 0)
			{
				const float CurrentRatioWidth = (AllottedGeometry.GetLocalSize().X / AllottedGeometry.GetLocalSize().Y);

				bool bFitMaxRatio = (CurrentRatioWidth > MaxAspectRatioWidth && MaxAspectRatioWidth != 0);
				bool bFitMinRatio = (CurrentRatioWidth < MinAspectRatioWidth && MinAspectRatioWidth != 0);
				if (bFitMaxRatio || bFitMinRatio)
				{
					XAlignmentResult = AlignChild<Orient_Horizontal>(AllottedGeometry.GetLocalSize().X, ChildSlot, SlotPadding);
					YAlignmentResult = AlignChild<Orient_Vertical>(AllottedGeometry.GetLocalSize().Y, ChildSlot, SlotPadding);

					float NewWidth;
					float NewHeight;

					if (bFitMaxRatio)
					{
						const float MaxAspectRatioHeight = 1.0f / MaxAspectRatioWidth;
						NewWidth = MaxAspectRatioWidth * XAlignmentResult.Size;
						NewHeight = MaxAspectRatioHeight * NewWidth;
					}
					else
					{
						const float MinAspectRatioHeight = 1.0f / MinAspectRatioWidth;
						NewWidth = MinAspectRatioWidth * XAlignmentResult.Size;
						NewHeight = MinAspectRatioHeight * NewWidth;
					}

					const float MaxWidth = AllottedGeometry.Size.X - SlotPadding.GetTotalSpaceAlong<Orient_Horizontal>();
					const float MaxHeight = AllottedGeometry.Size.Y - SlotPadding.GetTotalSpaceAlong<Orient_Vertical>();

					if ( NewWidth > MaxWidth )
					{
						float Scale = MaxWidth / NewWidth;
						NewWidth *= Scale;
						NewHeight *= Scale;
					}

					if ( NewHeight > MaxHeight )
					{
						float Scale = MaxHeight / NewHeight;
						NewWidth *= Scale;
						NewHeight *= Scale;
					}

					XAlignmentResult.Size = NewWidth;
					YAlignmentResult.Size = NewHeight;

					XAlignmentResult.Offset = (MaxWidth - NewWidth) / 2.0f;
					YAlignmentResult.Offset = (MaxHeight - NewHeight) / 2.0f;
					
					bAlignChildren = false;
				}
			}
		}

		if ( bAlignChildren )
		{
			XAlignmentResult = AlignChild<Orient_Horizontal>(AllottedGeometry.GetLocalSize().X, ChildSlot, SlotPadding);
			YAlignmentResult = AlignChild<Orient_Vertical>(AllottedGeometry.GetLocalSize().Y, ChildSlot, SlotPadding);
		}

		const float AlignedSizeX = XAlignmentResult.Size;
		const float AlignedSizeY = YAlignmentResult.Size;

		ArrangedChildren.AddWidget(
			AllottedGeometry.MakeChild(
				ChildSlot.GetWidget(),
				FVector2D(XAlignmentResult.Offset, YAlignmentResult.Offset),
				FVector2D(AlignedSizeX, AlignedSizeY)
			)
		);
	}
}

void SAspectRatioConstraintBox::SetDesiredAspectRatio(TAttribute<FOptionalSize> InDesiredAspectRatio)
{
	SetAttribute(DesiredAspectRatio, InDesiredAspectRatio, EInvalidateWidgetReason::Layout);
}
