// Copyright Void Interactive, 2023

#include "ReadyOrNotDebugSubsystem.h"

#include "CoverSystem.h"
#include "Characters/CyberneticCharacter.h"

#include "Components/CharacterHealthComponent.h"
#include "Components/PlayerPostProcessing.h"

#include "FMODBus.h"
#include "NavigationSystem.h"
#include "ReadyOrNotRecastNavMesh.h"
#include "Actors/BaseGrenade.h"
#include "Info/MusicManager.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"

#define FLIP_BOOL(x) x = !x

UReadyOrNotDebugSubsystem::UReadyOrNotDebugSubsystem()
{
	bShowObjectiveMarkers = true;
	bShowHesitationBar = true;
	bSuspectDynamicCover = true;
}

void UReadyOrNotDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IConsoleManager::Get().RegisterConsoleCommand(TEXT("EnableAllDebugLines"), TEXT("Enable all debug line drawing"),
		FConsoleCommandDelegate::CreateUObject(this, &UReadyOrNotDebugSubsystem::EnableAllDebugLines), ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(TEXT("DisableAllDebugLines"), TEXT("Disable all debug line drawing"),
	FConsoleCommandDelegate::CreateUObject(this, &UReadyOrNotDebugSubsystem::DisableAllDebugLines), ECVF_Default);
}

void UReadyOrNotDebugSubsystem::Deinitialize()
{
	Super::Deinitialize();

	IConsoleManager::Get().UnregisterConsoleObject(EnableAllDebugLinesCommand);
	IConsoleManager::Get().UnregisterConsoleObject(DisableAllDebugLinesCommand);
}

void UReadyOrNotDebugSubsystem::SetDebugLinesVisibility(bool bVisible)
{
	if (GetWorld())
	{
		if (GetWorld()->LineBatcher)
			GetWorld()->LineBatcher->SetVisibility(bVisible);

		if (GetWorld()->ForegroundLineBatcher)
			GetWorld()->ForegroundLineBatcher->SetVisibility(bVisible);

		if (GetWorld()->PersistentLineBatcher)
			GetWorld()->PersistentLineBatcher->SetVisibility(bVisible);
	}
}

void UReadyOrNotDebugSubsystem::EnableAllDebugLines()
{
	SetDebugLinesVisibility(true);
}

void UReadyOrNotDebugSubsystem::DisableAllDebugLines()
{
	SetDebugLinesVisibility(false);
}

void UReadyOrNotDebugSubsystem::ToggleDrawMeleeRange()
{
	FLIP_BOOL(bDrawMeleeRange);
}

void UReadyOrNotDebugSubsystem::SetMeleeRange(APlayerCharacter* PlayerCharacter, const float NewMeleeRange)
{
	if (PlayerCharacter)
		PlayerCharacter->SetMeleeRange(NewMeleeRange);
}

void UReadyOrNotDebugSubsystem::SetMeleeDamage(APlayerCharacter* PlayerCharacter, const float NewMeleeDamage)
{
	if (PlayerCharacter)
		PlayerCharacter->SetMeleeDamage(NewMeleeDamage);
}

void UReadyOrNotDebugSubsystem::ToggleInfiniteHealth()
{
	FLIP_BOOL(bInfiniteHealth);
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		PlayerCharacter->GetHealthComponent()->Server_SetUnlimitedResource(bInfiniteHealth);
		PlayerCharacter->GetHealthComponent()->SetUnlimitedResource(bInfiniteHealth);
	}
}

void UReadyOrNotDebugSubsystem::ToggleGodMode()
{
	FLIP_BOOL(bPlayerGodMode);
}

void UReadyOrNotDebugSubsystem::ToggleInfiniteAmmo()
{
	FLIP_BOOL(bInfiniteAmmo);
}

void UReadyOrNotDebugSubsystem::ToggleGrenadeDrawDebug()
{
	FLIP_BOOL(bDrawGrenadePath);

	if (bDrawGrenadePath)
	{
		for (TActorIterator<ABaseGrenade>It(GetWorld()); It; ++It)
		{
			if (ABaseGrenade* b = *It)
				b->DrawDebugType = EDrawDebugTrace::Persistent;
		}
	}
	else
	{
		for (TActorIterator<ABaseGrenade>It(GetWorld()); It; ++It)
		{
			if (ABaseGrenade* b = *It)
				b->DrawDebugType = EDrawDebugTrace::None;
		}
	}
}

void UReadyOrNotDebugSubsystem::WeakenAllEnemiesToLowHealth()
{
	for (TActorIterator<ACyberneticCharacter> It(GetWorld()); It; ++It)
	{
		const ACyberneticCharacter* CyberneticCharacter = *It;

		if (CyberneticCharacter && CyberneticCharacter->IsSuspect())
		{
			CyberneticCharacter->GetHealthComponent()->Server_SetResource(CyberneticCharacter->GetHealthComponent()->GetLowResource());
		}
	}
}

void UReadyOrNotDebugSubsystem::ToggleRTXSettings()
{
	FLIP_BOOL(bRTXOn);

	if (bRTXOn)
	{
		FString Command = FString("r.RayTracing.GlobalIllumination ") + FString::FromInt(1);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Translucency ") + FString::FromInt(1);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Reflections ") + FString::FromInt(1);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.AmbientOcclusion ") + FString::FromInt(1);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Shadows ") + FString::FromInt(1);
		GEngine->Exec(GetWorld(), *Command);

		bRTX_GlobalIlluminationOn = true;
		bRTX_ReflectionsOn = true;
		bRTX_AmbientOcclusionOn = true;
		bRTX_ShadowsOn = true;
		bRTX_TranslucencyOn = true;
	}
	else
	{
		FString Command = FString("r.RayTracing.GlobalIllumination ") + FString::FromInt(0);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Translucency ") + FString::FromInt(0);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Reflections ") + FString::FromInt(0);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.AmbientOcclusion ") + FString::FromInt(0);
		GEngine->Exec(GetWorld(), *Command);
		Command = FString("r.RayTracing.Shadows ") + FString::FromInt(0);
		GEngine->Exec(GetWorld(), *Command);

		bRTX_GlobalIlluminationOn = false;
		bRTX_ReflectionsOn = false;
		bRTX_AmbientOcclusionOn = false;
		bRTX_ShadowsOn = false;
		bRTX_TranslucencyOn = false;
	}
}

void UReadyOrNotDebugSubsystem::RTX_ToggleGlobalIllumination()
{
	FLIP_BOOL(bRTX_GlobalIlluminationOn);

	const FString Command = FString("r.RayTracing.GlobalIllumination ") + FString::FromInt(bRTX_GlobalIlluminationOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::RTX_ToggleReflections()
{
	FLIP_BOOL(bRTX_ReflectionsOn);

	const FString Command = FString("r.RayTracing.Reflections ") + FString::FromInt(bRTX_ReflectionsOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::RTX_ToggleAmbientOcclusion()
{
	FLIP_BOOL(bRTX_AmbientOcclusionOn);

	const FString Command = FString("r.RayTracing.AmbientOcclusion ") + FString::FromInt(bRTX_AmbientOcclusionOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::RTX_ToggleShadows()
{
	FLIP_BOOL(bRTX_ShadowsOn);

	const FString Command = FString("r.RayTracing.Shadows ") + FString::FromInt(bRTX_ShadowsOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::RTX_ToggleTranslucency()
{
	FLIP_BOOL(bRTX_TranslucencyOn);

	const FString Command = FString("r.RayTracing.Translucency ") + FString::FromInt(bRTX_TranslucencyOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::ToggleVSync()
{
	FLIP_BOOL(bVSyncOn);
	
	const FString Command = FString("r.VSync ") + FString::FromInt(bVSyncOn ? 1 : 0);
	GEngine->Exec(GetWorld(), *Command);
}

void UReadyOrNotDebugSubsystem::AddOrUpdatePostProcessMaterial(UMaterialInterface* InMaterial, bool& bMaterialOn)
{
	bMaterialOn = !bMaterialOn;
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (UPlayerPostProcessing* PlayerPostProcessing = PlayerCharacter->GetPlayerPostProcessing())
		{
			PlayerPostProcessing->AddOrUpdateBlendable(InMaterial, bMaterialOn ? 1.0f : 0.0f);
		}
	}
}

void UReadyOrNotDebugSubsystem::ToggleFibonacciOverlayGuide(UMaterialInterface* InMaterial)
{
	AddOrUpdatePostProcessMaterial(InMaterial, bOverlayOn_Fibonacci);
}

void UReadyOrNotDebugSubsystem::ToggleLineUpOverlayGuide(UMaterialInterface* InMaterial)
{
	AddOrUpdatePostProcessMaterial(InMaterial, bOverlayOn_LineUp);
}

void UReadyOrNotDebugSubsystem::TogglePistolLineOverlayGuide(UMaterialInterface* InMaterial)
{
	AddOrUpdatePostProcessMaterial(InMaterial, bOverlayOn_PistolLine);
}

void UReadyOrNotDebugSubsystem::ToggleRifleLineOverlayGuide(UMaterialInterface* InMaterial)
{
	AddOrUpdatePostProcessMaterial(InMaterial, bOverlayOn_RifleLine);
}

void UReadyOrNotDebugSubsystem::ToggleRuleOfThirdsOverlayGuide(UMaterialInterface* InMaterial)
{
	AddOrUpdatePostProcessMaterial(InMaterial, bOverlayOn_RuleOfThirds);
}

void UReadyOrNotDebugSubsystem::ToggleGlobalDamageMultiplier_Weapons()
{
	bApplyGlobalDamageMultiplier_Weapons = !bApplyGlobalDamageMultiplier_Weapons;
}

void UReadyOrNotDebugSubsystem::ToggleGlobalDamageMultiplier_Grenades()
{
	bApplyGlobalDamageMultiplier_Grenades = !bApplyGlobalDamageMultiplier_Grenades;
}

void UReadyOrNotDebugSubsystem::SetGlobalDamageMultiplier_Weapons(const float NewDamageMultiplier)
{
	GlobalDamageMultiplier_Weapons = FMath::Clamp(NewDamageMultiplier, 0.0f, BIG_NUMBER);
}

void UReadyOrNotDebugSubsystem::SetGlobalDamageMultiplier_Grenades(const float NewDamageMultiplier)
{
	GlobalDamageMultiplier_Grenades = FMath::Clamp(NewDamageMultiplier, 0.0f, BIG_NUMBER);
}

void UReadyOrNotDebugSubsystem::IncreaseGlobalDamageMultiplier_Weapons(const float Amount)
{
	GlobalDamageMultiplier_Weapons = FMath::Clamp(GlobalDamageMultiplier_Weapons + Amount, 0.0f, BIG_NUMBER);
}

void UReadyOrNotDebugSubsystem::DecreaseGlobalDamageMultiplier_Weapons(const float Amount)
{
	GlobalDamageMultiplier_Weapons = FMath::Clamp(GlobalDamageMultiplier_Weapons - Amount, 0.0f, GlobalDamageMultiplier_Weapons);
}

void UReadyOrNotDebugSubsystem::IncreaseGlobalDamageMultiplier_Grenades(const float Amount)
{
	GlobalDamageMultiplier_Grenades = FMath::Clamp(GlobalDamageMultiplier_Grenades + Amount, 0.0f, BIG_NUMBER);
}

void UReadyOrNotDebugSubsystem::DecreaseGlobalDamageMultiplier_Grenades(const float Amount)
{
	GlobalDamageMultiplier_Grenades = FMath::Clamp(GlobalDamageMultiplier_Grenades - Amount, 0.0f, GlobalDamageMultiplier_Grenades);
}

void UReadyOrNotDebugSubsystem::ToggleLogWeaponDamage()
{
	FLIP_BOOL(bLogWeaponDamageValuesToConsole);
}

void UReadyOrNotDebugSubsystem::ToggleObjectiveMarkers()
{
	FLIP_BOOL(bShowObjectiveMarkers);
}

void UReadyOrNotDebugSubsystem::ToggleAllEvidenceActorMarkers()
{
	FLIP_BOOL(bShowAllEvidenceActors);
}

void UReadyOrNotDebugSubsystem::ToggleHesitationBar()
{
	FLIP_BOOL(bShowHesitationBar);
}

void UReadyOrNotDebugSubsystem::ToggleLogPlayerAnimationStatus()
{
	FLIP_BOOL(bLogPlayerAnimationStatus);
}

void UReadyOrNotDebugSubsystem::ToggleDrawInteractableComponents()
{
	FLIP_BOOL(bDrawInteractableComponents);
}

void UReadyOrNotDebugSubsystem::ToggleInteractableComponents()
{
	FLIP_BOOL(bDisableInteractableComponent);
}

void UReadyOrNotDebugSubsystem::ToggleDrawDebugTraces()
{
	FLIP_BOOL(bDrawDebugTraces);
}

void UReadyOrNotDebugSubsystem::ToggleDrawDoorKillStunDistances()
{
	FLIP_BOOL(bDrawDoorKillStunDistances);
}

void UReadyOrNotDebugSubsystem::ToggleMuteFMOD()
{
	FLIP_BOOL(bMuteFMOD);

	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBlueprintStatics::BusSetMute(*It, bMuteFMOD);
	}
}

void UReadyOrNotDebugSubsystem::TogglePauseFMOD()
{
	FLIP_BOOL(bPauseFMOD);

	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBlueprintStatics::BusSetPaused(*It, bPauseFMOD);
	}
}

void UReadyOrNotDebugSubsystem::OpenAllDoors()
{
	bForceCloseAllDoors = false;
	bForceOpenAllDoors = true;
}

void UReadyOrNotDebugSubsystem::CloseAllDoors()
{
	bForceOpenAllDoors = false;
	bForceCloseAllDoors = true;
}

void UReadyOrNotDebugSubsystem::ToggleLaserEyes()
{
	FLIP_BOOL(bLaserEyes);
}

void UReadyOrNotDebugSubsystem::ToggleMusic(const bool bMusicOn)
{
	bDisableMusic = !bMusicOn;

	if (bMusicOn)
	{
		UMusicManager::Get(this)->StartMusicParametersUpdate();
	}
	else
	{
		UMusicManager::Get(this)->StopTheMusic(true);
	}
}

void UReadyOrNotDebugSubsystem::ToggleCoverPoints()
{
	FLIP_BOOL(bDrawCoverPoints);

	if (UCoverSystem* System = COVER_SYSTEM)
	{
		System->bDrawCoverData = bDrawCoverPoints;
	}
}

void UReadyOrNotDebugSubsystem::ToggleCoverOctree()
{
	FLIP_BOOL(bDrawCoverOctree);
	
	if (UCoverSystem* System = COVER_SYSTEM)
	{
		System->bDrawCoverOctree = bDrawCoverOctree;
	}
}

void UReadyOrNotDebugSubsystem::ToggleThreatPoints()
{
	if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
	{
		FLIP_BOOL(Subsystem->bDrawThreatPoints);
	}
}

void UReadyOrNotDebugSubsystem::ToggleThreatOctree()
{
	if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
	{
		FLIP_BOOL(Subsystem->bDrawThreatAwarenessOctree);
	}
}

void UReadyOrNotDebugSubsystem::ToggleThreatRoomNames()
{
	if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
	{
		FLIP_BOOL(Subsystem->bDrawThreatRoomNames);
	}
}

void UReadyOrNotDebugSubsystem::ToggleSWATDynamicCover()
{
	FLIP_BOOL(bSWATDynamicCover);
}

void UReadyOrNotDebugSubsystem::ToggleSuspectDynamicCover()
{
	FLIP_BOOL(bSuspectDynamicCover);
}

void UReadyOrNotDebugSubsystem::ToggleDrawSWATCoverLogic()
{
	FLIP_BOOL(bDrawSWATCoverLogic);

	if (!bSWATDynamicCover)
		bDrawSWATCoverLogic = false;

	const FString Command = "a.RonDrawSwatCoverLogic " + FString::FromInt(bDrawSWATCoverLogic ? 1 : 0);
	UGameplayStatics::GetPlayerController(this, 0)->ConsoleCommand(Command);
}

void UReadyOrNotDebugSubsystem::ToggleDrawSuspectCoverLogic()
{
	FLIP_BOOL(bDrawSuspectCoverLogic);
	
	if (!bSuspectDynamicCover)
		bDrawSuspectCoverLogic = false;
	
	const FString Command = "a.RonDrawSuspectCoverLogic " + FString::FromInt(bDrawSuspectCoverLogic ? 1 : 0);
	UGameplayStatics::GetPlayerController(this, 0)->ConsoleCommand(Command);
}

void UReadyOrNotDebugSubsystem::ToggleInfiniteSWATItems()
{
	FLIP_BOOL(bInfiniteSWATItems);
}

void UReadyOrNotDebugSubsystem::ToggleNavigation()
{
	FLIP_BOOL(bShowNavigation);
	
	UGameplayStatics::GetPlayerController(this, 0)->ConsoleCommand("show navigation");
}
