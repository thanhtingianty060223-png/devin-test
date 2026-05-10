// Copyright Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "KingOfTheHostageGM.generated.h"


/**
 * 
 */
UCLASS()
class READYORNOT_API AKingOfTheHostageGM : public AReadyOrNotGameMode_PVP
{
	
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	UFUNCTION(BlueprintCallable, Category = KOTH)
	APawn* SpawnHostage(TSubclassOf<APawn> HostageClass, TArray<FVector> SpawnLocations);
	UPROPERTY(BlueprintReadOnly, Category = KOTH)
	TArray<APawn*> SpawnedHostages;
	
	UFUNCTION(BlueprintImplementableEvent, Category = KOTH)
		bool AreAllHostagesSafe();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostageRescued, APawn*, HostageRescued);
	FOnHostageRescued OnHostageRescued;
	
	UPROPERTY(EditAnywhere, Category = KOTH)
		float Start_RoundTime = 900.0f;	

	UPROPERTY(BlueprintReadWrite, Category = Teams)
		bool bBlueTeamOnAttack;
	
};
