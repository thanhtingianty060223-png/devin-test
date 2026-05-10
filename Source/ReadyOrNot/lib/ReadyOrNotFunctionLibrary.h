// Copyright Void Interactive, 2024

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Templates/SubclassOf.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/TimelineComponent.h"
#include "Enums.h"
#include "ReadyOrNotGameUserSettings.h"
#include "Structs.h"
#include "Components/Widget.h"
#include "Data/PostProcessEffectData.h"
#include "ReadyOrNotFunctionLibrary.generated.h"

UCLASS()
class READYORNOT_API UReadyOrNotFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void SetupTimeline(UObject* Object, FTimeline& InTimeline, TArray<UCurveFloat*> InCurveFloats, bool bLooping, float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName = NAME_None);
	
	static void SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveFloat* InCurveFloat, bool bLooping, float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName = NAME_None);
	static void SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveLinearColor* InCurveLinearColor, bool bLooping, float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName = NAME_None);
	static void SetupTimeline(UObject* Object, FTimeline& InTimeline, UCurveVector* InCurveVector, bool bLooping, float InPlaybackSpeed, const FName& TimelineCallbackFuncName, const FName& TimelineFinishedCallbackFuncName = NAME_None);

	template<class UserClass>
	static void StartTimerForCallback(FTimerHandle& TimerHandle, UserClass* Object, typename FTimerDelegate::TMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop = false, bool bAdjustForTimeDilation = false, float FirstDelay = -1.0f);
	
	template<class UserClass>
	static void StartTimerForCallback(FTimerHandle& TimerHandle, UserClass* Object, FTimerDelegate const& InDelegate, float Rate, bool bLoop = false, bool bAdjustForTimeDilation = false, float FirstDelay = -1.0f);
	
	template<class UserClass>
	static void StartTimerForCallback(UserClass* Object, FTimerDelegate const& InDelegate, float Rate, bool bLoop = false, bool bAdjustForTimeDilation = false, float FirstDelay = -1.0f);

	template<class UserClass>
	static void StartTimerForCallback(UserClass* Object, typename FTimerDelegate::TMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop = false, bool bAdjustForTimeDilation = false, float FirstDelay = -1.0f);
	
	template<class UserClass>
	static void StartTimerForCallback(UserClass* Object, typename FTimerDelegate::TConstMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop = false, bool bAdjustForTimeDilation = false, float FirstDelay = -1.0f);
	
	static void StopCallbackTimer(UObject* Object, FTimerHandle& TimerHandle);
	static void ResumeCallbackTimer(UObject* Object, FTimerHandle& TimerHandle);
	static void PauseCallbackTimer(UObject* Object, FTimerHandle& TimerHandle);
	
	static bool IsCallbackTimerActive(UObject* Object, const FTimerHandle& TimerHandle);
	static bool IsCallbackTimerActive(const UObject* Object, const FTimerHandle& TimerHandle);
	static bool IsCallbackTimerPaused(UObject* Object, const FTimerHandle& TimerHandle);
	static bool IsCallbackTimerPaused(const UObject* Object, const FTimerHandle& TimerHandle);
	static bool IsCallbackTimerPending(UObject* Object, const FTimerHandle& TimerHandle);
	static bool IsCallbackTimerPending(const UObject* Object, const FTimerHandle& TimerHandle);

	// Make your callback function a UFUNCTION(), otherwise it will not work
	template <typename UObjectTemplate, typename... VarTypes>
    static void StartTimerForCallback_Args(FTimerHandle& TimerHandle, UObjectTemplate* Object, const FName& CallbackFuncName, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay, VarTypes... Vars);

	// Make your callback function a UFUNCTION(), otherwise it will not work
	template <typename UObjectTemplate, typename... VarTypes>
	static void StartTimerForCallback_Args(UObjectTemplate* Object, const FName& CallbackFuncName, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay, VarTypes... Vars);

	UFUNCTION(BlueprintCallable)
	static class UAnimMontage* GetAnimationFromTable(const FString& AnimationName, bool bIsCrouching = false);
	
	UFUNCTION(BlueprintPure)
	static bool IsTableMontagePlaying(class APlayerCharacter* PlayerCharacter, const FString& AnimationName, bool bIsCrouching = false);

	static class UDataSingleton* GetRoNData();

	template<class ObjectClass>
	static ObjectClass* GetObjectAs(UObject* InObject);

	template<class ObjectClass>
	static TArray<ObjectClass*> GetObjectsOfClass();

	template<class ActorClass>
    static TArray<ActorClass*> GetActorsOfClass(UWorld* WorldContext);

	UFUNCTION(BlueprintCallable, Category="Supporter", meta=(WorldContext = "WorldContextObject"))
	static void CopySupporterStringToClipboard(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "ROM Function Library | Widget")
	static void SetSafeZonePadding(class USafeZoneSlot* SafeZoneSlot, FMargin Padding);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Player Post Processing")
	static bool FulfillsAllPostProcessRequirements(UObject* Context, APlayerCharacter* OwningCharacter, AActor* DamageCauser, const TArray<TSubclassOf<class UPostProcessRequirement>>& InRequirementClasses, const bool bForceSuccess = false);
	
	static UPostProcessRequirement* CreatePostProcessRequirement(UObject* Outer, TSubclassOf<UPostProcessRequirement> InRequirementClass);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities", meta = (DeterminesOutputType = "Class"))
	static APlayerCharacter* GetPlayerCharacterMutableDefaultObject(UClass* Class);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities", meta = (DeterminesOutputType = "Class"))
	static UObject* GetClassDefaultObject(UClass* Class);

	UFUNCTION(BlueprintCallable, Category = "RON Function Library | Utilities", meta = (DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static void PlayRandomFMODEventAtLocation(UObject* WorldContextObject, FVector Location, UPARAM(ref) TArray<UFMODEvent*>& FMODEvents);
	
	UFUNCTION(BlueprintCallable, Category = "RON Function Library | Utilities", meta = (DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
	static void PlayRandomFMODEvent_2D(UObject* WorldContextObject, UPARAM(ref) TArray<UFMODEvent*>& FMODEvents);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static bool AnyTrue(UPARAM(ref) TArray<bool>& BoolArray);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static bool AnyFalse(UPARAM(ref) TArray<bool>& BoolArray);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static bool AllTrue(UPARAM(ref) TArray<bool>& BoolArray);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
    static bool AllFalse(UPARAM(ref) TArray<bool>& BoolArray);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static TSubclassOf<ABaseItem> FindItemClassInItemDataTable(FName RowName);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities", meta = (DisplayName = "Find Nearest Floor", AutoCreateRefTerm = "InActorsToIgnore,InComponentsToIgnore"))
	static float FindNearestFloor_BP(AActor* InActor, const TArray<AActor*>& InActorsToIgnore, const TArray<UPrimitiveComponent*>& InComponentsToIgnore);
	
	static float FindNearestFloor(AActor* InActor, const TArray<AActor*>& InActorsToIgnore = {}, const TArray<UPrimitiveComponent*>& InComponentsToIgnore = {});

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static bool IsActorInsideSplineEnclosure(class ASplineTrigger* InSplineTrigger, AActor* InActor);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static bool IsActorOutsideSplineEnclosure(class ASplineTrigger* InSplineTrigger, AActor* InActor);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static FText SwatCommandToText(ESwatCommand SwatCommand);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static FString DoorBreachTypeToVoiceline(const EDoorBreachType DoorBreachType);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static FString DoorBreachTypeToVoiceline_Negative(const EDoorBreachType DoorBreachType);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static FString DoorCheckResultToVoiceline(const EDoorCheckResult DoorBreachType);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Utilities")
	static FString SimulateAnimatedText(UPARAM(ref) FString& FinalString, UPARAM(ref) int32& Iterator, UPARAM(ref) TArray<FString>& Chars, UPARAM(ref) float& ElapsedTime, UPARAM(ref) float& CurrentDelay, float DelayBetweenLetters, float DelayBetweenWords, float DeltaTime, bool& bCompleted);

	static UDecalComponent* CreateDecalComponent(UObject* Owner, UMaterialInterface* DecalMaterial, FVector DecalSize);
	
	UFUNCTION(BlueprintCallable, Category = "RON Function Library | Decal")
	static void SetDecalSize(UDecalComponent* InDecalComponent, FVector DecalSize);
	
	// Removes all widgets in array from parent and empties when done
	UFUNCTION(BlueprintCallable, Category = "RON Function Library | Widget")
	static void RemoveFromParentAndClear(UPARAM(ref) TArray<UWidget*>& InWidgets);

	template<class WidgetClass>
	static void RemoveFromParentAndClear(TArray<WidgetClass*>& InWidgets);

	template<typename T>
	static void RemoveAllNullElements(TArray<T*>& Array);
	
	template<typename T>
	static void RemoveAllNullElements(TArray<T>& Array);
	
	static void RemoveAllNullElements_Object(TArray<UObject*>& Array);
	static void RemoveAllNullElements_Class(TArray<TSubclassOf<UClass>>& Array);

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "RON Function Library | Utilities", DisplayName = "Remove All Null Elements", meta = (ArrayParm = "Array"))
	static void RemoveAllNullElements_BP(const TArray<TSubclassOf<UClass>>& Array);

	static void RemoveAllNullElements(void* TargetArray, const FArrayProperty* ArrayProp);

	DECLARE_FUNCTION(execRemoveAllNullElements_BP)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(nullptr);
		void* ArrayAddress = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_FINISH;
		P_NATIVE_BEGIN;
		RemoveAllNullElements(ArrayAddress, ArrayProperty);
		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, DisplayName = "Find Closest Actor From Location")
	static AActor* FindClosestActorFromLocation_Blueprint(const FVector& InTestLocation, const TArray<AActor*>& InActors);
	
	template<typename T>
	static T* FindClosestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors);
	
	template<typename T>
	static T* FindFurthestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors);
	
	template<typename T>
	static T* FindFurthestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors, TFunctionRef<bool(const T*, const float)> Predicate);

	template<typename T>
	static T* FindClosestActor(UWorld* WorldContext, const FVector& FromLocation, float MaxDistance = FLT_MAX, TFunctionRef<bool(const T*, const float)> Predicate = [](const T*, const float){ return true; });
	
	template<typename T>
	static T* FindClosestActor(UWorld* WorldContext, const FVector& FromLocation, TFunctionRef<bool(const T*, const float)> Predicate = [](const T*, const float){ return true; });

	template<typename T>
	static T* FindFurthestActor(UWorld* WorldContext, const FVector& FromLocation, TFunctionRef<bool(const T*, const float)> Predicate = [](const T*, const float){ return true; });

	UFUNCTION(BlueprintCallable, Category = "RON Function Library | System")
	static void PauseFMOD(bool bPaused);
	
	UFUNCTION(BlueprintCallable, Category = "RON Function Library | System")
	static void MuteFMOD(bool bMuted);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static FString DevMenuSettingsConfigDir();
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static FString BadAIActionConfigDir();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static FString GetServerNameFromCurrentSession();
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static TArray<FString> GetAllSectionNamesFromINIFile(FString ConfigFilePath);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static TArray<FString> GetSingleLineArrayFromINIFile(FString ConfigFilePath, FString Section, FString Key);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static int32 GetIntegerFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static float GetFloatFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static bool GetBoolFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
    static FVector GetVectorFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
    static FVector2D GetVector2DFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static FString GetStringFromINIFile(FString ConfigFilePath, FString Section, FString Key);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
    static TArray<FString> GetStringArrayFromINIFile(FString ConfigFilePath, FString Section, FString Key);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
    static bool FindConfigKeyFromINIFile(FString ConfigFilePath, FString Section, FString Key);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Widget", DisplayName = "GetWidgetSize (Absolute)")
	static FVector2D GetWidgetSize_Absolute(UWidget* InWidget);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Widget", DisplayName = "GetWidgetSize (Local)")
    static FVector2D GetWidgetSize_Local(UWidget* InWidget);

	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
	static FVector2D GetViewportPositionOfWidget(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget, FVector2D InCoordinates = FVector2D(0.0f, 0.0f));
	
	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
    static FVector2D GetPixelPositionOfWidget(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget, FVector2D InCoordinates = FVector2D(0.0f, 0.0f));
	
	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
    static FVector2D GetViewportPositionOfWidgetCenter(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget);
	
	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
    static FVector2D GetPixelPositionOfWidgetCenter(UObject* WorldContext, UWidget* InParentWidget, UWidget* InWidget);

	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
	static FVector2D CalculateOffscreenPositionFromWorldLocation_Ellipse(UObject* WorldContext, const FVector& WorldLocation, float ViewportOffset, bool& bIsOffscreen, float& Angle, float& ForwardDotProduct, float& RightDotProduct);
	
	UFUNCTION(BlueprintPure, Category = "Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
	static FVector2D CalculateOffscreenPositionFromWorldLocation_Square(UObject* WorldContext, const FVector& WorldLocation, float ViewportOffset, bool& bIsOffscreen, float& Angle, float& ForwardDotProduct, float& RightDotProduct);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
	static bool DoesWidgetOverlap(UObject* WorldContext, UWidget* ParentWidget, UWidget* WidgetA, UWidget* WidgetB);
	
	static bool DoesWidgetOverlap(UObject* WorldContext, UWidget* ParentWidget, const FVector2D& WidgetA_Position, const FVector2D& WidgetA_LocalSize, UWidget* WidgetB);

	UFUNCTION(BlueprintPure)
	static bool IsUsingGamepad(AReadyOrNotPlayerController* InController);

	// Format: {InputType} <Red>{Key}</> to <Red>{Action}</>
	// Example Output: Press C to Crouch
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Widget")
	static FText FormatPlayerActionText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText, const FString& InColorLabel = "Red");
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Widget", meta = (DefaultToSelf = "WorldContext", HidePin = "WorldContext"))
	static UHumanCharacterHUD_V2* GetPlayerHUD(UObject* WorldContext);
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static ERONBuildConfiguration GetBuildConfiguration();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static bool IsBuildPirated();
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static bool IsAprilFools();
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static bool IsHalloween();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Level")
	static FName GetCurrentLevelNameForLookupTable(UObject* Context);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Syste,")
	static bool LoadStringArrayFromFile(TArray<FString>& StringArray, int32& ArraySize, FString FullFilePath, bool ExcludeEmptyLines);

	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static TArray<ABaseItem*> GetAllItemsInMemory();
	
	UFUNCTION(BlueprintPure, Category = "RON Function Library | Level")
	static TArray<USoundBase*> GetAllSoundsInWorld();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | Level", meta = (DefaultToSelf = "Context"))
	static AReadyOrNotLevelScript* GetReadyOrNotLevelScript(UObject* Context);

	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
	static TArray<UAudioComponent*> GetAllAudioComponents();
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
	static TArray<UFMODEvent*> GetAll2DFMODAudioEvents();

	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
	static TArray<class UFMODBus*> GetAllFMODBusObjects();
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
	static bool IsFMODBusMuted(class UFMODBus* Bus);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
    static bool IsFMODBusPaused(class UFMODBus* Bus);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Audio")
    static float GetFMODBusVolume(class UFMODBus* Bus);

	UFUNCTION(BlueprintCallable, Category = "Debug Subsystem | FMOD")
    static void SetFMODVolume(float Volume);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | System")
	static UClass* GetClassFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category="RON Function Library | Post-Processing")
	static void SetupPostProcessEffect(UObject* Context, FPostProcessEffect& InPostProcessEffect);

	UFUNCTION(BlueprintCallable, Category="RON Function Library | Post-Processing")
    static void StartPostProcessEffect(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffect& InPostProcessEffect, AActor* DamageCauser = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="RON Function Library | Post-Processing")
    static void StartPostProcessEffect_Specific(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffectPlayer& InPostProcessEffectPlayer, AActor* DamageCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category="RON Function Library | Post-Processing")
    static void StopPostProcessEffect(FPostProcessSettings& PostProcessSettings,FPostProcessEffect& InPostProcessEffect);

	UFUNCTION(BlueprintCallable, Category="RON Function Library | Post-Processing")
    static bool ProcessPostProcessEffect(UObject* Context, FPostProcessSettings& PostProcessSettings, FPostProcessEffect& InPostProcessEffect, float DeltaTime);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FKey GetKeyFromInputActionName(const FName& ActionName, bool bOnlyGamepadKey = false, int32 Index = 0);

	UFUNCTION(BluePrintPure, Category = "Ron Function Library | Input")
	static FSlateBrush GetIconFromInputKeyName(const FName& RonKeyName);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FKey GetKeyFromInputAxisName(const FName& AxisName, bool bUsingGamepad = false, int32 Index = -1);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FKey ConvertIntToFKey(int32 Integer);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FRonKey ConvertUnrealKeyToRonKey(const FKey& InKey);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FString ConvertUnrealKeyNameToRonKeyName(const FKey& InKey);
	
	UFUNCTION(BlueprintPure, Category="RON Function Library | Input")
	static FSlateBrush ConvertKeyToIcon(const FKey& InKey);

	UFUNCTION(BlueprintPure, Category = "Player")
	static bool IsItemEquipped(AReadyOrNotCharacter* PlayerCharacter, EItemCategory ItemCategory);
	
	UFUNCTION(BlueprintPure, Category = "Player")
	static bool IsItemInInventory(AReadyOrNotCharacter* PlayerCharacter, EItemCategory ItemCategory);
	
	UFUNCTION(BlueprintCallable, Category = "Debug")
	static bool ReportBadAIAction(class ABadAIAction* InBadAIActionActor, const FText& InSummary, const FText& InDescription = INVTEXT("Empty"), bool bReportToLog = true, bool bDrawDebugString = true);
	
	UFUNCTION(BlueprintCallable, Category = "Debug")
	static bool RemoveBadAIActionReport(class ABadAIAction* InBadAIActionActor, bool bReportToLog = true, bool bDrawDebugString = true);

	static FCollisionQueryParams CreateCollisionQueryParams(AReadyOrNotCharacter* InCharacterA, AReadyOrNotCharacter* InCharacterB);
	
	static void TickCodeAt(float TickInterval, float& ElapsedTime, float DeltaTime, TFunctionRef<void()> Func);

	UFUNCTION(BlueprintCallable, Category = "RON Function Library | Graphics")
	static void SetPlanarReflectionScreenPercentage(class UPlanarReflectionComponent* InPlanarReflectionComponent, float NewScreenPercentage);

	UFUNCTION(BlueprintPure, Category = "Ease")
	static EEasingFunc::Type StringToEasingFunc(const FString InEasingFunc);

	#if PLATFORM_WINDOWS
	/*// Process name is the exe name (with the .exe included)
	static int32 GetRunningProcessID_Windows(wchar_t const* ProcessName);

	// FromHash is a MD5 hash string
	static int32 GetRunningProcessID_Windows(const FString& FromHash);

	// Process name is the exe name (with the .exe included)
	static bool IsProcessRunning_Windows(wchar_t const* ProcessName);

	// FromHash is a MD5 hash string
	static bool IsProcessRunning_Windows(const FString& FromHash);
	
	// DLL name is the module name (with the .dll included)
	static bool IsDLLLoaded_Windows(wchar_t const* DLLName);

	// FromHash is a MD5 hash string
	static bool IsDLLLoaded_Windows(const FString& FromHash);

	static bool IsDLLLoadedThisProcess_Windows(wchar_t const* DLLName);
	static bool IsDLLLoadedThisProcess_Windows(const FString& FromHash);
	
	// Process name is the exe name (with the .exe included). DLL name is the module name (with the .dll included)
	static bool IsDLLLoadedInProcess_Windows(wchar_t const* ProcessName, wchar_t const* DLLName);
	static bool DoesProcessWindowTitleExist_Windows(const FString& ProcessWindowTitle);
	static bool DoesProcessWindowTitleContain_Windows(const FString& ProcessWindowTitleSubstring);*/
	#endif

	UFUNCTION(BlueprintCallable)
	static UReadyOrNotGameUserSettings* GetReadyOrNotGameUserSettings();

	#if PLATFORM_LINUX
	static bool IsProcessRunning_Linux(wchar_t const* ProcessName);
	#endif

	// if true replaces the radial for selecting gear with a list
	UFUNCTION(BlueprintPure, Category = "RON Function Library | System")
	static bool GetUseGearListInsteadOfRadial();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | DLSS")
	static bool IsDLSSEnabled();

	UFUNCTION(BlueprintPure, Category = "RON Function Library | FSR")
	static bool IsFSREnabled();

	UFUNCTION(BlueprintPure)
	static float GetAspectRatio();

	UFUNCTION()
	static float GetWeaponFOVOffset();

	// help scale the fov for non-constrained cameras so that they look normal at all aspect ratios
	// useful in the pre-mission planning where you don't want black bars but you want the game to look
	// normal in every monitor setup
	UFUNCTION(BlueprintPure)
	static float GetInterfaceFovOffset(float InFov);

	static bool IsCoop(UWorld* World);
	static bool HasStartedMatch(UWorld* World);

	static bool IsSinglePlayer(UWorld* World);

	UFUNCTION(BlueprintCallable)
	static void ServerTravel(FString URL);

	UFUNCTION(BlueprintPure)
	static ECOOPMode GetCOOPMode();

	static void StopAllAudio(UWorld* World);

	UFUNCTION(BlueprintPure)
	static bool IsInLobby();

	UFUNCTION(BlueprintPure)
	static bool IsInDefusalWarmup();

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void RestartGame(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	static void RegisterTick(AActor* Actor);
	UFUNCTION(BlueprintCallable)
	static void UnregisterTick(AActor* Actor);
	
	UFUNCTION(BlueprintCallable)
	static FRoom GetRoomDataForLocation(FVector Location);
	
	UFUNCTION(BlueprintCallable)
	static FRoom GetRoomDataFromName(FName Name);
	
	static FRoom* GetRoomDataForLocation_Ref(FVector Location);
	static FRoom* GetRoomDataFromName_Ref(FName Name);
	
	static FRoom* GetRoomDataForLocation_Editor(FVector Location);
	static FRoom* GetRoomDataFromName_Editor(FName Name);

	FORCEINLINE static void FormatTextColor(FText& Text, const FString& Color) { Text = FText::Format(FText::FromStringTable("TextModifierTable", "ColorText"), FText::FromString(Color), Text); };
};


template <class UserClass>
void UReadyOrNotFunctionLibrary::StartTimerForCallback(FTimerHandle& TimerHandle, UserClass* Object, typename FTimerDelegate::TMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, Object, CallbackFunc, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <class UserClass>
void UReadyOrNotFunctionLibrary::StartTimerForCallback(FTimerHandle& TimerHandle, UserClass* Object, FTimerDelegate const& InDelegate, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, InDelegate, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <class UserClass>
void UReadyOrNotFunctionLibrary::StartTimerForCallback(UserClass* Object, FTimerDelegate const& InDelegate, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			FTimerHandle TimerHandle;
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, InDelegate, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <class UserClass>
void UReadyOrNotFunctionLibrary::StartTimerForCallback(UserClass* Object, typename FTimerDelegate::TMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			FTimerHandle TimerHandle;
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, Object, CallbackFunc, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <class UserClass>
void UReadyOrNotFunctionLibrary::StartTimerForCallback(UserClass* Object, typename FTimerDelegate::TConstMethodPtr<UserClass> CallbackFunc, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			FTimerHandle TimerHandle;
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, Object, CallbackFunc, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <typename UObjectTemplate, typename ... VarTypes>
void UReadyOrNotFunctionLibrary::StartTimerForCallback_Args(FTimerHandle& TimerHandle, UObjectTemplate* Object, const FName& CallbackFuncName, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay, VarTypes... Vars)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUFunction(Object, CallbackFuncName, Vars...);
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <typename UObjectTemplate, typename ... VarTypes>
void UReadyOrNotFunctionLibrary::StartTimerForCallback_Args(UObjectTemplate* Object, const FName& CallbackFuncName, float Rate, bool bLoop, bool bAdjustForTimeDilation, float FirstDelay, VarTypes... Vars)
{
	if (Object)
	{
		if (Object->GetWorld())
		{
			FTimerHandle TimerHandle;
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUFunction(Object, CallbackFuncName, Vars...);
			Object->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Rate * (bAdjustForTimeDilation ? UGameplayStatics::GetGlobalTimeDilation(Object) : 1.0f), bLoop, FirstDelay);
		}
	}
}

template <class ObjectClass>
ObjectClass* UReadyOrNotFunctionLibrary::GetObjectAs(UObject* InObject)
{
	return Cast<ObjectClass>(InObject);
}

template <class ObjectClass>
TArray<ObjectClass*> UReadyOrNotFunctionLibrary::GetObjectsOfClass()
{
	static_assert(TIsDerivedFrom<ObjectClass, UObject>::Value, "ObjectClass must be derived from a UObject class");
	
	TArray<ObjectClass*> Objects;

	for (TObjectIterator<ObjectClass> It; It; ++It)
	{
		Objects.Add(*It);
	}

	return Objects;
}

template <class ActorClass>
TArray<ActorClass*> UReadyOrNotFunctionLibrary::GetActorsOfClass(UWorld* WorldContext)
{
	static_assert(TIsDerivedFrom<ActorClass, AActor>::Value, "ActorClass must be derived from an AActor class");

	if (!WorldContext)
		return TArray<ActorClass*>();
	
	TArray<ActorClass*> Actors;

	for (TActorIterator<ActorClass> It(WorldContext); It; ++It)
	{
		Actors.Add(*It);
	}

	return Actors;	
}

template <class WidgetClass>
void UReadyOrNotFunctionLibrary::RemoveFromParentAndClear(TArray<WidgetClass*>& InWidgets)
{
	static_assert(TIsDerivedFrom<WidgetClass, UWidget>::Value, "WidgetClass must be derived from a UWidget class");

	if (InWidgets.Num() == 0)
		return;
	
	InWidgets.Remove(nullptr);
	
	for (WidgetClass* Widget : InWidgets)
	{
		if (IsValid(Widget))
			Widget->RemoveFromParent();
	}

	InWidgets.Empty();
}

template <typename T>
void UReadyOrNotFunctionLibrary::RemoveAllNullElements(TArray<T*>& Array)
{
	if (Array.Num() > 0)
	{
		Array.RemoveAll([](T* Element)
        {
            return Element == nullptr;
        });
	}
}

template <typename T>
void UReadyOrNotFunctionLibrary::RemoveAllNullElements(TArray<T>& Array)
{
	if (Array.Num() > 0)
	{
		Array.RemoveAll([](T Element)
	    {
	        return Element == nullptr;
	    });
	}
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors)
{
	if (InActors.Num() == 0)
		return nullptr;

	if (InActors.Num() == 1)
		return InActors[0];
	
	float ClosestDistance = FLT_MAX;
	T* ClosestActor = nullptr;
	for (T* Actor : InActors)
	{
		if (Actor)
		{
			const float CurrentDistance = (InTestLocation - Actor->GetActorLocation()).SizeSquared();
			if (CurrentDistance < ClosestDistance)
			{
				ClosestDistance = CurrentDistance;
				ClosestActor = Actor;
			}
		}
	}

	return ClosestActor;
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors)
{
	if (InActors.Num() == 0)
		return nullptr;

	if (InActors.Num() == 1)
		return InActors[0];
	
	float FurthestDistance = 0.0f;
	T* FurthestActor = nullptr;
	for (T* Actor : InActors)
	{
		if (Actor)
		{
			const float CurrentDistance = (InTestLocation - Actor->GetActorLocation()).SizeSquared();
			if (CurrentDistance > FurthestDistance)
			{
				FurthestDistance = CurrentDistance;
				FurthestActor = Actor;
			}
		}
	}

	return FurthestActor;
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation(const FVector& InTestLocation, const TArray<T*>& InActors, TFunctionRef<bool(const T*, const float)> Predicate)
{
	if (InActors.Num() == 0)
		return nullptr;

	if (InActors.Num() == 1)
		return InActors[0];
	
	float FurthestDistance = 0.0f;
	T* FurthestActor = nullptr;
	for (T* Actor : InActors)
	{
		if (Actor)
		{
			const float CurrentDistance = FVector::Distance(InTestLocation, Actor->GetActorLocation());
			if (CurrentDistance > FurthestDistance && Predicate(Actor, CurrentDistance))
			{
				FurthestDistance = CurrentDistance;
				FurthestActor = Actor;
			}
		}
	}

	return FurthestActor;
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindClosestActor(UWorld* WorldContext, const FVector& FromLocation, float MaxDistance, TFunctionRef<bool(const T*, const float)> Predicate)
{
	static_assert(TIsDerivedFrom<T, AActor>::Value, "T must be derived from AActor");

	if (!WorldContext)
		return nullptr;

	float ClosestDistance = MaxDistance;
	T* ClosestActor = nullptr;
	
	for (TActorIterator<T> It(WorldContext); It; ++It)
	{
		T* Actor = *It;
		if (Actor)
		{
			const float Distance = FVector::Distance(FromLocation, Actor->GetActorLocation());

			if (Distance < ClosestDistance && Predicate(Actor, Distance))
			{
				ClosestDistance = Distance;
				ClosestActor = Actor;
			}
		}
	}

	return ClosestActor;
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindClosestActor(UWorld* WorldContext, const FVector& FromLocation, TFunctionRef<bool(const T*, const float)> Predicate)
{
	return FindClosestActor<T>(WorldContext, FromLocation, FLT_MAX, Predicate);
}

template <typename T>
T* UReadyOrNotFunctionLibrary::FindFurthestActor(UWorld* WorldContext, const FVector& FromLocation, TFunctionRef<bool(const T*, const float)> Predicate)
{
	static_assert(TIsDerivedFrom<T, AActor>::Value, "T must be derived from AActor");

	if (!WorldContext)
		return nullptr;

	float FurthestDistance = 0.0f;
	T* FurthestActor = nullptr;
	
	for (TActorIterator<T> It(WorldContext); It; ++It)
	{
		T* Actor = *It;
		if (Actor)
		{
			const float Distance = FVector::Distance(FromLocation, Actor->GetActorLocation());

			if (Distance > FurthestDistance && Predicate(Actor, Distance))
			{
				FurthestDistance = Distance;
				FurthestActor = Actor;
			}
		}
	}

	return FurthestActor;
}	
