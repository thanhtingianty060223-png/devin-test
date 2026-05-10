// Void Interactive, 2020

#include "ReadyOrNotAIConfig.h"

#include "GameModes/CoopGM.h"

UReadyOrNotAIConfig* UReadyOrNotAIConfig::Get()
{
	UE_CLOG(FUObjectThreadContext::Get().IsInConstructor, LogConfig, Fatal, TEXT("Do not call UReadyOrNotAIConfig::Get() in constructors"));
	
	if (!UBpGameplayHelperLib::GetWorldStatic())
		return nullptr;
	
	return UBpGameplayHelperLib::GetWorldStatic()->GetGameInstance<UReadyOrNotGameInstance>()->AIConfig;
}

FString UReadyOrNotAIConfig::GetConfigFileName() const
{
	return "AILevelData.ini";
}

FString UReadyOrNotAIConfig::GetConfigSectionName() const
{
	FString MapName = GetWorld()->GetName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	return MapName;
}

FString UReadyOrNotAIConfig::GetFallbackConfigSectionName() const
{
	return "Global";
}
