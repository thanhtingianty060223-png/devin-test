// Copyright Void Interactive, 2021

#include "VIPTriggerBox.h"

#include "GameModes/VIPEscortGM.h"
#include "GameModes/VIPEscortGS.h"

#include "Components/ObjectiveMarkerComponent.h"

AVIPTriggerBox::AVIPTriggerBox()
{
#if WITH_EDITOR
	TextRender->Text = FText::FromString("VIP Trigger");
#endif
	
	OnlyAcceptActorsWithTags.Empty();
	OnlyAcceptActorsWithTags.Add("VIP");

	OnlyAcceptTeams.Empty();
	OnlyAcceptTeams.Add(ETeamType::TT_NONE);
}

void AVIPTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	AVIPEscortGM* VIPGM = Cast<AVIPEscortGM>(UGameplayStatics::GetGameMode(this));
	AVIPEscortGS* VIPGS = Cast<AVIPEscortGS>(UGameplayStatics::GetGameState(this));

	// This trigger box should only work in VIP gamemodes and gamestates
	if (!VIPGM && !VIPGS)
	{
		Destroy();
		
		return;
	}
}

void AVIPTriggerBox::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	ShowObjectiveMarker();
}

void AVIPTriggerBox::OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;

	if (!OtherActor)
		return;
	
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (IsPlayerOnAcceptedTeam(Player))
	{
		if (OtherActor != this)
		{
			if (!GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry) && DoesActorHaveAnyAcceptedTags(OtherActor))
			{
				StartTimerEvent();

				bEntered = true;
			}

			CharactersInTriggerBox.Add(Player);
		
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has entered " + GetName());
			#endif
		}

		if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player->GetPlayerState()))
		{
			if (PS->bIsVIP)
				HideObjectiveMarker();
		}
	}
}

void AVIPTriggerBox::OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;

	if (!OtherActor)
		return;
	
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (IsPlayerOnAcceptedTeam(Player))
	{
		if (OtherActor != this)
		{
			CharactersInTriggerBox.Remove(Player);
			
			if (CharactersInTriggerBox.Num() == 0 || DoesActorHaveAnyAcceptedTags(Player))
			{
				bEntered = false;
				PreviousTimeElapsed = TimeElapsed;
				TimeElapsed = 0.0f;

				CancelTimerEvent();
			}
			
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has exited " + GetName() + ". Time to trigger box: " + FString::SanitizeFloat(PreviousTimeElapsed) + " seconds");
			#endif
		}

		if (!Player->IsArrested())
			ShowObjectiveMarker();
	}
}

bool AVIPTriggerBox::IsVIPInTriggerBox(APlayerCharacter*& OutVIPCharacter) const
{
	for (auto Character : CharactersInTriggerBox)
	{
		if (DoesActorHaveAnyAcceptedTags(Character))
		{
			OutVIPCharacter = Character;
			return true;
		}
	}

	return false;
}

void AVIPTriggerBox::ShowObjectiveMarker()
{
	if (!ObjectiveMarkerComponent)
		return;
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (AVIPEscortGS* VIPGS = Cast<AVIPEscortGS>(UReadyOrNotStatics::GetReadyOrNotGameState()))
		{
			// If we're on the same team as the VIP and we're not in this trigger box, show the objective marker
			if (PlayerCharacter->GetTeam() == VIPGS->CurrentVIPTeam && !IsActorInTriggerBox(PlayerCharacter))
			{
				ObjectiveMarkerComponent->ShowObjectiveMarker();
			}
			// Otherwise, hide it
			else
			{
				ObjectiveMarkerComponent->HideObjectiveMarker();
			}
		}
	}
	else
	{
		ObjectiveMarkerComponent->HideObjectiveMarker();
	}
}
