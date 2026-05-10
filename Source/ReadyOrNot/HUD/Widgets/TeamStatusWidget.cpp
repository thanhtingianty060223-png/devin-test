// Void Interactive, 2020

#include "TeamStatusWidget.h"

#include "HUD/Widgets/TeamPaperdollWidget.h"

#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

UTeamStatusWidget::UTeamStatusWidget()
{
#if !UE_SERVER
	static ConstructorHelpers::FObjectFinder<UFont> FontObject(TEXT("Font'/Game/ReadyOrNot/Assets/Font/BebasNeue-Regular_Font.BebasNeue-Regular_Font'"));
	EmptyTeamTextFont.FontObject = FontObject.Object;
	EmptyTeamTextFont.TypefaceFontName = "Default";
	EmptyTeamTextFont.Size = 24;
#endif
}

void UTeamStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateTeamEmblemImage(TeamEmblemBrush);
}

void UTeamStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeTeam();
	UpdateTeamStatus();
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UTeamStatusWidget::UpdateTeamStatus, 1.0f, true, true);

	ensureAlways(PaperdollWidgetClass);
}

void UTeamStatusWidget::InitializeTeam_Implementation()
{
	Teammates_Container_LeftAligned->ClearChildren();
	Teammates_Container_RightAligned->ClearChildren();

	UReadyOrNotFunctionLibrary::RemoveFromParentAndClear(TeamPaperdolls);

	for (int32 i = 0; i < MAX_GAMEMODE_PLAYERS; i++)
	{
		if (UTeamPaperdollWidget* PaperdollWidget = CreateWidget<UTeamPaperdollWidget>(GetOwningPlayer(), PaperdollWidgetClass))
		{
			PaperdollWidget->InitializeWidget(Team);
			PaperdollWidget->SetVisibility(ESlateVisibility::Collapsed);

			switch (Alignment)
			{
				case HAlign_Left:
					Teammates_Container_LeftAligned->AddChildToHorizontalBox(PaperdollWidget);
				break;

				case HAlign_Right:
					Teammates_Container_RightAligned->AddChildToHorizontalBox(PaperdollWidget);
				break;
		
				default:
					Teammates_Container_LeftAligned->AddChildToHorizontalBox(PaperdollWidget);
				break;
			}
			
			TeamPaperdolls.Add(PaperdollWidget);
		}
	}

	if (EmptyTeam_Text)
	{
		EmptyTeam_Text->RemoveFromParent();
		EmptyTeam_Text = nullptr;
	}
	
	EmptyTeam_Text = CreateEmptyTeamText();
	EmptyTeam_Text->SetVisibility(ESlateVisibility::Collapsed);
}

UTextBlock* UTeamStatusWidget::CreateEmptyTeamText()
{
	if (UTextBlock* Text = NewObject<UTextBlock>(this))
	{
		Text->SetColorAndOpacity(EmptyTeamTextColor);
		Text->SetFont(EmptyTeamTextFont);
		Text->SetText(EmptyTeamText);

		switch (Alignment)
		{
			case HAlign_Left:
				if (UHorizontalBoxSlot* HBS = Teammates_Container_LeftAligned->AddChildToHorizontalBox(Text))
				{
					HBS->SetVerticalAlignment(VAlign_Center);
				}
			break;

			case HAlign_Right:
				if (UHorizontalBoxSlot* HBS = Teammates_Container_RightAligned->AddChildToHorizontalBox(Text))
				{
					HBS->SetVerticalAlignment(VAlign_Center);
				}
			break;
		
			default:
				if (UHorizontalBoxSlot* HBS = Teammates_Container_LeftAligned->AddChildToHorizontalBox(Text))
				{
					HBS->SetVerticalAlignment(VAlign_Center);
				}
			break;
		}

		return Text;
	}

	return nullptr;
}

void UTeamStatusWidget::UpdateTeamStatus()
{
	if (RONGS)
	{
		const TArray<AReadyOrNotPlayerState*> PlayerStates = UReadyOrNotFunctionLibrary::GetActorsOfClass<AReadyOrNotPlayerState>(GetWorld());
		TArray<AReadyOrNotPlayerState*> FilteredPlayerStates;
		for (AReadyOrNotPlayerState* PS : PlayerStates)
		{
			if (PS->GetTeam() == Team)
			{
				FilteredPlayerStates.Add(PS);
			}
		}
		
		const int32 NumOfTeammates = FilteredPlayerStates.Num();
		if (NumOfTeammates == 0)
		{
			if (EmptyTeam_Text)
				EmptyTeam_Text->SetVisibility(ESlateVisibility::HitTestInvisible);

			for (UTeamPaperdollWidget* PaperdollWidget : TeamPaperdolls)
			{
				PaperdollWidget->SetVisibility(ESlateVisibility::Collapsed);
			}

			return;
		}

		if (EmptyTeam_Text)
			EmptyTeam_Text->SetVisibility(ESlateVisibility::Collapsed);
		
		for (int32 i = 0; i < NumOfTeammates; i++)
		{
			if (TeamPaperdolls.IsValidIndex(i))
				TeamPaperdolls[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		
		for (int32 i = NumOfTeammates; i < MAX_GAMEMODE_PLAYERS; i++)
		{
			if (TeamPaperdolls.IsValidIndex(i))
				TeamPaperdolls[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTeamStatusWidget::UpdateTeamEmblemImage(const FSlateBrush& Brush)
{
	TeamEmblem_Image_LeftAligned->SetBrush(Brush);
	TeamEmblem_Image_RightAligned->SetBrush(Brush);
}
