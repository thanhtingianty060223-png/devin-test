// Copyright Void Interactive, 2022

#pragma once

#include "Components/PostProcessComponent.h"
#include "Data/PostProcessEffectData.h"
#include "Data/LevelData.h"
#include "PlayerPostProcessing.generated.h"

DECLARE_STATS_GROUP(TEXT("Player Post Processing"), STATGROUP_PlayerPP, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UPlayerPostProcessing : public UPostProcessComponent
{
	GENERATED_BODY()

public:
	UPlayerPostProcessing();

	UFUNCTION(BlueprintCallable)
	void PlayPostProcessEffect_Name(FName EffectName, AActor* DamageCauser = nullptr);
	
	UFUNCTION(BlueprintCallable)
	void StopPostProcessEffect_Name(FName EffectName);
	
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void Serialize(FArchive& Ar) override;
	
	void SetupPostProcessEffect(FPostProcessEffect& InPostProcessEffect);

	FPostProcessEffect* FindPostProcessEffect(FName EffectName);

	UFUNCTION(BlueprintCallable, Category = "PP Effects")
	void FadeToGrey();
	
	UFUNCTION()
	void OnItemEquipped(ABaseItem* Item);
	
	UFUNCTION()
	void OnItemHolstered(ABaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	bool FulfillsAllRequirements(const TArray<TSubclassOf<UPostProcessRequirement>>& InRequirementClasses, AActor* InDamageCauser = nullptr, bool bForceFulfillment = false);

	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void StartPostProcessEffect(FPostProcessEffect& InPostProcessEffect, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void StopPostProcessEffect(FPostProcessEffect& InPostProcessEffect);

	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void ProcessPostProcessEffect(FPostProcessEffect& InPostProcessEffect, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void StartPostProcessEffect_Specific(FPostProcessEffectPlayer& InPostProcessSetting, AActor* DamageCauser);
	
	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void StartPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData, AActor* DamageCauser);
	
	UFUNCTION(BlueprintCallable, Category = "Player Post Processing")
	void StopPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData);
	
	void StartPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData, TArray<UPostProcessEffectData*>& OutDebugEffects, TArray<UMaterialInstanceDynamic*>& OutDebugMIDs, AActor* DamageCauser);
	
	UFUNCTION(BlueprintPure, Category = "Player Post Processing")
	bool IsPostProcessEffectPlaying(UPostProcessEffectData* InPostProcessEffectData) const;

	#if !UE_BUILD_SHIPPING
	bool ProcessDebugEffect(UPostProcessEffectData* InPostProcessEffectData, UMaterialInstanceDynamic* InMID, float DeltaTime);
	void ProcessDebugEffects(TArray<UPostProcessEffectData*>& InPostProcessEffects, TArray<UMaterialInstanceDynamic*>& MIDs, float DeltaTime);
	#endif

	UPROPERTY(BlueprintReadOnly, Category = "Player Post Processing | Debug")
	TArray<UPostProcessEffectData*> DebugPPEffects;
	
	UPROPERTY(BlueprintReadOnly, Category = "Player Post Processing | Debug")
	TArray<UMaterialInstanceDynamic*> DebugPPEffects_MIDs;
	
	TArray<FPostProcessEffectPlayer*> FadeOutEffects;

	float WhiteFlashAmount = 0.0f;

	float RedFlashAmount = 0.0f;
	
	UFUNCTION(BlueprintCallable)
	void ResetInjuryRadialBlur(float DeltaTime);

	float HeadshotRadialBlurAmount = 0.0f;
	
	float InjuryRadialBlurAmount = 0.0f;

	float InjurySharpnessAmount = 0.0f;

	float InjuryGreyScaleAmount = 0.0f;
	
	float HitAmount = 0.0f;
	float HitAmountDrain = 0.25f;
	
	float InjuryVignetteEdge = 0.0f;

	UFUNCTION(BlueprintCallable)
	int32 InitializePostProcessFloatParam(const FPostProcessEffect& InPostProcessEffect, const FName& InParameterName, float Value, int32 Instance = 0, int32 CurveKey = 0);
	
	UFUNCTION(BlueprintCallable)
	int32 GetPostProcessFromFloatParam(const FPostProcessEffect& InPostProcessEffect, const FName& InParameterName, int32 Instance = 0);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings")
	TArray<FPostProcessEffect> PostProcessEffects;
	
	void StartGreyscaleEffects();
	void StopGreyscaleEffects();
	
	void StartNVGEffects();
	void StopNVGEffects();
	
	void ProcessNVG(float DeltaTime);
	
	void StartGasMaskEffects();
	void StopGasMaskEffects();

	void ProcessGasMask(float DeltaTime);
	
	float HitTopIntensity = 0.0f;
	float HitBottomIntensity = 0.0f;
	float HitRightIntensity = 0.0f;
	float HitLeftIntensity = 0.0f;
	
	void ProcessBloodEffect(float DeltaTime);

	void StartBloodEffect(AActor* DamageCauser, float DirectionForward, float DirectionRight);
	void StopBloodEffect();

public:
	UFUNCTION(BlueprintCallable, Category = "Bleeding Effect")
	void StartBleedingEffect();

	UFUNCTION(BlueprintCallable, Category = "Bleeding Effect")
	void StopBleedingEffect();

	UFUNCTION(BlueprintCallable, Category = "Healing Effect")
	void StartHealingEffect();

	UFUNCTION(BlueprintCallable, Category = "Healing Effect")
	void StopHealingEffect();
	
protected:
	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StartDeathEffect(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StopDeathEffect();

	void StartInjuryTunnelVisionEffect(AActor* DamageCauser);
	void StartInjuryHitEffect(AActor* DamageCauser);
	void StartInjuryUnsharpEffect(AActor* DamageCauser);
	void StartInjuryGreyscaleEffect(AActor* DamageCauser);
	void StartInjuryRadialBlurEffect(AActor* DamageCauser);
	void StartInjuryRedFlashEffect(AActor* DamageCauser);
	void StartInjuryWhiteFlashEffect(AActor* DamageCauser);
	
	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StartInjuryEffects(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
    void StopInjuryEffects();

	void ProcessInjuryEffects(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StartGasEffect(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StopGasEffect();

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StartTaserEffect(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
	void StopTaserEffect();

public:
	UFUNCTION(BlueprintCallable, Category = "Heartbeat Effect")
	void StartHeartbeatEffect();

	UFUNCTION(BlueprintCallable, Category = "Heartbeat Effect")
	void StopHeartbeatEffect();
protected:

	UFUNCTION(BlueprintCallable, Category = "Suppression Effects Settings")
	void StartSuppressionEffects(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Suppression Effects Settings")
    void StopSuppressionEffects();

	float SupressionIntensity = 0.0f;

	void ProcessSuppressionEffect(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
    void StartPeppersprayEffect(AActor* DamageCauser);
	
	UFUNCTION(BlueprintCallable, Category = "Pepperspray Effects Settings")
    void StopPeppersprayEffect();

	UFUNCTION(BlueprintCallable, Category = "Flashbang Effects Settings")
	void StartFlashbangEffect(AActor* DamageCauser);
		
	UFUNCTION(BlueprintCallable, Category = "Flashbang Effects Settings")
    void StopFlashbangEffect();

	UFUNCTION(BlueprintCallable, Category = "Stinger Effects Settings")
	void StartStingerEffect(AActor* DamageCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Stinger Effects Settings")
	void StopStingerEffect();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP Settings (Dev)")
	TArray<UMaterialInterface*> DevPostProcessMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NVG")
	FNVGPostProcessSettings NVG_Settings;
	
	FPostProcessSettings PlayersDefaultPostProcessSettings;

	UPROPERTY(BlueprintReadOnly)
	class APlayerCharacter* OwningCharacter = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "PP Settings")
	AActor* RecentDamageCauser = nullptr;
	
	void ProcessDepthOfField(float DeltaTime);

	void ProcessMotionBlur();
	
	UFUNCTION()
	void OnPlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);
	
	UFUNCTION()
	void OnPlayerStunned(AReadyOrNotCharacter* Character, float Duration, EStunType StunType, AActor* DamageCauser);

	UFUNCTION()
	void OnPlayerPepperSprayed(AActor* DamageCauser);
	
	UFUNCTION()
	void OnBulletImpact(float DirectionForward, float DirectionRight);

	UFUNCTION()
	void OnSupression(float Strength);

	UFUNCTION()
	void OnDamageTaken(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining);
	
	UFUNCTION()
	void OnDamageTakenDetails(bool bWasHeadshot, float DamageTaken, float HealthRemaining, bool bBlockedByArmour, bool bBlockedByHelmet);

	int32 LastOutlineBlurDistanceSamples = -1;
	int32 LastOutlineBlurRadialSamples = -1;
	void ProcessOutlines();
	
	UFUNCTION()
	void UpdateWeaponHighlightVisibility();

public:
	UFUNCTION()
	void OnFire();

	FPostProcessSettings GetDefaultPlayerPostProcessSettings() const { return PlayersDefaultPostProcessSettings; }

	float OriginalMinBrightness = -10.0f;
	float OriginalMaxBrightness = 20.0f;
};

