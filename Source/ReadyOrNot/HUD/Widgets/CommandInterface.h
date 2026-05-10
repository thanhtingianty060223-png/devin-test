#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"

#include "CommandInterface.generated.h"

/**
 *
 */
UCLASS()
class READYORNOT_API UCommandInterface : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void EnableSelectionWidgets(FVector2D Selection);

	UFUNCTION(BlueprintCallable)
	void DisableSelectionWidgets();

	UFUNCTION(BlueprintCallable)
	void SetRedColor();

	UFUNCTION(BlueprintCallable)
	void SetBlueColor();

	UFUNCTION(BlueprintCallable)
	void SetGoldColor();

	UPROPERTY(BlueprintReadWrite)
	UImage* SelectionLeft;

	UPROPERTY(BlueprintReadWrite)
	UImage* SelectionRight;

	UPROPERTY(BlueprintReadWrite)
	UImage* SelectionUp;

	UPROPERTY(BlueprintReadWrite)
	UImage* SelectionDown;

	UPROPERTY(BlueprintReadWrite)
	UImage* MoveIn;

	UPROPERTY(BlueprintReadWrite)
	UImage* FallIn;

	UPROPERTY(BlueprintReadWrite)
	UImage* Cover;

	UPROPERTY(BlueprintReadWrite)
	UImage* Deploy;

private:
	void SetColor(FLinearColor Color);
};
