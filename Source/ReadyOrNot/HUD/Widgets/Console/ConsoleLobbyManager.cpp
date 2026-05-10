// Copyright Void Interactive, 2023


#include "HUD/Widgets/Console/ConsoleLobbyManager.h"
#include "ConsoleLobbyManager.h"

void UConsoleLobbyManager::SetLobbyPrivacy(ELobbyPrivacy privacy) 
{
	FOnlineSubsystemBPCallHelperAdvanced Helper(TEXT("LobbySessionCreateCompleted"), GetWorld());

	if (Helper.OnlineSub != nullptr) 
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
#if defined(TARGET_SONY)
        auto LobbySession = Sessions->GetNamedSession(NAME_PartySession);
#else       
		auto LobbySession = Sessions->GetNamedSession(NAME_GameSession);
#endif

		if (LobbySession->RegisteredPlayers.Num() > 1) 
		{
			// TODO: Warn if session must be closed
        } 

		// TODO: Switch privacy
		switch(privacy) 
		{
			case ELobbyPrivacy::LP_Public:
				break;

			case ELobbyPrivacy::LP_Private:
		        break;

			case ELobbyPrivacy::LP_Invite:
	            break;

		}
	}
}
