// Copyright Void Interactive, 2021

#include "PostProcessEffectData.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

FPostProcessSetting_FloatParam& UPostProcessEffectData::GetParameter_Float(const FName& InParameterName)
{
	if (InParameterName.IsNone())
		return Invalid_FloatParam;
		
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		FPostProcessSetting_FloatParam& CurrentParam = ScalarParameters[i];

		if (CurrentParam.ParameterName == InParameterName)
			return CurrentParam;
	}

	return Invalid_FloatParam;
}

FPostProcessSetting_VectorParam& UPostProcessEffectData::GetParameter_Vector(const FName& InParameterName)
{
	if (InParameterName.IsNone())
		return Invalid_VectorParam;
		
	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		FPostProcessSetting_VectorParam& CurrentParam = VectorParameters[i];

		if (CurrentParam.ParameterName == InParameterName)
			return CurrentParam;
	}

	return Invalid_VectorParam;
}

bool UPostProcessEffectData::HasEffectFinished() const
{
	bool bHaveAllEffectsFinished = true;
	
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		const FPostProcessSetting_FloatParam& CurrentParam = ScalarParameters[i];
		
		if (CurrentParam.bEnabled)
		{
			if (CurrentParam.bUseCurve)
			{
				if ((CurrentParam.EffectEndOption == EPostProcessEndOptions::Hold && CurrentParam.PlayHead.IsPlaying()) ||
					CurrentParam.PlayHead.Status != FPlayHead::EPlayHeadStatus::Stopped ||
					CurrentParam.PlayState != EPostProcessState::Ended)
				{
					bHaveAllEffectsFinished = false;
					break;
				}
			}
			else
			{
				if ((CurrentParam.EffectEndOption == EPostProcessEndOptions::Hold && CurrentParam.PlayHead.IsPlaying()) ||
					CurrentParam.PlayHead.Status != FPlayHead::EPlayHeadStatus::Stopped ||
					CurrentParam.PlayState != EPostProcessState::Ended)
				{
					bHaveAllEffectsFinished = false;
					break;
				}
			}
		}
	}

	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		const FPostProcessSetting_VectorParam& CurrentParam = VectorParameters[i];
		
		if (CurrentParam.bEnabled)
		{
			if (CurrentParam.bUseCurve)
			{
				if ((CurrentParam.EffectEndOption == EPostProcessEndOptions::Hold && CurrentParam.PlayHead.IsPlaying()) ||
					CurrentParam.PlayHead.Status != FPlayHead::EPlayHeadStatus::Stopped ||
                    CurrentParam.PlayState != EPostProcessState::Ended)
				{
					bHaveAllEffectsFinished = false;
					break;
				}
			}
			else
			{
				if ((CurrentParam.EffectEndOption == EPostProcessEndOptions::Hold && CurrentParam.PlayHead.IsPlaying()) ||
					CurrentParam.PlayHead.Status != FPlayHead::EPlayHeadStatus::Stopped ||
					CurrentParam.PlayState != EPostProcessState::Ended)
				{
					bHaveAllEffectsFinished = false;
					break;
				}
			}
		}
	}

	return bHaveAllEffectsFinished;
}

float UPostProcessEffectData::GetGlobalTimeRemaining()
{
	float MaxTimeRemaining = 0.0f;
	
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		const FPostProcessSetting_FloatParam& CurrentParam = ScalarParameters[i];
		
		if (CurrentParam.bEnabled)
		{
			if (CurrentParam.bUseCurve)
			{
				const FRuntimeFloatCurve Curve = CurrentParam.Curve;

				if (Curve.ExternalCurve)
				{
					if (Curve.ExternalCurve->FloatCurve.Keys.Last().Time > MaxTimeRemaining)
						MaxTimeRemaining = Curve.ExternalCurve->FloatCurve.Keys.Last().Time;
				}
				else if (Curve.EditorCurveData.GetNumKeys() > 1)
				{
					if (Curve.EditorCurveData.Keys.Last().Time > MaxTimeRemaining)
						MaxTimeRemaining = Curve.EditorCurveData.Keys.Last().Time;
				}
			}
			else
			{
				if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Reverse)
					MaxTimeRemaining = CurrentParam.EffectLifetime * 2;
				else
					MaxTimeRemaining = CurrentParam.EffectLifetime;
			}
		}
	}
	
	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		const FPostProcessSetting_VectorParam& CurrentParam = VectorParameters[i];
		
		if (CurrentParam.bEnabled)
		{
			const FRuntimeCurveLinearColor& Curve = CurrentParam.Curve;

			if (CurrentParam.bUseCurve)
			{
				if (Curve.ExternalCurve)
				{
					for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ExternalCurve->FloatCurves); j++)
					{
						const float LastKeyTime = Curve.ExternalCurve->FloatCurves[j].Keys.Last().Time;

						if (LastKeyTime > MaxTimeRemaining)
							MaxTimeRemaining = LastKeyTime;
					}
				}
				else
				{
					for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ColorCurves); j++)
					{
						if (Curve.ColorCurves[j].Keys.Last().Time > MaxTimeRemaining)
							MaxTimeRemaining = Curve.ColorCurves[j].Keys.Last().Time;
					}
				}
			}
			else
			{
				if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Reverse)
					MaxTimeRemaining = CurrentParam.EffectLifetime * 2;
				else
					MaxTimeRemaining = CurrentParam.EffectLifetime;
			}
		}
	}

	return MaxTimeRemaining;
}

void UPostProcessEffectData::InitializeData(APlayerCharacter* OwningCharacter, AActor* RecentDamageCauser, bool bBypassRequirements)
{
	// Initialize scalar parameter settings
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		FPostProcessSetting_FloatParam& CurrentParam = ScalarParameters[i];

		if (CurrentParam.bEnabled && UReadyOrNotFunctionLibrary::FulfillsAllPostProcessRequirements(this, OwningCharacter, RecentDamageCauser, CurrentParam.Requirements, bBypassRequirements))
		{
			CurrentParam.ElapsedTime = 0.0f;

			if (CurrentParam.bUseCurve)
			{
				const FRuntimeFloatCurve Curve = CurrentParam.Curve;
				
				if (Curve.ExternalCurve)
				{
					CurrentParam.TimeRemaining = Curve.ExternalCurve->FloatCurve.Keys.Last().Time;

					CurrentParam.PlayHead.Start = Curve.ExternalCurve->FloatCurve.Keys[0].Time;
					CurrentParam.PlayHead.End = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;

					if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop)
					{
						TArray<FRichCurveKey> RichCurveKeys = Curve.ExternalCurve->FloatCurve.Keys;
						
						RichCurveKeys.Sort([](const FRichCurveKey& Lhs, const FRichCurveKey& Rhs)
						{
							return Lhs.Time < Rhs.Time;
						});
						
						if (RichCurveKeys.IsValidIndex(CurrentParam.StartLoopAtCurveKey))
						{
							CurrentParam.PlayHead.LoopStartTime = RichCurveKeys[CurrentParam.StartLoopAtCurveKey].Time;
						}
					}
				}
				else if (Curve.EditorCurveData.GetNumKeys() > 1)
				{
					CurrentParam.TimeRemaining = Curve.EditorCurveData.Keys.Last().Time;
					
					CurrentParam.PlayHead.Start = Curve.EditorCurveData.Keys[0].Time;
					CurrentParam.PlayHead.End = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
					
					if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop)
					{
						TArray<FRichCurveKey> RichCurveKeys = Curve.EditorCurveData.Keys;
						
						RichCurveKeys.Sort([](const FRichCurveKey& Lhs, const FRichCurveKey& Rhs)
						{
							return Lhs.Time < Rhs.Time;
						});
						
						if (RichCurveKeys.IsValidIndex(CurrentParam.StartLoopAtCurveKey))
						{
							CurrentParam.PlayHead.LoopStartTime = RichCurveKeys[CurrentParam.StartLoopAtCurveKey].Time;
						}
					}
				}

				CurrentParam.PlayState = EPostProcessState::Forward;
			}
			else
			{
				if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Reverse)
				{
					CurrentParam.EffectLifetime_WithReverse = CurrentParam.EffectLifetime * 2;
					CurrentParam.TimeRemaining = CurrentParam.EffectLifetime_WithReverse;

					CurrentParam.PlayHead.Start = FMath::Min(CurrentParam.Start, CurrentParam.End);
					CurrentParam.PlayHead.End = CurrentParam.EffectLifetime;
					CurrentParam.PlayHead.Speed = CurrentParam.InterpSpeed;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
				}
				else
				{
					CurrentParam.TimeRemaining = CurrentParam.EffectLifetime;

					CurrentParam.PlayHead.Start = FMath::Min(CurrentParam.Start, CurrentParam.End);
					CurrentParam.PlayHead.End = CurrentParam.EffectLifetime;
					CurrentParam.PlayHead.Speed = CurrentParam.InterpSpeed;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
				}

				switch (CurrentParam.StartingState)
				{
					case EPostProcessStartingState::Forward:
						CurrentParam.PlayState = EPostProcessState::Forward;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
					break;

					case EPostProcessStartingState::Reverse:
						CurrentParam.PlayState = EPostProcessState::Reverse;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Backwards;
						CurrentParam.PlayHead.JumpToEnd();
					break;

					default:
						CurrentParam.PlayState = EPostProcessState::Forward;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
					break;
				}
			}
			
			CurrentParam.PlayHead.Play(CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop);
		}
	}
	
	// Initialize vector parameter settings
	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		FPostProcessSetting_VectorParam& CurrentParam = VectorParameters[i];

		if (CurrentParam.bEnabled && UReadyOrNotFunctionLibrary::FulfillsAllPostProcessRequirements(this, OwningCharacter, RecentDamageCauser, CurrentParam.Requirements, bBypassRequirements))
		{
			CurrentParam.ElapsedTime = 0.0f;

			if (CurrentParam.bUseCurve)
			{
				const FRuntimeCurveLinearColor Curve = CurrentParam.Curve;
				
				if (Curve.ExternalCurve)
				{
					float MinTime = 0.0f;
					float MaxTime = 0.0f;
				
					for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ExternalCurve->FloatCurves); j++)
					{
						const float FirstKeyTime = Curve.ExternalCurve->FloatCurves[j].Keys[0].Time;
						const float LastKeyTime = Curve.ExternalCurve->FloatCurves[j].Keys.Last().Time;

						if (LastKeyTime > MaxTime)
							MinTime = FirstKeyTime;
						
						if (LastKeyTime > MaxTime)
							MaxTime = LastKeyTime;
					}
					
					CurrentParam.TimeRemaining = MaxTime;
					
					CurrentParam.PlayHead.Start = MinTime;
					CurrentParam.PlayHead.End = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;

					CurrentParam.PlayState = EPostProcessState::Forward;

					if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop)
					{
						TArray<FRichCurveKey> RichCurveKeys;
						
						for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ExternalCurve->FloatCurves); j++)
						{
							for (FRichCurveKey Key : Curve.ExternalCurve->FloatCurves[j].Keys)
							{
								RichCurveKeys.Add(Key);
							}
						}
						
						RichCurveKeys.Sort([](const FRichCurveKey& Lhs, const FRichCurveKey& Rhs)
						{
							return Lhs.Time < Rhs.Time;
						});
						
						if (RichCurveKeys.IsValidIndex(CurrentParam.StartLoopAtCurveKey))
						{
							CurrentParam.PlayHead.LoopStartTime = RichCurveKeys[CurrentParam.StartLoopAtCurveKey].Time;
						}
					}
				}
				else
				{
					float MinTime = 0.0f;
					float MaxTime = 0.0f;
				
					for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ColorCurves); j++)
					{
						if (Curve.ColorCurves[j].Keys[0].Time > MinTime)
							MinTime = Curve.ColorCurves[j].Keys[0].Time;
						
						if (Curve.ColorCurves[j].Keys.Last().Time > MaxTime)
							MaxTime = Curve.ColorCurves[j].Keys.Last().Time;
					}

					CurrentParam.TimeRemaining = MaxTime;

					CurrentParam.PlayHead.Start = MinTime;
					CurrentParam.PlayHead.End = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;

					CurrentParam.PlayState = EPostProcessState::Forward;

					if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop)
					{
						TArray<FRichCurveKey> RichCurveKeys;
						
						for (int32 j = 0; j < UE_ARRAY_COUNT(Curve.ColorCurves); j++)
						{
							for (FRichCurveKey Key : Curve.ColorCurves[j].Keys)
							{
								RichCurveKeys.Add(Key);
							}
						}
						
						RichCurveKeys.Sort([](const FRichCurveKey& Lhs, const FRichCurveKey& Rhs)
						{
							return Lhs.Time < Rhs.Time;
						});
						
						if (RichCurveKeys.IsValidIndex(CurrentParam.StartLoopAtCurveKey))
						{
							CurrentParam.PlayHead.LoopStartTime = RichCurveKeys[CurrentParam.StartLoopAtCurveKey].Time;
						}
					}
				}
			}
			else
			{
				if (CurrentParam.EffectEndOption == EPostProcessEndOptions::Reverse)
				{
					CurrentParam.EffectLifetime_WithReverse = CurrentParam.EffectLifetime * 2;
					CurrentParam.TimeRemaining = CurrentParam.EffectLifetime_WithReverse;

					CurrentParam.PlayHead.Start = 0.0f;
					CurrentParam.PlayHead.End = CurrentParam.EffectLifetime;
					CurrentParam.PlayHead.Speed = CurrentParam.InterpSpeed;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
				}
				else
				{
					CurrentParam.TimeRemaining = CurrentParam.EffectLifetime;

					CurrentParam.PlayHead.Start = 0.0f;
					CurrentParam.PlayHead.End = CurrentParam.EffectLifetime;
					CurrentParam.PlayHead.Speed = CurrentParam.InterpSpeed;
					CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.Start;
					CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
				}

				switch (CurrentParam.StartingState)
				{
					case EPostProcessStartingState::Forward:
						CurrentParam.PlayState = EPostProcessState::Forward;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
					break;

					case EPostProcessStartingState::Reverse:
						CurrentParam.PlayState = EPostProcessState::Reverse;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Backwards;
						CurrentParam.PlayHead.PositionOnTimeline = CurrentParam.PlayHead.End;
						CurrentParam.PlayHead.TimeRemaining = CurrentParam.TimeRemaining;
					break;

					default:
						CurrentParam.PlayState = EPostProcessState::Forward;
						CurrentParam.PlayHead.PlayDirection = EPostProcessPlayDirection::Forwards;
					break;
				}
			}
			
			CurrentParam.PlayHead.Play(CurrentParam.EffectEndOption == EPostProcessEndOptions::Loop);
		}
	}
}

void UPostProcessEffectData::Update(const float DeltaTime)
{
	// Process scalar parameter settings
	UpdateScalarParameters(DeltaTime);

	// Process vector parameter settings
	UpdateVectorParameters(DeltaTime);
}

void UPostProcessEffectData::UpdateScalarParameters(const float DeltaTime)
{
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		FPostProcessSetting_FloatParam& CurrentParam = ScalarParameters[i];

		if (CurrentParam.PlayHead.IsStopped())
		{
			CurrentParam.PlayState = EPostProcessState::Ended;
		}
		else
		{
			if (CurrentParam.bEnabled && CurrentParam.PlayHead.IsPlaying())
			{
				CurrentParam.PlayHead.Update(DeltaTime);

				CurrentParam.ElapsedTime = CurrentParam.PlayHead.PositionOnTimeline;
				CurrentParam.TimeRemaining = CurrentParam.PlayHead.TimeRemaining;

				switch (CurrentParam.PlayState)
				{
					case EPostProcessState::Forward:
					{
						switch (CurrentParam.EffectEndOption)
						{
							case EPostProcessEndOptions::End:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;
					
							case EPostProcessEndOptions::Hold:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Hold;
								}
							}
							break;
					
							case EPostProcessEndOptions::Reverse:
							{
								if (CurrentParam.PlayHead.IsAtEnd())
								{
									CurrentParam.TimeRemaining = CurrentParam.ElapsedTime;
									CurrentParam.PlayState = EPostProcessState::WaitingForReverse;
								}
							}
							break;
					
							default:
							break;
						}
					}
					break;

					case EPostProcessState::Reverse:
					{
						CurrentParam.ElapsedTime = CurrentParam.PlayHead.PositionOnTimeline;
						CurrentParam.TimeRemaining = CurrentParam.PlayHead.TimeRemaining;
						
						switch (CurrentParam.EffectEndOption)
						{
							case EPostProcessEndOptions::End:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;

							case EPostProcessEndOptions::Hold:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Hold;
								}
							}
							break;

							case EPostProcessEndOptions::Reverse:
							{
								if (CurrentParam.PlayHead.IsAtStart())
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;

							default:
							break;
						}
					}
					break;

					case EPostProcessState::WaitingForReverse:
						CurrentParam.TimeRemaining = CurrentParam.ElapsedTime;
					break;

					case EPostProcessState::Ended:
						CurrentParam.PlayHead.Status = FPlayHead::EPlayHeadStatus::Stopped;
					break;

					default:
					break;
				}

				#if WITH_EDITOR
				if (bDebug)
				{
					const FString PlayStateString = ENUM_TO_STRING(EPostProcessState, CurrentParam.PlayState, true);
					const FString TimeRemainingString = FString::SanitizeFloat(CurrentParam.TimeRemaining);
					const FString ElapsedTimeString = FString::SanitizeFloat(CurrentParam.ElapsedTime);
					
					float CurveValue = 0.0f;
					if (CurrentParam.bUseCurve)
					{
						if (CurrentParam.Curve.ExternalCurve)
							CurveValue = CurrentParam.Curve.ExternalCurve->GetFloatValue(CurrentParam.PlayState >= EPostProcessState::Reverse ? CurrentParam.TimeRemaining : CurrentParam.ElapsedTime);
						else if (CurrentParam.Curve.EditorCurveData.GetNumKeys() > 1)
							CurveValue = CurrentParam.Curve.EditorCurveData.Eval(CurrentParam.PlayState >= EPostProcessState::Reverse ? CurrentParam.TimeRemaining : CurrentParam.ElapsedTime);
					}
					else
					{
						const float Alpha = CurrentParam.PlayState >= EPostProcessState::Reverse ? CurrentParam.TimeRemaining : CurrentParam.ElapsedTime;

						CurveValue = UKismetMathLibrary::Ease(CurrentParam.Start, CurrentParam.End, Alpha/CurrentParam.EffectLifetime, CurrentParam.EasingMethod);
					}

					const FString CurveValueString = FString::SanitizeFloat(CurveValue);

					const FString DebugInfo = CurrentParam.ParameterName.ToString() + " | " + PlayStateString + " | " + "Time Remaining: " + TimeRemainingString + " | Elapsed Time: " + ElapsedTimeString + " | Curve value: " + CurveValueString;
					
					ULog::Info(DebugInfo);
				}
				#endif
			}
		}
	}
}

void UPostProcessEffectData::UpdateVectorParameters(const float DeltaTime)
{
	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		FPostProcessSetting_VectorParam& CurrentParam = VectorParameters[i];
		
		if (CurrentParam.PlayHead.IsStopped())
		{
			CurrentParam.PlayState = EPostProcessState::Ended;
		}
		else
		{
			if (CurrentParam.bEnabled && CurrentParam.PlayHead.IsPlaying())
			{
				CurrentParam.PlayHead.Update(DeltaTime);

				CurrentParam.ElapsedTime = CurrentParam.PlayHead.PositionOnTimeline;
				CurrentParam.TimeRemaining = CurrentParam.PlayHead.TimeRemaining;

				switch (CurrentParam.PlayState)
				{
					case EPostProcessState::Forward:
					{
						switch (CurrentParam.EffectEndOption)
						{
							case EPostProcessEndOptions::End:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;
				
							case EPostProcessEndOptions::Hold:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Hold;
								}
							}
							break;
				
							case EPostProcessEndOptions::Reverse:
							{
								if (CurrentParam.PlayHead.IsAtEnd())
								{
									CurrentParam.TimeRemaining = CurrentParam.ElapsedTime;
									CurrentParam.PlayState = EPostProcessState::WaitingForReverse;
								}
							}
							break;
				
							default:
							break;
						}
					}
					break;

					case EPostProcessState::Reverse:
					{
						CurrentParam.ElapsedTime = CurrentParam.PlayHead.PositionOnTimeline;
						CurrentParam.TimeRemaining = CurrentParam.PlayHead.TimeRemaining;
							
						switch (CurrentParam.EffectEndOption)
						{
							case EPostProcessEndOptions::End:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;

							case EPostProcessEndOptions::Hold:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Hold;
								}
							}
							break;
								
							default:
							{
								if (CurrentParam.TimeRemaining <= 0.0f)
								{
									CurrentParam.PlayState = EPostProcessState::Ended;
								}
							}
							break;
						}
					}
					break;

					case EPostProcessState::WaitingForReverse:
						CurrentParam.TimeRemaining = CurrentParam.ElapsedTime;
					break;

					case EPostProcessState::Ended:
						CurrentParam.PlayHead.Status = FPlayHead::EPlayHeadStatus::Stopped;
					break;

					default:
					break;
				}

				#if WITH_EDITOR
				if (bDebug)
				{
					const FString PlayStateString = ENUM_TO_STRING(EPostProcessState, CurrentParam.PlayState, true);
					const FString TimeRemainingString = FString::SanitizeFloat(CurrentParam.TimeRemaining);
					const FString ElapsedTimeString = FString::SanitizeFloat(CurrentParam.ElapsedTime);
					
					FLinearColor CurveValue;
					if (CurrentParam.bUseCurve)
					{
						CurveValue = CurrentParam.Curve.GetLinearColorValue(CurrentParam.PlayState >= EPostProcessState::Reverse ? CurrentParam.TimeRemaining : CurrentParam.ElapsedTime);
					}
					else
					{
						const float Alpha = CurrentParam.PlayState >= EPostProcessState::Reverse ? CurrentParam.TimeRemaining : CurrentParam.ElapsedTime;
						CurveValue = FLinearColor(UKismetMathLibrary::VEase(CurrentParam.Start, CurrentParam.End, Alpha, CurrentParam.EasingMethod));
					}

					const FString CurveValueString = CurveValue.ToString();

					const FString DebugInfo = CurrentParam.ParameterName.ToString() + " | " + PlayStateString + " | " + "Time Remaining: " + TimeRemainingString + " | Elapsed Time: " + ElapsedTimeString + " | Curve value: " + CurveValueString;
					
					ULog::Info(DebugInfo, LO_Console);
				}
				#endif
			}
		}
	}
}

void UPostProcessEffectData::ResetData()
{
	// Reset scalar parameter settings
	for (int32 i = 0; i < ScalarParameters.Num(); i++)
	{
		ScalarParameters[i].ElapsedTime = 0.0f;
		ScalarParameters[i].TimeRemaining = 0.0f;
		ScalarParameters[i].PlayState = EPostProcessState::Ended;
		ScalarParameters[i].PlayHead.Stop();
	}

	// Reset vector parameter settings
	for (int32 i = 0; i < VectorParameters.Num(); i++)
	{
		VectorParameters[i].ElapsedTime = 0.0f;
		VectorParameters[i].TimeRemaining = 0.0f;
		VectorParameters[i].PlayState = EPostProcessState::Ended;
		VectorParameters[i].PlayHead.Stop();
	}
}
