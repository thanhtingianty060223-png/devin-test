// Copyright Void Interactive, 2024

#include "BpGameplayHelperLib.h"

#include "GameFeatureLibrary.h"
#include "NavigationSystem.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameState.h"
#include "ReadyOrNotFunctionLibrary.h"
#include "ReadyOrNotGameUserSettings.h"

#include "Actors/Door.h"
#include "Actors/SWATArmour.h"
#include "Actors/BaseGrenade.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/Attachments/ScopedWeaponAttachment.h"
#include "Actors/Items/NightvisionGoggles.h"
#include "Actors/Items/Tablet.h"

#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Characters/CyberneticController.h"
#include "Characters/ReplayController.h"
#include "Characters/AI/CivilianCharacter.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Commander/CommanderGM.h"
#include "Commander/CommanderProfile.h"

#include "Components/InteractableComponent.h"

#include "Data/PlayableCharacterData.h"

#include "Components/SkinComponent.h"

#include "Metagame/Profile.h"
#include "Metagame/LicenseSave.h"

#include "GameModes/VIPEscortGS.h"
#include "GameModes/CoopGM.h"
#include "GameModes/LobbyGM.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "Info/LoadoutManager.h"
#include "Info/MapStatisticsSystem.h"
#include "Objectives/NeutralizeSuspectByTag.h"
#include "Subsystems/SubtitlesSubsystem.h"


UReadyOrNotGameInstance* UBpGameplayHelperLib::GetRONGameInstance()
{
	if (!GEngine)
		return nullptr;

	const TIndirectArray<FWorldContext> WorldContexts = GEngine->GetWorldContexts();
	for (FWorldContext World : WorldContexts)
	{
		if (World.World())
		{
			UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(World.World()->GetGameInstance());
			if (gi)
			{
				return gi;
			}
		}
	}
	return nullptr;
}

UWorld* UBpGameplayHelperLib::GetWorldStatic()
{
	if (!GEngine)
		return nullptr;

	if (GEngine->GetCurrentPlayWorld())
		return GEngine->GetCurrentPlayWorld();
	
	if (GEngine->GetWorldContextFromPIEInstance(0) && GEngine->GetWorldContextFromPIEInstance(0)->World())
		return GEngine->GetWorldContextFromPIEInstance(0)->World();

	return GEngine->GetCurrentPlayWorld();	
}

UDataSingleton* UBpGameplayHelperLib::GetRoNData()
{
	if (!GEngine)
		return nullptr;

	UDataSingleton* DataInstance = Cast<UDataSingleton>(GEngine->GameSingleton);
	if (DataInstance)
	{
		return DataInstance;
	}
	return nullptr;
}

UCampaignData* UBpGameplayHelperLib::GetCampaignData()
{
	UDataSingleton* DataSingleton = GetRoNData();
	if (!DataSingleton)
		return nullptr;

	return DataSingleton->CampaignData;
}

bool UBpGameplayHelperLib::IsLeadPlayer(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
		return false;
	
	// TODO: first player to join / whoever is voted lead
	if (UKismetSystemLibrary::IsDedicatedServer(WorldContextObject))
		return false;
	
	if (APlayerController* PlayerController = GetLocalPlayerController(World))
		return PlayerController->HasAuthority();

	return false;
}

TArray<FLevelDataLookupTable> UBpGameplayHelperLib::GetLevels()
{
	TArray<FLevelDataLookupTable> returnData;

	UDataTable* dt = GetLevelLookupDataTable();
	if (dt)
	{
		for (FName row : dt->GetRowNames())
		{
			static const FString ContextString(TEXT("Get Levels Lookup"));
			FLevelDataLookupTable* LookupRow = dt->FindRow<FLevelDataLookupTable>(row, ContextString);
			if (LookupRow)
			{
				returnData.Add(*LookupRow);
			}
		}
	}
		return returnData;
}

void UBpGameplayHelperLib::GetFriendlyMapAndModeFromName(FString InUrl, FString& OutInternalMapName,
	FString& OutFriendlyMap, FString& OutFriendlyMode)
{
	FString Map, Mode;
	if (InUrl.Split("?game=", &Map, &Mode))
	{
		
		OutInternalMapName = FPackageName::GetShortName(Map);
		// NOTE: DO not change this without updating the Events Server side code
		// the ECOOPMode is passed on game finished, and mapped to these values to ensure a consistent view for metrics
		OutFriendlyMap = UBpGameplayHelperLib::GetMapDetailsFromName(Map).FriendlyLevelName.ToString();
		
		
		if (Mode == "BS_COOP")
		{
			OutFriendlyMode = "Barricaded Suspects";
		} else if (Mode == "AS_COOP")
		{
			OutFriendlyMode = "Active Shooter";
		} else if (Mode == "HR_COOP")
		{
			OutFriendlyMode = "Hostage Rescue";
		} else if (Mode == "RD_COOP")
		{
			OutFriendlyMode = "Raid";
		} else if (Mode == "BT_COOP")
		{
			OutFriendlyMode = "Bomb Threat";
		} else if (Mode == "RR_PVP")
		{
			OutFriendlyMode = "Rapid Response";
		}
	} 
}

FString UBpGameplayHelperLib::GetFriendlyModeFromECoopMode(const ECOOPMode InCoopMode)
{
	FString Mode;
	if (InCoopMode == ECOOPMode::CM_BarricadedSuspects)
	{
		Mode = "Barricaded Suspects";
	} else if (InCoopMode == ECOOPMode::CM_ActiveShooter)
	{
		Mode = "Active Shooter";
	} else if (InCoopMode == ECOOPMode::CM_HostageRescue)
	{
		Mode = "Hostage Rescue";
	} else if (InCoopMode == ECOOPMode::CM_Raid)
	{
		Mode = "Raid";
	} else if (InCoopMode == ECOOPMode::CM_BombThreat)
	{
		Mode = "Bomb Threat";
	}
	return Mode;
}

ECOOPMode UBpGameplayHelperLib::GetCoopModeFromModeName(const FString& InCoopName)
{
	if (InCoopName == "BT_COOP")
		return ECOOPMode::CM_BombThreat;
	if (InCoopName == "AS_COOP")
		return ECOOPMode::CM_ActiveShooter;
	if (InCoopName == "HR_COOP")
		return ECOOPMode::CM_HostageRescue;
	if (InCoopName == "BS_COOP")
		return ECOOPMode::CM_BarricadedSuspects;
	if (InCoopName == "RD_COOP")
		return ECOOPMode::CM_Raid;
	
	return ECOOPMode::CM_None;
}

FLevelDataLookupTable UBpGameplayHelperLib::GetMapDetailsFromName(FString MapName)
{
	if (MapName.Find("?") != -1)
	{
		MapName = MapName.Left(MapName.Find("?"));
	}
	MapName = FPackageName::GetShortName(MapName);

	MapName.ReplaceInline(TEXT("_BarricadedSuspects"), TEXT(""));
	MapName.ReplaceInline(TEXT("_ActiveShooter"), TEXT(""));
	MapName.ReplaceInline(TEXT("_BombThreat"), TEXT(""));
	MapName.ReplaceInline(TEXT("_HostageRescue"), TEXT(""));
	MapName.ReplaceInline(TEXT("_Raid"), TEXT(""));

	// Modded maps
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		if (GameInstance->GetBuiltModdedMapList().Contains(MapName))
		{
			// First try to find custom level data for this modded map
			FLevelDataLookupTable* ModdedLevelData = GameInstance->ModdedLevelData.Find(FName(MapName));
			if (ModdedLevelData)
				return *ModdedLevelData;

			// Otherwise automatically populate level data
			FLevelDataLookupTable ModdedMapLookup = GameInstance->ModdedMapLookUpData;
			ModdedMapLookup.LevelNickname = FText::FromString(MapName);
			ModdedMapLookup.FriendlyLevelName = FText::FromString(MapName);
			return ModdedMapLookup;
		}
	}

	// Built-in maps
	UDataTable* DataTable = UBpGameplayHelperLib::GetLevelLookupDataTable();
	if (DataTable)
	{
		if (!DataTable->GetRowNames().Contains(*MapName))
			return FLevelDataLookupTable();
		
		static const FString ContextString(TEXT("Map Details Level Lookup"));
		FLevelDataLookupTable* LookupRow = DataTable->FindRow<FLevelDataLookupTable>(*MapName, ContextString);
		if (LookupRow)
		{
			return *DataTable->FindRow<FLevelDataLookupTable>(*MapName, ContextString);
		}
	}
	
	return FLevelDataLookupTable();
}

FString UBpGameplayHelperLib::GetLoadURLFromData(FLevelDataLookupTable Lookup)
{
	UDataTable* dt = GetLevelLookupDataTable();
	if (dt)
	{
		for (const FName row : dt->GetRowNames())
		{
			static const FString ContextString(TEXT("Load URL Level Lookup"));
			FLevelDataLookupTable* LevelDataRow = dt->FindRow<FLevelDataLookupTable>(row, ContextString);
			if (LevelDataRow)
			{
				if (Lookup == *LevelDataRow)
					return row.ToString();

			}
		}
	}
	return "";
}

class AReadyOrNotPlayerController* UBpGameplayHelperLib::GetLocalRoNPlayerController(UWorld* World)
{
	return UReadyOrNotStatics::GetReadyOrNotPlayerController();
}

int32 UBpGameplayHelperLib::GetNumberOfVisibleSwat(ACyberneticController* CyberneticController)
{
	if (!CyberneticController)
		return 0;

	int32 VisibleSwat = 0;
	for (TActorIterator<APlayerCharacter> It(CyberneticController->GetWorld()); It; ++It)
	{
		APlayerCharacter* pc = *It;
		if (pc->IsOnSWATTeam() && CyberneticController->LineOfSightTo(pc))
		{
			VisibleSwat++;
		}
	}
	return VisibleSwat;
}

bool UBpGameplayHelperLib::HasMapLoadedInMainMenu(FString MapName, UWorld* World)
{
	UWorld* WorldContext = World;
	if (!WorldContext)
	{
		if (!GEngine)
			return false;

		if (GEngine->GameViewport)
		{
			WorldContext = GEngine->GameViewport->GetWorld();
		}
	}

	if (WorldContext)
	{
		// iterate over each level streaming object
		for (ULevelStreaming* LevelStreaming : WorldContext->GetStreamingLevels())
		{
			// see if name matches
			if (LevelStreaming->GetWorldAssetPackageName().Contains(MapName))
			{
				return true;
			}
		}
	}
	return false;
}

bool UBpGameplayHelperLib::IsFriendly(AReadyOrNotGameState* GameState, ETeamType TeamOne, ETeamType TeamTwo)
{
	if (GameState)
	{
		if (GameState->bPvPMode)
		{
			if (GameState->bFreeForAll)
				return false;

			if (TeamOne != TeamTwo)
				return false;

			return true;
		}
		
		if ((TeamOne == ETeamType::TT_SERT_BLUE || TeamOne == ETeamType::TT_SERT_RED || TeamOne == ETeamType::TT_CIVILIAN) && (TeamTwo == ETeamType::TT_SERT_BLUE || TeamTwo == ETeamType::TT_SERT_RED || TeamTwo == ETeamType::TT_CIVILIAN))
		{
			return true;
		}

		if (TeamOne == TeamTwo)
		{
			return true;
		}
	}

	return false;
}

bool UBpGameplayHelperLib::IsFriendlyWithMe(AReadyOrNotGameState* GameState, ETeamType TeamType)
{
	return IsFriendly(GameState, UReadyOrNotStatics::GetReadyOrNotPlayerController()->GetTeamType(), TeamType);
}

bool UBpGameplayHelperLib::IsEnemy(ETeamType TeamOne, ETeamType TeamTwo)
{
	if (TeamOne == ETeamType::TT_NONE || TeamTwo == ETeamType::TT_NONE)
		return false;
	
	switch (TeamOne)
	{
		case ETeamType::TT_NONE:
		return false;

		case ETeamType::TT_SQUAD:
		case ETeamType::TT_SERT_RED:
		case ETeamType::TT_SERT_BLUE:
		{
			// Is swat?
			if (TeamTwo != ETeamType::TT_CIVILIAN && TeamTwo != ETeamType::TT_SUSPECT)
				return false;

			if (TeamTwo == ETeamType::TT_SUSPECT)
				return true;

			if (TeamTwo == ETeamType::TT_CIVILIAN)
				return false;
		}
		break;
		
		case ETeamType::TT_SUSPECT:
		{
			// Is swat?
			if (TeamTwo != ETeamType::TT_CIVILIAN && TeamTwo != ETeamType::TT_SUSPECT)
				return true;

			if (TeamTwo == ETeamType::TT_SUSPECT)
				return false;

			if (TeamTwo == ETeamType::TT_CIVILIAN)
				return false;
		}
		break;
		
		case ETeamType::TT_CIVILIAN:
		{
			// Is swat?
			if (TeamTwo != ETeamType::TT_CIVILIAN && TeamTwo != ETeamType::TT_SUSPECT)
				return false;

			if (TeamTwo == ETeamType::TT_SUSPECT)
				return true;

			if (TeamTwo == ETeamType::TT_CIVILIAN)
				return false;
		}
		break;
		
		default:
		return false;
	}

	return false;
}

APlayerCharacter* UBpGameplayHelperLib::GetFirstAlivePlayerControlledCharacter(UWorld* WorldContext)
{
	if (WorldContext == nullptr)
	{
		WorldContext = GetWorldStatic();
		if (WorldContext == nullptr)
		{
			return nullptr;
		}
	}

	for (TActorIterator<APlayerCharacter> It(WorldContext); It; ++It)
	{
		APlayerCharacter* pc = *It;
		if (pc && !pc->IsDeadOrUnconscious() && !pc->IsIncapacitated())
		{
			return pc;
		}
	}

	return nullptr;
}

bool UBpGameplayHelperLib::HasLineOfSightExt(AActor* Observer, AActor* b, FHitResult& HitResult)
{
	if (!Observer || !b)
		return false;

	static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
	FCollisionQueryParams CollisionParams(NAME_LineOfSight);

	APlayerCharacter* ObserverCharacter = Cast<APlayerCharacter>(Observer);
	if (ObserverCharacter)
	{
		AAIController* AiController = Cast<AAIController>(ObserverCharacter->GetController());
		if (AiController)
		{
			return AiController->LineOfSightTo(b);
		}
	}
	APlayerCharacter* bPC = Cast<APlayerCharacter>(b);

	ABaseItem* bItem = Cast<ABaseItem>(b);
	if (bItem)
	{
		APlayerCharacter* bItemOwner = Cast<APlayerCharacter>(bItem->GetOwner());
		if (bItemOwner)
		{
			CollisionParams.AddIgnoredActors(bItemOwner->GetCollisionIgnoredActors());
		}
	}
	if (ObserverCharacter && bPC)
	{
		FVector StartTrace = ObserverCharacter->GetFaceMesh()->GetComponentLocation();
		FVector EndTrace = bPC->GetMesh()->GetComponentLocation();
		CollisionParams.AddIgnoredActors(ObserverCharacter->GetCollisionIgnoredActors());
		ObserverCharacter->GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);

		if (!HitResult.bBlockingHit)
		{
			return true;
		}

		StartTrace = ObserverCharacter->GetFaceMesh()->GetComponentLocation();
		EndTrace = bPC->GetFaceMesh()->GetComponentLocation();

		ObserverCharacter->GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);
		if (!HitResult.bBlockingHit)
		{
			return true;
		}

		if (bPC->GetEquippedItem())
		{
			StartTrace = ObserverCharacter->GetFaceMesh()->GetComponentLocation();
			EndTrace = bPC->GetEquippedItem()->GetActorLocation();


			/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
			//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
			ObserverCharacter->GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);
			if (!HitResult.bBlockingHit)
			{
				return true;
			}
		}
	}

	// This should be where the grenade currently is as we need to trace to the eyes
	const FVector StartTrace = Observer->GetActorLocation();
	/* End the trace so many units in front of the unit */
	FVector EndTrace = b->GetActorLocation();
	EndTrace.Z += 50.0f;
	const ADoor* door = Cast<ADoor>(b);
	if (door)
	{
		EndTrace = door->GetDoorway()->GetComponentLocation();
	}

	/* The visibility trace should not hit the target as this will invalidate results*/
	CollisionParams.AddIgnoredActor(Observer);
	CollisionParams.AddIgnoredActor(b);



	/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
	//DrawDebugLine(Observer->GetWorld(), StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
	Observer->GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);

	// If nothing blocks the trace we are clear
	return !HitResult.bBlockingHit;
}

bool UBpGameplayHelperLib::HasLineOfSight(AActor* Observer, AActor* b)
{
	FHitResult HitResult;
	return UBpGameplayHelperLib::HasLineOfSightExt(Observer, b, HitResult);
}

void UBpGameplayHelperLib::ShowLoadoutOnMeshes(FSavedLoadout Loadout, USkeletalMeshComponent* BodyMesh, USkeletalMeshComponent* HeadMesh, USkeletalMeshComponent* ArmorMesh, USkeletalMeshComponent* ItemMesh, UStaticMeshComponent* ItemMagMesh)
{
	if (Loadout.Primary)
	{
		ABaseMagazineWeapon* bw = Cast<ABaseMagazineWeapon>(Loadout.Primary->GetDefaultObject());
		if (bw)
		{
			ItemMesh->SetSkeletalMesh(bw->ItemMesh->SkeletalMesh);
			ItemMagMesh->SetStaticMesh(bw->Mag_01_Comp->GetStaticMesh());
			ItemMagMesh->AttachToComponent(ItemMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, bw->MagazineSocket);
		}
	}

	for (FCharacterData data : GetRoNData()->ItemData->CharacterSelection)
	{
		if (data.Handle == Loadout.CharacterType)
		{
			if (data.Blueprint)
			{
				UPlayableCharacterData* pcd = Cast<UPlayableCharacterData>(UAsyncLoader::GetLazyLoadedObject(data.Blueprint));
				if (pcd)
				{
					if (UAsyncLoader::GetLazyLoadedSkeletalMesh(pcd->FaceMesh))
					{
						HeadMesh->SetSkeletalMesh(UAsyncLoader::GetLazyLoadedSkeletalMesh(pcd->FaceMesh));
					}
				}
			}
		}
	}

	if (Loadout.Armor)
	{
		ABaseArmour* ba = Cast<ABaseArmour>(Loadout.Armor->GetDefaultObject());
		if (ba)
		{
			ArmorMesh->SetSkeletalMesh(ba->GetItemMesh()->SkeletalMesh);
		}
	}
	
	if (Loadout.PlayerSkin)
	{
		USkinComponent* skin = Cast<USkinComponent>(Loadout.PlayerSkin->GetDefaultObject());
		if (skin)
		{
			if (!skin->bResetsToFactorySkin)
			{
				skin->ApplySkin();
			}
			
		}
	}
}

bool UBpGameplayHelperLib::HasLineOfSightLoc(UWorld* worldContext, FVector a, FVector b, TArray<AActor*> ignoredActors, ECollisionChannel CollisionChannel)
{
	if (!worldContext)
		return false;


	// This should be where the grenade currently is as we need to trace to the eyes
	FVector StartTrace = a;
	/* End the trace so many units in front of the unit */
	FVector EndTrace = b;
	FHitResult HitResult;


	static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
	FCollisionQueryParams CollisionParams(NAME_LineOfSight);
	/* The visibility trace should not hit the target as this will invalidate results*/
	CollisionParams.AddIgnoredActors(ignoredActors);

	for (AActor* actor : ignoredActors)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(actor);
		if (pc)
		{
			CollisionParams.AddIgnoredActors(pc->GetCollisionIgnoredActors());
		}

	}

	/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
	//DrawDebugLine(worldContext, StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
	worldContext->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, CollisionChannel, CollisionParams);
	// If nothing blocks the trace we are clear
	return !HitResult.bBlockingHit;
}

float UBpGameplayHelperLib::GetDistanceBetweenActors(AActor* Actor1, AActor* Actor2)
{
	if (!Actor1 || !Actor2 || (Actor1 && !Actor1->IsValidLowLevel()) || (Actor2 && !Actor2->IsValidLowLevel()))
		return BIG_DIST;

	const float dis = (Actor1->GetActorLocation() - Actor2->GetActorLocation()).Size();
	return dis;
}

float UBpGameplayHelperLib::GetDistanceBetweenActors2D(AActor* Actor1, AActor* Actor2)
{
	if (!Actor1 || !Actor2 || (Actor1 && !Actor1->IsValidLowLevel()) || (Actor2 && !Actor2->IsValidLowLevel()))
		return -1;

	const float dis = (Actor1->GetActorLocation() - Actor2->GetActorLocation()).Size2D();
	return dis;
}

bool UBpGameplayHelperLib::SetUseMeshpainting(bool bUseMeshPainting)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bUseMeshPainting = bUseMeshPainting;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetUseMeshpainting(bool& bUseMeshPainting)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bUseMeshPainting = us->bUseMeshPainting;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetShellLifetime(float ShellLifeTime)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MaxShellLifeTime = ShellLifeTime;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetShellLifetime(float& ShellLifeTime)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		ShellLifeTime = us->MaxShellLifeTime;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetMouseInverted(bool bInvertVertical, bool bInvertHorizonal)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bInvertMousePitch = bInvertVertical;
		us->bInvertMouseYaw = bInvertHorizonal;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetMouseInverted(bool& bInvertVertical, bool& bInvertHorizontal)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bInvertVertical = us->bInvertMousePitch;
		bInvertHorizontal = us->bInvertMouseYaw;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetGamepadInverted(bool bInvertVertical, bool bInvertHorizontal)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bInvertGamepadVertical = bInvertVertical;
		us->bInvertGamepadHorizontal = bInvertHorizontal;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetGamepadInverted(bool& bInvertVertical, bool& bInvertHorizontal)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bInvertVertical = us->bInvertGamepadVertical;
		bInvertHorizontal = us->bInvertGamepadHorizontal;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetGamepadAimAssistIntensity(FString AimAssistIntensity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->AimAssistIntensity = AimAssistIntensity;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetGamepadAimAssistIntensity(FString& AimAssistIntensity)
{
	const UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		AimAssistIntensity = us->AimAssistIntensity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetMouseSensitivity(float MouseSensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MouseSensitivity = MouseSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetMouseSensitivity(float& MouseSensitvity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		MouseSensitvity = us->MouseSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetFreelookSensitivity(float Sensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->FreelookSensitivity = Sensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetFreelookSensitivity(float& Sensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		Sensitivity = us->FreelookSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetGamepadLookSensitivity(float GamepadLookSensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->GamepadLookSensitivity = GamepadLookSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetGamepadLookSensitivity(float& GamepadLookSensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		GamepadLookSensitivity = us->GamepadLookSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetGamepadAimSensitivity(float GamepadAimSensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->GamepadAimSensitivity = GamepadAimSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetGamepadAimSensitivity(float& GamepadAimSensitivity)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		GamepadAimSensitivity = us->GamepadAimSensitivity;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetFoV(float FOV)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->FieldofView = FOV;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetFoV(float& FOV)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		FOV = us->FieldofView;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetPublicLobbyCooldown(int32 Seconds)
{
	// UReadyOrNotSaveGame* lg = GetLoadGameInstance();
	// if (lg)
	// {
	// 	lg->PCDO = FDateTime().UtcNow().ToUnixTimestamp() + Seconds;
	// 	UGameplayStatics::SaveGameToSlot(lg, lg->SaveSlotName, lg->UserIndex);	
	// 	return true;
	// }
	return false;
}

bool UBpGameplayHelperLib::IsInPublicLobbyCooldown(float& SecondsRemaining)
{
	SecondsRemaining = 0.0f;
	return false;
	
	// UReadyOrNotSaveGame* lg = GetLoadGameInstance();
	// if (lg)
	// {// no cooldown
	// 	if (lg->PCDO == 0.0f)
	// 		return 0.0f;
	// 	// determine last cooldown time
	// 	
	// 	SecondsRemaining = lg->PCDO - FDateTime().UtcNow().ToUnixTimestamp();
	// 	return true;
	// 	if (SecondsRemaining < 0.0f) SecondsRemaining = 0.0f;
	// 	return true;
	// }
	// return false;
}

bool UBpGameplayHelperLib::SetMicInputGain(float MicInputGain)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MicInputGain = MicInputGain;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetMicInputGain(float& MicInputGain)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		MicInputGain = us->MicInputGain;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveSelectedAudioDevice(FString InAudioDevice)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->InputAudioDevice = InAudioDevice;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadSelectedAudioDevice(FString& OutAudioDevice)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		OutAudioDevice = us->InputAudioDevice;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetBounceLightEnabled(bool bBounceLightEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bBounceLightEnabled = bBounceLightEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetBounceLightEnabled(bool& bBounceLightEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bBounceLightEnabled = us->bBounceLightEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetFlashlightShadows(bool bFlashlightShadows)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bFlashlightShadowsEnabled = bFlashlightShadows;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetFlashlightShadows(bool& bFlashLightShadows)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bFlashLightShadows = us->bFlashlightShadowsEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveLoadout(FSavedLoadout Loadout, FString LoadoutName)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return false;

	DeleteLoadout(LoadoutName);
	
	Loadout.Name = LoadoutName;
	Profile->Loadouts.Add(Loadout);

	Profile->SaveProfile();
	
	return true;
}

bool UBpGameplayHelperLib::LoadLoadout(FSavedLoadout& Loadout, FString LoadoutName)
{
	Loadout = FSavedLoadout();

	// Try to load an existing loadout from the appropriate profile
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (Profile)
	{
		const TArray<FSavedLoadout>& Loadouts = Profile->Loadouts;
		for (const FSavedLoadout& SavedLoadout : Profile->Loadouts)
		{
			if (SavedLoadout.Name != LoadoutName)
				continue;
		
			Loadout = SavedLoadout;
			if (SanitizeLoadout(Loadout))
			{
				SaveLoadout(Loadout, LoadoutName);
			}
		
			return true;
		}
	}

	// Ensure we always return a valid default loadout
	if (UItemData* ItemData = GetItemData())
	{
		bool bFound = false;
		for (FSavedLoadout& SavedLoadout : ItemData->DefaultLoadouts)
		{
			if (SavedLoadout.Name != LoadoutName)
				continue;

			Loadout = SavedLoadout;
			SaveLoadout(Loadout, LoadoutName);
			
			bFound = true;
			break;
		}
		
		if (!bFound && ItemData->DefaultLoadouts.IsValidIndex(0))
		{
			Loadout = ItemData->DefaultLoadouts[0];
			SaveLoadout(Loadout, "default");
		}

		return true;
	}
	
	return false;
}

void UBpGameplayHelperLib::DeleteLoadout(FString LoadoutName)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return;
	
	Profile->Loadouts.RemoveAll([&](const FSavedLoadout& Loadout)
	{
		return Loadout.Name == LoadoutName;
	});

	Profile->SaveProfile();
}

void UBpGameplayHelperLib::LoadDefaultLoadout(FSavedLoadout& OutLoadout, FString LoadoutName)
{
	OutLoadout = FSavedLoadout();
	
	UItemData* ItemData = GetItemData();
	if (!ItemData)
		return;

	// Try to find the named loadout in the item data
	for (FSavedLoadout& Loadout : ItemData->DefaultLoadouts)
	{
		if (Loadout.Name != LoadoutName)
			continue;
		
		OutLoadout = Loadout;
		return;
	}

	// Last ditch
	if (ItemData->DefaultLoadouts.IsValidIndex(0))
	{
		OutLoadout = ItemData->DefaultLoadouts[0];
	}
}

bool UBpGameplayHelperLib::LoadLoadoutAndEquipPlayer(FSavedLoadout& Loadout, AReadyOrNotCharacter* EquipPlayer, FString LoadoutName)
{
	if (LoadLoadout(Loadout, LoadoutName))
	{
		return EquipLoadoutOnPlayer(Loadout, EquipPlayer, FLoadoutEquipOptions());
	}
	return false;
}

void UBpGameplayHelperLib::SpawnTacticalItem(ABaseItem** Target, TSubclassOf<ABaseItem> TargetClass, AReadyOrNotCharacter* TargetOwner, bool bReplicates, int32& GrenadeCount)
{
	if (!Target)
		return;

	FTransform SpawnTransform = FTransform();
	SpawnTransform.SetLocation(FVector(0.0f, 0.0f, -100000.0f));
	
	if (bReplicates)
	{
		TargetOwner->GetInventoryComponent()->DestroyInventoryItem(*Target);
	}
	else
	{
		ABaseItem* bi = *Target;
		if (bi)
			bi->Destroy();
	}

	if (ABaseItem* NewItem = TargetOwner->GetWorld()->SpawnActor<ABaseItem>(TargetClass, SpawnTransform))
	{
		NewItem->SetReplicates(bReplicates);
		
		const FName NewSocket = FName(*(NewItem->BodySocket.ToString() + "_" + FString::FromInt(GrenadeCount)));
		if (TargetOwner->GetMesh()->GetAllSocketNames().Contains(NewSocket))
		{
			NewItem->BodySocket = NewSocket;
		}

		NewItem->SetOwner(TargetOwner);
		*Target = NewItem;
		
		TargetOwner->GetInventoryComponent()->AddInventoryItem(NewItem);
		
		if (/*ABaseGrenade* Grenade = */Cast<ABaseGrenade>(NewItem))
		{
			TargetOwner->GetInventoryComponent()->SetSelectedDevice(NewItem);
			GrenadeCount++;
		}
	}
}

bool UBpGameplayHelperLib::EquipLoadoutOnPlayer(FSavedLoadout Loadout, AReadyOrNotCharacter* EquipPlayer, FLoadoutEquipOptions LoadoutEquipOptions)
{
	// We can only sanitize the local player, otherwise if we do DLC checks things go byebyes when we send it to the server
	if (Cast<APlayerCharacter>(EquipPlayer) && EquipPlayer->IsLocalPlayer())
	{
		if (LoadoutEquipOptions.bSanitizeLoadout && LoadoutEquipOptions.EquipItemCategory != EItemCategory::IC_None)
		{
			SanitizeLoadout(Loadout);
		}
	}
	
	FTransform SpawnTransform = FTransform();
	SpawnTransform.SetLocation(FVector(0.0f, 0.0f, -100000.0f));
	if (EquipPlayer)
	{
		if (AReadyOrNotPlayerState* ps = LoadoutEquipOptions.OverridePlayerState ? LoadoutEquipOptions.OverridePlayerState : EquipPlayer->GetPlayerState<AReadyOrNotPlayerState>())
		{
			if (AVIPEscortGS* vipgs = Cast<AVIPEscortGS>(EquipPlayer->GetWorld()->GetGameState()))
			{
				if (ps == vipgs->VIPPlayerState)
				{
					// VIP only gets a pistol
					Loadout = FSavedLoadout();
					Loadout.Name = ps->GetLoadout().Name;
					Loadout.Secondary = ULoadoutManager::GetItemByLookupIdx(EquipPlayer, "M1911");
					Loadout.SecondaryAmmoSlotsCount = ps->GetLoadout().SecondaryAmmoSlotsCount;
					Loadout.SecondaryMuzzle = ps->GetLoadout().SecondaryMuzzle;
					Loadout.SecondaryOverbarrel = ps->GetLoadout().SecondaryOverbarrel;
					Loadout.SecondaryScope = ps->GetLoadout().SecondaryScope;
					Loadout.SecondarySkin = ps->GetLoadout().SecondarySkin;
					Loadout.SecondaryUnderbarrel = ps->GetLoadout().SecondaryUnderbarrel;
					LoadoutEquipOptions.EquipItemCategory = EItemCategory::IC_Secondary;
				}
			}
		}

		UItemData* ItemData = GetItemData(EquipPlayer->GetWorld());
		if (!ItemData)
			return false;

		// Load blue team primary weapons, if blue team 
		if (EquipPlayer->GetTeam() == ETeamType::TT_SERT_BLUE)
		{
			for (int32 i = 0; i < ItemData->RedPVPUniquePrimaryWeapons.Num(); i++)
			{
				FWeaponData wd = ItemData->RedPVPUniquePrimaryWeapons[i];
				if (wd.Blueprint == Loadout.Primary)
				{
					if (ItemData->BluePVPUniquePrimaryWeapons.IsValidIndex(i))
					{
						Loadout.Primary = ItemData->BluePVPUniquePrimaryWeapons[i].Blueprint;
					}
				}
			}
		}
		// Load red team primary weapons, if red team 
		else if (EquipPlayer->GetTeam() == ETeamType::TT_SERT_RED)
		{
			for (int32 i = 0; i < ItemData->BluePVPUniquePrimaryWeapons.Num(); i++)
			{
				FWeaponData wd = ItemData->BluePVPUniquePrimaryWeapons[i];
				if (wd.Blueprint == Loadout.Primary)
				{
					if (ItemData->RedPVPUniquePrimaryWeapons.IsValidIndex(i))
					{
						Loadout.Primary = ItemData->RedPVPUniquePrimaryWeapons[i].Blueprint;
					}
				}
			}
		}

		// Equip body armor
		int32 GrenadeCount = 1;
		int32 MaxSlots = 10;
		SpawnTacticalItem(&EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Armor, Loadout.Armor, EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
		if (EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Armor)
		{
			if (ASWATArmour* Armour = Cast<ASWATArmour>(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Armor))
			{
				Armour->SetArmourCoverage(Loadout.ArmourCoverage);
				Armour->SetArmourMaterial(Loadout.ArmourMaterial);
				MaxSlots = Armour->TotalSlots;
			}
		}

		// Equip helmet
		SpawnTacticalItem(&EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Helmet, Loadout.Helmet, EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);

		// Sanitize the slots we received from the player (caps at 255)
		if (Cast<APlayerCharacter>(EquipPlayer))
			SanitizeSlots(Loadout, MaxSlots);

		// Equip the primary weapon
		if ((EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary && EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary->GetClass() != Loadout.Primary) || !EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary)
		{
			EquipPlayer->GetInventoryComponent()->DestroyInventoryItem(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary);

			if (ABaseItem* Primary = EquipPlayer->GetWorld()->SpawnActor<ABaseItem>(Loadout.Primary, SpawnTransform))
			{
				Primary->SetReplicates(LoadoutEquipOptions.bReplicates);
				
				EquipPlayer->GetInventoryComponent()->AddInventoryItem(Primary);
				if (LoadoutEquipOptions.EquipItemCategory == EItemCategory::IC_Primary)
				{
					EquipPlayer->GetInventoryComponent()->PutItemInHands(Primary);
				}
				EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary = Primary;
				
				if (Loadout.PrimarySkin)
				{
					USkinComponent* SkinComp = NewObject<USkinComponent>(Primary, Loadout.PrimarySkin);
					if (SkinComp)
					{
						SkinComp->RegisterComponent();
					}
				}

				if (ABaseMagazineWeapon* PrimaryWeapon = Cast<ABaseMagazineWeapon>(Primary))
				{
					int32 AmmoCount = Loadout.PrimaryAmmoSlotsCount;

					PrimaryWeapon->SetMagazineCount(AmmoCount, Loadout.PrimaryAmmoSlots);

					if (AReadyOrNotPlayerState* ps = LoadoutEquipOptions.OverridePlayerState ? LoadoutEquipOptions.OverridePlayerState : EquipPlayer->GetPlayerState<AReadyOrNotPlayerState>())
					{
						if ((PrimaryWeapon->AvailableFireModes.Contains(ps->LastFireMode) || (PrimaryWeapon->bHasSafeMode && ps->LastFireMode == EFireMode::FM_Safe)))
						{
							PrimaryWeapon->CurrentFireMode = ps->LastFireMode;
						}
					}
				}
			}
		}

		// Equip the primary weapon, if we're required to.
		if (EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary && LoadoutEquipOptions.EquipItemCategory == EItemCategory::IC_Primary)
		{
			EquipPlayer->GetInventoryComponent()->AddInventoryItem(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary);
			EquipPlayer->GetInventoryComponent()->PutItemInHands(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary);
		}

		// Add attachments to the primary weapon
		if (ABaseWeapon* bw = Cast<ABaseWeapon>(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Primary))
		{
			bw->AddAttachment(Loadout.PrimaryScope, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryIlluminator, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryGrip, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryMuzzle, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryStock, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryUnderbarrel, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryOverbarrel, LoadoutEquipOptions.bReplicates);
			bw->AddAttachment(Loadout.PrimaryAmmunition, LoadoutEquipOptions.bReplicates);
		}

		// Equip secondary weapon
		if ((EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary && EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary->GetClass() != Loadout.Secondary) || !EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary)
		{
			EquipPlayer->GetInventoryComponent()->DestroyInventoryItem(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary);
			
			if (ABaseItem* Secondary = EquipPlayer->GetWorld()->SpawnActor<ABaseItem>(Loadout.Secondary, SpawnTransform))
			{
				Secondary->SetReplicates(LoadoutEquipOptions.bReplicates);
				
				EquipPlayer->GetInventoryComponent()->AddInventoryItem(Secondary);
				EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary = Secondary;
				
				if (Loadout.SecondarySkin)
				{
					USkinComponent* SkinComp = NewObject<USkinComponent>(Secondary, Loadout.SecondarySkin);
					if (SkinComp)
					{
						SkinComp->RegisterComponent();
					}
				}

				if (ABaseMagazineWeapon* SecondaryWeapon = Cast<ABaseMagazineWeapon>(Secondary))
				{
					int32 AmmoCount = Loadout.SecondaryAmmoSlotsCount;
					SecondaryWeapon->SetMagazineCount(AmmoCount, Loadout.SecondaryAmmoSlots);

					if (AReadyOrNotPlayerState* ps = LoadoutEquipOptions.OverridePlayerState ? LoadoutEquipOptions.OverridePlayerState : EquipPlayer->GetPlayerState<AReadyOrNotPlayerState>())
					{
						if ((SecondaryWeapon->AvailableFireModes.Contains(ps->LastFireMode) || (SecondaryWeapon->bHasSafeMode && ps->LastFireMode == EFireMode::FM_Safe)))
						{
							// last fire mode we switched to is available on this weapon or they used safe mode and we have it
							SecondaryWeapon->CurrentFireMode = ps->LastFireMode;
						}
					}
				}
			}
		}

		// Equip the secondary weapon, if we're required to.
		if (EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary && LoadoutEquipOptions.EquipItemCategory == EItemCategory::IC_Secondary)
		{
			EquipPlayer->GetInventoryComponent()->PutItemInHands(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary);
		}

		// Add attachments to secondary weapon
		if (ABaseWeapon* sbw = Cast<ABaseWeapon>(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Secondary))
		{
			sbw->AddAttachment(Loadout.SecondaryScope, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryIlluminator, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryGrip, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryMuzzle, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryStock, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryUnderbarrel, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryOverbarrel, LoadoutEquipOptions.bReplicates);
			sbw->AddAttachment(Loadout.SecondaryAmmunition, LoadoutEquipOptions.bReplicates);
		}

		// Apply character data
		if (AReadyOrNotGameState* gs = EquipPlayer->GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			if (!gs->bPvPMode)
			{
				if (!Loadout.CharacterType.IsNone())
				{
					for (int32 i = 0; i < ItemData->CharacterSelection.Num(); i++)
					{
						if (ItemData->CharacterSelection[i].Handle == Loadout.CharacterType)
						{
							EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Character = ItemData->CharacterSelection[i].Blueprint.LoadSynchronous();
							//EquipPlayer->SetCurrentFaceROM(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Character->FaceROM.LoadSynchronous());
							EquipPlayer->Server_ChangeTPMesh(nullptr, EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Character->FaceMesh.Get());

							break;
						}
					}
				}
			}
		}
		
		if (Loadout.PlayerSkin)
		{
			USkinComponent* SkinComp = NewObject<USkinComponent>(EquipPlayer, Loadout.PlayerSkin);
			if (SkinComp)
			{
				SkinComp->RegisterComponent();
			}
		}
		
		// Equip long tactical
		SpawnTacticalItem(&EquipPlayer->GetInventoryComponent()->GetSpawnedGear().LongTactical, Loadout.LongTactical, EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
		GrenadeCount = 1;

		// Equip grenades
		EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Grenades.Init(nullptr, FMath::Clamp(Loadout.GrenadeSlotsCount, 0, MaxSlots));

		int32 TotalGrenades = FMath::Min(Loadout.GrenadeSlots.Num(), Loadout.GrenadeSlotsCount);
		for (int32 i = 0; i < TotalGrenades; i++)
		{
			if (!Loadout.GrenadeSlots[i])
				continue;

			// We don't look at the device amount for grenades as they are intended to be one per slot
			SpawnTacticalItem(&EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Grenades[i], Loadout.GrenadeSlots[i], EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
			GrenadeCount = i;
		}

		// Equip tactical devices
		EquipPlayer->GetInventoryComponent()->GetSpawnedGear().TacticalDevices.Empty();
		
		TMap<ABaseMagazineWeapon*, int32> TacticalDeviceMagazineCountMap;
		
		int32 TotalTacticalDevices = FMath::Min(Loadout.TacticalSlots.Num(), Loadout.TacticalSlotsCount);
		for (int32 i = 0; i < TotalTacticalDevices; i++)
		{
			if (!Loadout.TacticalSlots[i])
				continue;

			// Spawn device if just a regular item, if it's a magazine weapon give it more magazines instead
			if (!Loadout.TacticalSlots[i]->IsChildOf(ABaseMagazineWeapon::StaticClass()))
			{
				// Each slot can potentially spawn multiples of a single item
				const int32 DeviceAmount = Loadout.TacticalSlots[i].GetDefaultObject()->ItemsPerSlot;
				for (int32 j = 0; j < DeviceAmount; j++)
				{
					if (!Loadout.TacticalSlots[i]->IsChildOf(ABaseMagazineWeapon::StaticClass()))
					{
						ABaseItem* SpawnedItem = nullptr;
						SpawnTacticalItem(&SpawnedItem, Loadout.TacticalSlots[i], EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
						EquipPlayer->GetInventoryComponent()->GetSpawnedGear().TacticalDevices.Add(SpawnedItem);
					}
				}
			}
			else
			{
				// Find weapon, increment magazine count per slot
				bool bFoundWeapon = false;
				for (auto& Element : TacticalDeviceMagazineCountMap)
				{
					if (Element.Key && Element.Key->IsA(Loadout.TacticalSlots[i]))
					{
						Element.Value += 1;
						bFoundWeapon = true;
					}
				}

				// If we didn't find an existing weapon matching the class, spawn it
				if (!bFoundWeapon)
				{
					ABaseItem* SpawnedItem = nullptr;
					SpawnTacticalItem(&SpawnedItem, Loadout.TacticalSlots[i], EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
					EquipPlayer->GetInventoryComponent()->GetSpawnedGear().TacticalDevices.Add(SpawnedItem);

					// Ensure we add our new weapon to the map, starting at 1 magazine
					ABaseMagazineWeapon* SpawnedBaseMagazineWeapon = Cast<ABaseMagazineWeapon>(SpawnedItem);
					if (ensure(SpawnedBaseMagazineWeapon))
					{
						TacticalDeviceMagazineCountMap.Add(Cast<ABaseMagazineWeapon>(SpawnedItem), 1);
					}
				}
			}
		}

		// Give tactical device weapon magazines
		for (auto& Element : TacticalDeviceMagazineCountMap)
		{
			if (Element.Key)
			{
				Element.Key->SetMagazineCount(Element.Value, {});
			}
		}
		
		EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Grenades.Remove(nullptr);
		EquipPlayer->GetInventoryComponent()->GetSpawnedGear().TacticalDevices.Remove(nullptr);

		// Equip random gear
		SpawnTacticalItem(&EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear, Loadout.RandomGear, EquipPlayer, LoadoutEquipOptions.bReplicates, GrenadeCount);
		if ((EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear && EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear->GetClass() != Loadout.RandomGear) || !EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear)
		{
			EquipPlayer->GetInventoryComponent()->DestroyInventoryItem(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear);

			if (ABaseItem* RandomGear = EquipPlayer->GetWorld()->SpawnActor<ABaseItem>(Loadout.RandomGear, SpawnTransform))
			{
				RandomGear->SetReplicates(LoadoutEquipOptions.bReplicates);
				
				EquipPlayer->GetInventoryComponent()->AddInventoryItem(RandomGear);
				EquipPlayer->GetInventoryComponent()->GetSpawnedGear().RandomGear = RandomGear;
			}
		}

		for (int32 y = 0; y < EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Miscelaneous.Num(); y++)
		{
			EquipPlayer->GetInventoryComponent()->DestroyInventoryItem(EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Miscelaneous[y]);
		}
		
		for (int32 i = 0; i < Loadout.Miscelaneous.Num(); i++)
		{
			if (ABaseItem* item = EquipPlayer->GetWorld()->SpawnActor<ABaseItem>(Loadout.Miscelaneous[i], SpawnTransform))
			{
				item->SetReplicates(LoadoutEquipOptions.bReplicates);
				
				EquipPlayer->GetInventoryComponent()->AddInventoryItem(item);
				EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Miscelaneous.Add(item);
			}
		}

		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(EquipPlayer);
		if (PlayerCharacter)
		{
			if (!PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfClass(ANightvisionGoggles::StaticClass()))
			{
				PlayerCharacter->bNVGOn = false;
			}
			PlayerCharacter->Client_AutoSelectNewQuickthrowItem();
		}
		
		// We equipped a new loadout, refresh the quickthrow item
		if (EquipPlayer->IsOnSWATTeam())
		{
			UBpGameplayHelperLib::AddDefaultItemsToPlayer(EquipPlayer);
		}

		// Really try to get the skins to always apply
		EquipPlayer->Customization.ApplyCustomizationSkins(EquipPlayer);
		
		EquipPlayer->GetInventoryComponent()->GetSpawnedGear().Guid = FGuid::NewGuid();
		EquipPlayer->GetInventoryComponent()->SetLastEquippedLoadout(Loadout);
		EquipPlayer->GetInventoryComponent()->Client_NotifyInventoryItemsChanged();
		EquipPlayer->GetInventoryComponent()->Client_NotifyInventorySpawned();

		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SanitizeLoadout(FSavedLoadout& InLoadout)
{
	// Don't sanitize training loadout as it's intentionally not a "valid" loadout
	if (InLoadout.Name == "training")
		return false;

	UItemData* ItemData = GetItemData(GetWorldStatic());
	if (!ItemData)
		return false;

	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(UGameplayStatics::GetPlayerController(GetWorldStatic(), 0));
	if (!pc)
		return false;

	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->PlayerState);
	if (!ps)
		return false;

	bool bCheckDLC = true;
#if WITH_EDITOR
	bCheckDLC = pc->DLCChecksEnabled == 1;
#endif
	
	if (pc->IsLocalController() && bCheckDLC)
	{
		UDataTable* dt = GetCharacterLookOverrideDataTable();
		if (dt && !InLoadout.CharacterLookOverride.IsEmpty())
		{
			FCharacterLookOverride* LookupRow = dt->FindRow<FCharacterLookOverride>(*InLoadout.CharacterLookOverride, "Character Override Lookup", false);
			if (LookupRow)
			{
#ifdef CHECK_DLC_IS_VALID
				if (LookupRow->LockedToDLC != EDLC::DLC_None)
				{
					if (!UGameFeatureLibrary::IsGameVersionEnabled(LookupRow->LockedToDLC))
						InLoadout.CharacterLookOverride = "SWAT_Judge";
				}
#endif
			}
		}
		
		RemoveLockedDLC(InLoadout.Primary);
		RemoveLockedDLC(InLoadout.Secondary);
		RemoveLockedDLC(InLoadout.LongTactical);
		RemoveLockedDLC(InLoadout.Armor);
		RemoveLockedDLC(InLoadout.Helmet);
		RemoveLockedDLC(InLoadout.RandomGear);
		RemoveLockedDLC(InLoadout.RandomGear);
		if (InLoadout.PrimarySkin && !InLoadout.PrimarySkin->GetDefaultObject<USkinComponent>()->HasDLCUnlocked())
		{
			V_LOGM(LogReadyOrNotLoadout, "Removing Locked Priamry Skin: %s", *InLoadout.PrimarySkin->GetName());
			InLoadout.PrimarySkin = nullptr;
		}
	
		if (InLoadout.SecondarySkin && !InLoadout.SecondarySkin->GetDefaultObject<USkinComponent>()->HasDLCUnlocked())
		{
			V_LOGM(LogReadyOrNotLoadout, "Removing Secondary Skin: %s", *InLoadout.SecondarySkin->GetName());
			InLoadout.SecondarySkin = nullptr;
		}
	}

	bool bUpdatedLoadout = false;

	FSavedLoadout DefaultLoadout;
	if (ItemData->DefaultLoadouts.IsValidIndex(0))
		DefaultLoadout = ItemData->DefaultLoadouts[0];
	
	bool bFoundPrimary = false;
	if (ABaseItem* Primary = InLoadout.Primary.GetDefaultObject())
	{
		bFoundPrimary = Primary->bShowInLoadout;
	}
	if (!bFoundPrimary)
	{
		if (InLoadout.Primary)
		{
			V_LOGM(LogReadyOrNotLoadout, "Primary Not Found In Item Data: %s", *InLoadout.Primary->GetName());
		}
		else
		{
			V_LOGM(LogReadyOrNotLoadout, "Primary Not Found In Item Data: No Primary Weapon");
		}
		
		InLoadout.Primary = DefaultLoadout.Primary;
		bUpdatedLoadout = true;
	}
	
	bool bFoundSecondary = false;
	if (ABaseItem* Secondary = InLoadout.Secondary.GetDefaultObject())
	{
		bFoundSecondary = Secondary->bShowInLoadout;
	}
	if (!bFoundSecondary)
	{
		if (InLoadout.Secondary)
		{
			V_LOGM(LogReadyOrNotLoadout, "Secondary Not Found In Item Data: %s", *InLoadout.Secondary->GetName());
		}
		else
		{
			V_LOGM(LogReadyOrNotLoadout, "Secondary Not Found In Item Data: No Primary Weapon");
		}
		
		bUpdatedLoadout = true;
		InLoadout.Secondary = DefaultLoadout.Secondary;
	}

	if (!InLoadout.LongTactical || InLoadout.LongTactical->HasAnyClassFlags(CLASS_Abstract))
	{
		V_LOGM(LogReadyOrNotLoadout, "No Long Tactical Found");
		bUpdatedLoadout = true;
		
		InLoadout.LongTactical = DefaultLoadout.LongTactical;
	} 
	
	if (!InLoadout.Armor || InLoadout.Armor->HasAnyClassFlags(CLASS_Abstract))
	{
		V_LOGM(LogReadyOrNotLoadout, "No Armor Found");
		bUpdatedLoadout = true;

		InLoadout.Armor = DefaultLoadout.Armor;
	}

	if (!InLoadout.Helmet || InLoadout.Helmet->HasAnyClassFlags(CLASS_Abstract))
	{
		V_LOGM(LogReadyOrNotLoadout, "No Helmet Found");
		bUpdatedLoadout = true;

		InLoadout.Helmet = DefaultLoadout.Helmet;
	}

	if (!InLoadout.ArmourMaterial)
	{
		V_LOGM(LogReadyOrNotLoadout, "No Armor Material Found");
		bUpdatedLoadout = true;

		InLoadout.ArmourMaterial = DefaultLoadout.ArmourMaterial;
	}

	int32 MaxSlots = 10;
	if (const ASWATArmour* SWATArmourCDO = Cast<ASWATArmour>(InLoadout.Armor.GetDefaultObject()))
	{
		MaxSlots = SWATArmourCDO->TotalSlots;
	}
	if (SanitizeSlots(InLoadout, MaxSlots))
	{
		bUpdatedLoadout = true;
	}

	InLoadout.GrenadeSlots.RemoveAll([&bUpdatedLoadout](TSubclassOf<ABaseItem>& Item)
		{
			if (Item == nullptr ||
				Item->HasAnyClassFlags(CLASS_Abstract))
			{
				V_LOGM(LogReadyOrNotLoadout, "Invalid Grenade Slot");

				bUpdatedLoadout = true;
				return true;
			}
			return false;
		});

	InLoadout.TacticalSlots.RemoveAll([&bUpdatedLoadout](TSubclassOf<ABaseItem>& Item)
		{
			if (Item == nullptr ||
				Item->HasAnyClassFlags(CLASS_Abstract))
			{
				V_LOGM(LogReadyOrNotLoadout, "Invalid Tactical Slot");

				bUpdatedLoadout = true;
				return true;
			}
			return false;
		});

	return bUpdatedLoadout;
}

bool UBpGameplayHelperLib::SanitizeSlots(FSavedLoadout& InLoadout, int32 MaxSlots)
{
	UItemData* ItemData = GetItemData(GetWorldStatic());
	if (!ItemData)
		return false;

	int32 CurrentSlots = 0;
	CurrentSlots += FMath::Max(InLoadout.PrimaryAmmoSlotsCount, 0);
	CurrentSlots += FMath::Max(InLoadout.SecondaryAmmoSlotsCount, 0);
	CurrentSlots += FMath::Max(InLoadout.GrenadeSlotsCount, 0);
	CurrentSlots += FMath::Max(InLoadout.TacticalSlotsCount, 0);
	bool bResetSlotCounts = CurrentSlots > MaxSlots;

	// If any of our slots are negative or exceed 255, reset all slots
	if (InLoadout.PrimaryAmmoSlotsCount < 0 || InLoadout.PrimaryAmmoSlotsCount > 255 ||
		InLoadout.SecondaryAmmoSlotsCount < 0 || InLoadout.SecondaryAmmoSlotsCount > 255 ||
		InLoadout.GrenadeSlotsCount < 0 || InLoadout.GrenadeSlotsCount > 255 ||
		InLoadout.TacticalSlotsCount < 0 || InLoadout.TacticalSlotsCount > 255)
	{
		bResetSlotCounts = true;
	}

	if (!bResetSlotCounts)
		return false;

	ASWATArmour* CurrentArmour = Cast<ASWATArmour>(InLoadout.Armor.GetDefaultObject());

	// Primary Ammo Slots
	InLoadout.PrimaryAmmoSlotsCount = CurrentArmour ? CurrentArmour->DefaultPrimaryAmmoSlots : -1;
	if (InLoadout.PrimaryAmmoSlotsCount < 0 && ItemData->DefaultLoadouts.IsValidIndex(0))
		InLoadout.PrimaryAmmoSlotsCount = ItemData->DefaultLoadouts[0].PrimaryAmmoSlotsCount;

	// Secondary Ammo Slots
	InLoadout.SecondaryAmmoSlotsCount = CurrentArmour ? CurrentArmour->DefaultSecondaryAmmoSlots : -1;
	if (InLoadout.SecondaryAmmoSlotsCount < 0 && ItemData->DefaultLoadouts.IsValidIndex(0))
		InLoadout.SecondaryAmmoSlotsCount = ItemData->DefaultLoadouts[0].SecondaryAmmoSlotsCount;

	// Grenade Slots
	InLoadout.GrenadeSlotsCount = CurrentArmour ? CurrentArmour->DefaultGrenadeSlots : -1;
	if (InLoadout.GrenadeSlotsCount < 0 && ItemData->DefaultLoadouts.IsValidIndex(0))
		InLoadout.GrenadeSlotsCount = ItemData->DefaultLoadouts[0].GrenadeSlotsCount;

	// Tactical Slots
	InLoadout.TacticalSlotsCount = CurrentArmour ? CurrentArmour->DefaultTacticalDeviceSlots : -1;
	if (InLoadout.TacticalSlotsCount < 0 && ItemData->DefaultLoadouts.IsValidIndex(0))
		InLoadout.TacticalSlotsCount = ItemData->DefaultLoadouts[0].TacticalSlotsCount;

	// Hard limit even modded slots to 255
	InLoadout.PrimaryAmmoSlotsCount = FMath::Clamp(InLoadout.PrimaryAmmoSlotsCount, 0, 255);
	InLoadout.SecondaryAmmoSlotsCount = FMath::Clamp(InLoadout.SecondaryAmmoSlotsCount, 0, 255);
	InLoadout.GrenadeSlotsCount = FMath::Clamp(InLoadout.GrenadeSlotsCount, 0, 255);
	InLoadout.TacticalSlotsCount = FMath::Clamp(InLoadout.TacticalSlotsCount, 0, 255);

	return true;;
}

bool UBpGameplayHelperLib::RemoveLockedDLC(TSubclassOf<ABaseItem>& Item)
{
	if (!Item)
		return false;
	
	ABaseItem* DefaultItem = Item->GetDefaultObject<ABaseItem>();
	if (DefaultItem && DefaultItem->LockedToDLC.Num() > 0)
	{
		for (EGameVersionRestriction DLC : DefaultItem->LockedToDLC)
		{
			if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC))
			{
				V_LOGM(LogReadyOrNotLoadout, "Removing Locked DLC: %s", *Item->GetName());
				Item = nullptr;
				
				return true;
			}
		}
	}
	
	return false;
}

bool UBpGameplayHelperLib::IsDLCLocked(TSubclassOf<ABaseItem> Item)
{
	if (!Item)
		return false;
	
	ABaseItem* DefaultItem = Item->GetDefaultObject<ABaseItem>();
	if (DefaultItem && DefaultItem->LockedToDLC.Num() > 0)
	{
		for (EGameVersionRestriction DLC : DefaultItem->LockedToDLC)
		{
			if (!UGameFeatureLibrary::IsGameVersionEnabled(DLC))
			{
				Item = nullptr;
				return true;
			}
		}
	}
	
	return false;
}

void UBpGameplayHelperLib::AddDefaultItemsToPlayer(AReadyOrNotCharacter* Player)
{
	if (!Player)
		return;
	
	UItemData* ItemData = GetItemData(Player->GetWorld());

	if (!ItemData)
		return;

	for (int32 i = 0; i < ItemData->DefaultItemsGivenToPlayer.Num(); i++)
	{
		UClass* TargetClass = ItemData->DefaultItemsGivenToPlayer[i];
		if (!TargetClass)
			continue;

		// HACK: don't give tablets to AI
		if (!Player->IsA(APlayerCharacter::StaticClass()) && TargetClass->IsChildOf(ATablet::StaticClass()))
			continue;
		
		if (!Player->GetInventoryComponent()->HasAnyInventoryItemsOfClass(TargetClass))
		{
			ABaseItem* NewItem = Player->GetWorld()->SpawnActor<ABaseItem>(TargetClass, Player->GetActorTransform());
			if (NewItem)
			{
				Player->GetInventoryComponent()->AddInventoryItem(NewItem);
			}
		}
	
	}
}

bool UBpGameplayHelperLib::GetLoadoutNames(TArray<FString>& LoadoutNames)
{
	UBaseProfile* Profile = UBaseProfile::GetCurrentProfile();
	if (!Profile)
		return false;

	for (const FSavedLoadout& SavedLoadout : Profile->Loadouts)
	{
		LoadoutNames.Add(SavedLoadout.Name);
	}
	return true;
}

FString UBpGameplayHelperLib::ConvertFloatToStringMinutes(float Val)
{
	const bool bNegativeNumber = Val < 0;

	Val = FMath::Abs(Val);
	const int32 Minutes = int32(Val / 60) % 60;
	const int32 Seconds = int32(Val) % 60;

	FString FormattedTimeString = (bNegativeNumber ? "-" : "") + (Minutes >= 10 ? FString::FromInt(Minutes) : "0" + FString::FromInt(Minutes)) + ":" + (Seconds >= 10 ? FString::FromInt(Seconds) : "0" + FString::FromInt(Seconds));
	return FormattedTimeString;
}

FString UBpGameplayHelperLib::ConvertFloatToStringMinutes_Detail(float Val)
{
	const bool bNegativeNumber = Val < 0;

	Val = FMath::Abs(Val);
	const int32 Minutes = int32(Val / 60) % 60;
	const int32 Seconds = int32(Val) % 60;
	const int32 Milliseconds = (int32(Val * 1000) % 1000) / 10;

	FString FormattedTimeString = (bNegativeNumber ? "-" : "") + (Minutes >= 10 ? FString::FromInt(Minutes) : "0" + FString::FromInt(Minutes)) + ":" + (Seconds >= 10 ? FString::FromInt(Seconds) : "0" + FString::FromInt(Seconds)) + ":" + (Milliseconds >= 10 ? FString::FromInt(Milliseconds) : "0" + FString::FromInt(Milliseconds));
	return FormattedTimeString;
}

FVector2D UBpGameplayHelperLib::ConvertSquareVectorToCircle(FVector2D sv)
{
	FVector2D cv;

	if ((FMath::Abs(sv.X) != 0.0f) || (FMath::Abs(sv.Y) != 0.0f))
	{
		//cv.X = sv.X;
		//cv.Y = sv.Y;
		return sv;
	}

	const float x2 = sv.X * sv.X;
	const float y2 = sv.Y * sv.Y;
	const float r2 = x2 + y2;
	const float rad = sqrt(r2 - x2 * y2);

	// This code is amenable to the fast reciprocal sqrt floating point trick
	// https://en.wikipedia.org/wiki/Fast_inverse_square_root
	const float reciprocalSqrt = 1.0 / sqrt(r2);

	cv.X = sv.X * rad * reciprocalSqrt;
	cv.Y = sv.Y * rad * reciprocalSqrt;

	return cv;
}

bool UBpGameplayHelperLib::SaveMasterVolume(float Volume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MasterSoundVolume = Volume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveUIVolume(float Volume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->UISoundVolume = Volume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveSFXVolume(float Volume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->SFXSoundVolume = Volume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveMusicVolume(float Volume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MusicSoundVolume = Volume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveVOIPVolume(float Volume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->VOIPVolume = Volume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetHitmarkerSfxEnabled(bool& bHitmarkerSfxEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bHitmarkerSfxEnabled = us->bHitmarkerSfxEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveHitmarkerSfxEnabled(bool bHitmarkerSfxEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bHitmarkerSfxEnabled = bHitmarkerSfxEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveMaxShellsInWorld(int32 NewMaxShells)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->MaxShellsInWorld = NewMaxShells;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadMaxShellsInWorld(int32& MaxShells)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		MaxShells = us->MaxShellsInWorld;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetVolumes(float& MasterVolume, float& UIVolume, float& SFXVolume, float& MusicVolume, float& VOIPVolume)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		MasterVolume = us->MasterSoundVolume;
		UIVolume = us->UISoundVolume;
		SFXVolume = us->SFXSoundVolume;
		MusicVolume = us->MusicSoundVolume;
		VOIPVolume = us->VOIPVolume;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetVoiceType(EVoiceType& OutVoiceType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		OutVoiceType = us->DefaultVOIPChannel;
		return true;
	}
	return false;
}

void UBpGameplayHelperLib::SetVoiceType(EVoiceType InVoiceType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->DefaultVOIPChannel = InVoiceType;
	}
}

void UBpGameplayHelperLib::ChangeLocalization(FString Target)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		if (FInternationalization::Get().SetCurrentCulture(Target))
		{
			us->TargetLocale = Target;
			us->SaveSettings();
		}
	}
}

bool UBpGameplayHelperLib::GetLocalization(FString& Target)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		Target = us->TargetLocale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetRandomLoadingScreenTip(FText& Tip)
{
	UDataSingleton* ds = Cast<UDataSingleton>(GetRoNData());
	if (ds)
	{
		if (ds->LoadingScreen_Tips.Num() > 0)
			Tip = ds->LoadingScreen_Tips[FMath::RandRange(0, ds->LoadingScreen_Tips.Num() - 1)];
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::IsSupporterOnlyBuild()
{
#ifdef SUPPORTER_ONLY_BUILD
	return true;
#endif
	return false;
}

UTexture2D* UBpGameplayHelperLib::GetLoadingScreenLevelImage(FString Level)
{
	return nullptr;
}

bool UBpGameplayHelperLib::SaveToggleADS(bool ToggleADS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bToggleADS = ToggleADS;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadToggleADS(bool& ToggleADS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		ToggleADS = us->bToggleADS;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveHoldCrouch(bool HoldCrouch)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bHoldCrouch = HoldCrouch;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadHoldCrouch(bool& HoldCrouch)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		HoldCrouch = us->bHoldCrouch;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveTogglePS5Gyro(bool TogglePS5Gyro)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bTogglePS5Gyro = TogglePS5Gyro;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadTogglePS5Gyro(bool& TogglePS5Gyro)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		TogglePS5Gyro = us->bTogglePS5Gyro;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveControlScheme(bool UsingAlternateControls)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bUsingAlternateControls = UsingAlternateControls;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadControlScheme(bool& UsingAlternateControls)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		UsingAlternateControls = us->bUsingAlternateControls;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveServersideChecksum(bool bServerSideChecksumEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bServerSideChecksum = bServerSideChecksumEnabled;
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadServersideChecksum(bool& bServerSideChecksumEnabled)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bServerSideChecksumEnabled = us->bServerSideChecksum;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorResolutionScale(const float ResolutionScale)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->MirrorReflectionSettings.MirrorResolutionScale = ResolutionScale;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorResolutionScale(float& ResolutionScale)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		ResolutionScale = us->MirrorReflectionSettings.MirrorResolutionScale;
		return true;
	}

	ResolutionScale = 50.0f;
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorAntiAliasEnabled(bool bShowAntiAlias)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->MirrorReflectionSettings.bShowAntiAliasing = bShowAntiAlias;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorAntiAliasEnabled(bool& bShowAntiAlias)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowAntiAlias = us->MirrorReflectionSettings.bShowAntiAliasing;
		return true;
	}

	bShowAntiAlias = false;
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorDecalsEnabled(bool bShowDecals)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->MirrorReflectionSettings.bShowDecals = bShowDecals;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorDecalsEnabled(bool& bShowDecals)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowDecals = us->MirrorReflectionSettings.bShowDecals;
		return true;
	}

	bShowDecals = false;
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorDynamicShadowsEnabled(bool bShowDynamicShadows)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->MirrorReflectionSettings.bShowDynamicShadows = bShowDynamicShadows;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorDynamicShadowsEnabled(bool& bShowDynamicShadows)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowDynamicShadows = us->MirrorReflectionSettings.bShowDynamicShadows;
		return true;
	}

	bShowDynamicShadows = false;
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorReflectionEnabled(const bool bEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bMirrorReflectionEnabled = bEnabled;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorReflectionEnabled(bool& bEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bEnabled = us->bMirrorReflectionEnabled;
		return true;
	}

	bEnabled = false;
	return false;
}

bool UBpGameplayHelperLib::SaveMirrorEnabledOnlyInLobby(bool bEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bMirrorInLobbyOnly = bEnabled;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadMirrorEnabledOnlyInLobby(bool& bEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bEnabled = us->bMirrorInLobbyOnly;
		return true;
	}

	bEnabled = false;
	return false;
}

bool UBpGameplayHelperLib::SavePiPFPS(bool bEnabled, float FPS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->PiPFPS = FPS;
		us->bPiPFPSEnabled = bEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadPiPFPS(bool& bEnabled, float& FPS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		FPS = us->PiPFPS;
		bEnabled = us->bPiPFPSEnabled;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SavePiPResolutionScale(float ResolutionScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->PiPResolutionScale = ResolutionScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadPiPResolutionScale(float& ResolutionScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		ResolutionScale = us->PiPResolutionScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveOptiwandViewMode(const EOptiwandViewMode OptiwandViewMode)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->OptiwandViewMode = OptiwandViewMode;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadOptiwandViewMode(EOptiwandViewMode& OptiwandViewMode)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		OptiwandViewMode = us->OptiwandViewMode;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::IsDMOBuild()
{
#if defined DMO_BUILD
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::IsDMOPVPOnly()
{
#if defined DMO_PVP_ONLY
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::IsDMOMatchMake()
{
#if defined DMO_MATCHMAKE
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::IsRTXDMOBuild()
{
#if defined RTX_DMO
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::IsPreMissionBriefingBeforeLoadout()
{
#if defined PREMISSION_BRIEFING_BEFORE_LOADOUT
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::SaveKeybinds()
{
	if (UInputSettings* InputSettings = UInputSettings::GetInputSettings())
	{
		// Resave to config and rebuild keymaps again
		InputSettings->RemoveInvalidKeys();
		InputSettings->SaveKeyMappings();
		InputSettings->SaveConfig(CPF_Config, *GInputIni);		
		InputSettings->ForceRebuildKeymaps();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::IsShowHUDEnabled()
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		return us->bShowHUD;
	}

	return true;
}
bool UBpGameplayHelperLib::SaveShowHUD(bool bShowHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowHUD = bShowHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowHUD(bool& bShowHud)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowHud = us->bShowHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveCurvedHUD(bool bCurvedHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bCurvedHUD = bCurvedHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadCurvedHUD(bool& bCurvedHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bCurvedHUD = us->bCurvedHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowCompass(bool bShowCompass)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowCompass = bShowCompass;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowCompass(bool& bShowCompass)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowCompass = us->bShowCompass;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowWeaponHUD(bool bShowWeaponHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowWeaponHUD = bShowWeaponHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowWeaponHUD(bool& bShowWeaponHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowWeaponHUD = us->bShowWeaponHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowMagazineHUD(bool bShowMagazineHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowMagazineHUD = bShowMagazineHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowMagazineHUD(bool& bShowMagazineHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowMagazineHUD = us->bShowMagazineHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowChat(bool bShowChat)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowChat = bShowChat;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowChat(bool& bShowChat)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowChat = us->bShowChat;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveSwayHUD(bool bSwayHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bEnableHUDSwaying = bSwayHUD;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadSwayHUD(bool& bSwayHUD)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bSwayHUD = us->bEnableHUDSwaying;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::Save2DReload(bool b2DReload)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->b2DReloadIcons = b2DReload;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::Load2DReload(bool& b2DReload)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		b2DReload = us->b2DReloadIcons;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveIconScale(float IconScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->IconScale = IconScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadIconScale(float& IconScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		IconScale = us->IconScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveQuickThrowScale(float QuickThrowScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->QuickThrowScale = QuickThrowScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadQuickThrowScale(float& QuickThrowScale)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		QuickThrowScale = us->QuickThrowScale;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveFireModeDisplayOption(int32 FireModeDisplayOption)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->FireModeDisplayOption = FireModeDisplayOption;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadFireModeDisplayOption(int32& FireModeDisplayOption)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		FireModeDisplayOption = us->FireModeDisplayOption;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowMultiplayerNames(bool bShowMultiplayerNames)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowPlayerNamePlates = bShowMultiplayerNames;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowMultiplayerNames(bool& bShowMultiplayerNames)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowMultiplayerNames = us->bShowPlayerNamePlates;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowButtonPrompts(bool bShowButtonPrompts)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowButtonPrompts = bShowButtonPrompts;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShowButtonPrompts(bool& bShowButtonPrompts)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowButtonPrompts = us->bShowButtonPrompts;
		return true;
	}
	return false;
}



bool UBpGameplayHelperLib::SaveHUDSettings(bool bShowHUD /*= false*/, bool bCurvedHUD /*= true*/, bool bShowCompass /*= false*/, bool bShowWeaponHUD /*= false*/, bool bShowMagazineHUD /*= false*/, bool bShowChat /*= false*/, bool bSwayHUD /*= false*/, bool b2DReload /*= false*/, float IconScale /*= 1.0f*/, float QuickThrowScale /*= 1.0f*/, int32 FireModeDisplayOption /*= 0*/, bool bShowMultiplayerNames /*=true*/, bool bShowButtonPrompts /*=true*/)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowHUD = bShowHUD;
		us->bCurvedHUD = bCurvedHUD;
		us->bShowCompass = bShowCompass;
		us->bShowWeaponHUD = bShowWeaponHUD;
		us->bShowMagazineHUD = bShowMagazineHUD;
		us->bShowChat = bShowChat;
		us->bEnableHUDSwaying = bSwayHUD;
		us->b2DReloadIcons = b2DReload;
		us->IconScale = IconScale;
		us->QuickThrowScale = QuickThrowScale;
		us->FireModeDisplayOption = FireModeDisplayOption;
		us->bShowPlayerNamePlates = bShowMultiplayerNames;
		//us->bShowMultiplayerSpeeds = bShowMultiplayerSpeeds;
		us->bShowButtonPrompts = bShowButtonPrompts;

		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadHUDSettings(bool& bShowHud, bool& bCurvedHUD, bool& bShowCompass, bool& bShowWeaponHUD, bool& bShowMagazineHUD, bool& bShowChat, bool& bSwayHUD, bool& b2DReload, float& IconScale, float& QuickThrowScale, int32& FireModeDisplayOption, bool& bShowPlayerNames, bool& bShowButtonPrompts)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowHud = us->bShowHUD;
		bCurvedHUD = us->bCurvedHUD;
		bShowCompass = us->bShowCompass;
		bShowWeaponHUD = us->bShowWeaponHUD;
		bShowMagazineHUD = us->bShowMagazineHUD;
		bShowChat = us->bShowChat;
		bSwayHUD = us->bEnableHUDSwaying;
		b2DReload = us->b2DReloadIcons;
		IconScale = us->IconScale;
		QuickThrowScale = us->QuickThrowScale;
		FireModeDisplayOption = us->FireModeDisplayOption;
		bShowPlayerNames = us->bShowPlayerNamePlates;
		//bShowMultiplayerSpeeds = us->bShowMultiplayerSpeeds;
		bShowButtonPrompts = us->bShowButtonPrompts;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveShowHUDSetting(const bool bShowHUD)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowHUD = bShowHUD;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadShowHUDSetting(bool& bShowHud)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowHud = us->bShowHUD;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowPlayerNamesSetting(bool& bShowPlayerNames)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowPlayerNames = us->bShowPlayerNamePlates;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveShowHesitationBarSetting(const bool bShowHesitationBar)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowHesitationBar = bShowHesitationBar;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowHesitationBarSetting(bool& bShowHesitationBar)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowHesitationBar = us->bShowHesitationBar;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveShowPlayerIconSetting(const bool bShowPlayerIcon)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowPlayerIcon = bShowPlayerIcon;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowPlayerIconSetting(bool& bShowPlayerIcon)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowPlayerIcon = us->bShowPlayerIcon;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadSafeZoneSettings(float& SafeZoneX, float& SafeZoneY)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		SafeZoneX = us->SafeZoneX;
		SafeZoneY = us->SafeZoneY;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveSafeZoneSettings(const float SafeZoneX, const float SafeZoneY)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->SafeZoneX = SafeZoneX;
		us->SafeZoneY = SafeZoneY;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadTeamViewFPSSetting(bool& bEnabled, int32& TeamViewFPS)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		TeamViewFPS = us->TeamViewFPS;
		bEnabled = us->bTeamViewFPSEnabled;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveTeamViewSetting(bool bEnabled, const int32 TeamViewFPS)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->TeamViewFPS = TeamViewFPS;
		us->bTeamViewFPSEnabled = bEnabled;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveScoreReadoutSetting(const EScoreReadoutMode InScoreReadoutMode)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->ScoreReadoutMode = InScoreReadoutMode;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadScoreReadoutSetting(EScoreReadoutMode& OutScoreReadoutMode)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		OutScoreReadoutMode = us->ScoreReadoutMode;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveShowCommandContextHintSetting(const bool bShowCommandContextHint)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowCommandContextHint = bShowCommandContextHint;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowCommandContextHintSetting(bool& bShowCommandContextHint)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowCommandContextHint = us->bShowCommandContextHint;
		return true;
	}
	
	bShowCommandContextHint = false;
	return false;
}

bool UBpGameplayHelperLib::SaveZoomADSSetting(bool bZoomADS)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bZoomADS = bZoomADS;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadZoomADSSetting(bool& bZoomADS)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bZoomADS = us->bZoomADS;
		return true;
	}
	
	bZoomADS = false;
	return false;
}

bool UBpGameplayHelperLib::SaveColorblindStrength(float ColorblindStrength)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->ColorVisionDeficiencyStrength = ColorblindStrength;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadColorblindStrength(float& ColorblindStrength)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		ColorblindStrength = us->ColorVisionDeficiencyStrength;
		return true;
	}
	
	ColorblindStrength = 1.0f;
	return false;
}

bool UBpGameplayHelperLib::SaveColorblindMode(EColorVisionDeficiency ColorVisionDeficiency)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->ColorVisionDeficiency = ColorVisionDeficiency;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadColorblindMode(EColorVisionDeficiency& ColorVisionDeficiency)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		ColorVisionDeficiency = us->ColorVisionDeficiency;
		return true;
	}
	
	ColorVisionDeficiency = EColorVisionDeficiency::NormalVision;
	return false;
}

bool UBpGameplayHelperLib::SaveHighlightWeapons(bool bHighlightWeapons)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bHighlightWeapons = bHighlightWeapons;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadHighlightWeapons(bool& bHighlightWeapons)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bHighlightWeapons = us->bHighlightWeapons;
		return true;
	}
	
	bHighlightWeapons = false;
	return false;
}

bool UBpGameplayHelperLib::SaveWorldSpaceActionPrompts(bool bWorldSpaceActionPrompts)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bWorldSpaceActionPrompts = bWorldSpaceActionPrompts;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadWorldSpaceActionPrompts(bool& bWorldSpaceActionPrompts)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bWorldSpaceActionPrompts = us->bWorldSpaceActionPrompts;
		return true;
	}
	
	bWorldSpaceActionPrompts = false;
	return false;
}

bool UBpGameplayHelperLib::SaveHotkeyHintSetting(const bool bShowHotkeyHint)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowHotkeyHints = bShowHotkeyHint;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadHotkeyHintSetting(bool& bShowHotkeyHint)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowHotkeyHint = us->bShowHotkeyHints;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveShowHealthIconSetting(bool bShowHealthIcons)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowHealthIcons = bShowHealthIcons;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowHealthIconSetting(bool& bShowHealthIcons)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bShowHealthIcons = us->bShowHealthIcons;
		
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveItemSelectionStyleSettings(const EItemSelectionInterfaceType ItemSelectionInterface)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->ItemSelectionInterface = ItemSelectionInterface;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadItemSelectionStyleSettings(EItemSelectionInterfaceType& ItemSelectionInterface)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		ItemSelectionInterface = us->ItemSelectionInterface;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShowTeamStatus(const bool bShowTeamStatus)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bShowTeamStatus = bShowTeamStatus;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadShowTeamStatus(bool& bShowTeamStatus)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowTeamStatus = us->bShowTeamStatus;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveGrenadeSettings(EGrenadeThrowSettingType GrenadeThrowType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->GrenadeThrowType = GrenadeThrowType;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadGrenadeSettings(EGrenadeThrowSettingType& GrenadeThrowType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		GrenadeThrowType = us->GrenadeThrowType;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveShotgunSettings(EShotgunReloadType ShotgunReloadType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->ShotgunLoadType = ShotgunReloadType;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::LoadShotgunSettings(EShotgunReloadType& ShotgunReloadType)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		ShotgunReloadType = us->ShotgunLoadType;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SaveEmptyMagReloadSettings(const EEmptyMagReloadType EmptyMagReloadType)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->EmptyMagReloadType = EmptyMagReloadType;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadEmptyMagReloadSettings(EEmptyMagReloadType& EmptyMagReloadType)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		EmptyMagReloadType = us->EmptyMagReloadType;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveDefaultCommand(ESwatCommand DefaultCommand/*, ESwatCommand DefaultHumanCommand*/, ESwatCommand DefaultDoorUnknownCommand, ESwatCommand DefaultDoorOpenCommand, ESwatCommand DefaultDoorLockedCommand, ESwatCommand DefaultDoorUnlockedCommand)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultCommand = DefaultCommand;
		//us->DefaultHumanCommand = DefaultHumanCommand;
		us->DefaultDoorUnknownCommand = DefaultDoorUnknownCommand;
		us->DefaultDoorOpenCommand = DefaultDoorOpenCommand;
		us->DefaultDoorLockedCommand = DefaultDoorLockedCommand;
		us->DefaultDoorUnlockedCommand = DefaultDoorUnlockedCommand;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadDefaultCommands(ESwatCommand& DefaultCommand/*, ESwatCommand& DefaultHumanCommand*/, ESwatCommand& DefaultDoorUnknownCommand, ESwatCommand& DefaultDoorOpenCommand, ESwatCommand& DefaultDoorLockedCommand, ESwatCommand& DefaultDoorUnlockedCommand)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultCommand = us->DefaultCommand;
		//DefaultHumanCommand = us->DefaultHumanCommand;
		DefaultDoorUnknownCommand = us->DefaultDoorUnknownCommand;
		DefaultDoorOpenCommand = us->DefaultDoorOpenCommand;
		DefaultDoorLockedCommand = us->DefaultDoorLockedCommand;
		DefaultDoorUnlockedCommand = us->DefaultDoorUnlockedCommand;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveDefaultSurfaceCommand(ESwatCommand DefaultCommand, int32 DefaultCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultCommand = DefaultCommand;
		us->DefaultCommandOption = DefaultCommandIndex;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadDefaultSurfaceCommand(ESwatCommand& DefaultCommand, int32& DefaultCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultCommand = us->DefaultCommand;
		DefaultCommandIndex = us->DefaultCommandOption;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveDefaultDoorUnknownCommand(ESwatCommand DefaultDoorUnknownCommand, int32 DefaultDoorUnknownCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultDoorUnknownCommand = DefaultDoorUnknownCommand;
		us->DefaultDoorUnknownCommandOption = DefaultDoorUnknownCommandIndex;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadDefaultDoorUnknownCommand(ESwatCommand& DefaultDoorUnknownCommand, int32& DefaultDoorUnknownCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultDoorUnknownCommand = us->DefaultDoorUnknownCommand;
		DefaultDoorUnknownCommandIndex = us->DefaultDoorUnknownCommandOption;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveDefaultDoorOpenCommand(ESwatCommand DefaultDoorOpenCommand, int32 DefaultDoorOpenCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultDoorOpenCommand = DefaultDoorOpenCommand;
		us->DefaultDoorOpenCommandOption = DefaultDoorOpenCommandIndex;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadDefaultDoorOpenCommand(ESwatCommand& DefaultDoorOpenCommand, int32& DefaultDoorOpenCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultDoorOpenCommand = us->DefaultDoorOpenCommand;
		DefaultDoorOpenCommandIndex = us->DefaultDoorOpenCommandOption;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveDefaultDoorLockedCommand(ESwatCommand DefaultDoorLockedCommand, int32 DefaultDoorLockedCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultDoorLockedCommand = DefaultDoorLockedCommand;
		us->DefaultDoorLockedCommandOption = DefaultDoorLockedCommandIndex;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadDefaultDoorLockedCommand(ESwatCommand& DefaultDoorLockedCommand, int32& DefaultDoorLockedCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultDoorLockedCommand = us->DefaultDoorLockedCommand;
		DefaultDoorLockedCommandIndex = us->DefaultDoorLockedCommandOption;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveDefaultDoorUnlockedCommand(ESwatCommand DefaultDoorUnlockedCommand, int32 DefaultDoorUnlockedCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultDoorUnlockedCommand = DefaultDoorUnlockedCommand;
		us->DefaultDoorUnlockedCommandOption = DefaultDoorUnlockedCommandIndex;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadDefaultDoorUnlockedCommand(ESwatCommand& DefaultDoorUnlockedCommand, int32& DefaultDoorUnlockedCommandIndex)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultDoorUnlockedCommand = us->DefaultDoorUnlockedCommand;
		DefaultDoorUnlockedCommandIndex = us->DefaultDoorUnlockedCommandOption;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveDefaultCommandAsOption(int32 DefaultCommandOption/*, int32 DefaultHumanCommandOption*/, int32 DefaultDoorUnknownCommandOption, int32 DefaultDoorOpenCommandOption, int32 DefaultDoorLockedCommandOption, int32 DefaultDoorUnlockedCommandOption)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->DefaultCommandOption = DefaultCommandOption;
		//us->DefaultHumanCommandOption = DefaultHumanCommandOption;
		us->DefaultDoorUnknownCommandOption = DefaultDoorUnknownCommandOption;
		us->DefaultDoorOpenCommandOption = DefaultDoorOpenCommandOption;
		us->DefaultDoorLockedCommandOption = DefaultDoorLockedCommandOption;
		us->DefaultDoorUnlockedCommandOption = DefaultDoorUnlockedCommandOption;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadDefaultCommandsAsOption(int32& DefaultCommandOption/*, int32& DefaultHumanCommandOption*/, int32& DefaultDoorUnknownCommandOption, int32& DefaultDoorOpenCommandOption, int32& DefaultDoorLockedCommandOption, int32& DefaultDoorUnlockedCommandOption)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		DefaultCommandOption = us->DefaultCommandOption;
		//DefaultHumanCommandOption = us->DefaultHumanCommandOption;
		DefaultDoorUnknownCommandOption = us->DefaultDoorUnknownCommandOption;
		DefaultDoorOpenCommandOption = us->DefaultDoorOpenCommandOption;
		DefaultDoorLockedCommandOption = us->DefaultDoorLockedCommandOption;
		DefaultDoorUnlockedCommandOption = us->DefaultDoorUnlockedCommandOption;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveNVGStyle(const ENVGStyle NewNVGStyle)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->NVGStyle = NewNVGStyle;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadNVGStyle(ENVGStyle& NVGStyle)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		NVGStyle = us->NVGStyle;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveLowReadyStyle(bool bUseHighReady)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bUseHighReadyStyle = bUseHighReady;
		us->SaveSettings();
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::LoadLowReadyStyle(bool& bUseHighReady)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bUseHighReady = us->bUseHighReadyStyle;
		return true;
	}

	return false;
}

bool UBpGameplayHelperLib::SaveSubtitlesEnabled(bool bEnableSubtitles)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bEnableSubtitles = bEnableSubtitles;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadSubtitlesEnabled(bool& bEnableSubtitles)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bEnableSubtitles = us->bEnableSubtitles;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveSubtitlesSize(ESubtitlesSize SubtitlesSize)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->SubtitlesSize = SubtitlesSize;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadSubtitlesSize(ESubtitlesSize& SubtitlesSize)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		SubtitlesSize = us->SubtitlesSize;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveSubtitlesLocale(FString SubtitlesLocale)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->SubtitlesLocale = SubtitlesLocale;
		us->SaveSettings();

		USubtitlesStatics::SetLocale(GetWorldStatic(), SubtitlesLocale);
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadSubtitlesLocale(FString& SubtitlesLocale)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		SubtitlesLocale = us->SubtitlesLocale;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveSubtitlesBackgroundOpacity(float SubtitlesBackgroundOpacity)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->SubtitlesBackgroundOpacity = SubtitlesBackgroundOpacity;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadSubtitlesBackgroundOpacity(float& SubtitlesBackgroundOpacity)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		SubtitlesBackgroundOpacity = us->SubtitlesBackgroundOpacity;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveSubtitlesSpeed(float SubtitlesSpeed)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->SubtitlesSpeed = SubtitlesSpeed;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadSubtitlesSpeed(float& SubtitlesSpeed)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		SubtitlesSpeed = us->SubtitlesSpeed;
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::SaveReplayEnabled(bool bReplayEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		us->bReplayEnabled = bReplayEnabled;
		us->SaveSettings();
		return true;
	}
	
	return false;
}

bool UBpGameplayHelperLib::LoadReplayEnabled(bool& bReplayEnabled)
{
	if (UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings()))
	{
		bReplayEnabled = us->bReplayEnabled;
		return true;
	}
	
	return false;
}

ASuspectCharacter* UBpGameplayHelperLib::GetClosestActiveSuspect(const FVector& Location, float Distance, bool bMustHaveTarget)
{
	ASuspectCharacter* ClosestSpeaker = nullptr;

	for (TActorIterator<ASuspectCharacter>It(GetWorldStatic()); It; ++It)
	{
		ASuspectCharacter* CurrentSuspect = *It;
		if (!CurrentSuspect->GetCyberneticsController())
			continue;

		const bool bTrackedTargetPass = !bMustHaveTarget ? true : CurrentSuspect->GetCyberneticsController()->GetTrackedTarget() != nullptr;
		const float TestDistance = (CurrentSuspect->GetActorLocation() - Location).Size();
		if (bTrackedTargetPass && TestDistance < Distance && CurrentSuspect->IsDeadOrUnconscious() == false && CurrentSuspect->IsArrested() == false)
		{
			Distance = TestDistance;
			ClosestSpeaker = CurrentSuspect;
		}
	}

	return  ClosestSpeaker;
}

ACivilianCharacter* UBpGameplayHelperLib::GetClosestActiveCivilian(const FVector& Location, float Distance, bool bMustHaveTarget)
{
	ACivilianCharacter* ClosestSpeaker = nullptr;

	for (TActorIterator<ACivilianCharacter> It(GetWorldStatic()); It; ++It)
	{
		ACivilianCharacter* CurrentCivilian = *It;
		if (!CurrentCivilian->GetCyberneticsController())
			continue;

		bool bTrackedTargetPass = !bMustHaveTarget ? true : CurrentCivilian->GetCyberneticsController()->GetTrackedTarget() != nullptr;

		const float TestDistance = (CurrentCivilian->GetActorLocation() - Location).Size();
		if (bTrackedTargetPass && TestDistance < Distance && CurrentCivilian->IsDeadOrUnconscious() == false)
		{
			Distance = TestDistance;
			ClosestSpeaker = CurrentCivilian;
		}
	}

	return ClosestSpeaker;
}

UReadyOrNotSaveGame* UBpGameplayHelperLib::GetLoadGameInstance(FString LoadSlotName)
{
	UReadyOrNotSaveGame* LoadGameInstance = Cast<UReadyOrNotSaveGame>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotSaveGame::StaticClass()));

	if (LoadSlotName.IsEmpty())
		LoadSlotName = LoadGameInstance->SaveSlotName;

	LoadGameInstance = Cast<UReadyOrNotSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadSlotName, LoadGameInstance->UserIndex));
	if (!LoadGameInstance)
	{
		UReadyOrNotSaveGame* SaveGameInstance = UReadyOrNotSaveGame::CreateDefaultSavegame(LoadSlotName);
		LoadGameInstance = SaveGameInstance;
	}
	return LoadGameInstance;
}

UReadyOrNotProfile* UBpGameplayHelperLib::GetCurrentProfile(UWorld* WorldContext)
{
	if (WorldContext == nullptr)
	{
		WorldContext = GEngine->GameViewport->GetWorld();
		if (WorldContext == nullptr)
		{
			return nullptr;
		}
	}

	AReadyOrNotGameState* gs = WorldContext->GetGameState<AReadyOrNotGameState>();
	if (!gs)
	{
		return nullptr;
	}

	return gs->GetCurrentProfile();
}

UReadyOrNotMultiplayerProfile* UBpGameplayHelperLib::GetMultiplayerProfile(FString LoadSlotName/* = ""*/)
{

	UReadyOrNotMultiplayerProfile* MultiplayerProfile = Cast<UReadyOrNotMultiplayerProfile>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotMultiplayerProfile::StaticClass()));

	if (LoadSlotName.IsEmpty())
		LoadSlotName = MultiplayerProfile->SaveSlotName;

	MultiplayerProfile = Cast<UReadyOrNotMultiplayerProfile>(UGameplayStatics::LoadGameFromSlot(LoadSlotName, MultiplayerProfile->UserIndex));
	if (!MultiplayerProfile)
	{
		UReadyOrNotMultiplayerProfile* SaveProfile = Cast<UReadyOrNotMultiplayerProfile>(UReadyOrNotProfile::CreateDefaultSavegame(UReadyOrNotMultiplayerProfile::StaticClass(), LoadSlotName));
		MultiplayerProfile = SaveProfile;
	}
	return MultiplayerProfile;
}

FString UBpGameplayHelperLib::GetProjectVersion()
{
	#if WITH_EDITOR
	return "Editor";
	#endif
	
	FString ProjectVersion;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		ProjectVersion,
		GGameIni
	);
	return ProjectVersion;
}

int32 UBpGameplayHelperLib::GetProjectVersionAsInt()
{
	#if WITH_EDITOR
	return INT32_MAX;
	#endif
	
	FString ProjectVersionStr = UBpGameplayHelperLib::GetProjectVersion();
	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	const int32 nvn = FCString::Atoi(*ProjectVersionStr);
	return nvn;
}

FString UBpGameplayHelperLib::GetProjectName()
{
	if(UGameFeatureLibrary::IsGameDemo())
		return "Ready or Not Demo";
	if(!IsShippingBuild())
		return "Ready or Not (Dev Build)";
	return "Ready or Not";
}

bool UBpGameplayHelperLib::IsShippingBuild()
{
#if UE_BUILD_SHIPPING
	return true;
#endif
	return false;
}

bool UBpGameplayHelperLib::SetLastConnectedServerIP(FString IP)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->LastConnectedServerIP = IP;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetLastConnectedServerIP(FString& IP)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		IP = us->LastConnectedServerIP;
		return true;
	}
	return false;
}

void UBpGameplayHelperLib::ToggleGrenadeDrawDebug()
{
	for (TActorIterator<ABaseGrenade>It(GetWorldStatic()); It; ++It)
	{
		ABaseGrenade* b = *It;
		if (b->DrawDebugType == EDrawDebugTrace::None)
		{
			b->DrawDebugType = EDrawDebugTrace::Persistent;
		} else
		{
			b->DrawDebugType = EDrawDebugTrace::None;
		}
	}
}

void UBpGameplayHelperLib::ToggleFriendlyNameplates()
{
	GetRoNData()->bDrawNoNameplates = !GetRoNData()->bDrawNoNameplates;
}

FString UBpGameplayHelperLib::GetAdditionalBugReportInformation(APlayerController* PC)
{
	FString DebugString = "\r\n\r\n\r\nAddition Information Provided By Game;\r\n";
	if (PC)
	{
		DebugString += "\r\n Build: " + UBpGameplayHelperLib::GetProjectVersion();
		if (PC->GetWorld())
		{
			DebugString += "\r\n World: " + PC->GetWorld()->GetMapName();
			DebugString += "\r\n Resolution: " + GEngine->GameViewport->Viewport->GetSizeXY().ToString();

			if (PC->GetWorld()->GetGameState())
			{
				DebugString += "\r\n Game State: " + PC->GetWorld()->GetGameState()->GetFullName();
			}
		}

		DebugString += "\r\nPlayer Controller: " + PC->GetFullName();
		if (PC->GetPawn())
		{
			DebugString += "\r\nPlayer Pawn: " + PC->GetPawn()->GetFullName();
			DebugString += "\r\nPawn Location: " + PC->GetPawn()->GetActorLocation().ToString();
			DebugString += "\r\nPawn Rotation: " + PC->GetPawn()->GetActorRotation().ToString();
		}
		DebugString += "\r\nPlayer Network Address: " + PC->GetPlayerNetworkAddress();
		DebugString += "\r\nServer Network Address: " + PC->GetServerNetworkAddress();
		if (PC->PlayerState)
		{
			DebugString += "\r\nPlayer Name: " + PC->PlayerState->GetPlayerName();
			DebugString += ("\r\nPlayer ID: " + (PC->PlayerState->GetUniqueId().IsValid() ? PC->PlayerState->GetUniqueId()->ToString() : "Unknown"));
		}

		return DebugString;
	}
	return "Invalid Player Controller Provided.. Unable to retrieve data.";
}

bool UBpGameplayHelperLib::SaveSettings()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->SaveSettings();
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::ReloadSettings()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->LoadSettings(false);
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetShowFPS(bool& bShowFPS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowFPS = us->bShowFPS;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetShowFPS(bool bShowFPS)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowFPS = bShowFPS;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::GetShowControls(bool& bShowControls)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bShowControls = us->bShowControlsOnScreen;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetShowControls(bool bShowControls)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bShowControlsOnScreen = bShowControls;
		return true;
	}
	return false;
}

class UReadyOrNotGameInstance* UBpGameplayHelperLib::GetGameInstance(UWorld* WorldContext)
{
	if (WorldContext)
	{
		return Cast<UReadyOrNotGameInstance>(WorldContext->GetGameInstance());
	}
	else
	{
		if (GEngine->GameViewport)
		{
			UWorld* world = GEngine->GameViewport->GetWorld();
			if (world)
			{
				return Cast<UReadyOrNotGameInstance>(world->GetGameInstance());
			}
		}
	}
	return nullptr;
}

class AMapStatisticsSystem* UBpGameplayHelperLib::GetMapStatistics(UWorld* WorldContext)
{
#if defined(ENABLE_MAP_STATISTICS)
	auto World = WorldContext;
	if(!World)
	{
		if(GEngine && GEngine->GameViewport)
		{
			World = GEngine->GameViewport->GetWorld();
		}
	}

	if(!World)
	{
		return nullptr;
	}

	AMapStatisticsSystem* MapStatisticsSystem = nullptr;
	for (TActorIterator<AMapStatisticsSystem>It(World); It; ++It)
	{
		if(*It)
		{
			MapStatisticsSystem = *It;
			break;
		}
	}

	if(!MapStatisticsSystem)
	{
		MapStatisticsSystem = World->SpawnActor<AMapStatisticsSystem>();
	}

	return MapStatisticsSystem;
#else
	return nullptr;
#endif
}

bool UBpGameplayHelperLib::GetSendMapStatistics(bool& bSendMapStatistics)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		bSendMapStatistics = us->bSendMapStatistics;
		return true;
	}
	return false;
}

bool UBpGameplayHelperLib::SetSendMapStatistics(bool bSendMapStatistics)
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->bSendMapStatistics = bSendMapStatistics;
		return true;
	}
	return false;
}

FString UBpGameplayHelperLib::ConvertDegreeIntoLetter(float Degrees)
{
	if (Degrees > -5.0f && Degrees < 5.0f)
		return "N";

	if (Degrees > 40.0f && Degrees < 50.0f)
		return "NE";

	if (Degrees > 85.0f && Degrees < 95.0f)
		return "E";

	if (Degrees > 130.0f && Degrees < 140.0f)
		return "SE";

	if (Degrees > 175.0f && Degrees < 185.0f)
		return "S";

	if (Degrees > 220.0f && Degrees < 230.0f)
		return "SE";

	if (Degrees > 265.0f && Degrees < 275.0f)
		return "W";

	if (Degrees > 310 && Degrees < 320.0f)
		return "NW";

	return (FString::FromInt((int)Degrees));
}

FWidgetLookupData UBpGameplayHelperLib::GetWidgetDataFromLookupData(FString WidgetName, bool bWarnIfMissing)
{
	const UDataSingleton* RoNData = GetRoNData();
	if(!RoNData)
		return FWidgetLookupData();
	
	const UDataTable* dt = RoNData->WidgetDataLookupTable;
	if (dt)
	{
		FWidgetLookupData* LookupRow = dt->FindRow<FWidgetLookupData>(*WidgetName, "Widget Lookup: " + WidgetName, bWarnIfMissing);
		if (LookupRow)
		{
			return *LookupRow;
		}
	}
	return FWidgetLookupData();
}

UWidgetsData* UBpGameplayHelperLib::GetWidgetData()
{
	
	const UDataSingleton* RoNData = GetRoNData();
	if(!RoNData)
		return nullptr;
	return GetRoNData()->WidgetData;
}

UHumanCharacterHUD_V2* UBpGameplayHelperLib::GetHUDWidget()
{
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorldStatic(), 0)))
	{
		return PlayerCharacter->HumanCharacterWidget_V2;
	}

	//for (TObjectIterator<UHumanCharacterHUD_V2> Itr; Itr; ++Itr)
	//{
	//	return *Itr;
	//}

	return nullptr;
}

bool UBpGameplayHelperLib::HasWidgetInViewport(FString WidgetName)
{
	AReadyOrNotPlayerController* PlayerController = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	if (!PlayerController)
		return false;

	if (WidgetName == "CharacterHUD_V2")
	{
		for (TObjectIterator<UHumanCharacterHUD_V2>It; It; ++It)
		{
			if (It->IsInViewport())
				return true;
		}
	}

	// fast path if possible!
	if (PlayerController->CreatedWidgetMap.Find(WidgetName))
	{
		UUserWidget* Widget = PlayerController->CreatedWidgetMap[WidgetName];
		return Widget && Widget->IsInViewport();
	}
	
	return false;
}

void UBpGameplayHelperLib::RemoveWidgetFromViewport(FString WidgetName)
{
	const FWidgetLookupData WidgetData = GetWidgetDataFromLookupData(WidgetName);
	if (WidgetData.WidgetClass)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorldStatic(), FoundWidgets, WidgetData.WidgetClass, false);
		for (UUserWidget* Widget : FoundWidgets)
		{
			Widget->RemoveFromParent();
		}
	}
}

UDataTable* UBpGameplayHelperLib::GetAnimationDataTable()
{
	if (!GetRoNData())
		return nullptr;

	return GetRoNData()->AnimationDataLookupTable;
}

bool UBpGameplayHelperLib::IsVoiceOverSuspended(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return false;

	AReadyOrNotGameState* GameState = World->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return false;

	return GameState->bGlobalSuspendVoiceOver;
}

void UBpGameplayHelperLib::EnableInteractionFor(AActor* InInteractableActor, APlayerCharacter* InPlayerCharacter)
{
	if (!InPlayerCharacter)
		return;

	EnableInteractionForController(InInteractableActor, InPlayerCharacter->GetController<APlayerController>());
}

void UBpGameplayHelperLib::DisableInteractionFor(AActor* InInteractableActor, APlayerCharacter* InPlayerCharacter)
{
	if (!InPlayerCharacter)
		return;

	DisableInteractionForController(InInteractableActor, InPlayerCharacter->GetController<APlayerController>());
}

void UBpGameplayHelperLib::EnableInteractionForController(AActor* InInteractableActor, APlayerController* InPlayerController)
{
	if (!InInteractableActor || !InPlayerController)
		return;

	TInlineComponentArray<UInteractableComponent*> InteractableComponents(InInteractableActor, true);
	InInteractableActor->GetComponents(InteractableComponents, true);

	for (UInteractableComponent* InteractableComponent : InteractableComponents)
	{
		InteractableComponent->EnableInteractionFor(InPlayerController);
	}
}

void UBpGameplayHelperLib::DisableInteractionForController(AActor* InInteractableActor, APlayerController* InPlayerController)
{
	if (!InInteractableActor || !InPlayerController)
		return;

	TInlineComponentArray<UInteractableComponent*> InteractableComponents(InInteractableActor, true);
	InInteractableActor->GetComponents(InteractableComponents, true);

	for (UInteractableComponent* InteractableComponent : InteractableComponents)
	{
		InteractableComponent->DisableInteractionFor(InPlayerController);
	}
}

void UBpGameplayHelperLib::EnableInteractionCompForController(UInteractableComponent* InteractableComponent,
	APlayerController* InPlayerController)
{
	if (!InteractableComponent || !InPlayerController)
		return;

	InteractableComponent->EnableInteractionFor(InPlayerController);
}

void UBpGameplayHelperLib::DisableInteractionCompForController(UInteractableComponent* InteractableComponent,
	APlayerController* InPlayerController)
{
	if (!InteractableComponent || !InPlayerController)
		return;

	InteractableComponent->DisableInteractionFor(InPlayerController);
}

TArray<UUserWidget*> UBpGameplayHelperLib::GetWidgetsFromViewport(FString WidgetName)
{
	TArray<UUserWidget*> ReturnWidgets;

	const FWidgetLookupData WidgetData = GetWidgetDataFromLookupData(WidgetName);
	if (WidgetData.WidgetClass)
	{
		for (TObjectIterator<UUserWidget> Itr; Itr; ++Itr)
		{
			UUserWidget* LiveWidget = *Itr;

			/* If the Widget has no World, Ignore it (It's probably in the Content Browser!) */
			if (!LiveWidget->GetWorld())
			{
				continue;
			}

			if (LiveWidget->IsA(WidgetData.WidgetClass.Get()))
			{
				ReturnWidgets.AddUnique(LiveWidget);
			}
		}
	}
	return ReturnWidgets;
}

bool UBpGameplayHelperLib::IsWidgetOfClassInViewport(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass)
{ 
	if (!WidgetClass)
		return false;
	
	if (!WorldContextObject)
		return false;

	const UWorld* World = GetWorldStatic();
	if (!World)
		return false;
	  
	for (TObjectIterator<UUserWidget> Itr; Itr; ++Itr)
	{
		if (Itr->GetWorld() != World)
			continue;
		
		if (!Itr->IsA(WidgetClass))
			continue;
		    
		if (Itr->GetIsVisible())
		{
			return true;
		}
	}
	
	return false;
}

UUserWidget* UBpGameplayHelperLib::GetFirstWidgetFromViewport(FString WidgetName)
{
	const FWidgetLookupData WidgetData = GetWidgetDataFromLookupData(WidgetName);
	if (WidgetData.WidgetClass)
	{
	    for (TObjectIterator<UUserWidget> Itr; Itr; ++Itr)
	    {
    		UUserWidget* LiveWidget = *Itr;

    		/* If the Widget has no World, Ignore it (It's probably in the Content Browser!) */
    		if (!LiveWidget->GetWorld())
    		{
    			continue;
    		}

	    	if (!LiveWidget->IsInViewport())
	    		continue;

	    	if (LiveWidget->IsA(WidgetData.WidgetClass.Get()))
    		{
    			return LiveWidget;
    		}
	    }
	}
	return nullptr;
}

UItemData* UBpGameplayHelperLib::GetItemData(UWorld* WorldContext)
{
	// Attempt 1: get it from the levelscript (levels can modify the itemdata, for e.g. levels that take place in the past)
	if (GetGameInstance(WorldContext) && GetGameInstance(WorldContext)->GetWorld())
	{
		AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetGameInstance(WorldContext)->GetWorld()->GetLevelScriptActor());
		if (LevelScript && LevelScript->ItemData)
		{
			return LevelScript->ItemData;
		}
	}

	// Attempt 2: get it from the singleton
	UDataSingleton* DataSingleton = GetRoNData();
	if (DataSingleton)
	{
		return DataSingleton->ItemData;
	}

	return nullptr;
}

UDataTable* UBpGameplayHelperLib::GetPairedInteractionDataTable()
{
	UDataSingleton* RoNData = GetRoNData();
	if (RoNData)
	{
		return RoNData->PairedInteractionDataTable;
	}

	return nullptr;
}

UDataTable* UBpGameplayHelperLib::GetMoveStyleDataTable()
{
	UDataSingleton* RoNData = GetRoNData();
	if (RoNData)
	{
		return RoNData->MoveStyleDataTable;
	}

	return nullptr;
}

FLevelDataLookupTable UBpGameplayHelperLib::GetLevelData(UWorld* WorldContext)
{
	if (WorldContext)
	{
		AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(WorldContext->GetLevelScriptActor());
		if (ls)
		{
			return ls->LevelData;
		}
	}

	if (!UBpGameplayHelperLib::GetGameInstance(WorldContext))
		return FLevelDataLookupTable();

	if (!UBpGameplayHelperLib::GetGameInstance(WorldContext)->GetWorld())
		return FLevelDataLookupTable();

	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(UBpGameplayHelperLib::GetGameInstance(WorldContext)->GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		return ls->LevelData;
	}
	return  FLevelDataLookupTable();
}

UMusicData* UBpGameplayHelperLib::GetMusicData(UWorld* WorldContext)
{
	if (!UBpGameplayHelperLib::GetGameInstance(WorldContext))
		return nullptr;

	if (!UBpGameplayHelperLib::GetGameInstance(WorldContext)->GetWorld())
		return nullptr;

	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(UBpGameplayHelperLib::GetGameInstance(WorldContext)->GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		return ls->MusicData;
	}
	return nullptr;
}

void UBpGameplayHelperLib::AttachMagazinesToWeapon(TSubclassOf<class ABaseMagazineWeapon> WeaponClass, class ABaseMagazineWeapon* Weapon)
{
	if (!Weapon)
		return;

	if (!WeaponClass)
		return;

	ABaseMagazineWeapon* defaultData = WeaponClass->GetDefaultObject<ABaseMagazineWeapon>();
	if (defaultData)
	{

		Weapon->GetItemMesh()->SetAnimInstanceClass(defaultData->GetItemMesh()->AnimClass);

		Weapon->GetItemMesh()->SetAnimInstanceClass(defaultData->GetItemMesh()->AnimClass);

		Weapon->Mag_01_Socket = defaultData->Mag_01_Socket;
		Weapon->Mag_01_Bullets_Socket = defaultData->Mag_01_Bullets_Socket;
		Weapon->Mag_01_Extra_Socket = defaultData->Mag_01_Extra_Socket;


		Weapon->Mag_01_Static = defaultData->Mag_01_Static;
		Weapon->Mag_01_FMJ_Bullets_Static = defaultData->Mag_01_FMJ_Bullets_Static;
		Weapon->Mag_01_HP_Bullets_Static = defaultData->Mag_01_HP_Bullets_Static;
		Weapon->Mag_01_Extra_Static = defaultData->Mag_01_Extra_Static;

		Weapon->Mag_02_Socket = defaultData->Mag_02_Socket;
		Weapon->Mag_02_Bullets_Socket = defaultData->Mag_02_Bullets_Socket;
		Weapon->Mag_02_Extra_Socket = defaultData->Mag_02_Extra_Socket;


		Weapon->Mag_02_Static = defaultData->Mag_02_Static;
		Weapon->Mag_02_FMJ_Bullets_Static = defaultData->Mag_02_FMJ_Bullets_Static;
		Weapon->Mag_02_HP_Bullets_Static = defaultData->Mag_02_HP_Bullets_Static;
		Weapon->Mag_02_Extra_Static = defaultData->Mag_02_Extra_Static;
		Weapon->AttachStatic();

	}
}

int UBpGameplayHelperLib::GetAttachmentPointsRemaining(FSavedLoadout Loadout)
{
	UItemData* ItemData = UBpGameplayHelperLib::GetItemData();
	ensure(ItemData);
	if (!ItemData)
		return 0;
	int AttachmentPointsRemaining = ItemData->AttachmentPointsBase;

	if (Cast<ABaseMagazineWeapon>(Loadout.Primary.GetDefaultObject())) 
		AttachmentPointsRemaining += Cast<ABaseMagazineWeapon>(Loadout.Primary.GetDefaultObject())->AttachmentPoints;
	if (Cast<ABaseMagazineWeapon>(Loadout.Secondary.GetDefaultObject())) 
		AttachmentPointsRemaining += Cast<ABaseMagazineWeapon>(Loadout.Secondary.GetDefaultObject())->AttachmentPoints;
	if(Loadout.PrimaryScope.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.PrimaryScope.GetDefaultObject()->PointCost;
	if(Loadout.PrimaryMuzzle.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.PrimaryMuzzle.GetDefaultObject()->PointCost;
	if(Loadout.PrimaryUnderbarrel.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.PrimaryUnderbarrel.GetDefaultObject()->PointCost;
	if(Loadout.PrimaryOverbarrel.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.PrimaryOverbarrel.GetDefaultObject()->PointCost;
	if(Loadout.SecondaryScope.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.SecondaryScope.GetDefaultObject()->PointCost;
	if(Loadout.SecondaryMuzzle.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.SecondaryMuzzle.GetDefaultObject()->PointCost;
	if(Loadout.SecondaryUnderbarrel.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.SecondaryUnderbarrel.GetDefaultObject()->PointCost;
	if(Loadout.SecondaryOverbarrel.GetDefaultObject())
		AttachmentPointsRemaining -= Loadout.SecondaryOverbarrel.GetDefaultObject()->PointCost;

	return AttachmentPointsRemaining;
}

APlayerCharacter* UBpGameplayHelperLib::GetLocalPlayerCharacter(UWorld* World)
{
	if (!World)
	{
		if (GEngine && GEngine->GameViewport)
		{
			World = GEngine->GameViewport->GetWorld();
		}
	}

	if (World)
	{
		APlayerController* pc = Cast<APlayerController>(UGameplayStatics::GetPlayerController(World, 0));
		if (pc)
		{
			return Cast<APlayerCharacter>(pc->GetPawn());
		}
	}

	return nullptr;
}

AReadyOrNotPlayerController* UBpGameplayHelperLib::GetLocalPlayerController(UWorld* World)
{
	if (UKismetSystemLibrary::IsDedicatedServer(World))
		return nullptr;
	
	if (!World)
	{
		if (GEngine && GEngine->GameViewport)
		{
			World = GEngine->GameViewport->GetWorld();
		}
	}
	
	if (World)
	{
		return Cast<AReadyOrNotPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
	}
	
	return nullptr;
}

UWorld* UBpGameplayHelperLib::GetWorldBP(APlayerController* pc)
{
	if (pc)
	{
		return pc->GetWorld();
	}
	return nullptr;
}

bool UBpGameplayHelperLib::IsObjectiveTarget(AReadyOrNotCharacter* Target, AReadyOrNotCharacter* LocalPlayer)
{
	if (!Target || !LocalPlayer)
		return false;
	
	if (!LocalPlayer->GetNeutralizeSuspectTag())
		return false;

	const FName SuspectTag = LocalPlayer->GetNeutralizeSuspectTag()->GetSuspectTag();

	if (SuspectTag.ToString().IsEmpty())
		return false;
	
	return Target->Tags.Contains(SuspectTag);
}

class ULookupData* UBpGameplayHelperLib::GetLookupData()
{
	return GetRoNData()->LookupData;
}

void UBpGameplayHelperLib::LoadCustomizationLevels(UWorld* WorldContext)
{
	if (!WorldContext)
	{
		if (!GEngine)
			return;

		if (GEngine->GameViewport)
		{
			WorldContext = GEngine->GameViewport->GetWorld();
		}
	}
	if (WorldContext)
	{
		const FLatentActionInfo MenuStreamIn;
		UGameplayStatics::LoadStreamLevel(WorldContext, GetRoNData()->CustomizationMenuLevel, true, false, MenuStreamIn);
		ULevelStreaming* TabletLevel = UGameplayStatics::GetStreamingLevel(WorldContext, GetRoNData()->CustomizationMenuLevel);

		if (TabletLevel)
		{
			FTransform Transform;

			Transform.SetTranslation(FVector(0, 0, -10000.0f));

			TabletLevel->LevelTransform = Transform;
			TabletLevel->SetShouldBeVisible(true);
		}

		//UGameplayStatics::LoadStreamLevel(WorldContext, GetRoNData()->CustomizationCharacterLevel, true, false, CharacterStreamIn);
		// DOES NOT WORK BECAUSE SERVER
		// 		bool bSucessfulLoad;
// 		bool bHasMenuWorld = false;
// 		bool bHasCharacterWorld = false;
// 		for (int32 i = 0; i < WorldContext->GetStreamingLevels().Num(); i++)
// 		{
// 			ULevelStreaming* level = WorldContext->GetStreamingLevels()[i];
// 			if (level)
// 			{
// 				if (level->GetWorldAssetPackageName().Contains(GetRoNData()->CustomizationMenuLevel.ToString()))
// 				{
// 					bHasMenuWorld = true;
// 				}
// 				if (level->GetWorldAssetPackageName().Contains(GetRoNData()->CustomizationCharacterLevel.ToString()))
// 				{
// 					bHasCharacterWorld = true;
// 				}
// 			}
// 		}
// 
// 		if (!bHasMenuWorld)
// 		{
// 			ULevelStreamingKismet::LoadLevelInstance(WorldContext, GetRoNData()->CustomizationMenuLevel.ToString(), FVector::ZeroVector, FRotator::ZeroRotator, bSucessfulLoad);
// 		}
// 
// 		if (!bHasCharacterWorld)
// 		{
// 			ULevelStreamingKismet::LoadLevelInstance(WorldContext, GetRoNData()->CustomizationCharacterLevel.ToString(), FVector::ZeroVector, FRotator::ZeroRotator, bSucessfulLoad);
// 		}
// 

// 		
// 
// 		// unload all other levels right now
// 		// so they don't affect lighting etc on the customization
// 		for (int32 i = 0; i < WorldContext->GetStreamingLevels().Num(); i++)
// 		{
// 			ULevelStreaming* level = WorldContext->GetStreamingLevels()[i];
// 			if (level)
// 			{
// 				if (level->GetWorldAssetPackageName().Contains("lighting")
// 					|| level->GetWorldAssetPackageName().Contains("post"))
// 				{
// 					level->SetShouldBeVisible(false);
// 				}
// 			}
// 		}
	}
}

void UBpGameplayHelperLib::UnloadCustomizationLevels(UWorld* WorldContext)
{
	if (!WorldContext)
	{
		if (!GEngine)
			return;

		if (GEngine->GameViewport)
		{
			WorldContext = GEngine->GameViewport->GetWorld();
		}
	}

	if (WorldContext)
	{
		const FLatentActionInfo MenuStreamOut;
		const FLatentActionInfo CharacterStreamOut;
		UGameplayStatics::UnloadStreamLevel(WorldContext, GetRoNData()->CustomizationMenuLevel, MenuStreamOut, false);
		UGameplayStatics::UnloadStreamLevel(WorldContext, GetRoNData()->CustomizationCharacterLevel, CharacterStreamOut, false);
		// DOES NOT WORK BECAUSE SERVER
// 		Handles if the level has been already added  to the level list

// 		
// 		
// 				// try remove any dynamically created levels
// 				for (int32 i = 0; i < WorldContext->GetStreamingLevels().Num(); i++)
// 				{
// 					ULevelStreaming* level = WorldContext->GetStreamingLevels()[i];
// 		
// 					if (Cast<ULevelStreamingDynamic>(level))
// 					{
// 						for (int32 y = 0; y < 10; y++)
// 						{
// 							FLatentActionInfo info;
// 							info.UUID = FMath::RandRange(y, MAX_int32);
// 							FString PackageName = FPackageName::GetShortName(level->PackageNameToLoad.ToString());
// 							PackageName += "_LevelInstance_";
// 							PackageName += FString::FromInt(y);
// 							UGameplayStatics::UnloadStreamLevel(WorldContext, FName(*PackageName), info, false);
// 						}
// 					}
// 					else if (level->GetWorldAssetPackageName().Contains("lighting")
// 						|| level->GetWorldAssetPackageName().Contains("post"))
// 					{
// 						level->SetShouldBeVisible(true);
// 					}
// 				}
// 				GEngine->BlockTillLevelStreamingCompleted(WorldContext);

	}
}

UPenetrationData* UBpGameplayHelperLib::GetPenetrationData()
{
	return UBpGameplayHelperLib::GetRoNData()->PenetrationData;
}

UDataTable* UBpGameplayHelperLib::GetAnimatedIconLookupDataTable()
{
	if (!GetRoNData())
		return nullptr;

	return GetRoNData()->AnimatedIconLookupTable;
}

// NOTE(killo): the item data table has been deprecated, please use fields in weapon blueprint directly
// there are also the ULoadoutManager:: statics that may be helpful
// class UDataTable* UBpGameplayHelperLib::GetItemLookupDataTable()
// {
// 	if (!UBpGameplayHelperLib::GetRoNData())
// 		return nullptr;
//
// 	return UBpGameplayHelperLib::GetRoNData()->ItemDataLookupTable;
// }

class UDataTable* UBpGameplayHelperLib::GetAmmoLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->AmmoDataLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetInputKeyGamepadIconLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->InputKeyGamePadIconTable;
}

UDataTable* UBpGameplayHelperLib::GetLevelLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;
	return UBpGameplayHelperLib::GetRoNData()->LevelDataLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetAILookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->AIDataLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetSpeechLookupDataTable(FString Speaker)
{
	if (!GetRoNData())
		return nullptr;

	if (UDataTable** FoundTable = GetRoNData()->SpeechDataLookupTable.Find(Speaker))
	{
		return *FoundTable;
	}
	
	return nullptr;
}

UDataTable* UBpGameplayHelperLib::GetDoorLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->DoorDataLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetTrapLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->TrapDataLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetConversationLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->ConversationLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetCharacterLookOverrideDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->CharacterLookOverrideTable;
}

UDataTable* UBpGameplayHelperLib::GetGameModeSettingsLookupDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->GameSettingsLookupTable;
}

UDataTable* UBpGameplayHelperLib::GetSuspectArmourDataTable()
{
	if (!UBpGameplayHelperLib::GetRoNData())
		return nullptr;

	return UBpGameplayHelperLib::GetRoNData()->SuspectArmourDataTable;
}

FAnimatedIcon UBpGameplayHelperLib::GetAnimatedIconFromTable(const FName RowName, bool& bSuccess)
{
	FString RowNameNoSpaces = RowName.ToString();
	RowNameNoSpaces.RemoveSpacesInline();
	
	if (UDataTable* DT = GetAnimatedIconLookupDataTable())
	{
		FAnimatedIconTable* Row = DT->FindRow<FAnimatedIconTable>(*RowNameNoSpaces, "Animated Icon Lookup", !RowName.IsNone());
		if (Row)
		{
			bSuccess = true;
			return Row->AnimatedIcon; 
		}
	}
	
	bSuccess = false;
	return FAnimatedIcon();
}

bool UBpGameplayHelperLib::IsPvPSupported(FLevelDataLookupTable LookupTable)
{
	for (int32 i = 0; i < LookupTable.SupportedGameModes.Num(); i++)
	{
		AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(LookupTable.SupportedGameModes[i]->GetDefaultObject());
		if (gm)
		{
			if (!gm->IsA(ACoopGM::StaticClass()))
			{
				return true;
			}
		}
	}
	return false;
}

bool UBpGameplayHelperLib::IsCOOPSupported(FLevelDataLookupTable LookupTable)
{
	for (int32 i = 0; i < LookupTable.SupportedGameModes.Num(); i++)
	{
		AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(LookupTable.SupportedGameModes[i]->GetDefaultObject());
		if (gm)
		{
			if (gm->IsA(ACoopGM::StaticClass()))
			{
				return true;
			}
		}
	}
	return false;
}

AReadyOrNotPlayerState* UBpGameplayHelperLib::GetLocalPlayerState(UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr; // probably a dedicated server
	}

	AReadyOrNotPlayerController* Controller = GetLocalPlayerController(World);
	if (Controller == nullptr)
	{
		return nullptr; // probably a dedicated server?
	}

	return Controller->GetPlayerState<AReadyOrNotPlayerState>();
}

void UBpGameplayHelperLib::PlayInterfaceSound(UWorld* WorldContext, EInterfaceSoundType SoundClass)
{
	UWidgetsData* WidgetData = GetRoNData()->WidgetData;
	const int32 Number = (int32)SoundClass;

	if (WidgetData == nullptr || WidgetData->UISoundClasses.Num() <= Number)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(WorldContext, WidgetData->UISoundClasses[Number]);
}



ULicenseSave* UBpGameplayHelperLib::LoadLicenseSave()
{
	FString ProjectVersionStr = GetProjectVersion();
	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	UGameplayStatics::CreateSaveGameObject(ULicenseSave::StaticClass());
	ULicenseSave* LoadGameInstance = Cast<ULicenseSave>(UGameplayStatics::LoadGameFromSlot(TEXT("LicenseSave" + ProjectVersionStr), 0));
	if (!LoadGameInstance)
	{
		LoadGameInstance = Cast<ULicenseSave>(UGameplayStatics::CreateSaveGameObject(ULicenseSave::StaticClass()));
		if (LoadGameInstance)
		{
			UGameplayStatics::SaveGameToSlot(LoadGameInstance, TEXT("LicenseSave"), 0);
		}
	}

	return LoadGameInstance;
}

void UBpGameplayHelperLib::SaveLicenseSave(ULicenseSave* License)
{
	FString ProjectVersionStr = GetProjectVersion();
	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	UGameplayStatics::SaveGameToSlot(License, TEXT("LicenseSave" + ProjectVersionStr), 0);
}

FPointDamageEvent UBpGameplayHelperLib::CastToPointDamageEvent(FDamageEvent DamageEvent, EStructureCastPathway& Branches)
{
	if(!DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		Branches = EStructureCastPathway::CastFailed;
		return FPointDamageEvent();
	}
	Branches = EStructureCastPathway::CastSuccess;
	return *((FPointDamageEvent*)&DamageEvent);
}

FRadialDamageEvent UBpGameplayHelperLib::CastToRadialDamageEvent(FDamageEvent DamageEvent, EStructureCastPathway& Branches)
{
	if(DamageEvent.GetTypeID() != FRadialDamageEvent::ClassID)
	{
		Branches = EStructureCastPathway::CastFailed;
		return FRadialDamageEvent();
	}
	Branches = EStructureCastPathway::CastFailed;
	return *((FRadialDamageEvent*)&DamageEvent);
}

class APlayerCharacter* UBpGameplayHelperLib::FindClosestDeadGuyInRadius(FVector Origin, AActor* Causer, float Radius, bool bIncludeUnconscious)
{
	UWorld* World = Causer->GetWorld();
	float ClosestCharacterDist = Radius * 2;
	APlayerCharacter* ClosestCharacter = nullptr;

	const FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(ApplyRadialDamage), false, Causer);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects),
		FCollisionShape::MakeSphere(Radius), SphereParams);

	// query all overlaps
	for (int32 i = 0; i < Overlaps.Num(); i++)
	{
		FOverlapResult const& Overlap = Overlaps[i];
		AActor* const OverlapActor = Overlap.GetActor();
		APlayerCharacter* const Character = Cast<APlayerCharacter>(OverlapActor);

		if (!Character)
		{
			continue;
		}

		if (!Character->IsDeadOrUnconscious() || (bIncludeUnconscious && Character->IsUnconsciousNotDead()))
		{
			if (FVector::Dist(Character->GetActorLocation(), Origin) < ClosestCharacterDist)
			{
				ClosestCharacterDist = FVector::Dist(Character->GetActorLocation(), Origin);
				ClosestCharacter = Character;
			}
		}
	}

	return ClosestCharacter;
}

FString UBpGameplayHelperLib::GetDMOAddress()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		return us->DMOAddress;
	}
	return "";
}

FString UBpGameplayHelperLib::GetDMOGameMode()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		return us->DMOGameMode;
	}
	return "";
}

ETeamType UBpGameplayHelperLib::GetDMOTeamType()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		return us->DMOTeamType;
	}
	return ETeamType::TT_NONE;
}

FString UBpGameplayHelperLib::GetBuildDate()
{
	return __DATE__;
}

FString UBpGameplayHelperLib::GetBuildTime()
{
	return __TIME__;
}

EWeaponType UBpGameplayHelperLib::ConvertItemTypeToWeaponType(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::IT_None:
			return EWeaponType::WT_None;
		case EItemType::IT_Rifles:
			return EWeaponType::WT_Rifles;
		case EItemType::IT_SubmachineGun:
			return EWeaponType::WT_SubmachineGun;
		case EItemType::IT_Shotgun:
			return EWeaponType::WT_Shotgun;
		case EItemType::IT_PistolsLethal:
			return EWeaponType::WT_PistolsLethal;
		case EItemType::IT_PistolsNonLethal:
			return EWeaponType::WT_PistolsNonLethal;
		case EItemType::IT_PrimaryNonLethal:
			return EWeaponType::WT_PrimaryNonLethal;
		default:
			break;
	}
	
	return EWeaponType::WT_None;
}

EItemType UBpGameplayHelperLib::ConvertWeaponTypeToItemType(EWeaponType WeaponType)
{
	switch (WeaponType)
	{
		case EWeaponType::WT_None:
			break;
		case EWeaponType::WT_Rifles:
			return EItemType::IT_Rifles;
		case EWeaponType::WT_SubmachineGun:
			return EItemType::IT_SubmachineGun;
		case EWeaponType::WT_Shotgun:
			return EItemType::IT_Shotgun;
		case EWeaponType::WT_PistolsLethal:
			return EItemType::IT_PistolsLethal;
		case EWeaponType::WT_PistolsNonLethal:
			return EItemType::IT_PistolsNonLethal;
		case EWeaponType::WT_PrimaryNonLethal:
			return EItemType::IT_PrimaryNonLethal;
		case EWeaponType::WT_Special:
			break;
		case EWeaponType::WT_Unarmed:
			break;
		default:
			break;
	}
	
	return EItemType::IT_None;
}

bool UBpGameplayHelperLib::IsMultiplayer(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
		return false;

	if (World->GetNetMode() == NM_Client)
		return true;

	if (World->GetNetDriver())
		return true;
	
	int32 PlayerCount = 0;
	for (TActorIterator<AReadyOrNotPlayerController> It(World); It; ++It)
	{
		AReadyOrNotPlayerController* PlayerController = *It;
		if (!PlayerController->bIsReplaySpectator && !Cast<AReplayController>(PlayerController))
		{
			PlayerCount++;
		}
	}
	return PlayerCount > 1;
}

bool UBpGameplayHelperLib::IsLobby(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
		return false;

	return IsValid(World->GetAuthGameMode<ALobbyGM>());
}

bool UBpGameplayHelperLib::IsCommanderMode(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
		return false;

	AGameModeBase* GameMode = World->GetAuthGameMode();
	
	if (ALobbyGM* LobbyGM = Cast<ALobbyGM>(GameMode))
		return IsValid(LobbyGM->CommanderProfile);
	
	if (ACommanderGM* CommanderGM = Cast<ACommanderGM>(GameMode))
		return true;
	
	return false;
}

bool UBpGameplayHelperLib::IsIronmanMode(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
		return false;

	UBaseProfile* BaseProfile = UBaseProfile::GetCurrentProfile();
	if (!BaseProfile)
		return false;

	UCommanderProfile* CommanderProfile = Cast<UCommanderProfile>(BaseProfile);
	if (!CommanderProfile)
		return false;

	return CommanderProfile->bIronmanMode;
}

bool UBpGameplayHelperLib::IsConsole(const UObject* WorldContextObject) {
#if defined(TARGET_PS4) || defined(TARGET_PS5) || defined(TARGET_XSX) || defined(TARGET_XB1) || defined(TARGET_SWITCH)
	return true;
#else
	return false;
#endif
}

void UBpGameplayHelperLib::UpdateInteractableComponentsWorldSpaceActionPrompts(const UObject* WorldContextObject, bool bEnableWorldSpaceActionPrompts)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
		return;

	AReadyOrNotGameState* GS = World->GetGameState<AReadyOrNotGameState>();
	if (!IsValid(GS))
		return;

	for (UInteractableComponent* InteractableComponent : GS->AllInteractableComponents)
	{
		if (InteractableComponent && !InteractableComponent->bOverrideActionPromptUserSettings)
			InteractableComponent->bShowActionPromptInWorld = bEnableWorldSpaceActionPrompts;
	}

	// If we've just turned this on, set all UI action prompts to not in use
	if (bEnableWorldSpaceActionPrompts)
	{
		APlayerCharacter* PlayerCharacter = GetLocalPlayerCharacter(World);
		PlayerCharacter->HumanCharacterWidget_V2->ClearAllPlayerActionPrompts();
	}
}

UReadyOrNotSaveGame::UReadyOrNotSaveGame()
{
	SaveSlotName = "GameSettings";
	UserIndex = 0;
}

UReadyOrNotSaveGame* UReadyOrNotSaveGame::CreateDefaultSavegame(FString LoadSlotName)
{
	UReadyOrNotSaveGame *SaveGameInstance = Cast<UReadyOrNotSaveGame>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotSaveGame::StaticClass()));
	return SaveGameInstance;
}

UReadyOrNotSessionData::UReadyOrNotSessionData()
{
	SaveSlotName = "SessionData";
	UserIndex = 0;
}

UReadyOrNotSessionData* UReadyOrNotSessionData::CreateDefaultSavegame(FString LoadSlotName)
{
	UReadyOrNotSessionData* SaveGameInstance = Cast<UReadyOrNotSessionData>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotSessionData::StaticClass()));
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, LoadSlotName, SaveGameInstance->UserIndex);
	return SaveGameInstance;
}

UReadyOrNotModData::UReadyOrNotModData()
{
	SaveSlotName = "ModData";
	UserIndex = 0;
}

UReadyOrNotModData* UReadyOrNotModData::CreateDefaultSavegame(FString LoadSlotName)
{
	UReadyOrNotModData* SaveGameInstance = Cast<UReadyOrNotModData>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotModData::StaticClass()));
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, LoadSlotName, SaveGameInstance->UserIndex);
	return SaveGameInstance;
}

UReadyOrNotModData* UReadyOrNotModData::Get()
{
	UReadyOrNotModData* ModData = Cast<UReadyOrNotModData>(UGameplayStatics::CreateSaveGameObject(UReadyOrNotModData::StaticClass()));
	ModData = Cast<UReadyOrNotModData>(UGameplayStatics::LoadGameFromSlot(ModData->SaveSlotName, ModData->UserIndex));
	if (!ModData)
	{
		ModData = CreateDefaultSavegame("ModData");
	}
	return ModData;
}


