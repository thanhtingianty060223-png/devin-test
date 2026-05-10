// Void Interactive, 2020

#pragma once

#include "GameplayConfig.h"
#include "ReadyOrNotAIConfig.generated.h"

#define AI_OFFICIAL_CONFIG_CHECK 1

#define AI_CONFIG_GET_HASH() (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetHash() : FMD5Hash())
#define AI_CONFIG_GET_INT(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetInt(ConfigKey, ##__VA_ARGS__) : 0)
#define AI_CONFIG_GET_FLOAT(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetFloat(ConfigKey, ##__VA_ARGS__) : 0.0f)
#define AI_CONFIG_GET_BOOL(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetBool(ConfigKey, ##__VA_ARGS__) : false)
#define AI_CONFIG_GET_STRING(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetString(ConfigKey, ##__VA_ARGS__) : "")
#define AI_CONFIG_GET_VECTOR(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetVector(ConfigKey, ##__VA_ARGS__) : FVector::ZeroVector)
#define AI_CONFIG_GET_VECTOR2D(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetVector2D(ConfigKey, ##__VA_ARGS__) : FVector2D::ZeroVector)
#define AI_CONFIG_GET_STRING_ARRAY(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetStringArray_CPP(ConfigKey, ##__VA_ARGS__) : TArray<FString>())
#define AI_CONFIG_GET_STRING_ARRAY_SINGLE_LINE(ConfigKey, ...) (UReadyOrNotAIConfig::Get() ? UReadyOrNotAIConfig::Get()->GetStringArray_SingleLine_CPP(ConfigKey, ##__VA_ARGS__) : TArray<FString>())

/**
 * Helper class for accessing ini config properties for AILevelData.ini. Do not use in constructor
 */
UCLASS(Transient, NotBlueprintable, BlueprintType)
class READYORNOT_API UReadyOrNotAIConfig : public UGameplayConfig
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, DisplayName = "Get AI Config")
	static UReadyOrNotAIConfig* Get();

protected:
	virtual FString GetConfigFileName() const override;
	virtual FString GetConfigSectionName() const override;
	virtual FString GetFallbackConfigSectionName() const override;
};
