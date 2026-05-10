// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "ReplayCameraPawn.h"
#include "Actors/Door.h"
#include "HUD/Widgets/ReplayControls.h"
#include "ReplayController.generated.h"

UENUM(BlueprintType)
enum ECameraState
{
	Freecam     UMETA(DisplayName = "Free"),
	Orbit      UMETA(DisplayName = "Orbit"),
	Mounted   UMETA(DisplayName = "Mounted")
};

UENUM(BlueprintType)
enum ESplineRotation
{
	IntoPath      UMETA(DisplayName = "Into Path"),
	Default   UMETA(DisplayName = "Default"),
	Free   UMETA(DisplayName = "Free"),
};

USTRUCT(BlueprintType)
struct FReplaySocket
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	FString SocketName;

	UPROPERTY(BlueprintReadOnly)
	FString FriendlySocketName;
	
	FReplaySocket(FString NewSocketName, FString NewFriendlySocketName)
	{
		SocketName = NewSocketName;
		FriendlySocketName = NewFriendlySocketName;
	}

	FReplaySocket()
	{
		SocketName = "";
		FriendlySocketName = "NULL";
	}
};

USTRUCT(BlueprintType)
struct FReplaySubMesh
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(BlueprintReadOnly)
	FString FriendlyMeshName;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FReplaySocket> ReplaySockets;
	
	FReplaySubMesh(USkeletalMeshComponent* NewMesh, FString NewFriendlyMeshName, TArray<FReplaySocket> NewReplaySockets)
	{
		Mesh = NewMesh;
		FriendlyMeshName = NewFriendlyMeshName;
		ReplaySockets = NewReplaySockets;
	}

	FReplaySubMesh()
	{
		Mesh = nullptr;
		FriendlyMeshName = "NULL";
		ReplaySockets = {};
	}
};

UCLASS()
class READYORNOT_API AReplayController : public AReadyOrNotPlayerController
{
	GENERATED_BODY()

	public:

		/** Constructor */
		AReplayController();
	
		virtual void EscapeMenu() override;
		virtual void BeginPlay() override;
		virtual void Tick(float DeltaSeconds) override;
		virtual void SetupInputComponent() override;
		virtual void UpdateRotation(float DeltaTime) override;
		virtual ASpectatorPawn* SpawnSpectatorPawn() override;

public:

	// All possible selected actors.
	UPROPERTY(EditAnywhere)
	TArray<AActor*> SelectableActors;

	// Currently selected actor for spectating.
	UPROPERTY(BlueprintReadOnly)
	AActor* SelectedActor;

	// Selected actor index used for changing between actors.
	UPROPERTY(BlueprintReadOnly)
	int32 SelectedActorIndex = 0;

	// Called when a scrub is complete. Called back by DemoNetDriver.
	void OnScrubComplete(UWorld* World);

	// If the replay menu is open or not.
	UPROPERTY(BlueprintReadWrite)
	bool bIsReplayMenuOpen = true;

	// The replay spline actor.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AReplaySplineActor* ReplaySplineActor;

	// Adds a new spline point to the end of the path.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void AddSplinePoint(FVector Location, FRotator Rotation);

	// Removes a spline point at the given index.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void RemoveSplinePoint(int32 Index);

	// Clears all the spline points.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void ClearSplinePoints();

	// Starts following the spline path.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void BeginFollowingSpline();

	// Stops following the spline path.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	void StopFollowingSpline();

	// Gets an array of references to all the spline points.
	UFUNCTION(BlueprintCallable, Category = "Replays")
	TArray<FSplinePoint> GetSplinePoints();

	// Amount of time to be on the spline path.
	UPROPERTY(BlueprintReadWrite, Category = "Spline Path")
	float TotalSplineTime = 5;

	// Whether the spline path is being followed or not.
	UPROPERTY(BlueprintReadOnly, Category = "Spline Path")
	bool bIsFollowingSpline = false;

	// The elapsed time on the spline path.
	UPROPERTY(BlueprintReadOnly, Category = "Spline Path")
	float DeltaSplineTime = 0;

	// What state the spline rotation option is in.
	UPROPERTY(BlueprintReadWrite, Category = "Spline Path")
	TEnumAsByte<ESplineRotation> SplineRotationType = Default;

	
	// Set the paused state of the running replay to bDoPause. Returns new pause state.
	UFUNCTION(BlueprintCallable)
	bool SetPaused(bool bDoPause);

	// Set the paused state of the running replay to bDoPause. Returns new pause state. Additional argument for whether to change the state of the audio.
	UFUNCTION(BlueprintCallable)
	bool SetPausedState(bool bDoPause, bool bMuteAudio);

	// Gets the max number of seconds that were recorded in the current replay.
	UFUNCTION(BlueprintCallable)
	float GetCurrentReplayTotalTimeInSeconds() const;

	// Gets the second we are currently watching in the replay.
	UFUNCTION(BlueprintCallable)
	float GetCurrentReplayCurrentTimeInSeconds() const;

	// Jumps to the specified Second in the Replay we are watching.
	UFUNCTION(BlueprintCallable)
	void SetCurrentReplayTimeToSeconds(float Seconds);

	// Changes the play rate of the replay we are watching, enabling FastForward or SlowMotion.
	UFUNCTION(BlueprintCallable)
	void SetCurrentReplayPlayRate(float PlayRate = 1.f);

	// Sets the view override.
	UFUNCTION(BlueprintCallable)
	void SetViewOverride();

	// Switches to the next selectable actor.
	UFUNCTION(BlueprintCallable)
	void NextSelectableActor();

	// Switches to the previous selectable actor.
	UFUNCTION(BlueprintCallable)
	void PreviousSelectableActor();

	// Refreshes the list of the selectable actors.
	UFUNCTION(BlueprintCallable)
	void RefreshSelectableActors();

	UFUNCTION(BlueprintCallable)
	void OnFirstDynamicLoad();

	UFUNCTION(BlueprintCallable)
	void OnDynamicLoad();

	// This is called one tick after dynamic load to ensure that everything is rendered.
	UFUNCTION(BlueprintCallable)
	void OnPostDynamicLoad();

	// Called when the player changes their camera state.
	UFUNCTION(BlueprintCallable)
	void OnPlayerChangeCameraState(TEnumAsByte<ECameraState> NewState);

	// Called when the actor is changed.
	UFUNCTION(BlueprintCallable)
	void OnChangeSelectedActor();

	// Called when the scrub is initiated, used to cache values before data is wiped.
	UFUNCTION(BlueprintCallable)
	void OnScrubInitiated();

	// Reverts the previous camera states' effects.
	UFUNCTION(BlueprintCallable)
	void RevertPreviousCameraState();

	// Applies the new camera state.
	UFUNCTION(BlueprintCallable)
	void ApplyNewCameraState();

	// Reapplies the current camera state.
	UFUNCTION(BlueprintCallable)
	void ApplyCameraState();

	// Gets all the players in the world.
	UFUNCTION(BlueprintCallable)
	TArray<APlayerCharacter*> GetAllPlayers();

	// Gets all the SWAI ai in the world.
	UFUNCTION(BlueprintCallable)
	TArray<ASWATCharacter*> GetAllSwatAI();

	// Gets all the suspect ai in the world.
	UFUNCTION(BlueprintCallable)
	TArray<ASuspectCharacter*> GetAllSuspectAI();

	// Gets all the civilian ai in the world.
	UFUNCTION(BlueprintCallable)
	TArray<ACivilianCharacter*> GetAllCivilianAI();

	// The replay controls UI instance.
	UPROPERTY(BlueprintReadOnly)
	UReplayControls* ReplayControls;

	// The current camera state, freecam, orbit, mounted, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECameraState> CurrentCameraState;

	// Vertical offset used for
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AdjustableVerticalOffset = 0;

	// Gets the actor name based on what type it is.
	UFUNCTION(BlueprintCallable)
	FString GetActorName(AActor* Actor);

	// Whether the mounted transform is to be locked.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	bool bMountedTransformLock = true;

	// The mesh being mounted on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	USkeletalMeshComponent* MountedMesh = nullptr;

	// The mounted bone name.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	FString MountedBoneName;

	// All submeshes of the mounted actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	TArray<FReplaySubMesh> MountedSubMeshes;

	// The mounted location offset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	FVector MountedLocationOffset;

	// The mounted rotation offset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Mounted")
	FRotator MountedRotationOffset;

	// Creates the mount data for each type of actor.
	UFUNCTION(BlueprintCallable, Category = "Camera | Mounted")
	void CreateMountData();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPauseMenuClosed();

private:
	/* Returns true if all actors associated with the scrub have been found.*/
	bool VerifyScrubComplete();

	/* Update location if the spectator is orbiting */
	void UpdateOrbitTransform();

	/* Update location if the spectator is mounted*/
	void UpdateMountedTransform();

	// If dynamically loaded actors need to be verified.
	bool bTryVerifyDynamicLoaded = true;

	// If we should pause after a scrub, used to preserve a state from before the scrub.
	bool bShouldPauseAfterScrub = false;

	// If post dynamic load should be called.
	bool bShouldCallPostDynamicLoad = false;

	// If we have already paused after scrub.
	bool bHasPausedAfterScrub = true;

	// Number of dynamic actors we use to verify that a dynamic load has completed, at least most of the load is finished,
	int32 NumberTrackedDynamics = 0;

	// Wrapper function for pausing the replay.
	UFUNCTION()
	void PauseReplay();

	// Wrapper function for skipping the replay forward.
	UFUNCTION()
	void SkipReplayForward();

	// Wrapper function for skipping the replay backward.
	UFUNCTION()
	void SkipReplayBackward();

	// Wrapper function for changing to the next actor.
	UFUNCTION()
	void NextActor();

	// Wrapper function for changing to the previous actor.
	UFUNCTION()
	void PreviousActor();

	// Wrapper function for toggling the replay HUD.
	UFUNCTION()
	void ToggleHUD();

	// If the game is paused when the menu pause is pressed, used to preserve state.
	bool bIsPausedOnMenuPause = false;

	// Whether the replay is currently paused or not.
	bool bIsPaused = false;

	// The previous anti-aliasing setting when paused.
	int32 PreviousAASetting;

	// The previous --- setting when paused.
	int32 PreviousMBSetting;

	// Instance of the escape menu widget.
	UPROPERTY()
	UUserWidget* EscapeWidgetInstance;

	// Fixes doors in the replay to prevent visual issues.
	void FixDoors(UWorld* World);
	
	// Fixes ragdolls in the replay to prevent visual issues.
	void FixRagdolls(UWorld* World);

	// Cached index for what mesh is selected.
	int CachedFirstMountedIndex = 0;

	// Cached index for what bone is selected.
	int CachedSecondMountedIndex = 0;

	// Cached mounted offset.
	FVector CachedMountedOffset;

	// Cached mounted rotational offset.
	FRotator CachedMountedRotation;

	// Cached arm length for the ReplaySpringArm.
	float CachedArmLength = 300;
};
