// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PlayersList.generated.h"

class FUniqueNetId;

DECLARE_LOG_CATEGORY_EXTERN(PlayersListLog, Log, All);

USTRUCT(BlueprintType)
struct FLobbyPlayer {
  GENERATED_USTRUCT_BODY()

 public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  FString DisplayName;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  FString RealName;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  FString Presence;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  FString StatusString;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  FString UniqueNetId;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  bool bHasVoice;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Player")
  bool bIsMuted;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UPlayersList : public UCommonActivatableWidget
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayersListUpdated);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMuteStateDelegate);

public:
    UPROPERTY(BlueprintAssignable, Category = Events)
    FOnPlayersListUpdated OnSuccess;

    UPROPERTY(BlueprintAssignable, Category = Events)
    FOnPlayersListUpdated OnFailure;

    UPROPERTY(BlueprintReadOnly, Category = Lobby)
    TArray<FLobbyPlayer> PlayersList;

    UFUNCTION(BlueprintCallable)
    void GetPlayersList();

	UFUNCTION(BlueprintCallable)
	APlayerState* GetPlayerStateFromUniqueId(FString UniqueId);

	UFUNCTION(BlueprintCallable)
	bool GetMutedState(const FString UniqueNetId);

	UFUNCTION(BlueprintCallable)
	void UpdatedMutedState();

	UFUNCTION(BlueprintCallable)
	void SetMutedState(const FString UniqueNetId, bool value);

	UFUNCTION(BlueprintCallable)
	bool VivoxAvailable();

	UPROPERTY(BlueprintAssignable, Category = Events)
	FMuteStateDelegate OnMuteStateDelegate;
};
