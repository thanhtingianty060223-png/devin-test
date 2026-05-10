// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "BadAIAction.generated.h"

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ABadAIAction : public AActor
{
	GENERATED_BODY()
	
public:
	ABadAIAction();

	UFUNCTION(BlueprintCallable, Category = "Report")
	void Report(bool bReportToLog = true, bool bDrawString = true);
	
	UFUNCTION(BlueprintCallable, Category = "Report")
	void RemoveReport(bool bReportToLog = true, bool bDrawString = true);
	
	UFUNCTION(BlueprintCallable, Category = "Note")
	void AddNote(const FText& InSummary, const FText& InDescription = INVTEXT("Empty"));

	// A short summary of what the AI was doing here
	UPROPERTY(EditInstanceOnly, Category = "Note")
	FText Summary = FText::FromString("Empty");
	
	// A detailed description of the events leading up to the bad action and what the AI was doing here
	UPROPERTY(EditInstanceOnly, Category = "Note", meta = (MultiLine = true))
	FText Description = FText::FromString("Empty");
	
	int32 ID = -1;

	uint8 bRemoveReportOnDestroy : 1;

protected:
	virtual void Destroyed() override;

	virtual void PostActorCreated() override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UFUNCTION(CallInEditor, Category = "Editor")
	void ReportBadAIAction();
	
	UFUNCTION(CallInEditor, Category = "Editor")
	void RemoveBadAIAction();

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;

	FVector LastLocation = FVector::ZeroVector;
	FString SavedReportString;
};


