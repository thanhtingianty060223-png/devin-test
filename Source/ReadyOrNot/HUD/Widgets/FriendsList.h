// Copyright Void Interactive, 2017

#pragma once

#include "Blueprint/UserWidget.h"
#include "FriendsList.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(FriendsListLog, Log, All);

USTRUCT(BlueprintType)
struct FFriend
{
	GENERATED_USTRUCT_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString RealName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString Presence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString StatusString;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
	int32 StatusState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString UniqueNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		bool bRunningSameGame;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		bool bHasVoice;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		bool bJoinable;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UFriendsList : public UCommonActivatableWidget
{
	GENERATED_BODY()

		virtual void NativeConstruct() override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFriendsListUpdated);

public:

	UPROPERTY(BlueprintAssignable, Category = Events)
		FOnFriendsListUpdated OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = Events)
		FOnFriendsListUpdated OnFailure;

	UPROPERTY(BlueprintReadOnly, Category = Friends)
		TArray<FFriend> FriendsList;

	UFUNCTION(BlueprintPure, Category = Friends)
	static FBPUniqueNetId CreateUniqueNetIdFromString(const FString PlatformId);

	
	
	UFUNCTION(BlueprintCallable, Category = Friends)
		void GetFriendsList();

		void OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);

		FOnReadFriendsListComplete ReadCompleteDelegate;
	
};