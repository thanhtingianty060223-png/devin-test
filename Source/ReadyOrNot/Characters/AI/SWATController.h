// Copyright Void Interactive, 2022

#pragma once

#include "Characters/CyberneticController.h"
#include "SWATController.generated.h"

UCLASS()
class READYORNOT_API ASWATController : public ACyberneticController
{
	GENERATED_BODY()

public:
	virtual float GetReactionTime(const EActorSenseType& SenseType) const override;

	UFUNCTION(BlueprintPure)
	FORCEINLINE UTeamBreachAndClearActivity* GetBreachAndClearActivity() const { return BreachAndClearActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UDeployChemlightActivity* GetDeployChemlightActivity() const { return DeployChemlightActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UHoldActivity* GetHoldActivity() const { return HoldActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class USearchAndSecureActivity* GetSearchAndSecureActivity() const { return SearchAndSecureActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UReportTargetActivity* GetReportTargetActivity() const { return ReportTargetActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UCollectEvidenceActivity* GetCollectEvidenceActivity() const { return CollectEvidenceActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UDisarmStandaloneTrapActivity* GetDisarmStandaloneTrapActivity() const { return DisarmStandaloneTrapActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UDeployGrenadeAtLocationActivity* GetDeployGrenadeAtLocationActivity() const { return DeployGrenadeAtLocationActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UTeamFallinActivity* GetFallinActivity() const { return FallinActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UArrestTargetActivity* GetArrestTargetActivity() const { return ArrestTargetActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UPickUpCharacterActivity* GetPickUpCharacterActivity() const { return PickUpCharacterActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UTeamStackUpActivity* GetStackUpActivity() const { return StackUpActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UKickDoorActivity* GetKickDoorActivity() const { return KickDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UC2DoorActivity* GetC2DoorActivity() const { return C2DoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UShotgunDoorActivity* GetShotgunDoorActivity() const { return ShotgunDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class URamDoorActivity* GetRamDoorActivity() const { return RamDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UThrowGrenadeThroughDoorActivity* GetThrowGrenadeThroughDoorActivity() const { return ThrowGrenadeThroughDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class ULaunchGrenadeThroughDoorActivity* GetLaunchGrenadeThroughDoorActivity() const { return LaunchGrenadeThroughDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class ULockPickDoorActivity* GetLockPickDoorDoorActivity() const { return LockPickDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UDeployWedgeActivity* GetDeployWedgeActivity() const { return DeployWedgeActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UDisarmDoorTrapActivity* GetDisarmDoorTrapActivity() const { return DisarmDoorTrapActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UMirrorUnderDoorActivity* GetMirrorUnderDoorActivity() const { return MirrorUnderDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UToggleDoorActivity* GetToggleDoorActivity() const { return ToggleDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UScanDoorActivity* GetScanDoorActivity() const { return ScanDoorActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UTeamCoverAreaActivity* GetCoverAreaActivity() const { return CoverAreaActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class USearchLandmarkActivity* GetSearchLandmarkActivity() const { return SearchLandmarkActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UTrailerSearchAndSecureActivity* GetTrailerSearchAndSecureActivity() const { return TrailerSearchAndSecureActivity; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE class UEngageTargetLessLethalActivity* GetEngageLessLethalActivity() const { return EngageLessLethalActivity; }
	
	#if !UE_BUILD_SHIPPING
	virtual void DisplayAIDebugInfo(float DeltaTime) override;
	#endif
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void ProcessStimuli(FAIStimulus Stimulus, AActor* SensedActor, FActorPerceptionBlueprintInfo PerceptionOfActor) override;

	virtual bool CanSpotCharacter(AReadyOrNotCharacter* SensedCharacter) const override;
	
	virtual void OnSeenCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;
	virtual void OnHeardCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;
	virtual void OnDamagedByCharacter(AReadyOrNotCharacter* SensedCharacter, const FAIStimulus& Stimulus) override;

	virtual void OnSeenIncapHuman(AIncapacitatedHuman* IncapHuman) override;

	virtual void OnTrackedTargetKilled(AReadyOrNotCharacter* Insitgator, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void OnTrackedTargetIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter) override;
	virtual void OnTrackedTargetExitedSurrender(ACyberneticCharacter* InCharacter, ESurrenderExitType ExitType) override;

	void RespondToAIKilled(AReadyOrNotCharacter* AI);
	void RespondToAIIncapacitated(AReadyOrNotCharacter* AI);
	void RespondToAISurrenderExit(ACyberneticCharacter* AI, ESurrenderExitType ExitType);
	
	virtual bool IsCharacterNeutral_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterEnemy_Implementation(AReadyOrNotCharacter* InCharacter) const override;
	virtual bool IsCharacterFriendly_Implementation(AReadyOrNotCharacter* InCharacter) const override;

	UPROPERTY()
	UTeamFallinActivity* FallinActivity = nullptr;

	UPROPERTY()
	UArrestTargetActivity* ArrestTargetActivity = nullptr;
	
	UPROPERTY()
	UTeamStackUpActivity* StackUpActivity = nullptr;
	
	UPROPERTY()
	UTeamBreachAndClearActivity* BreachAndClearActivity = nullptr;

	UPROPERTY()
	UDeployChemlightActivity* DeployChemlightActivity = nullptr;

	UPROPERTY()
	UHoldActivity* HoldActivity = nullptr;

	UPROPERTY()
	UReportTargetActivity* ReportTargetActivity = nullptr;

	UPROPERTY()
	UCollectEvidenceActivity* CollectEvidenceActivity = nullptr;

	UPROPERTY()
	UDisarmStandaloneTrapActivity* DisarmStandaloneTrapActivity = nullptr;

	UPROPERTY()
	UDeployGrenadeAtLocationActivity* DeployGrenadeAtLocationActivity = nullptr;

	UPROPERTY()
	UKickDoorActivity* KickDoorActivity = nullptr;
	
	UPROPERTY()
	UC2DoorActivity* C2DoorActivity = nullptr;
	
	UPROPERTY()
	URamDoorActivity* RamDoorActivity = nullptr;
	
	UPROPERTY()
	UShotgunDoorActivity* ShotgunDoorActivity = nullptr;
	
	UPROPERTY()
	UThrowGrenadeThroughDoorActivity* ThrowGrenadeThroughDoorActivity = nullptr;
	
	UPROPERTY()
	ULaunchGrenadeThroughDoorActivity* LaunchGrenadeThroughDoorActivity = nullptr;
	
	UPROPERTY()
	USearchAndSecureActivity* SearchAndSecureActivity = nullptr;
	
	UPROPERTY()
	UMirrorUnderDoorActivity* MirrorUnderDoorActivity = nullptr;
	
	UPROPERTY()
	UDisarmDoorTrapActivity* DisarmDoorTrapActivity = nullptr;
	
	UPROPERTY()
	class UDeployWedgeActivity* DeployWedgeActivity = nullptr;
	
	UPROPERTY()
	class ULockPickDoorActivity* LockPickDoorActivity = nullptr;
	
	UPROPERTY()
	class UToggleDoorActivity* ToggleDoorActivity = nullptr;
	
	UPROPERTY()
	class UScanDoorActivity* ScanDoorActivity = nullptr;
	
	UPROPERTY()
	class UTeamCoverAreaActivity* CoverAreaActivity = nullptr;
	
	UPROPERTY()
	class USearchLandmarkActivity* SearchLandmarkActivity = nullptr;
	
	UPROPERTY()
	class UPickUpCharacterActivity* PickUpCharacterActivity = nullptr;
	
	UPROPERTY()
	class UTrailerSearchAndSecureActivity* TrailerSearchAndSecureActivity = nullptr;
	
	UPROPERTY()
	class UEngageTargetLessLethalActivity* EngageLessLethalActivity = nullptr;
};
