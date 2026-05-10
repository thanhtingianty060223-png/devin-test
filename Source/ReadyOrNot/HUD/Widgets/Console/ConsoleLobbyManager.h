// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "HUD/Widgets/MenuWidget.h"
#include "ConsoleLobbyManager.generated.h"

UENUM()
enum class ELobbyPrivacy : uint8 {
  LP_Public UMETA(DisplayName = "Public"),
  LP_Private UMETA(DisplayName = "Private"),
  LP_Invite UMETA(DisplayName = "Invite")
};
/**
 * 
 */
UCLASS()
class READYORNOT_API UConsoleLobbyManager : public UMenuWidget
{
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable)
  void SetLobbyPrivacy(ELobbyPrivacy privacy);
};
