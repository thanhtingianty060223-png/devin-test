// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "SwatTeamStatusWidget.generated.h"

DECLARE_STATS_GROUP(TEXT("Swat Tean Status Widget"), STATGROUP_SwatTeamStatusWidget, STATCAT_Advanced);

UCLASS()
class READYORNOT_API USwatTeamStatusWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
protected:
	void UpdateSwatStatusPlayerNameFromSquadPosition(class USwatCommandStatusWidget* StatusWidget, ESquadPosition Position);
	void UpdateSwatStatusPlayerHealth(class USwatCommandStatusWidget* StatusWidget, ESquadPosition Position);
	void UpdateSwatStatusCommandName(class USwatCommandStatusWidget* StatusWidget, class UBaseActivity*& SquadActivity, ESquadPosition Position);

	UFUNCTION()
	void OnDefaultCommandIssued(APlayerCharacter* Issuer, ESwatCommand CommandIssued);

	UFUNCTION()
	void UpdateStatus();
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UVerticalBox* SWAT_Status_Container = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandStatusWidget* SWAT_Alpha_Status = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandStatusWidget* SWAT_Beta_Status = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandStatusWidget* SWAT_Charlie_Status = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandStatusWidget* SWAT_Delta_Status = nullptr;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class USwatCommandStatusWidget* SWAT_Lead_Status = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	class UBaseActivity* AlphaActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	class UBaseActivity* BetaActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	class UBaseActivity* CharlieActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	class UBaseActivity* DeltaActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	class UBaseActivity* LeadActivity = nullptr;

	UPROPERTY(Transient)
	TMap<ESquadPosition, ASWATCharacter*> InitialSquadMap;

	ASWATCharacter* GetInitialCharacterFromPosition(ESquadPosition Position);
	void HideDuplicateCommandStatus(class UBaseActivity*& ActivityA, class UBaseActivity*& ActivityB, class USwatCommandStatusWidget*& SwatWidgetToHide);
	bool IsSwatDead(ESquadPosition Position);
};
