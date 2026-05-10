// Copyright Void Interactive, 2023

#include "BaseProfile.h"

#include "CommanderGM.h"
#include "CommanderProfile.h"
#include "MetaGameProfile.h"
#include "GameModes/LobbyGM.h"

constexpr int32 InitialBaseProfileVersion = 1;

UBaseProfile::UBaseProfile()
{
	BaseVersion = InitialBaseProfileVersion;
}

void UBaseProfile::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		// Example where we clear loadout
		if (BaseVersion < InitialBaseProfileVersion)
		{
			Loadouts.Empty();
			AttachmentSaveMap.Empty();
			LoadoutAttachmentSaveMap.Empty();
			SavedWeaponClassOfTypeMap.Empty();
			WeaponClassToDefaultFireModeMap.Empty();
			WeaponToWeaponPresetsMap.Empty();
			LoadoutPresetMap.Empty();
		}
		BaseVersion = InitialBaseProfileVersion;
	}
}

UBaseProfile* UBaseProfile::GetCurrentProfile()
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (!World)
		return nullptr;

	ACommanderGM* CommanderGM = World->GetAuthGameMode<ACommanderGM>();
	if (CommanderGM && CommanderGM->CommanderProfile)
		return CommanderGM->CommanderProfile;

	ALobbyGM* LobbyGM = World->GetAuthGameMode<ALobbyGM>();
	if (LobbyGM && LobbyGM->CommanderProfile)
		return LobbyGM->CommanderProfile;
	
	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (GameInstance && GameInstance->MetaGameProfile)
		return GameInstance->MetaGameProfile;

	return nullptr;
}
