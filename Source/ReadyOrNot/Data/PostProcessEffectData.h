// Copyright Void Interactive, 2021

#pragma once

#include "Characters/PlayerCharacter.h"
#include "Engine/DataAsset.h"
#include "PostProcessEffectData.generated.h"

UENUM()
enum class EPostProcessPlayDirection : uint8
{
	Forwards,
	Backwards
};

UENUM()
enum class EPostProcessStartingState : uint8
{
	Forward,
	Reverse
};

UENUM()
enum class EPostProcessEndOptions : uint8
{
	End,
	Hold,
	Loop,
	Reverse
};

UENUM(BlueprintType)
enum class EPostProcessState : uint8
{
	Hold,
	Forward,
	Reverse,
	WaitingForReverse,
	Ended
};

USTRUCT()
struct FPlayHead
{
	GENERATED_BODY()
	
	enum class EPlayHeadStatus : uint8
	{
		Playing,
		Paused,
		Stopped
	};
	
	void Play(const bool bLooping = false)
	{
		Status = EPlayHeadStatus::Playing;
		bLoop = bLooping;
	}

	void Pause()
	{
		Status = EPlayHeadStatus::Paused;
	}

	void Stop()
	{
		Status = EPlayHeadStatus::Stopped;
	}

	void Restart(const float OptionalStartTime = 0.0f)
	{
		Status = EPlayHeadStatus::Playing;

		if (OptionalStartTime > 0.0f)
			JumpTo(OptionalStartTime);
		else
			JumpToStart();
	}
	
	void Forward()
	{
		PlayDirection = EPostProcessPlayDirection::Forwards;
		TimeRemaining = End - PositionOnTimeline;
	}
	
	void Reverse()
	{
		PlayDirection = EPostProcessPlayDirection::Backwards;
		TimeRemaining = PositionOnTimeline;
	}

	void JumpToStart()
	{
		PositionOnTimeline = Start;
		TimeRemaining = End;
	}

	void JumpToEnd()
	{
		PositionOnTimeline = End;
		TimeRemaining = PlayDirection == EPostProcessPlayDirection::Forwards ? 0.0f : End;
	}

	void JumpTo(const float InTime)
	{
		PositionOnTimeline = FMath::Clamp(InTime, Start, End);
		
		if (PlayDirection == EPostProcessPlayDirection::Forwards)
			TimeRemaining = End - PositionOnTimeline;
		else
			TimeRemaining = PositionOnTimeline;
	}
	
	bool IsAtStart() const
	{
		return PositionOnTimeline <= Start;
	}
	
	bool IsAtEnd() const
	{
		return PositionOnTimeline >= End;
	}

	bool IsAt(const float InTime, const float InTolerance = 0.001f) const
	{
		return UKismetMathLibrary::NearlyEqual_FloatFloat(PositionOnTimeline, InTime, InTolerance);
	}

	bool IsPlaying() const
	{
		return Status == EPlayHeadStatus::Playing;
	}
	
	bool IsStopped() const
	{
		return Status == EPlayHeadStatus::Stopped;
	}
	
	void Update(const float DeltaTime)
	{
		switch (Status)
		{
			case EPlayHeadStatus::Playing:
				switch (PlayDirection)
				{
					case EPostProcessPlayDirection::Forwards:
					{
						PositionOnTimeline = FMath::Clamp(PositionOnTimeline + Speed * DeltaTime, Start, End);
						TimeRemaining = FMath::Clamp(TimeRemaining - Speed * DeltaTime, 0.0f, End);
						
						if (bLoop && TimeRemaining <= 0.0f)
						{
							Restart(LoopStartTime);
						}
					}
					break;

					case EPostProcessPlayDirection::Backwards:
					{
						PositionOnTimeline = FMath::Clamp(PositionOnTimeline - Speed * DeltaTime, Start, End);
						TimeRemaining = FMath::Clamp(TimeRemaining - Speed * DeltaTime, 0.0f, End);

						if (bLoop && TimeRemaining <= 0.0f)
						{
							Restart(LoopStartTime);
						}
					}
					break;
				}
			break;

			case EPlayHeadStatus::Paused:
			break;

			case EPlayHeadStatus::Stopped:
				PositionOnTimeline = Start;
				TimeRemaining = End;
				PlayDirection = EPostProcessPlayDirection::Forwards;
			break;
		}
	}
	
	float Start = 0.0f;
	float End = 1.0f;
	float Speed = 1.0f;
	
	float PositionOnTimeline = 0.0f;
	float TimeRemaining = 0.0f;

	float LoopStartTime = 0.0f;
	uint8 bLoop : 1;

	EPostProcessPlayDirection PlayDirection = EPostProcessPlayDirection::Forwards;

	EPlayHeadStatus Status = EPlayHeadStatus::Stopped;
};

USTRUCT(BlueprintType)
struct FPostProcessSetting_Base
{
	GENERATED_BODY()

	FPostProcessSetting_Base()
	{
		bEnabled = true;
		bUseCurve = true;
		bReverseAtAnyTime = false;
		EffectLifetime_WithReverse = EffectLifetime * 2;
		PlayState = EPostProcessState::Ended;
		StartingState = EPostProcessStartingState::Forward;
		EffectEndOption = EPostProcessEndOptions::End;
	}

	void Forward()
	{
		if (bEnabled)
		{
			PlayState = EPostProcessState::Forward;
			PlayHead.Forward();
		}
	}

	void Reverse()
	{
		if (bEnabled && (EffectEndOption == EPostProcessEndOptions::Reverse || bReverseAtAnyTime))
		{
			PlayState = EPostProcessState::Reverse;
			PlayHead.Reverse();
		}
	}

    UPROPERTY(EditAnywhere, Category = "Post Process Settings")
	uint8 bEnabled : 1;

	UPROPERTY(EditAnywhere, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
	TArray<TSubclassOf<class UPostProcessRequirement>> Requirements;
	
    UPROPERTY(EditAnywhere, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
    FName ParameterName = "None";
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
	EPostProcessEndOptions EffectEndOption = EPostProcessEndOptions::End;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && bUseCurve && EffectEndOption == EPostProcessEndOptions::Loop"))
	int32 StartLoopAtCurveKey = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
	uint8 bReverseAtAnyTime : 1;

	UPROPERTY(EditAnywhere, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && bReverseAtAnyTime || EffectEndOption == EPostProcessEndOptions::Reverse"))
	TArray<TSubclassOf<class UPostProcessRequirement>> ReverseRequirements;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
	uint8 bUseCurve : 1;
	
    UPROPERTY(EditAnywhere, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve"))
	TEnumAsByte<EEasingFunc::Type> EasingMethod = EEasingFunc::Linear;

    UPROPERTY(EditAnywhere, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve"))
	EPostProcessStartingState StartingState = EPostProcessStartingState::Forward;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve"))
	float InterpSpeed = 1.0f;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve"), DisplayName = "Effect Lifetime (seconds)")
	float EffectLifetime = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve && EffectEndOption == EPostProcessEndOptions::Loop"))
	float StartLoopAtTime = 0.0f;
	
	float EffectLifetime_WithReverse = 0.0f;

	float TimeRemaining = 0.0f;
	float ElapsedTime = 0.0f;

	EPostProcessState PlayState = EPostProcessState::Ended;

	FPlayHead PlayHead = {};
};
	
USTRUCT(BlueprintType)
struct FPostProcessSetting_FloatParam : public FPostProcessSetting_Base
{
	GENERATED_BODY()

	FPostProcessSetting_FloatParam()
	{
		bEnabled = true;
		ParameterName = "Scalar";
	}	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve", DisplayAfter = "StartingState"))
	float Start = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve", DisplayAfter = "Start"))
    float End = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Post Process Float Curve", meta = (EditCondition = "bEnabled && bUseCurve", DisplayAfter = "bUseCurve"))
	FRuntimeFloatCurve Curve{};
};

USTRUCT(BlueprintType)
struct FPostProcessSetting_VectorParam : public FPostProcessSetting_Base
{
	GENERATED_BODY()
	
	FPostProcessSetting_VectorParam()
	{
		bEnabled = true;
		ParameterName = "Vector";
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve", DisplayAfter = "StartingState"))
    FVector Start = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process Settings", meta = (EditCondition = "bEnabled && !bUseCurve", DisplayAfter = "Start"))
    FVector End = FVector::OneVector;
	
	UPROPERTY(EditAnywhere, Category = "Post Process Vector Curve", meta = (EditCondition = "bEnabled && bUseCurve", DisplayAfter = "bUseCurve"))
    FRuntimeCurveLinearColor Curve{};
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UPostProcessEffectData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Effect")
	UMaterialInterface* PostProcess_Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Effect")
	TArray<FPostProcessSetting_FloatParam> ScalarParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Effect")
    TArray<FPostProcessSetting_VectorParam> VectorParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Misc")
	uint8 bDebug : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Misc", meta = (MultiLine = "true"))
	FText Note;

	FPostProcessSetting_FloatParam& GetParameter_Float(const FName& InParameterName);
	
	FPostProcessSetting_VectorParam& GetParameter_Vector(const FName& InParameterName);
	
	FPostProcessSetting_FloatParam& GetInvalid_Float() { return Invalid_FloatParam; }
	FPostProcessSetting_VectorParam& GetInvalid_Vector() { return Invalid_VectorParam; }

	bool HasEffectFinished() const;

	float GetGlobalTimeRemaining();
	
	void InitializeData(APlayerCharacter* OwningCharacter, AActor* RecentDamageCauser = nullptr, bool bBypassRequirements = false);
	
	void Update(float DeltaTime);
	void UpdateScalarParameters(float DeltaTime);
	void UpdateVectorParameters(float DeltaTime);

	void ResetData();
	
private:
	FPostProcessSetting_FloatParam Invalid_FloatParam;
	FPostProcessSetting_VectorParam Invalid_VectorParam;
};

USTRUCT(BlueprintType)
struct FPostProcessEffectPlayer
{
	GENERATED_USTRUCT_BODY()

	FPostProcessEffectPlayer()
	{
		bEnabled = true;
		bDebug = false;
		bStarted = false;
		bRestartIfAlreadyPlaying = false;
		bWantsFadeOut = false;
		PostProcess_MID = nullptr;
		PostProcess_Data = nullptr;
	}

	FORCEINLINE bool CanStopPostProcessEffect() const
	{
		if (PostProcess_Data)
		{
			return PostProcess_Data->HasEffectFinished();
		}
		
		return true;
	}

	void Start(APlayerCharacter* OwningCharacter, AActor* DamageCauser = nullptr)
	{
		if (bStarted && bRestartIfAlreadyPlaying)
		{
			Reset();
		}
		
		if (PostProcess_Data)
		{
			PostProcess_Data->InitializeData(OwningCharacter, DamageCauser);
			bStarted = true;
		}
	}

	void Update(const float DeltaTime)
	{
		if (PostProcess_Data)
		{
			PostProcess_Data->Update(DeltaTime);
		}
	}
	
	void Reset()
	{
		if (PostProcess_Data)
		{
			PostProcess_Data->ResetData();
		}

		bStarted = false;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting")
	uint8 bEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"))
	uint8 bDebug : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"))
	uint8 bRestartIfAlreadyPlaying : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"))
	uint8 bWantsFadeOut : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled && bWantsFadeOut"))
	float FadeOutSpeed = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"), DisplayName = "Requirements")
	TArray<TSubclassOf<class UPostProcessRequirement>> RequirementsClasses;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
    FName EffectName = "Effect";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"))
	class UPostProcessEffectData* PostProcess_Data;
	
	UPROPERTY(BlueprintReadOnly, Category = "Post Process Setting")
	UMaterialInstanceDynamic* PostProcess_MID;
	
	UPROPERTY(BlueprintReadOnly, Category = "Post Process Settings")
	uint8 bStarted : 1;
};

USTRUCT(BlueprintType)
struct FPostProcessEffect
{
	GENERATED_USTRUCT_BODY()

    FPostProcessEffect()
	{
		bEnabled = true;
		bDebug = false;
		bStarted = false;
		bCustomProcess = false;
	}

	FORCEINLINE bool HasEffectFinished() const
	{
		bool bAllEffectsFinished = true;
		for (int32 i = 0; i < PostProcesses.Num(); i++)
		{
			const FPostProcessEffectPlayer& CurrentPPEffect = PostProcesses[i];

			if (CurrentPPEffect.bEnabled && CurrentPPEffect.bStarted)
			{
				if (!CurrentPPEffect.CanStopPostProcessEffect())
				{
					bAllEffectsFinished = false;
					break;
				}
			}
		}

		return bAllEffectsFinished;
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings")
    uint8 bEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
    uint8 bCustomProcess : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
	uint8 bDebug : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Setting", meta = (EditCondition = "bEnabled"))
	FName EffectName = "None";
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process Settings", meta = (EditCondition = "bEnabled"))
    TArray<FPostProcessEffectPlayer> PostProcesses;
    
	UPROPERTY(BlueprintReadOnly, Category = "Post Process Settings")
	uint8 bStarted : 1;
};
