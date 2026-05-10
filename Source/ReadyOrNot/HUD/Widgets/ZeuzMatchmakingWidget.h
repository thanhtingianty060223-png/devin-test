// ##UE5UPGRADE## Zeus
//// Copyright Void Interactive, 2021
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "Blueprint/UserWidget.h"
//#include "ApiMatchmaking.h"
//#include "ZeuzMatchmakingWidget.generated.h"
//
//UENUM(BlueprintType)
//enum class EMatchmakingStatus : uint8
//{
//	MS_None,
//	MS_Matchmaking,
//	MS_Success,
//	MS_Failure,
//	MS_Cancelled
//};
//
///**
// * 
// */
//UCLASS()
//class READYORNOT_API UZeuzMatchmakingWidget : public UUserWidget
//{
//	GENERATED_BODY()
//
//
//protected:
//
//	UFUNCTION(BlueprintCallable)
//	void ResetMatchmaking();
//
//	UFUNCTION(BlueprintCallable)
//	void StartMatchmaking(FString Region);
//
//	// Don't do anything but update the state for these
//	UFUNCTION(BlueprintCallable)
//	void StartPartyMatchmaking();
//
//	// don't do anything but update the state for these
//	UFUNCTION(BlueprintCallable)
//	void StopPartyMatchmaking();
//
//	UZeuzApiMatchmaking::FDelegateMatchmakingCreate MatchmakingCreateDelegate;
//
//	UFUNCTION()
//	void OnZeuzMatchmakingCreate(const FZeuzMatchMakingStatus& Status, FString Error);
//	
//	FTimerHandle MatchMakingUpdate_Handle;
//	UZeuzApiMatchmaking::FDelegateMatchmakingUpdate MatchmakingUpdateDelegate;
//	UFUNCTION()
//	void OnZeuzMatchmakingUpdate(const FZeuzMatchMakingStatus& Status, FString Error);
//
//	int32 ConnectionAttempt = 0;
//	FTimerHandle TryConnectServer_Handle;
//
//	UFUNCTION()
//	void TryConnectServer(FString ConnectIp);
//
//	UFUNCTION()
//	void MatchmakingUpdate();
//
//	UFUNCTION(BlueprintCallable)
//	void StopMatchmaking();
//
//	FString LastMatchmakeRegion;
//
//	UPROPERTY(BlueprintReadOnly)
//	EMatchmakingStatus MatchmakingStatus;
//
//	UPROPERTY(BlueprintReadOnly)
//	FZeuzMatchMakingStatus ZeuzMatchmakingStatus;
//	
//};
