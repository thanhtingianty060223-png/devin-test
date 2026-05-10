// Void Interactive, 2020

#include "MapActorComponent.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "HUD/Widgets/MapActorIconWidget.h"

UMapActorComponent::UMapActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.033f;

	bEnabled = true;
	bUseActorRotation = true;
	bAddedToMap = false;
	bCondition = true;

}

void UMapActorComponent::InitializeMapActorSettings(const FSlateBrush& InIconBrush, const FLinearColor& InIconColor, const FText& InIconText, const FLinearColor& InIconTextColor)
{
	IconBrush = InIconBrush;
	IconColor = InIconColor;
	IconText = InIconText;
	IconTextColor = InIconTextColor;
}

void UMapActorComponent::SetIconText(const FText& InIconText)
{
	IconText = InIconText;

	if (MapIconWidget)
		MapIconWidget->SetMapActorText(InIconText);
}

void UMapActorComponent::SetIconTextColor(const FLinearColor& InIconTextColor)
{
	IconTextColor = InIconTextColor;

	if (MapIconWidget)
		MapIconWidget->SetMapActorTextColor(InIconTextColor);
}

void UMapActorComponent::SetIconColor(const FLinearColor& InIconColor)
{
	IconColor = InIconColor;
	
	if (MapIconWidget)
		MapIconWidget->SetIconColor(InIconColor);
}

void UMapActorComponent::EnableMapActor()
{
	bEnabled = true;
}

void UMapActorComponent::DisableMapActor()
{
	bEnabled = false;
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		RemoveFromMap(PlayerCharacter->HumanCharacterWidget_V2);
	}
}

void UMapActorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!MapActorWidgetClass)
	{
		MapActorWidgetClass = LoadObject<UClass>(nullptr, TEXT("WidgetBlueprint'/Game/Blueprints/Widgets/HUD/Tablet/W_MapActorIcon.W_MapActorIcon_C'"));;
	}
}

void UMapActorComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bEnabled && !bAddedToMap)
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			AddToMap(PlayerCharacter->HumanCharacterWidget_V2);

			if (MapIconWidget)
			{
				MapIconWidget->SetVisibility(bCondition ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			}
		}
	}
}

void UMapActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DisableMapActor();
}

void UMapActorComponent::AddToMap(UHumanCharacterHUD_V2* HUD)
{
	if (!bEnabled || bAddedToMap)
		return;
	
	if (!HUD)
		return;

	if (!HUD->IsInViewport())
		return;

	MapIconWidget = Cast<UMapActorIconWidget>(HUD->AddMapActor(this, MapActorWidgetClass, IconBrush, IconColor, IconTextColor, RotationOffset));

	bAddedToMap = true;
}

void UMapActorComponent::RemoveFromMap(UHumanCharacterHUD_V2* HUD)
{
	if (!HUD)
		return;

	if (!HUD->IsInViewport())
		return;

	HUD->RemoveMapActor(this);

	MapIconWidget = nullptr;

	bAddedToMap = false;
}
