// Copyright Void Interactive, 2023


#include "AICombatMovesWidget.h"

#include "AICombatMovesWidgetEntry.h"
#include "AISelectionDebugWidget.h"
#include "Characters/CyberneticController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Info/Activities/BaseCombatActivity.h"

void UAICombatMovesWidget::SetAIToFocus(ACyberneticCharacter* CyberneticCharacter)
{
	if (!IsValid(CyberneticCharacter))
		return;

	if (IsValid(CurrentAI) && IsValid(CurrentAI->DebugAISelectionWidget))
		CurrentAI->DebugAISelectionWidget->DestroyComponent();

	CurrentAI = CyberneticCharacter;

	AIName_TextBlock->SetText(FText::FromString(CurrentAI->GetName()));

	CombatMoves_VerticalBox->ClearChildren();

	UAIArchetypeData* ArchetypeData = CurrentAI->Archetype;
	if (!ArchetypeData)
	{
		OnNextAIButtonClicked();
		return;
	}

	for (FAIActionDataContainer ActionData : ArchetypeData->CombatMoveActions)
	{
		UAICombatMovesWidgetEntry* NewEntry = CreateWidget<UAICombatMovesWidgetEntry>(this, WidgetEntryClass, ActionData.Name);
		NewEntry->CombatAction_TextBlock->SetText(FText::FromName(ActionData.Name));
		CombatMoves_VerticalBox->AddChildToVerticalBox(NewEntry);
	}

	if (!IsValid(CurrentAI->DebugAISelectionWidget))
	{
		FTransform WidgetTransform = FTransform::Identity;
		WidgetTransform.SetLocation(FVector(0, 0, 20));
		CurrentAI->DebugAISelectionWidget = Cast<UWidgetComponent>(CurrentAI->AddComponentByClass(UWidgetComponent::StaticClass(), false, WidgetTransform, false));
		CurrentAI->DebugAISelectionWidget->SetWidgetClass(AIWorldWidgetClass);
		CurrentAI->DebugAISelectionWidget->SetWidgetSpace(EWidgetSpace::Screen);
		CurrentAI->DebugAISelectionWidget->InitWidget();
	}

	UAISelectionDebugWidget* AIWidget = Cast<UAISelectionDebugWidget>(CurrentAI->DebugAISelectionWidget->GetWidget());
	if (AIWidget->AIName_TextBlock)
		AIWidget->AIName_TextBlock->SetText(FText::FromString(CurrentAI->GetName()));	
}

void UAICombatMovesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (!IsValid(CurrentAI) || !IsValid(CurrentAI->GetCyberneticsController()))
		return;

	ACyberneticController* Controller = CurrentAI->GetCyberneticsController();
	if (!Controller)
		return;

	UAIArchetypeData* ArchetypeData = CurrentAI->Archetype;
	if (!ArchetypeData)
		return;

	for (int32 i = 0; i < ArchetypeData->CombatMoveActions.Num(); i++)
	{
		UAICombatMovesWidgetEntry* CombatMovesWidgetEntry = Cast<UAICombatMovesWidgetEntry>(CombatMoves_VerticalBox->GetChildAt(i));
		if (!CombatMovesWidgetEntry)
			continue;

		bool bActiveCombatMove = false;
		bool bLastBegunCombatMove = false;
		bActiveCombatMove = Controller->CombatActivity->ActiveCombatMoveAction && (ArchetypeData->CombatMoveActions[i].Name.ToString() == Controller->CombatActivity->ActiveCombatMoveAction->Name.ToString() || ArchetypeData->CombatMoveActions[i].Name.ToString() == Controller->CombatActivity->ActiveCombatMoveAction->Name.ToString() + " (PRESET)");
		bLastBegunCombatMove = Controller->CombatActivity->LastBegunCombatMoveAction && (ArchetypeData->CombatMoveActions[i].Name.ToString() == Controller->CombatActivity->LastBegunCombatMoveAction->Name.ToString() || ArchetypeData->CombatMoveActions[i].Name.ToString() == Controller->CombatActivity->LastBegunCombatMoveAction->Name.ToString() + " (PRESET)");

		int32 SuccessfulConsiderations = Controller->CombatActivity->GetConsiderationsCount(&ArchetypeData->CombatMoveActions[i].GetActionData());
		int32 FailedConsiderations = Controller->CombatActivity->GetConsiderationsCount(&ArchetypeData->CombatMoveActions[i].GetActionData(), false);
		
		
		// Checking twice as some moves have " (PRESET)" appended to the combat move name
		float* StartTime = Controller->CombatActivity->CombatMoveActionsStartTime.Find(ArchetypeData->CombatMoveActions[i].Name);
		if (!StartTime)
		{
			FString MoveNameAsString = ArchetypeData->CombatMoveActions[i].Name.ToString();
			MoveNameAsString.RemoveFromEnd(" (PRESET)");
			StartTime = Controller->CombatActivity->CombatMoveActionsStartTime.Find(FName(*MoveNameAsString));
		}

		float* EndTime = Controller->CombatActivity->CombatMoveActionsEndTime.Find(ArchetypeData->CombatMoveActions[i].Name);
		if (!EndTime)
		{
			FString MoveNameAsString = ArchetypeData->CombatMoveActions[i].Name.ToString();
			MoveNameAsString.RemoveFromEnd(" (PRESET)");
			EndTime = Controller->CombatActivity->CombatMoveActionsEndTime.Find(FName(*MoveNameAsString));
		}

		float Runtime = 0;

		if (StartTime)
		{
			Runtime = FMath::Max(bActiveCombatMove ? GetWorld()->GetTimeSeconds() - *StartTime : EndTime ? *EndTime - *StartTime : 0.f, 0.f);
		}

		FText SuccessText = FText::FromString(FString::Printf(TEXT("%d"), SuccessfulConsiderations));
		FText FailText = FText::FromString(FString::Printf(TEXT("%d"), FailedConsiderations));
		FText RuntimeText = FText::FromString(FString::Printf(TEXT("%f"), Runtime));
		CombatMovesWidgetEntry->SuccessfulConsiderations_TextBlock->SetText(SuccessText);
		CombatMovesWidgetEntry->FailedConsiderations_TextBlock->SetText(FailText);
		CombatMovesWidgetEntry->RunTime_TextBlock->SetText(RuntimeText);

		FLinearColor Color = bActiveCombatMove ? FLinearColor(0, 255, 0) : bLastBegunCombatMove ? FLinearColor(255, 255, 0) : FLinearColor(255, 255, 255);
		CombatMovesWidgetEntry->CombatAction_TextBlock->SetColorAndOpacity(FSlateColor(Color));		
	}
}

void UAICombatMovesWidget::OnNextAIButtonClicked()
{
	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!IsValid(GameState))
		return;

	int32 StartIndex = 0;

	if (IsValid(CurrentAI) && GameState->AllAICharacters.Num())
	{
		StartIndex = GameState->AllAICharacters.IndexOfByKey(CurrentAI);
	}
	
	for (int32 i = StartIndex; i < GameState->AllAICharacters.Num(); i++)
	{
		ACyberneticCharacter* Character = GameState->AllAICharacters[i];
		if (!IsValid(Character) || Character->IsOnSWATTeam() || Character == CurrentAI)
			continue;

		SetAIToFocus(GameState->AllAICharacters[i]);
		return;
	}

	for (int32 i = 0; i < StartIndex; i++)
	{
		if (!IsValid(GameState->AllAICharacters[i]) || GameState->AllAICharacters[i]->IsOnSWATTeam())
			continue;

		SetAIToFocus(GameState->AllAICharacters[i]);
		return;
	}
}
