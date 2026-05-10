// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LoadoutOverviewWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ULoadoutOverviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadoutSlotInteraction, ULoadoutSlotWidget*, TriggeringSlot);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadoutSlotAttachmentInteraction, ULoadoutSlotAttachmentWidget*, AttachmentSlot);

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotInteraction OnOverviewItemClicked;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotInteraction OnOverviewItemHovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotInteraction OnOverviewItemUnhovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotAttachmentInteraction OnAttachmentSlotClicked;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotAttachmentInteraction OnAttachmentSlotHovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotAttachmentInteraction OnAttachmentSlotUnhovered;

	UFUNCTION()
	void InitializeOverviewList(bool bRemotePlayer);
};
