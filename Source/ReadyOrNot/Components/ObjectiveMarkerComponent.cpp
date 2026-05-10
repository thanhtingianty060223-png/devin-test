// Void Interactive, 2020

#include "ObjectiveMarkerComponent.h"

#include "HUD/Widgets/ObjectiveMarkerWidget.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"

#include "lib/ReadyOrNotMathLibrary.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#include "ReadyOrNotDebugSubsystem.h"

#include "Components/CanvasPanel.h"

#include "Blueprint/WidgetTree.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Tick"), STAT_ObjectiveMarkerTick, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Enabled Checks"), STAT_ObjectiveMarkerEnabledChecks, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Calculate Screen Position"), STAT_CalculateWidgetScreenPosition, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Offscreen Widget Update"), STAT_OffscreenWidgetUpdate, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Onscreen Widget Update"), STAT_OnscreenWidgetUpdate, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Widget Overlap Checks"), STAT_WidgetOverlapChecks, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Widget Obstruction Check"), STAT_WidgetObstructionCheck, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Widget Opacity Update"), STAT_WidgetOpacityUpdate, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Destroy Objective Marker"), STAT_DestroyObjectiveMarker, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Show Objective Marker"), STAT_ShowObjectiveMarker, STATGROUP_ObjectiveMarkerComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Objective Marker Comp ~ Hide Objective Marker"), STAT_HideObjectiveMarker, STATGROUP_ObjectiveMarkerComp);

float UObjectiveMarkerComponent::FadeSpeed = 10.0f;

UObjectiveMarkerComponent::UObjectiveMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.0167f;

	SetTickableWhenPaused(false);
	SetTickWhenOffscreen(false);
	SetRedrawTime(0.033f);
	SetCanEverAffectNavigation(false);

	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(128.0f, 128.0f));
	SetPivot(FVector2D::ZeroVector);

	UPrimitiveComponent::SetCollisionResponseToAllChannels(ECR_Ignore);
	UPrimitiveComponent::SetNotifyRigidBodyCollision(false);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bFillCollisionUnderneathForNavmesh = false;

	bTickInEditor = false;

	//static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultMarkerIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD_Revised/UnderConstruction_Icon_256.UnderConstruction_Icon_256'"));
	IconBrush.SetResourceObject(nullptr);

	WidgetClass = MarkerWidgetClass;
	
	bEnabled = true;
	bCustomLocation = false;
	bStartHidden = false;
	bFadeOffscreen = true;
	bCompletelyFadeWhenOverlappingOtherWidgets = false;
	bCompletelyFadeWhenClose = true;
	bDynamic = false;
	bDistanceScaleIcon = true;
	bHideIconOffscreen = true;
	
	bIsOffscreen = false;

	bHUDVisible = false;
}

void UObjectiveMarkerComponent::InitMarkerSettings(const FSlateBrush& InBrush, const FLinearColor& InIconColorAndOpacity)
{
	IconBrush = InBrush;
	IconColorAndOpacity = InIconColorAndOpacity;

	if (ObjectiveMarkerWidget_Offscreen)
	{
		ObjectiveMarkerWidget_Offscreen->SetIconImage(IconBrush);
		ObjectiveMarkerWidget_Offscreen->SetIconColorAndOpacity(IconColorAndOpacity);
	}
	
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetIconImage(IconBrush);
		ObjectiveMarkerWidget_Onscreen->SetIconColorAndOpacity(IconColorAndOpacity);
	}
}

void UObjectiveMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!MarkerWidgetClass)
	{
		MarkerWidgetClass = LoadObject<UClass>(nullptr, TEXT("WidgetBlueprint'/Game/Blueprints/Widgets/HUD/W_ObjectiveMarker.W_ObjectiveMarker_C'"));
	}
	
	if (bEnabled)
	{
		CreateObjectiveMarkerWidget();
	}
	else
	{
		HideObjectiveMarker();

		SetComponentTickInterval(1.0f);
	}
}

void UObjectiveMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	switch (EndPlayReason)
	{
		case EEndPlayReason::Destroyed:
			DestroyObjectiveMarkerWidget();
		break;

		case EEndPlayReason::LevelTransition:
			DestroyObjectiveMarkerWidget();
		break;

		case EEndPlayReason::EndPlayInEditor:
		break;

		case EEndPlayReason::RemovedFromWorld:
			DestroyObjectiveMarkerWidget();
		break;

		case EEndPlayReason::Quit:
		break;

		default: ;
	}
}

void UObjectiveMarkerComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_ObjectiveMarkerTick);
	
	if (bEnabled)
	{
		SCOPE_CYCLE_COUNTER(STAT_ObjectiveMarkerEnabledChecks);
		
		SetComponentTickInterval(0.0f);

		#if WITH_EDITOR
		if (bDebug)
		{
			ULog::Bool(CanShowObjectiveMarker(), "CanShowObjectiveMarker: ");
			ULog::Bool(bFirstTick, "bFirstTick: ");
			ULog::Bool(bStartHidden, "bStartHidden: ");
		}
		#endif
		
		if (!CanShowObjectiveMarker())
		{
			HideObjectiveMarker();
			bFirstTick = false;
			return;
		}

		if (!bFirstTick)
		{
			bStartHidden ? HideObjectiveMarker() : ShowObjectiveMarker();
		}
		
		bFirstTick = true;

		if (GetOwner() && ObjectiveMarkerWidget_Offscreen && ObjectiveMarkerWidget_Onscreen)
		{
			CurrentMarkerLocation = bCustomLocation ? GetComponentLocation() : GetOwner()->GetActorLocation();
			
			if (ABaseItem* OwnerAsItem = Cast<ABaseItem>(GetOwner()))
			{
				CurrentMarkerLocation = bCustomLocation ? GetComponentLocation() : OwnerAsItem->GetItemLocation();
			}

			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
			if (!PlayerPawn || !PlayerPawn->GetController())
			{
				HideObjectiveMarker(false);
				return;
			}

			const float DistanceToLocalPlayer = FVector::Distance(CurrentMarkerLocation, PlayerPawn->GetActorLocation());
			
			// If over 10km in distance from local player, destroy objective marker
			if (DistanceToLocalPlayer > 1000000.0f)
			{
				DestroyObjectiveMarkerWidget();

				DestroyComponent();

				return;
			}
			
			FVector2D ScreenPosition;
			float ForwardDotProduct;
			{
				SCOPE_CYCLE_COUNTER(STAT_CalculateWidgetScreenPosition)
				
				float Angle;
				float RightDotProduct;
				UReadyOrNotFunctionLibrary::CalculateOffscreenPositionFromWorldLocation_Square(this, CurrentMarkerLocation, 60.0f, bIsOffscreen, Angle, ForwardDotProduct, RightDotProduct);

				if (bIsOffscreen)
				{
					SCOPE_CYCLE_COUNTER(STAT_OffscreenWidgetUpdate)
					
					ObjectiveMarkerWidget_Offscreen->SetPositionInViewport(ScreenPosition, true);
					ObjectiveMarkerWidget_Offscreen->SetTargetLocation(CurrentMarkerLocation);
					ObjectiveMarkerWidget_Offscreen->SetDirectionAngle(Angle);

					if (!bOverrideIconVisibility)
					{
						ObjectiveMarkerWidget_Offscreen->ShowAll();
						
						if (bHideIconOffscreen)
							ObjectiveMarkerWidget_Offscreen->HideIcon();
						
						ObjectiveMarkerWidget_Onscreen->HideAll();
					}
				}
				else
				{
					SCOPE_CYCLE_COUNTER(STAT_OnscreenWidgetUpdate)
					
					if (!bOverrideIconVisibility)
					{
						ObjectiveMarkerWidget_Offscreen->HideAll();

						ObjectiveMarkerWidget_Onscreen->ShowAll();
					}
				}
			}
			
			// Fade on widget overlap
			bool bIntersectsWithAnyWidgetsInHUD = false;
			{
				SCOPE_CYCLE_COUNTER(STAT_WidgetOverlapChecks)
				
				if (bCompletelyFadeWhenOverlappingOtherWidgets)
				{
					if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn))
					{
						if (UHumanCharacterHUD_V2* HUD = PlayerCharacter->HumanCharacterWidget_V2)
						{
							for (const FName& WidgetFadeZoneName : HUD->GetWidgetFadeZoneNames())
							{
								if (UReadyOrNotFunctionLibrary::DoesWidgetOverlap(this, HUD->GetMainCanvas(), ScreenPosition, UReadyOrNotFunctionLibrary::GetWidgetSize_Local(ObjectiveMarkerWidget_Offscreen), HUD->WidgetTree->FindWidget(WidgetFadeZoneName)))
								{
									bIntersectsWithAnyWidgetsInHUD = true;
									break;
								}
							}
						}
					}
				}
			}
			
			bool bObstructingPlayerView = false;
			{
				SCOPE_CYCLE_COUNTER(STAT_WidgetObstructionCheck)
				
				if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerPawn))
				{
					FVector CameraLocation;
					FRotator CameraRotation;
					PlayerCharacter->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
					
					const FVector DirectionToCamera = (CurrentMarkerLocation - CameraLocation).GetSafeNormal();
					const FVector PlayerLookDirection = PlayerCharacter->GetControlRotation().Vector();
					
					bObstructingPlayerView = PlayerCharacter->bAiming && FVector::DotProduct(PlayerLookDirection, DirectionToCamera) > 0.98f;
				}
			}

			{
				SCOPE_CYCLE_COUNTER(STAT_WidgetOpacityUpdate)
				
				const bool bShouldHideObjectiveMarker = (bCompletelyFadeWhenClose && DistanceToLocalPlayer <= FadeAtDistance_Close) || (bCompletelyFadeWhenFar && DistanceToLocalPlayer >= FadeAtDistance_Far) || bIntersectsWithAnyWidgetsInHUD || bRequestingFadeOut || bObstructingPlayerView;
					
				if (bShouldHideObjectiveMarker)
				{
					ObjectiveMarkerWidget_Offscreen->SetRenderOpacity(FMath::FInterpTo(ObjectiveMarkerWidget_Offscreen->GetRenderOpacity(), 0.0f, DeltaTime, FadeSpeed));
					ObjectiveMarkerWidget_Onscreen->SetRenderOpacity(FMath::FInterpTo(ObjectiveMarkerWidget_Onscreen->GetRenderOpacity(), 0.0f, DeltaTime, FadeSpeed));
				}
				else
				{
					if (bFadeOffscreen)
					{
						ObjectiveMarkerWidget_Offscreen->SetRenderOpacity(bIsOffscreen ? FMath::FInterpTo(ObjectiveMarkerWidget_Offscreen->GetRenderOpacity(), FMath::GetMappedRangeValueClamped(FVector2D(0.6f, 0.0f), FVector2D(1.0f, 0.5f), ForwardDotProduct), DeltaTime, FadeSpeed) : FMath::FInterpTo(ObjectiveMarkerWidget_Offscreen->GetRenderOpacity(), 1.0f, DeltaTime, FadeSpeed));
						ObjectiveMarkerWidget_Onscreen->SetRenderOpacity(bIsOffscreen ? 0.0f : FMath::FInterpTo(ObjectiveMarkerWidget_Onscreen->GetRenderOpacity(), 1.0f, DeltaTime, FadeSpeed));
					}
				}
			}
		}
		else
		{
			DestroyObjectiveMarkerWidget();

			DestroyComponent();
		}
	}
	else
	{
		HideObjectiveMarker(false);

		SetComponentTickInterval(1.0f);
	}
}

void UObjectiveMarkerComponent::DestroyComponent(const bool bPromoteChildren)
{
	DestroyObjectiveMarkerWidget();

	Super::DestroyComponent(bPromoteChildren);
}

void UObjectiveMarkerComponent::CheckHUDOnScreen()
{
	if (bEnabled)
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			if (UHumanCharacterHUD_V2* HUD = PlayerCharacter->HumanCharacterWidget_V2)
			{
				bHUDVisible = HUD->IsInViewport();
			}
		}
		else
		{
			bHUDVisible = UBpGameplayHelperLib::HasWidgetInViewport("CharacterHUD_V2");
		}
	}
	else
	{
		bHUDVisible = false;
	}
}

void UObjectiveMarkerComponent::CreateObjectiveMarkerWidget()
{
	DestroyObjectiveMarkerWidget();

	if (GetWorld() && GetWorld()->bIsTearingDown)
		return;
	
	if (bEnabled)
	{
		SetWidgetClass(MarkerWidgetClass);

		ObjectiveMarkerWidget_Onscreen = static_cast<UObjectiveMarkerWidget*>(GetUserWidgetObject());
		if (ObjectiveMarkerWidget_Onscreen)
		{
			ObjectiveMarkerWidget_Onscreen->ParentComponent = this;
			ObjectiveMarkerWidget_Onscreen->SetIconImage(IconBrush);
			ObjectiveMarkerWidget_Onscreen->SetIconColorAndOpacity(IconColorAndOpacity);
			ObjectiveMarkerWidget_Onscreen->SetMarkerNameText(MarkerText);
			ObjectiveMarkerWidget_Onscreen->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		if (MarkerWidgetClass && !ObjectiveMarkerWidget_Offscreen)
		{
			ObjectiveMarkerWidget_Offscreen = CreateWidget<UObjectiveMarkerWidget>(GetWorld(), MarkerWidgetClass);
			if (ObjectiveMarkerWidget_Offscreen)
			{
				ObjectiveMarkerWidget_Offscreen->ParentComponent = this;
				ObjectiveMarkerWidget_Offscreen->SetIconImage(IconBrush);
				ObjectiveMarkerWidget_Offscreen->SetIconSize(IconSize);
				ObjectiveMarkerWidget_Offscreen->SetIconColorAndOpacity(IconColorAndOpacity);
				ObjectiveMarkerWidget_Offscreen->SetMarkerNameText(MarkerText);
				ObjectiveMarkerWidget_Offscreen->AddToViewport(0);
				ObjectiveMarkerWidget_Offscreen->SetVisibility(ESlateVisibility::Collapsed);

				bStartHidden ? HideObjectiveMarker() : ShowObjectiveMarker();

				//bHUDVisible = bDynamic;
				bHUDVisible = true;

				//GetWorld()->GetTimerManager().ClearTimer(TH_CheckHUD);
				//UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_CheckHUD, this, &UObjectiveMarkerComponent::CheckHUDOnScreen, 0.05f /*20hz*/, true);
			}

			SetComponentTickInterval(0.0f);
		}
		else
		{
			SetComponentTickInterval(1.0f);

			HideObjectiveMarker();

			#if WITH_EDITOR
			ULog::Error(CUR_CLASS_FUNC_2 + "Failed to create objective marker widget. WidgetClass is not valid!");
			#endif
		}
	}
}

void UObjectiveMarkerComponent::SetIconBrush(const FSlateBrush NewIconBrush)
{
	IconBrush = NewIconBrush;

	InitMarkerSettings(IconBrush, IconColorAndOpacity);
}

void UObjectiveMarkerComponent::SetIconColor(const FLinearColor InIconColorAndOpacity)
{
	IconColorAndOpacity = InIconColorAndOpacity;

	if (ObjectiveMarkerWidget_Offscreen)
	{
		ObjectiveMarkerWidget_Offscreen->SetIconColorAndOpacity(IconColorAndOpacity);
	}

	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetIconColorAndOpacity(IconColorAndOpacity);
	}
}

void UObjectiveMarkerComponent::SetMarkerTextColor(FLinearColor InIconColorAndOpacity)
{
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetMarkerNameTextColorAndOpacity(InIconColorAndOpacity);
	}
}

void UObjectiveMarkerComponent::SetIconSize(const FVector2D NewIconSize)
{
	if (bDistanceScaleIcon)
		return;
	
	if (ObjectiveMarkerWidget_Offscreen)
	{
		ObjectiveMarkerWidget_Offscreen->SetIconSize(NewIconSize);
	}
	
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetIconSize(NewIconSize);
	}
}

void UObjectiveMarkerComponent::ShowIcon()
{
	if (IconBrush.GetResourceObject() == nullptr)
	{
		HideIcon();
		return;
	}
	
	if (ObjectiveMarkerWidget_Offscreen)
	{
		if (bIsOffscreen)
		{
			if (bHideIconOffscreen)
				ObjectiveMarkerWidget_Offscreen->HideIcon();
			else
				ObjectiveMarkerWidget_Offscreen->ShowIcon();
			
			ObjectiveMarkerWidget_Offscreen->HideMarkerText();
		}
		else
		{
			ObjectiveMarkerWidget_Offscreen->HideIcon();
			ObjectiveMarkerWidget_Offscreen->HideMarkerText();
		}
	}

	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->ShowIcon();
	}
}

void UObjectiveMarkerComponent::HideIcon()
{
	if (ObjectiveMarkerWidget_Offscreen)
	{
		ObjectiveMarkerWidget_Offscreen->HideIcon();
	}

	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->HideIcon();
	}
}

void UObjectiveMarkerComponent::ShowMarkerText()
{
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->ShowMarkerText();
	}
}

void UObjectiveMarkerComponent::HideMarkerText()
{
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->HideMarkerText();
	}
}

void UObjectiveMarkerComponent::EnableObjectiveMarker()
{
	bEnabled = true;
}

void UObjectiveMarkerComponent::DisableObjectiveMarker()
{
	HideObjectiveMarker();
	
	bEnabled = false;
	bFirstTick = false;
}

void UObjectiveMarkerComponent::DestroyObjectiveMarkerWidget()
{
	SCOPE_CYCLE_COUNTER(STAT_DestroyObjectiveMarker);
	
	if (ObjectiveMarkerWidget_Offscreen)
	{
		ObjectiveMarkerWidget_Offscreen->RemoveFromParent();
		ObjectiveMarkerWidget_Offscreen = nullptr;
	}
	
	SetWidgetClass(nullptr);
}

void UObjectiveMarkerComponent::ToggleObjectiveMarkerVisibility()
{
	IsVisible() ? HideObjectiveMarker() : ShowObjectiveMarker();
}

void UObjectiveMarkerComponent::ShowObjectiveMarker()
{
	SCOPE_CYCLE_COUNTER(STAT_ShowObjectiveMarker);
	
	if (bEnabled)
	{
		//SetVisibility(true);
		//SetHiddenInGame(false);

		bRequestingFadeOut = false;

		if (ObjectiveMarkerWidget_Offscreen)
		{
			const ESlateVisibility PreviousVisibility = ObjectiveMarkerWidget_Offscreen->GetVisibility();
			
			// Ensures this event gets called once, to prevent spam
			if (PreviousVisibility != ESlateVisibility::HitTestInvisible)
			{
				ObjectiveMarkerWidget_Offscreen->SetVisibility(ESlateVisibility::HitTestInvisible);
				ObjectiveMarkerWidget_Offscreen->OnMarkerVisibilityEnabled();
			}
		}

		if (ObjectiveMarkerWidget_Onscreen)
		{
			const ESlateVisibility PreviousVisibility = ObjectiveMarkerWidget_Onscreen->GetVisibility();
			
			// Ensures this event gets called once, to prevent spam
			if (PreviousVisibility != ESlateVisibility::HitTestInvisible)
			{
				ObjectiveMarkerWidget_Onscreen->SetVisibility(ESlateVisibility::HitTestInvisible);
				ObjectiveMarkerWidget_Onscreen->OnMarkerVisibilityEnabled();
			}
		}
	}
	else
	{
		if (ObjectiveMarkerWidget_Offscreen)
		{
			if (ObjectiveMarkerWidget_Offscreen->GetVisibility() != ESlateVisibility::Collapsed)
				ObjectiveMarkerWidget_Offscreen->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (ObjectiveMarkerWidget_Onscreen)
		{
			if (ObjectiveMarkerWidget_Onscreen->GetVisibility() != ESlateVisibility::Collapsed)
				ObjectiveMarkerWidget_Onscreen->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UObjectiveMarkerComponent::HideObjectiveMarker(const bool bFadeOut)
{
	SCOPE_CYCLE_COUNTER(STAT_HideObjectiveMarker);
	
	//SetVisibility(false);
	//SetHiddenInGame(true);
	
	//if (bEnabled)
	//{
		bRequestingFadeOut = bFadeOut;

		if (bFadeOut)
			return;

		if (ObjectiveMarkerWidget_Offscreen && !bFadeOut)
		{
			const ESlateVisibility PreviousVisibility = ObjectiveMarkerWidget_Offscreen->GetVisibility();

			// Ensures this event gets called once, to prevent spam
			if (PreviousVisibility != ESlateVisibility::Collapsed)
			{
				ObjectiveMarkerWidget_Offscreen->SetVisibility(ESlateVisibility::Collapsed);
				ObjectiveMarkerWidget_Offscreen->OnMarkerVisibilityDisabled();
			}
		}

		if (ObjectiveMarkerWidget_Onscreen && !bFadeOut)
		{
			const ESlateVisibility PreviousVisibility = ObjectiveMarkerWidget_Onscreen->GetVisibility();

			// Ensures this event gets called once, to prevent spam
			if (PreviousVisibility != ESlateVisibility::Collapsed)
			{
				ObjectiveMarkerWidget_Onscreen->SetVisibility(ESlateVisibility::Collapsed);
				ObjectiveMarkerWidget_Onscreen->OnMarkerVisibilityDisabled();
			}
		}
	//}
}

void UObjectiveMarkerComponent::SetNewFadeDistance(const float NewDistance)
{
	FadeAtDistance_Close = FMath::Clamp(NewDistance, 0.0f, 1000000.0f);
}

void UObjectiveMarkerComponent::SetMarkerText(const FText NewMarkerText)
{
	MarkerText = NewMarkerText;

	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetMarkerNameText(NewMarkerText);
	}
}

void UObjectiveMarkerComponent::SetMarkerTextFontSize(const int32 NewFontSize)
{
	if (ObjectiveMarkerWidget_Onscreen)
	{
		ObjectiveMarkerWidget_Onscreen->SetMarkerNameTextFontSize(NewFontSize);
	}
}

bool UObjectiveMarkerComponent::CanShowObjectiveMarker() const
{
	bool bInPlanningMenu = false;
	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(UGameplayStatics::GetGameState(this)))
	{
		bInPlanningMenu = GS->bInPlanningMenu;
	}

	bool bInTabMenu = false;
	if (APlayerCharacter* PC = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		bInTabMenu = PC->bInTabMenu;
	}
	
#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
		return bEnabled && DEBUG_SUBSYSTEM->bShowObjectiveMarkers && bHUDVisible && !bInPlanningMenu && !bInTabMenu;

	return bEnabled && bHUDVisible && !bInPlanningMenu && !bInTabMenu;
#else
	return bEnabled && bHUDVisible && !bInPlanningMenu && !bInTabMenu;
#endif
}
