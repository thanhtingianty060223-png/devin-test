// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "TeamPaperdollWidget.generated.h"

/**
 * 
 */
UCLASS(meta=(DisableNativeTick))
class READYORNOT_API UTeamPaperdollWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Team Paperdoll")
	void InitializeWidget(ETeamType InTeam);

protected:
	virtual void InitializeWidget_Implementation(ETeamType InTeam);

	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Required Widgets", meta = (BindWidget))
	class UImage* Paperdoll_Image = nullptr;

private:
	void UpdatePaperdollColor(const FLinearColor& Color);
};
