// Copyright Void Interactive, 2021

#include "ReadyOrNotStatics.h"
#include "ReadyOrNotLevelScript.h"
#include "ReadyOrNotGameInstance.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameState.h"

AReadyOrNotLevelScript* UReadyOrNotStatics::GetReadyOrNotLevelScript()
{
    return Cast<AReadyOrNotLevelScript>(UBpGameplayHelperLib::GetWorldStatic()->GetLevelScriptActor());
}

UReadyOrNotGameInstance* UReadyOrNotStatics::GetReadyOrNotGameInstance()
{
    UReadyOrNotGameInstance* GameInstance = nullptr;
    if (UBpGameplayHelperLib::GetWorldStatic())
        GameInstance = Cast<UReadyOrNotGameInstance>(UBpGameplayHelperLib::GetWorldStatic()->GetGameInstance());

    if (!GameInstance)
    {
        // can't find just return any game instance
        for (TObjectIterator<UReadyOrNotGameInstance>It; It;)
        {
            return *It;
        }
    }

    return GameInstance;
}

AReadyOrNotGameMode* UReadyOrNotStatics::GetReadyOrNotGameMode()
{
    if (UBpGameplayHelperLib::GetWorldStatic())
        return Cast<AReadyOrNotGameMode>(UBpGameplayHelperLib::GetWorldStatic()->GetAuthGameMode());

    return nullptr;
}

AReadyOrNotGameState* UReadyOrNotStatics::GetReadyOrNotGameState()
{
    if (UBpGameplayHelperLib::GetWorldStatic())
        return Cast<AReadyOrNotGameState>(UBpGameplayHelperLib::GetWorldStatic()->GetGameState());

    return nullptr;
}

AConversationManager* UReadyOrNotStatics::GetConversationManager()
{
    return GetReadyOrNotLevelScript()->GetConversationManager();
}

AReadyOrNotPlayerController* UReadyOrNotStatics::GetReadyOrNotPlayerController()
{
    if (UBpGameplayHelperLib::GetWorldStatic())
        return Cast<AReadyOrNotPlayerController>(UBpGameplayHelperLib::GetWorldStatic()->GetFirstPlayerController());

    return nullptr;
}

bool UReadyOrNotStatics::DoesMapExist(FString Map)
{
    if (Map.Contains("?game="))
    {
        FString OutL, OutR;
        Map.Split("?", &OutL, &OutR);
        Map = OutL;
    }
    
    TArray<FString> MapList = UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetBuiltMapList();
    for (FString s : MapList)
    {
        if (s.Contains(Map))
            return true;
    }
    for (FString s : UReadyOrNotStatics::GetReadyOrNotGameInstance()->GetBuiltModdedMapList())
    {
        if (s.Contains(Map))
            return true;
    }
    
    return false;
}
