// Copyright Void Interactive, 2021

#pragma once

#include "lib/ReadyOrNotFunctionLibrary.h"
#include "UObject/Object.h"
#include "GameplayConfig.generated.h"

/**
 * 
 */
UCLASS(Abstract, Transient, NotBlueprintable, BlueprintType)
class READYORNOT_API UGameplayConfig : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void ReloadConfig();

	FMD5Hash GetHash() const;
		
	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	int32 GetInt(FString ConfigKey, int32 FallbackValue = 0);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	float GetFloat(FString ConfigKey, float FallbackValue = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	bool GetBool(FString ConfigKey, bool FallbackValue = false);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	FString GetString(FString ConfigKey, const FString& FallbackValue = "");

	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	FVector GetVector(FString ConfigKey, const FVector& FallbackValue = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	FVector2D GetVector2D(FString ConfigKey, const FVector2D& FallbackValue = FVector2D::ZeroVector);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	TArray<FString> GetStringArray(FString ConfigKey, const TArray<FString>& FallbackValue);
	TArray<FString> GetStringArray_CPP(FString ConfigKey, const TArray<FString>& FallbackValue = {});
	
	UFUNCTION(BlueprintPure, Category = "Gameplay Config")
	TArray<FString> GetStringArray_SingleLine(FString ConfigKey, const TArray<FString>& FallbackValue);
	TArray<FString> GetStringArray_SingleLine_CPP(FString ConfigKey, const TArray<FString>& FallbackValue = {});

protected:
	virtual FString GetConfigFileName() const;
	virtual FString GetConfigSectionName() const;
	virtual FString GetFallbackConfigSectionName() const;

	template<typename T>
	T GetKey(FString ConfigKey, const T& FallbackValue, TFunctionRef<T(const FString& SectionName)> GetterFunc);
};

template <typename T>
T UGameplayConfig::GetKey(FString ConfigKey, const T& FallbackValue, TFunctionRef<T(const FString& SectionName)> GetterFunc)
{
	ConfigKey.RemoveSpacesInline();

	const FString ConfigSectionName = GetConfigSectionName();
	const bool bFoundKey = UReadyOrNotFunctionLibrary::FindConfigKeyFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), ConfigSectionName, ConfigKey);
	const bool bFoundKeyInGlobalSection = UReadyOrNotFunctionLibrary::FindConfigKeyFromINIFile(FPaths::ProjectConfigDir() + GetConfigFileName(), GetFallbackConfigSectionName(), ConfigKey);

	if (!bFoundKey && !bFoundKeyInGlobalSection)
		return FallbackValue;
	
	return GetterFunc(bFoundKey ? ConfigSectionName : GetFallbackConfigSectionName());
}
