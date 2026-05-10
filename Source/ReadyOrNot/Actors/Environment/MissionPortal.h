// Copyright Void Interactive, 2023

#pragma once

#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "MissionPortal.generated.h"

UENUM()
enum EReadyState
{
	RS_None,
	RS_Minority,
	RS_Majority,
	RS_All
};

UCLASS()
class READYORNOT_API AMissionPortal : public AReadyOrNotTriggerVolume, public IUseabilityInterface
{
	GENERATED_BODY()

	AMissionPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;
	
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class AReadyOrNotGameMode>> SelectableGameModes;

	UPROPERTY()
	class AMissionSelect* MissionSelect;
	
	UPROPERTY()
	UReadyOrNotProfile* Profile;
	
	UPROPERTY()
	class UCommanderProfile* CommanderProfile;

	UPROPERTY()
	class UMetaGameProfile* MetaGameProfile;
	
	UPROPERTY()
	UTextRenderComponent* WhiteboardText;
	
	bool bPreparedTravel = false;
	
public:
	FString GetFormattedMissionURL();

	UFUNCTION(BlueprintCallable)
	void OnMissionSelected();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionSelected);
	UPROPERTY(BlueprintAssignable)
	FOnMissionSelected OnMissionSelected_Delegate;
	
	UPROPERTY(ReplicatedUsing=OnRep_MissionURL)
	FString MissionURL = "";

	UPROPERTY(ReplicatedUsing=OnRep_MissionURL)
	FString ModeURL = "";
	
	UFUNCTION()
	void OnRep_MissionURL();
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	FName SelectedEntryPoint;
	
	UFUNCTION(BlueprintCallable)
	static void SetSelectedMode(FName InMode);

	UFUNCTION(BlueprintCallable)
	static bool IsGameModeSelectable(ECOOPMode InMode);

	UFUNCTION(BlueprintCallable)
	static bool GetSelectedModeName(FString& OutName);

	UFUNCTION(BlueprintPure)
	static bool GetSelectedMode(FString& OutMode);
	
	UFUNCTION(BlueprintCallable)
	static void SetSelectedMission(FString InMissionURL);

	UFUNCTION(BlueprintPure)
	static void GetSelectedMission(FString& OutMissionURL);

	UFUNCTION(BlueprintPure)
	static bool IsMissionStarting(bool& bStarting, float& Countdown);

	UFUNCTION(BlueprintPure)
	static bool GetPlayersReady(int32& Ready, int32& Total);

	UFUNCTION(BlueprintPure)
	static bool IsLevelUnlocked(FString InURL, bool& OutIsUnlocked, float& OutScoreRequired, FString& OutLockedUrl);

	UFUNCTION(BlueprintPure)
	static bool DoesLevelExistInBuild(FString InUrl);

	UFUNCTION(BlueprintCallable)
	static void SetSelectedEntryPoint(FName EntryPoint);
	
	void NextMission();
	void PreviousMission();
	void SelectRandomMission(bool bUseServerMapList = true);

private:
	void StartMission();
	
protected:
	UPROPERTY()
	TArray<UStaticMeshComponent*> CompsToOutline;

	UPROPERTY()
	TArray<ULightComponent*> LightsToEnable;
	
	UPROPERTY()
	TArray<UStaticMeshComponent*> CompsToOutlineMissionSelected;

	UPROPERTY()
	TArray<ULightComponent*> LightsToEnableMissionSelected;
	
	void DrawMissionPortalOutline();
	void DisableMissionPortalOutline();

	void DrawMissionSelectedPortalOutLine();
	void DisableMissionSelectedPortalOutline();

	virtual FName DetermineAnimatedIcon_Implementation() const override;
	virtual FText DetermineActionText_Implementation() const override;
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteract_Implementation() const override;

	public:
	bool AreAllPlayersInPortal() const;
	int32 GetNumPlayers() const;
	int32 GetNumberOverlappingPlayers() const;

	UFUNCTION()
	void OnPlayerJoinedLobby(AReadyOrNotPlayerController* PlayerController);
	void OnPlayerLeftLobby(AGameModeBase* GM, AController* Controller);
	
	int32 RequiredReadyPlayers = 0;

	UFUNCTION()
	void OnPlayerReadyChange(AReadyOrNotPlayerController* ReadyOrNotPlayerController, bool bReady);

	virtual void PreInitializeComponents() override;

	bool bTimerRunning = false;
	float CurrentTimer = 0.f;

	UFUNCTION(NetMulticast, Reliable, Category="Ready")
	void Multicast_SetTimer(bool bEnabled, float SetTime);

public:
	float AllReadyCountdown = 10.f;
	float MajorityReadyCountdown = 120.f;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownStarted, float, Countdown);
	UPROPERTY(BlueprintAssignable)
	FOnCountdownStarted OnCountdownStarted;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownCancelled);
	UPROPERTY(BlueprintAssignable)
	FOnCountdownCancelled OnCountdownCancelled;
	
	UFUNCTION()
	void OnRep_ReadiedPlayersChange();
	UPROPERTY(ReplicatedUsing=OnRep_ReadiedPlayersChange, BlueprintReadOnly)
	int32 NumReadyPlayers = 0;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<AReadyOrNotPlayerController*> ReadiedPlayers;

private:
	EReadyState LastReadyState = RS_None;
};
