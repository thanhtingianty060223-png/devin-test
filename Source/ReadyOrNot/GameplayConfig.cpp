// Copyright Void Interactive, 2021

#include "GameplayConfig.h"

void UGameplayConfig::ReloadConfig()
{
	GConfig->LoadFile(FPaths::ProjectConfigDir() + GetConfigFileName());
}

FMD5Hash UGameplayConfig::GetHash() const
{
	return FMD5Hash::HashFile(*(FPaths::ProjectConfigDir() + GetConfigFileName()));
}

int32 UGameplayConfig::GetInt(const FString ConfigKey, const int32 FallbackValue)
{
	return GetKey<int32>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetIntegerFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

float UGameplayConfig::GetFloat(const FString ConfigKey, const float FallbackValue)
{
	return GetKey<float>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

bool UGameplayConfig::GetBool(const FString ConfigKey, const bool FallbackValue)
{
	return GetKey<bool>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetBoolFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

FString UGameplayConfig::GetString(const FString ConfigKey, const FString& FallbackValue)
{
	return GetKey<FString>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetStringFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

FVector UGameplayConfig::GetVector(const FString ConfigKey, const FVector& FallbackValue)
{
	return GetKey<FVector>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetVectorFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

FVector2D UGameplayConfig::GetVector2D(FString ConfigKey, const FVector2D& FallbackValue)
{
	return GetKey<FVector2D>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetVector2DFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

TArray<FString> UGameplayConfig::GetStringArray(const FString ConfigKey, const TArray<FString>& FallbackValue)
{
	return GetStringArray_CPP(ConfigKey, FallbackValue);
}

TArray<FString> UGameplayConfig::GetStringArray_CPP(const FString ConfigKey, const TArray<FString>& FallbackValue)
{
	return GetKey<TArray<FString>>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetStringArrayFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

TArray<FString> UGameplayConfig::GetStringArray_SingleLine(const FString ConfigKey, const TArray<FString>& FallbackValue)
{
	return GetStringArray_SingleLine_CPP(ConfigKey, FallbackValue);
}

TArray<FString> UGameplayConfig::GetStringArray_SingleLine_CPP(const FString ConfigKey, const TArray<FString>& FallbackValue)
{
	return GetKey<TArray<FString>>(ConfigKey, FallbackValue, [&](const FString& SectionName)
	{
		return UReadyOrNotFunctionLibrary::GetSingleLineArrayFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), SectionName, ConfigKey);
	});
}

FString UGameplayConfig::GetConfigFileName() const
{
	return "";
}

FString UGameplayConfig::GetConfigSectionName() const
{
	return "Gameplay";
}

FString UGameplayConfig::GetFallbackConfigSectionName() const
{
	return "Default";
}
