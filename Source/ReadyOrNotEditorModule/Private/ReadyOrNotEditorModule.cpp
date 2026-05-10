// Copyright Void Interactive, 2023

#include "ReadyOrNotEditorModule.h"

DEFINE_LOG_CATEGORY(ReadyOrNotEditorModule);

#define LOCTEXT_NAMESPACE "FReadyOrNotEditorModule"

void FReadyOrNotEditorModule::StartupModule()
{
	UE_LOG(ReadyOrNotEditorModule, Warning, TEXT("Ready Or Not Editor module has started!"));
}

void FReadyOrNotEditorModule::ShutdownModule()
{
	UE_LOG(ReadyOrNotEditorModule, Warning, TEXT("Ready Or Not Editor module has shut down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FReadyOrNotEditorModule,ReadyOrNotEditorModule)
