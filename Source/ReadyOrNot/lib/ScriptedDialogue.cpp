// Copyright Void Interactive, 2023

#include "ScriptedDialogue.h"

#include "Subsystems/SubtitlesSubsystem.h"

UScriptedDialogue* UScriptedDialogue::Start2DScriptedDialogue(UObject* WorldContextObject, UFMODEvent* Event,
                                                              const FString& Speaker, const FString& VoiceLine, FName SpeakerTag)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;
	
	UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get();
	if (!VoiceConfig)
		return nullptr;

	FString FileExtension = VoiceConfig->GetVoiceFileExtension();
	FileExtension.RemoveFromStart("*");
	
	FString OutFileName, OutFilePath;
	if (!VoiceConfig->GetSpecificVoiceLine(VoiceLine + FileExtension, Speaker, OutFilePath, OutFileName))
		return nullptr;
	
	USoundSource* VoiceSoundSource = USoundSource::CreateFirstPersonSound(World, Event, FTransform::Identity, {});
	if (!VoiceSoundSource)
		return nullptr;

	VoiceSoundSource->SetProgrammerSoundName(OutFilePath);
	VoiceSoundSource->bWantsProgrammerSoundLength = true;
	
	UScriptedDialogue* ScriptedDialogue = NewObject<UScriptedDialogue>();
	ScriptedDialogue->VoiceSoundSource = VoiceSoundSource;
	ScriptedDialogue->RegisterWithGameInstance(World);
	
	VoiceSoundSource->OnProgrammerSoundLengthReady.AddWeakLambda(ScriptedDialogue, [](float Length, FName SpeakerTag, FString FileName, UWorld* World, FString SpeakerName)
	{
		USubtitlesStatics::PlaySubtitles(World, SpeakerName, FileName, Length, SpeakerTag);
	}, SpeakerTag, OutFileName, World, Speaker);
	
	return ScriptedDialogue;
}

UScriptedDialogue* UScriptedDialogue::StartCharacterScriptedDialogue(AReadyOrNotCharacter* Character, const FString& VoiceLine)
{
	if (!Character)
		return nullptr;

	UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get();
	if (!VoiceConfig)
		return nullptr;

	FString FileExtension = VoiceConfig->GetVoiceFileExtension();
	FileExtension.RemoveFromStart("*");
	
	FString OutFileName, OutFilePath;
	if (!VoiceConfig->GetSpecificVoiceLine(VoiceLine + FileExtension, Character->GetSpeechCharacterName(), OutFilePath, OutFileName))
		return nullptr;
	
	UScriptedDialogue* ScriptedDialogue = NewObject<UScriptedDialogue>();
	ScriptedDialogue->Character = Character;
	ScriptedDialogue->FilePath = OutFileName;

	return ScriptedDialogue;
}

void UScriptedDialogue::Activate()
{
	Super::Activate();

	if (VoiceSoundSource)
	{
		VoiceSoundSource->OnEventStopped.AddDynamic(this, &UScriptedDialogue::HandleDialogueFinished);
		VoiceSoundSource->Play();
	}
	else if (Character)
	{
		Character->Multicast_PlayRawVO_Implementation(FilePath, "", false);
		if (Character->VoiceSoundSource)
			Character->VoiceSoundSource->OnEventStopped.AddDynamic(this, &UScriptedDialogue::HandleDialogueFinished);
	}
}

void UScriptedDialogue::HandleDialogueFinished()
{
	DialogueFinished.Broadcast();
	SetReadyToDestroy();
}
