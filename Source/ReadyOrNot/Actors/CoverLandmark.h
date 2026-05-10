// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "CoverLandmark.generated.h"

UENUM(BlueprintType)
enum class ECoverLandmarkAnimDirection : uint8
{
	Forward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class ECoverLandmarkType : uint8
{
	None,
	Closet,
	Bed,
	Table,
	Corner,
	Custom
};

USTRUCT(BlueprintType)
struct FCoverLandmarkAnimData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bForwardOnly : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bFromTable : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bFromTable && bForwardOnly", EditConditionHides))
	FString ForwardAnimRowName = "";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bFromTable && !bForwardOnly", EditConditionHides))
	FString LeftAnimRowName = "";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bFromTable && !bForwardOnly", EditConditionHides))
	FString RightAnimRowName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bFromTable && bForwardOnly", EditConditionHides))
	UAnimMontage* ForwardAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bFromTable && !bForwardOnly", EditConditionHides))
	UAnimMontage* LeftAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bFromTable && !bForwardOnly", EditConditionHides))
	UAnimMontage* RightAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimYawOffset = 0.0f;
};

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ACoverLandmark final : public AActor, public ISecurable
{
	GENERATED_BODY()
	
public:	
	ACoverLandmark();

	UFUNCTION(BlueprintCallable)
	void EnableLandmark();
	UFUNCTION(BlueprintCallable)
	void DisableLandmark();

	UFUNCTION(BlueprintCallable)
	void ToggleLandmarkEnabled(bool bEnable);
	
	UFUNCTION(BlueprintCallable)
	void AddCooldownFor(AController* InController, float InCooldownTime);
	UFUNCTION(BlueprintPure)
	bool IsCooldownActiveFor(const AController* InController) const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* BillboardComponent = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UArrowComponent* ArrowComponent = nullptr;
	#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	uint8 bEnabled : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	FString LandmarkName = "None";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	ECoverLandmarkType Type = ECoverLandmarkType::None;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled && Type == ECoverLandmarkType::Custom", EditConditionHides))
	TSubclassOf<class USearchLandmarkActivity> CustomSearchActivityClass = nullptr;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled && Type == ECoverLandmarkType::Custom", EditConditionHides))
	FString SwatSearchAnimation = "";
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	TSoftObjectPtr<AActor> CoverObject = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled && CoverObject != nullptr", EditConditionHides))
	uint8 bDisableCollision : 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	TArray<ETeamType> AllowedTeamsForCover;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	TArray<TSoftObjectPtr<AStaticMeshActor>> IgnoredMeshActors;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	float CooldownAfterUse = 60.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled"))
	bool bCharacterHiddenInWaitingState = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Exit", meta = (EditCondition = "bEnabled"))
	FTransform ExitTriggerBoxTransform;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Exit", meta = (EditCondition = "bEnabled"))
	FVector ExitTriggerBoxExtent = FVector(35.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Idle", meta = (EditCondition = "bEnabled"))
	FTransform IdleTriggerBoxTransform;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup|Triggers|Idle", meta = (EditCondition = "bEnabled"))
	FVector IdleTriggerBoxExtent = FVector(35.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "bEnabled"))
	FCoverLandmarkAnimData Entry;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "bEnabled"))
	FCoverLandmarkAnimData Loop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "bEnabled"))
	FCoverLandmarkAnimData Exit;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "bEnabled"))
	uint8 bAllowAbruptExit : 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Proxy", meta = (EditCondition = "bEnabled"))
	TArray<class ACoverLandmarkProxy*> EntryPoints;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Proxy", meta = (EditCondition = "bEnabled"))
	TArray<class ACoverLandmarkProxy*> ExitPoints;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Proxy", meta = (EditCondition = "bEnabled"))
	class ACoverLandmarkProxy* IdlePoint = nullptr;

	// The current controller that is using this landmark
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data", meta = (DisplayAfter = "IdlePoint"))
	AController* OccupiedByController = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data", meta = (DisplayAfter = "OccupiedByController"))
	AController* LastUsedByController = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	AReadyOrNotCharacter* CurrentSwatWithLineOfSight = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	bool bClearedBySwat = false;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditMove(bool bFinished) override;

	bool IsAnyProxySelected() const;
	#endif

	UFUNCTION(CallInEditor, Category = "Preview")
	void PreviewEntryAnim();
	UFUNCTION(CallInEditor, Category = "Preview")
	void PreviewIdleAnim();
	UFUNCTION(CallInEditor, Category = "Preview")
	void PreviewExitAnim();

	// ISecureable Interface
	virtual bool IsSecured_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;

	#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class ACoverLandmarkProxy* IdlePoint_EditorCopy = nullptr;

	UPROPERTY(EditInstanceOnly, Category = "Preview")
	USkeletalMesh* PreviewMesh = nullptr;
	
	UPROPERTY(EditInstanceOnly, Category = "Preview")
	UAnimSequenceBase* IdleAnim = nullptr;
	#endif

	UPROPERTY()
	TMap<AController*, float> CooldownMap;

private:
	#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class ASkeletalMeshActor* PreviewSkeletalMesh = nullptr;

	void PreviewAnim(ACoverLandmarkProxy* Proxy, UAnimSequenceBase* Anim, float YawOffset = 0.0f);

	UPROPERTY()
	UTexture2D* HidingIcon = nullptr;
	UPROPERTY()
	UTexture2D* HidingIcon_Off = nullptr;
	#endif
};
