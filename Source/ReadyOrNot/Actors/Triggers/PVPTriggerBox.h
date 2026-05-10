// Void Interactive, 2020

#pragma once

#include "Engine/TriggerBox.h"
#include "PVPTriggerBox.generated.h"

/**
 * Base class for all trigger boxes that are used in PVP gamemodes
 */
UCLASS(HideCategories=("HLOD", "Mobile", "Asset User Data", "Actor", "Rendering", "Physics", "Input", "Cooking", "LOD"))
class READYORNOT_API APVPTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	APVPTriggerBox();

	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    void StartTimerEvent();
	
	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    void CancelTimerEvent();

	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    void PauseTimerEvent();

	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    void ResumeTimerEvent();
	
	UFUNCTION(BlueprintPure, Category = "PVP Trigger Box")
    bool DoesActorHaveAnyAcceptedTags(AActor* OtherActor) const;

	UFUNCTION(BlueprintPure, Category = "PVP Trigger Box")
    bool IsPlayerOnAcceptedTeam(APlayerCharacter* Player) const;

	UFUNCTION(BlueprintPure, Category = "PVP Trigger Box")
    FORCEINLINE float GetCurrentElapsedTime() const { return TimeElapsed; }
	
	UFUNCTION(BlueprintPure, Category = "PVP Trigger Box")
    FORCEINLINE float GetLastElapsedTime() const { return PreviousTimeElapsed; }

	UFUNCTION(BlueprintPure, Category = "PVP Trigger Box")
    bool IsActorInTriggerBox(AActor* InActor) const;

	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    virtual void ToggleObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
	virtual void ShowObjectiveMarker();

	UFUNCTION(BlueprintCallable, Category = "PVP Trigger Box")
    virtual void HideObjectiveMarker();
	
protected:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent)
			void OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	virtual void OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent)
			void OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor);
    virtual void OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent)
			void OnTimerExpired();
	virtual void OnTimerExpired_Implementation();
	
	UFUNCTION(BlueprintNativeEvent)
			void OnRep_CharactersInTriggerBoxUpdated();
	virtual void OnRep_CharactersInTriggerBoxUpdated_Implementation();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PVP Trigger Box")
	UTextRenderComponent* TextRender;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PVP Trigger Box")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CharactersInTriggerBoxUpdated, Category = "PVP Trigger Box|Data")
	TArray<APlayerCharacter*> CharactersInTriggerBox;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "PVP Trigger Box|Data")
	float TimeElapsed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "PVP Trigger Box|Data")
    float PreviousTimeElapsed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "PVP Trigger Box|Data")
	uint8 bEntered : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Settings")
	float TimeNeededToStay_Editor = 3.0f;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Settings")
	float TimeNeededToStay = 30.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "PVP Trigger Box|Data")
	float TimeNeededToStay_Active = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TArray<ETeamType> OnlyAcceptTeams;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> OnlyAcceptActorsWithTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Debug")
	uint8 bLogDebugInfo : 1;

	UPROPERTY(BlueprintReadOnly, Category = "PVP Trigger Box|Data")
	FTimerHandle TH_TimerEventExpiry;
};
