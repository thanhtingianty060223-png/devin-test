// Void Interactive, 2020

#include "IncriminationTriggerBox.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

#include "Actors/Gameplay/EvidenceActor.h"

AIncriminationTriggerBox::AIncriminationTriggerBox()
{
#if WITH_EDITOR
	TextRender->Text = FText::FromString("Incrimination Trigger");
#endif

	bAlwaysRelevant = true;
	
	OnlyAcceptActorsWithTags.Empty();

	OnlyAcceptTeams.Empty();
	OnlyAcceptTeams.Add(ETeamType::TT_SERT_BLUE);
	OnlyAcceptTeams.Add(ETeamType::TT_SERT_RED);
}

void AIncriminationTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}

void AIncriminationTriggerBox::OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor)
		return;
	
	AIncriminationGS* IncriminationGS = Cast<AIncriminationGS>(UGameplayStatics::GetGameState(this));

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);

	if (Player)
		CharactersInTriggerBox.Add(Player);
	
	if (Player && !Player->IsDeadOrUnconscious() && Player->GetTeam() == IncriminationGS->PickupTeam)
	{
		if (OtherActor != this)
		{
			if ((!GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry) && DoesActorHaveAnyAcceptedTags(Player)) || !GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry))
			{
				StartTimerEvent();

				bEntered = true;
			}

		
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has entered " + GetName());
			#endif
		}
	}
}

void AIncriminationTriggerBox::OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{	
	if (!OtherActor)
		return;

	AIncriminationGS* IncriminationGS = Cast<AIncriminationGS>(UGameplayStatics::GetGameState(this));

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);

	if (Player)
		CharactersInTriggerBox.Remove(Player);

	if (Player && !Player->IsDeadOrUnconscious() && Player->GetTeam() == IncriminationGS->PickupTeam)
	{
		if (OtherActor != this)
		{
			if (CharactersInTriggerBox.Num() == 0 || DoesActorHaveAnyAcceptedTags(Player) || GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry))
			{
				bEntered = false;
				PreviousTimeElapsed = TimeElapsed;
				TimeElapsed = 0.0f;

				CancelTimerEvent();
			}
			
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has exited " + GetName() + ". Time in trigger box: " + FString::SanitizeFloat(PreviousTimeElapsed) + " seconds");
			#endif
		}
	}
}
