// Copyright Void Interactive, 2017

#pragma once

#include "Engine/DataAsset.h"
#include <Animation/PoseAsset.h>
#include "PlayableCharacterData.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UPlayableCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	// JUDGE, ALABAMA, KING, etc, it's the name that shows up in the UI (allcaps)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FText CharacterNameUI;

	// Judge, Alabama, King, etc, it's the name that shows up in the UI (regular)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FText CharacterName;

	// The tactical 'role' name of the character. Flavor text. Appears below the character's name.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FText CharacterRole;

	// The bio of the character.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Bio")
	FText CharacterBio;

	// The 'Real Name' that the character has. Flavor text.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Bio")
	FText CharacterRealName;

	// The 'Years of Service' that the character has done. Flavor text.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Bio")
	FText CharacterYearsOfService;

	// The 'Date of Birth' that the character has. Flavor text.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI | Bio")
	FText CharacterDateOfBirth;

	// The handle that is referenced internally to keep the cycler in sync
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	FName HandleName;

	// The face mesh to use for this character.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Art)
	TSoftObjectPtr<USkeletalMesh> FaceMesh;

	// The face rom for facial movement to use for this character.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Art)
	TSoftObjectPtr<UPoseAsset> FaceROM;
};
