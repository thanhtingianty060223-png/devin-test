// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "HUD/Widgets/BaseWidget.h"
#include "BriefingWidget.generated.h"

UENUM(BlueprintType)
enum class EBioType : uint8
{
	BT_None,
	BT_Suspect,
	BT_Civilian
};
/**
 * 
 */
UCLASS()
class READYORNOT_API UBriefingWidget : public UBaseWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/*
	 * Gets the text for the objective formatted how it would be on a whiteboard
	 * ie.
	 * 1.) Bring Order to Chaos
	 * 2.) Arrest All Civilians
	 * 3.) ....
	 */
	UFUNCTION(BlueprintPure)
	void GetWhiteboardObjectiveText(FString& OutString);

	/*
	 *
	 *	Allows us to quickly and  easy grab the bios
	 *	todo: slot in unknown suspects bios if none available
	 */
	UFUNCTION(BlueprintPure)
	void GetBios(EBioType BioType, TArray<FCharacterBio>& OutBios);

	UFUNCTION(BlueprintPure)
	void GetBioDetails(FCharacterBio Bio, TArray<FCriminalRecord>& RapSheet, UTexture2D*& ProfileImage, FString& OutDescription, FString& OutBioText);

	UFUNCTION(BlueprintPure)
	void GetBriefing(FMissionAudio& OutBriefing);

	UFUNCTION(BlueprintPure)
	void GetLevelNickname(FString& OutNickname);

	UFUNCTION(BlueprintPure)
	void GetWhiteboardTimelineAsString(FString& OutString);

	FTransform OriginalCameraTransform;
	bool bZoomedInOnWhiteboard = false;
	UFUNCTION(BlueprintCallable)
	void MoveCameraToMouseCursor(FPointerEvent PointerEvent);

	FString LastAudioPlayed;
	UFUNCTION(BlueprintCallable)
	void PlayMissionAudio(FString AudioName);

	UFUNCTION(BlueprintPure)
	void DoesLevelHaveMissionAudio(FString AudioName, bool& bHasMissionAudio);
	
};
