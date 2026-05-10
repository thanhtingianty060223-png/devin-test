#include "TugOfWarButton.h"
#include "ReadyOrNot.h"
#include "GameModes/TugOfWarGS.h"

ATugOfWarButton::ATugOfWarButton() : Super()
{
	bHoldButtonPrompt = true;
	bCompleteIcon = false;
}

void ATugOfWarButton::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{	// Tug of War buttons only spawn in Tug of War
		ATugOfWarGS* gs = GetWorld()->GetGameState<ATugOfWarGS>();
		if (!gs)
		{
			Destroy();
			return;
		}
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		// we only need to bind these on the server, they aren't used on the client
		InfluencerKilledDelegate.BindUFunction(this, "OnInfluencerKilled");
		InfluencerArrestedDelegate.BindUFunction(this, "OnInfluencerArrested");
		InfluencerStunnedDelegate.BindUFunction(this, "OnInfluencerStunned");
	}
}

bool ATugOfWarButton::CanBeUsedNow_Implementation(AActor* PotentialUser)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(PotentialUser);
	if (!pc)
	{
		return false;
	}

	if (OnlyTeamUse != ETeamType::TT_NONE)
	{
		if (pc->GetTeam() != OnlyTeamUse)
		{
			return false;
		}
	}

	return bCanUseNow;
}

bool ATugOfWarButton::CanUse_Implementation(class APlayerCharacter* User)
{
	if (!User)
	{
		return false;
	}

	if (OnlyTeamUse != ETeamType::TT_NONE)
	{
		if (User->GetTeam() != OnlyTeamUse)
		{
			return false;
		}
	}

	return bCanUseNow;
}

void ATugOfWarButton::Server_TryUse_Implementation(AActor* User)
{
	if (!CanBeUsedNow(User))
	{
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(User);
	if (!pc)
	{
		return;
	}

	if (!Mover)
	{
		return;
	}
	
	if (CurrentUser)
	{
		return;
	}

	CurrentUser = pc;

	// add events for when specific stuff happens to the influencer here
	pc->OnCharacterKilled.AddUnique(InfluencerKilledDelegate);
	pc->OnPlayerArrestStart.AddUnique(InfluencerArrestedDelegate);
	pc->OnStunnedEvent.AddUnique(InfluencerStunnedDelegate);

	// add this person to the list of influencers
	Mover->Influencers.AddUnique(pc);
	Mover->UpdateMovement();
}

void ATugOfWarButton::Server_EndUse_Implementation(AActor* User)
{
	if (!CanBeUsedNow(User))
	{
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(User);
	if (!pc)
	{
		return;
	}

	if (!Mover)
	{
		return;
	}

	if (pc == CurrentUser)
	{
		CurrentUser = nullptr;
	}

	// remove this person from the list of influencers
	if (Mover->Influencers.Contains(pc))
	{
		Mover->Influencers.Remove(pc);
		Mover->UpdateMovement();
	}
}

void ATugOfWarButton::OnInfluencerKilled(AActor* Causer, ACharacter* InstigatorCharacter, ACharacter* KilledCharacter, struct FDamageEvent const& DamageEvent, APlayerState* LastPlayerState)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(KilledCharacter);
	if (!pc)
	{
		return;
	}

	RemoveInfluencer(pc);
}

void ATugOfWarButton::OnInfluencerArrested(APlayerCharacter* ArrestedCharacter, APlayerCharacter* InstigatorCharacter)
{
	RemoveInfluencer(ArrestedCharacter);
}

void ATugOfWarButton::OnInfluencerStunned(APlayerCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	RemoveInfluencer(StunnedCharacter);
}

void ATugOfWarButton::RemoveInfluencer(APlayerCharacter* Influencer)
{
	if (!Mover)
	{
		return;
	}

	if (Influencer == CurrentUser)
	{
		CurrentUser = nullptr;
	}

	if (Mover->Influencers.Contains(Influencer))
	{
		Mover->Influencers.Remove(Influencer);
		Mover->UpdateMovement();
	}
}