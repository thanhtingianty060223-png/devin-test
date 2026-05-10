// Copyright Void Interactive, 2023

#include "ReadyOrNotEditorEngine.h"

#include "Debug/BadAIAction.h"

#include "Editor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "EngineUtils.h"
#include "ISourceControlModule.h"
#include "ReadyOrNotNavigationSystem.h"
#include "AI/NavigationSystemConfig.h"

void UReadyOrNotEditorEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
	
	// Move window to the corresponding monitor

	FEditorDelegates::OnMapOpened.AddUObject(this, &UReadyOrNotEditorEngine::OnMapOpened);
	FEditorDelegates::EndPIE.AddUObject(this, &UReadyOrNotEditorEngine::OnEndPIE);

	// Register state branches
	const ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	if (SourceControlModule.IsEnabled())
	{
		ISourceControlProvider& SourceControlProvider = SourceControlModule.GetProvider();
		// Order matters. Lower values are lower in the hierarchy, i.e., changes from higher branches get automatically merged down.
		// (Automatic merging requires an appropriately configured CI pipeline)
		// With this paradigm, the higher the branch is, the stabler it is, and has changes manually promoted up.
		const TArray<FString> Branches
		{
			"//ReadyOrNot/main",
			"//ReadyOrNot/experimental"
		};
		SourceControlProvider.RegisterStateBranches(Branches, TEXT("Content"));
	}
}

void UReadyOrNotEditorEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);
}

void UReadyOrNotEditorEngine::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	TryDestroyDuplicateWorldSettings();
	
	ULevel* LevelObject = GWorld->GetCurrentLevel();
	if (LevelObject && LevelObject->GetWorld() && LevelObject->GetWorldSettings(false))
	{
		LevelObject->GetWorldSettings(false)->GetNavigationSystemConfig()->NavigationSystemClass = UReadyOrNotNavigationSystem::StaticClass();
	}

	SpawnBadAIActionActors();
}

void UReadyOrNotEditorEngine::OnEndPIE(const bool bIsSimulating)
{
	if (!bIsSimulating)
		SpawnBadAIActionActors();
	
	TryDestroyDuplicateWorldSettings();
}

void UReadyOrNotEditorEngine::SpawnBadAIActionActors(const bool bFixDuplicateIDs)
{
	for (TActorIterator<ABadAIAction> It(GWorld); It; ++It)
	{
		ABadAIAction* Actor = *It;
		Actor->bRemoveReportOnDestroy = false;
		Actor->Destroy(true);
	}
	
	ULevel* LevelObject = GWorld->GetCurrentLevel();
	if (!LevelObject || !LevelObject->GetWorld())
		return;
	
	const FString LevelName = LevelObject->GetWorld()->GetName();

	const FString INIFilePath = FPaths::ProjectConfigDir() + "BadAIActions.ini";

	GConfig->LoadFile(INIFilePath);
	GConfig->Flush(true, INIFilePath);

	auto GetStringArrayFromINIFile = [&](FString ConfigFilePath, FString Section, FString Key)
	{
		TArray<FString> ConfigStringArray;
		GConfig->GetArray(*Section, *Key, ConfigStringArray, ConfigFilePath);
		
		for (int32 i = 0; i < ConfigStringArray.Num(); i++)
		{
			ConfigStringArray[i].RemoveFromEnd(TEXT(","));
		}

		return ConfigStringArray;
	};
	
	TArray<FString> BadLocationEntries = GetStringArrayFromINIFile(INIFilePath, LevelName, "BadLocation");
	int32 i = 0;
	for (FString& BadLocationEntry : BadLocationEntries)
	{
		if (!BadLocationEntry.IsEmpty())
		{			
			TArray<FString> BadLocationInfo;
			BadLocationEntry.ParseIntoArray(BadLocationInfo, TEXT(","));
				
			if (BadLocationInfo.Num() > 3)
			{
				if (bFixDuplicateIDs)
				{
					FString IDReplacementString = "ID:" + FString::FromInt(i);
					BadLocationEntry.ReplaceInline(*BadLocationInfo[0], *IDReplacementString);
				}
				
				FVector SpawnLocation;
				SpawnLocation.InitFromString(BadLocationInfo[1]);

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.ObjectFlags = RF_Transient | RF_DuplicateTransient;

				if (ABadAIAction* BadAIAction = GWorld->SpawnActor<ABadAIAction>(ABadAIAction::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters))
				{
					if (bFixDuplicateIDs)
						BadAIAction->ID = i;

					BadAIAction->AddNote(FText::FromString(BadLocationInfo[2]), FText::FromString(BadLocationInfo[3]));
					//BadAIAction->Summary = FText::FromString(BadLocationInfo[2]);
					//BadAIAction->Description = FText::FromString(BadLocationInfo[3]);
				}
			}

			i++;
		}
	}

	if (bFixDuplicateIDs)
	{
		GConfig->EnableFileOperations();
		GConfig->SetArray(*LevelName, TEXT("BadLocation"), BadLocationEntries, INIFilePath);
		GConfig->Flush(false, INIFilePath);
	}
}

// void UReadyOrNotEditorEngine::OnPreSaveWorld(uint32 SaveFlags, UWorld* World)
// {
// 	Super::OnPreSaveWorld(SaveFlags, World);
// 
// 	TryDestroyDuplicateWorldSettings();
// }

bool UReadyOrNotEditorEngine::LoadMap(FWorldContext& WorldContext, FURL URL, UPendingNetGame* Pending, FString& Error)
{
	const bool bResult = Super::LoadMap(WorldContext, URL, Pending, Error);

	TryDestroyDuplicateWorldSettings();
	
	return bResult;
}

void UReadyOrNotEditorEngine::TryDestroyDuplicateWorldSettings()
{
	TArray<AActor*> Settings;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldSettings::StaticClass(), Settings);

	for (int32 i = 1; i < Settings.Num(); i++)
	{
		if (AWorldSettings* s = Cast<AWorldSettings>(Settings[i]))
		{
			s->Destroy();
		}
	}
}
