// Copyright Void Interactive, 2023

#include "CheckpointActivityTriggerVolume.h"
#include "GameModes/TrainingGM.h"

ACheckpointActivityTriggerVolume::ACheckpointActivityTriggerVolume()
{
#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

	if (!IsRunningCommandlet())
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_CheckPoint.T_CheckPoint'"));

		if (GetSpriteComponent())
		{
			GetSpriteComponent()->SetSprite(Icon.Object);
		}

		if (ArrowComponent)
		{
			ArrowComponent->ArrowColor = FColor(150, 200, 255);

			ArrowComponent->ArrowSize = 1.0f;
			ArrowComponent->bTreatAsASprite = true;
			ArrowComponent->SetupAttachment(RootComponent);
			ArrowComponent->bIsScreenSizeScaled = true;
		}
	}
#endif
}

void ACheckpointActivityTriggerVolume::Activate()
{
	Super::Activate();

	// Set the active checkpoint in the game mode.
	if (ATrainingGM* TrainingGM = GetWorld()->GetAuthGameMode<ATrainingGM>())
	{
		TrainingGM->CurrentCheckpoint = this;
	}
}
