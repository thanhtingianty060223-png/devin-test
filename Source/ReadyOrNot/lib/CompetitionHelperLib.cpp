// Copyright Void Interactive, 2021

#include "CompetitionHelperLib.h"
#include "ReadyOrNot.h"
#include "Runtime/Online/HTTP/Public/Http.h"

void UCompetitionHelperLib::AddKill(int32 EventID, FString SteamID, FString SteamName, FString KilledPlayer)
{
	if (EventID == 0)
		return;
	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetVerb("GET");
	FString EncodedName = FGenericPlatformHttp::UrlEncode(SteamName);
	FString EncodedKilledPlayer = FGenericPlatformHttp::UrlEncode(KilledPlayer);
	Request->SetURL(FString::Printf(TEXT("https://competition.playreadyornot.com:8081/add_kill?steam_id=%s&steam_name=%s&killed_player=%s&event_id=%d"), *SteamID, *EncodedName, *EncodedKilledPlayer, EventID));
	Request->ProcessRequest();
}

void UCompetitionHelperLib::AddDeath(int32 EventID, FString SteamID, FString SteamName, FString KilledBy)
{
	if (EventID == 0)
		return;
	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetVerb("GET");
	FString EncodedName = FGenericPlatformHttp::UrlEncode(SteamName);
	FString EncodedKilledBy = FGenericPlatformHttp::UrlEncode(KilledBy);
	Request->SetURL(FString::Printf(TEXT("https://competition.playreadyornot.com:8081/add_death?steam_id=%s&steam_name=%s&killed_by=%s&event_id=%d"), *SteamID, *EncodedName, *KilledBy, EventID));
	Request->ProcessRequest();
}

void UCompetitionHelperLib::AddArrest(int32 EventID, FString SteamID, FString SteamName, FString ArrestedPlayer)
{
	if (EventID == 0)
		return;
	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetVerb("GET");
	FString EncodedName = FGenericPlatformHttp::UrlEncode(SteamName);
	FString EncodedArrestedPlayer = FGenericPlatformHttp::UrlEncode(ArrestedPlayer);
	Request->SetURL(FString::Printf(TEXT("https://competition.playreadyornot.com:8081/add_arrest?steam_id=%s&steam_name=%s&arrested=%s&event_id=%d"), *SteamID, *EncodedName, *EncodedArrestedPlayer, EventID));
	Request->ProcessRequest();
}

void UCompetitionHelperLib::AddScore(int32 EventID, FString SteamID, FString SteamName, float Score)
{
	if (EventID == 0)
		return;
	if (Score == 0.0f)
		return;

	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetVerb("GET");
	FString EncodedName = FGenericPlatformHttp::UrlEncode(SteamName);
	Request->SetURL(FString::Printf(TEXT("https://competition.playreadyornot.com:8081/add_score?steam_id=%s&steam_name=%s&score=%f&event_id=%d"), *SteamID, *EncodedName, Score, EventID));
	Request->ProcessRequest();
}

void UCompetitionHelperLib::AddWin(int32 EventID, FString SteamID, FString SteamName, FString ServerName, FString ServerMode)
{
	if (EventID == 0)
		return;
	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetVerb("GET");
	FString EncodedName = FGenericPlatformHttp::UrlEncode(SteamName);
	FString EncodedServerName = FGenericPlatformHttp::UrlEncode(ServerName);
	FString EncodedServerMode = FGenericPlatformHttp::UrlEncode(ServerMode);
	Request->SetURL(FString::Printf(TEXT("https://competition.playreadyornot.com:8081/add_win?steam_id=%s&steam_name=%s&server_name=%s&server_mode=%s&event_id=%d"), *SteamID, *EncodedName, *EncodedServerName, *EncodedServerMode, EventID));
	Request->ProcessRequest();
}
