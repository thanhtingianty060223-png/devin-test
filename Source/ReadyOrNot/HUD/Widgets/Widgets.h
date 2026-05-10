// Copyright Void Interactive, 2017

#pragma once

#include "BaseWidget.h"
#include "Enums.h"
#include "Widgets.generated.h"

UENUM(BlueprintType)
enum class EMapType : uint8
{
	Axis 	UMETA(DisplayName = "Axis Mapping"),
	Action	UMETA(DisplayName = "Action Mapping"),
	Auto	UMETA(DisplayName = "Auto")
};

UENUM(BlueprintType)
enum class EPlayerObjectiveMarkerType : uint8
{
	POMT_None,			// No objective marker type
	POMT_VipRescue,		// rescue the VIP
	POMT_VipExecute,	// Execute the VIP
	POMT_Free,			// free this player
};

UENUM(BlueprintType)
enum class ECompletedActionType : uint8
{
	CAT_ArrestedTarget,
	CAT_ReportedTarget,
	CAT_FreedTarget,
	CAT_LockPicked,
	CAT_LadderRetracted,
	CAT_ItemRetrieved,
	CAT_MagInserted,
	CAT_MagRemoved,
	CAT_MagChanged,
	CAT_InteractActor,
	CAT_WedgeDeployed,
	CAT_C2Deployed,
	CAT_WedgeRemoved,
	CAT_TrapDisarmed,
	CAT_C2Removed,
};

USTRUCT(BlueprintType)
struct FInputEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Bind;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey DefaultKeyBind1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey DefaultKeyBind2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EMapType MappingType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AxisScale = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDisplayOnScreen;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<class ABaseItem> ShowWithEquippedItem;
};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API USpectatorCharacterHUD : public UBaseWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnNewCharacterViewed(class AReadyOrNotCharacter* NewViewTarget);

	UFUNCTION(BlueprintImplementableEvent)
		void ChatPressed();

	UFUNCTION(BlueprintImplementableEvent)
		void TeamChatPressed();

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
		void CenterPrint(FName MessageType, float Duration, class APlayerCharacter* Other);
};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UObjectiveWidget : public UBaseWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Objective")
		class UWidgetComponent* OwningComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Objective")
		ETeamType ObjectiveTeam = ETeamType::TT_NONE;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Objective")
		void SetObjectiveText(const FText& NewText);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Objective")
		void SetObjectiveType(EPlayerObjectiveMarkerType NewType);
};
