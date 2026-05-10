// Void Interactive, 2020

#pragma once
#include "CoopGS.h"
#include "TutorialGS.generated.h"

UENUM(BlueprintType)
enum class ETutorialMissionType : uint8
{
	ETM_None,
	ETM_ShootingRange,
    ETM_KillHouse,
	ETM_BasicControls,
	ETM_Mirrorgun,
	ETM_StackUp,
	ETM_Arrest,
	ETM_Grenades,
	ETM_Movement
};

USTRUCT(BlueprintType, Blueprintable)
struct FTutorialMissionData
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETutorialMissionType MissionType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSpawnSWATTeam = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSavedLoadout Loadout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SpawnTag = "";

	bool IsValid() const
	{
		return MissionType != ETutorialMissionType::ETM_None;
	}
};

UCLASS()
class READYORNOT_API ATutorialGS : public ACoopGS
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATutorialGS();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:

	UFUNCTION()
	void OnPostUpdateSwatCommands(class USwatCommandWidget* Widget, TArray<FSwatCommand>& SwatCommands);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTutorialMissionData CurrentTutorialData;

	UFUNCTION(BlueprintCallable)
	void SetCurrentTutorialData(FTutorialMissionData TutorialData);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> TutorialMenuLevel;

	UPROPERTY(BlueprintReadOnly)
	ULevelStreaming* TutorialMenuStreamedLevel;

	UPROPERTY(BlueprintReadOnly)
	ULevelStreaming* CurrentTutorialStreamedLevel;

	UFUNCTION(BlueprintCallable)
    TSoftObjectPtr<UWorld> GetCurrentTutorialStreamedLevel(); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> ShootingRangeLevel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> KillHouseLevel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> BasicControlsLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> MirrorgunLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> StackUpLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UWorld> ArrestLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UWorld> GrenadesLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UWorld> MovementLevel;

	// Returns true if loaded
	bool bHasLoadedTutorialMenuLevel = false;
	bool TryLoadMenuTutorialLevel();
	void LoadMenuTutorialLevel() { TryLoadMenuTutorialLevel(); }
	void UnloadStreamedLevel(ULevelStreaming* StreamedLevel);
	
	bool bHasLoadedTutorialMission = false;
	bool TryLoadTutorialMission();
	void LoadTutorialMission() { TryLoadTutorialMission(); }
	
	UPROPERTY(BlueprintReadWrite)
	bool bFinishedUsingTutorialMenu = false;

	virtual void PreGamePlayingStateLogic() override;
	virtual void LoadStartupWidgetsAfterLoadingScreen() override;

protected:
	void SetSpawnPoliceState(bool bSpawnPolice);
};
