// Copyright Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "UseabilityInterface.generated.h"

USTRUCT(BlueprintType)
struct FAnimatedIcon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated Icon")
	TArray<UTexture2D*> Icons;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animated Icon")
    float FrameRate = 0.0f;

	friend bool operator !=(const FAnimatedIcon& LHS, const FAnimatedIcon& RHS)
	{
		return 	LHS.Icons != RHS.Icons ||
				LHS.FrameRate != RHS.FrameRate;
	}
};

USTRUCT(BlueprintType)
struct FAnimatedIconTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Icon")
	FAnimatedIcon AnimatedIcon;
};

/**
 * 
 */
UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UUseabilityInterface : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IUseabilityInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	void Interact(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	void EndInteract(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	void DoubleTapInteract(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    void MeleeInteract(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    void Fire(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    void EndFire(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	void OnFocusGain(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	void OnFocusLost(class AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	class UInteractableComponent* GetInteractableComponent() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    EInputEvent DetermineInputEvent() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	FName DetermineAnimatedIcon() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    FText DetermineActionText() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	float DetermineInteractionDistance() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	float DetermineCurrentProgress() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
	bool CanInteract() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Useability Interface")
    bool CanInteractThroughHitActors(const FHitResult& Hit) const;
};

