// Void Interactive, 2020

#include "BadAIAction.h"

#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "lib/ReadyOrNotFunctionLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/ConstructorHelpers.h"

ABadAIAction::ABadAIAction()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
	
	bIsEditorOnlyActor = false;
	bFindCameraComponentWhenViewTarget = false;

	bRemoveReportOnDestroy = true;

	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UTexture2D> BadAIActionIcon(TEXT("Texture2D'/Game/_23__ALI_MAP._23__ALI_MAP'"));
	
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->ScreenSize = 0.0025f;
	BillboardComponent->SetSprite(BadAIActionIcon.Object);
	SetRootComponent(BillboardComponent);
}

void ABadAIAction::Destroyed()
{
	if (bRemoveReportOnDestroy) 
		RemoveReport(false, false);

	Super::Destroyed();
}

void ABadAIAction::PostActorCreated()
{
	LastLocation = GetActorLocation();

	Super::PostActorCreated();
}

#if WITH_EDITOR
void ABadAIAction::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	Report();
}
#endif

void ABadAIAction::ReportBadAIAction()
{
	Report();
}

void ABadAIAction::RemoveBadAIAction()
{
	RemoveReport();
}

void ABadAIAction::AddNote(const FText& InSummary, const FText& InDescription)
{
	if (InSummary.IsEmpty())
		return;

	Summary = InSummary;

	if (!InDescription.IsEmpty())
		Description = InDescription;

	SavedReportString = "ID:" + FString::FromInt(ID) + "," + "(" + GetActorLocation().ToString() + ")" + "," + Summary.ToString() + "," + Description.ToString();
}

void ABadAIAction::Report(const bool bReportToLog, const bool bDrawString)
{
	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, true);
	
	const FString INIFilePath = FPaths::ProjectConfigDir() + "BadAIActions.ini";

	GConfig->LoadFile(INIFilePath);
	//GConfig->Flush(true, INIFilePath);

	TArray<FString> BadLocationEntries = UReadyOrNotFunctionLibrary::GetStringArrayFromINIFile(INIFilePath, MapName, "BadLocation");

	// Output = BadLocation=ID:0,(X=0.0 Y=0.0 Z=0.0),"Summary","Description"

	if (ID < 0)
		ID = BadLocationEntries.Num();
	
	SavedReportString = "ID:" + FString::FromInt(ID) + "," + "(" + GetActorLocation().ToString() + ")" + "," + Summary.ToString() + "," + Description.ToString();

	bool bModifyingExistingEntry = false;
	for (FString& BadLocationEntry : BadLocationEntries)
	{
		//if (const int32 FoundIDIndex = BadLocationEntry.Find("ID:" + FString::FromInt(ID)) != INDEX_NONE)
		if (const int32 FoundIDIndex = BadLocationEntry.Find("(" + GetActorLocation().ToString() + ")") != INDEX_NONE)
		{
			//ID = FoundIDIndex;
			
			bModifyingExistingEntry = true;
			BadLocationEntry = SavedReportString;
		}
	}

	if (!bModifyingExistingEntry)
	{
		BadLocationEntries.AddUnique(SavedReportString);
	}

	LastLocation = GetActorLocation();

	GConfig->EnableFileOperations();
	GConfig->SetArray(*MapName, TEXT("BadLocation"), BadLocationEntries, INIFilePath);
	GConfig->Flush(false, INIFilePath);

	if (bDrawString)
	{
		DrawDebugString(GetWorld(), LastLocation, "Bad AI Action Reported at (" + LastLocation.ToString() + ")", nullptr, FColor::White, 5.0f, false, 1.0f);
	}
	
	if (bReportToLog)
	{
		ULog::Info("Bad AI Action Reported at (" + LastLocation.ToString() + ")");
	}
}

void ABadAIAction::RemoveReport(const bool bReportToLog, const bool bDrawString)
{
	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, true);

	const FString INIFilePath = FPaths::ProjectConfigDir() + "BadAIActions.ini";

	GConfig->LoadFile(INIFilePath);
	//GConfig->Flush(true, INIFilePath);

	TArray<FString> BadLocationEntries = UReadyOrNotFunctionLibrary::GetStringArrayFromINIFile(INIFilePath, MapName, "BadLocation");
	if (BadLocationEntries.Num() > 0)
	{
		if (BadLocationEntries.IsValidIndex(ID))
			BadLocationEntries.RemoveAt(ID);
		
		BadLocationEntries.Remove(SavedReportString);
		BadLocationEntries.Remove("(" + GetActorLocation().ToString() + ")");

		const int32 FoundEntry = BadLocationEntries.Find("(" + GetActorLocation().ToString() + ")");
		if (FoundEntry > -1)
			BadLocationEntries.RemoveAt(FoundEntry);
	}

	GConfig->EnableFileOperations();
	GConfig->SetArray(*MapName, TEXT("BadLocation"), BadLocationEntries, INIFilePath);
	GConfig->Flush(false, INIFilePath);

	#if WITH_EDITOR
	GEditor->SelectActor(this, false, true, true, true);
	#endif
	
	if (bDrawString)
	{
		DrawDebugString(GetWorld(), GetActorLocation(), "Bad AI Action Removed at (" + GetActorLocation().ToString() + ")", nullptr, FColor::White, 5.0f, false, 1.0f);
	}

	if (bReportToLog)
	{
		ULog::Info("Bad AI Action Removed at (" + GetActorLocation().ToString() + ")");
	}

	Destroy();
}
