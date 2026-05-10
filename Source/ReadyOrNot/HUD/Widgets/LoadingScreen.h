// Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"


#include "LoadingScreen.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoadingScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// ##UE5UPGRADE## Check
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);

	float TimeSinceCreated = 0.0f;

	UPROPERTY()
	ULevelStreaming* StreamedLevel;

	FString Map;
	FString Mode;
	FString SessionName;
	bool bSeamlessTravel;

	FString FoundRelatingGameMode;

	bool bIsHostMigrationLoadingScreen = false;
	int32 ExpectedPlayerCount = 1;
	FString NextHostText = "";
	
public:
	
	UFUNCTION(Category = LoadingScreen)
	void SetLoadingScreen(FString InMap, FString InMode, FString InSessionName, bool bSeamlessTravel);

	UFUNCTION(BlueprintPure)
	void GetLoadingScreenDetails(FString& OutMap, FString& OutMode, FString& OutSessionName);

	float InterpLoadPercentage = 0.0f;

	void CalculateLoadPercentage(float DeltaTime);


	UFUNCTION(BlueprintPure)
	float GetLoadingPercentage();
	
	UFUNCTION(BlueprintPure)
	FString GetMapName();


	UFUNCTION(BlueprintCallable)
	void UpdateTip(UTextBlock* TipBlock);
	
};
