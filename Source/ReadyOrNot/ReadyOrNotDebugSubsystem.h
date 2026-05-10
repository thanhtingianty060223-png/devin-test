// Copyright Void Interactive, 2023

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ReadyOrNotDebugSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UReadyOrNotDebugSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem")
	void SetDebugLinesVisibility(bool bVisible);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem")
	void EnableAllDebugLines();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem")
	void DisableAllDebugLines();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Melee")
	void ToggleDrawMeleeRange();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Melee")
	void SetMeleeRange(class APlayerCharacter* PlayerCharacter, float NewMeleeRange);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Melee")
    void SetMeleeDamage(class APlayerCharacter* PlayerCharacter, float NewMeleeDamage);
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Melee")
	uint8 bDrawMeleeRange : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Health")
	void ToggleInfiniteHealth();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Health")
	uint8 bInfiniteHealth : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Health")
	void ToggleGodMode();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Health")
	uint8 bPlayerGodMode : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Ammo")
    void ToggleInfiniteAmmo();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Ammo")
	uint8 bInfiniteAmmo : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Grenade")
	void ToggleGrenadeDrawDebug();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Grenade")
	uint8 bDrawGrenadePath : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | AI")
    void WeakenAllEnemiesToLowHealth();

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
	void ToggleRTXSettings();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
	void RTX_ToggleGlobalIllumination();

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
    void RTX_ToggleReflections();

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
    void RTX_ToggleAmbientOcclusion();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
    void RTX_ToggleShadows();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | RTX")
    void RTX_ToggleTranslucency();

	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTXOn : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTX_GlobalIlluminationOn : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTX_ReflectionsOn : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTX_AmbientOcclusionOn : 1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTX_ShadowsOn : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Debug Subsystem | RTX")
	uint8 bRTX_TranslucencyOn : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | V-Sync")
    void ToggleVSync();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | V-Sync")
	uint8 bVSyncOn : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
	void AddOrUpdatePostProcessMaterial(UMaterialInterface* InMaterial, bool& bMaterialOn);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
    void ToggleFibonacciOverlayGuide(UMaterialInterface* InMaterial);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
    void ToggleLineUpOverlayGuide(UMaterialInterface* InMaterial);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
    void TogglePistolLineOverlayGuide(UMaterialInterface* InMaterial);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
    void ToggleRifleLineOverlayGuide(UMaterialInterface* InMaterial);
    
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Overlay Guide")
    void ToggleRuleOfThirdsOverlayGuide(UMaterialInterface* InMaterial);
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Overlay Guide")
	bool bOverlayOn_Fibonacci;

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Overlay Guide")
	bool bOverlayOn_LineUp;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Overlay Guide")
	bool bOverlayOn_PistolLine;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Overlay Guide")
	bool bOverlayOn_RifleLine;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Overlay Guide")
	bool bOverlayOn_RuleOfThirds;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Toggle Global Damage Multiplier (Weapons)")
	void ToggleGlobalDamageMultiplier_Weapons();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Toggle Global Damage Multiplier (Grenades)")
	void ToggleGlobalDamageMultiplier_Grenades();

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Set Global Damage Multiplier (Weapons)")
	void SetGlobalDamageMultiplier_Weapons(float NewDamageMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Set Global Damage Multiplier (Grenades)")
    void SetGlobalDamageMultiplier_Grenades(float NewDamageMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Increase Global Damage Multiplier (Weapons)")
	void IncreaseGlobalDamageMultiplier_Weapons(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Decrease Global Damage Multiplier (Weapons)")
    void DecreaseGlobalDamageMultiplier_Weapons(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Increase Global Damage Multiplier (Grenades)")
    void IncreaseGlobalDamageMultiplier_Grenades(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage", DisplayName = "Decrease Global Damage Multiplier (Grenades)")
    void DecreaseGlobalDamageMultiplier_Grenades(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Damage")
	void ToggleLogWeaponDamage();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Damage")
	uint8 bApplyGlobalDamageMultiplier_Weapons : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Damage")
	uint8 bApplyGlobalDamageMultiplier_Grenades : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Damage")
	float GlobalDamageMultiplier_Weapons = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Damage")
	float GlobalDamageMultiplier_Grenades = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Damage")
	uint8 bLogWeaponDamageValuesToConsole : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | UI")
	void ToggleObjectiveMarkers();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | UI")
	uint8 bShowObjectiveMarkers : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | UI")
	void ToggleAllEvidenceActorMarkers();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | UI")
	uint8 bShowAllEvidenceActors : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | UI")
	void ToggleHesitationBar();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | UI")
	uint8 bShowHesitationBar : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Animation")
    void ToggleLogPlayerAnimationStatus();
    
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Animation")
	uint8 bLogPlayerAnimationStatus : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Interaction")
    void ToggleDrawInteractableComponents();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Interaction")
	uint8 bDrawInteractableComponents : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Interaction")
    void ToggleInteractableComponents();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Interaction")
	uint8 bDisableInteractableComponent : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Debug")
    void ToggleDrawDebugTraces();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Debug")
	uint8 bDrawDebugTraces : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Debug")
    void ToggleDrawDoorKillStunDistances();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Debug")
	uint8 bDrawDoorKillStunDistances : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | FMOD")
    void ToggleMuteFMOD();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | FMOD")
	uint8 bMuteFMOD : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | FMOD")
    void TogglePauseFMOD();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | FMOD")
    uint8 bPauseFMOD : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Doors")
    void OpenAllDoors();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Doors")
    uint8 bForceOpenAllDoors : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Doors")
    void CloseAllDoors();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Doors")
    uint8 bForceCloseAllDoors : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Debug")
	void ToggleLaserEyes();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Debug")
    uint8 bLaserEyes : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Music")
	void ToggleMusic(bool bMusicOn);
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Music")
	uint8 bDisableMusic : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleCoverPoints();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bDrawCoverPoints : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleCoverOctree();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bDrawCoverOctree : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Threat System")
	void ToggleThreatPoints();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Threat System")
	void ToggleThreatOctree();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Threat System")
	void ToggleThreatRoomNames();
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleSWATDynamicCover();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bSWATDynamicCover : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleSuspectDynamicCover();

	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bSuspectDynamicCover : 1;

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleDrawSWATCoverLogic();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bDrawSWATCoverLogic : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Cover System")
	void ToggleDrawSuspectCoverLogic();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Cover System")
	uint8 bDrawSuspectCoverLogic : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | SWAT")
	void ToggleInfiniteSWATItems();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | SWAT")
	uint8 bInfiniteSWATItems : 1;
	
	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | Navigation")
	void ToggleNavigation();
	
	UPROPERTY(BlueprintReadWrite, Category = "Debug Subsystem | Navigation")
	uint8 bShowNavigation : 1;

private:
	IConsoleCommand* EnableAllDebugLinesCommand;
	IConsoleCommand* DisableAllDebugLinesCommand;
};
