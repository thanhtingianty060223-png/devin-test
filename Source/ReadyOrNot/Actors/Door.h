// Copyright Void Interactive, 2023

#pragma once

#include "DoorwayWithoutDoor.h"
#include "Components/FMODAudioPropagationComponent.h"
#include "Interfaces/CanPlaceC2On.h"
#include "Interfaces/CanIssueCommandOn.h"
#include "Interfaces/CanUseMultitoolOn.h"
#include "Interfaces/UseabilityInterface.h"
#include "Interfaces/GatherDebugText.h"
#include "Interfaces/ReceiveAISenseUpdates.h"
#include "Structs.h"
#include "Enums.h"
#include "Door.generated.h"

DECLARE_STATS_GROUP(TEXT("Door"), STATGROUP_Door, STATCAT_Advanced);

UENUM()
enum class EDoorOneWayDirection : uint8
{
	Forward,
	Backward
};

UENUM()
enum class EDoorTrapSide : uint8
{
	Front,
	Back
};

UENUM()
enum class ESubDoorPosition : uint8
{
	None,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EDoorDamageType : uint8
{
	DDT_None,
	DDT_Blasting,		// C2
	DDT_Shotgunning,	// Breaching Shotgun
	DDT_Ramming,		// Battering Ram
	DDT_Kicking,		// Kick
	DDT_Bash			// Body ram
};

static FString DoorDamageTypeToString(const EDoorDamageType& InDoorDamageType)
{
	switch (InDoorDamageType)
	{
		case EDoorDamageType::DDT_None:			return "None";
		case EDoorDamageType::DDT_Blasting:		return "Blasting";
		case EDoorDamageType::DDT_Shotgunning:	return "Shotgunning";
		case EDoorDamageType::DDT_Ramming:		return "Ramming";
		case EDoorDamageType::DDT_Kicking:		return "Kicking";
		default: return "None";
	}
}

UENUM(BlueprintType)
enum class EDoorInteraction : uint8
{
	None,
    Open,
    Close,
    Peek,
    Push,
    Kick,
    Kick_Fail,
	Ram
};

UENUM(BlueprintType)
enum class ETrapSetup : uint8
{
	Automatic,
	Manual
};

USTRUCT(BlueprintType)
struct FDoorChunkData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	class UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere)
	bool bIsHinge = false;

	UPROPERTY(EditAnywhere)
	bool bIsDoorHandleChunk = false;

	UPROPERTY(EditAnywhere)
	bool bCannotKickIfDestroyed = false;
	
	// The chunks that this chunk is supporting
	UPROPERTY(EditAnywhere)
	TArray<int32> SupportChunks;
};

USTRUCT(BlueprintType)
struct FOutStackupData
{
	GENERATED_USTRUCT_BODY()

	EStackupGenArea Area;
	
	TArray<FVector> Locations;

	FOutStackupData()
	{
		Area = EStackupGenArea::SGA_None;
		Locations = {};
	}
};

USTRUCT(BlueprintType)
struct FDoorData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	FDoorData()
	{
		bIsDestructible = false;
		bCanMirrorUnderDoor = true;
		bDoorHandleFront = true;
		bDoorHandleBack = true;
		bCustomLockpickLocation = false;
		bCustomDoorPeekLocation = false;
		bLockable = true;
	}

	UPROPERTY(EditAnywhere)
	float DoorMaxOpenClose = 90.0f;
	
	UPROPERTY(EditAnywhere)
	uint8 bIsDestructible : 1;
	
	UPROPERTY(EditAnywhere)
	uint8 bCanMirrorUnderDoor : 1;
	
	UPROPERTY(EditAnywhere)
	uint8 bCanBreakOffWithKick : 1;
	
	UPROPERTY(EditAnywhere)
	uint8 bCanBreakOffOneWayDoorWithKick : 1;

	// Note: All doors must use exactly 9 chunks
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bIsDestructible"))
	TArray<FDoorChunkData> DestructibleChunks;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "!bIsDestructible"))
	UStaticMesh* DoorMesh = nullptr;

	UPROPERTY(EditAnywhere)
	UStaticMesh* DoorHandle = nullptr;

	// The offset from the door's origin to the doorway's origin
	UPROPERTY(EditAnywhere)
	FVector DoorwayOffset = FVector::ZeroVector;

	// The amount to scale the doorway offset by
	UPROPERTY(EditAnywhere)
	FVector DoorwayOffsetScale = FVector::OneVector;

	// Whether the door can be locked (not all doors can be)
	UPROPERTY(EditAnywhere)
	uint8 bLockable : 1;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bLockable"))
	uint8 bCustomLockpickLocation : 1;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bLockable && bCustomLockpickLocation"))
	FVector LockpickRelativeLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere)
	uint8 bCustomDoorPeekLocation : 1;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bCustomDoorPeekLocation"))
	FVector DoorPeekRelativeLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere)
	uint8 bDoorHandleFront : 1;
	
	UPROPERTY(EditAnywhere)
	uint8 bDoorHandleBack : 1;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDoorHandleFront"))
	FTransform DoorHandleFrontRelativeTransform;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDoorHandleBack"))
	FTransform DoorHandleBackRelativeTransform;
	
	UPROPERTY(EditAnywhere, Category = "Kicking")
	int32 NumSuccessfulKicksToBreakDown = 1;
	
	UPROPERTY(EditAnywhere, Category = "Kicking")
	float DoorKickSuccessChance = 0.7f;

	// Particle system to use when a door is kicked
	UPROPERTY(EditAnywhere, Category = "Kicking")
	FTransform KickParticleTransform = FTransform(FVector(0.0f, 60.0f, 0.0f));
	
	// Override particle system used when a door is kicked
	UPROPERTY(EditAnywhere, Category = "Kicking")
	TSoftObjectPtr<UParticleSystem> KickedParticleSystem = TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXDestruction/P_Destruction_DoorKick.P_Destruction_DoorKick'")));

	// Override particle system used when a door is kicked and its lock is broken
	UPROPERTY(EditAnywhere, Category = "Kicking")
	TSoftObjectPtr<UParticleSystem> LockBrokenParticleSystem = TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("ParticleSystem'/Game/ReadyOrNot/VFX/VFXDestruction/P_Destruction_LockBreak_Wood.P_Destruction_LockBreak_Wood'")));
	
	UPROPERTY(EditAnywhere, Category = "C2")
	FVector C2PlacementPoint_Front;
	
	UPROPERTY(EditAnywhere, Category = "C2")
	FVector C2PlacementPoint_Back;

	// Which particle effect gets spawned when this door is blown open by C2
	UPROPERTY(EditAnywhere, Category = "C2")
	UParticleSystem* C2ExplosionParticle = nullptr;

	// Which FMOD event to play when C2 is detonated on this door
	UPROPERTY(EditAnywhere, Category = "C2")
	UFMODEvent* C2ExplosionEvent = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* OpenSound = nullptr;
    
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* CloseSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* PushOpenSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* PushCloseSound = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* ManipulateSound = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* BashSound = nullptr;

	// Sound that is played when trying to open a locked door
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* LockedSound = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* RamSound = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* KickSuccessSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* KickFailSound = nullptr;

	// Sound that is played when this door's alarm is triggered, if electronic door
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* AlarmSound = nullptr;

	// Sound that is played when this door is opened with a keycard, if electronic door
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* KeycardSound = nullptr;

	// Sound that is played when attempting to open this door without a keycard, if electronic door
	UPROPERTY(EditAnywhere, Category = "Sound")
	UFMODEvent* KeycardDenySound = nullptr;
};

USTRUCT(BlueprintType)
struct FTrapData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FTransform TrapRelativeTransform;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ATrapActorAttachedToDoor> TrapClass;

	// if true will attach to door instead of door way
	UPROPERTY(EditAnywhere)
	bool bAttachToDoor = false;

	UPROPERTY(EditAnywhere)
	ETrapType TrapType = ETrapType::Unknown;

	UPROPERTY(EditAnywhere)
	float InvertOffset = 5.0f;
};

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "LOD", "Cooking"), ShowCategories = ("Actor"))
class READYORNOT_API ADoor final : public ADoorwayWithoutDoor, public ICanIssueCommandOn, public ICanPlaceC2On,
							public ICanUseMultitoolOn, public IUseabilityInterface, public IGatherDebugInterface,
							public IReceiveAISenseUpdates
{
	GENERATED_BODY()

public:
	ADoor();

	static const FName MOVE_DOOR_NOISE_TAG;
	static const FName KICK_DOOR_NOISE_TAG;
	static const FName EXPLODE_DOOR_NOISE_TAG;
	static const FName RAM_DOOR_NOISE_TAG;

	bool IsPointsOnOppositeSideOfDoor(FVector A, FVector B) const;

#if WITH_EDITOR
	virtual void ClearCrossLevelReferences() override;
#endif
	
	FORCEINLINE UStaticMeshComponent* GetDoorMesh() const { return DoorStatic; }
	FORCEINLINE UStaticMeshComponent* GetDoorHandleFront() const { return DoorHandleFront; }
	FORCEINLINE UStaticMeshComponent* GetDoorHandleBack() const { return DoorHandleBack; }
	FORCEINLINE USceneComponent* GetLockpickHighlight() const { return LockpickArea; }
	FORCEINLINE USceneComponent* GetDoorHighlight() const { return DoorArea; }
	FORCEINLINE USceneComponent* GetC2Highlight() const { return C2Area; }
	FORCEINLINE USceneComponent* GetBSGHighlight() const { return BSGArea; }
	FORCEINLINE USceneComponent* GetMirrorAimHighlight() const { return MirrorgunArea; }
	FORCEINLINE USceneComponent* GetWedgeHighlight() const { return WedgeArea; }
	FORCEINLINE class UMirrorPortalComponent* GetFrontMirrorPoint() const { return FrontMirrorPoint; }
	FORCEINLINE class UMirrorPortalComponent* GetBackMirrorPoint() const { return BackMirrorPoint; }

	FORCEINLINE UInteractableComponent* GetOpenInteractableComponent() const { return DoorOpenInteractableComp; }
	FORCEINLINE UInteractableComponent* GetPushInteractableComponent() const { return DoorPushInteractableComp; }
	FORCEINLINE UInteractableComponent* GetBSGInteractableComponent() const { return BSGInteractableComponent; }
	FORCEINLINE UInteractableComponent* GetBSGInteractableComponent_2() const { return BSGInteractableComponent_2; }
	FORCEINLINE UInteractableComponent* GetLockpickInteractableComponent() const { return LockpickInteractableComponent; }
	FORCEINLINE UInteractableComponent* GetBSGHighlight_Upper() const { return BSGInteractableComponent; }
	FORCEINLINE UInteractableComponent* GetBSGHighlight_Lower() const { return BSGInteractableComponent_2; }
	FORCEINLINE UInteractableComponent* GetOptiwandInteractableComponent() const { return OptiwandInteractableComponent; }
	FORCEINLINE UInteractableComponent* GetKickInteractableComponent() const { return DoorKickInteractableComp; }
	FORCEINLINE UInteractableComponent* GetC2InteractableComponent() const { return C2InteractableComponent; }
	FORCEINLINE UInteractableComponent* GetRamInteractableComponent() const { return DoorRamInteractableComponent; }
	FORCEINLINE UInteractableComponent* GetWedgeInteractableComponent() const { return WedgeInteractableComponent; }

	FORCEINLINE TArray<class UDestructibleDoorChunkComponent*> GetChunkComponents() const { return ChunkComponents; }
	FORCEINLINE UParticleSystem* GetC2ExplosionParticle() const { return DoorData.C2ExplosionParticle; }
	FORCEINLINE UFMODEvent* GetC2DetonationEvent() const { return DoorData.C2ExplosionEvent; }

	bool bForceClosedDoorNavArea = false;
	
	UFUNCTION(CallInEditor, Category = "Tools")
	void BlockAllDoorways();
	UFUNCTION(CallInEditor, Category = "Tools")
	void UnblockAllDoorways();
	void ActivateDoorBlocker();
	UFUNCTION(CallInEditor, Category = "Tools")
	void ActivateDoorBlockerForWorldGen();
	void ActivateDoorBlocker_Trap();
	UFUNCTION(CallInEditor, Category = "Tools")
	void DeactivateDoorBlocker();
	
	void ActivateDoorBlocker(bool bForce);

	void SetDoorBlockerAreaClass(TSubclassOf<UNavAreaBase> NewNavArea);

	void ActivateBreachBlockers(bool bFront);
	void DeactivateBreachBlockers();
	
	UFUNCTION(CallInEditor, Category = "Tools")
	void ToggleLightBlocker();

	UFUNCTION(BlueprintCallable)
	void Setup();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Navigation")
    void EnableNavLink();

	void SetSubDoor(ADoor* InDoor, bool bInMainDoor);

	FVector NavLinkStart;

	bool bSearchingPath = false;
	float TimeUntilNextPathTest = 2.0f;
	float LastTestedOpenCloseAmount = 0.0f;
	void TestCanPathBothSidesOfDoor();
	void OnTestCanPathBothSidesOfDoor(uint32 PathId, const ENavigationQueryResult::Type ResultType, const FNavPathSharedPtr NavPath);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Navigation")
    void DisableNavLink();

	void UpdateNavLinkLocations();

	UFUNCTION()
	void DestroyNavLink();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Interaction")
	void DisableAllInteractables();

	UFUNCTION(NetMulticast, Reliable, Category = "Door|Interaction")
	void Multicast_DisableDoorInteraction(bool bSetClosed = false);
	
	void OpenDoorFullyInstantly(AReadyOrNotCharacter* DoorOpenCharacter);
	void OpenDoorFullyInstantly(bool bForward);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    float OpenDoor(AReadyOrNotCharacter* DoorOpenCharacter, bool bInstant = false, bool bAnimateDoorHandle = true, const bool bNoCloseThreshold = false);

	UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenDoor_SpecificAngle(AReadyOrNotCharacter* DoorOpenCharacter, float CustomTargetAngle, bool bInstant = false, bool bAnimateDoorHandle = true);

	UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenSubDoor(AReadyOrNotCharacter* DoorOpenCharacter, bool bInstant = false, bool bAnimateDoorHandle = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseSubDoor(AReadyOrNotCharacter* DoorCloseCharacter, bool bInstant = false, bool bAnimateDoorHandle = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
	void OpenDoor_Debug();

	UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoor_Debug();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Restore();
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoor(AReadyOrNotCharacter* DoorCloserCharacter, bool bInstant = false, bool bAnimateDoorHandle = true);

	UFUNCTION(BlueprintCallable, Category = "Door")
    float PushDoor(AReadyOrNotCharacter* DoorPusherCharacter, float InIncrementAngle, bool bAnimateDoorHandle = true, bool bPlaySound = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void PushDoor_SpecificAngle(AReadyOrNotCharacter* DoorPusherCharacter, float CustomTargetAngle, bool bAnimateDoorHandle = true);

	UFUNCTION(BlueprintCallable, Category = "Door")
    float PeekDoor(AReadyOrNotCharacter* DoorPeekerCharacter, float InIncrementAngle, bool bAnimateDoorHandle = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    float RamDoor(AReadyOrNotCharacter* DoorRamCharacter, bool bPlayRamSound = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    float BodyRamDoor(AReadyOrNotCharacter* DoorRamCharacter);

	UFUNCTION(BlueprintCallable, Category = "Door")
    void BreachDoor(AReadyOrNotCharacter* DoorBreacherCharacter, float InIncrementAngle);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void BreachDoorFromPoint(AReadyOrNotCharacter* DoorBreacherCharacter, FVector BreachPoint, float InIncrementAngle);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void ExplodeDoor(AReadyOrNotCharacter* DoorBreacherCharacter, AActor* ExplosionCauser, bool bKeepHinges = false);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ExplodeDoor(AReadyOrNotCharacter* DoorBreacherCharacter, AActor* ExplosionCauser, bool bKeepHinges = false);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void CollapseDoor(AReadyOrNotCharacter* DoorBreacherCharacter, FVector BreachLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void KickDoor(AReadyOrNotCharacter* DoorKickCharacter, bool bKickSubDoor = false, bool bForce = false);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void KickSubDoor(AReadyOrNotCharacter* DoorKickCharacter);

	UFUNCTION(BlueprintCallable, Category = "Door")
    void BreakDoor(bool bDestroyAllChunks = true, AReadyOrNotCharacter* DoorBreakerCharacter = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void BreakAndDetachDoor(bool bDestroyAllChunks = true, AReadyOrNotCharacter* DoorBreakerCharacter = nullptr, float Impulse = 15500.0f, float ForwardOffset = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void ApplyDoorDamage(EDoorDamageType InDoorDamage, AReadyOrNotCharacter* Victim);
	
	UFUNCTION(BlueprintCallable, Category = "Door")
    void BreakDoorHandles();

	UFUNCTION(BlueprintCallable, Category = "Door|Interaction")
	void DestroyChunk_Index(int32 ChunkIndex, const FVector& Impulse, float ImpulseStrength = 2500.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Interaction")
	void DestroyChunk(UDestructibleDoorChunkComponent* InChunk, const FVector& Impulse, float ImpulseStrength = 2500.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Interaction")
	void DestroyAllChunks(const FVector& Impulse, float ImpulseStrength = 2500.0f, bool bKeepHinges = false);

	void DestroyAllChunkComponents();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Trap")
    void AttachTrap(class ATrapActorAttachedToDoor* NewTrap, bool bAttachToDoor = true);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Wedge")
    void AttachWedge(class ADoorJam* NewWedge);

	UFUNCTION(BlueprintCallable, Category = "Door|Wedge")
	void RemoveWedges();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Kick")
    void DecreaseNumKicksToBreakDown(AReadyOrNotCharacter* DoorKickCharacter, bool& bShouldOpenDoor, bool& bCanBreakLock, float KickChanceOffset = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Damage")
    void ApplyRandomDamageToChunks(float MinDamage = 0.0f, float MaxDamage = 1500.0f);

	UFUNCTION(BlueprintCallable, Category = "Door|Trap")
	void SetDoorTrapKnowledge(bool bSuspectTeam, bool bKnowledge);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Trap")
    void ResetDoorTrapKnowledge();

	UFUNCTION(BlueprintCallable, Category = "Door|Lock")
    void SetDoorLockKnowledge(bool bSuspectTeam, bool bKnowledge);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Lock")
    void ResetDoorLockKnowledge();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Lock")
	void LockDoor(bool bLockSubDoor = true);

	// may help fix locked/unlock desync by forcing to replicate
	UFUNCTION(NetMulticast, Reliable, Category = "Door|Lock")
	void Multicast_SetLocked(bool bShouldLocked);
	
	UFUNCTION(BlueprintCallable, Category = "Door|Lock")
	void UnlockDoor(bool bUnlockSubDoor = true);

	UFUNCTION(BlueprintCallable, Category = "Door|Lock")
    void SetLocked(bool bNewLocked);
	
	UFUNCTION(BlueprintPure, Category = "Door|Lock")
	bool IsLocked() const;

	UFUNCTION(BlueprintPure, Category = "Door|Flee")
	bool IsIgnoredForFlee();
	
	UFUNCTION(BlueprintPure, Category = "Door|Lock")
	bool IsLockable() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Jam")
    bool IsJammed() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsDoorwayOnly() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsMirrorBlocked() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsSubDoorOpen();

	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsClosed() const;
    	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen_Forward() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen_Backward() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsFullyOpen_Forward() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsFullyOpen_Backward() const;
		
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsFullyOpen() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsHalfwayOpen() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenBeyondIncrementThreshold() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenBeyondCloseThreshold() const;
	
	// Percentage = Value from 0 to 1
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenBy(float Percentage) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsOpenBy_Angle(float Angle) const;
	
	// Percentage = Value from 0 to 1
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenBeyond(float Percentage) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenAtOrBeyond(float Percentage) const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenBeyond_Angle(float Angle) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpenAtOrBeyond_Angle(float Angle) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsAttachedToRoot() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool CanPushDoor(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool CanPeekDoor(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool CanPullDoor(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool SubDoor_CanOpenDoors(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool SubDoor_CanCloseDoors(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool MainSubDoor_CanShowOpenDoorPrompt() const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool NonMainSubDoor_CanShowOpenDoorPrompt() const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsActorInFrontOfDoor(AActor* Actor) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsActorInFrontOfDoorway(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsActorRightOfDoorway(AActor* Actor) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsActorBehindDoor_Relative(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsPointInFrontOfDoor(FVector Vector) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool IsPointRightOfDoorway(FVector Vector) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanKickDoor(AReadyOrNotCharacter* PlayerCharacter = nullptr) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanOpenDoor(AReadyOrNotCharacter* PlayerCharacter = nullptr) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanCloseDoor(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanMirrorUnderDoor(AReadyOrNotCharacter* PlayerCharacter) const;
    
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipMultitool(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipC2Explosive(AReadyOrNotCharacter* PlayerCharacter) const;
    
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipWedge(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipOptiwand(AReadyOrNotCharacter* PlayerCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipBreachingShotgun(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanEquipBatteringRam(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanRamDoor(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanLockpickDoor(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanPlaceC2Explosive(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanDeployWedge(AReadyOrNotCharacter* PlayerCharacter) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
    bool CanPushDoorWhileBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsTooFarForKick() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Damage")
	bool CanTakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const;

	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AnyChunksDestroyed() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool IsMiddleChunkBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AllMajorDoorChunksDestroyed() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool IsDoorChunkDestroyed(UDestructibleDoorChunkComponent* InChunkComponent) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AllBottomDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AnyBottomDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AllMiddleDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AnyMiddleDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AllTopDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Chunks")
    bool AnyTopDoorChunksBroken() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	bool AnyHingesLeft() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
	bool CanSpawnTrap() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Lock")
    bool TeamKnowsDoorLockState(bool bSuspectTeam) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
    bool TeamKnowsDoorTrapState(bool bSuspectTeam) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
	bool IsLocationSameSideAsTrap(FVector InLocation) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
	bool IsActorSameSideAsTrap(AActor* InActor) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
	bool IsTrapLive() const;

	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	float GetOpenAmountAsPercentage() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsStackUpDisabled(FVector CommandLocation) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|UI")
	bool IsOutlineEnabled(EActorOutlineType OutlineType) const;
	
	UFUNCTION(BlueprintPure, Category = "Door|UI")
	bool IsOutlineDisabled() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	bool IsAnyAIOpening() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	bool IsAnyAIClosing() const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsElectronicDoor() const { return bIsElectronicDoor; }

	UFUNCTION(BlueprintPure, Category = "Door|Lock")
	FORCEINLINE bool GetSWATKnowsLockState() const { return bSWATKnowsLockState; }

	UFUNCTION(BlueprintPure, Category = "Door|Lock")
	FORCEINLINE bool GetSuspectKnowsLockState() const { return bSuspectKnowsLockState; }

	UFUNCTION(BlueprintPure, Category = "Door|Trap")
    FORCEINLINE bool DoesSWATKnowTrapState() const { return bSWATKnowsTrapState; }

	UFUNCTION(BlueprintPure, Category = "Door|Trap")
    FORCEINLINE bool DoesSuspectKnowTrapState() const { return bSuspectKnowsTrapState; }
    
	UFUNCTION(BlueprintPure, Category = "Door|Trap")
    FORCEINLINE class ATrapActorAttachedToDoor* GetAttachedTrap() const { return AttachedTrap; }

	UFUNCTION(BlueprintPure, Category = "Door|Trap")
	FORCEINLINE bool HasTrapAndSWATKnowsTrap() const { return AttachedTrap && bSWATKnowsTrapState; }

	UFUNCTION(BlueprintPure, Category = "Door|Trap")
    FORCEINLINE bool HasTrapAndSuspectKnowsTrap() const { return AttachedTrap && bSuspectKnowsTrapState; }

	UFUNCTION(BlueprintPure, Category = "Door|Wedge")
    FORCEINLINE class ADoorJam* GetAttachedWedge() const { return AttachedWedge; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE bool IsPendingSubDoorKick() const { return bPendingSubDoorKick; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE bool HasEverBeenOpened() const { return bHasEverBeenOpenedBySwat; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE bool IsDestructible() const { return DoorData.bIsDestructible; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
    FORCEINLINE bool IsMainSubdoor() const { return bMainSubDoor && DriveSubDoor; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
    FORCEINLINE bool IsNonMainSubdoor() const { return !bMainSubDoor && DriveSubDoor; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
    FORCEINLINE bool IsLockChanceOverridden() const { return bOverrideLockChance || (DriveSubDoor ? DriveSubDoor->bOverrideLockChance : false); }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE float GetIncrementAngle() const { return IncrementAngle; }

	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsAnyInteractionPlaying() const { return TL_DoorOpenClose.IsPlaying() || TL_DoorPush.IsPlaying() || TL_DoorRam.IsPlaying() || TL_DoorExplode.IsPlaying() || TL_DoorKickSuccess.IsPlaying() || TL_DoorKickFail.IsPlaying() || TL_DoorLocked.IsPlaying(); }
	
	FORCEINLINE bool IsAnyInteractionPlayingExcept(const FTimeline& InTimeline) const { return !InTimeline.IsPlaying() && (TL_DoorOpenClose.IsPlaying() || TL_DoorPush.IsPlaying() || TL_DoorRam.IsPlaying() || TL_DoorExplode.IsPlaying() || TL_DoorKickSuccess.IsPlaying() || TL_DoorKickFail.IsPlaying() || TL_DoorLocked.IsPlaying()); }

	// Open, close, push, peak, locked
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsDoorInteractionPlaying() const { return TL_DoorOpenClose.IsPlaying() || TL_DoorPush.IsPlaying() || TL_DoorLocked.IsPlaying(); }

	// Kick, ram, explode
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsNonDoorInteractionPlaying() const { return TL_DoorRam.IsPlaying() || TL_DoorExplode.IsPlaying() || TL_DoorKickSuccess.IsPlaying() || TL_DoorKickFail.IsPlaying(); }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsOpening() const { return TL_DoorOpenClose.IsPlaying(); }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsClosing() const { return IsOpening() && LastTargetAngle == 0.0f; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetStartingOpenAngle() const { return StartingOpenAngle; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetOpenAmount() const { return OpenCloseAmount; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetMaxOpenAmount() const { return MaxOpenClose; }

	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsC2Placed() const { return bC2Placed; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsAlwaysLocked() const { return bAlwaysLocked; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE bool IsOverridingLockChance() const { return bOverrideLockChance; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	FORCEINLINE float GetOverrideLockChance() const { return LockedChance; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetOpenThreshold() const { return OpenThreshold; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetTargetAngle() const { return LastTargetAngle; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
    FORCEINLINE float GetPseudoVelocity() const { return FMath::Abs(DistanceMovedThisFrame/TimeMovedThisFrame); }

	UFUNCTION(BlueprintPure, Category = "Door|Damage")
    FORCEINLINE bool IsDoorBroken() const { return bDoorBroken; }

	UFUNCTION(BlueprintPure, Category = "Door|Damage")
	FORCEINLINE bool IsHandleBroken() const { return bDoorHandlesBroken; }

	UFUNCTION(BlueprintPure, Category = "Door|Damage")
	FORCEINLINE TMap<EDoorDamageType, float> GetDoorKillDistance() const { return DoorKillDistance; }
	
	UFUNCTION(BlueprintPure, Category = "Door|Damage")
    FORCEINLINE TMap<EDoorDamageType, float> GetDoorStunDistance() const { return DoorStunDistance; }

	UFUNCTION(BlueprintPure, Category = "Door")
    FORCEINLINE ADoor* GetSubDoor() const { return DriveSubDoor; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
    FORCEINLINE AReadyOrNotCharacter* GetLastDoorUser() const { return LastDoorUser; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE FName GetTypeOfDoorRow() { return TypeOfDoor.RowName; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FORCEINLINE FName GetTypeOfDoorTrap() { return TypeOfTrap.RowName; }

	UFUNCTION(BlueprintPure, Category = "Door|C2")
	FORCEINLINE APlacedC2Explosive* GetPlacedC2() const { return PlacedC2; }
	
	UFUNCTION(BlueprintPure, Category = "Door")
	TArray<class AStackUpActor*> GetStackupsForArea(EStackupGenArea StackupArea) const;

	UFUNCTION(BlueprintPure, Category = "Door")
	FName GetFrontThreatOwningRoom() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	FName GetBackThreatOwningRoom() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FVector GetDoorMidLocation() const;
	
	UFUNCTION(BlueprintPure, Category = "Door|Wedge")
    FVector GetWedgeLocation() const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FVector GetBestDoorInteraction_FromLocation(const FVector& InInteractionLocation, bool bDoorwayBased = true) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	FVector GetBestDoorInteraction_FromStackUpArea(const EStackupGenArea& InStackUpArea, bool bDoorwayBased = true) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	EStackupGenArea FindStackUpAreaFromLocation(const FVector& InInteractionLocation) const;
	
	UFUNCTION(BlueprintPure, Category = "Door")
	static void FlipStackUpArea(EStackupGenArea& OutStackUpArea, bool bHorizontalFlip, bool bVerticalFlip);

	// Set lock state of all electronic locks in this level, used when player disables the electronic security box
	UFUNCTION(BlueprintCallable, Category = "Door", meta = (WorldContext = "WorldContextObject"))
	static void SetAllElectronicLocks(UObject* WorldContextObject, bool bLocked);

	// Set all electronic locks unlockable by SWAT in this level, used when player finds a door keycard
	UFUNCTION(BlueprintCallable, Category = "Door", meta = (WorldContext = "WorldContextObject"))
	static void SetSWATHasAllKeycards(UObject* WorldContextObject);

	UFUNCTION(NetMulticast, Reliable, Category = "Door")
    void Multicast_CheckSupports();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Door|Lock")
	void Server_SetLockKnowledgeState(bool bSuspectTeam, bool bNewKnowledgeState);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Door|Trap")
    void Server_SetTrapKnowledgeState(bool bSuspectTeam, bool bNewKnowledgeState);
	
	UFUNCTION(BlueprintCallable)
	FVector CalculateClosestPoint(FVector Location) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorOpened);
	UPROPERTY(BlueprintAssignable, Category = "Door")
    FOnDoorOpened OnDoorOpened;
	
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnDoorOpened OnSubDoorOpened;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorClosed);
	UPROPERTY(BlueprintAssignable, Category = "Door")
    FOnDoorClosed OnDoorClosed;
    
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorBroken);
	UPROPERTY(BlueprintAssignable, Category = "Door")
    FOnDoorBroken OnDoorBroken;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorMovementBlocked);
	UPROPERTY(BlueprintAssignable, Category = "Door")
    FOnDoorMovementBlocked OnDoorMovementBlocked;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoorExplode, ADoor*, Door, AReadyOrNotCharacter*, InstigatorCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnDoorExplode OnDoorExploded;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDoorKick, ADoor*, Door, AReadyOrNotCharacter*, InstigatorCharacter, bool, bSuccess);
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnDoorKick OnDoorKicked;

	// The type of door that is currently selected
	// Select the from a list of door styles from the DoorDataTable
	UPROPERTY(EditAnywhere, Replicated, Category = "Setup")
    FDataTableRowHandle TypeOfDoor;

	// Force a custom starting open angle for this door. Only works on non-doorway doors
	// The min and max angle is determined by the type of door selected
	// Note: This will override the "RandomlyOpenAtGameStart" setting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	float StartingOpenAngle = 0.0f;

	// If true, this door should only open in one direction, instead of bi-directional and away from the door user.
	// The OneWayDirection setting controls which direction the door would open in
	//
	// Note: Rarely enable this setting, only reserve this for specical doors like shipping containers, etc. as the AI are not
	// designed to handle doors that open one way due to navigation and robustness issues
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
    uint8 bOneWay : 1;

	// If OneWay is true, which direction should this door open in
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bOneWay && !bIsDoorway", EditConditionHides))
	EDoorOneWayDirection OneWayDirection = EDoorOneWayDirection::Forward;
	
	// Should this door be randomly opened at game start
	// Note: If StartingOpenAngle is other than 0.0 or if it's doorway only, this setting will have no effect
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bRandomlyOpenAtGameStart = true;
	
	// If true, suspects can open the door even when it is locked
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bSuspectAlwaysUnlocks = false;

	// If true, this door will be ignored for fleeing by the Suspect/Civilian AI
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bIgnoreForFlee = false;

	// Used for stack up point generation. Default = false
	//
	// If true, do not enable the door blocker so that adjacent doors/doorways
	// are able to generate stack up points that can flow into us. Useful for adjacent doorways
	//
	// If false, always enable the door blocker so that at generation-time, stack up points
	// will not flow/cross over into any adjacent doors/doorways
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bNoNavBlockerForGen = false;

	// Used by swat to determine whether they should automatically breach and clear using this door into the next room.
	// If true, swat will ignore this door when looking for rooms to automatically breach and clear
	// Note: Only works for doorways. Open doors don't count
	UPROPERTY(EditInstanceOnly, Category = "Setup", meta = (EditCondition = "bIsDoorway"))
	bool bNoAutomaticClearing = false;

	// If true, players can place C2 on this door
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bCanUseC2 = true;

	// If true, players can use a Breaching Shotgun on this door
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bCanUseBSG = true;

	// If true, players can use a Wedge on this door
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bCanUseWedge = true;

	// If true, players can use a Ram on this door
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bCanUseRam = true;

	// If true, players can use a Mirrorgun/Optiwand on this door
	UPROPERTY(EditAnywhere, Category = "Setup", meta = (EditCondition = "!bIsDoorway", EditConditionHides))
	bool bCanUseOptiwand = true;
	
	UPROPERTY(BlueprintReadWrite)
	uint8 bCanPlayerInteract : 1;

	// Whether or not we can issue orders on the front side of this door
	// Set this to false if we should not allow the player to command
	// the swat ai to issue orders on the front side of this door (red arrow)
	UPROPERTY(EditInstanceOnly, Category = "Generated")
    bool bCanIssueOrdersOnFrontSide = true;

	// Whether or not we can issue orders on the back side of this door
	// Set this to false if we should not allow the player to command
	// the swat ai to issue orders on the back side of this door (cyan arrow)
	UPROPERTY(EditInstanceOnly, Category = "Generated")
    bool bCanIssueOrdersOnBackSide = true;

	// Does this door have a frame? Assumed true by default. Double doors are always assumed to be framed
	// Baked-in at generation-time. Used by swat to determine whether they should perform a threshold assessment on this door (PIE, Center Check, etc.)
	// If a door does not have a frame, swat will skip threshold assessment as they can pretty clearly see the other side
	UPROPERTY(VisibleInstanceOnly, Category = "Generated")
	bool bHasFrame = true;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated|Threats")
	class AThreatAwarenessActor* FrontThreat = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated|Threats")
	class AThreatAwarenessActor* BackThreat = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated|Threats")
	TArray<class AThreatAwarenessActor*> FrontThreatAwarenessPoints;

	UPROPERTY(VisibleInstanceOnly, Category = "Generated|Threats")
	TArray<class AThreatAwarenessActor*> BackThreatAwarenessPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Stackup Points")
	TArray<class AStackUpActor*> FrontLeftStackUpPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Stackup Points")
	TArray<class AStackUpActor*> FrontRightStackUpPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Stackup Points")
	TArray<class AStackUpActor*> BackLeftStackUpPoints;
	
	UPROPERTY(EditInstanceOnly, Category = "Generated|Stackup Points")
	TArray<class AStackUpActor*> BackRightStackUpPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Points", meta=(MakeEditWidget=true))
	TArray<FClearPoint> FrontLeftClearPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Points", meta=(MakeEditWidget=true))
	TArray<FClearPoint> FrontRightClearPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Points", meta=(MakeEditWidget=true))
	TArray<FClearPoint> BackLeftClearPoints;

	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Points", meta=(MakeEditWidget=true))
	TArray<FClearPoint> BackRightClearPoints;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleInstanceOnly, Category = "Status (Read Only)")
	bool bIsDoorway = false;
	#endif
	
	// Is this door blocked by a wedge?
	UPROPERTY(VisibleInstanceOnly, Replicated, Transient, Category = "Status (Read Only)")
    uint8 bDoorJammed : 1;
	
	UPROPERTY(VisibleInstanceOnly, Replicated, Transient, Category = "Status (Read Only)")
	uint8 bLocked : 1;

	UPROPERTY(ReplicatedUsing = "OnRep_DestroyedChunkIdxChanged")
    TArray<int32> DestroyedChunkIdxs;

	UPROPERTY(BlueprintReadOnly)
	bool bHeldPushDoor = false;
	UPROPERTY(BlueprintReadOnly)
	float PushDoorHoldTime = 0.0f;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	void Init();

	void PlayLockedAnimation();
	void PlayHandleAnimation();
	
    void PlayLockedSound();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Reset() override;
	virtual void PostLoad() override;
	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual FVector GetTargetLocation(AActor* RequestedBy = nullptr) const override;

	virtual bool IsComponentRelevantForNavigation(UActorComponent* Component) const override;
	
#if WITH_EDITOR
	bool bFirstEditorTick = true;
	void EditorTick(float DeltaSeconds);
	
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	
	void DrawKillStunDistances(float DeltaTime);
#endif

public:
	void DrawClearPoints(TArray<AThreatAwarenessActor*> ClearPoints, FColor Color);
	void DrawClearPointsV2(TArray<FClearPoint> ClearPoints, float Lifetime = 0.15f);
	void DrawStackUpPoints(TArray<AStackUpActor*> StackUpPoints, FColor Color);
	
	UFUNCTION(CallInEditor, Category = "Tools")
	void CalculateRoomPositioning();
	UFUNCTION(CallInEditor, Category = "Tools")
	void GenerateStackUpPoints();
	UFUNCTION(CallInEditor, Category = "Tools")
	void GenerateClearPoints();

protected:
    void SetupDoor();

	void TriggerAlarm(AReadyOrNotCharacter* DoorInteractionInstigator);
	void CheckKeycards(AReadyOrNotCharacter* DoorInteractionInstigator);

public:
	// Baked-in at generation time. The front position of this door relative to the room this is placed in
	// Used by the world data generator to determine the type of stack up generation it should employ.
	// Center-fed rooms generate stack ups on the left and right side of the door, where as corner-fed rooms
	// generate stack ups on one side of the door depending on whether it's on a left or right corner of a room.
	// You should never have to manually change this but the option is there if desired.
	// After manually changing the position, regenerate the stack up points using the "Generate Stack Up Points" button on this door
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Generated|Room Position")
	EDoorRoomPosition FrontRoomPosition = EDoorRoomPosition::Center;
	
	// Baked-in at generation time. The back position of this door relative to the room this is placed in
	// Used by the world data generator to determine the type of stack up generation it should employ.
	// Center-fed rooms generate stack ups on the left and right side of the door, where as corner-fed rooms
	// generate stack ups on one side of the door depending on whether it's on a left or right corner of a room.
	// You should never have to manually change this but the option is there if desired.
	// After manually changing the position, regenerate the stack up points using the "Generate Stack Up Points" button on this door
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Generated|Room Position")
	EDoorRoomPosition BackRoomPosition = EDoorRoomPosition::Center;

	// If true, the world data generator will ignore this door for automatically determining the room positions
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Generated|Room Position")
	bool bManualRoomPositionSetup = false;

	void SetFrontRoomPosition(EDoorRoomPosition NewPosition);
	void SetBackRoomPosition(EDoorRoomPosition NewPosition);
	
    void SetupTrap();

	void SetElectronicallyLocked(bool bIsLocked);

	void SetTypeOfTrapRowName(FName InName) { TypeOfTrap.RowName = InName; }
	void SetTypeOfDoorRowName(FName InName) { TypeOfDoor.RowName = InName; }
	FName GetTypeOfTrapRowName() const { return TypeOfTrap.RowName; }

	float TimeUntilNextVizTest = 0.0f;
	float TimeSinceSwatLastSeenDoor = FLT_MAX;
	
	// The maximum horiziontal clearing distance for the front side of this door (red arrow)
	// Used at generation time to clamp the max width when generating clearing path.
	// Useful for when you need to narrow down the clearing path for very wide/large room
	// Distance values are in centimeters. 1000cm == 10m
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	float MaxFrontHorizontalClearingDistance = FLT_MAX;
	
	// The maximum horiziontal clearing distance for the back side of this door (cyan arrow)
	// Used at generation time to clamp the max width when generating clearing path.
	// Useful for when you need to narrow down the clearing path for very wide/large room
	// Distance values are in centimeters. 1000cm == 10m
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	float MaxBackHorizontalClearingDistance = FLT_MAX;

	// A bias from -1.0 to 1.0. Default = -0.25 to 0.5
	// Used by the world data generator when generating clear points for the front right side of this door.
	//
	// This setting is used to limit how much we should cross over to the other side of the door (in this case, the left side)
	// The default min (X) is -0.25. Increasing the min (in the positive direction) will bias the generation to the right side,
	// the higher the min, the more you cut off the threshold to the right. A bit of trial and error will get you the results you desire.
	//
	// The default max (Y) is 0.5. This is the max right threshold where the generation will clamp to. Increase this for wider clear points
	//
	// For example: A min (X) value of 0.0 will cut off everything to the front left side of this door.
	// If you go the opposite extreme and decrease the min to -1.0, then the left side is open up to consideration
	// when looking for clear points to generate. Never go this far as the results are nonsensical, but the freedom is there to tweak
	//
	// After tweaking these values, click the "Generate Clear Points" button to see the new generation result
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	FVector2D MaxFrontRightClearThreshold = FVector2D(-0.25f, 0.5f);
	
	// A bias from -1.0 to 1.0. Default = -0.25 to 0.5
	// Used by the world data generator when generating clear points for the front left side of this door.
	//
	// This setting is used to limit how much we should cross over to the other side of the door (in this case, the right side)
	// The default min (X) is -0.25. Increasing the min (in the positive direction) will bias the generation to the left side,
	// the higher the min, the more you cut off the threshold to the left. A bit of trial and error will get you the results you desire.
	//
	// The default max (Y) is 0.5. This is the max left threshold where the generation will clamp to. Increase this for wider clear points
	//
	// For example: A min (X) value of 0.0 will cut off everything to the front right side of this door.
	// If you go the opposite extreme and decrease the min to -1.0, then the right side is open up to consideration
	// when looking for clear points to generate. Never go this far as the results are nonsensical, but the freedom is there to tweak
	//
	// After tweaking these values, click the "Generate Clear Points" button to see the new generation result
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	FVector2D MaxFrontLeftClearThreshold = FVector2D(-0.25f, 0.5f);
	
	// A bias from -1.0 to 1.0. Default = -0.25 to 0.5
	// Used by the world data generator when generating clear points for the back right side of this door.
	//
	// This setting is used to limit how much we should cross over to the other side of the door (in this case, the left side)
	// The default min (X) is -0.25. Increasing the min (in the positive direction) will bias the generation to the right side,
	// the higher the min, the more you cut off the threshold to the right. A bit of trial and error will get you the results you desire.
	//
	// The default max (Y) is 0.5. This is the max right threshold where the generation will clamp to. Increase this for wider clear points
	//
	// For example: A min (X) value of 0.0 will cut off everything to the back left side of this door.
	// If you go the opposite extreme and decrease the min to -1.0, then the left side is open up to consideration
	// when looking for clear points to generate. Never go this far as the results are nonsensical, but the freedom is there to tweak
	//
	// After tweaking these values, click the "Generate Clear Points" button to see the new generation result
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	FVector2D MaxBackRightClearThreshold = FVector2D(-0.25f, 0.5f);

	// A bias from -1.0 to 1.0. Default = -0.25 to 0.5
	// Used by the world data generator when generating clear points for the back left side of this door.
	//
	// This setting is used to limit how much we should cross over to the other side of the door (in this case, the right side)
	// The default min (X) is -0.25. Increasing the min (in the positive direction) will bias the generation to the left side,
	// the higher the min, the more you cut off the threshold to the left. A bit of trial and error will get you the results you desire.
	//
	// The default max (Y) is 0.5. This is the max left threshold where the generation will clamp to. Increase this for wider clear points
	//
	// For example: A min (X) value of 0.0 will cut off everything to the back right side of this door.
	// If you go the opposite extreme and decrease the min to -1.0, then the right side is open up to consideration
	// when looking for clear points to generate. Never go this far as the results are nonsensical, but the freedom is there to tweak
	//
	// After tweaking these values, click the "Generate Clear Points" button to see the new generation result
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	FVector2D MaxBackLeftClearThreshold = FVector2D(-0.25f, 0.5f);

	// The maximum allowed clear points that can be generated for the front right path. Leave 0 for unlimited
	// Useful for when you dont want to manually set up the clear points but would still like to retain the automatic settings but limit how many should be generated
	// After changing this setting, click "Generate Clear Points" button
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	uint8 MaxFrontRightClearPoints = 0;
	
	// The maximum allowed clear points that can be generated for the front left path. Leave 0 for unlimited
	// Useful for when you dont want to manually set up the clear points but would still like to retain the automatic settings but limit how many should be generated
	// After changing this setting, click "Generate Clear Points" button
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	uint8 MaxFrontLeftClearPoints = 0;
	
	// The maximum allowed clear points that can be generated for the back right path. Leave 0 for unlimited
	// Useful for when you dont want to manually set up the clear points but would still like to retain the automatic settings but limit how many should be generated
	// After changing this setting, click "Generate Clear Points" button
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	uint8 MaxBackRightClearPoints = 0;
	
	// The maximum allowed clear points that can be generated for the back left path. Leave 0 for unlimited
	// Useful for when you dont want to manually set up the clear points but would still like to retain the automatic settings but limit how many should be generated
	// After changing this setting, click "Generate Clear Points" button
	// Note: This setting has no effect when "ManualClearPointSetup" is true
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings", meta = (EditCondition = "!bManualClearPointSetup"))
	uint8 MaxBackLeftClearPoints = 0;

	// If true, let the designer manually setup the clear points.
	// The world data generator will skip this door when trying to generate clear points
	// A good strategy would be to generate clear points on this door, then enable this setting, then modify the clear points as desired,
	// then with any subsequent generation of the world, this door will be skipped and the user modified clear points will remain unchanged
	UPROPERTY(EditInstanceOnly, Category = "Generated|Clear Point Gen Settings")
	bool bManualClearPointSetup = false;

protected:
	// ICanIssueCommandOn
	///////////////////////////////////
	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;
	///////////////////////////////////

	// IUseabilityInterface
	///////////////////////////////////
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InteractableComponent) override;
	virtual void EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual void Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InteractableComponent) override;
	virtual void EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InteractableComponent) override;
	virtual void OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteract_Implementation() const override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;
	virtual UInteractableComponent* GetInteractableComponent_Implementation() const override;
	///////////////////////////////////
	
	// ICanUseMultitoolOn
	///////////////////////////////////
	virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual void Client_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual float GetMultitoolUseTime_Implementation() override;
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	///////////////////////////////////

	// ICanPlaceC2On
	///////////////////////////////////
	virtual void C2StartPlacement_Implementation(class AC2Explosive* C2) override; // C2 has started being placed, we can interrupt this if need be
	virtual void C2StopPlacement_Implementation(class AC2Explosive* C2) override; // C2 has stopped being placed (either it has finished being placed or canceled)
	virtual void OnC2Detonated_Implementation(class APlacedC2Explosive* C2) override;
	virtual void OnC2Removed_Implementation(class APlacedC2Explosive* C2) override;
	virtual bool CanPlaceC2OnNow_Implementation(class APlayerCharacter* C2Owner, class AC2Explosive* C2, FHitResult Hit) override;
	virtual FVector GetPlacementLocation_Implementation(FHitResult TraceHit) override;
	virtual FRotator GetPlacementRotation_Implementation(FHitResult TraceHit) override;
	///////////////////////////////////

	// IGatherDebugInterface
	///////////////////////////////////
	virtual void GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData) override;
	///////////////////////////////////
	
	// IReceiveAISenseUpdates
	///////////////////////////////////
	virtual void OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	virtual void OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor) override;
	///////////////////////////////////

	void Multicast_CheckSupports_Implementation();
	
	void Server_SetLockKnowledgeState_Implementation(bool bSuspectTeam, bool bNewKnowledgeState);
	bool Server_SetLockKnowledgeState_Validate(bool bSuspectTeam, bool bNewKnowledgeState) { return true; }

	void Server_SetTrapKnowledgeState_Implementation(bool bSuspectTeam, bool bNewKnowledgeState);
	bool Server_SetTrapKnowledgeState_Validate(bool bSuspectTeam, bool bNewKnowledgeState) { return true; }
	
	UFUNCTION(BlueprintCallable, Category = "Door|Damage")
    void PlayDoorSound(EDoorInteraction DoorInteraction, AReadyOrNotCharacter* DoorInteractionInstigator, const TArray<FMODParam>& Params);

	UFUNCTION(BlueprintCallable, Category = "Door|Damage")
    void PlayDoorDamageSound(EDoorDamageType DoorDamage, const TArray<FMODParam>& Params);

	FTimerHandle ReactToDoorDamagedHandle;
	float ReactToDoorDamagedDelay = 1.0f;
	UFUNCTION()
	void AIResponseToDoorDamage();
	
	UFUNCTION(BlueprintCallable, Category = "Door|Damage")
	void PlayDoorKickSound(AReadyOrNotCharacter* Kicker, float Result = 1.0f);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Door|Damage")
	void Multicast_PlayDoorSound(EDoorInteraction DoorDamage, AReadyOrNotCharacter* DoorInteractionInstigator, const TArray<FMODParam>& Params);
	void Multicast_PlayDoorSound_Implementation(const EDoorInteraction DoorDamage, AReadyOrNotCharacter* DoorInteractionInstigator, const TArray<FMODParam>& Params) { PlayDoorSound(DoorDamage, DoorInteractionInstigator, Params); }

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Door|Damage")
	void Multicast_PlayDoorDamageSound(EDoorDamageType DoorDamage, const TArray<FMODParam>& Params);
	void Multicast_PlayDoorDamageSound_Implementation(const EDoorDamageType DoorDamage, const TArray<FMODParam>& Params) { PlayDoorDamageSound(DoorDamage, Params); }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayElectronicDoorSound(UFMODEvent* Event);

	UPROPERTY(Transient)
	UParticleSystem* KickedParticleSystem;

	UPROPERTY(Transient)
	UParticleSystem* LockBrokenParticleSystem;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDoorKickEffects(bool bBreakLock, bool bInFront);
	
	UFUNCTION()
    void OnRep_DoorHandlesBroken();

	UFUNCTION(NetMulticast, Reliable)
	void ForceDoorReset();
	
	UFUNCTION()
    void OnRep_ClientResetDoor();
	
	UFUNCTION()
    void OnRep_DestroyedChunkIdxChanged();

	UFUNCTION()
    void Tick_DoorOpenClose();
	
	UFUNCTION()
    void Tick_DoorKick_Success();
	
	UFUNCTION()
    void Tick_DoorKick_Fail();
	
	UFUNCTION()
    void Tick_DoorPush();
	
	UFUNCTION()
    void Tick_DoorHandle_Open();

	UFUNCTION()
    void Tick_DoorHandle_Push();

	UFUNCTION()
    void Tick_DoorHandleLocked();
	
	UFUNCTION()
    void Tick_DoorLocked();
	
	UFUNCTION()
    void Tick_DoorRam();
	
	UFUNCTION()
    void Tick_DoorExplode();

	UFUNCTION()
	void Tick_DoorBreach();

	UFUNCTION()
    void Finished_DoorRam();
	
	UFUNCTION()
    void Finished_DoorExplode();
	
	UFUNCTION()
    void Finished_DoorKick_Success();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootScene = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* DoorStatic = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* DoorHandleFront = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* DoorHandleBack = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UMirrorPortalComponent* FrontMirrorPoint = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UMirrorPortalComponent* BackMirrorPoint = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk0 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk1 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk2 = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk3 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk4 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk5 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk6 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk7 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UDestructibleDoorChunkComponent* DoorChunk8 = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* LightBlocker = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* DoorOpenInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* DoorSublinkOpenInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* DoorSublinkPushInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* DoorPushInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* DoorKickInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* DoorSublinkKickInteractableComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* LockpickInteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* C2InteractableComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* WedgeInteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* OptiwandInteractableComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* BSGInteractableComponent = nullptr;
    
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* BSGInteractableComponent_2 = nullptr;
    
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UInteractableComponent* DoorRamInteractableComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UDecalComponent* C2ExplosionDecalComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = sound)
    class UFMODAudioPropagationComponent* FMODAudioPropagationComp = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* LockpickArea = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* DoorArea = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* C2Area = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* BSGArea = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* MirrorgunArea = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* WedgeArea = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* BatteringRamArea = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* DoorBlockerComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* BreachBlocker1Component = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* BreachBlocker2Component = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* BreachBlocker3Component = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorOpenBillboard_Front = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorPushBillboard_Front = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorKickBillboard_Front = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class UBillboardComponent* LockpickBillboard_Front = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* C2Billboard_Front = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* BSGBillboard_Front = nullptr;
    
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class UBillboardComponent* BSGBillboard_2_Front = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* OptiwandBillboard_Front = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* WedgeBillboard_Front = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class UBillboardComponent* BatteringRamBillboard_Front = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorOpenBillboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorPushBillboard_Back = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* DoorKickBillboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* LockpickBillboard_Back = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* C2Billboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* BSGBillboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* BSGBillboard_2_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* OptiwandBillboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* WedgeBillboard_Back = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* BatteringRamBillboard_Back = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* RoomPositionBillboard_Front = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* RoomPositionBillboard_Back = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* RoomPositionBillboard_DoubleDoor_Front = nullptr;
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    class UBillboardComponent* RoomPositionBillboard_DoubleDoor_Back = nullptr;

	UPROPERTY()
	UTexture2D* CenterIcon = nullptr;
	UPROPERTY()
	UTexture2D* CornerRightIcon = nullptr;
	UPROPERTY()
	UTexture2D* CornerLeftIcon = nullptr;
	UPROPERTY()
	UTexture2D* HallwayIcon = nullptr;
	UPROPERTY()
	UTexture2D* HallwayLeftIcon = nullptr;
	UPROPERTY()
	UTexture2D* HallwayRightIcon = nullptr;

	void CreateEditorComponents();
	void DestroyEditorComponents();
	void ShowEditorComponents();
	void HideEditorComponents();
	#endif
	
public:
	#if WITH_EDITORONLY_DATA
	// Visualize C2 placement points for the current type of door
	// These placement points are edited from the door data table
	UPROPERTY(EditAnywhere, Category = "Debug")
	uint8 bDrawC2PlacementPoints : 1;

	// Visualize player collision area for the current type of door
	// The collision area can be edited from the door data table
	UPROPERTY(EditAnywhere, Category = "Debug")
	uint8 bDrawDoorwayExtent : 1;

	UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bDrawDoorwayExtent", EditConditionHides))
	FVector DebugDoorwayOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bDrawDoorwayExtent", EditConditionHides))
	FVector DebugDoorwayScale = FVector::OneVector;
	#endif

	// Has this door ever been opened by the swat ai?
	UPROPERTY(VisibleAnywhere, Transient, Category = "Status (Read Only)")
	uint8 bHasEverBeenOpenedBySwat : 1;
	
	// Used by swat. When the array is not empty, the nav link is disabled on the door to block the suspect AI from interacting/navigating
	// through this door while swat are operating on it, whether it being mirroring, placing door wedge, stacking up, breaching, etc.
	// Can be used for debugging purposes to see what operating states this door is on
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Status (Read Only)")
	TArray<FString> OperatingStates;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status (Read Only)")
	class ANavLinkProxy* NavLinkProxy = nullptr;

	// Used for checking pathing through door. Also used as navlink start
	FVector FrontDoorPoint = FVector::ZeroVector;
	// Used for checking pathing through door. Also used as navlink end
	FVector BackDoorPoint = FVector::ZeroVector;

	// If true, never allow spawning a trap on this door
	UPROPERTY(EditAnywhere, Category = "Setup|Trap")
	uint8 bNoSpawnTrap : 1;

	// Which side should the trap spawn in?
	UPROPERTY(EditAnywhere, Category = "Setup|Trap", meta = (EditCondition = "!bNoSpawnTrap"))
	EDoorTrapSide TrapSide = EDoorTrapSide::Front;

	// The type of trap to spawn. Use the TrapDataTable to select from a list of door traps
	UPROPERTY(EditAnywhere, Category = "Setup|Trap", meta = (EditCondition = "!bNoSpawnTrap"))
    FDataTableRowHandle TypeOfTrap;
	
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    TMap<EDoorDamageType, float> DoorKillDistance;
	
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    TMap<EDoorDamageType, float> DoorStunDistance;
	
	// The maximum amount that this door can be opened (in degrees)
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    float MaxOpenClose = 90.0f;
	
	// How much rotation value is needed to determine whether the door is open
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    float OpenThreshold = 5.0f;

	// How much rotation value is needed to determine whether the door can be closed
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    float CloseThreshold = 75.0f;

	// The angle amount to push by when pushing a door
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    float IncrementAngle = 15.0f;
    
	UPROPERTY(BlueprintReadOnly, Category = "Setup")
    float PhysicalPushAmount = 30.0f;

protected:
	// Used for double doors. The DriveSubDoor is a reference to a door adjacent to this door
	// This is automatically setup from the world data generator when regenerating a world
	// Can be manually setup, if needed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Sub Door", meta = (EditCondition = "!bIsDoorway"))
    ADoor* DriveSubDoor = nullptr;

	// If we have a subdoor, is this door the main one of the two?
	// Note: Only one door must be set as the main door
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Sub Door", meta = (EditCondition = "DriveSubDoor != nullptr && !bIsDoorway"))
    uint8 bMainSubDoor : 1;

public:
	// If true, kicking this door will always fail
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Kicking", meta = (EditCondition = "!bIsDoorway"))
    uint8 bKickAlwaysFails : 1;

	// The probability for the door to break open when kicked
	UPROPERTY(BlueprintReadOnly, Category = "Setup|Kicking")
    float DoorKickSuccessChance = 0.9f;

	// The number of kicks needed to break open the door
	// Successful kicks means the RNG was successful and the number of kicks needed to kick open is then reduced
	UPROPERTY(BlueprintReadOnly, Category = "Setup|Kicking")
    uint8 NumSuccessfulKicksToBreakDown = 1;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup|C2", meta = (EditCondition = "!bIsDoorway"))
    UMaterialInterface* C2ExplosionDecal = nullptr;

	// If true, this door is always locked
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Lock", meta = (EditCondition = "!bIsDoorway"))
	uint8 bAlwaysLocked : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Lock", meta = (EditCondition = "!bAlwaysLocked && !bIsDoorway"))
	uint8 bOverrideLockChance : 1;
	
	// The probability this door will be locked at the start of the game (0.0 to 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Lock", meta = (EditCondition = "!bAlwaysLocked && bOverrideLockChance && !bIsDoorway"))
    float LockedChance = 0.2f;

	// Whether or not this door is an electronic door, which needs either a keycard or to be disabled in order to be opened silently
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Lock", meta = (EditCondition = "!bIsDoorway"))
	bool bIsElectronicDoor = false;
	
	// Random chance for this door to be considered an electronic one. Keep this at 0.0 to never allow this to become an electronic door
	// Note: if "IsElectronicDoor" is maunally set to true, this setting will have no effect.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Lock", meta = (EditCondition = "!bIsElectronicDoor && !bIsDoorway"))
	float ElectronicLockChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
	UCurveFloat* DoorPushCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorOpenCurve = nullptr;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorCloseCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorKickSuccessCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorKickFailCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorLockedCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorRamCurve = nullptr;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorExplodeCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
	UCurveFloat* DoorBreachCurve = nullptr;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorHandleOpenCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorHandlePushCurve = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Curves", meta = (EditCondition = "!bIsDoorway"))
    UCurveFloat* DoorHandleLockedCurve = nullptr;

	// The C2 that is being placed on this door
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status (Read Only)")
    class APlacedC2Explosive* PlacedC2 = nullptr;   
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Generated")
    TArray<class UDestructibleDoorChunkComponent*> ChunkComponents;
	
	UPROPERTY(BlueprintReadOnly, Category = "Door")
	uint8 bPendingSubDoorKick : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Door")
	AReadyOrNotCharacter* LastDoorUser = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Door|Damage")
    EDoorDamageType LastDoorDamage = EDoorDamageType::DDT_None;

	// The currently attached trap on this door
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Status (Read Only)")
    ATrapActorAttachedToDoor* AttachedTrap = nullptr;

	// The currently attached wedge on this door
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Status (Read Only)")
    class ADoorJam* AttachedWedge = nullptr;

	// The current suspect running the check door behaviour
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadWrite, Category = "Status (Read Only)")
	ACyberneticCharacter* AICheckingDoor = nullptr;

	// Used by swat. When stacking up or breaching on this door, swat will register their activities here,
	// this allows the door to know when it should and shouldn't disable the door nav blocker
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Status (Read Only)")
	TArray<class UTeamStackUpActivity*> CurrentStackUpActivities;
	
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Status (Read Only)")
	TArray<class UBaseActivity*> CurrentActivities;

	UPROPERTY(Replicated)
    float OpenCloseAmount = 0.0f;

	UPROPERTY(Replicated)
    float DoorHandlePitchAmount = 0.0f;

	UPROPERTY(Replicated)
    uint8 bC2Placed : 1;
	
	UPROPERTY(Replicated)
    uint8 bSWATKnowsLockState : 1;

	UPROPERTY(Replicated)
    uint8 bSuspectKnowsLockState : 1;

	UPROPERTY(Replicated)
    uint8 bSWATKnowsTrapState : 1;
	
	UPROPERTY(Replicated)
    uint8 bSuspectKnowsTrapState : 1;

	UPROPERTY(Replicated)
    uint8 bDoorBroken : 1;

	void DisableDoorChunkNavigation();

	UPROPERTY(Replicated)
	uint8 bTrapInFront : 1;
	
	UPROPERTY(ReplicatedUsing = OnRep_ClientResetDoor)
	uint8 bClientReset : 1;
	
	UPROPERTY(ReplicatedUsing = OnRep_DoorHandlesBroken)
    uint8 bDoorHandlesBroken : 1;

	UPROPERTY()
	uint8 bSWATHasKeycard : 1;

	UPROPERTY()
	uint8 bSuspectsHaveKeycard : 1;

	UPROPERTY()
	uint8 bAlarmTriggered : 1;

private:
	void CheckDoorHit();
	
	void PlayTimeline(FTimeline& Timeline, bool bRestartIfAlreadyPlaying = true, bool bStopAllTimelines = true, bool bStopTimelinesIfAlreadyPlaying = true);
	void PlayTimelines(TArray<FTimeline*> Timelines, TArray<bool> bRestartIfAlreadyPlaying = {}, bool bStopAllTimelines = true, bool bStopTimelinesIfAlreadyPlaying = true);
	void StopTimeline(FTimeline& Timeline, bool bStopIfAlreadyPlaying = true);
	void StopAllTimelines(bool bStopTimelinesIfAlreadyPlaying = true);

	void SetDoorHasEverBeenOpenedBySwat();

	void DestroyAllInteractionComponents();

	FMODParam MakeOcclusionParam() const;
	float GetDistanceToLocalPlayer() const;

	FRotator GetInteractionRotation() const;
	FVector GetInteractionLocation() const;

	FString DoorStateAsString(bool bIsSuspect) const;

	void SetupTags();

	float LastStartAngle = 0.0f;
	float LastTargetAngle = 0.0f;
	float TargetHandleAngle = 0.0f;

	float AccumulatedDistance = 0.0f;
	float DistanceMovedThisFrame = 0.0f;
	float TimeMovedThisFrame = 0.0f;
	float AccumulatedTime = 0.0f;
	
	float TimeSinceLastActionPromptUpdate = 0.0f;
	float TimeSinceLastDoorBlockCheck = 0.0f;

	float TimeSinceLastBlocked = 999.0f;
	float OpenCloseAmountSinceLastBlocked = 0.0f;

	int32 TimesBreached = 0;
	
	uint8 bHasOpenDoorEventBroadcasted : 1;
	uint8 bHasCloseDoorEventBroadcasted : 1;

	UFUNCTION()
	void OnRep_DoorDataUpdated();
	UFUNCTION()
	void OnRep_TrapDataUpdated();

	FTimeline TL_DoorOpenClose;
	FTimeline TL_DoorPush;
	FTimeline TL_DoorHandleOpen;
	FTimeline TL_DoorHandlePush;
	FTimeline TL_DoorHandleLocked;
	FTimeline TL_DoorLocked;
	FTimeline TL_DoorRam;
	FTimeline TL_DoorBreach;
	FTimeline TL_DoorExplode;
	FTimeline TL_DoorKickSuccess;
	FTimeline TL_DoorKickFail;

	UPROPERTY()
	TArray<AReadyOrNotCharacter*> CharactersOverlappingDoor;

	float TimeSinceBreachBlockersActivated = 86400.0f;
	float TimeWithoutStackUpActivity = 86400.0f;

public:
	// Reflected data from the currently selected TypeOfDoor setting
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_DoorDataUpdated, Category = "Status (Read Only)")
	FDoorData DoorData;
	// Reflected data from the currently selected TypeOfTrap setting
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_TrapDataUpdated, Category = "Status (Read Only)")
	FTrapData TrapData;
	
	// Multiplier for how much more a door should occlude sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup|Audio", meta = (EditCondition = "!bIsDoorway"))
	float OcclusionMultiplier = 2.0f;
};
