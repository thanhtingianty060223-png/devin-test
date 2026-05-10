// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "TeamStatusWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UTeamStatusWidget : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UTeamStatusWidget();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Team Status Widget")
	void InitializeTeam();

protected:
	void NativePreConstruct() override;
	void NativeConstruct() override;
	
	virtual void InitializeTeam_Implementation();

	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Required Widgets", meta = (BindWidget))
	class UImage* TeamEmblem_Image_LeftAligned = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Required Widgets", meta = (BindWidget))
	class UHorizontalBox* Teammates_Container_LeftAligned = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Required Widgets", meta = (BindWidget))
	class UImage* TeamEmblem_Image_RightAligned = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Required Widgets", meta = (BindWidget))
	class UHorizontalBox* Teammates_Container_RightAligned = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties", meta = (ExposeOnSpawn = true))
	TEnumAsByte<EHorizontalAlignment> Alignment = HAlign_Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties", meta = (ExposeOnSpawn = true))
	FSlateBrush TeamEmblemBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties", meta = (ExposeOnSpawn = true))
	ETeamType Team = ETeamType::TT_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties", meta = (ExposeOnSpawn = true))
	TSubclassOf<class UTeamPaperdollWidget> PaperdollWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties|Empty Team Text", meta = (ExposeOnSpawn = true))
	FText EmptyTeamText = FText::FromString("Nobody On Team");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties|Empty Team Text", meta = (ExposeOnSpawn = true))
	FSlateColor EmptyTeamTextColor = FSlateColor(FColor(255, 255, 255, 127));
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Status Widget|Exposed Properties|Empty Team Text", meta = (ExposeOnSpawn = true))
	FSlateFontInfo EmptyTeamTextFont;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Data")
	TArray<class UTeamPaperdollWidget*> TeamPaperdolls;
	
	UPROPERTY(BlueprintReadOnly, Category = "Team Status Widget|Data")
	class UTextBlock* EmptyTeam_Text = nullptr;

private:
	class UTextBlock* CreateEmptyTeamText();
	
	void UpdateTeamEmblemImage(const FSlateBrush& Brush);
	
	void UpdateTeamStatus();
};
