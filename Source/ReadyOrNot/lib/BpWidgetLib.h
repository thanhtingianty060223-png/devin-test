// Copyright Void Interactive, 2023

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UMG/Public/Blueprint/UserWidget.h"
#include "BpWidgetLib.generated.h"

/**
*
*/
UCLASS()
class READYORNOT_API UBpWidgetLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Like Epic's DrawLine, but this one has the THICCNESS
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "4"), Category = "Painting")
	static void DrawLineWithThickness(UPARAM(ref) FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, FLinearColor Tint = FLinearColor::White, bool bAntiAlias = true, float Thickness = 1.0f, FVector2D Offset = FVector2D::ZeroVector);

	// Like Epic's DrawLines, but this one has the THICCNESS
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "3"), Category = "Painting")
	static void DrawLinesWithThickness(UPARAM(ref) FPaintContext& Context, const TArray<FVector2D>& Points, FLinearColor Tint = FLinearColor::White, bool bAntiAlias = true, float Thickness = 1.0f, FVector2D Offset = FVector2D::ZeroVector);

	// Draw line with centered offset
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "4"), Category = "Painting")
	static void DrawLineWithCenteredOffset(UPARAM(ref) FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, 
		FLinearColor Tint = FLinearColor::White, bool bAntiAlias = true, float Thickness = 1.0f, 
		FVector2D Offset = FVector2D::ZeroVector,
		FVector2D Center = FVector2D::ZeroVector, float Scale = 1.0f);

	// Like Epic's DrawLines, but this one has the THICCNESS
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "3"), Category = "Painting")
		static void DrawLinesWithCenteredOffset(UPARAM(ref) FPaintContext& Context, const TArray<FVector2D>& Points, 
			FLinearColor Tint = FLinearColor::White, bool bAntiAlias = true, float Thickness = 1.0f, 
			FVector2D Offset = FVector2D::ZeroVector,
			FVector2D Center = FVector2D::ZeroVector, float Scale = 1.0f);

	// Little stub for Ryan to put in later --eez
	UFUNCTION(BlueprintCallable, Category = "VOID Interactive|Mantis Bugtracker")
	static bool PostBugReport(FString Summary, FString Description, FString Category);

	// Simple helper function to get another text by key from the same string table as target, returns target if the lookup failed
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FText ChangeStringTableTextKey(const FText Target, const FString& NewKey);

	// Converts a click into normalized (0.0-1.0) coordinates on the specified widget geometry
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FVector2D GetNormalizedClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);

	// Checks if this widget belongs to a world, and if it does whether or not it is tearing down
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool IsWorldTearingDown(const UObject* WorldContextObject);

	// Gets the local player's tablet, if available
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static class ATablet* GetPlayerTablet(const UObject* WorldContextObject);

	// Attempt to play a sound from the player's tablet, if it is available and active
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void PlayEventFromTablet(const UObject* WorldContextObject, class UFMODEvent* Event);
};
