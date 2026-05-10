#include "MusicSequencerUnreal.h"
#include "ReadyOrNot.h"

AMusicSequencerUnreal::AMusicSequencerUnreal() : Super()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioPlayer"));
	AudioPlayer->SetupAttachment(Scene);

	AudioPlayer->OnAudioPlaybackPercent.AddDynamic(this, &AMusicSequencerUnreal::OnAudioPlaybackPercent);
	AudioPlayer->OnAudioFinished.AddDynamic(this, &AMusicSequencerUnreal::OnAudioFinished);
}

void AMusicSequencerUnreal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeRemaining -= DeltaSeconds;

	if (TimeRemaining < 0.0f && CurrentState != NextState && bFadingOut)
	{	// Stop the music if we are out of time. The transition will occur there.
		Multicast_StopAudio();
		Multicast_StopAudio_Implementation();
	}
	else if (TimeRemaining < 0.0f && CurrentState != NextState && !bFadingOut)
	{
		UMusicData* MusicData = GetMusicData();

		switch (CurrentState)
		{
			case EMusicState::MS_Action:
				StartFadeoutInternal(MusicData->ActionTrack.FadeTime);
				break;
			case EMusicState::MS_Ambient:
				StartFadeoutInternal(MusicData->AmbientTrack.FadeTime);
				break;
			case EMusicState::MS_Preplanning:
				StartFadeoutInternal(MusicData->PreplanningTrack.FadeTime);
				break;
		}
	}
}

void AMusicSequencerUnreal::Multicast_StopAudio_Implementation()
{
	AudioPlayer->Stop();
}

void AMusicSequencerUnreal::OnAudioPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	LastPlayTime = CurrentTrackDuration * PlaybackPercent;
}

void AMusicSequencerUnreal::StartFadeoutInternal(float FadeTime)
{
	AudioPlayer->FadeOut(FadeTime, 0.0f);
	bFadingOut = true;
	TimeRemaining = FadeTime;
}

void AMusicSequencerUnreal::PrimeTransitionInternal(float MinimumTime, const TArray<FMusicKeyframe>& Keyframes)
{
	float Threshold = LastPlayTime + MinimumTime;
	float TotalTime = 0.0f;
	if (Threshold >= CurrentTrackDuration)
	{	// Keep the cutoff to be within the bounds of the music track length
		TotalTime += (CurrentTrackDuration - LastPlayTime);
		Threshold -= CurrentTrackDuration;
	}

	// Find the next transition point 
	TotalTime += Threshold;
	for (int32 i = 0; i < Keyframes.Num(); i++)
	{
		if (!Keyframes[i].bTransitionExit)
		{
			continue;
		}

		if (Keyframes[i].Position < Threshold)
		{
			// This transition works.
			TotalTime += (Keyframes[i].Position - Threshold);
			bFadingOut = true;
			TimeRemaining = TotalTime;
			TransitionIndex = i;
			return;
		}
	}

	// This is bad, we should really be firing off warning flares because we couldn't find a proper transition.
}

void AMusicSequencerUnreal::TransitionForCurrentState()
{
	UMusicData* MusicData = GetMusicData();
	if (!MusicData)
	{
		return;
	}
	switch (CurrentState)
	{
	case EMusicState::MS_Action:
		if (MusicData->ActionTrack.bTransitionMusic)
		{
			PrimeTransitionInternal(MusicData->ActionTrack.MinimumTime, MusicData->ActionTrack.Keyframes);
		}
		else
		{
			StartFadeoutInternal(MusicData->ActionTrack.FadeTime);
		}
		break;

	case EMusicState::MS_Ambient:
		if (MusicData->AmbientTrack.bTransitionMusic)
		{
			PrimeTransitionInternal(MusicData->AmbientTrack.MinimumTime, MusicData->AmbientTrack.Keyframes);
		}
		else
		{
			StartFadeoutInternal(MusicData->AmbientTrack.FadeTime);
		}
		break;

	case EMusicState::MS_Preplanning:
		if (MusicData->PreplanningTrack.bTransitionMusic)
		{
			PrimeTransitionInternal(MusicData->PreplanningTrack.MinimumTime, MusicData->PreplanningTrack.Keyframes);
		}
		else
		{
			StartFadeoutInternal(MusicData->PreplanningTrack.FadeTime);
		}
		break;

	case EMusicState::MS_Scripted:
		// You shouldn't be calling this function. Call PlayScriptedMusic instead.
		return;
	}
}

void AMusicSequencerUnreal::Multicast_StartTransitioningToState_Implementation(EMusicState NewState)
{
	UMusicData* MusicData = GetMusicData();
	if (!MusicData)
	{
		return;
	}

	if (NextState == EMusicState::MS_Scripted)
	{	// Don't allow transitioning to new states when there's a scripted track about to play.
		return;
	}
	else if (CurrentState == NewState)
	{
		// Cancel the previous transition.
		if (NewState == EMusicState::MS_Action)
		{
			// ...unless we are in an action state. In which case, we simply adjust the time.
			PrimeTransitionInternal(MusicData->ActionTrack.MinimumTime, MusicData->ActionTrack.Keyframes);
			return;
		}
		bFadingOut = false;
		NextState = CurrentState;
		return;
	}
	else if (NextState == NewState)
	{
		// We've already got this state queued up, don't do anything.
		return;
	}

	TransitionForCurrentState();
	NextState = NewState;
}

void AMusicSequencerUnreal::PlayScriptedMusic(USoundCue* Music, EMusicState NewScriptedState, bool bImmediately)
{
	if (bImmediately)
	{
		NextState = NewScriptedState;
		CurrentState = EMusicState::MS_Scripted;
		AudioPlayer->SetVolumeMultiplier(1.0f); // fixme
		AudioPlayer->SetSound(Music);
		ScriptedAudio = Music;
		bFadingOut = false;
		bTransition = false;
	}
	else
	{
		NextState = EMusicState::MS_Scripted;
		TransitionForCurrentState();
	}
}

void AMusicSequencerUnreal::OnAudioFinished()
{
	UMusicData* MusicData = GetMusicData();
	if (!MusicData)
	{
		return;
	}

	if (!AudioPlayer)
	{
		return;
	}

	if (CurrentState != NextState)
	{
		// The audio has finished and there's another state queued up
		if (bTransition && !bPlayedTransition && TransitionIndex != -1)
		{
			// There's a transition. Let's play that first.
			switch (CurrentState)
			{
				case EMusicState::MS_Action:
					AudioPlayer->SetSound(MusicData->ActionTrack.Keyframes[TransitionIndex].TransitionPiece);
					CurrentTrackDuration = MusicData->ActionTrack.Keyframes[TransitionIndex].TransitionPiece->GetDuration();
					break;
				case EMusicState::MS_Ambient:
					AudioPlayer->SetSound(MusicData->AmbientTrack.Keyframes[TransitionIndex].TransitionPiece);
					CurrentTrackDuration = MusicData->AmbientTrack.Keyframes[TransitionIndex].TransitionPiece->GetDuration();
					break;
				case EMusicState::MS_Preplanning:
					AudioPlayer->SetSound(MusicData->PreplanningTrack.Keyframes[TransitionIndex].TransitionPiece);
					CurrentTrackDuration = MusicData->PreplanningTrack.Keyframes[TransitionIndex].TransitionPiece->GetDuration();
					break;
			}
			bPlayedTransition = true;
			AudioPlayer->Play();
			return;
		}
		else
		{
			if (NextState == EMusicState::MS_Scripted)
			{
				NextState = NextScriptedState;
				AudioPlayer->SetSound(ScriptedAudio);
			}
			CurrentState = NextState;
			bFadingOut = false;
			bPlayedTransition = false;
			TransitionIndex = -1;
		}
	}

	// loop it?
	switch (CurrentState)
	{
		case EMusicState::MS_Action:
			if (MusicData->ActionTrack.MusicPiece)
			{
				AudioPlayer->SetSound(MusicData->ActionTrack.MusicPiece);
				CurrentTrackDuration = MusicData->ActionTrack.MusicPiece->GetDuration();
				TimeRemaining = MusicData->ActionTrack.MinimumTime;
				NextState = EMusicState::MS_Ambient;
			}
			break;
		case EMusicState::MS_Ambient:
			if (MusicData->AmbientTrack.MusicPiece)
			{
				AudioPlayer->SetSound(MusicData->AmbientTrack.MusicPiece);
				CurrentTrackDuration = MusicData->AmbientTrack.MusicPiece->GetDuration();
			}
			break;
		case EMusicState::MS_Preplanning:
			if (MusicData->PreplanningTrack.MusicPiece)
			{
				AudioPlayer->SetSound(MusicData->PreplanningTrack.MusicPiece);
				CurrentTrackDuration = MusicData->PreplanningTrack.MusicPiece->GetDuration();
			}
			else if (MusicData->AmbientTrack.MusicPiece)
			{
				AudioPlayer->SetSound(MusicData->AmbientTrack.MusicPiece);
				CurrentTrackDuration = MusicData->AmbientTrack.MusicPiece->GetDuration();
			}
			break;
	}
	AudioPlayer->SetUISound(true);
	AudioPlayer->SetVolumeMultiplier(/* fixme */ 1.0f);
	AudioPlayer->Play();
}

void AMusicSequencerUnreal::Multicast_ResetAudio_Implementation()
{
	OnAudioFinished();
}
