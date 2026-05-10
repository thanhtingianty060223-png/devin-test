// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "SplineTrigger.generated.h"

DECLARE_STATS_GROUP(TEXT("Spline Trigger"), STATGROUP_SplineTrigger, STATCAT_Advanced);

UCLASS(HideCategories=("HLOD", "Mobile", "Asset User Data", "Actor", "Rendering", "Physics", "Input", "Cooking", "LOD", "Collision"))
class READYORNOT_API ASplineTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	ASplineTrigger();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSplineEnclosureEntered, class APlayerCharacter*, PlayerCharacter);
	// Bind a delegate event to notify when the player has entered the spline enclosure
	UPROPERTY(BlueprintAssignable, Category = "Spline Trigger|Events")
	FOnSplineEnclosureEntered Delegate_OnSplineEnclosureEntered;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSplineEnclosureExited, class APlayerCharacter*, PlayerCharacter);
	// Bind a delegate event to notify when the player has exited the spline enclosure
	UPROPERTY(BlueprintAssignable, Category = "Spline Trigger|Events")
	FOnSplineEnclosureExited Delegate_OnSplineEnclosureExited;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Trigger")
	bool bInvertBounds = false;

	// Is the player outside the spline enclosure?
	UFUNCTION(BlueprintPure, Category = "Spline Trigger")
	FORCEINLINE bool IsOutsideSplineEnclosure() const { return bIsOutsideSplineEnclosure; }

	// Is the player inside the spline enclosure?
	UFUNCTION(BlueprintPure, Category = "Spline Trigger")
    FORCEINLINE bool IsInsideSplineEnclosure() const { return !bIsOutsideSplineEnclosure; }

	UFUNCTION(BlueprintPure, Category = "Spline Trigger")
    bool IsActorInsideSplineEnclosure(AActor* InActor) const;

	UFUNCTION(BlueprintPure, Category = "Spline Trigger")
    bool IsActorOutsideSplineEnclosure(AActor* InActor) const;
    
	UFUNCTION(BlueprintCallable, Category = "Spline Trigger")
	void ToggleDrawDebug();
	
	UFUNCTION(BlueprintCallable, Category = "Spline Trigger")
	void EnableTrigger();
	
	UFUNCTION(BlueprintCallable, Category = "Spline Trigger")
    void DisableTrigger();

	UPROPERTY(BlueprintReadOnly, Category = "Spline Trigger")
	FVector LastValidPlayerLocation;
	
protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	// Called when the player has entered the spline enclosure
	UFUNCTION(BlueprintNativeEvent, Category = "Spline Trigger|Events")
			void OnSplineEnclosureEntered(class APlayerCharacter* PlayerCharacter);
	virtual void OnSplineEnclosureEntered_Implementation(class APlayerCharacter* PlayerCharacter);

	// Called when the player has exited the spline enclosure
	UFUNCTION(BlueprintNativeEvent, Category = "Spline Trigger|Events")
			void OnSplineEnclosureExited(class APlayerCharacter* PlayerCharacter);
	virtual void OnSplineEnclosureExited_Implementation(class APlayerCharacter* PlayerCharacter);

	// Draw debug elements such as line traces?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Trigger")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline Trigger")
	uint8 bDrawDebugElements : 1;
	
	// The average of all spline point locations
	UPROPERTY(BlueprintReadOnly, Category = "Spline Trigger")
	FVector AverageSplinePointLocation;

	// Is the player outside the spline enclosure?
	UPROPERTY(BlueprintReadOnly, Category = "Spline Trigger")
	uint8 bIsOutsideSplineEnclosure : 1;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Spline Trigger|Components")
	class USplineComponent* SplineComponent = nullptr;

private:
	uint8 bSplineEnclosureEnteredEventBroadcasted : 1;
	uint8 bSplineEnclosureExitedEventBroadcasted : 1;
	
	void TryBroadcast_SplineEnclosureEntered(class APlayerCharacter* PlayerCharacter);
	void TryBroadcast_SplineEnclosureExited(class APlayerCharacter* PlayerCharacter);
	
	bool ShouldInvertBoundsChecks();
};
