// Void Interactive, 2020

#include "TeamPaperdollWidget.h"

#include "Components/Image.h"

void UTeamPaperdollWidget::InitializeWidget_Implementation(const ETeamType InTeam)
{
	switch (InTeam)
	{
		case ETeamType::TT_NONE:
			UpdatePaperdollColor(FColor::White);
		break;

		case ETeamType::TT_SERT_RED:
			UpdatePaperdollColor(FColor::FromHex("FF2C35"));
		break;

		case ETeamType::TT_SERT_BLUE:
			UpdatePaperdollColor(FColor::FromHex("1C85F4"));
		break;

		case ETeamType::TT_SUSPECT:
			UpdatePaperdollColor(FColor::FromHex("FF2C35"));
		break;

		case ETeamType::TT_CIVILIAN:
			UpdatePaperdollColor(FColor::FromHex("1C85F4"));
		break;

		case ETeamType::TT_SQUAD:
			UpdatePaperdollColor(FColor::FromHex("F4D143"));
		break;

		default:
			UpdatePaperdollColor(FColor::White);
		break;
	}
}

void UTeamPaperdollWidget::UpdatePaperdollColor(const FLinearColor& Color)
{
	Paperdoll_Image->SetColorAndOpacity(Color);
}
