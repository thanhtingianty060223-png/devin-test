// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ScriptedDialogue.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScriptedDialogueComplete);

/**
 * 
 */
UCLASS()
class READYORNOT_API UScriptedDialogue : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnScriptedDialogueComplete DialogueFinished;

	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UScriptedDialogue* Start2DScriptedDialogue(UObject* WorldContextObject, UFMODEvent* Event, const FString& Speaker, const FString& VoiceLine, FName SpeakerTag);

	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UScriptedDialogue* StartCharacterScriptedDialogue(AReadyOrNotCharacter* Character, const FString& VoiceLine);
	
	virtual void Activate() override;
	
private:
	UPROPERTY()
	const UObject* WorldContextObject;

	UPROPERTY()
	AReadyOrNotCharacter* Character;
	
	UPROPERTY()
	USoundSource* VoiceSoundSource;

	FString Speaker;
	FString VoiceLine;
	FName SpeakerTag;
	
	FString FilePath;

	UFUNCTION()
	void HandleDialogueFinished();
};
