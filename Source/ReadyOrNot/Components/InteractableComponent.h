// Void Interactive, 2020

#pragma once

#include "Components/WidgetComponent.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "InteractableComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("Interactable Component"), STATGROUP_InteractableComp, STATCAT_Advanced);

USTRUCT(BlueprintType)
struct FPlayerActionPromptSlot
{
	GENERATED_BODY()

	FPlayerActionPromptSlot()
	{
		bAnimate = true;
		bLoopAnimation = false;
		bCondition = false;
		bUseCustomActionText = false;
		bCheckForDisallowedItems = true;
		bUseCustomDisallowedActionText = false;
	}

	void Init(const FName& InInputActionName, const TEnumAsByte<EInputEvent>& InInputEvent, const FText& InActionText, const FString& InColorLabel = "Red")
	{
		InputActionName = InInputActionName;
		InputEvent = InInputEvent;
		ActionText = InActionText;
		ColorLabel = InColorLabel;

		DisallowedItemActionText = InActionText;

		bCondition = true;
		bUseCustomActionText = false;
	}

	bool IsValid() const { return bCondition && (!ActionText.EqualToCaseIgnored(FText::FromString("None")) || !InputActionName.IsNone() || !CustomActionPromptText.EqualToCaseIgnored(FText::FromString("None"))); }
	
	bool IsCustomTextValid() const { return bCondition && bUseCustomActionText && !CustomActionPromptText.EqualToCaseIgnored(FText::FromString("None")); }
	
	// Refer to Project Settings -> Input to get the exact input action name
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName InputActionName = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TEnumAsByte<EInputEvent> InputEvent = IE_Pressed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ActionText = FText::FromString("None");

	// The correct list of color labels can be found in the KeyToActionTextStyle data table
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ColorLabel = "Red";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bUseCustomActionText : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseCustomActionText"))
	FText CustomActionPromptText = FText::FromString("None");
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	uint8 bCheckForDisallowedItems : 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	uint8 bUseCustomDisallowedActionText : 1;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (EditCondition = "bCheckForDisallowedItems && bUseCustomDisallowedActionText"))
	FText DisallowedItemActionText = FText::FromString("None");
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (EditCondition = "bCheckForDisallowedItems"))
	TArray<EItemCategory> DisallowedItems;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bAnimate : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAnimate"))
    uint8 bLoopAnimation : 1;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	uint8 bCondition : 1;
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), HideCategories = ("Interaction", "Physics", "Collision", "Animation", "Navigation", "VirtualTexture", "HLOD", "Mobile", "AssetUserData"))
class READYORNOT_API UInteractableComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UInteractableComponent();

	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void EnableInteractable();
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
    void DisableInteractable();
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void EnableInteractionFor(APlayerCharacter* InCharacter);
	void EnableInteractionFor(APlayerController* InPlayerController);
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void DisableInteractionFor(APlayerCharacter* InCharacter);
	void DisableInteractionFor(APlayerController* InPlayerController);
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void SetInteractionIconState(bool bValid);
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void SetInteractionIconSize(float InInteractCircleSize, float InInteractIconSize = 48.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void ResetToOriginalLocation();

	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void SetAnimatedIconName(const FName& NewIconName);
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsInteractionEnabledFor(APlayerCharacter* InCharacter);
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsInteractionEnabledForController(APlayerController* InController) const;
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool CanInteract(bool bLog = false) const;
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool InputActionNameMatchesAnySlot(FName InInputActionName);
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool InputActionNameMatchesAnyValidSlot(FName InInputActionName);
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	AActor* GetUseActor() const;

	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsFocused() const;
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsIconVisible() const;
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	bool IsBeingLookedAt(APlayerController* InPlayerController, float MaxRange = 1000.0f, float LookatThreshold = 0.97f, bool bUseActorLocation = false);
	
	template<typename T>
	T* GetUseActor() const;

	UFUNCTION(BlueprintPure, Category = "Interactable")
	FORCEINLINE FName GetOriginalIconName() const { return OriginalAnimatedIconName; }
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	FORCEINLINE float GetDistanceFromPlayer() const { return DistanceFromPlayer; }
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	FORCEINLINE FVector GetOriginalLocation() const { return OriginalLocation_World; }
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
    FORCEINLINE FVector GetOriginalLocation_Relative() const { return OriginalLocation_Relative; }

	UFUNCTION(BlueprintPure, Category = "Interactable")
	FORCEINLINE bool AnyActionSlotValid() const { return ActionSlot1.IsValid() || ActionSlot2.IsValid() || ActionSlot3.IsValid() || ActionSlot4.IsValid(); }
	
	UFUNCTION(BlueprintPure, Category = "Interactable")
	FORCEINLINE TArray<APlayerCharacter*> GetPlayersFocusing() const { return PlayersFocusing; }

	UFUNCTION(BlueprintNativeEvent, Category = "Interactable")
	void OnInteract(APlayerCharacter* InteractInstigator);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	uint8 bEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FName AnimatedIconName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bShowActionPromptInWorld = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bShowActionPromptInWorld"))
	bool bEnableActionPromptBackground = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bOverrideActionPromptUserSettings = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	uint8 bShowIconWhenActionsLocked : 1;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	FPlayerActionPromptSlot ActionSlot1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
    FPlayerActionPromptSlot ActionSlot2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
    FPlayerActionPromptSlot ActionSlot3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
    FPlayerActionPromptSlot ActionSlot4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	bool bDistanceChecksEnabled = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled && bDistanceChecksEnabled"))
	float MinShowPromptAtDistance = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled && bDistanceChecksEnabled"))
	float ShowPromptAtDistance = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	uint8 bMustBeLookingAt : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled && bMustBeLookingAt", ClampMin = 0.0f, ClampMax = 1.0f))
	float RequiredLookAtPercentage = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	bool bMustBeOverlapping = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	float InteractCircleSize = 40.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	float InteractIconSize = 38.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	uint8 bDistanceFadeIcon : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	uint8 bHideUponInteraction : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	uint8 bHideUponPlayerMovement : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	uint8 bImprintIconOnHUDUponInteraction : 1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	TArray<APlayerController*> DisallowedPlayerControllers;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "UI", meta = (EditCondition = "bEnabled"))
	TArray<AActor*> IgnoreInteractionBlockingActors;
	
	UPROPERTY(BlueprintReadWrite, Category = "Interactable")
	float CurrentProgress = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Interactable")
	FAnimatedIcon AnimatedIcon;
	
	UPROPERTY(BlueprintReadWrite, Category = "Interactable")
	AActor* UseActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clients", meta = (EditCondition = "bEnabled"))
	bool bClientInteract = false;

	/** If true, sends interact command to server. Can be used in conjunction with bClientInteract to run interact both locally and on server */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clients", meta = (EditCondition = "bEnabled"))
	bool bExecuteOnServer = true;
	
	uint8 bCanHide : 1;
	
	uint8 bOverrideTickInterval : 1;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;

	void ShowActionPrompts(APlayerCharacter* PlayerCharacter);
	void HideActionPrompts(APlayerCharacter* PlayerCharacter, bool bForce = false);

	void HideAllUIElements(bool bForce = false, bool bDestroyWidget = true, bool bResetLastInteractable = false);

	FAnimatedIcon DetermineAnimatedIcon();

	virtual void OnInteract_Implementation(APlayerCharacter* InteractInstigator);
	
private:
	void UpdateIconVisibility(APlayerController* PlayerController, APlayerCharacter* PlayerCharacter, float DeltaTime);

	void AssignActionSlot(UHumanCharacterHUD_V2* HUD, FPlayerActionPromptSlot* InActionSlot, int32 SlotIndex, bool bUsingGamepad);
	void ClearActionSlot(UHumanCharacterHUD_V2* HUD, int32 SlotIndex);
	
	void InitIconWidget();
	void DestroyIconWidget();

	void ImprintIcon();

	bool IsDisallowedItemEquipped(APlayerCharacter* PlayerCharacter) const;

	UPROPERTY()
	TArray<APlayerCharacter*> PlayersFocusing;
	
	UPROPERTY()
	TSubclassOf<UUserWidget> CachedWidgetClass = nullptr;
	
	UPROPERTY()
	class UAnimatedIconWidgetWithActionPrompt* IconWidget = nullptr;

	UPROPERTY()
	TSubclassOf<UUserWidget> ImprintIconWidgetClass = nullptr;

	UPROPERTY()
	class UAnimatedIconWidget_Imprint* IconWidget_Imprint = nullptr;

	UPROPERTY()
	TMap<FName, FAnimatedIcon> AnimatedIconMap;
	
	float LastDotProduct = 0.0f;
	float DistanceFromPlayer = 0.0f;
	bool bIsOverlappingPlayer = false;

	float DesiredRenderOpacity = 0.0f;

	float TimeSinceLastDistanceFadeCheck = 0.0f;

	FPlayerActionPromptSlot* ActiveActionPromptSlot = nullptr;
	
	FName OriginalAnimatedIconName = NAME_None;

	FVector OriginalLocation_World = FVector::ZeroVector;
	FVector OriginalLocation_Relative = FVector::ZeroVector;

	uint8 bPlayerPromptsCleared : 1;
	uint8 bPlayedFocusAnim : 1;
	uint8 bIsFocusedComponent : 1;
	uint8 bHasLineOfSightToUseActor : 1;
	uint8 bDisallowedItemEquipped : 1;
	uint8 bWasDisallowedItemEquipped : 1;
	uint8 bCurrentInteractStateValid : 1;
	uint8 bPreviousInteractStateValid : 1;
	uint8 bUIHidden : 1;

	FTimerHandle TH_UpdateTickRate;
	FTimerHandle TH_CheckLOS;

	//FHitResult LOSHit;
	FHitResult FocusedLOSHit;
};

template <typename T>
T* UInteractableComponent::GetUseActor() const
{
	return Cast<T>(UseActor);
}
