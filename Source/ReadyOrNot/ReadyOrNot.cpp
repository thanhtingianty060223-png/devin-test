// Copyright Void Interactive, 2023

#include "ReadyOrNot.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, ReadyOrNot, "ReadyOrNot" );
 
DEFINE_LOG_CATEGORY(LogReadyOrNot);
DEFINE_LOG_CATEGORY(LogReadyOrNotLoadout);
DEFINE_LOG_CATEGORY(LogReadyOrNotInit);
DEFINE_LOG_CATEGORY(LogReadyOrNotAI);
DEFINE_LOG_CATEGORY(LogReadyOrNotAudio);
DEFINE_LOG_CATEGORY(LogReadyOrNotUI);
DEFINE_LOG_CATEGORY(LogReadyOrNotSteam);
DEFINE_LOG_CATEGORY(LogReadyOrNotModding);
DEFINE_LOG_CATEGORY(LogReadyOrNotCriticalErrors);
DEFINE_LOG_CATEGORY(LogReadyOrNotChat);
DEFINE_LOG_CATEGORY(LogReadyOrNotProgression);
DEFINE_LOG_CATEGORY(LogReadyOrNotMatchmaking);
DEFINE_LOG_CATEGORY(LogReadyOrNotSubtitles);
DEFINE_LOG_CATEGORY(LogReadyOrNotHitRegistration);

// When updating these, also update the crash reporter - as it will clear "mods" which don't match thos list
const TArray<FString> VALID_GAME_PAK_FILES = {
		XorString("ReadyOrNot-WindowsNoEditor.pak"),
		XorString("pakchunk0-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk1-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk2-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk3-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk4-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk5-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk6-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk7-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk8-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk9-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk10-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk11-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk12-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk13-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk14-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk15-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk16-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk17-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk18-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk19-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk20-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk21-WindowsNoEditor_0_P.pak"),
		XorString("pakchunk0-WindowsNoEditor.pak"),
		XorString("pakchunk1-WindowsNoEditor.pak"),
		XorString("pakchunk2-WindowsNoEditor.pak"),
		XorString("pakchunk3-WindowsNoEditor.pak"),
		XorString("pakchunk4-WindowsNoEditor.pak"),
		XorString("pakchunk5-WindowsNoEditor.pak"),
		XorString("pakchunk6-WindowsNoEditor.pak"),
		XorString("pakchunk7-WindowsNoEditor.pak"),
		XorString("pakchunk8-WindowsNoEditor.pak"),
		XorString("pakchunk9-WindowsNoEditor.pak"),
		XorString("pakchunk10-WindowsNoEditor.pak"),
		XorString("pakchunk11-WindowsNoEditor.pak"),
		XorString("pakchunk12-WindowsNoEditor.pak"),
		XorString("pakchunk13-WindowsNoEditor.pak"),
		XorString("pakchunk14-WindowsNoEditor.pak"),
		XorString("pakchunk15-WindowsNoEditor.pak"),
		XorString("pakchunk16-WindowsNoEditor.pak"),
		XorString("pakchunk17-WindowsNoEditor.pak"),
		XorString("pakchunk18-WindowsNoEditor.pak"),
		XorString("pakchunk19-WindowsNoEditor.pak"),
		XorString("pakchunk20-WindowsNoEditor.pak"),
		XorString("pakchunk21-WindowsNoEditor.pak"),
#if !(UE_BUILD_SHIPPING)
		XorString("pakchunk99-WindowsNoEditor.pak"),
		XorString("pakchunk99-WindowsNoEditor_0_P.pak"),
#endif
		XorString("ReadyOrNot-WindowsServer.pak"),
		XorString("pakchunk0-WindowsServer_0_P.pak"),
		XorString("pakchunk1-WindowsServer_0_P.pak"),
		XorString("pakchunk2-WindowsServer_0_P.pak"),
		XorString("pakchunk3-WindowsServer_0_P.pak"),
		XorString("pakchunk4-WindowsServer_0_P.pak"),
		XorString("pakchunk5-WindowsServer_0_P.pak"),
		XorString("pakchunk6-WindowsServer_0_P.pak"),
		XorString("pakchunk7-WindowsServer_0_P.pak"),
		XorString("pakchunk8-WindowsServer_0_P.pak"),
		XorString("pakchunk9-WindowsServer_0_P.pak"),
		XorString("pakchunk10-WindowsServer_0_P.pak"),
		XorString("pakchunk11-WindowsServer_0_P.pak"),
		XorString("pakchunk12-WindowsServer_0_P.pak"),
		XorString("pakchunk0-WindowsServer.pak"),
		XorString("pakchunk1-WindowsServer.pak"),
		XorString("pakchunk2-WindowsServer.pak"),
		XorString("pakchunk3-WindowsServer.pak"),
		XorString("pakchunk4-WindowsServer.pak"),
		XorString("pakchunk5-WindowsServer.pak"),
		XorString("pakchunk6-WindowsServer.pak"),
		XorString("pakchunk7-WindowsServer.pak"),
		XorString("pakchunk8-WindowsServer.pak"),
		XorString("pakchunk9-WindowsServer.pak"),
		XorString("pakchunk10-WindowsServer.pak"),
		XorString("pakchunk11-WindowsServer.pak"),
		XorString("pakchunk12-WindowsServer.pak")
		};