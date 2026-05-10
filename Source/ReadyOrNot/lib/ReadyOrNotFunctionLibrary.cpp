// Copyright Void Interactive, 2022

#include "ReadyOrNotFunctionLibrary.h"
#include "FMODStudio/Classes/FMODBus.h"
#include "Animation/AnimationDataTable.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "HAL/PlatformApplicationMisc.h"

#if defined(WITH_STEAM)
#include "steam/steam_gameserver.h"
#endif

#include "lib/DataSingleton.h"

#include "Engine/Engine.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"

#include "Info/SWATManager.h"

#include "Data/PostProcessRequirement.h"

#include "Components/SafeZoneSlot.h"

#include "Actors/Splines/SplineTrigger.h"

// ##UE5UPGRADE## Re add this after DLSS upgraded   
//#include "DLSSBlueprint/Public/DLSSLibrary.h"

#include "Components/PlanarReflectionComponent.h"
#include "GameModes/CoopGM.h"
#include "GameModes/CoopGS.h"
#include "Misc/RemoteConfigIni.h"

#if !UE_BUILD_SHIPPING
//#include "DevMenuFunctionLibrary.h"
#endif

#include "CommonInputSubsystem.h"
#include "NavigationSystem.h"
#include "Actors/WorldDataGenerator.h"
#include "Debug/BadAIAction.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"
#include "GameModes/DefusalGS.h"
#include "Kismet/KismetTextLibrary.h"

// #if PLATFORM_WINDOWS
// // this might look weird but it fixes a recompile error about allow windows types
// #include "Windows/AllowWindowsPlatformTypes.h"
// #include <windows.h>
// #include <tlhelp32.h>
// #include <Psapi.h>
// #include <string>
// #include "Windows/HideWindowsPlatformTypes.h"
// #endif

void UReadyOrNotFunctionLibrary::SetupTimeline(UObject* Object, FTimeline& InTimeline, TArray<UCurveFloat*> InCurveFloats, const bool bLooping, const float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName)
{
	FOnTimelineFloat TimelineCallback;
	TimelineCallback.BindUFunction(Object, TimelineCallbackFuncName);

	float MaxTime = 0.0f;
	for (int32 i = 0; i < InCurveFloats.Num(); i++)
	{
		UCurveFloat* FloatCurve = InCurveFloats[i];
		if (FloatCurve)
		{
			InTimeline.AddInterpFloat(FloatCurve, TimelineCallback);

			const float FloatCurveMaxTime = FloatCurve->FloatCurve.Keys[FloatCurve->FloatCurve.Keys.Num() - 1].Time;
			if (FloatCurveMaxTime > MaxTime)
				MaxTime = FloatCurveMaxTime;
		}
	}

	InTimeline.SetTimelineLength(MaxTime);

	if (TimelineCallbackFuncName != NAME_None)
	{
		FOnTimelineEvent TimelineFinishedCallback;
		TimelineFinishedCallback.BindUFunction(Object, TimelineFinishedCallbackFuncName);
		InTimeline.SetLooping(bLooping);
		InTimeline.SetPlayRate(InPlaybackSpeed);
		InTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
		InTimeline.SetTimelineLengthMode(TL_TimelineLength);
	}
}

void UReadyOrNotFunctionLibrary::SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveFloat* InCurveFloat, const bool bLooping, const float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName)
{
	FOnTimelineFloat TimelineCallback;
	TimelineCallback.BindUFunction(Object, TimelineCallbackFuncName);

	if (InCurveFloat)
	{
		InTimeline.AddInterpFloat(InCurveFloat, TimelineCallback);
		InTimeline.SetTimelineLength(InCurveFloat->FloatCurve.Keys[InCurveFloat->FloatCurve.Keys.Num() - 1].Time);
	}

	if (TimelineCallbackFuncName != NAME_None)
	{
		FOnTimelineEvent TimelineFinishedCallback;
		TimelineFinishedCallback.BindUFunction(Object, TimelineFinishedCallbackFuncName);
		InTimeline.SetLooping(bLooping);
		InTimeline.SetPlayRate(InPlaybackSpeed);
		InTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
		InTimeline.SetTimelineLengthMode(TL_TimelineLength);
	}
}

void UReadyOrNotFunctionLibrary::SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveLinearColor* InCurveLinearColor, const bool bLooping, const float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName)
{
	FOnTimelineLinearColor TimelineCallback;
	TimelineCallback.BindUFunction(Object, TimelineCallbackFuncName);

	if (InCurveLinearColor)
	{
		InTimeline.AddInterpLinearColor(InCurveLinearColor, TimelineCallback);
		InTimeline.SetTimelineLength(InCurveLinearColor->FloatCurves->Keys[InCurveLinearColor->FloatCurves->GetNumKeys() - 1].Time);
	}

	if (TimelineCallbackFuncName != NAME_None)
	{
		FOnTimelineEvent TimelineFinishedCallback;
		TimelineFinishedCallback.BindUFunction(Object, TimelineFinishedCallbackFuncName);
		InTimeline.SetLooping(bLooping);
		InTimeline.SetPlayRate(InPlaybackSpeed);
		InTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
		InTimeline.SetTimelineLengthMode(TL_TimelineLength);
	}
}

void UReadyOrNotFunctionLibrary::SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveVector* InCurveVector, const bool bLooping, const float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName)
{
	FOnTimelineVector TimelineCallback;
	TimelineCallback.BindUFunction(Object, TimelineCallbackFuncName);

	if (InCurveVector)
	{
		InTimeline.AddInterpVector(InCurveVector, TimelineCallback);
		InTimeline.SetTimelineLength(InCurveVector->FloatCurves->Keys[InCurveVector->FloatCurves->GetNumKeys() - 1].Time);
	}

	if (TimelineCallbackFuncName != NAME_None)
	{
		FOnTimelineEvent TimelineFinishedCallback;
		TimelineFinishedCallback.BindUFunction(Object, TimelineFinishedCallbackFuncName);
		InTimeline.SetLooping(bLooping);
		InTimeline.SetPlayRate(InPlaybackSpeed);
		InTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
		InTimeline.SetTimelineLengthMode(TL_TimelineLength);
	}
}

void UReadyOrNotFunctionLibrary::StopCallbackTimer(UObject* Object, FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			Object->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
}

void UReadyOrNotFunctionLibrary::ResumeCallbackTimer(UObject* Object, FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			Object->GetWorld()->GetTimerManager().UnPauseTimer(TimerHandle);
		}
	}
}

void UReadyOrNotFunctionLibrary::PauseCallbackTimer(UObject* Object, FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			Object->GetWorld()->GetTimerManager().PauseTimer(TimerHandle);
		}
	}
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerActive(UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerActive(TimerHandle);
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerActive(const UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerActive(TimerHandle);
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerPaused(UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerPaused(TimerHandle);
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerPaused(const UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerPaused(TimerHandle);
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerPending(UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerPending(TimerHandle);
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::IsCallbackTimerPending(const UObject* Object, const FTimerHandle& TimerHandle)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			return Object->GetWorld()->GetTimerManager().IsTimerPending(TimerHandle);
		}
	}

	return false;
}

UAnimMontage* UReadyOrNotFunctionLibrary::GetAnimationFromTable(const FString& AnimationName, const bool bIsCrouching)
{
	UDataTable* DT = GetRoNData()->AnimationDataLookupTable;
	if (DT)
	{
		FAnimationDataTable* LookupRow = DT->FindRow<FAnimationDataTable>(*AnimationName, "Animation Lookup");
		if (LookupRow)
		{
			const EAnimWeaponType WeaponType = EAnimWeaponType::CWT_Any;

			FAnimStanceData StanceData = *LookupRow->AnimData.Find(WeaponType);
			FAnimWeaponData WeaponAnimData = bIsCrouching ? StanceData.CrouchedAnimData : StanceData.StandingAnimData;
		
			if (WeaponAnimData.AnimMontages.Num() > 0)
			{
				return WeaponAnimData.AnimMontages[FMath::RandRange(0, WeaponAnimData.AnimMontages.Num() - 1)];
			}
		}
	}
	
	return nullptr;
}

bool UReadyOrNotFunctionLibrary::IsTableMontagePlaying(APlayerCharacter* PlayerCharacter, const FString& AnimationName, const bool bIsCrouching)
{
	if (!PlayerCharacter)
		return false;
	
	UDataTable* DT = GetRoNData()->AnimationDataLookupTable;
	if (DT)
	{
		FAnimationDataTable* LookupRow = DT->FindRow<FAnimationDataTable>(*AnimationName, "Animation Lookup");
		if (LookupRow)
		{
			for (auto StanceData: LookupRow->AnimData)
			{
				FAnimWeaponData WeaponAnimData = bIsCrouching ? StanceData.Value.CrouchedAnimData : StanceData.Value.StandingAnimData;
				if (WeaponAnimData.AnimMontages.Contains(PlayerCharacter->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage()))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

UDataSingleton* UReadyOrNotFunctionLibrary::GetRoNData()
{
	if (!GEngine)
		return nullptr;

	return Cast<UDataSingleton>(GEngine->GameSingleton);
}

void UReadyOrNotFunctionLibrary::CopySupporterStringToClipboard(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
		return;

	UReadyOrNotGameInstance* GameInstance = World->GetGameInstance<UReadyOrNotGameInstance>();
	if (!GameInstance)
		return;

	APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController();
	if (!PlayerController || !PlayerController->PlayerState)
		return;
	
	FString UniqueNetId = PlayerController->PlayerState->GetUniqueId().ToString();
	FString OneTimeUseCode = GameInstance->GetDiscordOneTimeUseCode();

	FString Description = "[Paste this to Verification Bot on the NDA Supporter Discord (https://discord.gg/A75brBGmzh)"
						  " to become verified and gain access to all the channels]";
	
	FString SupporterCode = FString::Printf(TEXT("!steamverify %s %s %s"), *UniqueNetId, *OneTimeUseCode, *Description);
	FPlatformApplicationMisc::ClipboardCopy(*SupporterCode);
}

void UReadyOrNotFunctionLibrary::SetSafeZonePadding(USafeZoneSlot* SafeZoneSlot, const FMargin Padding)
{
	if (SafeZoneSlot)
	{
		SafeZoneSlot->Padding = Padding;
		SafeZoneSlot->SynchronizeProperties();
	}
}

bool UReadyOrNotFunctionLibrary::FulfillsAllPostProcessRequirements(UObject* Context, APlayerCharacter* OwningCharacter, AActor* DamageCauser, const TArray<TSubclassOf<UPostProcessRequirement>>& InRequirementClasses, const bool bForceSuccess)
{
	if (InRequirementClasses.Num() == 0 || bForceSuccess)
		return true;
	
	bool bFulfillsAllRequirements = true;
	for (TSubclassOf<UPostProcessRequirement> RequirementClass : InRequirementClasses)
	{
		if (RequirementClass)
		{
			UPostProcessRequirement* Requirement = CreatePostProcessRequirement(Context, RequirementClass);
			if (Requirement)
			{
				Requirement->Initialize(Cast<APlayerCharacter>(OwningCharacter), DamageCauser);

				if (!Requirement->EnablePostProcessEffect())
				{
					bFulfillsAllRequirements = false;
					Requirement->ConditionalBeginDestroy();
					break;
				}

				Requirement->ConditionalBeginDestroy();
			}
		}
	}

	return bFulfillsAllRequirements;	
}

UPostProcessRequirement* UReadyOrNotFunctionLibrary::CreatePostProcessRequirement(UObject* Outer, const TSubclassOf<UPostProcessRequirement> InRequirementClass)
{
	if (InRequirementClass)
		return NewObject<UPostProcessRequirement>(Outer, InRequirementClass.Get(), InRequirementClass->GetFName(), RF_NoFlags, InRequirementClass.GetDefaultObject(), true);

	return nullptr;
}

APlayerCharacter* UReadyOrNotFunctionLibrary::GetPlayerCharacterMutableDefaultObject(UClass* Class)
{
	return GetMutableDefault<APlayerCharacter>(Class);
}

UObject* UReadyOrNotFunctionLibrary::GetClassDefaultObject(UClass* Class)
{
	return Class ? Class->GetDefaultObject() : nullptr; 
}

void UReadyOrNotFunctionLibrary::PlayRandomFMODEventAtLocation(UObject* WorldContextObject, FVector Location, TArray<UFMODEvent*>& FMODEvents)
{
	FMODEvents.RemoveAll([](UFMODEvent* FMODEvent)
	{
	    return FMODEvent == nullptr;
	});
	
	if (FMODEvents.Num() == 0)
		return;

	UFMODBlueprintStatics::PlayEventAtLocation(WorldContextObject, FMODEvents[FMath::RandRange(0, FMODEvents.Num() - 1)], FTransform(Location), true);
}

void UReadyOrNotFunctionLibrary::PlayRandomFMODEvent_2D(UObject* WorldContextObject, TArray<UFMODEvent*>& FMODEvents)
{
	FMODEvents.RemoveAll([](UFMODEvent* FMODEvent)
	{
		return FMODEvent == nullptr;
	});
	
	if (FMODEvents.Num() == 0)
		return;

	UFMODBlueprintStatics::PlayEvent2D(WorldContextObject, FMODEvents[FMath::RandRange(0, FMODEvents.Num() - 1)], true);
}

bool UReadyOrNotFunctionLibrary::AnyTrue(TArray<bool>& BoolArray)
{
	for (int32 i = 0; i < BoolArray.Num(); i++)
	{
		if (BoolArray[i])
		{
			return true;
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::AnyFalse(TArray<bool>& BoolArray)
{
	for (int32 i = 0; i < BoolArray.Num(); i++)
	{
		if (!BoolArray[i])
		{
			return true;
		}
	}

	return false;
}

bool UReadyOrNotFunctionLibrary::AllTrue(TArray<bool>& BoolArray)
{
	for (int32 i = 0; i < BoolArray.Num(); i++)
	{
		if (!BoolArray[i])
		{
			return false;
		}
	}

	return true;
}

bool UReadyOrNotFunctionLibrary::AllFalse(TArray<bool>& BoolArray)
{
	for (int32 i = 0; i < BoolArray.Num(); i++)
	{
		if (BoolArray[i])
		{
			return false;
		}
	}

	return true;
}

TSubclassOf<ABaseItem> UReadyOrNotFunctionLibrary::FindItemClassInItemDataTable(const FName RowName)
{
	if (UDataTable* DT = GetRoNData()->ItemDataLookupTable)
	{
		if (FItemLookupTable* LookupRow = DT->FindRow<FItemLookupTable>(RowName, "Item Lookup"))
		{
			return LookupRow->BlueprintClass.LoadSynchronous();
		}
	}

	return nullptr;
}

float UReadyOrNotFunctionLibrary::FindNearestFloor_BP(AActor* InActor, const TArray<AActor*>& InActorsToIgnore, const TArray<UPrimitiveComponent*>& InComponentsToIgnore)
{
	return FindNearestFloor(InActor, InActorsToIgnore, InComponentsToIgnore);
}

float UReadyOrNotFunctionLibrary::FindNearestFloor(AActor* InActor, const TArray<AActor*>& InActorsToIgnore, const TArray<UPrimitiveComponent*>& InComponentsToIgnore)
{
	if (!InActor)
		return 0.0f;
	
	FHitResult HitResult;
	FVector Start = InActor->GetActorLocation();
	FVector End = InActor->GetActorLocation() - FVector(0.0f, 0.0f, 1000000.0f);
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(InActor);
	CollisionQueryParams.AddIgnoredActors(InActorsToIgnore);
	CollisionQueryParams.AddIgnoredComponents(InComponentsToIgnore);

	if (InActor->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, CollisionQueryParams))
	{
		return HitResult.Location.Z;
	}

	return InActor->GetActorLocation().Z;
}

bool UReadyOrNotFunctionLibrary::IsActorInsideSplineEnclosure(ASplineTrigger* InSplineTrigger, AActor* InActor)
{
	if (!InSplineTrigger || !InActor)
		return false;

	return InSplineTrigger->IsActorInsideSplineEnclosure(InActor);
}

bool UReadyOrNotFunctionLibrary::IsActorOutsideSplineEnclosure(ASplineTrigger* InSplineTrigger, AActor* InActor)
{
	if (!InSplineTrigger || !InActor)
		return false;

	return InSplineTrigger->IsActorOutsideSplineEnclosure(InActor);
}

FText UReadyOrNotFunctionLibrary::SwatCommandToText(const ESwatCommand SwatCommand)
{
	switch (SwatCommand)
	{
	case ESwatCommand::SC_None:
		return FText::FromStringTable("SwatCommandTable", "None");
		
		case ESwatCommand::SC_Roger:
		return FText::FromStringTable("SwatCommandTable",  "Roger");
		
		case ESwatCommand::SC_MoveTo:
		return FText::FromStringTable("SwatCommandTable",  "MoveTo");
			
		case ESwatCommand::SC_FallIn:
		return FText::FromStringTable("SwatCommandTable",  "FallIn");
			
		case ESwatCommand::SC_Cover:
		return FText::FromStringTable("SwatCommandTable",  "Cover");
			
		case ESwatCommand::SC_Hold:
		return FText::FromStringTable("SwatCommandTable",  "Hold");
			
		case ESwatCommand::SC_Resume:
		return FText::FromStringTable("SwatCommandTable",  "Resume");
			
		case ESwatCommand::SC_DeployFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "DeployFlashbang");
			
		case ESwatCommand::SC_DeployStinger:
		return FText::FromStringTable("SwatCommandTable",  "DeployStinger");
			
		case ESwatCommand::SC_DeployCSGas:
		return FText::FromStringTable("SwatCommandTable",  "DeployCSGas");
			
		case ESwatCommand::SC_DeployChemlight:
		return FText::FromStringTable("SwatCommandTable",  "DeployChemlight");
			
		case ESwatCommand::SC_DoCollectEvidence:
		return FText::FromStringTable("SwatCommandTable",  "SecureEvidence");
		
		case ESwatCommand::SC_DoArrestTarget:
		return FText::FromStringTable("SwatCommandTable",  "Restrain");
			
		case ESwatCommand::SC_DoReportTarget:
		return FText::FromStringTable("SwatCommandTable",  "Report");
			
		case ESwatCommand::SC_DisarmStandaloneTrap:
		return FText::FromStringTable("SwatCommandTable",  "DisarmTrap");
		
		case ESwatCommand::SC_KillMe:
		return FText::FromStringTable("SwatCommandTable",  "KillMe");
		
		case ESwatCommand::SC_StackUp:
		return FText::FromStringTable("SwatCommandTable",  "StackUp");
		
		case ESwatCommand::SC_StackUpLeft:
		return FText::FromStringTable("SwatCommandTable",  "StackUpLeft");
		
		case ESwatCommand::SC_StackUpRight:
		return FText::FromStringTable("SwatCommandTable",  "StackUpRight");
		
		case ESwatCommand::SC_StackUpSplit:
		return FText::FromStringTable("SwatCommandTable",  "StackUpSplit");
			
		case ESwatCommand::SC_PickLock:
		return FText::FromStringTable("SwatCommandTable",  "PickLock");
			
		case ESwatCommand::SC_RemoveDoorJam:
		return FText::FromStringTable("SwatCommandTable",  "RemoveWedge");
			
		case ESwatCommand::SC_DeployMirrorgun:
		return FText::FromStringTable("SwatCommandTable",  "MirrorUnderDoor");
			
		case ESwatCommand::SC_DeployDoorJam:
		return FText::FromStringTable("SwatCommandTable",  "WedgeDoor");
			
		case ESwatCommand::SC_CheckForTrap:
		return FText::FromStringTable("SwatCommandTable",  "MirrorForTraps");
			
		case ESwatCommand::SC_DisarmTrap:
		return FText::FromStringTable("SwatCommandTable",  "DisarmTrap");
			
		case ESwatCommand::SC_OpenDoor:
		return FText::FromStringTable("SwatCommandTable",  "OpenDoor");
			
		case ESwatCommand::SC_CloseDoor:
		return FText::FromStringTable("SwatCommandTable",  "CloseDoor");

		case ESwatCommand::SC_MoveAndClear:
		return FText::FromStringTable("SwatCommandTable",  "MoveAndClear");
			
		case ESwatCommand::SC_MoveAndClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "MoveFlashbangAndClear");
			
		case ESwatCommand::SC_MoveAndClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "MoveStingAndClear");
			
		case ESwatCommand::SC_MoveAndClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "MoveGasAndClear");
		
		case ESwatCommand::SC_MoveAndClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "MoveLauncherAndClear");
		
		case ESwatCommand::SC_MoveAndClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "MoveLeaderAndClear");
			
		case ESwatCommand::SC_OpenAndClear:
		return FText::FromStringTable("SwatCommandTable",  "OpenAndClear");
			
		case ESwatCommand::SC_OpenAndClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "OpenFlashbangAndClear");
			
		case ESwatCommand::SC_OpenAndClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "OpenStingAndClear");
			
		case ESwatCommand::SC_OpenAndClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "OpenGasAndClear");
		
		case ESwatCommand::SC_OpenAndClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "OpenLauncherAndClear");
		
		case ESwatCommand::SC_OpenAndClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "OpenLeaderAndClear");
			
		case ESwatCommand::SC_KickAndClear:
		return FText::FromStringTable("SwatCommandTable",  "KickAndClear");
			
		case ESwatCommand::SC_KickAndClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "KickFlashbangAndClear");
			
		case ESwatCommand::SC_KickAndClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "KickStingAndClear");
			
		case ESwatCommand::SC_KickAndClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "KickGasAndClear");
		
		case ESwatCommand::SC_KickAndClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "KickLauncherAndClear");
		
		case ESwatCommand::SC_KickAndClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "KickLeaderAndClear");
			
		case ESwatCommand::SC_ShotgunClear:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunAndClear");
			
		case ESwatCommand::SC_ShotgunClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunFlashbangAndClear");
			
		case ESwatCommand::SC_ShotgunClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunStingAndClear");
			
		case ESwatCommand::SC_ShotgunClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunGasAndClear");
		
		case ESwatCommand::SC_ShotgunClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunLauncherAndClear");
		
		case ESwatCommand::SC_ShotgunClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "ShotgunLeaderAndClear");
			
		case ESwatCommand::SC_C2Clear:
		return FText::FromStringTable("SwatCommandTable",  "C2AndClear");
			
		case ESwatCommand::SC_C2ClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "C2FlashbangAndClear");
			
		case ESwatCommand::SC_C2ClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "C2StingAndClear");
			
		case ESwatCommand::SC_C2ClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "C2GasAndClear");
		
		case ESwatCommand::SC_C2ClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "C2LauncherAndClear");
		
		case ESwatCommand::SC_C2ClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "C2LeaderAndClear");
		
		case ESwatCommand::SC_LeaderAndClear:
		return FText::FromStringTable("SwatCommandTable",  "LeaderAndClear");
			
		case ESwatCommand::SC_LeaderAndClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "LeaderFlashbangAndClear");
			
		case ESwatCommand::SC_LeaderAndClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "LeaderStingAndClear");
			
		case ESwatCommand::SC_LeaderAndClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "LeaderGasAndClear");
		
		case ESwatCommand::SC_LeaderAndClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "LeaderLauncherAndClear");
		
		case ESwatCommand::SC_LeaderAndClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "LeaderLeaderAndClear");
		
		case ESwatCommand::SC_Execute:
		return FText::FromStringTable("SwatCommandTable",  "Execute");
			
		case ESwatCommand::SC_Cancel:
		return FText::FromStringTable("SwatCommandTable",  "Cancel");

		case ESwatCommand::SC_FallIn_Snake:
		return FText::FromStringTable("SwatCommandTable",  "FallInSingleFile");
		
		case ESwatCommand::SC_FallIn_HalfSnake:
		return FText::FromStringTable("SwatCommandTable",  "FallInDoubleFile");
		
		case ESwatCommand::SC_FallIn_Diamond:
		return FText::FromStringTable("SwatCommandTable",  "FallInDiamond");
		
		case ESwatCommand::SC_FallIn_Flock:
		return FText::FromStringTable("SwatCommandTable",  "FallInWedge");
		
		case ESwatCommand::SC_Slide:
		return FText::FromStringTable("SwatCommandTable",  "Slide");
		
		case ESwatCommand::SC_Slice:
		return FText::FromStringTable("SwatCommandTable",  "PIE");
		
		case ESwatCommand::SC_Snap:
		return FText::FromStringTable("SwatCommandTable",  "Peek");
		
		case ESwatCommand::SC_SearchAndSecure:
		return FText::FromStringTable("SwatCommandTable",  "SearchAndSecure");
		
		case ESwatCommand::SC_SearchAndSecureRoom:
		return FText::FromStringTable("SwatCommandTable",  "SearchRoom");
		
		case ESwatCommand::SC_RamAndClear:
		return FText::FromStringTable("SwatCommandTable",  "RamAndClear");
		
		case ESwatCommand::SC_RamAndClearFlashbang:
		return FText::FromStringTable("SwatCommandTable",  "RamFlashbangAndClear");
		
		case ESwatCommand::SC_RamAndClearStinger:
		return FText::FromStringTable("SwatCommandTable",  "RamStingAndClear");
		
		case ESwatCommand::SC_RamAndClearCSGas:
		return FText::FromStringTable("SwatCommandTable",  "RamGasAndClear");
		
		case ESwatCommand::SC_RamAndClearLauncher:
		return FText::FromStringTable("SwatCommandTable",  "RamLauncherAndClear");
		
		case ESwatCommand::SC_RamAndClearLeader:
		return FText::FromStringTable("SwatCommandTable",  "RamLeaderAndClear");
		
		case ESwatCommand::SC_SwapWithAlpha:
		return FText::FromStringTable("SwatCommandTable",  "SwapWith Alpha");
		
		case ESwatCommand::SC_SwapWithBeta:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithBravo");
		
		case ESwatCommand::SC_SwapWithCharlie:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithCharlie");
		
		case ESwatCommand::SC_SwapWithDelta:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithDelta");
		
		case ESwatCommand::SC_SwapWithAlphaOpposite:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithAlpha");
		
		case ESwatCommand::SC_SwapWithBetaOpposite:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithBravo");
		
		case ESwatCommand::SC_SwapWithCharlieOpposite:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithCharlie");
			
		case ESwatCommand::SC_SwapWithDeltaOpposite:
		return FText::FromStringTable("SwatCommandTable",  "SwapWithDelta");
		
		case ESwatCommand::SC_MoveToAlpha:
		return FText::FromStringTable("SwatCommandTable",  "MoveToAlpha");
		
		case ESwatCommand::SC_MoveToBeta:
		return FText::FromStringTable("SwatCommandTable",  "MoveToAlpha");
		
		case ESwatCommand::SC_MoveToCharlie:
		return FText::FromStringTable("SwatCommandTable",  "MoveToCharlie");
			
		case ESwatCommand::SC_MoveToDelta:
		return FText::FromStringTable("SwatCommandTable",  "MoveToDelta");
		
		case ESwatCommand::SC_MoveTo_Individual:
		return FText::FromStringTable("SwatCommandTable",  "MoveHere");
		
		case ESwatCommand::SC_MoveTo_MyPosition_Individual:
		return FText::FromStringTable("SwatCommandTable",  "MyPosition");
		
		case ESwatCommand::SC_MoveTo_Stop_Individual:
		return FText::FromStringTable("SwatCommandTable",  "Stop");
		
		case ESwatCommand::SC_MoveTo_Exit_Individual:
		return FText::FromStringTable("SwatCommandTable",  "MoveToExit");
		
		case ESwatCommand::SC_TurnAround_Individual:
		return FText::FromStringTable("SwatCommandTable",  "TurnAround");
		
		case ESwatCommand::SC_MoveToAndBack_Individual:
		return FText::FromStringTable("SwatCommandTable",  "HerethenBack");
		
		case ESwatCommand::SC_Focus_Individual:
		return FText::FromStringTable("SwatCommandTable",  "FocusHere");
		
		case ESwatCommand::SC_FocusDoor_Individual:
		return FText::FromStringTable("SwatCommandTable",  "FocusDoor");
		
		case ESwatCommand::SC_FocusTarget_Individual:
		return FText::FromStringTable("SwatCommandTable",  "FocusTarget");
		
		case ESwatCommand::SC_DeployShield:
		return FText::FromStringTable("SwatCommandTable",  "DeployShield");
		
		case ESwatCommand::PC_Deploy:
		return FText::FromStringTable("SwatCommandTable",  "Deploy");
		
		case ESwatCommand::PC_ConfirmOrderRequest:
		return FText::FromStringTable("SwatCommandTable",  "ConfirmOrderRequest");
		
		case ESwatCommand::PC_StackUp:
		return FText::FromStringTable("SwatCommandTable",  "StackUp");

		case ESwatCommand::PC_Open:
		return FText::FromStringTable("SwatCommandTable",  "Open");

		case ESwatCommand::PC_Move:
		return FText::FromStringTable("SwatCommandTable",  "Move");

		case ESwatCommand::PC_Kick:
		return FText::FromStringTable("SwatCommandTable",  "Kick");
		
		case ESwatCommand::PC_Shotgun:
		return FText::FromStringTable("SwatCommandTable",  "Shotgun");
		
		case ESwatCommand::PC_C2:
		return FText::FromStringTable("SwatCommandTable",  "C2");
		
		case ESwatCommand::PC_Breach:
		return FText::FromStringTable("SwatCommandTable",  "Breach");

		case ESwatCommand::SC_DeployTaser:
		return FText::FromStringTable("SwatCommandTable",   "DeployTaser");
		
		case ESwatCommand::SC_DeployPepperspray:
		return FText::FromStringTable("SwatCommandTable",   "DeployPepperspray");
		
		case ESwatCommand::SC_DeployPepperball:
		return FText::FromStringTable("SwatCommandTable",   "DeployPepperball");
		
		case ESwatCommand::SC_DeployBeanbag:
		return FText::FromStringTable("SwatCommandTable",   "DeployBeanbag");
		
		case ESwatCommand::SC_MeleeTarget:
		return FText::FromStringTable("SwatCommandTable",   "MeleeTarget");

		case ESwatCommand::SC_SearchAndSecureRoom_Individual:
		return FText::FromStringTable("SwatCommandTable",  "SearchRoom");
		
		case ESwatCommand::SC_Focus_MyPosition_Individual:
		return FText::FromStringTable("SwatCommandTable",  "FocusMyPosition");
		
		case ESwatCommand::SC_UnFocus_Individual:
		return FText::FromStringTable("SwatCommandTable",  "Unfocus");
		
		default:
		return FText::FromString("Ruh-Roh");
	}
}

FString UReadyOrNotFunctionLibrary::DoorBreachTypeToVoiceline(const EDoorBreachType DoorBreachType)
{
	switch (DoorBreachType)
	{
		case EDoorBreachType::None:		return "";
		case EDoorBreachType::Open:		return VO_SWAT_GENERAL::RESPONSE_OPEN_CLEAR_DOOR;
		case EDoorBreachType::Move:		return VO_SWAT_GENERAL::RESPONSE_MOVE_TO;
		case EDoorBreachType::Kick:		return VO_SWAT_GENERAL::RESPONSE_BREACH_KICK;
		case EDoorBreachType::Shotgun:	return VO_SWAT_GENERAL::RESPONSE_BREACH_SHOTGUN;
		case EDoorBreachType::Ram:		return VO_SWAT_GENERAL::RESPONSE_OPEN_CLEAR_DOOR;
		case EDoorBreachType::C2:		return VO_SWAT_GENERAL::RESPONSE_BREACH_C2;
		case EDoorBreachType::Leader:	return VO_SWAT_GENERAL::RESPONSE_OPEN_CLEAR_DOOR;
		default:						return "";
	}
}

FString UReadyOrNotFunctionLibrary::DoorBreachTypeToVoiceline_Negative(const EDoorBreachType DoorBreachType)
{
	switch (DoorBreachType)
	{
		case EDoorBreachType::None:		return "";
		case EDoorBreachType::Open:		return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		case EDoorBreachType::Move:		return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		case EDoorBreachType::Kick:		return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		case EDoorBreachType::Shotgun:	return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_SHOTGUN;
		case EDoorBreachType::Ram:		return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		case EDoorBreachType::C2:		return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_C2;
		case EDoorBreachType::Leader:	return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		case EDoorBreachType::Custom:	return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
		default:						return VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC;
	}
}

FString UReadyOrNotFunctionLibrary::DoorCheckResultToVoiceline(const EDoorCheckResult DoorBreachType)
{
	switch (DoorBreachType)
	{
		case EDoorCheckResult::None:		return "";
		case EDoorCheckResult::Unlocked:	return VO_SWAT_GENERAL::CALL_DOOR_UNLOCKED;
		case EDoorCheckResult::Locked:		return VO_SWAT_GENERAL::CALL_DOOR_LOCKED;
		case EDoorCheckResult::Jammed:		return VO_SWAT_GENERAL::CALL_DOOR_JAMMED;
		case EDoorCheckResult::Blocked:		return VO_SWAT_GENERAL::CALL_DOOR_BLOCKED;
		default:							return "";
	}
}

FString UReadyOrNotFunctionLibrary::SimulateAnimatedText(FString& FinalString, int32& Iterator, TArray<FString>& Chars, float& ElapsedTime, float& CurrentDelay, const float DelayBetweenLetters, const float DelayBetweenWords, const float DeltaTime, bool& bCompleted)
{
	if (Chars.Num() == 0)
		return FinalString;

	if (Iterator >= Chars.Num())
	{
		bCompleted = true;
	}
	else
	{
		ElapsedTime += DeltaTime;
		if (ElapsedTime > CurrentDelay)
		{
			ElapsedTime = 0.0f;

			FinalString += Chars[Iterator];

			const bool bIsSpace = Chars[Iterator] == " ";

			if (bIsSpace)
			{
				CurrentDelay = DelayBetweenWords;
			}
			else
			{
				CurrentDelay = DelayBetweenLetters;
			}

			Iterator++;
		}

		bCompleted = false;
	}

	return FinalString;
}

UDecalComponent* UReadyOrNotFunctionLibrary::CreateDecalComponent(UObject* Owner, UMaterialInterface* DecalMaterial, FVector DecalSize)
{
	if (!Owner)
		return nullptr;

	if (!Owner->GetWorld())
		return nullptr;
	
	UDecalComponent* DecalComp = NewObject<UDecalComponent>(Owner);
	DecalComp->bAllowAnyoneToDestroyMe = false;
	DecalComp->SetDecalMaterial(DecalMaterial);
	DecalComp->DecalSize = DecalSize;
	DecalComp->SetUsingAbsoluteScale(true);
	DecalComp->RegisterComponentWithWorld(Owner->GetWorld());
	DecalComp->SetVisibility(false);
	DecalComp->Deactivate();

	return DecalComp;
}

void UReadyOrNotFunctionLibrary::SetDecalSize(UDecalComponent* InDecalComponent, FVector DecalSize)
{
	if (!InDecalComponent)
		return;

	InDecalComponent->DecalSize = DecalSize;
	InDecalComponent->RecreateRenderState_Concurrent();
}

void UReadyOrNotFunctionLibrary::RemoveFromParentAndClear(TArray<UWidget*>& InWidgets)
{
	InWidgets.RemoveAll([](UWidget* Widget)
	{
		return Widget == nullptr;
	});

	for (UWidget* Widget : InWidgets)
	{
		if (Widget)
			Widget->RemoveFromParent();
	}

	InWidgets.Empty();
}

void UReadyOrNotFunctionLibrary::RemoveAllNullElements_Object(TArray<UObject*>& Array)
{
	Array.RemoveAll([](UObject* Object)
	{
		return Object == nullptr;
	});
}

void UReadyOrNotFunctionLibrary::RemoveAllNullElements_BP(const TArray<TSubclassOf<UClass>>& Array)
{
	// Should not be called from C++. Only blueprint
	check(0);
}

void UReadyOrNotFunctionLibrary::RemoveAllNullElements(void* TargetArray, const FArrayProperty* ArrayProp)
{
	if (TargetArray)
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, TargetArray);
		if (const FObjectProperty* ObjectProperty = CastField<const FObjectProperty>(ArrayProp->Inner))
		{
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				UObject* Object = ObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(i));
				if (!Object)
				{
					ArrayHelper.RemoveValues(i);
				}
			}
		}
	}
}

AActor* UReadyOrNotFunctionLibrary::FindClosestActorFromLocation_Blueprint(const FVector& InTestLocation, const TArray<AActor*>& InActors)
{
	if (InActors.Num() == 0)
		return nullptr;

	if (InActors.Num() == 1)
		return InActors[0];
	
	float ClosestDistance = FLT_MAX;
	AActor* ClosestActor = nullptr;
	for (AActor* Actor : InActors)
	{
		if (Actor)
		{
			const float CurrentDistance = FVector::Distance(InTestLocation, Actor->GetActorLocation());
			if (CurrentDistance < ClosestDistance)
			{
				ClosestDistance = CurrentDistance;
				ClosestActor = Actor;
			}
		}
	}

	return ClosestActor;
}

void UReadyOrNotFunctionLibrary::RemoveAllNullElements_Class(TArray<TSubclassOf<UClass>>& Array)
{
	Array.RemoveAll([](const TSubclassOf<UClass> Class)
	{
	    return Class == nullptr;
	});
}

void UReadyOrNotFunctionLibrary::PauseFMOD(const bool bPaused)
{
	for (TObjectIterator<UFMODAudioComponent>It; It; ++It)
	{
		if (!It->IsTemplate())
			It->SetPaused(bPaused);
	}

	for (TObjectIterator<UFMODEvent>It; It; ++It)
	{
		const TArray<FFMODEventInstance>& Instances = UFMODBlueprintStatics::FindEventInstances(nullptr, *It);
		for (const FFMODEventInstance& Instance : Instances)
			UFMODBlueprintStatics::EventInstanceSetPaused(Instance, bPaused);
	}

	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBlueprintStatics::BusSetPaused(*It, bPaused);
	}
}

void UReadyOrNotFunctionLibrary::MuteFMOD(const bool bMuted)
{
	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBlueprintStatics::BusSetMute(*It, bMuted);
	}
}

FString UReadyOrNotFunctionLibrary::DevMenuSettingsConfigDir()
{
	return FPaths::ProjectConfigDir() + "DevMenuCommands.ini";
}

FString UReadyOrNotFunctionLibrary::BadAIActionConfigDir()
{
	return FPaths::ProjectConfigDir() + "BadAIActions.ini";
}

FString UReadyOrNotFunctionLibrary::GetServerNameFromCurrentSession()
{
	if (IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr SessionInt = Online::GetSessionInterface())
		{
#if defined(WITH_STEAM)
			ISteamGameServer* SteamGameServerPtr = SteamGameServer();
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
			if (Session && SteamGameServerPtr)
			{
				FString ServerName;
				Session->SessionSettings.Get<FString>("ServerName", ServerName);

				return ServerName;
			}
#endif
		}
	}

	return "Unknown";
}

TArray<FString> UReadyOrNotFunctionLibrary::GetAllSectionNamesFromINIFile(const FString ConfigFilePath)
{
	TArray<FString> SectionNames;
	GConfig->GetSectionNames(ConfigFilePath, SectionNames);
	
	return SectionNames;
}

TArray<FString> UReadyOrNotFunctionLibrary::GetSingleLineArrayFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	TArray<FString> StringArray;
	GConfig->GetSingleLineArray(*Section, *Key, StringArray, ConfigFilePath);

	TArray<FString> ResultingStrings;

	for (int32 i = 0; i < StringArray.Num(); i++)
	{
		FString String = StringArray[i];

		// We've reached the comment, stop
		if (String.Contains(";"))
			break;

		String.RemoveFromEnd(TEXT(","));
		ResultingStrings.Add(String);
	}

	return ResultingStrings;
}

int32 UReadyOrNotFunctionLibrary::GetIntegerFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	int32 ConfigInt = 0;
	GConfig->GetInt(*Section, *Key, ConfigInt, ConfigFilePath);
	
	return ConfigInt;
}

float UReadyOrNotFunctionLibrary::GetFloatFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	float ConfigFloat = 0.0f;
	GConfig->GetFloat(*Section, *Key, ConfigFloat, ConfigFilePath);

	return ConfigFloat;
}

bool UReadyOrNotFunctionLibrary::GetBoolFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	bool ConfigBool = false;
	GConfig->GetBool(*Section, *Key, ConfigBool, ConfigFilePath);

	return ConfigBool;
}

FVector UReadyOrNotFunctionLibrary::GetVectorFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	FVector ConfigVector = FVector::ZeroVector;
	GConfig->GetVector(*Section, *Key, ConfigVector, ConfigFilePath);

	return ConfigVector;
}

FVector2D UReadyOrNotFunctionLibrary::GetVector2DFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	FVector2D ConfigVector = FVector2D::ZeroVector;
	GConfig->GetVector2D(*Section, *Key, ConfigVector, ConfigFilePath);

	return ConfigVector;
}

FString UReadyOrNotFunctionLibrary::GetStringFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	FString ConfigString = "";
	GConfig->GetString(*Section, *Key, ConfigString, ConfigFilePath);
	
	return ConfigString;
}

TArray<FString> UReadyOrNotFunctionLibrary::GetStringArrayFromINIFile(FString ConfigFilePath, FString Section, FString Key)
{
	TArray<FString> ConfigStringArray;
	GConfig->GetArray(*Section, *Key, ConfigStringArray, ConfigFilePath);

	for (int32 i = 0; i < ConfigStringArray.Num(); i++)
	{
		ConfigStringArray[i].RemoveFromEnd(TEXT(","));
	}

	return ConfigStringArray;
}

bool UReadyOrNotFunctionLibrary::FindConfigKeyFromINIFile(const FString ConfigFilePath, const FString Section, const FString Key)
{
	// ##UE5UPGRADE##
	FConfigFile* File = GConfig->Find(ConfigFilePath);
	if( !File )
	{
		return false;
	}
	FConfigSection* Sec = File->Find( Section );
	if( !Sec )
	{
		return false;
	}
	
	return Sec->Find(*Key) != nullptr;
}

FVector2D UReadyOrNotFunctionLibrary::GetWidgetSize_Absolute(UWidget* InWidget)
{
	if (InWidget)
	{
		return InWidget->GetCachedGeometry().GetAbsoluteSize();
	}

	return FVector2D::ZeroVector;
}

FVector2D UReadyOrNotFunctionLibrary::GetWidgetSize_Local(UWidget* InWidget)
{
	if (InWidget)
	{
		return InWidget->GetCachedGeometry().GetLocalSize();
	}

	return FVector2D::ZeroVector;
}

FVector2D UReadyOrNotFunctionLibrary::GetViewportPositionOfWidget(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget, const FVector2D InCoordinates)
{
	if (InWidget)
	{
		FVector2D ViewportPosition;
		FVector2D PixelPosition;
		USlateBlueprintLibrary::LocalToViewport(WorldContext, InParentWidget->GetTickSpaceGeometry(), InWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(InCoordinates), PixelPosition, ViewportPosition);
		
		return ViewportPosition;
	}

	return FVector2D::ZeroVector;
}

FVector2D UReadyOrNotFunctionLibrary::GetPixelPositionOfWidget(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget, const FVector2D InCoordinates)
{
	if (InWidget)
	{
		FVector2D ViewportPosition;
		FVector2D PixelPosition;
		USlateBlueprintLibrary::LocalToViewport(WorldContext, InParentWidget->GetTickSpaceGeometry(), InWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(InCoordinates), PixelPosition, ViewportPosition);
		
		return PixelPosition;
	}

	return FVector2D::ZeroVector;
}

FVector2D UReadyOrNotFunctionLibrary::GetViewportPositionOfWidgetCenter(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget)
{
	return GetViewportPositionOfWidget(WorldContext, InParentWidget, InWidget, {0.5f, 0.5f});
}

FVector2D UReadyOrNotFunctionLibrary::GetPixelPositionOfWidgetCenter(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget)
{
	return GetPixelPositionOfWidget(WorldContext, InParentWidget, InWidget, {0.5f, 0.5f});
}

FVector2D UReadyOrNotFunctionLibrary::CalculateOffscreenPositionFromWorldLocation_Ellipse(UObject* WorldContext, const FVector& WorldLocation, const float ViewportOffset, bool& bIsOffscreen, float& Angle, float& ForwardDotProduct, float& RightDotProduct)
{
	if (!GEngine->GameViewport)
		return FVector2D::ZeroVector;

	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
	const FVector2D ViewportCenter = FVector2D(ViewportSize.X/2, ViewportSize.Y/2);
	const FVector2D ScreenBounds = ViewportSize - FMath::Clamp(ViewportOffset, 0.0f, ViewportSize.X);
	
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContext, 0))
	{
		FVector2D ScreenPosition;
		const bool bSuccess = UGameplayStatics::ProjectWorldToScreen(PlayerController, WorldLocation, ScreenPosition, true);

		if (bSuccess &&
            ScreenPosition.X >= 0.0f && ScreenPosition.X <= ViewportSize.X &&
            ScreenPosition.Y >= 0.0f && ScreenPosition.Y <= ViewportSize.Y)
		{
			bIsOffscreen = false;

			return ScreenPosition;
		}
		
		bIsOffscreen = true;

		FVector CameraLocation;
		FRotator CameraRotation;

		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);
		}

		const FVector DirectionToMarker = WorldLocation - CameraLocation;
		FVector4 DirectionToMarker_ScreenSpace = DirectionToMarker;
		
		ULocalPlayer* const LP = PlayerController->GetLocalPlayer();
		if (LP && LP->ViewportClient)
		{
			// Get the projection data
			FSceneViewProjectionData ProjectionData;
			if (LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData, 0))
			{
				const FMatrix ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();

				DirectionToMarker_ScreenSpace = ViewProjectionMatrix.TransformVector(DirectionToMarker).GetSafeNormal();
				
				if (DirectionToMarker_ScreenSpace.W > 0)
					DirectionToMarker_ScreenSpace = DirectionToMarker_ScreenSpace / DirectionToMarker_ScreenSpace.W;
            }
		}

		if (APawn* Pawn = PlayerController->GetPawn())
		{
			ForwardDotProduct = FVector::DotProduct(Pawn->GetActorForwardVector(), DirectionToMarker.GetSafeNormal());
			RightDotProduct = FVector::DotProduct(Pawn->GetActorRightVector(), DirectionToMarker.GetSafeNormal());
			//DrawDebugLine(Pawn->GetWorld(), CameraLocation + CameraRotation.Vector() * 50.0f, WorldLocation, FColor::Red, false, -1, 0, 2.0f);
		}
		
		Angle = 360.0f-DirectionToMarker_ScreenSpace.Rotation().Yaw;

		const float ForwardDirection = DirectionToMarker_ScreenSpace.Y;
		const float RightDirection = DirectionToMarker_ScreenSpace.X;
	
		// if forward is positive and right is negative, we are in quadrant 1
		if (ForwardDirection > 0.0f && RightDirection < 0.0f)
		{
			const float AngleRadians = 3*PI/2-FMath::Acos(ForwardDirection);

			return FVector2D(ViewportCenter.X + ScreenBounds.X/2 * FMath::Cos(AngleRadians), ViewportCenter.Y + ScreenBounds.Y/2 * FMath::Sin(AngleRadians));
		}
		
		// if forward is positive and right is positive, we are in quadrant 2
		if (ForwardDirection > 0.0f && RightDirection > 0.0f)
		{
			const float AngleRadians = 3*PI/2+FMath::Acos(ForwardDirection);

			return FVector2D(ViewportCenter.X + ScreenBounds.X/2 * FMath::Cos(AngleRadians), ViewportCenter.Y + ScreenBounds.Y/2 * FMath::Sin(AngleRadians));
		}
		
		// if forward is negative and right is negative, we are in quadrant 3
		if (ForwardDirection < 0.0f && RightDirection < 0.0f)
		{
			const float AngleRadians = 1.5*PI-FMath::Acos(ForwardDirection);

			return FVector2D(ViewportCenter.X + ScreenBounds.X/2 * FMath::Cos(AngleRadians), ViewportCenter.Y + ScreenBounds.Y/2 * FMath::Sin(AngleRadians));
		}
		
		// if forward is negative and right is positive, we are in quadrant 4
		if (ForwardDirection < 0.0f && RightDirection > 0.0f)
		{
			const float AngleRadians = 1.5*PI+FMath::Acos(ForwardDirection);

			return FVector2D(ViewportCenter.X + ScreenBounds.X/2 * FMath::Cos(AngleRadians), ViewportCenter.Y + ScreenBounds.Y/2 * FMath::Sin(AngleRadians));
		}

		return ScreenPosition;
	}

	return FVector2D::ZeroVector;
}

FVector2D UReadyOrNotFunctionLibrary::CalculateOffscreenPositionFromWorldLocation_Square(UObject* WorldContext, const FVector& WorldLocation, const float ViewportOffset, bool& bIsOffscreen, float& Angle, float& ForwardDotProduct, float& RightDotProduct)
{
	if (!GEngine->GameViewport || !GEngine->GameViewport->Viewport)
		return FVector2D::ZeroVector;

	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
	
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContext, 0))
	{
		FVector2D ScreenPosition;
		const bool bSuccess = UGameplayStatics::ProjectWorldToScreen(PlayerController, WorldLocation, ScreenPosition, true);

		if (bSuccess &&
            ScreenPosition.X >= 0.0f && ScreenPosition.X <= ViewportSize.X &&
            ScreenPosition.Y >= 0.0f && ScreenPosition.Y <= ViewportSize.Y)
		{
			bIsOffscreen = false;

			return ScreenPosition;
		}
		
		bIsOffscreen = true;

		FVector CameraLocation;
		FRotator CameraRotation;

		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);
		}

		const FVector DirectionToMarker = WorldLocation - CameraLocation;
		FVector4 DirectionToMarker_ScreenSpace = DirectionToMarker;
		
		ULocalPlayer* const LP = PlayerController->GetLocalPlayer();
		if (LP && LP->ViewportClient)
		{
			// Get the projection data
			FSceneViewProjectionData ProjectionData;
			if (LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData, 0))
			{
				const FMatrix ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();

				DirectionToMarker_ScreenSpace = ViewProjectionMatrix.TransformVector(DirectionToMarker).GetSafeNormal();
				
				if (DirectionToMarker_ScreenSpace.W > 0)
					DirectionToMarker_ScreenSpace = DirectionToMarker_ScreenSpace / DirectionToMarker_ScreenSpace.W;
            }
		}

		if (APawn* Pawn = PlayerController->GetPawn())
		{
			ForwardDotProduct = FVector::DotProduct(Pawn->GetActorForwardVector(), DirectionToMarker.GetSafeNormal());
			RightDotProduct = FVector::DotProduct(Pawn->GetActorRightVector(), DirectionToMarker.GetSafeNormal());
			//DrawDebugLine(Pawn->GetWorld(), CameraLocation + CameraRotation.Vector() * 50.0f, WorldLocation, FColor::Red, false, -1, 0, 2.0f);
		}

		Angle = 360.0f-DirectionToMarker_ScreenSpace.Rotation().Yaw;
		
		const float ForwardDirection = DirectionToMarker_ScreenSpace.Y;
		const float RightDirection = DirectionToMarker_ScreenSpace.X;

		//ULog::Number(ForwardDirection, "ForwardDirection: ");
		//ULog::Number(RightDirection, "RightDirection: ");
		//ULog::Number(Abs, "Abs: ");
		
		const float Difference = FMath::Abs(ForwardDirection) - FMath::Abs(RightDirection);

		// if forward is positive and right is negative, we are in quadrant 1
		if (ForwardDirection > 0.0f && RightDirection < 0.0f)
		{
			const float X = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0 + ViewportOffset, ViewportSize.X/2), Difference);
			const float Y = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, -1.0f), FVector2D(0 + ViewportOffset, ViewportSize.Y/2), Difference);

			return FVector2D(X, Y);
		}
		
		// if forward is positive and right is positive, we are in quadrant 2
		if (ForwardDirection > 0.0f && RightDirection > 0.0f)
		{
			const float X = FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 0.0f), FVector2D(ViewportSize.X/2, ViewportSize.X - ViewportOffset), Difference);
			const float Y = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, -1.0f), FVector2D(0 + ViewportOffset, ViewportSize.Y/2), Difference);

			return FVector2D(X, Y);
		}
		
		// if forward is negative and right is negative, we are in quadrant 3
		if (ForwardDirection < 0.0f && RightDirection < 0.0f)
		{
			const float X = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0 + ViewportOffset, ViewportSize.X/2), Difference);
			const float Y = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 0.0f), FVector2D(ViewportSize.Y/2, ViewportSize.Y - ViewportOffset), Difference);

			return FVector2D(X, Y);
		}
		
		// if forward is negative and right is positive, we are in quadrant 4
		if (ForwardDirection < 0.0f && RightDirection > 0.0f)
		{
			const float X = FMath::GetMappedRangeValueClamped(FVector2D(1.0f, 0.0f), FVector2D(ViewportSize.X/2, ViewportSize.X - ViewportOffset), Difference);
			const float Y = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, -1.0f), FVector2D(ViewportSize.Y - ViewportOffset, ViewportSize.Y/2), Difference);

			return FVector2D(X, Y);
		}

		return ScreenPosition;
	}

	return FVector2D::ZeroVector;
}

bool UReadyOrNotFunctionLibrary::DoesWidgetOverlap(UObject* WorldContext, UWidget* ParentWidget, UWidget* WidgetA, UWidget* WidgetB)
{
	if (!WidgetA || !WidgetB || !ParentWidget)
		return false;

	if (!WorldContext)
		return false;
	
	if (!WorldContext->GetWorld())
		return false;

	if (!WidgetA->IsVisible() || WidgetA->GetRenderOpacity() <= 0.0f)
		return false;

	if (!WidgetB->IsVisible() || WidgetB->GetRenderOpacity() <= 0.0f)
		return false;
	
	const FVector2D WidgetA_Size = GetWidgetSize_Local(WidgetA);
	const FVector2D WidgetB_Size = GetWidgetSize_Local(WidgetB);

	const FVector2D WidgetA_Position = GetPixelPositionOfWidgetCenter(WorldContext, ParentWidget, WidgetA);
	const FVector2D WidgetB_Position = GetPixelPositionOfWidgetCenter(WorldContext, ParentWidget, WidgetB);

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(WorldContext);

	return  WidgetA_Position.X + (WidgetA_Size.X/2)*ViewportScale >= WidgetB_Position.X - (WidgetB_Size.X/2)*ViewportScale &&
			WidgetA_Position.X - (WidgetA_Size.X/2)*ViewportScale <= WidgetB_Position.X + (WidgetB_Size.X/2)*ViewportScale &&
			WidgetA_Position.Y + (WidgetA_Size.Y/2)*ViewportScale >= WidgetB_Position.Y - (WidgetB_Size.Y/2)*ViewportScale &&
			WidgetA_Position.Y - (WidgetA_Size.Y/2)*ViewportScale <= WidgetB_Position.Y + (WidgetB_Size.Y/2)*ViewportScale;
}

bool UReadyOrNotFunctionLibrary::DoesWidgetOverlap(UObject* WorldContext, UWidget* ParentWidget, const FVector2D& WidgetA_Position, const FVector2D& WidgetA_LocalSize, UWidget* WidgetB)
{
	if (!WorldContext)
		return false;
		
	if (!WorldContext->GetWorld())
		return false;
	
	if (!WidgetB || !ParentWidget)
		return false;

	if (!WidgetB->IsVisible() || WidgetB->GetRenderOpacity() <= 0.0f)
		return false;

	const FVector2D WidgetB_Size = GetWidgetSize_Local(WidgetB);

	const FVector2D WidgetB_Position = GetPixelPositionOfWidgetCenter(WorldContext, ParentWidget, WidgetB);

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(WorldContext);

	return  WidgetA_Position.X + (WidgetA_LocalSize.X/2)*ViewportScale >= WidgetB_Position.X - (WidgetB_Size.X/2)*ViewportScale &&
            WidgetA_Position.X - (WidgetA_LocalSize.X/2)*ViewportScale <= WidgetB_Position.X + (WidgetB_Size.X/2)*ViewportScale &&
            WidgetA_Position.Y + (WidgetA_LocalSize.Y/2)*ViewportScale >= WidgetB_Position.Y - (WidgetB_Size.Y/2)*ViewportScale &&
            WidgetA_Position.Y - (WidgetA_LocalSize.Y/2)*ViewportScale <= WidgetB_Position.Y + (WidgetB_Size.Y/2)*ViewportScale;
}

bool UReadyOrNotFunctionLibrary::IsUsingGamepad(AReadyOrNotPlayerController* InController)
{
	if (InController && InController->GetLocalPlayer())
	{
		if (const UCommonInputSubsystem* InputSubsystem = InController->GetLocalPlayer()->GetSubsystem<UCommonInputSubsystem>())
		{
			return InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;
		}
	}

	return false;
}

FText UReadyOrNotFunctionLibrary::FormatPlayerActionText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText, const FString& InColorLabel)
{
	const bool bIsValidColorLabel = !InColorLabel.IsEmpty() && (InColorLabel.Contains("Red") || InColorLabel.Contains("Blue") || InColorLabel.Contains("Yellow") ||
															InColorLabel.Contains("Orange") || InColorLabel.Contains("Black") || InColorLabel.Contains("Cyan") ||
															InColorLabel.Contains("White")|| InColorLabel.Contains("Grey") || InColorLabel.Contains("Green"));

	const FString FinalColorLabel = (bIsValidColorLabel ? InColorLabel : "Red");

	FString InputActionKey;

	switch (InInputEvent)
	{
		case IE_Pressed:		InputActionKey = "PressPrompt"; break;
		case IE_Released:		InputActionKey = "ReleasePrompt"; break;
		case IE_Repeat:			InputActionKey = "HoldPrompt"; break;
		case IE_DoubleClick:	InputActionKey = "DoubleTapPrompt"; break;
		case IE_Axis:			InputActionKey = "MovePrompt"; break;
		case IE_MAX:			InputActionKey = "InvalidPrompt"; break;
		default:				InputActionKey = "PressPrompt"; break;
	}

	FText InputActionText = FText::FromStringTable("ActionPromptTable", InputActionKey);

	FString FormattedInput;
	if (InKey.IsGamepadKey())
	{
		FString InKeyString = InKey.ToString();
		#if defined PLATFORM_PS5
		InKeyString += "_PS";
		#elif defined PLATFORM_PS4
		InKeyString += "_PS";
		#endif
		FormattedInput = "<img id=\"" + InKeyString + "\"/>";
	}
	else
	{
		// Convert unreal key name to our custom key name
		// Example: 'Left Mouse Button' becomes 'LMB'
		const FString KeyName = ConvertUnrealKeyNameToRonKeyName(InKey);
		FormattedInput = "<" + FinalColorLabel + ">" + (InKey.IsValid() ? KeyName : "Unbound") + "</>";
	}

	// Format: {InputType} <Red>{Key}</> to <Red>{Action}</>
	// Example Output: Press F to Interact
	FText FormattedText = FText::Format(InputActionText, FText::FromString(FormattedInput), FText::FromString(FinalColorLabel),InActionText);
	return FormattedText;
}

UHumanCharacterHUD_V2* UReadyOrNotFunctionLibrary::GetPlayerHUD(UObject* WorldContext)
{
	if (!WorldContext)
		return nullptr;
	
	if (APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(WorldContext, 0)))
	{
		return LocalPlayer->HumanCharacterWidget_V2;
	}

	return nullptr;
}

ERONBuildConfiguration UReadyOrNotFunctionLibrary::GetBuildConfiguration()
{
	#if WITH_EDITOR
		return ERONBuildConfiguration::Editor;
	#elif UE_BUILD_DEBUG
		return ERONBuildConfiguration::Debug;
	#elif UE_BUILD_DEVELOPMENT
		return ERONBuildConfiguration::Development;
	#elif UE_BUILD_TEST
		return ERONBuildConfiguration::Test;
	#elif UE_BUILD_SHIPPING
		return ERONBuildConfiguration::Shipping;
	#elif UE_BUILD_FINAL_RELEASE
		return ERONBuildConfiguration::FinalRelease;
	#else
		return ERONBuildConfiguration::Unknown;
	#endif
}

bool UReadyOrNotFunctionLibrary::IsBuildPirated()
{
	return UReadyOrNotGameInstance::bIsBuildPirated;
}

bool UReadyOrNotFunctionLibrary::IsAprilFools()
{
	const FDateTime Now = FDateTime::Now();
	return Now.GetDay() == 1 && Now.GetMonthOfYear() == EMonthOfYear::April;
}

bool UReadyOrNotFunctionLibrary::IsHalloween()
{
	const FDateTime Now = FDateTime::Now();
	return Now.GetDay() == 31 && Now.GetMonthOfYear() == EMonthOfYear::October;
}

bool UReadyOrNotFunctionLibrary::LoadStringArrayFromFile(TArray<FString>& StringArray, int32& ArraySize, FString FullFilePath, bool ExcludeEmptyLines)
{
	ArraySize = 0;
	
	if (FullFilePath == "" || FullFilePath == " ")
		return false;
	
	// empty any previous contents!
	StringArray.Empty();
	
	TArray<FString> FileArray;
	 
	if (!FFileHelper::LoadANSITextFileToStrings(*FullFilePath, NULL, FileArray))
	{
		return false;
	}

	if (ExcludeEmptyLines)
	{
		for (const FString& Each : FileArray)
		{
			if (Each == "")
				continue;
			
			// check for any non whitespace
			bool FoundNonWhiteSpace = false;
			for(int32 v = 0; v < Each.Len(); v++)
			{
				if(Each[v] != ' ' && Each[v] != '\n')
				{
					FoundNonWhiteSpace = true;
					break;
				}
			}
			
			if (FoundNonWhiteSpace)
			{
				StringArray.Add(Each);
			}
		}
	}
	else
	{
		StringArray.Append(FileArray);
	}
	
	ArraySize = StringArray.Num();
	return true; 
}

FName UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(UObject* Context)
{
	if (const UWorld* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::LogAndReturnNull))
	{
		FString LevelName = World->GetMapName();

		// Start at index 1 to skip the persistent level
		for (uint8 i = 1; i < World->GetStreamingLevels().Num(); ++i)
		{
			if (const ULevelStreaming* Stream = World->GetStreamingLevels()[i])
			{
				FString SubLevelName = Stream->GetWorldAssetPackageName();
				if (SubLevelName.EndsWith("_Core"))
				{
					LevelName = SubLevelName;
					break;
				}
			}
		}
		
		if (LevelName.IsEmpty())
		{
			LevelName = World->GetOutermost()->GetName();
		}

		// Just return the name of the map, not the rest of the path
		LevelName = FPackageName::GetLongPackageAssetName(LevelName);
		
		LevelName.RemoveFromStart(World->StreamingLevelsPrefix);
		
		return *LevelName;
	}
	
	return NAME_None;
}

TArray<USoundBase*> UReadyOrNotFunctionLibrary::GetAllSoundsInWorld()
{
	return GetObjectsOfClass<USoundBase>();
}

AReadyOrNotLevelScript* UReadyOrNotFunctionLibrary::GetReadyOrNotLevelScript(UObject* Context)
{
	if (!Context)
		return nullptr;
	
	return Cast<AReadyOrNotLevelScript>(Context->GetWorld()->GetLevelScriptActor(Context->GetWorld()->GetCurrentLevel()));
}

TArray<UAudioComponent*> UReadyOrNotFunctionLibrary::GetAllAudioComponents()
{
	return GetObjectsOfClass<UAudioComponent>();
}

TArray<ABaseItem*> UReadyOrNotFunctionLibrary::GetAllItemsInMemory()
{
	return GetObjectsOfClass<ABaseItem>();
}

TArray<UFMODEvent*> UReadyOrNotFunctionLibrary::GetAll2DFMODAudioEvents()
{
	TArray<UFMODEvent*> Sounds2D;
	for (TObjectIterator<UFMODEvent> It; It; ++It)
	{
		UFMODEvent* Event = *It;
		FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(Event, EFMODSystemContext::Max);
		bool bIs3D;
		if (EventDesc)
		{
			EventDesc->is3D(&bIs3D);

			if (!bIs3D)
			{
				Sounds2D.Add(*It);
			}
		}
	}

	return Sounds2D;
}

TArray<UFMODBus*> UReadyOrNotFunctionLibrary::GetAllFMODBusObjects()
{
	TArray<UFMODBus*> Buses;
	for (TObjectIterator<UFMODBus> It; It; ++It)
	{
		UFMODBus* Bus = *It;
		if (Bus)
		{
			Buses.Add(Bus);
		}
	}

	return Buses;
}

bool UReadyOrNotFunctionLibrary::IsFMODBusMuted(UFMODBus* Bus)
{
	if (!Bus)
		return false;

	bool bIsMuted = false;

	FMOD::Studio::System *StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (StudioSystem != nullptr && IsValid(Bus))
	{
		FMOD::Studio::ID guid = FMODUtils::ConvertGuid(Bus->AssetGuid);
		FMOD::Studio::Bus *bus = nullptr;
		const FMOD_RESULT Result = StudioSystem->getBusByID(&guid, &bus);
		if (Result == FMOD_OK && bus != nullptr)
		{
			bus->getMute(&bIsMuted);
		}
	}

	return bIsMuted;
}

bool UReadyOrNotFunctionLibrary::IsFMODBusPaused(UFMODBus* Bus)
{
	if (!Bus)
		return false;

	bool bIsPaused = false;

	FMOD::Studio::System *StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (StudioSystem != nullptr && IsValid(Bus))
	{
		FMOD::Studio::ID guid = FMODUtils::ConvertGuid(Bus->AssetGuid);
		FMOD::Studio::Bus *bus = nullptr;
		const FMOD_RESULT Result = StudioSystem->getBusByID(&guid, &bus);
		if (Result == FMOD_OK && bus != nullptr)
		{
			bus->getPaused(&bIsPaused);
		}
	}

	return bIsPaused;
}

float UReadyOrNotFunctionLibrary::GetFMODBusVolume(UFMODBus* Bus)
{
	if (!Bus)
		return false;

	float BusVolume = 0.0f;

	FMOD::Studio::System *StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (StudioSystem != nullptr && IsValid(Bus))
	{
		FMOD::Studio::ID guid = FMODUtils::ConvertGuid(Bus->AssetGuid);
		FMOD::Studio::Bus *bus = nullptr;
		const FMOD_RESULT Result = StudioSystem->getBusByID(&guid, &bus);
		if (Result == FMOD_OK && bus != nullptr)
		{
			bus->getVolume(&BusVolume);
		}
	}

	return BusVolume;
}

void UReadyOrNotFunctionLibrary::SetFMODVolume(const float Volume)
{
	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBlueprintStatics::BusSetVolume(*It, Volume);
	}	
}

UClass* UReadyOrNotFunctionLibrary::GetClassFromObject(UObject* Object)
{
	if (!Object)
		return nullptr;
	
	return Object->GetClass(); 
}

void UReadyOrNotFunctionLibrary::SetupPostProcessEffect(UObject* Context, FPostProcessEffect& InPostProcessEffect)
{
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		if (InPostProcessEffect.PostProcesses[i].PostProcess_Data)
			InPostProcessEffect.PostProcesses[i].PostProcess_MID = UMaterialInstanceDynamic::Create(InPostProcessEffect.PostProcesses[i].PostProcess_Data->PostProcess_Material, Context);
	}
}

void UReadyOrNotFunctionLibrary::StartPostProcessEffect(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffect& InPostProcessEffect, AActor* DamageCauser)
{
	if (InPostProcessEffect.bEnabled)
	{
		for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
		{
			StartPostProcessEffect_Specific(Context, PostProcessSettings, InPostProcessEffect.PostProcesses[i], DamageCauser);
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

void UReadyOrNotFunctionLibrary::StartPostProcessEffect_Specific(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffectPlayer& InPostProcessEffectPlayer, AActor* DamageCauser)
{
	if (InPostProcessEffectPlayer.bEnabled)
	{
		if (InPostProcessEffectPlayer.PostProcess_Data)
		{
			if (InPostProcessEffectPlayer.PostProcess_MID && FulfillsAllPostProcessRequirements(Context, nullptr, DamageCauser, InPostProcessEffectPlayer.RequirementsClasses))
			{
				if (InPostProcessEffectPlayer.bRestartIfAlreadyPlaying)
				{
					PostProcessSettings.RemoveBlendable(InPostProcessEffectPlayer.PostProcess_MID);
					InPostProcessEffectPlayer.Reset();

					#if WITH_EDITOR
					if (InPostProcessEffectPlayer.bDebug)
						ULog::Info(CUR_FUNC_2 + InPostProcessEffectPlayer.EffectName.ToString() + " removed");
					#endif
				}

				PostProcessSettings.AddBlendable(InPostProcessEffectPlayer.PostProcess_MID, 1.0f);

				InPostProcessEffectPlayer.Start(nullptr, DamageCauser);

				#if WITH_EDITOR
				if (InPostProcessEffectPlayer.bDebug)
					ULog::Info(CUR_FUNC_2 + InPostProcessEffectPlayer.PostProcess_Data->GetName() + ": started");
				#endif
			}
		}
	}
}

void UReadyOrNotFunctionLibrary::StopPostProcessEffect(FPostProcessSettings& PostProcessSettings, FPostProcessEffect& InPostProcessEffect)
{
	if (!InPostProcessEffect.bStarted)
		return;

	if (InPostProcessEffect.bEnabled)
	{
		for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
		{
			FPostProcessEffectPlayer& PostProcessEffectPlayer = InPostProcessEffect.PostProcesses[i];
			
			if (PostProcessEffectPlayer.bEnabled)
			{
				if (PostProcessEffectPlayer.PostProcess_MID)
				{
					PostProcessSettings.RemoveBlendable(PostProcessEffectPlayer.PostProcess_MID);
					PostProcessEffectPlayer.Reset();

					#if WITH_EDITOR
					if (PostProcessEffectPlayer.bDebug)
						ULog::Info(CUR_FUNC_2 + PostProcessEffectPlayer.PostProcess_Data->GetName() + ": removed");
					#endif
				}
			}
		}

		InPostProcessEffect.bStarted = false;
	}
}

bool UReadyOrNotFunctionLibrary::ProcessPostProcessEffect(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffect& InPostProcessEffect, const float DeltaTime)
{
	if (!InPostProcessEffect.bStarted || !InPostProcessEffect.bEnabled)
		return false;
	
	for (int32 i = 0; i < InPostProcessEffect.PostProcesses.Num(); i++)
	{
		FPostProcessEffectPlayer& CurrentPPEffectSetting = InPostProcessEffect.PostProcesses[i];
		
		if (CurrentPPEffectSetting.bEnabled && CurrentPPEffectSetting.bStarted)
		{
			if (CurrentPPEffectSetting.PostProcess_MID && (CurrentPPEffectSetting.PostProcess_Data->ScalarParameters.Num() > 0 || CurrentPPEffectSetting.PostProcess_Data->VectorParameters.Num() > 0))
			{
				if (CurrentPPEffectSetting.CanStopPostProcessEffect())
				{
					StopPostProcessEffect(PostProcessSettings, InPostProcessEffect);
					
					return false;
				}
				
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
								if (CurrentScalarParam.bUseCurve)
								{
									if (CurrentScalarParam.Curve.ExternalCurve)
									{
										const float CurveValue = CurrentScalarParam.Curve.ExternalCurve->GetFloatValue(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
										CurrentPPEffectSetting.PostProcess_MID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);

										#if WITH_EDITOR
										if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
											ULog::Number(CurveValue, CurrentScalarParam.ParameterName.ToString() + " Curve value: ");
										#endif
									}
									else if (CurrentScalarParam.Curve.EditorCurveData.GetNumKeys() > 1)
									{
										const float CurveValue = CurrentScalarParam.Curve.EditorCurveData.Eval(CurrentScalarParam.PlayState >= EPostProcessState::Reverse ? CurrentScalarParam.TimeRemaining : CurrentScalarParam.ElapsedTime);
										CurrentPPEffectSetting.PostProcess_MID->SetScalarParameterValue(CurrentScalarParam.ParameterName, CurveValue);

										#if WITH_EDITOR
										if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
											ULog::Number(CurveValue, CurrentScalarParam.ParameterName.ToString() + " Curve value: ");
										#endif
									}

									if (CurrentScalarParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentScalarParam.bReverseAtAnyTime)
									{
										if (CurrentScalarParam.PlayState == EPostProcessState::WaitingForReverse)
										{
											if (CurrentScalarParam.ReverseRequirements.Num() > 0)
											{
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
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
												if (!FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
												{
													CurrentScalarParam.Forward();
												}
											}
										}
									}
								}
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
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
												if (!FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentScalarParam.ReverseRequirements))
												{
													CurrentScalarParam.Forward();
												}
											}
										}
									}

									#if WITH_EDITOR
									if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
										ULog::Number(EasingValue, CurrentScalarParam.ParameterName.ToString() + " EasingValue: ");
									#endif
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
								if (CurrentVectorParam.bUseCurve)
								{
									if (CurrentVectorParam.Curve.ExternalCurve)
									{
										const FLinearColor CurveValue = CurrentVectorParam.Curve.ExternalCurve->GetLinearColorValue(CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime);
										CurrentPPEffectSetting.PostProcess_MID->SetVectorParameterValue(CurrentVectorParam.ParameterName, CurveValue);

										#if WITH_EDITOR
										if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
											ULog::Color(CurveValue, false, CurrentVectorParam.ParameterName.ToString() + " Curve value: ");
										#endif
									}
									else
									{
										const FLinearColor CurveValue = CurrentVectorParam.Curve.GetLinearColorValue(CurrentVectorParam.PlayState >= EPostProcessState::Reverse ? CurrentVectorParam.TimeRemaining : CurrentVectorParam.ElapsedTime);
										CurrentPPEffectSetting.PostProcess_MID->SetVectorParameterValue(CurrentVectorParam.ParameterName, CurveValue);

										#if WITH_EDITOR
										if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
											ULog::Color(CurveValue, false, CurrentVectorParam.ParameterName.ToString() + " Curve value: ");
										#endif
									}

									if (CurrentVectorParam.EffectEndOption == EPostProcessEndOptions::Reverse || CurrentVectorParam.bReverseAtAnyTime)
									{
										if (CurrentVectorParam.PlayState == EPostProcessState::WaitingForReverse)
										{
											if (CurrentVectorParam.ReverseRequirements.Num() > 0)
											{
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
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
												if (!FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
												{
													CurrentVectorParam.Forward();
												}
											}
										}
									}
								}
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
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
												if (FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
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
												if (!FulfillsAllPostProcessRequirements(Context, nullptr, nullptr, CurrentVectorParam.ReverseRequirements))
												{
													CurrentVectorParam.Forward();
												}
											}
										}
									}

									#if WITH_EDITOR
									if (CurrentPPEffectSetting.PostProcess_Data->bDebug)
										ULog::Vector(EasingValue, false, CurrentVectorParam.ParameterName.ToString() + " EasingValue: ");
									#endif
								}
							}
						}
					}
				}
			}
		}
	}

	return true;
}

FKey UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(const FName& ActionName, const bool bUsingGamepad, int32 Index)
{
	TArray<FInputActionKeyMapping> ActionMapping;
	UInputSettings::GetInputSettings()->GetActionMappingByName(ActionName, ActionMapping);

	int32 CurrentMappingIndex = 0;
	for (const FInputActionKeyMapping& ActionKeyMapping : ActionMapping)
	{
		if (!(bUsingGamepad ^ ActionKeyMapping.Key.IsGamepadKey()))
		{
			if (Index < 0 || CurrentMappingIndex == Index)
				return ActionKeyMapping.Key;

			CurrentMappingIndex++;
		}
	}

	// backup for if they've bound UseOnly instead of Use
	if (ActionName == "Use")
	{
		return GetKeyFromInputActionName("UseOnly", bUsingGamepad, Index);
	}
	return EKeys::Invalid;
}

FSlateBrush UReadyOrNotFunctionLibrary::GetIconFromInputKeyName(const FName& RonKeyFName)
{
	if (!GetRoNData())
		return FSlateBrush();

	if (const UDataTable* InputKeyGamePadIconTable = GetRoNData()->InputKeyGamePadIconTable)
	{
		if (const FRonInputKeyGamePadIconTable* InputKeyRow = InputKeyGamePadIconTable->FindRow<FRonInputKeyGamePadIconTable>(RonKeyFName, TEXT("GetIconFromInputKeyName")))
		{
			return InputKeyRow->PS5;
		}
	}
	
	return FSlateBrush();
}

FKey UReadyOrNotFunctionLibrary::GetKeyFromInputAxisName(const FName& AxisName, const bool bOnlyGamepadKey, const int32 Index)
{
	TArray<FInputAxisKeyMapping> AxisMapping;
	UInputSettings::GetInputSettings()->GetAxisMappingByName(AxisName, AxisMapping);

	if (bOnlyGamepadKey)
	{
		if (AxisMapping.IsValidIndex(Index))
		{
			int32 CurrentAxisMappingIndex = 0;
			for (const FInputAxisKeyMapping& AxisKeyMapping : AxisMapping)
			{
				if (AxisKeyMapping.Key.IsGamepadKey())
				{
					if (CurrentAxisMappingIndex == Index)
						return AxisKeyMapping.Key;

					CurrentAxisMappingIndex++;
				}
			}
		}
	}
	else
	{
		if (AxisMapping.IsValidIndex(Index))
			return AxisMapping[Index].Key;
	}
	
	return EKeys::Invalid;
}

FKey UReadyOrNotFunctionLibrary::ConvertIntToFKey(const int32 Integer)
{
	switch (FMath::Clamp(Integer, 0, 9))
	{
		case 0:
			return EKeys::Zero;
		case 1:
			return EKeys::One;
		case 2:
			return EKeys::Two;
		case 3:
			return EKeys::Three;
		case 4:
			return EKeys::Four;
		case 5:
			return EKeys::Five;
		case 6:
			return EKeys::Six;
		case 7:
			return EKeys::Seven;
		case 8:
			return EKeys::Eight;
		case 9:
			return EKeys::Nine;
		default:
			return EKeys::Invalid;
	}
}

FRonKey UReadyOrNotFunctionLibrary::ConvertUnrealKeyToRonKey(const FKey& InKey)
{
	if (!GetRoNData())
		return FRonKey();

	if (UDataTable* RonInputKeyTable = GetRoNData()->RonInputKeyTable)
	{
		if (FRonInputKeyTable* InputKeyRow = RonInputKeyTable->FindRow<FRonInputKeyTable>(InKey.GetFName(), TEXT("ConvertUnrealKeyToRonKey")))
		{
			return InputKeyRow->Key;
		}
	}

	return FRonKey();
}

FString UReadyOrNotFunctionLibrary::ConvertUnrealKeyNameToRonKeyName(const FKey& InKey)
{
	if (!GetRoNData() || InKey.GetFName() == NAME_None)
		return InKey.GetDisplayName().ToString();
	
	if (const UDataTable* RonInputKeyTable = GetRoNData()->RonInputKeyTable)
	{
		if (FRonInputKeyTable* InputKeyRow = RonInputKeyTable->FindRow<FRonInputKeyTable>(InKey.GetFName(), TEXT("ConvertUnrealKeyNameToRonKeyName"), false))
		{
			return InputKeyRow->Key.InputName;
		}
	}

	return InKey.GetDisplayName().ToString();
}

FSlateBrush UReadyOrNotFunctionLibrary::ConvertKeyToIcon(const FKey& InKey)
{
	if (!GetRoNData())
		return FSlateBrush();

	if (UDataTable* RonInputKeyTable = GetRoNData()->RonInputKeyTable)
	{
		if (FRonInputKeyTable* InputKeyRow = RonInputKeyTable->FindRow<FRonInputKeyTable>(InKey.GetFName(), TEXT("ConvertKeyToIcon")))
		{
			return InputKeyRow->Key.IconBrush;
		}
	}

	return FSlateBrush();
}

bool UReadyOrNotFunctionLibrary::IsItemEquipped(AReadyOrNotCharacter* PlayerCharacter, const EItemCategory ItemCategory)
{
	if (!PlayerCharacter)
		return false;
	
	return PlayerCharacter->GetEquippedItem() ? PlayerCharacter->GetEquippedItem()->ContainsItemCategory(ItemCategory) : false;
}

bool UReadyOrNotFunctionLibrary::IsItemInInventory(AReadyOrNotCharacter* PlayerCharacter, const EItemCategory ItemCategory)
{
	if (!PlayerCharacter)
		return false;
	
	return PlayerCharacter->GetInventoryComponent()->GetInventoryItemOfType(ItemCategory) != nullptr;
}

bool UReadyOrNotFunctionLibrary::ReportBadAIAction(ABadAIAction* InBadAIActionActor, const FText& InSummary, const FText& InDescription, const bool bReportToLog, const bool bDrawDebugString)
{
	if (!InBadAIActionActor)
		return false;

	InBadAIActionActor->AddNote(InSummary, InDescription);
	InBadAIActionActor->Report(bReportToLog, bDrawDebugString);

	return true;
}

bool UReadyOrNotFunctionLibrary::RemoveBadAIActionReport(ABadAIAction* InBadAIActionActor, const bool bReportToLog, const bool bDrawDebugString)
{
	if (!InBadAIActionActor)
		return false;

	InBadAIActionActor->RemoveReport(bReportToLog, bDrawDebugString);

	return true;
}

FCollisionQueryParams UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(AReadyOrNotCharacter* InCharacterA, AReadyOrNotCharacter* InCharacterB)
{
	FCollisionQueryParams CollisionQueryParams;
	
	if (InCharacterA)
	{
		CollisionQueryParams.AddIgnoredActor(InCharacterA);
		CollisionQueryParams.AddIgnoredActors(InCharacterA->GetCollisionIgnoredActors());
		CollisionQueryParams.AddIgnoredComponents(InCharacterA->GetCollisionIgnoredComponents());
	}

	if (InCharacterB)
	{
		CollisionQueryParams.AddIgnoredActor(InCharacterB);
		CollisionQueryParams.AddIgnoredActors(InCharacterB->GetCollisionIgnoredActors());
		CollisionQueryParams.AddIgnoredComponents(InCharacterB->GetCollisionIgnoredComponents());
	}
	
	return CollisionQueryParams;
}

void UReadyOrNotFunctionLibrary::TickCodeAt(const float TickInterval, float& ElapsedTime, const float DeltaTime, TFunctionRef<void()> Func)
{
	ElapsedTime += DeltaTime;
	if (ElapsedTime > TickInterval)
	{
		ElapsedTime = 0.0f;
		
		Func();
	}
}

void UReadyOrNotFunctionLibrary::SetPlanarReflectionScreenPercentage(UPlanarReflectionComponent* InPlanarReflectionComponent, float NewScreenPercentage)
{
	if (InPlanarReflectionComponent)
	{
		InPlanarReflectionComponent->ScreenPercentage = NewScreenPercentage;
	}
}

EEasingFunc::Type UReadyOrNotFunctionLibrary::StringToEasingFunc(const FString InEasingFunc)
{
	if (InEasingFunc.Equals("Linear", ESearchCase::IgnoreCase))
		return EEasingFunc::Linear;
	
	if (InEasingFunc.Equals("EaseIn", ESearchCase::IgnoreCase))
		return EEasingFunc::EaseIn;

	if (InEasingFunc.Equals("EaseOut", ESearchCase::IgnoreCase))
		return EEasingFunc::EaseOut;

	if (InEasingFunc.Equals("EaseInOut", ESearchCase::IgnoreCase))
		return EEasingFunc::EaseInOut;

	if (InEasingFunc.Equals("ExpoIn", ESearchCase::IgnoreCase))
		return EEasingFunc::ExpoIn;

	if (InEasingFunc.Equals("ExpoOut", ESearchCase::IgnoreCase))
		return EEasingFunc::ExpoOut;

	if (InEasingFunc.Equals("ExpoInOut", ESearchCase::IgnoreCase))
		return EEasingFunc::ExpoInOut;

	if (InEasingFunc.Equals("CircularIn", ESearchCase::IgnoreCase))
		return EEasingFunc::CircularIn;

	if (InEasingFunc.Equals("CircularOut", ESearchCase::IgnoreCase))
		return EEasingFunc::CircularOut;

	if (InEasingFunc.Equals("CircularInOut", ESearchCase::IgnoreCase))
		return EEasingFunc::CircularInOut;
	
	return EEasingFunc::Linear;
}

#if PLATFORM_WINDOWS
/*// Normalizes the path returned by GetProcessImageFileName
static HRESULT NormalizeNTPath(wchar_t* pszPath)
{
	#define NUMCHARS(a) (sizeof(a)/sizeof(*a))
	
    wchar_t* DoubleSlash = wcschr(&pszPath[1], '\\');
    if (DoubleSlash)
    	DoubleSlash = wcschr(DoubleSlash+1, '\\');
	
    if (!DoubleSlash)
		return E_FAIL;

	const wchar_t cSave = *DoubleSlash;
	*DoubleSlash = 0;

    wchar_t szNTPath[_MAX_PATH];
    wchar_t szDrive[_MAX_PATH] = L"A:";
	
    // We'll need to query the NT device names for the drives to find a match with pszPath
    for (wchar_t DriveLetter = 'A'; DriveLetter <= 'Z'; ++DriveLetter)
    {
        szDrive[0] = DriveLetter;
        szNTPath[0] = 0;

    	// Do the paths match?
        if (QueryDosDevice(szDrive, szNTPath, NUMCHARS(szNTPath)) != 0 && _wcsicmp(szNTPath, pszPath) == 0)
        {
            wcscat_s(szDrive, NUMCHARS(szDrive), L"\\");
            wcscat_s(szDrive, NUMCHARS(szDrive), DoubleSlash+1);

        	wcscpy_s(pszPath, _MAX_PATH, szDrive);
        	
            return S_OK;
        }
    }
	
    *DoubleSlash = cSave;
	
    return E_FAIL;
}
    
int32 UReadyOrNotFunctionLibrary::GetRunningProcessID_Windows(wchar_t const* ProcessName)
{
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	int32 FoundProcessID = -1;

	const HANDLE SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(SnapshotHandle, &ProcessEntry))
	{
		while (Process32Next(SnapshotHandle, &ProcessEntry))
		{
			if (!_wcsicmp(ProcessEntry.szExeFile, ProcessName))
			{
				FoundProcessID = ProcessEntry.th32ProcessID;
				break;
			}
		}
	}

	CloseHandle(SnapshotHandle);
	return FoundProcessID;
}

int32 UReadyOrNotFunctionLibrary::GetRunningProcessID_Windows(const FString& FromHash)
{
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	int32 FoundProcessID = -1;

	const HANDLE SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(SnapshotHandle, &ProcessEntry))
	{
		while (Process32Next(SnapshotHandle, &ProcessEntry))
		{
			const HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessEntry.th32ProcessID);
			
			TCHAR Path[_MAX_PATH+1];
			GetProcessImageFileName(hProcess, Path, ProcessEntry.dwSize);

			NormalizeNTPath(Path);
			if (FString(Path).Contains(".exe"))
			{
				FMD5Hash Hash = FMD5Hash::HashFile(Path);
				
				FString ExeFileHashString = LexToString(Hash);
				
				if (ExeFileHashString == FromHash)
				{
					FoundProcessID = ProcessEntry.th32ProcessID;
					break;
				}
			}
		}
	}

	CloseHandle(SnapshotHandle);
	return FoundProcessID;
}

bool UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(wchar_t const* ProcessName)
{
	bool bExists = false;
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

	const HANDLE SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(SnapshotHandle, &ProcessEntry))
	{
		while (Process32Next(SnapshotHandle, &ProcessEntry))
		{
			if (!_wcsicmp(ProcessEntry.szExeFile, ProcessName))
			{
				bExists = true;
				break;
			}
		}
	}

	CloseHandle(SnapshotHandle);
	return bExists;
}

bool UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(const FString& FromHash)
{
	bool bExists = false;
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

	const HANDLE SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(SnapshotHandle, &ProcessEntry))
	{
		while (Process32Next(SnapshotHandle, &ProcessEntry))
		{
			const HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessEntry.th32ProcessID);
			
			TCHAR Path[_MAX_PATH+1];
			GetProcessImageFileName(hProcess, Path, sizeof(Path)/sizeof(Path[0]));

			NormalizeNTPath(Path);
			if (FString(Path).Contains(".exe"))
			{
				FMD5Hash Hash = FMD5Hash::HashFile(Path);
				
				FString ExeFileHashString = LexToString(Hash);
				
				if (ExeFileHashString == FromHash)
				{
					bExists = true;
					break;
				}
			}
		}
	}

	CloseHandle(SnapshotHandle);
	return bExists;			
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	TArray<FString>* WindowTitles = reinterpret_cast<TArray<FString>*>(lParam);

	const int Length = GetWindowTextLengthA(hwnd);
	
	std::string WindowTitle(Length + 1, '\0');
	GetWindowTextA(hwnd, &WindowTitle[0], Length + 1);
	
	const FString WindowTitleString = WindowTitle.c_str();

	if (!WindowTitleString.IsEmpty())
		WindowTitles->Add(WindowTitleString);

	return true;
}

bool UReadyOrNotFunctionLibrary::DoesProcessWindowTitleExist_Windows(const FString& ProcessWindowTitle)
{
	TArray<FString> WindowTitles;
	EnumWindows(EnumWindowsProc, LPARAM(&WindowTitles));

	for (const FString& Title : WindowTitles)
	{
		if (Title == ProcessWindowTitle)
		{
			return true;
		}
	}
	
	return false;
}

bool UReadyOrNotFunctionLibrary::DoesProcessWindowTitleContain_Windows(const FString& ProcessWindowTitleSubstring)
{
	TArray<FString> WindowTitles;
	EnumWindows(EnumWindowsProc, LPARAM(&WindowTitles));

	for (const FString& Title : WindowTitles)
	{
		if (Title.Contains(ProcessWindowTitleSubstring))
		{	
			return true;
		}
	}
	
	return false;
}


bool UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(wchar_t const* DLLName)
{
	bool bExists = false;

	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return false;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	HMODULE Modules[1024];
	DWORD ModulesNeeded;

	for (unsigned int i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			// Get a handle to this process
			const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, aProcesses[i]);

			if (Process != NULL)
			{
				// Get a list of all the modules in this process
				if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
				{
					for (uint32 j = 0; j < ModulesNeeded / sizeof(HMODULE); j++)
					{
						TCHAR ModuleName[MAX_PATH];

						// Get the name module
						GetModuleBaseName(Process, Modules[j], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

						if (FString(ModuleName) == FString(DLLName))
						{
							// DLL has been found
							bExists = true;
							break;
						}
					}
				}
			    
				// Release the handle to this process
				CloseHandle(Process);
			}
		}
	}
	return bExists;
}

bool UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(const FString& FromHash)
{
	bool bExists = false;

	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return false;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	HMODULE Modules[1024];
	DWORD ModulesNeeded;

	for (unsigned int i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			// Get a handle to this process
			const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, aProcesses[i]);

			if (Process != NULL)
			{
				// Get a list of all the modules in this process
				if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
				{
					for (uint32 j = 0; j < ModulesNeeded / sizeof(HMODULE); j++)
					{
						TCHAR ModuleName[MAX_PATH];

						// Get the name module
						GetModuleBaseName(Process, Modules[j], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

						TCHAR Path[_MAX_PATH+1];
						GetModuleFileName(GetModuleHandle(ModuleName), Path, sizeof(Path)/sizeof(Path[0]));
			
						NormalizeNTPath(Path);
						if (FString(Path).Contains(".dll"))
						{
							FMD5Hash Hash = FMD5Hash::HashFile(Path);

							FString DLLFileHashString = LexToString(Hash);
				
							if (DLLFileHashString == FromHash)
							{
								bExists = true;
								break;
							}
						}
					}
				}
			    
				// Release the handle to this process
				CloseHandle(Process);
			}
		}
	}
	return bExists;
	
	/*
	HMODULE Modules[1024];
	DWORD ModulesNeeded;

	// Get a handle to this process
	const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

	if (!Process)
		return false;

	bool bExists = false;

	// Get a list of all the modules in this process
	if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
	{
		for (uint32 i = 0; i < (ModulesNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR ModuleName[MAX_PATH];

			// Get the name module
			GetModuleBaseName(Process, Modules[i], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

			TCHAR Path[_MAX_PATH+1];
			GetModuleFileName(GetModuleHandle(ModuleName), Path, sizeof(Path)/sizeof(Path[0]));
			
			NormalizeNTPath(Path);
			FMD5Hash Hash = FMD5Hash::HashFile(Path);

			FString DLLFileHashString = LexToString(Hash);
			
			if (DLLFileHashString == FromHash)
			{
				bExists = true;
				break;
			}
		}
	}
    
	// Release the handle to the process.
	CloseHandle(Process);
	return bExists;#1#
}

bool UReadyOrNotFunctionLibrary::IsDLLLoadedThisProcess_Windows(wchar_t const* DLLName)
{
	bool bExists = false;

	// Get the list of process identifiers.
	HMODULE Modules[1024];
	DWORD ModulesNeeded;

	// Get a handle to this process
	const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, GetCurrentProcessId());

	if (Process != NULL)
	{
		// Get a list of all the modules in this process
		if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
		{
			for (uint32 j = 0; j < ModulesNeeded / sizeof(HMODULE); j++)
			{
				TCHAR ModuleName[MAX_PATH];

				// Get the name module
				GetModuleBaseName(Process, Modules[j], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

				if (FString(ModuleName) == FString(DLLName))
				{
					// DLL has been found
					bExists = true;
					break;
				}
			}
		}
	    
		// Release the handle to this process
		CloseHandle(Process);
	}
	
	return bExists;
}

bool UReadyOrNotFunctionLibrary::IsDLLLoadedThisProcess_Windows(const FString& FromHash)
{
	HMODULE Modules[1024];
	DWORD ModulesNeeded;

	// Get a handle to this process
	const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, GetCurrentProcessId());

	if (!Process)
		return false;

	bool bExists = false;

	// Get a list of all the modules in this process
	if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
	{
		for (uint32 i = 0; i < (ModulesNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR ModuleName[MAX_PATH];

			// Get the name module
			GetModuleBaseName(Process, Modules[i], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

			TCHAR Path[_MAX_PATH+1];
			GetModuleFileName(GetModuleHandle(ModuleName), Path, sizeof(Path)/sizeof(Path[0]));
			
			NormalizeNTPath(Path);
			if (FString(Path).Contains(".dll"))
			{
				FMD5Hash Hash = FMD5Hash::HashFile(Path);

				FString DLLFileHashString = LexToString(Hash);
				
				if (DLLFileHashString == FromHash)
				{
					bExists = true;
					break;
				}
			}
		}
	}
    
	// Release the handle to the process.
	CloseHandle(Process);
	return bExists;
}

bool UReadyOrNotFunctionLibrary::IsDLLLoadedInProcess_Windows(wchar_t const* ProcessName, wchar_t const* DLLName)
{
	HMODULE Modules[1024];
	DWORD ModulesNeeded;
	const DWORD ProcessID = GetRunningProcessID_Windows(ProcessName);

	// Get a handle to the process.
	const HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, ProcessID);

	if (!Process)
		return false;

	bool bExists = false;

	// Get a list of all the modules in this process
	if (EnumProcessModules(Process, Modules, sizeof(Modules), &ModulesNeeded))
	{
		for (uint32 i = 0; i < (ModulesNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR ModuleName[MAX_PATH];

			// Get the name module
			GetModuleBaseName(Process, Modules[i], ModuleName, sizeof(ModuleName) / sizeof(TCHAR));

			if (FString(ModuleName) == FString(DLLName))
			{
				bExists = true;
				break;
			}
		}
	}
    
	// Release the handle to the process.
	CloseHandle(Process);
	return bExists;
}*/
#endif

UReadyOrNotGameUserSettings* UReadyOrNotFunctionLibrary::GetReadyOrNotGameUserSettings()
{
	return Cast<UReadyOrNotGameUserSettings>(GEngine->GameUserSettings);
}

bool UReadyOrNotFunctionLibrary::GetUseGearListInsteadOfRadial()
{
#ifdef USE_GEARLIST_IN_PREMISSIONPLANNING
	return true;
#endif
	return false;
}

bool UReadyOrNotFunctionLibrary::IsDLSSEnabled()
{
	// ##UE5UPGRADE## Re add this after DLSS upgraded   
	return false;
	// return UDLSSLibrary::IsDLSSSupported();
}

bool UReadyOrNotFunctionLibrary::IsFSREnabled()
{
#ifdef AMD_FSR_ENABLED
	return true;
#endif
	return false;
}

float UReadyOrNotFunctionLibrary::GetAspectRatio()
{
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Result2D;
		GEngine->GameViewport->GetViewportSize(Result2D);
		return (Result2D.X * 1.0f) / (Result2D.Y * 1.0f);
	}
	return 0;
}

float UReadyOrNotFunctionLibrary::GetWeaponFOVOffset()
{
	float FOV;
	UBpGameplayHelperLib::GetFoV(FOV);
	float AspectRatio = GetAspectRatio();
	//float Offset = (AspectRatio - 1.0f) * 8.0f;
	float DeltaFOV = 90.0f - FOV;
	//Offset = Offset + (DeltaFOV / (AspectRatio * 2.0f));
	
	//GEngine->AddOnScreenDebugMessage(9999999, -1, FColor::White, "Aspect Ratio: " + FString::SanitizeFloat(AspectRatio) + " FOV: " + FString::SanitizeFloat(FOV) + " Calculated Offset: " + FString::SanitizeFloat(Offset) + " DeltaFOV: " + FString::SanitizeFloat(DeltaFOV));
	
	return FMath::Clamp(((AspectRatio - 16/9) * 4.0f) + (DeltaFOV / 5.0f), -6.0f, 30.0f); //FMath::Clamp(Offset, -3.0f, 22.0f);
	
	float WeaponFOVOffset = FMath::Clamp((AspectRatio - 16/9) * 8.0f, 4.0f, 8.0f);
	return WeaponFOVOffset;
}

float UReadyOrNotFunctionLibrary::GetInterfaceFovOffset(float InFov)
{
	float AspectRatio = GetAspectRatio();
	float OutFov = InFov + ((AspectRatio - 1.2f) * 15.0f);
	return FMath::Clamp(OutFov, InFov *= 0.5f, 140.0f);
}

bool UReadyOrNotFunctionLibrary::IsCoop(UWorld* World)
{
	if (World)
	{
		return World->GetGameState<ACoopGS>() != nullptr;
	}
	return false;
}

bool UReadyOrNotFunctionLibrary::HasStartedMatch(UWorld* World)
{
	if (World)
	{
		AReadyOrNotGameState* gs = World->GetWorld()->GetGameState<AReadyOrNotGameState>();
		return gs && gs->MatchState == EMatchState::MS_Playing;
	}
	return false;
}

bool UReadyOrNotFunctionLibrary::IsSinglePlayer(UWorld* World)
{
	if (World)
	{
		if (const AReadyOrNotGameState* GS = World->GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			return GS->GetNetMode() == NM_Standalone || GS->PlayerArray.Num() == 1;
		}
	}
	
	return true;
}

void UReadyOrNotFunctionLibrary::ServerTravel(FString URL)
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (World)
	{
		if (World->GetAuthGameMode())
		{
			World->GetAuthGameMode()->ProcessServerTravel(URL, true);
		}
	}
}

ECOOPMode UReadyOrNotFunctionLibrary::GetCOOPMode()
{
	ACoopGM* CoopGM = Cast<ACoopGM>(UBpGameplayHelperLib::GetWorldStatic()->GetAuthGameMode<ACoopGM>());
	if (CoopGM)
	{
		return CoopGM->GetCOOPMode();
	}
	ACoopGS* CoopGS = Cast<ACoopGS>(UBpGameplayHelperLib::GetWorldStatic()->GetGameState());
	if (CoopGS)
	{
		return CoopGS->Mode;
	}
	return ECOOPMode::CM_None;
}

void UReadyOrNotFunctionLibrary::StopAllAudio(UWorld* World)
{
	for (TObjectIterator<UFMODBus>It; It; ++It)
	{
		UFMODBus* FMODBus = *It;
		UFMODBlueprintStatics::BusStopAllEvents(FMODBus, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
	}

	for (TObjectIterator<UFMODEvent>It; It; ++It)
	{
		UFMODEvent* Event = *It;
		TArray<FFMODEventInstance> EventInst = UFMODBlueprintStatics::FindEventInstances(World, Event);
		for (FFMODEventInstance e : EventInst)
		{
			UFMODBlueprintStatics::EventInstanceStop(e, true);
		}
	}

	for (TObjectIterator<UAudioComponent>It; It; ++It)
	{
		It->Stop();
	}
}

bool UReadyOrNotFunctionLibrary::IsInLobby()
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (!World)
		return false;

	FString LobbyLevel = World->GetGameInstance<UReadyOrNotGameInstance>()->LobbyLevel;
	return World->GetMapName().Contains(LobbyLevel);
}

bool UReadyOrNotFunctionLibrary::IsInDefusalWarmup()
{
	UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	if (!World)
		return false;

	if (World && World->GetGameState<ADefusalGS>())
	{
		return World->GetGameState<ADefusalGS>()->GetDefusalMatchstate() == EDefusalMatchSate::DMS_PreRoundTimer;
	}
	return false;
}

void UReadyOrNotFunctionLibrary::RestartGame(const UObject* WorldContextObject)
{
#if PLATFORM_WINDOWS && defined(WITH_MODIO)
	if (UReadyOrNotStatics::GetReadyOrNotGameInstance())
	{
		// call this for cleanup purposes
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->MountInstalledMods();
		// handle in pre-exit to give a guartenteed chance for mod paks handles to be released
		UReadyOrNotStatics::GetReadyOrNotGameInstance()->bWantsToRestartGameExe = true;
	}
#endif
	UKismetSystemLibrary::QuitGame(WorldContextObject, nullptr, EQuitPreference::Quit, false);
}

void UReadyOrNotFunctionLibrary::RegisterTick(AActor* Actor)
{
	if (Actor)
		Actor->PrimaryActorTick.RegisterTickFunction(Actor->GetLevel());
}

void UReadyOrNotFunctionLibrary::UnregisterTick(AActor* Actor)
{
	if (Actor)
		Actor->PrimaryActorTick.UnRegisterTickFunction();
}

FRoom UReadyOrNotFunctionLibrary::GetRoomDataForLocation(FVector Location)
{
	if (FRoom* Room = GetRoomDataForLocation_Ref(Location))
	{
		return *Room;
	}

	return FRoom();
}

FRoom UReadyOrNotFunctionLibrary::GetRoomDataFromName(FName Name)
{
	if (FRoom* Room = GetRoomDataFromName_Ref(Name))
	{
		return *Room;
	}

	return FRoom();
}

FRoom* UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(FVector Location)
{
	if (const AReadyOrNotGameState* GS = UReadyOrNotStatics::GetReadyOrNotGameState())
	{
		if (GS->RoomData)
		{
			// Project to nav mesh for accurate detection
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GS->GetWorld()))
			{
				FNavLocation NavLocation;
				constexpr float ZHeight = 10000.0f;
				const FVector Extent = FVector(300.0f, 300.0f, ZHeight);
				FVector ProjectedLocation = Location;
				if (NavSys->ProjectPointToNavigation(Location - FVector::UpVector * (ZHeight - 100.0f), NavLocation, Extent))
				{
					ProjectedLocation = NavLocation.Location + FVector::UpVector * 40.0f;
				}

				// Make sure we can see the projected point (in the case of multiple floors in a building)
				FHitResult Hit;
				if (GS->GetWorld()->LineTraceSingleByChannel(Hit, Location, ProjectedLocation, ECC_Pawn))
				{
					Location = Hit.Location + FVector::UpVector * 40.0f;
				}
				else
				{
					Location = ProjectedLocation;
				}
			}
			
			//DrawDebugBox(GS->GetWorld(), Location, FVector(15.0f), FColor::Cyan, false, 0.033f);

			// find the nearest threat actor within 10m of the location, requiring line of sight
			if (const AThreatAwarenessActor* NearestThreat = UThreatAwarenessSubsystem::Get(GS)->GetNearestThreatForLocation(Location, 1000.0f, 200.0f, true))
			{
				//DrawDebugBox(GS->GetWorld(), NearestThreat->GetActorLocation(), FVector(15.0f), FColor::Orange, false, 0.033f);
				//LOG_NUMBER(GS->RoomData->Rooms.Num());
				for (uint8 i = 0; i < GS->RoomData->Rooms.Num(); i++)
				{
					if (GS->RoomData->Rooms[i].Name == NearestThreat->OwningRoom)
						return &GS->RoomData->Rooms[i];
				}
			}
		}
	}

	return nullptr;
}

FRoom* UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(FName Name)
{
	if (AReadyOrNotGameState* GS = UReadyOrNotStatics::GetReadyOrNotGameState())
	{
		return GS->RoomData->Rooms.FindByPredicate([&Name](const FRoom& Room)
		{
			return Room.Name == Name;
		});
	}

	return nullptr;
}

FRoom* UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Editor(FVector Location)
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GWorld))
	{
		// Project to nav mesh for accurate detection
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(WorldData->GetWorld()))
		{
			FNavLocation NavLocation;
			constexpr float ZHeight = 10000.0f;
			const FVector Extent = FVector(300.0f, 300.0f, ZHeight);
			FVector ProjectedLocation = Location;
			if (NavSys->ProjectPointToNavigation(Location - FVector::UpVector * (ZHeight - 100.0f), NavLocation, Extent))
			{
				ProjectedLocation = NavLocation.Location + FVector::UpVector * 40.0f;
			}

			// Make sure we can see the projected point (in the case of multiple floors in a building)
			FHitResult Hit;
			if (WorldData->GetWorld()->LineTraceSingleByChannel(Hit, Location, ProjectedLocation, ECC_Pawn))
			{
				Location = Hit.Location + FVector::UpVector * 40.0f;
			}
			else
			{
				Location = ProjectedLocation;
			}
		}
		
		//DrawDebugBox(GS->GetWorld(), Location, FVector(15.0f), FColor::Cyan, false, 0.033f);

		// find the nearest threat actor within 10m of the location, requiring line of sight
		if (const AThreatAwarenessActor* NearestThreat = FindClosestActor<AThreatAwarenessActor>(WorldData->GetWorld(), Location, 1000.0f, [&](const AThreatAwarenessActor* T, const float Distance)
		{
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			return !WorldData->GetWorld()->LineTraceTestByObjectType(T->GetActorLocation(), Location, CollisionObjectQueryParams);
		}))
		{
			//DrawDebugBox(GS->GetWorld(), NearestThreat->GetActorLocation(), FVector(15.0f), FColor::Orange, false, 0.033f);
			//LOG_NUMBER(GS->RoomData->Rooms.Num());
			for (uint8 i = 0; i < WorldData->RoomData.Rooms.Num(); i++)
			{
				if (WorldData->RoomData.Rooms[i].Name == NearestThreat->OwningRoom)
					return &WorldData->RoomData.Rooms[i];
			}
		}
	}

	return nullptr;
}

FRoom* UReadyOrNotFunctionLibrary::GetRoomDataFromName_Editor(FName Name)
{
	if (AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(GWorld))
	{
		return WorldData->RoomData.Rooms.FindByPredicate([&Name](const FRoom& Room)
		{
			return Room.Name == Name;
		});
	}

	return nullptr;
}

#if PLATFORM_LINUX
bool UReadyOrNotFunctionLibrary::IsProcessRunning_Linux(wchar_t const* ProcessName)
{
	return false;
}
#endif
