#include "HighgroundVolume.h"
#include "ReadyOrNot.h"
#include "Characters/PlayerCharacter.h"

AHighgroundVolume::AHighgroundVolume()
{
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &AHighgroundVolume::OnOverlapBegin);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &AHighgroundVolume::OnOverlapEnd);
	RootComponent = Bounds;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Speaker"));
	AudioComp->SetupAttachment(Bounds);
	AudioComp->OnAudioFinished.AddDynamic(this, &AHighgroundVolume::OnAudioFinished);
}

void AHighgroundVolume::EnableHighgroundVolume(int32 NewSierraDesignation)
{
	SierraDesignation = NewSierraDesignation;
	bWatching = true;
}

void AHighgroundVolume::StartAudio(USoundBase* Audio)
{
	bPlayingSoundCurrent = true;
	AudioComp->SetSound(Audio);
	AudioComp->SetUISound(true);
	AudioComp->Play();
}

void AHighgroundVolume::OnAudioFinished()
{
	LastSoundFinished = GetWorld()->GetTimeSeconds();
	bPlayingSoundCurrent = false;
}

void AHighgroundVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	int32 RandomPick;

	if (!bWatching)
	{
		// Round hasn't started yet, or we aren't configured
		return;
	}

	if (bPlayingSoundCurrent || LastSoundFinished + AudioDebounce > GetWorld()->GetTimeSeconds())
	{
		// Either we are currently playing a sound or we are waiting on the timer to finish up, so don't do anything.
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (!pc)
	{
		// We only concern ourselves with AI and players
		return;
	}

	switch (pc->GetTeam())
	{
		case ETeamType::TT_CIVILIAN:
		case ETeamType::TT_SUSPECT:
			if (ContactEnteredVolumeAudio.Num() == 0)
			{
				return;
			}
			RandomPick = FMath::RandRange(0, ContactEnteredVolumeAudio.Num() - 1);
			StartAudio(ContactEnteredVolumeAudio[RandomPick]);
			break;
		default:
			if (bPlayedEntryTeamEnteringSound)
			{
				return;
			}
			if (SwatEnteredVolumeAudio.Num() == 0)
			{
				return;
			}
			RandomPick = FMath::RandRange(0, SwatEnteredVolumeAudio.Num() - 1);
			StartAudio(SwatEnteredVolumeAudio[RandomPick]);
			bPlayedEntryTeamEnteringSound = true;
			break;
	}
}

void AHighgroundVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	int32 RandomPick;
	if (!bWatching)
	{
		// Round hasn't started yet, or we aren't configured
		return;
	}

	if (bPlayingSoundCurrent || LastSoundFinished + AudioDebounce > GetWorld()->GetTimeSeconds())
	{
		// Either we are currently playing a sound or we are waiting on the timer to finish up, so don't do anything.
		return;
	}

	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (!pc)
	{
		// We only concern ourselves with AI and players
		return;
	}

	switch (pc->GetTeam())
	{
		case ETeamType::TT_CIVILIAN:
		case ETeamType::TT_SUSPECT:
			if (ContactExitedVolumeAudio.Num() == 0)
			{
				return;
			}
			RandomPick = FMath::RandRange(0, ContactExitedVolumeAudio.Num() - 1);
			StartAudio(ContactExitedVolumeAudio[RandomPick]);
			break;
		default:
			if (bPlayedEntryTeamExitingSound)
			{
				return;
			}
			if (SwatExitedVolumeAudio.Num() == 0)
			{
				return;
			}
			RandomPick = FMath::RandRange(0, SwatExitedVolumeAudio.Num() - 1);
			StartAudio(SwatExitedVolumeAudio[RandomPick]);
			bPlayedEntryTeamExitingSound = true;
			break;
	}
}
