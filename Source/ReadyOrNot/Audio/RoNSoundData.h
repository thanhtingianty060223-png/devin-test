// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FMODEvent.h"
#include "RoNSoundData.generated.h"

USTRUCT(BlueprintType)
struct FWeaponSoundData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds)
	USoundCue* Gunshot;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds)
	USoundCue* Gunshot_Supressed;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds)
	USoundCue* GunTail;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds)
	USoundCue* GunTail_Supressed;
};

/**
*
*/
UCLASS()
class READYORNOT_API UWeaponSound : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds)
	bool bPlayFMODFiringAudio = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds, meta = (EditCondition = "bPlayFMODFiringAudio"))
	UFMODEvent* FMODGunShot1P;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = GunSounds, meta = (EditCondition = "bPlayFMODFiringAudio"))
	UFMODEvent* FMODGunShot3P;

	// Full Sequence sounds. DO NOT use these in combination with the other sounds, otherwise it won't work right.
	// MagCheck
	UPROPERTY(EditAnywhere, Category = "Full Sequence")
	UFMODEvent* MagCheck_FullSeq;

	UPROPERTY(EditAnywhere, Category = "Full Sequence")
	UFMODEvent* QuickReload_FullSeq;

	UPROPERTY(EditAnywhere, Category = "Full Sequence")
	UFMODEvent* QuickReloadEmpty_FullSeq;

	UPROPERTY(EditAnywhere, Category = "Full Sequence")
	UFMODEvent* Reload_FullSeq;

	UPROPERTY(EditAnywhere, Category = "Full Sequence")
	UFMODEvent* ReloadEmpty_FullSeq;

	// Reloading and Magcheck
	// Magazine inserted (tactical)
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagIn;

	// Magazine extracted (tactical)
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagOut;

	// Magazine inserted (speedloading)
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagInQuick;

	// Magazine extracted (speedloading)
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagOutQuick;

	// Magazine Drop FX
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagDrop;

	// Magazine Drop FX Quick
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* MagDropQuick;

	// Weapon Drop FX
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* WeaponDrop;

	// Bolt closed, tactical
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* BoltClose;

	// Bolt closed, speedloading
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* BoltCloseQuick;

	// Bolt opened, tactical
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* BoltOpen;

	// Bolt opened, speedloading
	UPROPERTY(EditAnywhere, Category = "Reloading")
	UFMODEvent* BoltOpenQuick;

	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* OnADSSound;

	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* OnEndADSSound;

	// NVG
	// NVG turn on (full sequence)
	UPROPERTY(EditAnywhere, Category = "NVG")
	UFMODEvent* NightvisionOn;

	// NVG turn off (full sequence)
	UPROPERTY(EditAnywhere, Category = "NVG")
	UFMODEvent* NightvisionOff;

	// Holster / Draw
	// Holster (full sequence)
	UPROPERTY(EditAnywhere, Category = "Holster Draw")
	UFMODEvent* Holster;

	// Draw (full sequence)
	UPROPERTY(EditAnywhere, Category = "Holster Draw")
	UFMODEvent* Draw;

	// Draw first (full sequence)
	UPROPERTY(EditAnywhere, Category = "Holster Draw")
	UFMODEvent* DrawFirst;

	// Firing mode sound effects
	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* SelectSemi;

	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* SelectBurst;

	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* SelectAuto;

	UPROPERTY(EditAnywhere, Category = "Fire Mode")
	UFMODEvent* SelectSafe;

	// Firing sound effects
	UPROPERTY(EditAnywhere, Category = "Firing")
	FWeaponSoundData Firing_Inside;

	UPROPERTY(EditAnywhere, Category = "Firing")
	FWeaponSoundData Firing_Outside;

	// Dry fire, no bullet in chamber
	UPROPERTY(EditAnywhere, Category = "Firing")
	UFMODEvent* DryFire;

	// Occurs when firing the last bullet (is layered on top of the firing sound)
	UPROPERTY(EditAnywhere, Category = "Firing")
	UFMODEvent* FireLast;

	FWeaponSoundData GetFiringSound(bool bOutside);

	// Whizz sound effects (used when bullets pass by the player)
	UPROPERTY(EditAnywhere, Category = "Whizz")
	bool bPlayBulletWhizz = false;

	UPROPERTY(EditAnywhere, Category = "Whizz")
	UFMODEvent* BulletWhizzFar;

	UPROPERTY(EditAnywhere, Category = "Impact")
	UFMODEvent* HitMarker = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Impact")
	UFMODEvent* HeadshotMarker = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Impact")
    UFMODEvent* KillMarker = nullptr;

	UPROPERTY(EditAnywhere, Category = "Physics")
	UFMODEvent* PhysicsImpact;

	UPROPERTY(EditAnywhere, Category = "Physics")
	UFMODEvent* PlayerImpact;

	UPROPERTY(EditAnywhere, Category = "Physics")
	float PhysicsImpactMinimumVelocity = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Movement Layer")
	UFMODEvent* MovementLayer;
};

UCLASS()
class READYORNOT_API UDoorSound : public UDataAsset
{
	GENERATED_BODY()

public:
	// Event to play when the door is closed (eg, the ker-thunk of the latch bolt closing)
	UPROPERTY(EditAnywhere, Category = "Manipulation")
	UFMODEvent* DoorClosed;

	// Event to play when the door is opened
	UPROPERTY(EditAnywhere, Category = "Manipulation")
	UFMODEvent* DoorOpened;

	// Event to play when the door has started being manipulated
	UPROPERTY(EditAnywhere, Category = "Manipulation")
	UFMODEvent* DoorStartManipulation;

	// Event to play when the door has stopped being manipulated
	UPROPERTY(EditAnywhere, Category = "Manipulation")
	UFMODEvent* DoorStopManipulation;

	// Event to play when the door is broken via C2.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* BrokenByC2;

	// Event to play when the door is broken via battering ram.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* BrokenByRam;

	// Event to play when the door is broken via breaching shotgun.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* BrokenByShotgun;

	// Event to play when the door is kicked in.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* KickedDown;

	// Event to play when the door kick fails.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* KickedDownFailed;

	// Event to play when the door kick breaks the lock.
	UPROPERTY(EditAnywhere, Category = "Breaking")
	UFMODEvent* KickedDownBreak;
};
