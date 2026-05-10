// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UObject/Object.h"
#include "LoadoutVerticalItemListWidget.generated.h"

UCLASS()
class READYORNOT_API ULoadoutVerticalItemListWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadoutSlotInteraction, ULoadoutSlotWidget*, TriggeringSlot);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadoutSlotAttachmentInteraction, ULoadoutSlotAttachmentWidget*, AttachmentSlot);

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotInteraction OnOverviewItemHovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotInteraction OnOverviewItemUnhovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotAttachmentInteraction OnAttachmentSlotHovered;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FLoadoutSlotAttachmentInteraction OnAttachmentSlotUnhovered;
};
