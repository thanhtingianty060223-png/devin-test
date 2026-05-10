// Void Interactive, 2020


#include "ReadyOrNotBackend.h"

#include <string>

#include "HttpModule.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IHttpResponse.h"
#include "lib/GameFeatureLibrary.h"

#if defined(WITH_STEAM)
#include "steam/steam_api.h"
#endif

UReadyOrNotBackend::UReadyOrNotBackend()
{
}

void UReadyOrNotBackend::TickLoginDelay()
{
	if (LoginDelay > 0.0f)
	{
		LoginDelay -= 1.0f;
	} else
	{
		GetWorld()->GetTimerManager().ClearTimer(TH_TickLoginDelay);
	}

	
}


bool UReadyOrNotBackend::IsLoggedIn()
{
	return LoginState == ELoginState::LS_LoggedIn && !Ticket.IsEmpty();
}

bool UReadyOrNotBackend::IsLoggingIn()
{
	return LoginState == ELoginState::LS_LoggingIn;
}

bool UReadyOrNotBackend::HasLoginFailed()
{
	return LoginState == ELoginState::LS_LoginFail;
}


FString UReadyOrNotBackend::GetSteamID()
{
// Tmp fix to prevent crash on console
#if defined(TARGET_CONSOLE) || defined(USE_EOS)
	return "";
#endif
	if (SteamId == "")
	{
		if (UReadyOrNotStatics::GetReadyOrNotGameInstance() && UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetFirstGamePlayer())
		{
			SteamId = UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetFirstGamePlayer()->GetPreferredUniqueNetId()->ToString();
		}
	}
	return SteamId;
}


FString UReadyOrNotBackend::GetSteamName()
{
	if (SteamName == "")
	{
		if (UReadyOrNotStatics::GetReadyOrNotGameInstance() && UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetFirstGamePlayer())
		{
			SteamName = UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetFirstGamePlayer()->GetNickname();
		}
	}
	// intentionally double quoted so that slashes and other things don't break when sent over the network
	return FGenericPlatformHttp::UrlEncode(FGenericPlatformHttp::UrlEncode(SteamName));
}


FString UReadyOrNotBackend::GetEncodedTicket()
{
	return FGenericPlatformHttp::UrlEncode(Ticket);
}

FString UReadyOrNotBackend::BuildEndPoint()
{
#if UE_BUILD_DEVELOPMENT
	return UTF8_TO_TCHAR(XorString("https://vx-dev.readyornotgame.com/"));
#endif
	return UTF8_TO_TCHAR(XorString("https://vx-prod.readyornotgame.com/"));
}

FString UReadyOrNotBackend::GetApiKey()
{
	return UTF8_TO_TCHAR(XorString("hy4ks9kMoVXJPL1WiUvtkaqVspat79kc"));
}

void UReadyOrNotBackend::SetLoginFailed(FString Msg)
{
	LoginState = ELoginState::LS_LoginFail;
	LoginFailedMsg = Msg;
}


void UReadyOrNotBackend::StartLogin()
{
	if(UGameFeatureLibrary::IsGameDemo())
	{
		// Not supported with demo
		LoginState = ELoginState::LS_LoginFail;
		return;
	}
#if defined(TARGET_CONSOLE)  || defined(USE_EOS)
	LoginState = ELoginState::LS_LoggedIn;
	Ticket = "NO TICKET YET";
	return; // for now
#endif
	if (IsLoggingIn() || IsLoggedIn())
		return;

	if (RetryCount > 20 || LoginDelay > 0.0f)
	{
		LoginState = ELoginState::LS_LoginFail;
		return;
	}
	
#if UE_SERVER
	return;
#endif
	LoginState = ELoginState::LS_LoggingIn;

#if defined(WITH_STEAM)
	// can't possibly login without a steam user
	if (!SteamUser())
	{
		SetLoginFailed(XorString("No Connection To Steam"));
		return;
	}

	uint32 unTokenLen = 0;
	HAuthTicket m_hAuthTicket = SteamUser()->GetAuthSessionTicket(rgchToken, sizeof(rgchToken), &unTokenLen);

	if (m_hAuthTicket != k_HAuthTicketInvalid)
	{
		
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetTimerManager().SetTimer(TH_DoLogin, this, &UReadyOrNotBackend::DoLogin, 3.0f, false);
	} else
	{
		SetLoginFailed(XorString("Steam Authentication Ticket Invalid"));
	}
#endif
}

void UReadyOrNotBackend::DoLogin()
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/octet-stream"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());

	Request->SetVerb("POST");
	TArray<uint8> Bytes;
	for (int32 i = 0; i < 1024; i++)
	{
		Bytes.Add(rgchToken[i]);
	}
	Request->SetContent(Bytes);
	Request->SetURL(BuildEndPoint() +  UTF8_TO_TCHAR(XorString("/login/")) + GetSteamID() + XorString("/") + GetSteamName());
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnLogin_ResponseReceived);
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnLogin_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bWasSuccessful)
{
#if defined(WITH_VIVOX)
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		Ticket = Response.Get()->GetContentAsString();
		LoginState = ELoginState::LS_LoggedIn;
		GetOneTimeUseDiscordCode();
		CheckDLCOwnership(STEAM_DLC_SUPPORTER);
		CheckIfIAmBanned();
		GetWorld()->GetTimerManager().SetTimer(TH_Heartbeat, this, &UReadyOrNotBackend::Heartbeat, 300.0f, true);
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->StartVivoxLogin();
		RetryCount = 0;
	} else
	{
		SetLoginFailed(XorString("Failed to Authenticate with Backend. Code: ") + FString::FromInt(Response->GetResponseCode() + 570));
		GetWorld()->GetTimerManager().SetTimer(TH_TickLoginDelay, this, &UReadyOrNotBackend::TickLoginDelay, 1.0f, true);
		RetryCount++;
		LoginDelay = RetryCount * 5.0f;
	}
#endif
}


void UReadyOrNotBackend::Logout()
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());

	Request->SetVerb("POST");
	Request->SetURL(BuildEndPoint() +  UTF8_TO_TCHAR(XorString("/logout/")) + GetSteamID() + "/" + GetEncodedTicket());
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnLogout_ResponseReceived);
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnLogout_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		V_LOGM(LogReadyOrNot, "Logged Out!");
	}
}

void UReadyOrNotBackend::GetOneTimeUseDiscordCode()
{
#if UE_SERVER
	return;
#endif
	

	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	
	Request->SetVerb("GET");
	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/verification_code/")) + GetSteamID() + "/" + GetEncodedTicket());
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnOneTimeUseDiscordCode_ResponseReceived);
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnOneTimeUseDiscordCode_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
#if UE_BUILD_DEVELOPMENT
		V_LOGM(LogReadyOrNot, "Verification Code: %s", *Response->GetContentAsString());
#endif
		CachedDiscordOneTimeUseCode = Response->GetContentAsString();
	} else
	{
		LoginState = ELoginState::LS_LoggedOut;
	}
}

void UReadyOrNotBackend::DownloadHashes()
{
#if UE_SERVER
	return;
#endif
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");
	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/jQU775nhsRkEZSPbBCW5HNIF7Jrgh68gx8r9oq2B6aqNMzKbvgPE0RPC8hF7uktFnVc0zASmKJN/")));
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnDownloadedHashes_ResponseReceived);
	Request->ProcessRequest();
}


void UReadyOrNotBackend::OnDownloadedHashes_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                             bool bWasSuccessful)
{
	/*UReadyOrNotGameInstance* gi = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{

			TArray<TSharedPtr<FJsonValue> > BlackListArray_Hashes = JsonObject->GetArrayField(XorString("BLDLLHZ"));
			for (TSharedPtr<FJsonValue> DLLHash : BlackListArray_Hashes)
			{
#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "%s", *DLLHash->AsString());
#endif
				gi->Hashes.BLDLLHZ.AddUnique(FBase64::Encode(DLLHash->AsString()));
			}
			TArray<TSharedPtr<FJsonValue> > BlackListArray_Programs = JsonObject->GetArrayField(XorString("BLPN"));
			for (TSharedPtr<FJsonValue> ProgramName : BlackListArray_Programs)
			{
#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "%s", *ProgramName->AsString());
#endif
				
				gi->Hashes.BLPN.AddUnique(FBase64::Encode(ProgramName->AsString()));
			}
			TArray<TSharedPtr<FJsonValue> > BlackListArray_ProgramHashes = JsonObject->GetArrayField(XorString("BLHZ"));
			for (TSharedPtr<FJsonValue> ProgramHash : BlackListArray_ProgramHashes)
			{
				
#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "%s", *ProgramHash->AsString());
#endif
				gi->Hashes.BLPHZ.AddUnique(FBase64::Encode(ProgramHash->AsString()));
			}
			TArray<TSharedPtr<FJsonValue> > BlackListArray_ProgramWindowTitles = JsonObject->GetArrayField(XorString("BLWT"));
			for (TSharedPtr<FJsonValue> ProgramTitle : BlackListArray_ProgramWindowTitles)
			{
#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "%s", *ProgramTitle->AsString());
#endif
				gi->Hashes.BLWT.AddUnique(FBase64::Encode(ProgramTitle->AsString()));
			}
			TArray<TSharedPtr<FJsonValue> > BlackListArray_DLLNames = JsonObject->GetArrayField(XorString("BLDN"));
			for (TSharedPtr<FJsonValue> DLLName : BlackListArray_DLLNames)
			{
#if !UE_BUILD_SHIPPING
				V_LOGM(LogReadyOrNot, "%s", *DLLName->AsString());
#endif
				gi->Hashes.BLDLLN.AddUnique(FBase64::Encode(DLLName->AsString()));
			}
			gi->SaveHashMap();
		}
	} else
	{
		gi->LoadHashMap();
	}*/
}

void UReadyOrNotBackend::RetrieveVivoxLoginToken(FString PlayerName)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");

	
	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/vx_login/")) + GetSteamID() + "/" + GetEncodedTicket() + "/" + FGenericPlatformHttp::UrlEncode(PlayerName));
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnVivoxLoginToken_ResponseReceived);
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnVivoxLoginToken_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
															bool bWasSuccessful)
{
#if defined(WITH_VIVOX)
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		FString VivoxLoginToken = Response->GetContentAsString();
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->DoVivoxLogin(VivoxLoginToken);
	} else
	{
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->bVivoxLoggingIn = false;
		LoginState = ELoginState::LS_LoggedOut;
	}
#endif
}


void UReadyOrNotBackend::RetrieveJoinToken(FString PlayerName, FString ChannelName)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	if (ChannelName.IsEmpty())
		return;

	// never ever allow this in single player
	if (GetWorld())
	{
		if (GetWorld()->GetNetMode() == NM_Standalone)
			return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");


	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/vx_join/")) + GetSteamID() + "/" + GetEncodedTicket() + "/" + FGenericPlatformHttp::UrlEncode(PlayerName) + "/" + FGenericPlatformHttp::UrlEncode(ChannelName));
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnJoinToken_ResponseReceived);
	Request->ProcessRequest();
}


void UReadyOrNotBackend::OnJoinToken_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
													bool bWasSuccessful)
{
#if defined(WITH_VIVOX)
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		FString NA, ChannelName;
		Request->GetURL().Split("/", &NA, &ChannelName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->OnJoinTokenReceived(ChannelName, Response->GetContentAsString());
	} else
	{
		LoginState = ELoginState::LS_LoggedOut;
	}
#endif
}



void UReadyOrNotBackend::CheckDLCOwnership(int32 DLC)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");


	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/check_dlc_ownership/")) + GetSteamID() + "/" + GetEncodedTicket() + "/" + FString::FromInt(DLC));
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnOwnsDLC_ResponseReceived);
	Request->ProcessRequest();
}


void UReadyOrNotBackend::OnOwnsDLC_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                    bool bWasSuccessful)
{
	
	FString NA, DLC;
	Request->GetURL().Split("/", &NA, &DLC, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (!DLC.IsEmpty())
	{
#if UE_BUILD_DEVELOPMENT
		V_LOGM(LogReadyOrNot, "Owns DLC %s %d", *DLC, bWasSuccessful && Response->GetResponseCode() == 200);
#endif
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->OwnedDLCMap.Add(FCString::Atoi(*DLC), bWasSuccessful && Response->GetResponseCode() == 200);
	}
}


void UReadyOrNotBackend::CheckIsBanned(FString OtherSteamId)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");


	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/check_is_banned/")) + GetSteamID() + "/" + GetEncodedTicket() + "/" + OtherSteamId);
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnIsBanned_ResponseReceived);
	Request->ProcessRequest();
}

void UReadyOrNotBackend::CheckIfIAmBanned()
{
	CheckIsBanned(GetSteamID());
}


void UReadyOrNotBackend::DoPostGameMetric(const EGameEventMetric InEventType, TSharedRef<FJsonObject> InObj)
{
	ensure(InEventType != EGameEventMetric::GEM_NONE);
	if(InEventType == EGameEventMetric::GEM_NONE)
	{
		return;
	}
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->GetApiKey());
	
	// Setup the constant fields
	InObj->SetNumberField(TEXT("type"), static_cast<int32_t>(InEventType));

#if !UE_BUILD_DEVELOPMENT
	InObj->SetStringField(TEXT("buildid"), UBpGameplayHelperLib::GetProjectVersion());
#else
	InObj->SetStringField(TEXT("buildid"), TEXT("development"));
#endif

	if(USteamworksIntegration::IsDLCInstalled(STEAM_DLC_SUPPORTER))
		InObj->SetStringField(TEXT("edition"), "supporter");
	else
		InObj->SetStringField(TEXT("edition"), "standard");


	FString PayloadJsonStr;
	auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&PayloadJsonStr);
	FJsonSerializer::Serialize(InObj, JsonWriter);
	JsonWriter->Close();
	
	Request->SetVerb("POST");
	Request->SetContentAsString(PayloadJsonStr);
	Request->SetURL(XorString("https://events.voidinteractive.io/events/"));
	Request->ProcessRequest();
	
}

void UReadyOrNotBackend::OnGameStartedMetric(const FString InMap, const FString InGameType, const int32 InNumPlayers)
{
	const TSharedRef<FJsonObject> Info = MakeShared<FJsonObject>();
	Info->SetStringField(TEXT("map"), InMap);
	Info->SetStringField(TEXT("gametype"), InGameType);
	Info->SetStringField(TEXT("numplayers"), FString::FormatAsNumber(InNumPlayers));

	DoPostGameMetric(EGameEventMetric::GEM_GAME_STARTED, Info);
}

void UReadyOrNotBackend::OnGameFinishedMetric(const FString InMap, const FString InGameType,
	const FString InGameResult)
{
	const TSharedRef<FJsonObject> Info = MakeShared<FJsonObject>();
	Info->SetStringField(TEXT("map"), InMap);
	Info->SetStringField(TEXT("gametype"), InGameType);
	Info->SetStringField(TEXT("result"), InGameResult);

	DoPostGameMetric(EGameEventMetric::GEM_GAME_FINISHED, Info);
}

void UReadyOrNotBackend::OnPlayerGameFinishedMetric(const FString InMap, const FString InGameType, float InAverageFps)
{
	const TSharedRef<FJsonObject> Info = MakeShared<FJsonObject>();
	Info->SetStringField(TEXT("map"), InMap);
	Info->SetStringField(TEXT("gametype"), InGameType);
	Info->SetStringField(TEXT("averagefps"), FString::FormatAsNumber(InAverageFps));

	DoPostGameMetric(EGameEventMetric::GEM_PLAYER_GAME_FINISHED, Info);
}

void UReadyOrNotBackend::OnGameCrashedMetric(const FString InState)
{
	const TSharedRef<FJsonObject> Info = MakeShared<FJsonObject>();
	Info->SetStringField(TEXT("map"), InState);

	DoPostGameMetric(EGameEventMetric::GEM_GAME_FINISHED, Info);
}

void UReadyOrNotBackend::OnMapAnalyticsActorAdded(const FGuid InGameId, int8 InActorId, const AActor* InActor, const TMap<FString, FString>& InProperties) const
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->GetApiKey());
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	JsonObject->SetNumberField(TEXT("id"), InActorId);
	JsonObject->SetStringField(TEXT("name"), UKismetSystemLibrary::GetObjectName(InActor));
	for(auto Property : InProperties)
	{
		JsonObject->SetStringField(Property.Key, Property.Value);
	}
	
	FString PayloadJsonStr;
	auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&PayloadJsonStr);
	FJsonSerializer::Serialize(JsonObject, JsonWriter);
	JsonWriter->Close();
	
	Request->SetVerb("POST");
	Request->SetContentAsString(PayloadJsonStr);
	Request->SetURL(XorString("https://events.voidinteractive.io/mapanalytics/actor?g=") + InGameId.ToString());
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnMapAnalyticsGameStarted(const FGuid InGameId, const int8 InDataVersion, const FString InLevelName, const FString InMode) const
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	JsonObject->SetStringField(TEXT("level"), InLevelName);
	JsonObject->SetStringField(TEXT("mode"), InMode);
	JsonObject->SetNumberField(TEXT("version"), InDataVersion);
	JsonObject->SetStringField(TEXT("start"), FDateTime::UtcNow().ToIso8601());


#if !UE_BUILD_DEVELOPMENT
	JsonObject->SetStringField(TEXT("buildid"), UBpGameplayHelperLib::GetProjectVersion());
#else
	JsonObject->SetStringField(TEXT("buildid"), TEXT("development"));
#endif

	if(USteamworksIntegration::IsDLCInstalled(STEAM_DLC_SUPPORTER))
		JsonObject->SetStringField(TEXT("edition"), "supporter");
	else
		JsonObject->SetStringField(TEXT("edition"), "standard");
	
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->GetApiKey());

	FString PayloadJsonStr;
	auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&PayloadJsonStr);
	FJsonSerializer::Serialize(JsonObject, JsonWriter);
	JsonWriter->Close();
	
	Request->SetVerb("POST");
	Request->SetContentAsString(PayloadJsonStr);
	Request->SetURL(XorString("https://events.voidinteractive.io/mapanalytics/start?g=") + InGameId.ToString());
	
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnMapAnalyticsGameData(const FGuid InGameId, const uint32 PacketIndex, const bool IsGamedEnded, const FBufferArchive& InData) const
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/octet-stream"));
	Request->SetHeader(XorString("x-api-key"), UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->GetApiKey());

	Request->SetVerb("POST");
	Request->SetContent(InData);
	Request->SetURL(XorString("https://events.voidinteractive.io/mapanalytics/data?g=")
		+ InGameId.ToString()
		+ "&e=" + (IsGamedEnded ? "1" : "0")
		+ "&i=" + FString::FormatAsNumber(PacketIndex));
	Request->ProcessRequest();
}


void UReadyOrNotBackend::OnIsBanned_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                     bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			bool bIsBanned = false;
			JsonObject->TryGetBoolField(XorString("banned"), bIsBanned);
			FString BanReason = "";
			JsonObject->TryGetStringField(XorString("ban_reason"), BanReason);
			FString BannedSteamId = JsonObject->GetStringField(XorString("steam_id"));
			OnCheckedBanStatus.Broadcast(BannedSteamId, bIsBanned, BanReason, BannedSteamId == GetSteamID());
		}
	}
}


void UReadyOrNotBackend::StartCapturingProfile()
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	
	UReadyOrNotStatics::GetReadyOrNotPlayerController()->ConsoleCommand("stat startfile");
	GetWorld()->GetTimerManager().SetTimer(TH_FinishedCapturingProfile, this, &UReadyOrNotBackend::OnFinishedCapturingProfile, 20.0f, false);
	OnStatsStarted.Broadcast();
}

void UReadyOrNotBackend::OnFinishedCapturingProfile()
{
	if (!UReadyOrNotStatics::GetReadyOrNotPlayerController())
		return;
	UReadyOrNotStatics::GetReadyOrNotPlayerController()->ConsoleCommand("stat stopfile");
	// find the latest profile
	IFileManager& FileManager = IFileManager::Get();


	TArray<FString> Files;
	FString Root =  FPaths::ProjectSavedDir() + "Profiling/UnrealStats";
	FileManager.FindFilesRecursive(Files, *Root, TEXT("*.ue4stats"), true, true);
	FDateTime NewestDateTime = FDateTime::MinValue();
	FString NewestStatsFile = ""; 
	for (FString file : Files) 
	{
		FDateTime DateTime = FileManager.GetTimeStamp(*file);
		if (DateTime.ToUnixTimestamp() > NewestDateTime.ToUnixTimestamp())
		{
			NewestDateTime = DateTime;
			NewestStatsFile = file;
		}
		
	}

	if (!NewestStatsFile.IsEmpty())
	{
		FHttpModule* Http = &FHttpModule::Get();
		if (!Http)
		{
			return;
		}

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
		Request->SetHeader(XorString("Content-Type"), XorString("application/octet-stream"));
		Request->SetHeader(XorString("x-api-key"), GetApiKey());

		FString LeftS, RightS;
		NewestStatsFile.Split("/", &LeftS, &RightS, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		Request->SetVerb("POST");
		UploadedStatsName = RightS;
		OnStatsUploadProgress.Broadcast(UploadedStatsName, 0.0f);
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *NewestStatsFile);
		Request->SetContent(Bytes);
		Request->SetURL(XorString("https://stats.readyornotgame.com/UploadStatsV2/") + FGenericPlatformHttp::UrlEncode(UBpGameplayHelperLib::GetProjectVersion() + '_' + RightS) + "/" + GetSteamName() + "/" + GetSteamID());
		Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnStatsSaved_ResponseReceived);
		Request->OnRequestProgress().BindUObject(this, &UReadyOrNotBackend::OnStatsProgress);
		Request->ProcessRequest();
	}
}

void UReadyOrNotBackend::LogMessage(FString Message, bool bVerbose)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}
	
	if (!UReadyOrNotStatics::GetReadyOrNotGameInstance())
		return;

	UReadyOrNotBackend* Be = UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend;
	if (!Be)
		return;

	if (Be->SteamName.IsEmpty() || Be->SteamId.IsEmpty())
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/octet-stream"));
	Request->SetHeader(XorString("x-api-key"), UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend->GetApiKey());
	
	Request->SetVerb("POST");
	FString From = "[" + UBpGameplayHelperLib::GetProjectVersion() + "] " + Be->SteamName + " (" + Be->SteamId + ")";
	if (Be->GetWorld())
	{
		FString Type = "";
		ESessionType SessionType = UReadyOrNotStatics::GetReadyOrNotGameInstance()->SessionType;
		if (Be->GetWorld()->GetNetMode() == ENetMode::NM_Client)
		{
			Type = "Client";
		} else if (Be->GetWorld()->GetNetMode() == ENetMode::NM_Standalone)
		{
			Type = "Standalone";
		} else
		{
			switch (SessionType)
			{
			case ESessionType::ST_None: break;
			case ESessionType::ST_SinglePlayer:
				Type = "Standalone";
				break;
			case ESessionType::ST_Public:
				Type = "Public";
				break;
			case ESessionType::ST_Friends:
				Type = "Friends";
				break;
			default: ;
			}
		}
		if (!Type.IsEmpty())
		{
			From += " [" + Type + "]\n"; 
		}
	}
	Request->SetContentAsString((bVerbose ? "Verbose" : "") + From + "\n" + Message);
	Request->SetURL(XorString("https://stats.readyornotgame.com/Log/"));
	Request->ProcessRequest();
}

void UReadyOrNotBackend::OnStatsSaved_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                       bool bWasSuccessful)
{
	bProfileInProgress = false;
	OnStatsSaved.Broadcast(bWasSuccessful, UploadedStatsName);	
}

void UReadyOrNotBackend::OnStatsProgress(FHttpRequestPtr Request, int32 SentBytes, int32 ReceivedBytes)
{
	OnStatsUploadProgress.Broadcast(UploadedStatsName, ((float)SentBytes / (float)Request->GetContentLength()));
}


void UReadyOrNotBackend::Heartbeat()
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(XorString("User-Agent"), XorString("X-UnrealEngine-Agent"));
	Request->SetHeader(XorString("Content-Type"), XorString("application/json"));
	Request->SetHeader(XorString("x-api-key"), GetApiKey());
	Request->SetVerb("GET");


	Request->SetURL(BuildEndPoint() + UTF8_TO_TCHAR(XorString("/heartbeat/")) + GetSteamID() + "/" + GetEncodedTicket());
	Request->OnProcessRequestComplete().BindUObject(this, &UReadyOrNotBackend::OnHeartbeat_ResponseReceived);
	Request->ProcessRequest();
}


void UReadyOrNotBackend::OnHeartbeat_ResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bWasSuccessful)
{
	if (!bWasSuccessful || Response->GetResponseCode() != 200)
	{
		GetWorld()->GetTimerManager().ClearTimer(TH_Heartbeat);
		LoginState = ELoginState::LS_LoggedOut;
	}
}



