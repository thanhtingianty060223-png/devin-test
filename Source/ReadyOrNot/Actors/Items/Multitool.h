// Void Interactive, 2020

#pragma once

#include "Tool.h"

#include "Multitool.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class READYORNOT_API AMultitool : public ATool
{
	GENERATED_BODY()

public:
	AMultitool();

	UFUNCTION(BlueprintCallable)
	void ChangeToolkit(EMultitoolFunctions MultitoolFunction, bool bPlayAnimation = false);
	
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_PlayMultitoolAudio();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_ChangeToolkit(EMultitoolFunctions MultitoolFunction, bool bPlayAnimation = false);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StopMultitoolAudio();

	UFUNCTION(BlueprintPure, Category = "Multitool")
	float GetMultitoolOperatingTimeFromAudioLength(UFMODEvent* Event) const;
	
	UFUNCTION(BlueprintPure, Category = "Multitool")
	float GetMultitoolOperatingTimeFromToolkit(EMultitoolFunctions MultitoolFunction) const;
	
	UFUNCTION(BlueprintPure, Category = "Multitool")
	float GetMultitoolOperatingTimeFromActiveToolkit() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Multitool")
	TMap<EMultitoolFunctions, UReadyOrNotWeaponAnimData*> MultitoolAnimData;

	// Standing arrest interaction
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multitool")
    UInteractionsData* PvPFreeInteraction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Multitool")
    EMultitoolFunctions CurrentToolKit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multitool")
	uint8 bAudioBasedProgress : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Multitool|Audio")
    UFMODEvent* FMODLockpickingSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Multitool|Audio")
	UFMODEvent* FMODKnifeSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Multitool|Audio")
	UFMODEvent* FMODWirecutterSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Multitool|Audio")
    UFMODEvent* FMODFreeingSound;

	UPROPERTY(BlueprintReadOnly, Category = "Multitool|Data")
    APlayerCharacter* PendingFreeCharacter;
	
	virtual bool PlayDraw(bool bDrawFirst) override;

	virtual void OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence = true) override;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions = {}) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Client_StopToolAnimation_Implementation() override;

	virtual void Client_FinishedToolUse_Implementation(AActor* Target, class APlayerCharacter* pc) override;

	virtual void Server_StartUsingTool_Implementation(AActor* Target) override;
	virtual void Server_StopUsingTool_Implementation(AActor* Target) override;

	virtual void Server_ToolComplete_Implementation() override;
	
	virtual void Client_PlayMultitoolAudio_Implementation();
	virtual void Client_ChangeToolkit_Implementation(EMultitoolFunctions MultitoolFunction, bool bPlayAnimation = false);
	virtual void Client_StopMultitoolAudio_Implementation();
};
