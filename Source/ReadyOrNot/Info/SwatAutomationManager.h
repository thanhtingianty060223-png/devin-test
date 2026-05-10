// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "SwatAutomationManager.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ASwatAutomationManager : public AInfo
{
	GENERATED_BODY()

public:
	ASwatAutomationManager();
	void StartAutomation();
	void StopAutomation();

	bool bRunningAutomation = false;

	UPROPERTY()
	TArray<class ADoor*> Doors;

	UPROPERTY()
	TArray<class ADoor*> BreachedDoors;

	UPROPERTY()
	class ADoor* CurrentDoor = nullptr;

	virtual void Tick(float DeltaSeconds) override;

	class ADoor* FindClosestPathableDoor(FVector& CommandLocation);
	class ASWATCharacter* GetSquadMember();
	bool IsSquadReadyForNextCommand();
	
};
