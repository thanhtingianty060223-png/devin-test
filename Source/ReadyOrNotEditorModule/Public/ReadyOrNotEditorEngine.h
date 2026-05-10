// Copyright Void Interactive, 2023

#pragma once

#include "Editor/UnrealEdEngine.h"
#include "ReadyOrNotEditorEngine.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOTEDITORMODULE_API UReadyOrNotEditorEngine : public UUnrealEdEngine
{
	GENERATED_BODY()

	virtual void Init(IEngineLoop* InEngineLoop) override;

	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;

	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void OnEndPIE(bool bIsSimulating);

	void SpawnBadAIActionActors(bool bFixDuplicateIDs = true);

	// virtual void OnPreSaveWorld(uint32 SaveFlags, UWorld* World);

	virtual bool LoadMap(FWorldContext& WorldContext, FURL URL, UPendingNetGame* Pending, FString& Error) override;

	void TryDestroyDuplicateWorldSettings();
	
	bool bHasMovedScreenToSecondMonitor = false;
};
