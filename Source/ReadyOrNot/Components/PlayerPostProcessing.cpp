// Copyright Void Interactive, 2022

#include "PlayerPostProcessing.h"

#include "Characters/PlayerCharacter.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick"), STAT_PlayerPPTick, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Post Process Effects"), STAT_PlayerPP_TickPostProcessEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Debug Effects"), STAT_PlayerPP_TickDebugEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ NVG Effects"), STAT_PlayerPP_TickNVGEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Gas Mask Effects"), STAT_PlayerPP_TickGasMaskEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Blood Effects"), STAT_PlayerPP_TickBloodEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Injury Effects"), STAT_PlayerPP_TickInjuryEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ Suppression Effects"), STAT_PlayerPP_TickSuppressionEffects, STATGROUP_PlayerPP);
DECLARE_CYCLE_STAT(TEXT("PlayerPostProcessing ~ Tick ~ DOF Effects"), STAT_PlayerPP_TickDOFEffects, STATGROUP_PlayerPP);

TAutoConsoleVariable<int32> CVarRonOutlineBlurDistanceSamples(TEXT("r.ron.OutlineBlurDistanceSamples"), 3, TEXT("Number of distance samples in outline blur"), ECVF_Scalability);
TAutoConsoleVariable<int32> CVarRonOutlineBlurRadialSamples(TEXT("r.ron.OutlineBlurRadialSamples"), 6, TEXT("Number of radial samples in outline blur"), ECVF_Scalability);

UPlayerPostProcessing::UPlayerPostProcessing()
{
	PrimaryComponentTick.bCanEverTick = true;
	bUnbound = false;
	bEnabled = false;
}

void UPlayerPostProcessing::BeginPlay()
{
	Super::BeginPlay();

	#if !UE_BUILD_SHIPPING
	for (UMaterialInterface* DevMaterial : DevPostProcessMaterials)
		Settings.AddBlendable(DevMaterial, 0.0f);
	#else
	DevPostProcessMaterials.Empty();
	#endif

	NVG_Settings = UBpGameplayHelperLib::GetLevelData(GetWorld()).NVG_PostProcessOverride;
	
	OwningCharacter = Cast<APlayerCharacter>(GetOwner());

	if (OwningCharacter)
	{
		// Remove dynamic first to fix invocation error in editor
		OwningCharacter->OnStunnedEvent.RemoveDynamic(this, &UPlayerPostProcessing::OnPlayerStunned);
		OwningCharacter->OnStunnedEvent.AddDynamic(this, &UPlayerPostProcessing::OnPlayerStunned);
		OwningCharacter->OnBulletImpacted.RemoveDynamic(this, &UPlayerPostProcessing::OnBulletImpact);
		OwningCharacter->OnBulletImpacted.AddDynamic(this, &UPlayerPostProcessing::OnBulletImpact);
		OwningCharacter->OnPlayerSupressed.RemoveDynamic(this, &UPlayerPostProcessing::OnSupression);
		OwningCharacter->OnPlayerSupressed.AddDynamic(this, &UPlayerPostProcessing::OnSupression);
		OwningCharacter->OnCharacterTakeDamage.RemoveDynamic(this, &UPlayerPostProcessing::OnDamageTaken);
		OwningCharacter->OnCharacterTakeDamage.AddDynamic(this, &UPlayerPostProcessing::OnDamageTaken);
		OwningCharacter->OnPlayerTakenDamageDetails.RemoveDynamic(this, &UPlayerPostProcessing::OnDamageTakenDetails);
		OwningCharacter->OnPlayerTakenDamageDetails.AddDynamic(this, &UPlayerPostProcessing::OnDamageTakenDetails);
		
		if (OwningCharacter->GetInventoryComponent())
		{
			OwningCharacter->GetInventoryComponent()->OnItemAddedToInventory.RemoveDynamic(this, &UPlayerPostProcessing::OnItemEquipped);
			OwningCharacter->GetInventoryComponent()->OnItemAddedToInventory.AddDynamic(this, &UPlayerPostProcessing::OnItemEquipped);
			OwningCharacter->GetInventoryComponent()->OnItemHolstered.RemoveDynamic(this, &UPlayerPostProcessing::OnItemHolstered);
			OwningCharacter->GetInventoryComponent()->OnItemHolstered.AddDynamic(this, &UPlayerPostProcessing::OnItemHolstered);
		}
		
		OwningCharacter->OnCharacterKilled.RemoveAll(this);
		OwningCharacter->OnCharacterKilled.AddDynamic(this, &UPlayerPostProcessing::OnPlayerKilled);

		PlayersDefaultPostProcessSettings = Settings;
	}

	for (FPostProcessEffect& PPEffect : PostProcessEffects)
	{
		SetupPostProcessEffect(PPEffect);
	}
		
	OriginalMinBrightness = Settings.AutoExposureMinBrightness;
	OriginalMaxBrightness = Settings.AutoExposureMaxBrightness;

	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us)
	{
		us->OnSettingsSaved.AddUFunction(this, FName("UpdateWeaponHighlightVisibility"));
		if (us->bHighlightWeapons && OwningCharacter)
		{
			PlayPostProcessEffect_Name("WeaponHighlight", OwningCharacter);
		}
	}
}

void UPlayerPostProcessing::TickComponent(const float DeltaTime, const enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_PlayerPPTick);
	
	if (OwningCharacter)
	{
		if (!OwningCharacter->IsLocalPlayer() && !OwningCharacter->IsDeadOrUnconscious())
		{
			PrimaryComponentTick.bCanEverTick = false;
			return;
		}
		
		// For some reason, in a networked environment (client-side only), the OnPlayerKilled delegate event is not making the death fade show on screen
		// even though it has actually passed the requirements, added to the blendables array and the play head is progressing with the correct material param being modified every frame.
		// So bizarre. I hate doing it this way. - Ali
		if (FPostProcessEffect* DeathEffect = FindPostProcessEffect("Death"))
		{
			if (OwningCharacter->GetNetMode() == NM_Client)
			{
				if (OwningCharacter->IsDeadNotUnconscious() && !OwningCharacter->IsDowned())
				{
					if (DeathEffect->HasEffectFinished())
						StartDeathEffect(nullptr);
				}
			}
		}
		
		// Do all the processing of effects
		{
			SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickPostProcessEffects);

			for (FPostProcessEffect& PPEffect : PostProcessEffects)
			{
				if (!PPEffect.bCustomProcess)
					ProcessPostProcessEffect(PPEffect, DeltaTime);
			}
			
			ProcessNVG(DeltaTime);
			ProcessGasMask(DeltaTime);
			ProcessBloodEffect(DeltaTime);
			ProcessInjuryEffects(DeltaTime);
			ProcessSuppressionEffect(DeltaTime);
			ProcessOutlines();

			// The blendables that want a fade out instead of a hard stop
			{
				TArray<FPostProcessEffectPlayer*> PendingFadeOutEffectsToRemove;
				PendingFadeOutEffectsToRemove.Reserve(FadeOutEffects.Num());
				
				for (FPostProcessEffectPlayer* PPEffect : FadeOutEffects)
				{
					for (FWeightedBlendable& WeightedBlendable : Settings.WeightedBlendables.Array)
					{
						if (PPEffect->PostProcess_MID == WeightedBlendable.Object)
						{
							if (WeightedBlendable.Weight <= 0.0f)
							{
								PendingFadeOutEffectsToRemove.AddUnique(PPEffect);
								
								PPEffect->Reset();
								
								#if WITH_EDITOR
								if (PPEffect->bDebug)
									ULog::Info(CUR_FUNC_2 + PPEffect->PostProcess_Data->GetName() + ": removed");
								#endif
								
								continue;
							}
							
							AddOrUpdateBlendable(WeightedBlendable.Object.Get(), FMath::Clamp(WeightedBlendable.Weight - (PPEffect->FadeOutSpeed * DeltaTime), 0.0f, 1.0f));
						}
					}
				}

				for (FPostProcessEffectPlayer* FadeOutEffect : PendingFadeOutEffectsToRemove)
				{
					FadeOutEffects.Remove(FadeOutEffect);

					Settings.RemoveBlendable(FadeOutEffect->PostProcess_MID);
				}
			}
		}

		#if !UE_BUILD_SHIPPING
		ProcessDebugEffects(DebugPPEffects, DebugPPEffects_MIDs, DeltaTime);
		#endif

		ProcessDepthOfField(DeltaTime);

		ProcessMotionBlur();
		
		// finally apply them to the camera
		OwningCharacter->GetFirstPersonCameraComponent()->PostProcessSettings = Settings;
	}
}

#if !UE_BUILD_SHIPPING
bool UPlayerPostProcessing::ProcessDebugEffect(UPostProcessEffectData* InPostProcessEffectData, UMaterialInstanceDynamic* InMID, const float DeltaTime)
{
	if (InPostProcessEffectData && InMID &&
	   (InPostProcessEffectData->ScalarParameters.Num() > 0 || InPostProcessEffectData->VectorParameters.Num() > 0))
	{
		if (InPostProcessEffectData->HasEffectFinished())
		{
			Settings.RemoveBlendable(InMID);
			InPostProcessEffectData->ResetData();
			
			#if WITH_EDITOR
			if (InPostProcessEffectData->bDebug)
				ULog::Info(CUR_FUNC_2 + InPostProcessEffectData->GetName() + ": stopped");
			#endif
			
			return true;
		}
		
		InPostProcessEffectData->Update(DeltaTime);
		
		// Process scalar params
		{
			TArray<FPostProcessSetting_FloatParam>& CurrentScalarParams = InPostProcessEffectData->ScalarParameters;
			
			for (int32 j = 0; j < CurrentScalarParams.Num(); j++)
			{
				FPostProcessSetting_FloatParam& CurrentScalarParam = CurrentScalarParams[j];
				
				if (CurrentScalarParam.bEnabled && CurrentScalarParam.PlayState != EPostProcessState::Ended)
				{
					if (CurrentScalarParam.bUseCurve)
					{
						if (CurrentScalarParam.Curve.ExternalCurve)
						{
							const float CurveValue = CurrentScalarParam.Curve.ExternalCurve->GetFloatValue(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
							InMID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);
						}
						else if (CurrentScalarParam.Curve.EditorCurveData.GetNumKeys() > 1)
						{
							const float CurveValue = CurrentScalarParam.Curve.EditorCurveData.Eval(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
							InMID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);
						}

						if (CurrentScalarParam.EffectEndOption == EPostProcessEndOptions::Reverse)
						{
							if (CurrentScalarParam.PlayState == EPostProcessState::WaitingForReverse)
							{
								if (CurrentScalarParam.ReverseRequirements.Num() > 0)
								{
									if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
									{
										CurrentScalarParam.Reverse();
									}
								}
								else
								{
									CurrentScalarParam.Reverse();
								}
							}
						}
					}
					else
					{
						const float Alpha = CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime;
						const float EasingValue = UKismetMathLibrary::Ease(CurrentScalarParam.Start, CurrentScalarParam.End, Alpha, CurrentScalarParam.EasingMethod);

						InMID->SetScalarParameterValue(CurrentScalarParam.ParameterName, EasingValue);

						if (CurrentScalarParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentScalarParam.bReverseAtAnyTime)
						{
							if (CurrentScalarParam.PlayState == EPostProcessState::WaitingForReverse)
							{
								if (CurrentScalarParam.ReverseRequirements.Num() > 0)
								{
									if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
									{
										CurrentScalarParam.Reverse();
									}
								}
								else
								{
									CurrentScalarParam.Reverse();
								}
							}
						}
					}
				}
			}
		}

		// Process vector params
		{
			TArray<FPostProcessSetting_VectorParam>& CurrentVectorParams = InPostProcessEffectData->VectorParameters;

			for (int32 j = 0; j < CurrentVectorParams.Num(); j++)
			{
				FPostProcessSetting_VectorParam& CurrentVectorParam = CurrentVectorParams[j];

				if (CurrentVectorParam.bEnabled && CurrentVectorParam.PlayState != EPostProcessState::Ended)
				{
					if (CurrentVectorParam.bUseCurve)
					{
						const FLinearColor CurveValue = CurrentVectorParam.Curve.GetLinearColorValue(CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime);
						InMID->SetVectorParameterValue(CurrentVectorParam.ParameterName, CurveValue);

						if (CurrentVectorParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentVectorParam.bReverseAtAnyTime)
						{
							if (CurrentVectorParam.PlayState == EPostProcessState::WaitingForReverse)
							{
								if (CurrentVectorParam.ReverseRequirements.Num() > 0)
								{
									if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
									{
										CurrentVectorParam.Reverse();
									}
								}
								else
								{
									CurrentVectorParam.Reverse();
								}
							}
						}
					}
					else
					{
						const float Alpha = CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime;
						const FVector EasingValue = UKismetMathLibrary::VEase(CurrentVectorParam.Start, CurrentVectorParam.End, Alpha, CurrentVectorParam.EasingMethod);
						
						InMID->SetVectorParameterValue(CurrentVectorParam.ParameterName, EasingValue);

						if (CurrentVectorParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentVectorParam.bReverseAtAnyTime)
						{
							if (CurrentVectorParam.PlayState == EPostProcessState::WaitingForReverse)
							{
								if (CurrentVectorParam.ReverseRequirements.Num() > 0)
								{
									if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
									{
										CurrentVectorParam.Reverse();
									}
								}
								else
								{
									CurrentVectorParam.Reverse();
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

void UPlayerPostProcessing::ProcessDebugEffects(TArray<UPostProcessEffectData*>& InPostProcessEffects, TArray<UMaterialInstanceDynamic*>& MIDs, const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickDebugEffects);

	TArray<int32> Indices;

	for (int32 i = 0; i < InPostProcessEffects.Num(); i++)
	{
		if (ProcessDebugEffect(InPostProcessEffects[i], MIDs[i], DeltaTime))
			Indices.Add(i);
    }

	for (int32 i = 0; i < Indices.Num(); i++)
	{
		if (Indices.IsValidIndex(i))
		{
			InPostProcessEffects.RemoveAt(Indices[i]);
			MIDs.RemoveAt(Indices[i]);
		}
	}
}
#endif

bool UPlayerPostProcessing::FulfillsAllRequirements(const TArray<TSubclassOf<UPostProcessRequirement>>& InRequirementClasses, AActor* InDamageCauser, const bool bForceFulfillment)
{
	return UReadyOrNotFunctionLibrary::FulfillsAllPostProcessRequirements(this, Cast<APlayerCharacter>(GetOwner()), InDamageCauser == nullptr ? RecentDamageCauser : InDamageCauser, InRequirementClasses, bForceFulfillment);
}

void UPlayerPostProcessing::StartPostProcessEffect(FPostProcessEffect& InPostProcessEffect, AActor* DamageCauser)
{
	if (InPostProcessEffect.bEnabled)
	{
		for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
		{
			StartPostProcessEffect_Specific(InPostProcessEffect.PostProcesses[i], DamageCauser);
		}

		if (InPostProcessEffect.PostProcesses.Num() > 0)
		{
			InPostProcessEffect.bStarted = true;
			
			#if WITH_EDITOR
			if (InPostProcessEffect.bDebug)
				ULog::Info(CUR_FUNC_2 + InPostProcessEffect.EffectName.ToString() + ": started");
			#endif
		}
	}
}

void UPlayerPostProcessing::StartPostProcessEffect_Specific(FPostProcessEffectPlayer& InPostProcessSetting, AActor* DamageCauser)
{
	if (InPostProcessSetting.bEnabled)
	{
		if (InPostProcessSetting.PostProcess_Data)
		{
			if (InPostProcessSetting.PostProcess_MID && FulfillsAllRequirements(InPostProcessSetting.RequirementsClasses, DamageCauser))
			{
				if (InPostProcessSetting.bStarted)
				{
					if (InPostProcessSetting.bRestartIfAlreadyPlaying)
					{
						//Settings.RemoveBlendable(InPostProcessSetting.PostProcess_MID);
						//InPostProcessSetting.Reset();

						//#if WITH_EDITOR
						//if (InPostProcessSetting.bDebug)
						//	ULog::Info(CUR_FUNC_2 + InPostProcessSetting.EffectName.ToString() + " removed");
						//#endif

						//AddOrUpdateBlendable(InPostProcessSetting.PostProcess_MID, 1.0f);

						InPostProcessSetting.Start(Cast<APlayerCharacter>(GetOwner()), DamageCauser);

						#if WITH_EDITOR
						if (InPostProcessSetting.bDebug)
							ULog::Info(CUR_FUNC_2 + InPostProcessSetting.PostProcess_Data->GetName() + ": started");
						#endif
					}
				}
				else
				{
					AddOrUpdateBlendable(InPostProcessSetting.PostProcess_MID, 1.0f);

					InPostProcessSetting.Start(Cast<APlayerCharacter>(GetOwner()), DamageCauser);

					#if WITH_EDITOR
					if (InPostProcessSetting.bDebug)
						ULog::Info(CUR_FUNC_2 + InPostProcessSetting.PostProcess_Data->GetName() + ": started");
					#endif
				}
			}
		}
	}
}

void UPlayerPostProcessing::StartPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData, AActor* DamageCauser)
{
	StartPostProcessEffect_FromDataAsset(InPostProcessEffectData, DebugPPEffects, DebugPPEffects_MIDs, DamageCauser);
}

void UPlayerPostProcessing::StopPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData)
{
	if (!InPostProcessEffectData)
		return;

	if (DebugPPEffects.Contains(InPostProcessEffectData))
	{
		Settings.RemoveBlendable(DebugPPEffects_MIDs[DebugPPEffects.Find(InPostProcessEffectData)]);
		DebugPPEffects_MIDs.RemoveAt(DebugPPEffects.Find(InPostProcessEffectData));
		DebugPPEffects.RemoveSingle(InPostProcessEffectData);

		InPostProcessEffectData->ResetData();
			
		#if WITH_EDITOR
		if (InPostProcessEffectData->bDebug)
			ULog::Info(CUR_FUNC_2 + InPostProcessEffectData->GetName() + ": stopped");
		#endif
	}
}

void UPlayerPostProcessing::StartPostProcessEffect_FromDataAsset(UPostProcessEffectData* InPostProcessEffectData, TArray<UPostProcessEffectData*>& OutDebugEffects, TArray<UMaterialInstanceDynamic*>& OutDebugMIDs, AActor* DamageCauser)
{
	if (!InPostProcessEffectData)
		return;

	StopPostProcessEffect_FromDataAsset(InPostProcessEffectData);

	InPostProcessEffectData->InitializeData(Cast<APlayerCharacter>(GetOwner()), DamageCauser, true);

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(InPostProcessEffectData->PostProcess_Material, this); 
	AddOrUpdateBlendable(MID, 1.0f);

	OutDebugMIDs.Add(MID);
	OutDebugEffects.Add(InPostProcessEffectData);

	#if WITH_EDITOR
	if (InPostProcessEffectData->bDebug)
		ULog::Info(CUR_FUNC_2 + InPostProcessEffectData->GetName() + ": started");
	#endif
}

bool UPlayerPostProcessing::IsPostProcessEffectPlaying(UPostProcessEffectData* InPostProcessEffectData) const
{
	if (!InPostProcessEffectData)
		return false;

	return !InPostProcessEffectData->HasEffectFinished();
}

void UPlayerPostProcessing::StopPostProcessEffect(FPostProcessEffect& InPostProcessEffect)
{
	if (!InPostProcessEffect.bStarted)
		return;
	
	if (InPostProcessEffect.bEnabled)
	{
		for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
		{
			FPostProcessEffectPlayer& PostProcessSetting = InPostProcessEffect.PostProcesses[i];
			
			if (PostProcessSetting.bEnabled)
			{
				if (PostProcessSetting.PostProcess_MID)
				{
					if (PostProcessSetting.bWantsFadeOut)
					{
						FadeOutEffects.AddUnique(&PostProcessSetting);
					}
					else
					{
						Settings.RemoveBlendable(PostProcessSetting.PostProcess_MID);
						
						PostProcessSetting.Reset();

						#if WITH_EDITOR
						if (PostProcessSetting.bDebug)
							ULog::Info(CUR_FUNC_2 + PostProcessSetting.PostProcess_Data->GetName() + ": removed");
						#endif
					}
				}
			}
		}

		InPostProcessEffect.bStarted = false;
	}
}

void UPlayerPostProcessing::ProcessPostProcessEffect(FPostProcessEffect& InPostProcessEffect, const float DeltaTime)
{
	if (!InPostProcessEffect.bStarted || !InPostProcessEffect.bEnabled)
		return;
	
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		FPostProcessEffectPlayer& CurrentPPEffectSetting = InPostProcessEffect.PostProcesses[i];
		
		if (CurrentPPEffectSetting.bEnabled && CurrentPPEffectSetting.bStarted)
		{
			if (CurrentPPEffectSetting.PostProcess_MID && (CurrentPPEffectSetting.PostProcess_Data->ScalarParameters.Num() > 0 || CurrentPPEffectSetting.PostProcess_Data->VectorParameters.Num() > 0))
			{
				if (CurrentPPEffectSetting.CanStopPostProcessEffect())
				{
					Settings.RemoveBlendable(CurrentPPEffectSetting.PostProcess_MID);
					CurrentPPEffectSetting.Reset();

					#if WITH_EDITOR
					if (CurrentPPEffectSetting.bDebug)
						ULog::Info(CUR_FUNC_2 + CurrentPPEffectSetting.EffectName.ToString() + " removed");
					#endif
				}
				else
				{
					CurrentPPEffectSetting.Update(DeltaTime);
					
					if (CurrentPPEffectSetting.PostProcess_Data)
					{
						// Process scalar params
						{
							TArray<FPostProcessSetting_FloatParam>& CurrentScalarParams = CurrentPPEffectSetting.PostProcess_Data->ScalarParameters;
							
							for (int32 j = 0; j < CurrentScalarParams.Num(); j++)
							{
								FPostProcessSetting_FloatParam& CurrentScalarParam = CurrentScalarParams[j];
								
								if (CurrentScalarParam.bEnabled && CurrentScalarParam.PlayState != EPostProcessState::Ended)
								{
									// Curve mode
									if (CurrentScalarParam.bUseCurve)
									{
										if (CurrentScalarParam.Curve.ExternalCurve)
										{
											const float CurveValue = CurrentScalarParam.Curve.ExternalCurve->GetFloatValue(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
											CurrentPPEffectSetting.PostProcess_MID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);
										}
										else if (CurrentScalarParam.Curve.EditorCurveData.GetNumKeys() > 1)
										{
											const float CurveValue = CurrentScalarParam.Curve.EditorCurveData.Eval(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
											CurrentPPEffectSetting.PostProcess_MID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);
										}

										if (CurrentScalarParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentScalarParam.bReverseAtAnyTime)
										{
											if (CurrentScalarParam.PlayState == EPostProcessState::WaitingForReverse)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Reverse();
													}
												}
												else
												{
													CurrentScalarParam.Reverse();
												}
											}
											else if (CurrentScalarParam.PlayState == EPostProcessState::Forward)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Reverse();
													}
												}
												else
												{
													CurrentScalarParam.Reverse();
												}
											}
											else if (CurrentScalarParam.PlayState == EPostProcessState::Reverse)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (!FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Forward();
													}
												}
											}
										}
									}
									// Manual easing mode
									else
									{
										const float Alpha = CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime;
										const float EasingValue = UKismetMathLibrary::Ease(CurrentScalarParam.Start, CurrentScalarParam.End, Alpha, CurrentScalarParam.EasingMethod);

										CurrentPPEffectSetting.PostProcess_MID->SetScalarParameterValue(CurrentScalarParam.ParameterName, EasingValue);

										if (CurrentScalarParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentScalarParam.bReverseAtAnyTime)
										{
											if (CurrentScalarParam.PlayState == EPostProcessState::WaitingForReverse)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Reverse();
													}
												}
												else
												{
													CurrentScalarParam.Reverse();
												}
											}
											else if (CurrentScalarParam.PlayState == EPostProcessState::Forward)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Reverse();
													}
												}
												else
												{
													CurrentScalarParam.Reverse();
												}
											}
											else if (CurrentScalarParam.PlayState == EPostProcessState::Reverse)
											{
												if (CurrentScalarParam.ReverseRequirements.Num() > 0)
												{
													if (!FulfillsAllRequirements(CurrentScalarParam.ReverseRequirements))
													{
														CurrentScalarParam.Forward();
													}
												}
											}
										}
									}
								}
							}
						}

						// Process vector params
						{
							TArray<FPostProcessSetting_VectorParam>& CurrentVectorParams = CurrentPPEffectSetting.PostProcess_Data->VectorParameters;

							for (int32 j = 0; j < CurrentVectorParams.Num(); j++)
							{
								FPostProcessSetting_VectorParam& CurrentVectorParam = CurrentVectorParams[j];

								if (CurrentVectorParam.bEnabled && CurrentVectorParam.PlayState != EPostProcessState::Ended)
								{
									// Curve mode
									if (CurrentVectorParam.bUseCurve)
									{
										const FLinearColor CurveValue = CurrentVectorParam.Curve.GetLinearColorValue(CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime);
										CurrentPPEffectSetting.PostProcess_MID->SetVectorParameterValue(CurrentVectorParam.ParameterName, CurveValue);

										if (CurrentVectorParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentVectorParam.bReverseAtAnyTime)
										{
											if (CurrentVectorParam.PlayState == EPostProcessState::WaitingForReverse)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Reverse();
													}
												}
												else
												{
													CurrentVectorParam.Reverse();
												}
											}
											else if (CurrentVectorParam.PlayState == EPostProcessState::Forward)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Reverse();
													}
												}
												else
												{
													CurrentVectorParam.Reverse();
												}
											}
											else if (CurrentVectorParam.PlayState == EPostProcessState::Reverse)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (!FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Forward();
													}
												}
											}
										}
									}
									// Manual easing mode
									else
									{
										const float Alpha = CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime;
										const FVector EasingValue = UKismetMathLibrary::VEase(CurrentVectorParam.Start, CurrentVectorParam.End, Alpha, CurrentVectorParam.EasingMethod);
										
										CurrentPPEffectSetting.PostProcess_MID->SetVectorParameterValue(CurrentVectorParam.ParameterName, EasingValue);

										if (CurrentVectorParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentVectorParam.bReverseAtAnyTime)
										{
											if (CurrentVectorParam.PlayState == EPostProcessState::WaitingForReverse)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Reverse();
													}
												}
												else
												{
													CurrentVectorParam.Reverse();
												}
											}
											else if (CurrentVectorParam.PlayState == EPostProcessState::Forward)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Reverse();
													}
												}
												else
												{
													CurrentVectorParam.Reverse();
												}
											}
											else if (CurrentVectorParam.PlayState == EPostProcessState::Reverse)
											{
												if (CurrentVectorParam.ReverseRequirements.Num() > 0)
												{
													if (!FulfillsAllRequirements(CurrentVectorParam.ReverseRequirements))
													{
														CurrentVectorParam.Forward();
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void UPlayerPostProcessing::SetupPostProcessEffect(FPostProcessEffect& InPostProcessEffect)
{
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		if (InPostProcessEffect.PostProcesses[i].PostProcess_Data)
		{
			InPostProcessEffect.PostProcesses[i].PostProcess_MID = UMaterialInstanceDynamic::Create(InPostProcessEffect.PostProcesses[i].PostProcess_Data->PostProcess_Material, this);			
		}
	}
}

FPostProcessEffect* UPlayerPostProcessing::FindPostProcessEffect(const FName EffectName)
{
	for (FPostProcessEffect& PPEffect : PostProcessEffects)
	{
		if (PPEffect.EffectName == EffectName)
			return &PPEffect;
	}

	return nullptr;
}

void UPlayerPostProcessing::FadeToGrey()
{
	StartGreyscaleEffects();
}

void UPlayerPostProcessing::StartGreyscaleEffects()
{
	PlayPostProcessEffect_Name("Greyscale");
}

void UPlayerPostProcessing::StopGreyscaleEffects()
{
	StopPostProcessEffect_Name("Greyscale");
}

void UPlayerPostProcessing::StartNVGEffects()
{
	PlayPostProcessEffect_Name("NVG");
}

void UPlayerPostProcessing::StopNVGEffects()
{
	StopPostProcessEffect_Name("NVG");

	FWeightedBlendables CurrentActiveBlendables = Settings.WeightedBlendables;
	Settings = PlayersDefaultPostProcessSettings;
	Settings.WeightedBlendables = CurrentActiveBlendables;
	
}

void UPlayerPostProcessing::OnItemEquipped(ABaseItem* Item)
{
	if (!Item)
		return;
	
	if (Item->ContainsItemCategory(EItemCategory::IC_GasMask))
	{
		StartGasMaskEffects();
	}
}

void UPlayerPostProcessing::OnItemHolstered(ABaseItem* Item)
{
}

void UPlayerPostProcessing::ProcessNVG(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickNVGEffects);

	FPostProcessEffect* NVGEffects = FindPostProcessEffect("NVG");
	
	if (!NVGEffects)
		return;
	
	if (!NVGEffects->bEnabled)
		return;

	// Handle NVG
	if (OwningCharacter->bNVGOn)
	{
		if (!NVGEffects->bStarted)
		{
			StartNVGEffects();
		}
	}
	else
	{		
		if (NVGEffects->bStarted)
			StopNVGEffects();
	}
}

void UPlayerPostProcessing::StartGasMaskEffects()
{
	PlayPostProcessEffect_Name("Gas Mask");
}

void UPlayerPostProcessing::StopGasMaskEffects()
{
	StopPostProcessEffect_Name("Gas Mask");
}

void UPlayerPostProcessing::ProcessGasMask(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickGasMaskEffects);

	FPostProcessEffect* GasMaskEffects = FindPostProcessEffect("Gas Mask");
	if (!GasMaskEffects)
		return;

	if (OwningCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet && OwningCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet->ContainsItemCategory(EItemCategory::IC_GasMask))
	{
		if (!GasMaskEffects->bStarted)
			StartGasMaskEffects();
	}
	else
	{
		StopGasMaskEffects();
	}

	ProcessPostProcessEffect(*GasMaskEffects, DeltaTime);
}

void UPlayerPostProcessing::StartStingerEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Stinger");
}

void UPlayerPostProcessing::StopStingerEffect()
{
	StopPostProcessEffect_Name("Stinger");
}

void UPlayerPostProcessing::ProcessBloodEffect(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickBloodEffects);

	FPostProcessEffect* BloodEffects = FindPostProcessEffect("Blood");
	
	if (!BloodEffects)
		return;
	
	ProcessPostProcessEffect(*BloodEffects, DeltaTime);

	HitTopIntensity = FMath::FInterpConstantTo(HitTopIntensity, 0.0f, DeltaTime, 3.0f);
	HitBottomIntensity = FMath::FInterpConstantTo(HitBottomIntensity, 0.0f, DeltaTime, 3.0f);
	HitRightIntensity = FMath::FInterpConstantTo(HitRightIntensity, 0.0f, DeltaTime, 3.0f);
	HitLeftIntensity = FMath::FInterpConstantTo(HitLeftIntensity, 0.0f, DeltaTime, 3.0f);
}

void UPlayerPostProcessing::StartBloodEffect(AActor* DamageCauser, const float DirectionForward, const float DirectionRight)
{
	PlayPostProcessEffect_Name("Blood", DamageCauser);
}

void UPlayerPostProcessing::StopBloodEffect()
{
	StopPostProcessEffect_Name("Blood");
}

void UPlayerPostProcessing::StartBleedingEffect()
{
	PlayPostProcessEffect_Name("Bleed");
}

void UPlayerPostProcessing::StopBleedingEffect()
{
	StopPostProcessEffect_Name("Bleed");
}

void UPlayerPostProcessing::StartHealingEffect()
{
	PlayPostProcessEffect_Name("Healing");
}

void UPlayerPostProcessing::StopHealingEffect()
{
	StopPostProcessEffect_Name("Healing");
}

void UPlayerPostProcessing::PlayPostProcessEffect_Name(const FName EffectName, AActor* DamageCauser)
{
	if (EffectName.IsNone())
		return;
	
	for (FPostProcessEffect& PPEffect : PostProcessEffects)
	{
		if (PPEffect.EffectName == EffectName)
		{
			StartPostProcessEffect(PPEffect, DamageCauser);
			break;
		}
	}
}

void UPlayerPostProcessing::StopPostProcessEffect_Name(const FName EffectName)
{
	if (EffectName.IsNone())
		return;
	
	for (FPostProcessEffect& PPEffect: PostProcessEffects)
	{
		if (PPEffect.EffectName == EffectName)
		{
			StopPostProcessEffect(PPEffect);
			break;
		}
	}
}

void UPlayerPostProcessing::ResetInjuryRadialBlur(const float DeltaTime)
{
}

int32 UPlayerPostProcessing::InitializePostProcessFloatParam(const FPostProcessEffect& InPostProcessEffect, const FName& InParameterName, const float Value, const int32 Instance, const int32 CurveKey)
{
	TMap<UPostProcessEffectData*, FPostProcessSetting_FloatParam*> SeenParams;
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		const FPostProcessEffectPlayer& PPEffectSetting = InPostProcessEffect.PostProcesses[i];

		if (PPEffectSetting.bEnabled)
		{
			if (PPEffectSetting.PostProcess_Data)
			{
				FPostProcessSetting_FloatParam& FloatParam = PPEffectSetting.PostProcess_Data->GetParameter_Float(InParameterName);
				
				if (Instance > 0)
				{
					SeenParams.Add(PPEffectSetting.PostProcess_Data, &FloatParam);
				}
				else
				{
					if (&FloatParam != &PPEffectSetting.PostProcess_Data->GetInvalid_Float())
					{
						if (FloatParam.bUseCurve)
						{
							if (FloatParam.Curve.ExternalCurve)
							{
								if (FloatParam.Curve.ExternalCurve->FloatCurve.Keys.IsValidIndex(CurveKey))
									FloatParam.Curve.ExternalCurve->FloatCurve.Keys[CurveKey].Value = Value;

								//FloatParam.Curve.ExternalCurve->FloatCurve.UpdateOrAddKey(0.0f, Value);
							}
							else
							{
								if (FloatParam.Curve.EditorCurveData.Keys.IsValidIndex(CurveKey))
									FloatParam.Curve.EditorCurveData.Keys[CurveKey].Value = Value;

								//FloatParam.Curve.EditorCurveData.UpdateOrAddKey(0.0f, Value);
							}
						}
						else
						{
							if (CurveKey == 0)
								FloatParam.Start = Value;
							else
								FloatParam.End = Value;
						}

						#if WITH_EDITOR
						if (PPEffectSetting.PostProcess_Data->bDebug)
							ULog::Number(Value, PPEffectSetting.EffectName.ToString() + " | Initializing float param (" + FloatParam.ParameterName.ToString() + " (instance " + FString::FromInt(Instance) + ")) to: ");
						#endif

						return i;
					}
				}
			}
		}
	}

	if (Instance > 0 && SeenParams.Num() > 1)
	{
		TMap<UPostProcessEffectData*, FPostProcessSetting_FloatParam*> MultipleParams;
		for (auto Param : SeenParams)
		{
			if (Param.Value->ParameterName == InParameterName)
			{
				MultipleParams.Add(Param.Key, Param.Value);	
			}
		}

		if (Instance < MultipleParams.Num())
		{
			TArray<FPostProcessSetting_FloatParam*> Values;
			MultipleParams.GenerateValueArray(Values);

			const int32 CorrectedInstance = FMath::Clamp(Instance, 0, MultipleParams.Num() - 1);
			if (FPostProcessSetting_FloatParam*& FloatParam = Values[CorrectedInstance])
			{
				if (FloatParam->bUseCurve)
				{
					if (FloatParam->Curve.ExternalCurve)
					{
						if (FloatParam->Curve.ExternalCurve->FloatCurve.Keys.IsValidIndex(CurveKey))
							FloatParam->Curve.ExternalCurve->FloatCurve.Keys[CurveKey].Value = Value;

						//FloatParam.Curve.ExternalCurve->FloatCurve.UpdateOrAddKey(0.0f, Value);
					}
					else
					{
						if (FloatParam->Curve.EditorCurveData.Keys.IsValidIndex(CurveKey))
							FloatParam->Curve.EditorCurveData.Keys[CurveKey].Value = Value;

						//FloatParam.Curve.EditorCurveData.UpdateOrAddKey(0.0f, Value);
					}
				}
				else
				{
					if (CurveKey == 0)
						FloatParam->Start = Value;
					else
						FloatParam->End = Value;
				}

				TArray<UPostProcessEffectData*> MultiParam_Keys;
				MultipleParams.GenerateKeyArray(MultiParam_Keys);
				
				#if WITH_EDITOR
				if (MultiParam_Keys[CorrectedInstance]->bDebug)
					ULog::Number(Value, "Initializing float param (" + FloatParam->ParameterName.ToString() + " (instance " + FString::FromInt(CorrectedInstance) + ")) to: ");
				#endif

				TArray<UPostProcessEffectData*> Keys;
				SeenParams.GenerateKeyArray(Keys);
				
				return Keys.Find(MultiParam_Keys[CorrectedInstance]);
			}
		}
	}

	return INDEX_NONE;
}

int32 UPlayerPostProcessing::GetPostProcessFromFloatParam(const FPostProcessEffect& InPostProcessEffect, const FName& InParameterName, int32 Instance)
{
	TMap<UPostProcessEffectData*, FPostProcessSetting_FloatParam*> SeenParams;
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		const FPostProcessEffectPlayer& PPEffectSetting = InPostProcessEffect.PostProcesses[i];

		if (PPEffectSetting.bEnabled)
		{
			if (PPEffectSetting.PostProcess_Data)
			{
				FPostProcessSetting_FloatParam& FloatParam = PPEffectSetting.PostProcess_Data->GetParameter_Float(InParameterName);
				
				if (Instance > 0)
				{
					SeenParams.Add(PPEffectSetting.PostProcess_Data, &FloatParam);
				}
				else
				{
					if (&FloatParam != &PPEffectSetting.PostProcess_Data->GetInvalid_Float())
					{
						return i;
					}
				}
			}
		}
	}

	if (Instance > 0 && SeenParams.Num() > 1)
	{
		TMap<UPostProcessEffectData*, FPostProcessSetting_FloatParam*> MultipleParams;
		for (auto Param : SeenParams)
		{
			if (Param.Value->ParameterName == InParameterName)
			{
				MultipleParams.Add(Param.Key, Param.Value);	
			}
		}

		if (Instance < MultipleParams.Num())
		{
			const int32 CorrectedInstance = FMath::Clamp(Instance, 0, MultipleParams.Num() - 1);
			
			TArray<UPostProcessEffectData*> MultiParam_Keys;
			MultipleParams.GenerateKeyArray(MultiParam_Keys);
			
			TArray<UPostProcessEffectData*> SeenParam_Keys;
			SeenParams.GenerateKeyArray(SeenParam_Keys);
			
			return SeenParam_Keys.Find(MultiParam_Keys[CorrectedInstance]);
		}
	}

	return INDEX_NONE;
}

void UPlayerPostProcessing::StartInjuryTunnelVisionEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_VignetteEdge", InjuryVignetteEdge);
}

void UPlayerPostProcessing::StartInjuryHitEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_Hit", HitAmount);
}

void UPlayerPostProcessing::StartInjuryUnsharpEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_Sharpness", InjurySharpnessAmount);
}

void UPlayerPostProcessing::StartInjuryGreyscaleEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_Greyscale", InjuryGreyScaleAmount);
}

void UPlayerPostProcessing::StartInjuryRadialBlurEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	// Since there are two postprocesses with the same "Hit_RadialBlur" material param name, we need to skip the first instance of that param name and retrieve the second one, hence Instance = 1.
	// We're getting the second instance of "Hit_RadialBlur", instead of the first
	const int32 Index = GetPostProcessFromFloatParam(*InjuryEffects, "Hit_RadialBlur", 1);

	if (InjuryEffects->PostProcesses.IsValidIndex(Index) && !InjuryEffects->PostProcesses[Index].bStarted){}
		StartPostProcessEffect_Specific(InjuryEffects->PostProcesses[Index], DamageCauser);
}

void UPlayerPostProcessing::StartInjuryRedFlashEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_RedFlash", RedFlashAmount);
}

void UPlayerPostProcessing::StartInjuryWhiteFlashEffect(AActor* DamageCauser)
{
	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	// Update one specific params "start" value
	InitializePostProcessFloatParam(*InjuryEffects, "Injury_WhiteFlash", WhiteFlashAmount);
}

void UPlayerPostProcessing::StartDeathEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Death");
}

void UPlayerPostProcessing::StopDeathEffect()
{
	StopPostProcessEffect_Name("Death");
}

void UPlayerPostProcessing::StartInjuryEffects(AActor* DamageCauser)
{
	StartInjuryTunnelVisionEffect(DamageCauser);
	StartInjuryHitEffect(DamageCauser);
	StartInjuryUnsharpEffect(DamageCauser);
	StartInjuryRedFlashEffect(DamageCauser);
	StartInjuryGreyscaleEffect(DamageCauser);
	StartInjuryRadialBlurEffect(DamageCauser);
	StartInjuryWhiteFlashEffect(DamageCauser);

	PlayPostProcessEffect_Name("Injury");
}

void UPlayerPostProcessing::StopInjuryEffects()
{
	StopPostProcessEffect_Name("Injury");
}

void UPlayerPostProcessing::StartGasEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Gas");
}

void UPlayerPostProcessing::StopGasEffect()
{
	StopPostProcessEffect_Name("Gas");
}

void UPlayerPostProcessing::StartTaserEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Taser");
}

void UPlayerPostProcessing::StopTaserEffect()
{
	StopPostProcessEffect_Name("Taser");
}

void UPlayerPostProcessing::StartHeartbeatEffect()
{
	PlayPostProcessEffect_Name("Heartbeat");
}

void UPlayerPostProcessing::StopHeartbeatEffect()
{
	StopPostProcessEffect_Name("Heartbeat");
}

void UPlayerPostProcessing::ProcessInjuryEffects(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickInjuryEffects);

	FPostProcessEffect* InjuryEffects = FindPostProcessEffect("Injury");
	
	if (!InjuryEffects)
		return;
	
	ProcessPostProcessEffect(*InjuryEffects, DeltaTime);

	InjuryVignetteEdge = FMath::FInterpConstantTo(InjuryVignetteEdge, 0.0f, DeltaTime, 2.0f);
	HeadshotRadialBlurAmount = FMath::FInterpConstantTo(HeadshotRadialBlurAmount, 0.0f, DeltaTime, 0.125f);
	InjuryRadialBlurAmount = FMath::FInterpConstantTo(InjuryRadialBlurAmount, 0.0f, DeltaTime, 0.125f);
	InjurySharpnessAmount = FMath::FInterpConstantTo(InjurySharpnessAmount, 0.0f, DeltaTime, 3.0f);
	HitAmount = FMath::FInterpConstantTo(HitAmount, 0.0f, DeltaTime, HitAmountDrain);
	RedFlashAmount = FMath::FInterpConstantTo(RedFlashAmount, 0.0f, DeltaTime, 10.0f);
	WhiteFlashAmount = FMath::FInterpConstantTo(WhiteFlashAmount, 0.0f, DeltaTime, 8.8f);
	InjuryGreyScaleAmount = FMath::FInterpConstantTo(InjuryGreyScaleAmount, 0.0f, DeltaTime, 0.1f);
}

void UPlayerPostProcessing::StartSuppressionEffects(AActor* DamageCauser)
{
	FPostProcessEffect* SuppressionEffects = FindPostProcessEffect("Suppression");
	
	if (!SuppressionEffects)
		return;
	
	// Update BlurRadius specific params "end" value
	InitializePostProcessFloatParam(*SuppressionEffects, "BlurRadius", SupressionIntensity * 15.0f, 0, 1);
	
	PlayPostProcessEffect_Name("Suppression", DamageCauser);
}

void UPlayerPostProcessing::StopSuppressionEffects()
{
	StopPostProcessEffect_Name("Suppression");
}

void UPlayerPostProcessing::ProcessSuppressionEffect(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickSuppressionEffects);

	FPostProcessEffect* SuppressionEffects = FindPostProcessEffect("Suppression");
	
	if (!SuppressionEffects)
		return;
	
	ProcessPostProcessEffect(*SuppressionEffects, DeltaTime);

	SupressionIntensity = FMath::FInterpConstantTo(SupressionIntensity, 0.0f, DeltaTime, 0.2f);
}

void UPlayerPostProcessing::StartFlashbangEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Flashbang", DamageCauser);
}

void UPlayerPostProcessing::StopFlashbangEffect()
{
	StopPostProcessEffect_Name("Flashbang");
}

void UPlayerPostProcessing::StartPeppersprayEffect(AActor* DamageCauser)
{
	PlayPostProcessEffect_Name("Pepperspray", DamageCauser);
}

void UPlayerPostProcessing::StopPeppersprayEffect()
{
	StopPostProcessEffect_Name("Pepperspray");
}

void UPlayerPostProcessing::ProcessDepthOfField(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PlayerPP_TickDOFEffects);

	ABaseWeapon* Weapon = Cast<ABaseWeapon>(OwningCharacter->GetEquippedItem());
	UCameraComponent* CachedPlayerFPCameraDefaultObject = OwningCharacter->GetClass()->GetDefaultObject<APlayerCharacter>()->GetFirstPersonCameraComponent();
	if (OwningCharacter->bAiming && Weapon)
	{
		Settings.bOverride_DepthOfFieldFstop = true;
		Settings.bOverride_DepthOfFieldFocalDistance = true;
		
		UScopedWeaponAttachment* ScopeAttachment = Weapon->GetScopedAttachment();
		if (!ScopeAttachment || (ScopeAttachment && !ScopeAttachment->bUsePipRendering))
		{
			Settings.DepthOfFieldFstop = FMath::FInterpTo(Settings.DepthOfFieldFstop, 5.0f, DeltaTime, 10.0f);
			Settings.DepthOfFieldFocalDistance = FMath::FInterpTo(Settings.DepthOfFieldFocalDistance, 1000.0f, DeltaTime, 10.0f);
		}
		else if (ScopeAttachment && ScopeAttachment->bUsePipRendering)
		{
			Settings.bOverride_DepthOfFieldFocalRegion = true;
			Settings.DepthOfFieldFstop = FMath::FInterpTo(Settings.DepthOfFieldFstop, 10.0f, DeltaTime, 10.0f);
			Settings.DepthOfFieldFocalDistance = FMath::FInterpTo(Settings.DepthOfFieldFocalDistance, 50.0f, DeltaTime, 10.0f);
			Settings.DepthOfFieldFocalRegion = 200.0f;
		}
	}
	else
	{
		Settings.DepthOfFieldFstop = FMath::FInterpTo(Settings.DepthOfFieldFstop, CachedPlayerFPCameraDefaultObject->PostProcessSettings.DepthOfFieldFstop, DeltaTime, 10.0f);
		Settings.DepthOfFieldFocalDistance = FMath::FInterpTo(Settings.DepthOfFieldFocalDistance, CachedPlayerFPCameraDefaultObject->PostProcessSettings.DepthOfFieldFocalDistance, DeltaTime, 10.0f);

		if (Settings.DepthOfFieldFstop == CachedPlayerFPCameraDefaultObject->PostProcessSettings.DepthOfFieldFstop)
		{
			Settings.bOverride_DepthOfFieldFstop = false;
		}
		
		if (Settings.DepthOfFieldFocalDistance == CachedPlayerFPCameraDefaultObject->PostProcessSettings.DepthOfFieldFocalDistance)
		{
			Settings.bOverride_DepthOfFieldFocalDistance = false;
		}

		if (!Settings.bOverride_DepthOfFieldFstop && !Settings.bOverride_DepthOfFieldFocalDistance)
		{
			Settings.bOverride_DepthOfFieldFocalRegion = false;
		}
	}
}

void UPlayerPostProcessing::ProcessMotionBlur()
{
	UReadyOrNotGameUserSettings* GameUserSettings = UReadyOrNotFunctionLibrary::GetReadyOrNotGameUserSettings();
	if (!GameUserSettings)
		return;

	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = GameUserSettings->bMotionBlur ? GameUserSettings->MotionBlurStrength : 0.0f;
	Settings.MotionBlurTargetFPS = 0; // Adapt to current framerate
}

void UPlayerPostProcessing::OnPlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	//if (OwningCharacter->GetNetMode() != NM_Client)
	//{
	//	if (DeathEffects.HasEffectFinished())
			StartDeathEffect(InstigatorCharacter);
	//}
}

void UPlayerPostProcessing::OnPlayerStunned(AReadyOrNotCharacter* Character, float Duration, EStunType StunType, AActor* DamageCauser)
{
	// player stunned with no cause or owner?
	//check(OwningCharacter);

	RecentDamageCauser = DamageCauser;

	// Handle stun cases here
	switch (StunType)
	{
	case EStunType::ST_None:
	break;
		
	case EStunType::ST_Tased:
		StartTaserEffect(DamageCauser);
	break;
		
	case EStunType::ST_Gassed:
	{
		FPostProcessEffect* GasEffects = FindPostProcessEffect("Gas");
	
		if (!GasEffects)
			break;
		
		if (GasEffects->HasEffectFinished())
			StartGasEffect(DamageCauser);
	}
	break;
		
	case EStunType::ST_Flash:
		StartFlashbangEffect(DamageCauser);
	break;
		
	case EStunType::ST_Stung:
		StartStingerEffect(DamageCauser);
	break;

	case EStunType::ST_Pepperball:
	case EStunType::ST_Rubberball:
		StartStingerEffect(DamageCauser);
	break;
		
	default:
	break;
	}
}

void UPlayerPostProcessing::OnPlayerPepperSprayed(AActor* DamageCauser)
{
	RecentDamageCauser = DamageCauser;
	FPostProcessEffect* PeppersprayEffects = FindPostProcessEffect("Pepperspray");
	if (!PeppersprayEffects)
		return;
	
	if (PeppersprayEffects->HasEffectFinished())
		StartPeppersprayEffect(DamageCauser);
}

void UPlayerPostProcessing::OnBulletImpact(float DirectionForward, float DirectionRight)
{
	// how did a bullet impact with no player?
	check(OwningCharacter);

	StartHeartbeatEffect();

	StartBloodEffect(nullptr, DirectionForward, DirectionRight);
}

void UPlayerPostProcessing::OnSupression(float Strength)
{
	SupressionIntensity = FMath::Clamp(SupressionIntensity + (Strength * 0.05f), 0.0f, 1.0f);

	StartSuppressionEffects(nullptr);
}

void UPlayerPostProcessing::OnDamageTaken(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	//PlayPostProcessEffect_Name("Injury", DamageCauser);
	
	//StartPostProcessEffect(InjuryEffects, DamageCauser);
}

void UPlayerPostProcessing::OnDamageTakenDetails(bool bWasHeadshot, float DamageTaken, float HealthRemaining, bool bBlockedByArmour, bool bBlockedByHelmet)
{
	InjuryGreyScaleAmount = FMath::GetMappedRangeValueClamped(FVector2D(0.0f,100.0f),FVector2D(0.0f,0.6f), HealthRemaining);

	if (bBlockedByHelmet || bWasHeadshot)
	{
		HeadshotRadialBlurAmount = FMath::Clamp(HeadshotRadialBlurAmount + 1.0f, 0.0f, 5.0f);
	}

	if (bBlockedByArmour || bBlockedByHelmet)
	{
		WhiteFlashAmount += 1.6f;
		HitAmount = 0.0f;
		RedFlashAmount = 0.0f;
	}
	else
	{
  		HitAmount += (1.0f - UKismetMathLibrary::NormalizeToRange(DamageTaken, 0.0f, 100.0f));
 		HitAmount = FMath::Clamp(HitAmount, 0.0f, 3.0f);
  		HitAmountDrain = 1.0f;
 		RedFlashAmount = 2.0f;
	}

	InjuryVignetteEdge += (1.0f - UKismetMathLibrary::NormalizeToRange(HealthRemaining, 0.0f, 100.0f)) * 2.0f;
	InjuryVignetteEdge += (1.0f - UKismetMathLibrary::NormalizeToRange(DamageTaken, 0.0f, 100.0f)) * 1.0f;
	InjuryVignetteEdge = FMath::Clamp(InjuryVignetteEdge, 0.0f, 1.0f);
 	InjurySharpnessAmount = 3.0f;
	SupressionIntensity = 0.0f;

	if (HealthRemaining <= 0.0f)
	{
		HitAmount = 3.0f;
		HitAmountDrain = 0.1f;
		HeadshotRadialBlurAmount = 1.0f;
	}

	StartInjuryEffects(nullptr);
}

void UPlayerPostProcessing::ProcessOutlines()
{
	FPostProcessEffect* OutlineEffect = FindPostProcessEffect("WeaponHighlight");
	if (!OutlineEffect || OutlineEffect->PostProcesses.Num() <= 0)
		return;

	UMaterialInstanceDynamic* MaterialInstanceDynamic = OutlineEffect->PostProcesses[0].PostProcess_MID;
	if (!IsValid(MaterialInstanceDynamic))
		return;
	
	int32 DistanceSamples = FMath::Clamp(CVarRonOutlineBlurDistanceSamples.GetValueOnAnyThread(), 1, 32);
	if (DistanceSamples != LastOutlineBlurDistanceSamples)
	{
		MaterialInstanceDynamic->SetScalarParameterValue("BlurDistanceSamples", DistanceSamples);
		LastOutlineBlurDistanceSamples = DistanceSamples;
	}
	
	int32 RadialSamples = FMath::Clamp(CVarRonOutlineBlurRadialSamples.GetValueOnAnyThread(), 1, 32);
	if (RadialSamples != LastOutlineBlurRadialSamples)
	{
		MaterialInstanceDynamic->SetScalarParameterValue("BlurRadialSamples", RadialSamples);
		LastOutlineBlurRadialSamples = RadialSamples;
	}
}

void UPlayerPostProcessing::UpdateWeaponHighlightVisibility()
{
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	TArray<ABaseWeapon*> FoundActors;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseWeapon::StaticClass(), FoundActors);
	for (TActorIterator<ABaseWeapon> It(GetWorld()); It; ++It)
	{
		FoundActors.Add(*It);
	}

	if (us)
	{
		for (ABaseWeapon* Weapon : FoundActors)
		{
			if (Weapon->bInInventory || !(us->bHighlightWeapons))
			{
				Weapon->DisableOutline();
			}
			else if (us->bHighlightWeapons)
			{
				Weapon->DrawOutline();
			}
			//else
			//{
			//	Weapon->DisableOutline();
			//}
		}

		if (us->bHighlightWeapons && OwningCharacter)
		{
			PlayPostProcessEffect_Name("WeaponHighlight", OwningCharacter);
		}
		else
		{
			StopPostProcessEffect_Name("WeaponHighlight");
		}
	}
}

void UPlayerPostProcessing::OnFire()
{
	if (OwningCharacter)
	{
		if (OwningCharacter->IsLimbHit(ELimbType::LT_LeftLeg) || OwningCharacter->IsLimbHit(ELimbType::LT_RightLeg))
		{
			InjuryRadialBlurAmount = FMath::Clamp(InjuryRadialBlurAmount + 0.2f, 0.0f, 0.5f);
		}
	}
}

void UPlayerPostProcessing::OnRegister()
{
	Super::Super::OnRegister();
}

void UPlayerPostProcessing::OnUnregister()
{
	Super::Super::OnUnregister();
}

void UPlayerPostProcessing::Serialize(FArchive& Ar)
{
	Super::Super::Serialize(Ar);
}