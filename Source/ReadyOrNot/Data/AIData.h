// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "lib/DataSingleton.h"
#include "Enums.h"
#include "AI/Archetypes/AIArchetypeData.h"
#include "AIData.generated.h"

USTRUCT(BlueprintType)
struct FAttachedMeshData
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshData")
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Socket;

	UPROPERTY()
	UStaticMeshComponent* StaticMeshComponent;

	FAttachedMeshData()
	{
		StaticMesh = nullptr;
		StaticMeshComponent = nullptr;
	}
};


USTRUCT(BlueprintType)
struct FAttachedSkeletalMeshData
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeshData")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere)
	bool bUseMasterPose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Socket;

	UPROPERTY()
	USkeletalMeshComponent* SkeletalMeshComponent;

	FAttachedSkeletalMeshData()
	{
		SkeletalMesh = nullptr;
		SkeletalMeshComponent = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FCharacterMesh
{
	GENERATED_USTRUCT_BODY()

	// Help force replication by changing this each time to a new one
	UPROPERTY()
	FGuid Guid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* Head;

	// The face rom for facial movement to use for this character.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	UPoseAsset* FaceROM;
	
	// Whether this character is considered female
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	bool bIsFemale = false;

	// Whether this charcater is considered a child
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	bool bIsChild = false;

	// Used in the speech lookup table to find what lines we should play
	UPROPERTY(EditAnywhere, Category="Speech")
	FString CharacterSpeechHandle;

	// Used to override the default footstep sounds for this character
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	UFMODEvent* Footsteps;

	// Used to set the foley event used when this character moves
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	UFMODEvent* MovementFoley;

	// The socket to attach the movement foley event to on the character
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	FName MovementFoleySocket = "spine_3";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttachedMeshData> AttachedMeshData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttachedSkeletalMeshData> AttachedSkeletalMeshData;

	// Which bones (and their children) are excluded from being damaged and causing hit effects / dismemberment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> DamageExcludedBones;
	
	FCharacterMesh()
	{
		Guid = FGuid::NewGuid();
		Body = nullptr;
		Head = nullptr;
		FaceROM = nullptr;
		Footsteps = nullptr;
		MovementFoley = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FAIMovementStyleData
{
	GENERATED_BODY()

	// Unarmed move style, set to "none" to use default male/female unarmed move style determined per AI at runtime
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UnarmedMoveStyle = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName StunnedMoveStyle = "male02_shared_stunned";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LoweredTwoHandedMoveStyle = "male01_suspect_rifle";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RaisedTwoHandedMoveStyle = "male01_suspect_rifle_strafe";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HesitationMoveStyle = "male01_shared_unarmed_strafe_hesitation";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HesitationRifleMoveStyle = "male01_suspect_rifle_strafe_hesitation";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SurrenderedMoveStyle = "male02_shared_comply";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ArrestedMoveStyle = "male02_shared_cuffed";
};

USTRUCT(BlueprintType)
struct FAIMoveDataBlock
{
	GENERATED_USTRUCT_BODY()

	/* added entries to set personality through movement styles */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName UnarmedMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName RifleMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName RifleStrafeMovementStyle;
	
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName RifleStrafeFastMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName HeavyRifleStrafeMovementStyle;
	
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName RifleStrafeCrouchMovementSet;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName PistolMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName PistolStrafeMovementStyle;
	
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName PistolStrafeCrouchMovementStyle;
	
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName ComplyMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName CuffedMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName StunnedMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName GassedMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName InjuredMovementStyle;

	UPROPERTY(EditAnywhere, Category = "Animation")
	FName SprintMovementStyle;

	/* added this section specifically for civilians */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TArray<FName> UnarmedCalmStyles;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TArray<FName> UnarmedPanicStyles;

	FAIMoveDataBlock()
	{
		UnarmedMovementStyle = "male01_shared_unarmed";
		RifleMovementStyle = "male01_suspect_rifle";
		RifleStrafeMovementStyle = "male01_suspect_rifle_strafe";
		RifleStrafeCrouchMovementSet = "male01_suspect_rifle_strafe_crouch";
		HeavyRifleStrafeMovementStyle = "male01_suspect_rifle_strafe_heavy";
		PistolMovementStyle = "male01_suspect_pistol";
		PistolStrafeMovementStyle = "male01_suspect_pistol_strafe";
		PistolStrafeCrouchMovementStyle = "male01_suspect_pistol_strafe_crouch";
		ComplyMovementStyle = "male02_shared_comply";
		CuffedMovementStyle = "male02_shared_cuffed";
		StunnedMovementStyle = "male02_shared_stunned";
		GassedMovementStyle = "male02_shared_gassed";
		InjuredMovementStyle = "male02_shared_limp_left_leg";
		SprintMovementStyle = "superset_test";
	}

	void Empty()
	{
		UnarmedMovementStyle = "";
		RifleMovementStyle = "";
		RifleStrafeMovementStyle = "";
		HeavyRifleStrafeMovementStyle = "";
		RifleStrafeCrouchMovementSet = "";
		PistolMovementStyle = "";
		PistolStrafeMovementStyle = "";
		PistolStrafeCrouchMovementStyle = "";
		ComplyMovementStyle = "";
		StunnedMovementStyle = "";
		GassedMovementStyle = "";
		InjuredMovementStyle = "";
		SprintMovementStyle = "";
	}
};

USTRUCT(BlueprintType)
struct FAIDataLookupTable : public FTableRowBase
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	UAIArchetypeData* Archetype = nullptr;

	// Leave blank for game modes that shouldn't override the base archetype
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	TMap<ECOOPMode, UAIArchetypeData*> GameModeArchetypeOverride;
	
	// If specified faction is not found, no faction is assigned. Will just be a regular default AI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	FDataTableRowHandle Faction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	uint8 bIsLeaderOfFaction : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Style")
	FAIMoveDataBlock DefaultMoveData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Style")
	FAIMovementStyleData MovementStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Selection")
	bool bUseRandomLoadout = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Selection")
	TArray<TSubclassOf<ABaseItem>> AIWeaponSelection;

	// Random selection of ammo types for this AI, leave as none to use default ammo type for weapon, skips incompatible types
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Selection")
	TArray<FName> AIAmmoTypeSelection;

	// chance to select armor 0.0f (0%), 1.0f (100%)
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor Selection")
	//float ChanceToSelectArmorPiece = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Selection")
	float ChanceToFireGunOnDeath = 0.1f;

	// List of armour to randomly select from suspect armour data table, with "None" for no armor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour Selection")
	TArray<FName> AIBodyArmourSelection;

	// List of helmets to randomly select from suspect armour data table, with "None" for no armor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour Selection")
	TArray<FName> AIHelmetSelection;

	// Simple override for AI body armour
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armour Selection")
	TSubclassOf<class ASuspectArmour> AIBodyArmourOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Selection")
	FSavedLoadout DefaultLoadout;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Selection")
	ETeamType SpawningTeamType = ETeamType::TT_SUSPECT;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Selection")
	TSubclassOf<class ACyberneticCharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Selection")
	TArray<FCharacterMesh> RandomCharacterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Selection")
	uint8 bOverrideControllerClass : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Selection", meta = (EditCondition = "bOverrideControllerClass"))
	TSubclassOf<class AAIController> ControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Tags)
	TArray<FName> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surrendering")
	uint8 bChanceToSurrenderWithItem : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surrendering", meta = (InlineEditConditionToggle, EditCondition = "bChanceToSurrenderWithItem"))
	uint8 bOverrideSurrenderWithItemChance : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surrendering", meta = (EditCondition = "bOverrideSurrenderWithItemChance"))
	float SurrenderWithItemChance = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surrendering", meta = (EditCondition = "bChanceToSurrenderWithItem"))
	TArray<FString> SurrenderItems = {"phone"};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suicide")
	bool bCanEverSuicide = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suicide", meta = (InlineEditConditionToggle))
	bool bOverrideSuicideChance = false;
	// When morale hits zero, what is the chance of this AI commiting suicide
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suicide", meta = (ClampMin = 0.0f, ClampMax = 1.0f, EditCondition = "bOverrideSuicideChance"))
	float SuicideChance = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stun Damage")
	bool bImmuneToGas = false;

	bool GetRandomCharacterMeshOverride(FCharacterMesh& OutMesh) const
	{
		if (RandomCharacterMesh.Num() > 0)
		{
			OutMesh = RandomCharacterMesh[FMath::RandRange(0, RandomCharacterMesh.Num() - 1)];
			return true;
		}
		return false;
	}
};

/**
 *	Each type of AI present in the game should have an associated AIData data blueprint associated with it.
 *	There may be subclasses of this later on, e.g, SuspectAIData vs HostageAIData
 *	@author	eezstreet
 */
UCLASS(BlueprintType)
class READYORNOT_API UAIData : public UDataAsset
{
	GENERATED_BODY()

public:
	static FSavedLoadout GetLoadout(const FAIDataLookupTable* LookupRow);
};
