#include "Referendum.h"
#include "ReadyOrNot.h"

AReferendum::AReferendum()
{
	bAlwaysRelevant = true;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AReferendum::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReferendum, YesVotes);
	DOREPLIFETIME(AReferendum, NoVotes);
	DOREPLIFETIME(AReferendum, ReferendumTimeRemaining);
	DOREPLIFETIME(AReferendum, bReferendumRunning);
	DOREPLIFETIME(AReferendum, ReferendumCaller);
	DOREPLIFETIME(AReferendum, ReferendumHoldingTimeRemaining);
}

void AReferendum::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	// all player controllers are considered eligible voters
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AReadyOrNotPlayerController::StaticClass(), Actors);
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		AReadyOrNotPlayerController* Controller = Cast<AReadyOrNotPlayerController>(Actors[i]);
		if (!Controller)
		{
			continue;
		}

		EligibleVoters.Add(Controller);
	}

	// Run the referendum
	ReferendumTimeRemaining = ReferendumTime;
	bReferendumRunning = true;
}

void AReferendum::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	if (bReferendumWaitingToTakeEffect)
	{
		if (ReferendumHoldingTimeRemaining > 0.0f)
		{
			ReferendumHoldingTimeRemaining -= DeltaSeconds;
		}
		else
		{
			OnReferendumPassed();
			bReferendumWaitingToTakeEffect = false;

			AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
			if (gs)
			{
				gs->CurrentReferendum = nullptr;
				PrimaryActorTick.bCanEverTick = false;
			}
			return;
		}
	}
	else if (ReferendumTimeRemaining > 0.0f)
	{
		ReferendumTimeRemaining -= DeltaSeconds;
		if (ReferendumTimeRemaining <= 0.0f)
		{
			// out of time
			if (YesVotes > NoVotes)
			{
				Multicast_AnnounceVotePassed();
				bReferendumWaitingToTakeEffect = true;
			}
			else
			{
				Multicast_AnnounceVoteFailed();
			}
			bReferendumRunning = false;
			return;
		}
	}
}

void AReferendum::CastedYesVote(class AReadyOrNotPlayerController* Voter)
{
	if (!bReferendumRunning)
	{
		return;
	}

	if (YesVoters.Contains(Voter) || !EligibleVoters.Contains(Voter))
	{
		return;
	}
	else if (NoVoters.Contains(Voter))
	{
		NoVoters.Remove(Voter);
	}

	YesVoters.AddUnique(Voter);
	YesVotes = YesVoters.Num();

	if (YesVotes > EligibleVoters.Num() / 2)
	{	// the vote succeeded
		Multicast_AnnounceVotePassed();
		bReferendumRunning = false;
		bReferendumWaitingToTakeEffect = true;
		return;
	}
}

void AReferendum::CastedNoVote(class AReadyOrNotPlayerController* Voter)
{
	if (!bReferendumRunning)
	{
		return;
	}

	if (NoVoters.Contains(Voter) || !EligibleVoters.Contains(Voter))
	{
		return;
	}
	else if (YesVoters.Contains(Voter))
	{
		YesVoters.Remove(Voter);
	}

	NoVoters.AddUnique(Voter);
	NoVotes = NoVoters.Num();

	if (NoVotes > EligibleVoters.Num() / 2)
	{	// the vote failed
		Multicast_AnnounceVoteFailed();
		bReferendumRunning = false;
		return;
	}
}

// Multicast event - announce that the vote has started
void AReferendum::Multicast_AnnounceVoteStarted_Implementation(AReadyOrNotPlayerState* CallingVoter)
{
	OnAnnounceVoteStarted(CallingVoter);
}

// Multicast event - announce that the vote passed
void AReferendum::Multicast_AnnounceVotePassed_Implementation()
{
	OnAnnounceVotePassed();
}

// Multicast event - announce that the vote failed
void AReferendum::Multicast_AnnounceVoteFailed_Implementation()
{
	OnAnnounceVoteFailed();
}

// Multicast event - announce that someone has voted yes
void AReferendum::Multicast_AnnounceYesVote_Implementation(AReadyOrNotPlayerState* Voter)
{
	OnAnnounceYesVote(Voter);
}

// Multicast event - announce that someone has voted no
void AReferendum::Multicast_AnnounceNoVote_Implementation(AReadyOrNotPlayerState* Voter)
{
	OnAnnounceNoVote(Voter);
}