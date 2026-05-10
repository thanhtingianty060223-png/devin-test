#include "CommandInterface.h"

void UCommandInterface::EnableSelectionWidgets(FVector2D Selection)
{
	SelectionRight->SetIsEnabled(Selection.X > 0.0f);
	SelectionLeft->SetIsEnabled(Selection.X < 0.0f);
	SelectionUp->SetIsEnabled(Selection.Y > 0.0f);
	SelectionDown->SetIsEnabled(Selection.Y < 0.0f);
}

void UCommandInterface::DisableSelectionWidgets()
{
	SelectionRight->SetIsEnabled(false);
	SelectionLeft->SetIsEnabled(false);
	SelectionUp->SetIsEnabled(false);
	SelectionDown->SetIsEnabled(false);
}

void UCommandInterface::SetRedColor()
{
	SetColor(FLinearColor(FColor(198, 108, 139)));
}

void UCommandInterface::SetBlueColor()
{
	SetColor(FLinearColor(FColor(76, 76, 187)));
}

void UCommandInterface::SetGoldColor()
{
	SetColor(FLinearColor(FColor(243, 233, 135)));
}

void UCommandInterface::SetColor(FLinearColor Color)
{
	MoveIn->SetColorAndOpacity(Color);
	FallIn->SetColorAndOpacity(Color);
	Cover->SetColorAndOpacity(Color);
	Deploy->SetColorAndOpacity(Color);
}
