// Copyright Void Interactive, 2024

#pragma once

#include "ReadyOrNotCharacter.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Animation/AnimationDataTable.h"
#include "Components/ItemVisualizationComponent.h"
#include "Interfaces/CanUseMultitoolOn.h"
#include "Interfaces/GatherDebugText.h"
#include "Interfaces/UseabilityInterface.h"
#include "Data/BloodData.h"
#include "HUD/Widgets/CommandInterface.h"
#include "FMODSnapshot.h"
#include "PlayerCharacter.generated.h"

DECLARE_STATS_GROUP(TEXT("PlayerCharacter"), STATGROUP_PlayerCharacter, STATCAT_Advanced);

class UFMODAudioComponent;
class UFMODEvent;
class UPoseAsset;
class UPlayerPostProcessing;
class UReadyOrNotCharacterAnimData;
class UBloodData;
class APickupActor;
class ABloodPool;

UENUM(BlueprintType)
enum class EHolsterAnimationType : uint8
{
	HAT_Normal,
	HAT_SkipHolster,			// skip the holster animation (used for detonator)
	HAT_AlwaysPlayHolster,		// always holster, even if we're in a blocking animation (used for chemlight)
};

UENUM(BlueprintType)
enum class ELedgeWidth : uint8
{
	LW_None,
	LW_Ledge,
	LW_Rail,
	LW_Fall
};
UENUM(BlueprintType)
enum class ELedgeHeight : uint8
{
	LH_None,
	LH_Step,
	LH_Vault,
	LH_Mantle
};

UENUM(BlueprintType)
enum class ELightRadialSelection : uint8
{
	LR_None,
	LR_NVGs,
	LR_WeaponLight,
	LR_Chemlight
};

USTRUCT(BlueprintType)
struct FCameraFreelookSetting
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Freelook")
	float PitchMin = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Freelook")
	float PitchMax = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Freelook")
	float YawMin = -45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Freelook")
	float YawMax = 45.0f;
};

USTRUCT(BlueprintType)
struct FMovementSound
{
	GENERATED_USTRUCT_BODY()

	FMovementSound()
	{
		ChanceToPlay = 100.0f;
	}
	
	UPROPERTY(EditAnywhere, Category = Audio)
	USoundCue* Sound = nullptr;

	UPROPERTY(EditAnywhere, Category = Audio)
	float ChanceToPlay = 100.0f;

};
USTRUCT(BlueprintType)
struct FBoneVelocity
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName BoneName;

	UPROPERTY()
	FVector LastLocation;

	UPROPERTY()
	float CalculatedSpeed = 0;

};

// For blood pooling.
USTRUCT(BlueprintType)
struct FInjury
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName BoneName;

	UPROPERTY()
	int32 InjuryCount;
};

UENUM(BlueprintType)
enum class EFootConstEnum : uint8
{
	FCE_Forward	UMETA(DisplayName = "ForwardDirection"),
	FCE_Inverse	UMETA(DisplayName = "InverseDirection")
};

UENUM(BlueprintType)
enum class EIKStateEnum : uint8
{
	IKE_None		UMETA(DisplayName = "NoIK"),
	IKE_Optimize	UMETA(DisplayName = "Optimized IK"),
	IKE_Full		UMETA(DisplayName = "Full IK"),
};

USTRUCT(BlueprintType)
struct FFootIKStruct
{
	GENERATED_BODY()

		/** Foot Offset calculated */
		UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Offset;

	/** Foot rotation calculated */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		FRotator FootRotation;

	FFootIKStruct()
	{
		Offset = 0.0f;
		FootRotation = FRotator::ZeroRotator;
	}
};

USTRUCT(BlueprintType)
struct FHealingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
	float CurrentHealth = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
	float MinHealth = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
    FName HealerName = "None";
	
	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
    FName HealeeName = "None";

	UPROPERTY(BlueprintReadOnly, Category = "Healing Data")
	EMedicalHealScreen HealScreen;
};


USTRUCT(BlueprintType)
struct FWoundData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Gore")
	float WoundSize = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Gore")
	FVector WoundOffset = { 0.0f, 0.0f, 0.0f };

	UPROPERTY(EditAnywhere, Category = "Gore")
	UStaticMesh* UpperMesh;

	UPROPERTY(EditAnywhere, Category = "Gore")
	FTransform UpperMeshTranform;

	UPROPERTY(EditAnywhere, Category = "Gore")
	UStaticMesh* LowerMesh;

	UPROPERTY(EditAnywhere, Category = "Gore")
	FTransform LowerMeshTranform;

	UPROPERTY(EditAnywhere, Category = "Gore")
	bool bBreaksBone = true;
};

UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API APlayerCharacter : public AReadyOrNotCharacter
{
	GENERATED_BODY()

public:
	explicit APlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category="Customization")
	TArray<UMeshComponent*> CustomizationFirstPersonMeshes;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="Customization")
	TArray<UMeshComponent*> CustomizationFirstPersonBodyMeshes;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category="Customization")
	TArray<UMaterialInstanceDynamic*> CustomizationActorMaterials;

	UPROPERTY(Transient)
	bool bFirstPersonMeshesDirty;
	
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	FORCEINLINE class USkeletalMeshComponent* GetMeshBody1P() const { return MeshBody1P; }
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	FORCEINLINE class UPlayerPostProcessing* GetPlayerPostProcessing() const { return PlayerPostProcessingComp; }
	FORCEINLINE class UBleedComponent* GetBleedComponent() const { return BleedComponent; }
	FORCEINLINE class UFMODAudioComponent* GetFMODBreathingComp() const { return FMODBreathingAudioComp; }
	FORCEINLINE USkeletalMeshComponent* GetMeshBody() { return MeshBody1P; }
	
	FORCEINLINE UItemVisualizationComponent* GetPrimaryVisualizationComponent() const { return PrimaryItemVisualizationComponent; }
	FORCEINLINE UItemVisualizationComponent* GetSecondaryVisualizationComponent() const { return SecondaryItemVisualizationComponent; }
	FORCEINLINE UItemVisualizationComponent* GetLongTacticalVisualizationComponent() const { return LongTacticalVisualizationComponent; }

	bool GetItemWheelActive();
	bool GetCommandWheelActive();

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class UCameraComponent* ThirdPersonCameraComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class USpringArmComponent* ThirdPersonCameraArm = nullptr;
	
	// Pawn mesh: 1st person view (arms; seen only by self)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* Mesh1P = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* MeshBody1P = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UCameraComponent* FirstPersonCameraComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UPlayerPostProcessing* PlayerPostProcessingComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UFMODAudioComponent* FMODBreathingAudioComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBleedComponent* BleedComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UScoringComponent* ScoringComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* PrimaryItemVisualizationComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* SecondaryItemVisualizationComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* LongTacticalVisualizationComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* HelmetVisualizationComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* ArmorVisualizationComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UItemVisualizationComponent* EquippedItemVisualizationComponent = nullptr;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Tick_Authority(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	FInputActionBinding BindingNoConsume(FName InActionName, EInputEvent InKeyEvent, void (APlayerCharacter::* Func) ());
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual bool OnTakeDamage(float& Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// overriden for freelook camera
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;

	virtual void Jump() override;
	virtual void StopJumping() override;
	
	// interface
	virtual void OnMelee_Implementation(AReadyOrNotCharacter* Attacker, FHitResult Hit) override;

public:
	virtual void OnEquippedWeaponFire(ABaseMagazineWeapon* Weapon, bool bServer) override;
	virtual void OnEquippedWeaponDryFire(ABaseMagazineWeapon* Weapon, bool bServer) override;

	virtual void PlayWeaponFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, bool bOnlyTP = false) override;
	virtual void PlayWeaponDryFireAnimation(ABaseMagazineWeapon* Weapon, bool bIsAiming, bool bOnlyTP = false) override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	virtual FRotator GetBaseAimRotation() const override;

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const override;

	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual bool IsOutside() override;

	virtual bool ShouldEnableDepthFade() override;

	virtual bool GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface) override;

	virtual void RespondToBleedOutDamage() override;

	virtual bool TryApplyStunDamage(UStunDamage* InStunDamage, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual bool TryApplyBulletDamage(float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyHeadDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyUpperBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLowerBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightArmDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightLegDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyLeftFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void ApplyRightFootDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyBodyDamage(float& Damage, FPointDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void StartStun(EStunType StunType, AActor* StunCauser) override;
	virtual void StartPepperSprayed(APepperspray* Pepperspray) override;

	virtual void Multicast_OnKilled_Implementation(FName LastBoneHit, AActor* DamageCauser) override;

	virtual void OnGameEnded_Implementation() override;

	virtual void ReportToTOC_Implementation(AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true) override;

	virtual void Multicast_InflictSuppression_Implementation(FSuppressionData SuppressionData, TSubclassOf<ULegacyCameraShake> CameraShake, bool bLessLethal) override;
	virtual void Multicast_TakeDamage_Implementation(float Damage, const FDamageEvent& DamageEvent, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser) override;

	virtual bool CanShowActionPrompt1() const override;

	float TimeUntilNextLowReadyTrace = 0.0f;
	virtual void DoLowReadyTrace() override;

	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter) override;
	
	virtual bool IsAnimationBlocking() const override;
	
	virtual void LockAllActions() override;
	virtual void UnlockAllActions() override;
	virtual void LockMovementAndActions() override;
	virtual void UnlockMovementAndActions() override;
	virtual void LockMovement() override;
	virtual void UnlockMovement() override;
	virtual void LockAim() override;
	virtual void UnlockAim() override;
	virtual void LockItemSelection() override;
	virtual void UnlockItemSelection() override;
	virtual void LockCommandMenu() override;
	virtual void UnlockCommandMenu() override;
	virtual void LockWeaponAttachments() override;
	virtual void UnlockWeaponAttachments() override;
	virtual void LockCantedSight() override;
	virtual void UnlockCantedSight() override;
	
	virtual void Play1PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f) override;
	virtual void Play1PMontageDeferred_Implementation(UAnimMontage* Montage, const FString& AnimationName) override;
	virtual void Play1PMontage_NonClient(UAnimMontage* NewMontage, float PlayRate = 1.0f) override;
	virtual void Multicast_Stop1PMontage_Implementation(UAnimMontage* Montage, float BlendoutTime = 0.25f) override;
	virtual void PlayLocal1PMontage(UAnimMontage* NewMontage, float PlayRate = 1.0f) override;
	virtual void Client_Play1PMontage_Implementation(UAnimMontage* NewMontage, float PlayRate = 1.0f) override;
	virtual bool IsTableMontagePlaying(const FString& Animation) const override;
	virtual bool IsTableMontagePlayingWithTimeRemaining(const FString& Animation, float& TimeRemaining) const override;

	/** Stop Animation Montage. If nullptr, it will stop what's currently active. The Blend Out Time is taken from the montage asset that is being stopped. **/
	virtual void StopFPAnimMontage(class UAnimMontage* AnimMontage = nullptr, float BlendoutTime = 0.0f) override;

	virtual void Multicast_PauseAllAnims_Implementation(bool bPaused) override;

	virtual void OnItemEquipped(ABaseItem* NewEquippedItem) override;
	virtual void OnItemHolstered(ABaseItem* HolsteredItem) override;

	virtual void OnEquippedWeaponMagCheck(ABaseMagazineWeapon* Weapon) override;

	virtual void OnRep_MeshReplicated() override;

	void OnKilledOrGoneUnconscious();

	// Called when this character is killed or goes unconcious on both client and server
	UFUNCTION(BlueprintImplementableEvent)
	void OnKilledOrGoneUnconciousBP();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MeshReplicated)
	USkeletalMesh* Rep_FPBodyMesh;

	UPROPERTY()
	UMaterialInterface* LastSetMesh1PDynamicMaterial;
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicWeaponFovMats;
	
	float DesiredDynamicWeaponFoVBlendEffectAmount = 1.0f;
	float CurrentDynamicWeaponFoVBlendEffectAmount = 1.0f;

	float LastSetAspectRatio = 0.0f;

	USkeletalMesh* GetAppropriateFPMesh();

	UPROPERTY()
	TArray<USkeletalMeshComponent*> MeshComps;

	bool bWantsToCrouch = false;
	
public:

	UPROPERTY()
	APlayerState* LastKnownPlayerState;

	// IUseablility Interface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual FName DetermineAnimatedIcon_Implementation() const override;
	////////////////////////

	// IPingInterface
	virtual FSlateBrush GetPingIcon_Implementation() override;
	virtual FText GetPingText_Implementation() override;
	virtual FVector GetPingLocation_Implementation() override;
	virtual float GetPingDuration_Implementation() override;
	virtual bool CanPing_Implementation() override;
	////////////////////////

	UFUNCTION(CallInEditor)
	void StartBleeding();
	
protected:
	UFUNCTION(Exec)
	void DestroyNonDevelopmentComponents();

public:
	void SetDesiredWeaponFOVBlend(float NewDesiredBlend) { DesiredDynamicWeaponFoVBlendEffectAmount = NewDesiredBlend; }

	bool bDisableWeaponFOV_FromNotify = false;
	
	UPROPERTY(Replicated)
	FRotator ReplicatedFPMesh;

	float TimeUntilNextPushOverlappingAI = 0.0f;
	float TimeMoving = 0.0f;
	void PushOverlappingAI();

	virtual void MoveBlockedBy(const FHitResult& Impact) override;
	FVector AvoidanceLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Input)
	FVector CurInputVector;

	UPROPERTY()
	FTimerHandle PushOverlappingAI_Handle;

	UPROPERTY()
	class UMaterialInstanceDynamic* Body1PMat;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Camera")
	AReadyOrNotCharacter* CurrentViewCharacter = nullptr;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDefaultCommandIssued, APlayerCharacter*, Issuer, ESwatCommand, CommandIssued);
	UPROPERTY(BlueprintAssignable, Category = "Swat Command")
	FOnDefaultCommandIssued OnDefaultCommandIssued;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamViewSet, AReadyOrNotCharacter*, NewViewCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Team View")
	FOnTeamViewSet OnTeamViewSet;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team View")
	int32 CurrentTeamViewIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Team View")
	class APlayerViewActor* PlayerViewActor = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team View")
	TSubclassOf<class APlayerViewActor> PlayerViewActorClass = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Team View", DisplayName = "Try Next Player View")
    void TryNextPlayerView_Released();
    void TryNextPlayerView_Pressed();

	UPROPERTY(BlueprintReadOnly, Category = "Team View", meta = (ClampMin = 0.01f))
	FTimerHandle TH_TeamViewInput;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team View", meta = (ClampMin = 0.01f))
	float TeamViewInputHoldTime = 0.25f;

	UFUNCTION(BlueprintCallable, Category = "Team View")
    void ClosePlayerView();
	
    void NextPlayerView(bool bRequestClose = false, const bool bIncludeDeadViews = true);
	void DestroyPlayerViewActor();

	UFUNCTION(BlueprintPure, Category = "Team View")
	TArray<AReadyOrNotCharacter*> GetAvaliablePlayersForTeamView(const bool bIncludeDeadViews = true) const;
	
	UFUNCTION(BlueprintCallable, Category = "Team View")
	void CaptureFPCamera(float DeltaTime);
	
	//	FMOD Variables for rooms and surfaces
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	float surfaceType = 0.0f;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	float roomType = 0.0f;
	
	UPROPERTY(EditAnywhere)
	FString PVPSpeakerName = "";

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	virtual void Server_PlayPVPSpeech(const FString& SpeechRowName, ETeamType TeamType);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPVPSpeech(const FString& SpeechRowName, ETeamType TeamType);

	// Killfeed
	UFUNCTION(Server, Reliable, WithValidation, Category = Scripting)
			void Server_KillfeedMessage(APlayerCharacter* Killer, APlayerCharacter* Victim, ABaseItem* Weapon);
	virtual void Server_KillfeedMessage_Implementation(APlayerCharacter* Killer, APlayerCharacter* Victim, ABaseItem* Weapon);
	virtual bool Server_KillfeedMessage_Validate(APlayerCharacter* Killer, APlayerCharacter* Victim, ABaseItem* Weapon) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, Category = Scripting)
			void Server_ArrestfeedMessage(APlayerCharacter* Arrester, APlayerCharacter* Victim);
	virtual void Server_ArrestfeedMessage_Implementation(APlayerCharacter* Arrester, APlayerCharacter* Victim);
	virtual bool Server_ArrestfeedMessage_Validate(APlayerCharacter* Arrester, APlayerCharacter* Victim) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, Category = Scripting)
			void Server_FreefeedMessage(APlayerCharacter* Freer, APlayerCharacter* Victim);
	virtual void Server_FreefeedMessage_Implementation(APlayerCharacter* Freer, APlayerCharacter* Victim);
	virtual bool Server_FreefeedMessage_Validate(APlayerCharacter* Freer, APlayerCharacter* Victim) { return true; }

	UFUNCTION(NetMulticast, Reliable)
	void LocalKillFeed(APlayerCharacter* Killer, APlayerCharacter* Victim, ABaseItem* Weapon);

	UFUNCTION(NetMulticast, Reliable)
	void LocalArrestFeed(APlayerCharacter* Arrester, APlayerCharacter* Victim);

	UFUNCTION(NetMulticast, Reliable)
	void LocalFreeFeed(APlayerCharacter* Freer, APlayerCharacter* Victim);

	UFUNCTION(NetMulticast, Reliable)
	void LocalDeathFeed(class AReadyOrNotPlayerController* PlayerController);

	void IssueDefaultCommand();
	void ToggleSwatCommandMenu();
	void CycleSwatElementNext();
	void CycleSwatElementPrevious();
	void SelectElementGold();
	void SelectElementBlue();
	void SelectElementRed();

	UFUNCTION(Server, WithValidation, Reliable)
	void Server_GiveAIMoveTo(ACyberneticCharacter* AI, FVector Location);
	void Server_GiveAIMoveTo_Implementation(ACyberneticCharacter* AI, FVector Location);
	bool Server_GiveAIMoveTo_Validate(ACyberneticCharacter* AI, FVector Location) { return true; }
	
	UFUNCTION(Server, WithValidation, Reliable)
	void Server_StopAIMoveTo(ACyberneticCharacter* AI);
	void Server_StopAIMoveTo_Implementation(ACyberneticCharacter* AI);
	bool Server_StopAIMoveTo_Validate(ACyberneticCharacter* AI) { return true; }
	
	UFUNCTION(Server, WithValidation, Reliable)
	void Server_GiveAIMoveToExit(ACyberneticCharacter* AI);
	void Server_GiveAIMoveToExit_Implementation(ACyberneticCharacter* AI);
	bool Server_GiveAIMoveToExit_Validate(ACyberneticCharacter* AI) { return true; }
	
	UFUNCTION(Server, WithValidation, Reliable)
	void Server_GiveAITurnAroundOrder(ACyberneticCharacter* AI);
	void Server_GiveAITurnAroundOrder_Implementation(ACyberneticCharacter* AI);
	bool Server_GiveAITurnAroundOrder_Validate(ACyberneticCharacter* AI) { return true; }

	UPROPERTY(BlueprintReadOnly, Category = "Swat Command")
	bool bIsSwatCommandOpen = false;
	void CreateSwatCommandWidgetIfNotExists();
	UPROPERTY(BlueprintReadOnly, Category = "Swat Command")
	class USwatCommandWidget* SwatCommandWidget;
	bool bDisableInventoryInput = false;
	void EnableInventoryInput() { bDisableInventoryInput = false; }

	bool bWasSwatCommandKeyPressed = false;

	/** True if we have the command menu open */
	UPROPERTY(BlueprintReadWrite, Category = Command)
	bool bInCommandMenu;

	/** True if we have the devices menu open */
	UPROPERTY(BlueprintReadWrite, Category = HUD)
	bool bInDevicesMenu;
	
	UPROPERTY(BlueprintReadWrite, Category = HUD)
	bool bInTabMenu = false;
	
	UPROPERTY(BlueprintReadWrite, Category = HUD)
	bool bFadeToGray;
	
	virtual void PlayerControlledOnlyTick(float DeltaSeconds);

	// hides any initialization like when equipped item is syncing etc
	virtual void PlayInitialCameraFade();
	
	bool bStartedFadeIn = false;

	// returns count of visible light sources
	// if no minimum light level passed in it will use the light level specified in the level script.
	UFUNCTION(BlueprintCallable, Category = Visibility)
	bool IsInLightSource(int32& VisibleLightSources, float MinimumLightLevel = 0.0f);
	
	int32 DropArrestedInteractionSlot = -1;
	int32 CarryArrestedInteractionSlot = -1;
private:

	// cache the results and send them if the last check happened within a certain period of time (reduce performance cost).
	float TimeSinceLastLightSourceCheck = 0.0f;
	int32 LastVisibleLightSources = 0;
	float LastMinimumLightSourceIntensity = 0.0f;
	bool bLastIsInLightSource = false;
	
public:

	UFUNCTION(BlueprintCallable, Category = Movement)
	bool CalculateStopLocation(
			FVector& OutStopLocation,
			const FVector& CurrentLocation,
			const FVector& Velocity,
			const FVector& Acceleration,
			float Friction,
			float BrakingDeceleration,
			float TimeStep,
			int MaxSimulationIterations /*= 10*/);

	/* Mouse sensitivity, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	float Sensitivity = 1.0f;

	/* Invert mouse horizontally, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bInvertYaw = false;

	/* Invert mouse vertically, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bInvertPitch = false;

	/* Gamepad look sensitivity, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	float GamepadLookSensitivity = 1.0f;

	/* Gamepad aim sensitivity, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	float GamepadAimSensitivity = 1.0f;

	/* Invert gamepad horizontally, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bInvertGamepadHorizontal = false;

	/* Invert gamepad vertically, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bInvertGamepadVertical = false;

	/* Hold to crouch (gamepad), should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bHoldCrouchGamepad = false;

	/* Toggle ADS (gamepad), should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bToggleADSGamepad = false;

	/* Gamepad control scheme, should be loaded from settings. */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bUsingAlternateControls = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> HUD_Widget;
	
	UPROPERTY(BlueprintReadOnly)
	class UHumanCharacterHUD_V2* HumanCharacterWidget_V2;
	
	UPROPERTY(BlueprintReadOnly)
	class UUserWidget* MagCheckUI;
	
	UPROPERTY(BlueprintReadOnly)
	class UTeamViewWidget* TeamViewWidget = nullptr;
		
	UFUNCTION(BlueprintCallable)
	void ToggleHUD();
	
	UFUNCTION()
    virtual void SetHumanCharacterWidget_V2(class UHumanCharacterHUD_V2* NewHumanCharacterWidget);
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Interaction)
	class UInteractableComponent* LastInteractableComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Environment)
	class ABuildingTrigger* LastBuildingEntered = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = Environment)
	class ABuildingTrigger* InsideCurrentBuilding = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Interaction)
	class ADoor* lastHighlightedDoor;
	UPROPERTY(BlueprintReadOnly, Category = Interaction)
	class ABaseItem* lastHighlightedEvidence;
	UPROPERTY(BlueprintReadOnly, Category = Interaction)
	class APickupMagazineActor* lastHighlightedPickupMagazine;

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Animation")
			void Server_SpawnEjectedMagazine(FTransform SpawnTransform, class ABaseMagazineWeapon* weapon);
	virtual void Server_SpawnEjectedMagazine_Implementation(FTransform SpawnTransform, class ABaseMagazineWeapon* Weapon);
	virtual bool Server_SpawnEjectedMagazine_Validate(FTransform SpawnTransform, class ABaseMagazineWeapon* weapon) { return true; }

	// If true, when possessed, this character will show an indication on the HUD that the player can exit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bExitWithFireSelect = false;

	// The previous character that was possessed (if we are possessed by a tablet)
	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
	APlayerCharacter* PreviousPosessedCharacter;

	UFUNCTION(Client, Reliable)
			void Client_PossessedBy(AController* NewController);
	virtual void Client_PossessedBy_Implementation(AController* NewController);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientPossessed, AController*, NewController);
	UPROPERTY(BlueprintAssignable)
	FOnClientPossessed OnClientPossessed;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CreateHUDWidget();
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CreateTeamViewWidget();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnPlayerTakeDamageDetails, bool, bWasHeadshot, float, DamageTaken, float, HealthRemaining, bool, bBlockedByArmour, bool, bBlockedByHelmet);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerTakeDamageDetails OnPlayerTakenDamageDetails;

	UFUNCTION(Client, Reliable)
			void Client_OnTakenDamageDetail(bool bWasHeadshot, bool bTorsoShot, bool bLeftArm, bool bLeftLeg, bool bRightArm, bool bRightLeg, float DamageTaken, float RemainingHealth, bool bBlockedByArmour, bool bBlockedByHelmet);
	virtual void Client_OnTakenDamageDetail_Implementation(bool bWasHeadshot, bool bTorsoShot, bool bLeftArm, bool bLeftLeg, bool bRightArm, bool bRightLeg, float DamageTaken, float RemainingHealth, bool bBlockedByArmour, bool bBlockedByHelmet);

	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override;

	UFUNCTION(Server, Reliable)
	void Server_TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	void Server_TakeDamage_Implementation(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	bool Server_TakeDamage_Validate(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) { return true; }
	
	virtual void DeactivateVoiceAudioComp();

	virtual FString MutateVoiceline(const FString& VO) override;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* CriticalInjuredEvent;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* FlatlineEvent;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* FlatlineEventPvP;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* HeadshotEvent;

	UPROPERTY(EditAnywhere, Category = "FMOD Audio")
	UFMODEvent* SAPIPlateHitEvent;

	UFUNCTION()
	void OnFullHealth();
	UFUNCTION()
	void OnLowHealth(float CurrentHealth);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_PrepareForHeal();
	void Server_PrepareForHeal_Implementation();
	bool Server_PrepareForHeal_Validate() { return true; }

	UFUNCTION(Server, Reliable)
	void Server_FinishHealing();
	
	UPROPERTY(BlueprintReadOnly, Category = "Optiwand")
    uint8 bMirroring : 1;

	// 0 to 360.0f
	float LastDamageHitAngle = 0.0f;

	UPROPERTY(EditAnywhere, Category = Items)
	bool bSpawnInventoryItemsOnPossess = true;

	bool bCustomizationSpawned = false;
	
	UFUNCTION(BlueprintCallable)
	void StartFreeLook();

	UFUNCTION(BlueprintCallable)
	void StopFreeLook();
	
	UFUNCTION(BlueprintPure, Category = Camera)
	bool IsFreelooking() const;

	void ClampFreelookRotation();

	// we use this in the anim bp
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "FreeLook")
	FRotator FreeLookCache;

	UFUNCTION(Server, Unreliable, WithValidation)
			void Server_UpdateFreeLookCache(FRotator NewFreeLookCache);
	virtual void Server_UpdateFreeLookCache_Implementation(FRotator NewFreeLookCache);
	virtual bool Server_UpdateFreeLookCache_Validate(FRotator NewFreeLookCache) { return true; }

	FRotator FreeLookStartRotation;
	bool bFreelooking = false;
	bool bEndingFreelookPitch = false;
	bool bEndingFreelookYaw = false;

	UPROPERTY(EditAnywhere, Category = "Player | AI")
	float YellOutEffectLength = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Player | AI")
	float YelloutEffectRadius = 3500;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bShowReadyStatus = false;
	
	virtual bool CanYell() const override;

	// The last playerstate of the person who possessed this
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	AReadyOrNotPlayerState* LastPlayerState = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_PlaySound(USoundCue* Cue);
	virtual void Server_PlaySound_Implementation(USoundCue* Cue);
	virtual bool Server_PlaySound_Validate(USoundCue* Cue) { return true; }

	UFUNCTION(NetMulticast, Reliable)
			void Multicast_PlaySound(USoundCue* Cue);
	virtual void Multicast_PlaySound_Implementation(USoundCue* Cue);

	// Bones that have been hit in the course of this players life. Useful for spawning things like blood when that bone hits the ground
	UPROPERTY()
	TArray<FName> HitBones;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Arm_L;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Arm_R;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Low;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Leg_L;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Leg_R;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Head_Front;

	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Animations")
	TSubclassOf<ULegacyCameraShake> Camera_Hit_Head_Back;
	
	/* the crouch height of the camera when crouching */
	UPROPERTY(EditAnywhere, Category = "Player | Camera")
	float CrouchHeight;

	/* sound file to play when the impact occurs */
	UPROPERTY(EditAnywhere, Category = "Player | Damage | Impact Sounds")
	USoundCue* BodyImpactGroundSound;
	
	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
	FRotator Camera_RotationRate;

	UFUNCTION(Server, Unreliable, WithValidation)
			void Server_UpdateCameraRotationRate(FRotator NewCameraRotRate);
	virtual void Server_UpdateCameraRotationRate_Implementation(FRotator NewCameraRotRate);
	virtual bool Server_UpdateCameraRotationRate_Validate(FRotator NewCameraRotRate) { return true; }

	// Finds the closest playercharacter to us that's on a particular team, NOT INCLUDING self. TT_SQUAD will give both red and blue. TT_NONE gives all.
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	APlayerCharacter* GetClosestPlayerCharacter(ETeamType Team, float& OutClosestDistance, bool bExcludeArrested = false);

	// Finds all player characters on a particular team, NOT INCLUDING self. TT_SQUAD will give both red and blue. TT_NONE gives all.
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	TArray<APlayerCharacter*> GetAllOtherPlayerCharacters(ETeamType Team);
	
	/* Functions to trace from the center of the players view X distance in front on y collision channels*/

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual FHitResult GetHitFromCamera(float MaxDistance, TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels, FRotator OffsetRotation = FRotator::ZeroRotator, FVector OffsetVector = FVector::ZeroVector, bool bDrawTrace = false);

	UPROPERTY(BlueprintReadOnly, Category = Vehicles)
	APawn* CurrentlyPiloting = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_ChangeMesh(USkeletalMesh * FPMesh = nullptr, USkeletalMesh * TPMesh = nullptr, USkeletalMesh* TPHeadMesh = nullptr);
	virtual void Server_ChangeMesh_Implementation(USkeletalMesh * FPMesh = nullptr, USkeletalMesh * TPMesh = nullptr, USkeletalMesh* TPHeadMesh = nullptr);
	virtual bool Server_ChangeMesh_Validate(USkeletalMesh * FPMesh = nullptr, USkeletalMesh * TPMesh = nullptr, USkeletalMesh* TPHeadMesh = nullptr) { return true; }

	FHitResult LastPointDamageHit;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	APlayerCharacter* RevivingPlayer = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated)
	APlayerCharacter* BeingRevivedByPlayer = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	float RevivingOperatingTime = 0.0f;

	void StartReviving(APlayerCharacter* PlayerCharacter);
	void StopReviving(APlayerCharacter* PlayerCharacter);

	void OnReviveComplete(APlayerCharacter* PlayerCharacter);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnReviveComplete(APlayerCharacter* PlayerCharacter);
	void Server_OnReviveComplete_Implementation(APlayerCharacter* PlayerCharacter) { OnReviveComplete(PlayerCharacter); }
	bool Server_OnReviveComplete_Validate(APlayerCharacter* PlayerCharacter) { return true; }
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Damage)
	float StunMovementSpeedMultiplier = 0.75f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Damage)
	bool bStunAimLocked = false;

	UPROPERTY()
	ABaseItem* LastEquippedItemBeforeStun = nullptr;

	float LastMaxMovementSpeedWhileStunned = 1.0f;
	
	UFUNCTION(BlueprintCallable, Client, Reliable)
	virtual void Client_StartStun(EStunType StunType, AActor* StunCauser, FVector DamageCauserLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	virtual void Client_StartPepperSprayed(APepperspray* Pepperspray, FVector DamageCauserLocation);
	
	UPROPERTY(BlueprintReadWrite)
	float FirstPersonShakeAmount = 0.0f;

	/* will simulate physics on the bown structure down */
	void StartBoneBlend(FName BoneName);

	UPROPERTY(EditAnywhere, Category = "Player | Movement | Camera")
	float ViewBlendMultiplier = 0.025f;
	
	UFUNCTION()
	void OnRep_StartBoneBlend();

	bool bBlendingIn;
	UPROPERTY(ReplicatedUsing = OnRep_StartBoneBlend)
	FName BlendedBone;
	float CurrentBlend;

	void Respawn();

	UPROPERTY(ReplicatedUsing = OnRep_UpdateAnimInstance)
	TSubclassOf<UAnimInstance> Replicated_3PAnimInstance;

	UPROPERTY(ReplicatedUsing = OnRep_UpdateAnimInstance)
	TSubclassOf<UAnimInstance> Replicated_1PAnimInstance;

	UPROPERTY()
	TArray<UAnimMontage*> MontageQueue_3P;
	UPROPERTY()
	TArray<UAnimMontage*> MontageQueue_1P;

	
	bool CanThrowChemlight() const;
	
	bool CanUse() const;
	
	void Use();

	void UseOnly();
	void EndUseOnly();
	
	bool ShouldExecuteUseOnKeyDown();

	void Execute_Use(bool bUseOnly);
	void Execute_EndUse();
	void Execute_DoubleTapUse();

	UFUNCTION(Server, Reliable, WithValidation)
			void Server_ActorPickedUp(APickupActor* PickupActor);
	virtual void Server_ActorPickedUp_Implementation(APickupActor* PickupActor);
	virtual bool Server_ActorPickedUp_Validate(APickupActor* PickupActor) { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_InstaStartArrest(APlayerCharacter* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_InstaStartFree(APlayerCharacter* Target);
	
	UPROPERTY(BlueprintReadOnly, Category = Use)
	float HoldingUseTime = 0.0f;
		
	UPROPERTY(BlueprintReadOnly, Category = Use)
	bool bHoldingUse = false;

	UPROPERTY(BlueprintReadOnly)
	bool bLookingAtEvidenceItem = false;

	UPROPERTY(BlueprintReadOnly)
	bool bLookingAtDoor = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bLookingAtHuman = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bLookingAtTarget = false;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetUserLowReady(bool bShouldUserLowReady);

	virtual void SetLowReady(bool bUp, bool bLowReady) override;
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLowReady(bool bUp, bool bLowReady, bool bIsUserLowReady);
	
	UFUNCTION(BlueprintCallable, Category = LowReady)
	void OnLowReadyButtonDown();

	UFUNCTION(BlueprintCallable, Category = LowReady)
	void OnLowReadyButtonUp();

	UFUNCTION(BlueprintCallable, Category = LowReady)
	void ToggleLowReady();

	// true if the user is giving input to lowready
	UPROPERTY(BlueprintReadOnly, Category = LowReady)
	bool bUserLowReady = false;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = LowReady)
	bool bForceLowReady = false;

	UFUNCTION(BlueprintCallable, Category = "Low Ready")
	void SetForceLowReady(bool bShouldForceLowReady);

	bool bIsLowReadyFromWall = false;
	bool bIsLowReadyFromVolume = false;

	void DoLowReadyVolumeTrace();
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = LowReady)
	float LowReadyTraceDistance = 10000.0f;
	
	UFUNCTION(BlueprintCallable, Category = Animation)
	void ForceFirstDraw();

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Evidence")
	class AEvidenceActor* LastCollectedEvidence = nullptr;
	
	// used to ovveride armor mesh if default is set (won't override skins etc)
	UPROPERTY(EditAnywhere, Category = "Armor Override")
	TMap<TSubclassOf<class ABaseArmour>, USkeletalMesh*> ArmorOverrideMapFP;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeFPMesh(USkeletalMesh* NewFPMesh);
	
	UFUNCTION(Exec)
	void PrintItemAttachmentListToLog();

	float TimeEquipped = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Items)
	TArray<TSubclassOf<class AActor>> ChemlightClasses;

	UFUNCTION(BlueprintCallable, Category = "Gameplay Effect")
	void ApplyPlayerEffect(class UBasePlayerEffect* InPlayerEffect, bool bResettable = true);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Effect")
	void ApplyPlayerEffectFor(UBasePlayerEffect* InPlayerEffect, float Seconds);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Gameplay Effect")
	void Client_ApplyPlayerEffect(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bResettable = true, bool bMulticast = false);
	void Client_ApplyPlayerEffect_Implementation(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bResettable = true, bool bMulticast = false);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Gameplay Effect")
	void Server_ApplyPlayerEffect(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bResettable = true, bool bMulticast = false);
	void Server_ApplyPlayerEffect_Implementation(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bResettable = true, bool bMulticast = false);
	bool Server_ApplyPlayerEffect_Validate(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bResettable = true, bool bMulticast = false) { return true; }

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Gameplay Effect")
	void Server_ApplyPlayerEffectFor(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, float Seconds, bool bMulticast = false);
	void Server_ApplyPlayerEffectFor_Implementation(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, float Seconds, bool bMulticast = false);
	bool Server_ApplyPlayerEffectFor_Validate(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, float Seconds, bool bMulticast = false) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Gameplay Effect")
	void Server_ResetPlayerEffect(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bMulticast = false);
	void Server_ResetPlayerEffect_Implementation(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bMulticast = false);
	bool Server_ResetPlayerEffect_Validate(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass, bool bMulticast = false) { return true; }

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Gameplay Effect")
	void Client_ResetPlayerEffect(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass);
	void Client_ResetPlayerEffect_Implementation(TSubclassOf<class UBasePlayerEffect> InPlayerEffectClass);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gameplay Effect")
	void OnPlayerEffectExpired(TSubclassOf<class UReadyOrNotGameplayEffect> InPlayerEffectClass);
	virtual void OnPlayerEffectExpired_Implementation(TSubclassOf<class UReadyOrNotGameplayEffect> InPlayerEffectClass);

	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Effect")
	TArray<class UBasePlayerEffect*> PlayerEffects;
	
	UPROPERTY(Instanced, EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effect")
	class UBasePlayerEffect* RecoilNerfEffect = nullptr;
	
	// Authoritative locking of all actions
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_LockAllActions();
	virtual void Server_LockAllActions_Implementation() { LockAllActions(); }
	virtual bool Server_LockAllActions_Validate() { return true; }

	// Authoritative unlocking of all actions
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_UnlockAllActions();
	virtual void Server_UnlockAllActions_Implementation() { UnlockAllActions(); }
	virtual bool Server_UnlockAllActions_Validate() { return true; }

	// Client requests locking of movement + "actions" (not camera movement)
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_LockMovementAndActions();
	virtual void Server_LockMovementAndActions_Implementation() { LockMovementAndActions(); }
	virtual bool Server_LockMovementAndActions_Validate() { return true; }

	// Client requests locking of movement (not camera movement)
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_LockMovement();
	virtual void Server_LockMovement_Implementation() { LockMovement(); }
	virtual bool Server_LockMovement_Validate() { return true; }

	// Client requests un-locking of movement (not camera movement)
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_UnlockMovement();
	virtual void Server_UnlockMovement_Implementation() { UnlockMovement(); }
	virtual bool Server_UnlockMovement_Validate() { return true; }

	// Client requests unlocking of movement + "actions" (not camera movement)
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_UnlockMovementAndActions();
	virtual void Server_UnlockMovementAndActions_Implementation() { UnlockMovementAndActions(); }
	virtual bool Server_UnlockMovementAndActions_Validate() { return true; }

	// Client requests locking of camera movement
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_LockAim();
	virtual void Server_LockAim_Implementation() { LockAim(); }
	virtual bool Server_LockAim_Validate() { return true; }

	// Client requests unlocking of camera movement
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_UnlockAim();
	virtual void Server_UnlockAim_Implementation() { UnlockAim(); }
	virtual bool Server_UnlockAim_Validate() { return true; }
	
	// Client requests locking of player movement and camera movement
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_LockMovementAndAim();
	virtual void Server_LockMovementAndAim_Implementation() { LockMovement(); LockAim(); }
	virtual bool Server_LockMovementAndAim_Validate() { return true; }

	// Client requests unlocking of player movement and camera movement
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Gameplay)
			void Server_UnlockMovementAndAim();
	virtual void Server_UnlockMovementAndAim_Implementation() { UnlockMovement(); UnlockAim(); }
	virtual bool Server_UnlockMovementAndAim_Validate() { return true; }

	UPROPERTY(BlueprintReadWrite, Category = "Paperdoll")
	uint8 bOverrideHeadwearPaperdollTexture : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Paperdoll")
	UTexture2D* HeadwearPaperdollTexture_Override = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Paperdoll")
	UTexture2D* HeadwearPaperdollTexture_Crouch_Override = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Loadout")
	FLoadout DefaultItems;
	
	UPROPERTY(BlueprintReadOnly, Category = Items)
	int32 EquipIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player | Loadout")
	TArray<FLoadout> RandomLoadoutSelection;
	
	UPROPERTY(EditAnywhere, Category = Camera)
	TSubclassOf<ULegacyCameraShake> ForwardShake;

	UPROPERTY(EditAnywhere, Category = Camera)
	TSubclassOf<ULegacyCameraShake> RightShake;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponFireModeChanged, APlayerCharacter*, PlayerCharacter, EFireMode, NewFireMode, EFireMode, LastFireMode);
	UPROPERTY(BlueprintAssignable, Category = "Gameplay")
	FOnWeaponFireModeChanged OnWeaponFireModeChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, ABaseWeapon*, Weapon);
	UPROPERTY(BlueprintAssignable)
	FOnWeaponFired OnWeaponFired;
	
	FTimerHandle TH_LogDryFireAbuse_Civilian;
	FTimerHandle TH_LogDryFireAbuse_Suspect;
	
	// Updates the visibility of certain things within the owner's Picture in Picture sights (should only be called on locally owned player)
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void UpdatePictureInPictureVisibility();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
			void OnItemSelectionStyleChanged(EItemSelectionInterfaceType NewItemSelectionInterface);
	virtual void OnItemSelectionStyleChanged_Implementation(EItemSelectionInterfaceType NewItemSelectionInterface);
	
	UFUNCTION(BlueprintImplementableEvent, Category = Inventory)
	void OnSelectDevicePressed(FKey Key);

	UFUNCTION(BlueprintImplementableEvent, Category = Inventory)
	void OnSelectDeviceReleased(FKey Key);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Input)
			void Server_EquipMultitool(EMultitoolFunctions MultitoolFunction);
	virtual void Server_EquipMultitool_Implementation(EMultitoolFunctions MultitoolFunction);
	virtual bool Server_EquipMultitool_Validate(EMultitoolFunctions MultitoolFunction) { return true; }

	UFUNCTION(BlueprintCallable, Category = "Wedge")
	void JamDoor(ADoor* Door);
	
	UFUNCTION(BlueprintCallable, Category = "Wedge")
	void C2Door(ADoor* Door);

	UFUNCTION(BlueprintPure, Category = Door)
	bool HasC2();

	UFUNCTION(BlueprintPure, Category = Door)
	bool HasBSG();

	UFUNCTION(BlueprintPure, Category = Door)
	bool HasOptiwand();

	UFUNCTION(BlueprintPure, Category = Door)
	bool HasWedge();

	UFUNCTION(BlueprintPure, Category = Door)
	bool HasLockpick();
	
	UFUNCTION(BlueprintPure, Category = Headgear)
    bool HasNVG();

	// ICanUseMultitoolOn implementation
	////////////////////////////////////////
	virtual bool CanUseMultitoolNow_Implementation(class AReadyOrNotCharacter* ToolOwner, class AMultitool* Tool, FHitResult TraceHit) override;
	virtual void Server_FinishedUsingMultitool_Implementation(class AReadyOrNotCharacter* ToolOwner) override;
	virtual bool CanCancelMultitoolAction_Implementation() override { return true; }
	virtual float GetMultitoolUseTime_Implementation() override;
	virtual EMultitoolFunctions GetMultitoolUseType_Implementation() override;
	////////////////////////////////////////

	UPROPERTY()
	ABloodPool* BloodPool = nullptr;

	/* checks to see if montage is playing, if NULL supplied will check if ANY montage playing */
	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool Is1PMontagePlaying(UAnimMontage* Montage) const;
	
	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool IsAny1PMontagePlaying() const;

	/* how far does the mesh space move when rotating */
	UPROPERTY(EditDefaultsOnly, Category = MeshSpace)
		FVector CameraRotationRateMeshSpaceMultiplier = FVector(1.0f);

	/* how far the mesh space moves when recoil is applied */
	UPROPERTY(EditDefaultsOnly, Category = MeshSpace)
		FVector MeshspaceRecoilMovementMultiplier = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, Category = MeshSpace)
		float MeshSpaceMovementMultiplier1P = 0.5f;

	/* used to clamp the recoil values within acceptable limits 
	in case a gun has crazy high recoil you don't want the mesh going off the screen */
	UPROPERTY(EditDefaultsOnly, Category = MeshSpace)
		FVector MeshspaceRecoilMovementMinMax = FVector(10.0f);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, UInteractableComponent*, InteractableComp);
	/* used for things like opening doors, arresting suspects, etc */
	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnInteract OnInteract;

	/* used for things like firing a gun, or primary use for other items */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	virtual void PrimaryUse();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUseStart, ABaseItem*, Item);
	/* used to propagate the primary use event of an item to the player character level */
	FOnItemUseStart OnItemUseStart;

	UFUNCTION()
	void OnItemPrimaryUse(ABaseItem* Item);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUseCompleted, ABaseItem*, Item);
	/* used to propagate the primary use completed event of an item to the player character level */
	FOnItemUseCompleted OnItemUseCompleted;

	UFUNCTION()
	void OnItemPrimaryUseCompleted(ABaseItem* Item);
	
	virtual void PrimaryUseAxis(float AxisValue);

	uint8 bTriggerAxisPressed : 1;

	bool bHoldingPrimaryUse = false;
	float TimeHoldingPrimaryUse = 0.0f;

	bool bForceFireEvenWhenDead = false;

	/** Primary Use */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = Gameplay)
		void Server_PrimaryUse();
	virtual void Server_PrimaryUse_Implementation();
	virtual bool Server_PrimaryUse_Validate() { return true; }

	/* variable for our animgraph to check, used to put the state in a wepaon down*/
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
		bool bWeaponDown3P = false;

	/* variable for our animgraph to check which sprint anim to use based on the body armour currently worn. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Equipment)
		bool bIsWearingHeavyArmour = false;

	/* not used anymore due to fire_single just being called insstead.
	Keep for lagacy reasons, used for the animgraph to move guns into a FIRE LOOP state */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Gameplay)
	bool bFireLoop;

	UPROPERTY(BlueprintReadOnly)
	FTimerHandle FullAutoLoop_Handle;

	bool bIsFullAutoFiring = false;
	float TimeUntilNextFullAutoFiring = 0.0f;

	UPROPERTY(BlueprintReadOnly)
		float TimeSinceAiming = 999.0f;

	int32 burstFireCount = 0;
	FTimerHandle BurstLoop_Handle;

	/* when the player stops primary use (aka release left mouse) */
	void EndPrimaryUse();

	/** End Primary Use (mouse released etc)*/
	UFUNCTION(Server, Reliable, WithValidation)
			void Server_EndPrimaryUse();
	virtual void Server_EndPrimaryUse_Implementation();
	virtual bool Server_EndPrimaryUse_Validate() { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation)
		void Server_UpdateIsBlockingAnimationPlaying(bool bIsBlockingAnimationPlaying);
	virtual void Server_UpdateIsBlockingAnimationPlaying_Implementation(bool bIsBlockingAnimationPlaying);
	virtual bool Server_UpdateIsBlockingAnimationPlaying_Validate(bool bIsBlockingAnimationPlaying) { return true; }

	UPROPERTY(Replicated)
	bool bServerIsBlockingAnimationPlaying = false;

	UFUNCTION(BlueprintCallable, Category = "Lockpicking")
	void StartLockPicking(AActor* Target);
	UFUNCTION(BlueprintCallable, Category = "Lockpicking")
	void StopLockPicking(AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category = "Multitool")
	void StartUsingMultitool(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Multitool")
    void StopUsingMultitool(AActor* Target);

	// local functions, so blocking animation can be checked
	UFUNCTION(BlueprintCallable)
	void EquipPrimaryItem();
	UFUNCTION(BlueprintCallable)
	void EquipSecondaryItem();
	UFUNCTION(BlueprintCallable)
	void EquipLongTactical();
	UFUNCTION(BlueprintCallable)
	void EquipZipcuffs();
	UFUNCTION(BlueprintCallable)
	void EquipDetonator();
	UFUNCTION(BlueprintCallable)
	void EquipMultitool();
	UFUNCTION(BlueprintCallable)
	void EquipC2();
	UFUNCTION(BlueprintCallable)
	void EquipBreachingShotgun();
	UFUNCTION(BlueprintCallable)
	void EquipBatteringRam();
	UFUNCTION(BlueprintCallable)
	void EquipPepperspray();
	UFUNCTION(BlueprintCallable)
	void EquipDoorJam();
	UFUNCTION(BlueprintCallable)
	void EquipMirrorgun();
	UFUNCTION(BlueprintCallable)
	void EquipFlashbang();
	UFUNCTION(BlueprintCallable)
	void EquipCSGas();
	UFUNCTION(BlueprintCallable)
	void EquipStinger();

	UFUNCTION(BlueprintCallable)
	ABaseItem* EquipItemOfType(EItemCategory ItemCategory);
	
	UFUNCTION(BlueprintCallable, DisplayName = "Equip Item From Group (Index)")
	ABaseItem* EquipItemFromGroup_Index(int32 GroupIndex, int32 ItemCategoryIndex = -1);
	UFUNCTION(BlueprintCallable, DisplayName = "Equip Item From Group (Name)")
	ABaseItem* EquipItemFromGroup_Name(FName GroupName, int32 ItemCategoryIndex = -1);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemGroupSelection, int32, GroupIndex, int32, CategoryIndex);
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category = "UI")
	FOnItemGroupSelection OnItemGroupSelection_Pressed;
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category = "UI")
	FOnItemGroupSelection OnItemGroupSelection_Held;
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category = "UI")
	FOnItemGroupSelection OnItemGroupSelection_Released;
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category = "UI")
	FOnItemGroupSelection OnItemGroupSelection_ItemChanged;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bItemGroupSelectionHeld = false;

	uint8 LastSelectedItemGroupIndex = -1;

	UFUNCTION(BlueprintCallable)
	void ToggleUnderbarrelAttachment();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttachmentLightToggled);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAttachmentLightToggled OnAttachmentLightToggled;

	void DoubleTapUse();
	void EndUse();

	/* used for secondary actions on items etc for like ADS */
	UFUNCTION(BlueprintCallable, Category = Player)
	virtual void SecondaryUse();

	/** Secondary Use */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SecondaryUse();
	virtual void Server_SecondaryUse_Implementation();
	virtual bool Server_SecondaryUse_Validate() { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_UpdateADS(bool bADS);
	virtual void Server_UpdateADS_Implementation(bool bADS);
	virtual bool Server_UpdateADS_Validate(bool bADS) { return true; }

	FTimerHandle SecondaryUse_Handle;
	/** Same as Secondary Use but repeats while key is held down */
	void SecondaryUse_Repeat();

	bool bSecondaryUsePressed = false;

	void DoToggleLeanRight();
	void DoToggleLeanLeft();

	void ToggleADS();
	bool bToggled = false;

	void ToggleAimDownSights();
	void DoAimDownSights();
	
	/* Called when secondary use key is released */
	UFUNCTION(BlueprintCallable)
	virtual void EndSecondaryUse();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMelee);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMelee OnMelee;

	UFUNCTION(BlueprintCallable)
	void Melee();
	
	UFUNCTION(BlueprintCallable)
	void Ping();
	
	UFUNCTION(Server, Unreliable, WithValidation)
    void Server_Ping();
    void Server_Ping_Implementation();
    bool Server_Ping_Validate() { return true; }
	
	UFUNCTION(BlueprintCallable)
	bool CanPingActor(AActor* Actor) const;

	bool bWaitingForDoubleTapMelee = false;
	float DoubleTapTimeRemaining = 0.0f;
	
	/* our default FOV, all math operations are based off this value.
	Should be loaded from settings or set in BP*/
	float DefaultFoV;

	/* used to apply FOV changes over time*/
	void ApplyFoV(float DeltaSeconds);

	// The FOV factor to apply when we are sprinting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	float SprintFovFactor = 1.1f;

	// How fast we apply the sprint FOV factor to the camera
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	float SprintFovInterpTime = 8.0f;

	// How fast we return from the sprint FOV
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay)
	float DefaultFovInterpTime = 10.0f;
	
	bool bOverrideFov = false;
	float FovOverride = 90.0f;
	
	/* changed fire mode on our weapons */
	virtual void FireSelect();
	virtual void FireSelectReleased();
	FTimerHandle FireSelect_Handle;
	virtual void SafeMode();

	UFUNCTION(BlueprintCallable)
	EFireMode GetFiringMode();

	UFUNCTION(BlueprintCallable)
	void CycleFireMode();

	UFUNCTION(BlueprintPure)
	bool EquippedWeaponHasFireModes();
	
	// Switches the ammo type for the currently equipped weapon, if available
	void SwitchAmmoType();
	void SwitchAmmoType(FName AmmoType);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSwitchAmmoType, APlayerCharacter*, PlayerCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponSwitchAmmoType OnWeaponSwitchAmmoType;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartChemThrow, APlayerCharacter*, PlayerCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStartChemThrow OnChemlightThrown;

	UFUNCTION(BlueprintCallable, Category = "Chemlight")
	void StartChemThrow();

	bool bPendingChemlightThrow = false;

	UPROPERTY()
	class APlacedC2Explosive* PendingC2Removal = nullptr;

	UFUNCTION(Client, Reliable)
	void Client_OnBeginRemoveC2(APlacedC2Explosive* C2);

	// Has either successfully removed, or stopped early
	UFUNCTION(Client, Reliable)
	void Client_OnEndRemoveC2();

	UFUNCTION()
	void RemovePendingC2();

	/* checks if any animation is playing on the 1P mesh */
	bool IsAnyAnimationPlaying();

	// The current grenade that is equipped to the quickthrow slot
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	class ABaseGrenade* QuickThrowItem;

	bool bStartedQuickThrow = false;
	UFUNCTION(BlueprintPure, Category = Gameplay)
	bool CanQuickThrow();
	
	// Called when we are quick throwing a grenade
	UFUNCTION(BlueprintCallable, Category = Gameplay)
		void StartQuickThrow();

	// Whether we are quick throwing
	UPROPERTY(BlueprintReadOnly, Category = Gameplay)
	bool bQuickThrowing = false;

	bool bTryEndQuickThrow = false;
	float EndQuickThrowTime = 0.0f;
	
	// Called when we have stopped quick throwing a grenade
	UFUNCTION(BlueprintCallable, Category = Gameplay)
        void EndQuickThrow();

	void DoEndQuickThrow();

	// Client-side: Issue something new for quick throw
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = Gameplay)
		void Client_AutoSelectNewQuickthrowItem(ABaseGrenade* CallingGrenade = nullptr);
	virtual void Client_AutoSelectNewQuickthrowItem_Implementation(ABaseGrenade* CallingGrenade = nullptr);
	
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Weapon Clearing")
	float ClearingScore = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float AimTime = 0.0f;

	float LastFireAttemptTime = 0.0f;

	bool bHasPlayedLongAimTimeAnnouncement = false;

	bool bWasAiming;

	/* used to put certain player  animgraphs (shotguns) into reload loop state */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bReloadLoop;

	UFUNCTION()
	virtual void Reload();

	//virtual void CancelReload();

	UFUNCTION()
		void ReloadOrMagCheck();

	UFUNCTION()
		void ReloadOrMagCheck_Released();

	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void ReplenishAllMagazineAmmo();
	
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void ReplenishAllGrenadeAmmo();

	bool bFireSelectHeld = false;
	float FireSelectHeldTime = 0.0f;
	bool bReloadHeld = false;
	float ReloadHeldTime = 0.0f;

	void TacticalReload();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadSignature, APlayerCharacter*, PlayerCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponReloadSignature OnWeaponReload;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTacticalReloadSignature, APlayerCharacter*, PlayerCharacter);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponTacticalReloadSignature OnWeaponTacticalReload;

	FTimerHandle ReloadOrMagCheck_Handle;

	UFUNCTION()
	void MagCheck();

	// Flag to show the mag check UI after a reload, set when switching ammo types
	bool bShowMagCheckAfterReload = false;

	UFUNCTION(BlueprintPure, Category = "Fire Select")
	bool IsFireModeSelectPlaying() const;

	UFUNCTION(BlueprintPure, Category = "Fire Select")
	bool IsMagCheckPlaying() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponMagCheckSignature, ABaseMagazineWeapon*, MagazineWeapon);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponMagCheckSignature OnWeaponMagCheck;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MarkWeaponCleaned(ABaseItem* Item);

	/** Called when current reload is complete */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = ReadyOrNot)
	void Server_OnReloadComplete();
	virtual void Server_OnReloadComplete_Implementation();
	virtual bool Server_OnReloadComplete_Validate() { return true; }

	FGenericTeamId TeamId;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ForwardStrafeSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float BackwardStrafeSpeedMultiplier = 0.85f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SideStrafeSpeedMultiplier = 0.85f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float LeanSpeedMultiplier = 0.85f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SpeedPercentLossPerLegInjury = 0.10f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SpeedPercentLossWhenCarrying = 0.10f;
	
	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	void AddYaw(float Rate);
	 
	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate);

	void AddPitch(float Rate);

	UFUNCTION(BlueprintImplementableEvent, Category = Turns)
		void OnTurn();

	UFUNCTION(BlueprintImplementableEvent, Category = Jump)
		void OnJumpStart();

	UFUNCTION(BlueprintImplementableEvent, Category = Jump)
		void OnJumpLand();

	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetFreelookPitchMax(float NewPitchMaxValue);
	
	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetFreelookPitchMin(float NewPitchMinValue);
	
	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetFreelookYawMax(float NewYawMaxValue);
	
	UFUNCTION(BlueprintCallable, Category = Camera)
	void SetFreelookYawMin(float NewYawMinValue);
	
	UFUNCTION(BlueprintPure, Category = Camera)
	FCameraFreelookSetting GetCurrentFreelookSettings() const { return FreelookSetting; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera)
	FCameraFreelookSetting FreelookSetting;

	void DoVaultTraces();
	bool bCanVault = false;

	UPROPERTY()
	FHitResult VaultTraceForward;
	UPROPERTY()
	FHitResult VaultTraceDownClose;
	UPROPERTY()
	FHitResult VaultTraceDownMiddle;
	UPROPERTY()
	FHitResult VaultTraceDownFar;

	UPROPERTY(BlueprintReadOnly, Category = Vault)
	bool bLedgeFound;

	UPROPERTY(BlueprintReadWrite, Category = Vault)
	bool bVaulting = false;

	FVector Ledge;
	FVector LedgeWallNormal;
	FVector LedgeTraceDown;
	float LedgeZ;
	ELedgeHeight LedgeHeight;
	ELedgeWidth LedgeWidth;

	UFUNCTION(BlueprintImplementableEvent, Category = "Vaulting")
		void PlayVaultAnimation(FVector ledge, FVector ledgeWallNormal, FVector ledgeTraceDown, float ledgeZ, ELedgeWidth ledgeWidth, ELedgeHeight ledgeHeight);

		UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlayVaultAnimation(FVector ledge, FVector legeWallNormal, FVector ledgeTraceDown, float ledgeZ, ELedgeWidth ledgeWidth, ELedgeHeight ledgeheight);

	UPROPERTY(BlueprintReadWrite, Category = "Montage")
	UAnimMontage* LastPlayedVaultMontage = nullptr;


	void PlayerJump();

	float JumpDelayTimer = 0.0f;

	void PlayerStopJumping();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Incremental Use")
			bool CanUseIncrementalSystem() const;
	virtual bool CanUseIncrementalSystem_Implementation() const;

	/* our incremental use system, called from input 
	used for adjusting doors incrementally or walk speed*/
	UFUNCTION(BlueprintCallable, Category = Input)
	void IncrementalUse(float Val);

	UFUNCTION(BlueprintCallable, Category = Input)
	void Drone_MoveForward(float Val);
	UFUNCTION(BlueprintCallable, Category = Input)
	void Drone_Right(float Val);
	UFUNCTION(BlueprintCallable, Category = Input)
	void Drone_Throttle(float Val);
	UFUNCTION(BlueprintCallable, Category = Input)
	void Drone_Yaw(float Val);
	UFUNCTION(BlueprintCallable, Category = Input)
	void Drone_Steady();

	void ApplyCameraLocation(float DeltaSeconds);

	// Our current runspeed, replicated
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_RunSpeedUpdate, Category = "Player|Movement")
	float RunSpeed;

	// Default max acceleration, before being modified by armour
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float MaxAcceleration = 500.0f;

	// The speed percent when using aim focus
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_AimFocus = 0.25f;

	// The speed percent when using ADS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_Aim = 0.5f;

	// The speed percent when using crouch
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_Crouch = 0.5f;

	// The speed percent when using sprint (minimum)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_Sprint = 1.2f;

	// The speed percent when using sprint (maximum)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_SprintMax = 1.3f;

	// How fast it takes to get from minimum sprint speed to maximum sprint speed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SpeedModifier_SprintTime = 5.0f;

	UFUNCTION()
	void OnRep_RunSpeedUpdate();

private:
	UPROPERTY(Replicated, EditAnywhere)
	float HitSpeedMultiplier = 1.0f;
	
	UPROPERTY(Replicated, EditAnywhere)
		float SlowDownSpeedMultiplier = 1.0f;

	UPROPERTY(Replicated)
	float WalkSpeedRampMultiplier = 1.0f;

	UPROPERTY(Replicated)
	float SprintSpeedRampUpMultiplier = 1.0f;

public:
	void SetSlowDownSpeed(float SpeedMultiplier = 1.0f);
	void EndSlowDownSpeed();

	// 0 to 1 = 0% to 100%
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
		float CurrentRunSpeedPercent = 1.0f;

	// used for moving the gun back and forward and to onl update it when you're not ADS
	float LastNonADSRunSpeedPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
		float MaxRunSpeedPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
		float MaxCrouchRunSpeedPercent = 0.5f;

	UFUNCTION(BlueprintCallable)
	void SetMaxRunSpeed(float newMaxSpeed);
	UFUNCTION(BlueprintCallable)
	void SetRunSpeed(float newRunSpeed);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
		float MinWalkSpeedPercent = 0.15f;

	UFUNCTION(Server, Unreliable, WithValidation, BlueprintCallable, Category = Gameplay)
		void Server_SetWalkSpeed(float newWalkSpeed, float newCrouchWalkSpeed);
	virtual void Server_SetWalkSpeed_Implementation(float newWalkSpeed, float newCrouchWalkSpeed);
	virtual bool Server_SetWalkSpeed_Validate(float newWalkSpeed, float newCrouchWalkSpeed) { return true; }

	UFUNCTION(Client, Unreliable)
		void Client_SetWalkSpeed(float newWalkSpeed, float newCrouchWalkSpeed);

	void SetWalkSpeed(float newWalkSpeed, float newCrouchWalkSpeed);

	// we add a slight delay to setting the walk speed so if a player spam adjust it doesn't send heaps of commands to the server, just the last one..
	FTimerHandle SetWalkSpeedServer_Handle;
	FTimerHandle SetWalkSpeedClient_Handle;

	/* last set run speed from the mouse wheel used for returning runspeed after sprint/walk/crouch */
	UPROPERTY(Replicated, BlueprintReadOnly)
	float LastSetRunSpeed = 0.5f;

	UFUNCTION(Server, Unreliable, WithValidation)
		void Server_UpdateLastSetRunSpeed(float newRunSpeed);
	virtual void Server_UpdateLastSetRunSpeed_Implementation(float newRunSpeed);
	virtual bool Server_UpdateLastSetRunSpeed_Validate(float newRunSpeed) { return true; }

	/* multiplier to apply when walking (or not sprinting) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
		float WalkSpeedMultiplier = 0.25f;

	// Used for applying a custom speed to the player for all conditions. default = 1.0x ie. no change.
	UPROPERTY(BlueprintReadOnly, Category = "Player|Movement")
		float DeployableWalkSpeedMultiplier = 1.0f;

	void Walk();
	
	void ToggleWalk();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Walk();
	virtual void Server_Walk_Implementation();
	virtual bool Server_Walk_Validate() { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_ToggleWalk();
	virtual void Server_ToggleWalk_Implementation();
	virtual bool Server_ToggleWalk_Validate() { return true; }

	void Sprint();

	void FastWalk();
	void EndFastWalk();

	UPROPERTY(Replicated)
	bool bHoldingFastWalk;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FastWalk();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EndFastWalk();

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsHoldingFastWalk() const { return bHoldingFastWalk; }
	
	void TryRepeatSprint();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Sprint();
	virtual void Server_Sprint_Implementation();
	virtual bool Server_Sprint_Validate() { return true; }

	UFUNCTION(BlueprintPure, Category = "Sprinting")
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Sprinting")
		bool IsHoldingSprint() const { return bHoldingSprint; }

	UPROPERTY(BlueprintReadOnly, Category = "Sprinting")
		bool bHoldingSprint;

	float SprintMaxTurnRateLeft = 1.0f;
	float SprintMaxTurnRateRight = 1.0f;
	float SprintVectorOffset;
	
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	uint8 bDisableSprinting : 1;

	UFUNCTION(BlueprintCallable)
	void ToggleCrouch();

	void DoCrouch();
	void EndCrouch();

	UFUNCTION(BlueprintCallable)
	void ToggleSprint();

	UFUNCTION(BlueprintCallable)
	void ToggleFreeLook();

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Gameplay)
	bool bWalking = true;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = Gameplay)
	bool bAllowPlacement;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	void OnChatPressed();
	virtual void OnChatPressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	void OnTeamChatPressed();
	virtual void OnTeamChatPressed_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void FireLaserEyes();
	
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DrawPermanentMarker();

	/* keep track of our default camera position from spawn. used to apply leans etc to */
	FVector DefaultRelativeLocation;
	/* our crouching camera position. Used to apply leans etc too*/
	FVector LowerRelativeLocation;

	FRotator LastFrameControlRotation;
	FVector InertiaDragAimLocation;
	FRotator InertiaDragAimRotation;
	FVector InertiaDragStrafeLocation;
	FRotator InertiaDragStrafeRotation;

	/* how quickly our meshspace will interpolate when it is changed */
	UPROPERTY(EditAnywhere, Category = Animation)
		float MeshspaceInterp = 8.0f;
	/* how much the camera will roll with left and right movement */
	UPROPERTY(EditAnywhere, Category = Animation)
		float VelocityCameraRollMultiplier = 0.01f;

	/* how much recoil we should apply to the camera (over time)*/
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	FRotator PendingRecoil;
	FRotator AccumulatedPendingRecoil;

	/* the spped the recoil is applied */
	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	float RecoilSpeed = 2.0f;
	
	/* actually apply the recoil */
	void ApplyRecoil(float DeltaSeconds);

	void RecalculatePendingRecoil(ABaseMagazineWeapon* Weapon);

	FTimerHandle BlurDuration_Handle; 

	UPROPERTY()
	UAnimMontage* Last1PMontage;


	UFUNCTION(BlueprintCallable, Category = Animation)
	void StopFPMontageFromTable(const FString& Animation, float BlendoutTime = 0.0f);
	UPROPERTY()
	TMap<FString, UAnimMontage*> PlayedTableMontageMap1P;

	FTimerHandle IsAnimPlaying_Handle;

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = Inventory)
		void Server_ToggleLightByClass(ELightRadialSelection LightType);
	virtual void Server_ToggleLightByClass_Implementation(ELightRadialSelection LightType);
	virtual bool Server_ToggleLightByClass_Validate(ELightRadialSelection LightType) { return true; }

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_ToggleLaserLight();
	virtual void Multicast_ToggleLaserLight_Implementation();

	UFUNCTION(Client, Unreliable)
	void Client_PlayPostProcessEffect(const FName& InPostProcessEffect, AActor* DamageCauser);
	void Client_PlayPostProcessEffect_Implementation(const FName& InPostProcessEffect, AActor* DamageCauser);
	
	void PlayArmourRelatedEffects(ABaseArmour* Armour, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayArmourRelatedEffects(ABaseArmour* Armour, UParticleSystem* Particle, const FTransform& AtTransform);

	UFUNCTION()
		void OnRep_UpdateAnimInstance();

	void HandleMovement();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Movement")
	UFMODEvent* JumpStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Movement")
	UFMODEvent* JumpLandSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Mix")
	UFMODSnapshot* InMix;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Mix")
	UFMODSnapshot* OutMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Mix")
	bool bOutMixPlaying = true;
	

	UPROPERTY(EditAnywhere)
		UFMODEvent* InjuredScreamPVP;


	bool bHasAnnouncedCriticalCondition = false;

	UPROPERTY(EditAnywhere)
	UFMODEvent* DeathScreamPVP;

	UPROPERTY()
	UFMODAudioComponent* InjuredScreamComponent;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayInjuredScream();
	virtual void Multicast_PlayInjuredScream_Implementation();
	float injuredSoundDelay = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
		float MovementRequiredPerSound = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
		float RotationRequiredPerSound = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
		float NegativeVelocityRequiredForLandingSound = 50.0f;

	// Used to play sound effects when rotationexceeds last rotation
	FRotator RotationAtLastSound;
	// Used to play footsteps / sound effects when location exceed last distance
	FVector LocationAtLastSound;
	bool bPlayLandingSound;

	float GetRunSpeed() const;
	float GetMaxAcceleration() const;

	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TArray<FMovementSound> WalkSounds;
	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TSoftClassPtr<class AImpactEffect> WalkSounds_Environmental;
	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TArray<FMovementSound> RunSounds;
	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TSoftClassPtr<class AImpactEffect> RunSounds_Environmental;
	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TArray<FMovementSound> TurningSounds;
	UPROPERTY(EditAnywhere, Category = "Player|Movement|Sounds")
	TArray<FMovementSound> LandingSounds;

	void PlayArrayOfMovementSounds(TArray<FMovementSound> SoundArray);
	void PlayRandomSound(TArray<USoundCue*> SoundArray, FVector Location = FVector::ZeroVector);

	//* Post Process *//

public:

	UFUNCTION(BlueprintImplementableEvent, Category = Damage)
	void OnBulletImpact(float DirectionForward, float DirectionRight);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBulletImpact, float, DirectionForward, float, DirectionRight);
	FOnBulletImpact OnBulletImpacted;

	UFUNCTION(BlueprintCallable, Client, Reliable, Category = Damage)
	void Client_BulletHit(FHitResult BulletImpact);
	virtual void Client_BulletHit_Implementation(FHitResult BulletImpact);

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float HitDirectionForward = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float HitDirectionRight = 0.0f;
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Damage)
		void OnSupression(float Strength);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSupression, float, Strength);
	FOnSupression OnPlayerSupressed;

public:
	
	// The default breath sound effect
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		UFMODEvent* BreathingBaseEvent;

	// Exhaustion level for breathing event (player only)
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
		float ExhaustionLevel = 0.0f;

	// Exhaustion dissipation rate (when not sprinting)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float ExhaustionDissipationRate = 0.05f;

	// Exhaustion increase rate (when sprinting)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float ExhaustionIncreaseRate = 0.1f;

	// Exhaustion threshold (above this amount = exhausted audio)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float ExhaustionThreshold = 0.8f;

	// Current fear level gotten from suppression (player only, affects breathing)
	UPROPERTY(BlueprintReadOnly, Category = "Audio")
		float FearLevel = 0.0f;

	bool bHasPlayedFearAnnouncement = false;

	// Fear dissipation rate
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float FearDissipationRate = 0.05f;

	// How much to scale incoming fear by
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float FearSuppressionScale = 0.1f;

	// The cutoff for feeling fear
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Audio")
		float FearThreshold = 0.75f;

	
	/*
	Third Person Camera addition for testing
	*/

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleThirdPerson();

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleFreeThirdPerson();

	UFUNCTION(Exec, Category = "Console Command")
		void AdjustScopeOffsetVertical(float NewOffset);

	UFUNCTION(Exec, Category = "Console Command")
		void AdjustScopeOffsetHorizontal(float NewOffset);

	void SetThirdPerson(bool bThirdPerson);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Third Person")
		void Multicast_HideThirdPerson();
	virtual void Multicast_HideThirdPerson_Implementation();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Third Person")
		void Multicast_ShowThirdPerson();
	virtual void Multicast_ShowThirdPerson_Implementation();

	UFUNCTION()
		void HidePlayer();

	UFUNCTION()
		void ShowPlayer();
#if WITH_EDITOR
	UFUNCTION(Exec, Category = "Console Command")
		void DoNetUpdate();
#endif


	//////////////////////////////////////////////////////////
	//
	//	CHEAT CODES
	//	FIXME: Migrate these to a proper CheatManager class.

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleGodMode();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
		void Server_ToggleGodMode();
	virtual void Server_ToggleGodMode_Implementation();
	virtual bool Server_ToggleGodMode_Validate()
	{
#if !UE_BUILD_SHIPPING
		return true;
#else
		return false;
#endif
	}

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleFastMovement();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_ToggleFastMovement();
	virtual void Server_ToggleFastMovement_Implementation();
	virtual bool Server_ToggleFastMovement_Validate()
	{
#if !UE_BUILD_SHIPPING
		return true;
#else
		return false;
#endif
	}

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleNoTarget();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_ToggleNoTarget();
	virtual void Server_ToggleNoTarget_Implementation();
	virtual bool Server_ToggleNoTarget_Validate() 
	{
#if !UE_BUILD_SHIPPING
		return true; 
#else
		return false;
#endif
	}

	UFUNCTION(Exec, Category = "Console Command")
	void DebugDetachAllComponentsAndSubComponents();

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleCrosshairOverlay();

	UFUNCTION(Exec, Category = "Console Command")
	void ToggleSightTweaker();

	UFUNCTION(Exec, Category = "Console Command")
		void ForceMaxLODs_Items();

	UFUNCTION(Exec, Category = "Console Command")
		void ResetLODs_Items();

	UFUNCTION(Exec, Category = "Console Command")
		void ForceMaxLODs_Player();

	UFUNCTION(Exec, Category = "Console Command")
		void ResetLODS_Player();

	UPROPERTY(BlueprintReadOnly, Category = Animation)
	bool bIsSightTweakMode;

	UPROPERTY()
	TSubclassOf<UUserWidget> SightTweakerWidgetTemplate;
	UPROPERTY()
	UUserWidget * SightTweakerOverlay;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Animation)
	FVector SightTweakerPosOffset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Animation)
	FRotator SightTweakerRotOffset;

	FTimerHandle TH_LogCharacterUnconscious;
	FTimerHandle TH_LogCharacterDeath;

	UPROPERTY(BlueprintReadOnly)
	float SpawnProtectionTime = 5.0f;

	////////////////////////////////////////////////////////

	/** flag used to toggle third person camera view */
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsThirdPerson;

	UPROPERTY(ReplicatedUsing = OnRep_BaseAimRotation)
	FRotator Server_BaseAimRotation;

	UFUNCTION()
	void OnRep_BaseAimRotation();

	// free aim additions
	
	/* Free Aim Rotation */
	UPROPERTY(BlueprintReadOnly, Category = Aim)
	FRotator FreeAimCache;

	float TimeSinceLastYawInput = 0.0f;
	float TimeSinceLastPitchInput = 0.0f;

	/*
	Are we currenty peformaing a Interaction?
	*/
	UPROPERTY(BlueprintReadOnly, Category = Interaction)
	bool IsPlayingInteraction;

	// Returns the view pitch rotation
	UFUNCTION(BlueprintPure, Category = Aim)
	float GetViewPitch() const;

	FVector MeshPostureLeanOffset;
	FVector MeshWeaponOffset;
	FRotator MeshWeaponRotation;

	bool bOverrideMeshWeaponTransform = false;

	FRotator MeshWeaponFreeAimRotation;

	FVector MeshWeaponLeanOffset;
	FRotator MeshWeaponLeanRotation;

	/** get aim offsets */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	virtual FRotator GetAimOffsets() const;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Locomotion")
	bool bIsStopping;

	virtual void CalcStop(float DeltaSeconds);

	class UAnimMontage* GetCurrentFPMontage();

	virtual void EnableNightVisionGoggles() override;
	virtual void DisableNightVisionGoggles() override;
	
	UFUNCTION(BlueprintCallable)
	void FadeToBlackEnable();

	UFUNCTION(BlueprintCallable)
	void FadeToBlackDisable();

	// Stuff for ladder placement
	UPROPERTY(BlueprintReadWrite)
	class ALadderSnapZone* LadderPlacementZone = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_RemoveLadder(class ATelescopicLadder* Ladder);
	virtual void Server_RemoveLadder_Implementation(class ATelescopicLadder* Ladder);
	virtual bool Server_RemoveLadder_Validate(class ATelescopicLadder* Ladder) { return true; }

	float YawDelta;
	float LastYawCache;
	float YawDeltaSmoothed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float MoveForwardInput;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float MoveRightInput;

	FVector LastPath;

	FVector VelocityLocal;

	FVector GetRefBoneLocalLocation(const USkeletalMeshComponent* TargetMesh, const FName& BoneName) const;

	/** Left foot IK socket name */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		FName LeftFootSocketName = TEXT("foot_LE");

	/** Right foot IK socket name */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		FName RighttFootSocketName = TEXT("foot_RI");

	/** Left foot IK constant enum  */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		EFootConstEnum LeftFootEnum;

	/** Right foot IK constant enum  */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		EFootConstEnum RightFootEnum;

	/** Foot adjust offset for both feet to match on ground */
	UPROPERTY(EditAnywhere, Category = "Customize|IKSetup")
		float FootAdjustOffset;

	/** Add interpolation for smooth movement of feet to reach IK location */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		float FootInterpSpeed;

	/** Add interpolation for smooth rotation of feet to match surface rotation */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		float FootRotationInterpSpeed;

	/** Add interpolation for smooth movement of hip to reach hip adjusted location */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		float HipInterpSpeed;

	/** IK trace distance for check ground bellow capsule collider */
	UPROPERTY(EditAnywhere, Category = "Customize|IKSetup")
		float TraceDistance;

	/** IK trace offset how bellow character pelvis trace will start */
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		float TraceOffset;

	/** IK movement threshold when velocity in XY plane cross this limit character IK will sense movement*/
	UPROPERTY(EditDefaultsOnly, Category = "Customize|IKSetup")
		float VelocityThreshold;
	

	/** Is character moving or not */
	UFUNCTION(BlueprintPure, Category = "IKFunctions")
	bool IsMoving() const;

	/** Is character moving or not */
	UFUNCTION(BlueprintPure, Category = "IKFunctions")
    bool IsMovingForward() const;

	// How much speed to add from incremental use (the "speed bubble" amount)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Speed)
		float SpeedBubbleAmount = 0.2f;

	/* Temp Rotation to keep characters feet locked while standing still to allow aim offset to have yaw rotation properly working */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rotation")
	FRotator CurrentMeshRot;

	/* Per Tick calculation to keep character feet aligned to ground */
	virtual void CalculateMeshRot(float DeltaTime);
	
	bool ResetWeaponStance;

	bool ThirdPersonFreeLook;

	// Ignore Alex's rotation override. Only used by the preview character in CustomizationMenuWorld.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Rotation")
	bool bIgnoreRotationOverride;

private:
	bool bInCinematicSequence = false;
public:

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Animation")
	bool IsInCinematicAnimation() const { return bInCinematicSequence; }
	
	// Trailer stuff
	// Makes the target in front of you instantly surrender
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Press")
	void Server_InstantSurrenderTarget();
	virtual void Server_InstantSurrenderTarget_Implementation();
	virtual bool Server_InstantSurrenderTarget_Validate() { return true; }

	void InstantSurrenderTarget() { Server_InstantSurrenderTarget(); }
	
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "HUD")
			void Client_OnPlayerDamage(bool bTakenDamage, float InDamage, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser);
	virtual void Client_OnPlayerDamage_Implementation(bool bTakenDamage, float InDamage, AReadyOrNotCharacter* InstigatorCharacter, AActor* DamageCauser);
	
#if WITH_EDITOR
	virtual bool IsSelectable() const override { return bAllowSelectable; }

	bool bAllowSelectable = true;

	virtual void ClearCrossLevelReferences() override
	{
		if (bDontSave)
		{
			Destroy();
		}
	};

	bool bDontSave = false;
#endif

	// FriendlyNameInterface
	virtual FString GetFriendlyName_Implementation() override;

	FRotator LeanTempYawCache;

	/* camera proc bob start */
	virtual void TickCameraWeaponBob(float DeltaTime);
	float CameraBobTimer;
	float WeaponBobTimer;

	UPROPERTY(BlueprintReadOnly, Category = "Camera Bob")
	FVector CameraBobTrans;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Bob")
	FVector WeaponBobTrans;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Bob")
	FRotator WeaponBobRot;

	UPROPERTY(BlueprintReadOnly, Category = "Camera Bob")
	FRotator CameraBobRot;

	UPROPERTY(BlueprintReadOnly, Category = "Sights")
	bool bIsSecondarySightActive;

	bool bHadScopeMask;

	bool HasSecondarySight();

	UFUNCTION(BlueprintCallable, Category = "Attachments")
	void ToggleSecondarySight();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCantedSightToggled, bool, bUsingCantedSight);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCantedSightToggled OnCantedSightToggled;

	bool bCantedSightEnabled = false;
	void ToggleCantedSight();

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsTabletFocused() const { return bTabletFocused; }

private:
	bool bTabletButtonHeld = false;
	float TabletFocusTimer = 0.0f;

	bool bTabletFocused = false;
	float TabletLerp = 0.0f;
	
	void TickTabletLogic(float DeltaSeconds);

	void OpenTabletPressed();
	void OpenTabletReleased();
	
	void OpenBuyMenuPressed();

	float CalculateTabletFov() const;
	
public:
	FORCEINLINE bool IsTabletFocused() { return bTabletFocused; }
	
	UFUNCTION(BlueprintCallable)
	void SetTabletFocused(bool bFocused);
	
	UFUNCTION(BlueprintPure, Category = "Attachments")
	bool EquippedWeaponHasLaserAttachment() const;

	UFUNCTION(BlueprintPure, Category = "Attachments")
	bool EquippedWeaponHasLightAttachment() const;

	UFUNCTION(BlueprintPure, Category = "Attachments")
	bool EquippedWeaponHasSecondarySight() const;
	
	UFUNCTION(BlueprintPure, Category = "Grenade")
	bool HasGrenadesInInventory() const;
	
	UFUNCTION(BlueprintPure, Category = "Grenade")
	int32 GetQuickthrowGrenadeAmmo() const;
	
	UFUNCTION(BlueprintPure, Category = "Chemlight")
	bool HasChemlightsInInventory() const;
	
	UFUNCTION(BlueprintPure, Category = "Chemlight")
	int32 GetChemlightAmmo() const;
	
	void EndChemThrow();

	UFUNCTION(BlueprintCallable)
	void SetCommandInterfaceActive(bool CommandInterfaceActive);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSecondarySightToggled, bool, bUsingSecondarySight, ABaseMagazineWeapon*, Weapon);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSecondarySightToggled OnSecondarySightToggled;

	virtual void GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData) override;
	virtual void GatherDebugText_Implementation(FString& OutText) override;
	virtual void DrawVisualDebug_Implementation() override;

	UPROPERTY(BlueprintReadOnly, Category = "Pelvis")
	bool bIsPelvisFPMovementBobActive;

	UPROPERTY(BlueprintReadOnly, Category = "Pelvis")
	float PelvisFPMovementDamping;

	UPROPERTY(EditAnywhere, Category = "FirstPerson")
	bool bCopyTPMeshTransformsToFP;

	FVector OriginalFPMeshRelTranslation;
	FVector FPMeshRelativeTransform;

	virtual bool PlayDeathAnimation() override;
	virtual void Multicast_PlayDeathAnimation_Implementation(UAnimMontage* Montage) override;

	UFUNCTION(BlueprintCallable)
	void ScreenPositionToWeaponFOV(const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection);

	UFUNCTION(BlueprintCallable)
	void TeleportPlayerToLocation(FVector Location);

	UFUNCTION(Server, Reliable)
	void Server_TeleportPlayerToLocation(FVector Location);

	virtual void Restart() override;

	UFUNCTION(Exec)
	void SetAmmo(const FString& AmmoType);

private:
	// Gamepad-related input
	static constexpr float LongPressDuration = 0.5f;
	static constexpr float GamepadLeanDeadzone = 0.2f;
	
	bool HasTactialEquipped();
	void Cycle();
	void EquipCurrentTool();
	void ExecuteGamepadCrouch();
	void ExecuteGamepadReloadLongPress();
	void ExecuteGamepadWeaponCycleLongPress();

	void GamepadTacticalCyclePressed();
	void GamepadTacticalCycleReleased();
	void GamepadAttachmentCyclePressed();
	void GamepadAttachmentCycleReleased();
	void GamepadGrenadeCyclePressed();
	void GamepadGrenadeCycleReleased();
	void GamepadWeaponCyclePressed();
	void GamepadWeaponCycleReleased();
	void GamepadToggleCrouchPressed();
	void GamepadToggleCrouchReleased();
	void GamepadOpenCommandInterfacePressed();
	void GamepadOpenCommandInterfaceReleased();
	void GamepadCloseCommandInterfacePressed();
	void GamepadCloseCommandInterfaceReleased();
	void GamepadCommandInterfaceLeft();
	void GamepadCommandInterfaceRight();
	void GamepadCommandInterfaceUpPressed();
	void GamepadCommandInterfaceUpReleased();
	void GamepadCommandInterfaceDownPressed();
	void GamepadCommandInterfaceDownReleased();
	void GamepadToggleNightvision();
	void GamepadADSPressed();
	void GamepadADSReleased();
	void GamepadToolCyclePressed();
	void GamepadToolCycleReleased();
	void GamepadReloadPressed();
	void GamepadReloadReleased();
	void GamepadAcceptPressed();
	void GamepadAcceptReleased();
	void GamepadDeclinePressed();
	void GamepadDeclineReleased();
	void GamepadItemWheelPressed();
	void GamepadItemWheelReleased();
	void GamepadItemWheelCancelPressed();
	void GamepadItemWheelCancelReleased();
	void GamepadCommandWheelPressed();
	void GamepadCommandWheelReleased();
	void GamepadCommandWheelFreeViewCancel();
	void GamepadCommandWheelFreeViewConfirm();
	void GamepadLeanLeftPressed();
	void GamepadLeanRightPressed();
	// void GamepadTeamViewPressed();
	// void GamepadTeamViewReleased();
	void GamepadMeleePressed();
	void GamepadMeleeReleased();
	void GamepadCycleSwatElementPressed();
	void GamepadCycleSwatElementReleased();
	void GamepadAimCycle();

	UPROPERTY()
	UUserWidget* ScoreboardWidget;
	bool bGamepadScoreboardToggle = false; 

	UPROPERTY()
	UCommandInterface* CommandInterface = nullptr;
	bool bCommandInterfaceActive = false;
	FTimerHandle CommandInterfaceTimerHandle;

	// UPROPERTY()
	// class UItemWheel* ItemWheel = nullptr;
	UPROPERTY()
	bool bItemWheelActive = false;
	FTimerHandle ItemWheelTimerHandle;
	
	UPROPERTY()
	bool bCommandWheelActive = false;
	FTimerHandle CommandWheelTimerHandle;
	
	void GamepadLeanAxis(float Value);

	bool bGamepadLeanActive = false;
	int CurrentTactical = 0;
	int CurrentGrenade = 0;
	int CurrentTool = 0;
	bool bGamepadTacticalCycleActive = false;
	bool bGamepadAttachmentCycleActive = false;
	bool bGamepadAttachmentCycleToggledSafety = false;
	bool bGamepadGrenadeCycleActive = false;
	bool bGamepadReloadActive = false;
	bool bGamepadWeaponCycleActive = false;
	bool bGamepadToggleCrouchActive = false;
	bool bGamepadADSActive = false;
	bool bGamepadToolCycleActive = false;
	bool bGamepadAcceptActive = false;
	bool bGamepadDeclineActive = false;
	ETeamType CurrentTeam = ETeamType::TT_SQUAD;
	FTimerHandle GamepadCrouchTimerHandle;
	FTimerHandle GamepadAttachmentTimerHandle;
	FTimerHandle GamepadWeaponCycleTimerHandle;
	FTimerHandle GamepadReloadTimerHandle;
};
