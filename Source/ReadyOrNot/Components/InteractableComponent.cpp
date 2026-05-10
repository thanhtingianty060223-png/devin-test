// Void Interactive, 2020

#include "InteractableComponent.h"

#include "CommonInputSubsystem.h"
#include "InventoryComponent.h"
#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "HUD/Widgets/AnimatedIconWidget.h"
#include "HUD/Widgets/AnimatedIconWidgetWithActionPrompt.h"
#include "HUD/Widgets/AnimatedIconWidget_Imprint.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "ReadyOrNotDebugSubsystem.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Tick"), STAT_InteractableCompTick, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Enabled Checks"), STAT_EnabledChecks, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ UpdateIconVisibility"), STAT_UpdateIconVisibility, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Show Action Prompts"), STAT_ShowActionPrompts, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Hide Action Prompts"), STAT_HideActionPrompts, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ LineTrace"), STAT_LineTrace, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ SetRedrawTime"), STAT_SetRedrawTime, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ InitIconWidget"), STAT_InitIconWidget, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ DestroyIconWidget"), STAT_DestroyIconWidget, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ CanUseIconWidget"), STAT_IconCanUseWidget, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Determine Animated Icon"), STAT_DetermineAnimatedIcon, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Hide All UI Elements"), STAT_HideAllUIElements, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Assign Action Slot"), STAT_AssignActionSlot, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Clear Action Slot"), STAT_ClearActionSlot, STATGROUP_InteractableComp);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Interactable Comp ~ Icon Update"), STAT_IconUpdate, STATGROUP_InteractableComp);

#define LOCTEXT_NAMESPACE "InteractableComponent"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.033f;

	SetTickableWhenPaused(false);
	SetTickWhenOffscreen(false);
	SetRedrawTime(0.033f);
	SetCanEverAffectNavigation(false);

	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(128.0f, 128.0f));

	UPrimitiveComponent::SetCollisionResponseToAllChannels(ECR_Ignore);
	UPrimitiveComponent::SetNotifyRigidBodyCollision(false);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bFillCollisionUnderneathForNavmesh = false;
	bTickInEditor = false;

	bEnabled = true;
	bCanHide = true;
	bMustBeLookingAt = true;
	bHasLineOfSightToUseActor = true;
	bDistanceFadeIcon = true;
	bImprintIconOnHUDUponInteraction = false;
	bPreviousInteractStateValid = true;
	bCurrentInteractStateValid = true;

	ActionSlot1.DisallowedItems.Empty();
	ActionSlot2.DisallowedItems.Empty();
	ActionSlot3.DisallowedItems.Empty();
	ActionSlot4.DisallowedItems.Empty();
	
	ActionSlot1.DisallowedItems.Add(EItemCategory::IC_Shield);
	ActionSlot2.DisallowedItems.Add(EItemCategory::IC_Shield);
	ActionSlot3.DisallowedItems.Add(EItemCategory::IC_Shield);
	ActionSlot4.DisallowedItems.Add(EItemCategory::IC_Shield);
}

void UInteractableComponent::InitIconWidget()
{
	SCOPE_CYCLE_COUNTER(STAT_InitIconWidget);
	
	//IconWidget = static_cast<UAnimatedIconWidgetWithActionPrompt*>(GetUserWidgetObject());
	IconWidget = Cast<UAnimatedIconWidgetWithActionPrompt>(GetUserWidgetObject());
	if (IconWidget)
	{
		IconWidget->SetRenderOpacity(0.0f);
		IconWidget->SetParentComponent(this);
		IconWidget->ResetAnim();
		IconWidget->SetInteractIconSize(InteractCircleSize, InteractIconSize);
		
		bPlayedFocusAnim = false;
		bCanHide = true;
		bHasLineOfSightToUseActor = false;
		bPreviousInteractStateValid = true;
		bCurrentInteractStateValid = true;
		
		ActiveActionPromptSlot = &ActionSlot1;

		if (bShowActionPromptInWorld)
		{
			IconWidget->ShowActionPrompt(true, 0);
		}

		IconWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInteractableComponent::DestroyIconWidget()
{
	bHasLineOfSightToUseActor = false;

	if (IconWidget && IconWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		IconWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInteractableComponent::ImprintIcon()
{
	if (AnimatedIcon.Icons.Num() == 0)
		return;

	if (ensureMsgf(IconWidget_Imprint, TEXT("IconWidget_Imprint should not be null")))
	{
		IconWidget_Imprint->Init(GetComponentLocation(), AnimatedIcon.Icons.Last());
	
		if (!IconWidget_Imprint->IsInViewport())
			IconWidget_Imprint->AddToViewport();
	}
}

bool UInteractableComponent::IsDisallowedItemEquipped(APlayerCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter)
		return false;

	if (!PlayerCharacter->GetEquippedItem())
		return false;

	if (!ActiveActionPromptSlot)
		return false;

	if (!ActiveActionPromptSlot->bCheckForDisallowedItems)
		return false;

	// todo: need to support multiple active slots, not just one. if any slot is disallowed then the entire interaction is disabled, which is bad. need to improve this 
	// slot 1: press f to open
	// slot 2: cannot kick with x (disallowed)
	// slot 3: press b to boop

	for (EItemCategory ItemCategory : ActiveActionPromptSlot->DisallowedItems)
	{
		if (PlayerCharacter->GetEquippedItem()->ItemCategories.Contains(ItemCategory))
		{
			return true;
		}
	}

	return false;
}

void UInteractableComponent::OnRegister()
{
	Super::OnRegister();
		
	const FString W_AnimatedIconWidgetPath = "WidgetBlueprint'/Game/Blueprints/Widgets/HUD/W_AnimatedIconWidgetWithActionPrompt.W_AnimatedIconWidgetWithActionPrompt_C'";
	UClass* AnimatedIconClass = LoadObject<UClass>(nullptr, *W_AnimatedIconWidgetPath);
	SetWidgetClass(AnimatedIconClass);

	const FWidgetLookupData ImprintWidgetData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("AnimatedWidgetImprintIcon");
    ImprintIconWidgetClass = TSubclassOf<UAnimatedIconWidget_Imprint>(ImprintWidgetData.WidgetClass);
}

void UInteractableComponent::EnableInteractable()
{
	bEnabled = true;
}

void UInteractableComponent::DisableInteractable()
{
	if (bEnabled)
	{
		bEnabled = false;

		ResetToOriginalLocation();

		HideAllUIElements(true);
	}
}

void UInteractableComponent::EnableInteractionFor(APlayerCharacter* InCharacter)
{
	if (InCharacter)
		EnableInteractionFor(InCharacter->GetController<APlayerController>());
}

void UInteractableComponent::DisableInteractionFor(APlayerCharacter* InCharacter)
{
	if (InCharacter)
		DisableInteractionFor(InCharacter->GetController<APlayerController>());
}

void UInteractableComponent::EnableInteractionFor(APlayerController* InPlayerController)
{
	if (InPlayerController)
		DisallowedPlayerControllers.Remove(InPlayerController);
}

void UInteractableComponent::DisableInteractionFor(APlayerController* InPlayerController)
{
	if (InPlayerController)
		DisallowedPlayerControllers.AddUnique(InPlayerController);
}

void UInteractableComponent::SetInteractionIconState(const bool bValid)
{
	if (bPreviousInteractStateValid == bValid)
		return;
	
	bPreviousInteractStateValid = bValid;
	bCurrentInteractStateValid = bDisallowedItemEquipped ? false : bValid;
	
	if (IconWidget)
		IconWidget->SetInteractState(bCurrentInteractStateValid);
}

void UInteractableComponent::SetInteractionIconSize(const float InInteractCircleSize, const float InInteractIconSize)
{
	InteractCircleSize = InInteractCircleSize;
	InteractIconSize = InInteractIconSize;
	
	if (IconWidget)
	{
		IconWidget->SetInteractIconSize(InInteractCircleSize, InInteractIconSize);
	}
}

void UInteractableComponent::ResetToOriginalLocation()
{
	if (GetComponentLocation() != OriginalLocation_World)
	{
		SetWorldLocation(OriginalLocation_World);
	}
	if (GetRelativeLocation() != OriginalLocation_Relative)
	{
		SetRelativeLocation(OriginalLocation_Relative);
	}
}

void UInteractableComponent::SetAnimatedIconName(const FName& NewIconName)
{
	if (bDisallowedItemEquipped)
	{
		AnimatedIconName = "Empty";
		return;
	}

	AnimatedIconName = NewIconName;
}

bool UInteractableComponent::IsInteractionEnabledFor(APlayerCharacter* InCharacter)
{
	if (!InCharacter)
		return false;

	return !DisallowedPlayerControllers.Contains(InCharacter->GetController<APlayerController>());
}

bool UInteractableComponent::IsInteractionEnabledForController(APlayerController* InController) const
{
	return !DisallowedPlayerControllers.Contains(InController);
}

bool UInteractableComponent::CanInteract(const bool bLog) const
{
	if (!GetOwner() || !GetWorld() || !UseActor)
		return false;

	if (!bEnabled)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. No Enabled");
		#endif
		
		return false;
	}

	if (bMustBeLookingAt && LastDotProduct < RequiredLookAtPercentage)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. Last DotProduct Less than Required Look At Percentage %f < %f", LastDotProduct, RequiredLookAtPercentage);
		#endif

		return false;
	}

	if (bDistanceChecksEnabled && DistanceFromPlayer < MinShowPromptAtDistance)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. Distance From Player Less than Min Show Prompt At Distance (%f < %f)", DistanceFromPlayer, MinShowPromptAtDistance);
		#endif

		return false;
	}

	if (bDistanceChecksEnabled && DistanceFromPlayer > ShowPromptAtDistance)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. Distance From Player Greater than Show Prompt At Distance (%f > %f)", DistanceFromPlayer, ShowPromptAtDistance);
		#endif

		return false;
	}

	if (bMustBeOverlapping && !bIsOverlappingPlayer)
	{
		#if !UE_BUILD_SHIPPING
				if (bLog)
					V_LOGM(LogReadyOrNot, "Can't Interact. Not Overlapping This Interactable Components Parent Actor");
		#endif

		return false;
	}

	if (!(ActionSlot1.IsValid() || ActionSlot2.IsValid() || ActionSlot3.IsValid() || ActionSlot4.IsValid()))
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. No Action Slows Valid");
		#endif

		return false;
	}
	
	if (!bHasLineOfSightToUseActor)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. No LOS To Use Actor");
		#endif

		return false;
	}

	if (bDisallowedItemEquipped)
	{
		#if !UE_BUILD_SHIPPING
		if (bLog)
			V_LOGM(LogReadyOrNot, "Can't Interact. Disallowed Item Equipped");
		#endif

		return false;
	}

	// Fixes an issue where you can 'Use' this interactable comp but its not focused?
	if (!bIsFocusedComponent)
	{
		#if WITH_EDITOR
		if (bLog)
		{
			for (TObjectIterator<UInteractableComponent>It; It; ++It)
			{
				const AActor* Owner = It->GetOwner();
				if (Owner && It->bIsFocusedComponent)
				{
					V_LOGM(LogReadyOrNot, "%s is focused (owner: %s)", *It->GetName(), *Owner->GetName());
				}
			}
		}
		#endif

		#if !UE_BUILD_SHIPPING
		if (bLog)
		{
			V_LOGM(LogReadyOrNot, "Can't Interact. Not Focused Component (%s %s)", *GetName(), *GetOwner()->GetName());
		}
		#endif
		
		return false;
	}
	
	return true;
}

bool UInteractableComponent::InputActionNameMatchesAnySlot(const FName InInputActionName)
{
	return ActionSlot1.InputActionName == InInputActionName || ActionSlot2.InputActionName == InInputActionName || ActionSlot3.InputActionName == InInputActionName || ActionSlot4.InputActionName == InInputActionName;
}

bool UInteractableComponent::InputActionNameMatchesAnyValidSlot(const FName InInputActionName)
{
	if (ActionSlot1.IsValid())
	{
		if (ActionSlot1.InputActionName == InInputActionName)
			return true;
	}

	if (ActionSlot2.IsValid())
	{
		if (ActionSlot2.InputActionName == InInputActionName)
			return true;
	}

	if (ActionSlot3.IsValid())
	{
		if (ActionSlot3.InputActionName == InInputActionName)
			return true;
	}

	if (ActionSlot4.IsValid())
	{
		if (ActionSlot4.InputActionName == InInputActionName)
			return true;
	}

	return false;
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedWidgetClass = GetWidgetClass();

	InitIconWidget();

	UseActor = GetAttachmentRootActor();

	OriginalLocation_World = GetComponentLocation();
	OriginalLocation_Relative = GetRelativeLocation();

	OriginalAnimatedIconName = AnimatedIconName;

	ActiveActionPromptSlot = &ActionSlot1;

	bHasLineOfSightToUseActor = false;

	APlayerController* LocalPlayerController = GetWorld()->GetGameInstance()->GetFirstLocalPlayerController();
	IconWidget_Imprint = CreateWidget<UAnimatedIconWidget_Imprint>(LocalPlayerController, ImprintIconWidgetClass);

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllInteractableComponents.AddUnique(this);
	}

	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(UBpVideoSettingsLib::GetGameUserSettings());
	if (Settings && Settings->bWorldSpaceActionPrompts && !bOverrideActionPromptUserSettings)
	{
		bShowActionPromptInWorld = Settings->bWorldSpaceActionPrompts;
	}

	// Some interactable components are set to hidden in game like mission portal etc. Rather than go through and find them all,
	// just gonna set them to visible here
	SetHiddenInGame(false);
	
	IconWidget->EnableActionPromptBackground(bEnableActionPromptBackground);
}

void UInteractableComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_InteractableCompTick);
	
	if (!GetOwner() || !GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown) || !UseActor)
	{
		PrimaryComponentTick.UnRegisterTickFunction();
		HideAllUIElements(true, true, true);
		return;
	}

	if (const AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		if (GS->bInPlanningMenu)
		{
			HideAllUIElements(true, true, true);

			return;
		}
	}
	
	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bDrawInteractableComponents)
		{
			if (UseActor->GetOwner() != UGameplayStatics::GetPlayerCharacter(this, 0))
			{
				if (FVector::Distance(GetComponentLocation(), UGameplayStatics::GetPlayerCharacter(this, 0)->GetActorLocation()) < 1000.0f)
				{
					DrawDebugSphere(GetWorld(), GetComponentLocation(), 10.0f, 4, FColor::Red, false, DeltaTime, 0, 0.5f);

					APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
					if (IsBeingLookedAt(PlayerController, ShowPromptAtDistance, 0.985f))
					{
						const FString DebugString = GetName() + " (" + UseActor->GetName() + ": " + GetComponentLocation().ToString() + ")" + LINE_TERMINATOR +
												"Enabled: " + FString(bEnabled ? "True" : "False") + LINE_TERMINATOR +
												"Interaction enabled: " + FString(DisallowedPlayerControllers.Contains(PlayerController) ? "False" : "True")  + LINE_TERMINATOR +
												"Is Focused: " + FString(bIsFocusedComponent ? "True" : "False")  + LINE_TERMINATOR +
												"Can Interact: " + FString(CanInteract() ? "True" : "False")  + LINE_TERMINATOR +
												"Any action slots valid: " + FString(AnyActionSlotValid() ? "True" : "False")  + LINE_TERMINATOR +
												"Has LOS to player: " + FString(bHasLineOfSightToUseActor ? "True" : "False") + LINE_TERMINATOR +
												(bHasLineOfSightToUseActor ? "" : ("Actor blocking LOS: " + FString(FocusedLOSHit.GetActor() ? FocusedLOSHit.GetActor()->GetName() : "None") + LINE_TERMINATOR)) +
												(bHasLineOfSightToUseActor ? "" : ("Component blocking LOS: " + FString(FocusedLOSHit.GetComponent() ? FocusedLOSHit.GetComponent()->GetName() : "None") + LINE_TERMINATOR)) +
												"Disallowed Item Equipped: " + FString(bDisallowedItemEquipped ? "True" : "False") + LINE_TERMINATOR;
													
						DrawDebugString(GetWorld(), GetComponentLocation(), DebugString, nullptr, FColor::White, 0.04f, true);
					}
				}
			}
		}
		
		if (DEBUG_SUBSYSTEM->bDisableInteractableComponent)
		{
			HideAllUIElements(true);

			return;
		}
	}
	#endif
	
	if (bEnabled || (bIsFocusedComponent && !bHideUponInteraction))
	{
		SCOPE_CYCLE_COUNTER(STAT_EnabledChecks)
		
		APlayerController* PlayerController = GetWorld()->GetGameInstance()->GetFirstLocalPlayerController(GetWorld());
		if (!IsValid(PlayerController))
			return;
		
		if (DisallowedPlayerControllers.Contains(PlayerController))
		{
			HideAllUIElements(true, true, true);

			DisableInteractable();
			
			return;
		}

		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn());
		if (!IsValid(PlayerCharacter))
			return;
		
		SCOPE_CYCLE_COUNTER(STAT_IconCanUseWidget)

		if (PlayerCharacter == GetOwner())
		{
			DestroyIconWidget();

			return;
		}
		
        FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
		
		DistanceFromPlayer = FVector::Distance(CameraLocation, GetComponentLocation());

		// Don't tick if we're really out of range to consider interaction
		if (DistanceFromPlayer > ShowPromptAtDistance * 3.0f && bDistanceChecksEnabled)
		{
			DestroyIconWidget();

			return;
		}
		
		LastDotProduct = FVector::DotProduct(CameraRotation.Vector(), UKismetMathLibrary::FindLookAtRotation(CameraLocation, GetComponentLocation()).Vector());

		bIsOverlappingPlayer = PlayerCharacter->IsOverlappingActor(GetOwner());
		
		if (!bOverrideTickInterval)
		{
			PrimaryComponentTick.TickInterval = FMath::GetMappedRangeValueClamped(FVector2D(ShowPromptAtDistance*2.0f, ShowPromptAtDistance*5.0f), FVector2D(0.033f, 1.0f), DistanceFromPlayer);
		}

		// Out of range to consider interaction?
		if (DistanceFromPlayer > ShowPromptAtDistance * 2.0f && bDistanceChecksEnabled)
		{
			DestroyIconWidget();
		}
		else
		{
			if (!IconWidget)
			{
				SetWidgetClass(CachedWidgetClass);

				InitIconWidget();
			}

			bDisallowedItemEquipped = IsDisallowedItemEquipped(PlayerCharacter);

			// Hide everything when under these conditions
			if (PlayerCharacter->bAiming || PlayerCharacter->IsCarried() || PlayerCharacter->IsCarrying() || PlayerCharacter->IsTabletFocused())
			{
				HideAllUIElements(true, false, true);
			}
			// Should we consider interaction?
			else if (PlayerCharacter->CanUse() || (bIsFocusedComponent && (bShowIconWhenActionsLocked || (!bHideUponInteraction && !PlayerCharacter->IsAnimationBlocking()))))
			{
				if (bDisallowedItemEquipped)
					bWasDisallowedItemEquipped = true;

				bool bOutOfRange = (DistanceFromPlayer < MinShowPromptAtDistance || DistanceFromPlayer > ShowPromptAtDistance) && bDistanceChecksEnabled;	// Out of range to consider interaction?
				bool bNotOverlapping = bMustBeOverlapping && !bIsOverlappingPlayer;																			// If overlap is enabled, are we not overlapping?
				bool bNotLookingAt = LastDotProduct < RequiredLookAtPercentage && bMustBeLookingAt;															// Are we not looking at it when we need to be?
				bool bIsMoving = bHideUponPlayerMovement && PlayerCharacter->IsMoving();																	// If player is moving, should we hide if desired?
				if (bOutOfRange || bNotLookingAt || bNotOverlapping || bIsMoving)				
				{
					HideActionPrompts(PlayerCharacter);
				}
				else
				{
					if (bIsFocusedComponent)
					{
						// Anything blocking line of sight to player camera?
						{
							SCOPE_CYCLE_COUNTER(STAT_LineTrace)
							
							FCollisionQueryParams CollisionQueryParams;
							CollisionQueryParams.AddIgnoredActor(UseActor);
							CollisionQueryParams.AddIgnoredActor(PlayerCharacter);
							CollisionQueryParams.AddIgnoredActors((TArray<AActor*>)PlayerCharacter->GetInventoryComponent()->GetInventoryItems());
							CollisionQueryParams.AddIgnoredActors(IgnoreInteractionBlockingActors);
							CollisionQueryParams.bTraceComplex = true;
							GetWorld()->LineTraceSingleByChannel(FocusedLOSHit, GetComponentLocation(), CameraLocation, ECC_Visibility, CollisionQueryParams);
				
							//DrawDebugLine(GetWorld(), GetComponentLocation(), CameraLocation, FColor::Red, false, 0.15f, 2.0f);
				
							bHasLineOfSightToUseActor = !FocusedLOSHit.bBlockingHit || IUseabilityInterface::Execute_CanInteractThroughHitActors(UseActor, FocusedLOSHit);
						}
					}

					if (bMustBeOverlapping)
					{
						if (bIsOverlappingPlayer)
						{
							ShowActionPrompts(PlayerCharacter);
						}
						else
						{
							HideActionPrompts(PlayerCharacter);
						}
					}

					// Fufills requirements for interaction?
					else if (bMustBeLookingAt)
					{
						if (LastDotProduct >= RequiredLookAtPercentage && (!bIsFocusedComponent || bHasLineOfSightToUseActor))
						{
							ShowActionPrompts(PlayerCharacter);
						}
						else
						{
							HideActionPrompts(PlayerCharacter);
						}
					}
					else
					{
						ShowActionPrompts(PlayerCharacter);
					}
				}

				// Null out the player's LastInteractableComponent if focus is lost, from either looking away or out of interaction range or moving
				if (bIsFocusedComponent)
				{
					bOutOfRange = (DistanceFromPlayer < MinShowPromptAtDistance || DistanceFromPlayer > ShowPromptAtDistance) && bDistanceChecksEnabled;	// Out of range to consider interaction?
					bNotOverlapping = bMustBeOverlapping && !bIsOverlappingPlayer;																			// If overlap is enabled, are we not overlapping?
					bNotLookingAt = LastDotProduct < RequiredLookAtPercentage && bMustBeLookingAt;															// Are we not looking at it when we need to be?
					bIsMoving = bHideUponPlayerMovement && PlayerCharacter->IsMoving();																		// If player is moving, should we hide if desired?
					if (bOutOfRange || bNotLookingAt || bNotOverlapping || bIsMoving)				
					{
						HideActionPrompts(PlayerCharacter);

						PlayerCharacter->LastInteractableComponent = nullptr;
						bIsFocusedComponent = false;
						ActiveActionPromptSlot = nullptr;

						PlayersFocusing.Remove(PlayerCharacter);
						
						IUseabilityInterface::Execute_OnFocusLost(UseActor, PlayerCharacter, this);
					}
				}
				
				UpdateIconVisibility(PlayerController, PlayerCharacter, DeltaTime);
			}
			else
			{
				HideAllUIElements(true, false, true);
			}
		}
	}
	else
	{
		HideAllUIElements(true, false, true);
		PrimaryComponentTick.TickInterval = 0.25f;
	}
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	HideAllUIElements(true, true, true);
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllInteractableComponents.Remove(this);
	}
}

void UInteractableComponent::ShowActionPrompts(APlayerCharacter* PlayerCharacter)
{
	SCOPE_CYCLE_COUNTER(STAT_ShowActionPrompts)
	
	if (!PlayerCharacter)
		return;

	if (AnyActionSlotValid())
	{
		if ((PlayerCharacter->LastInteractableComponent && LastDotProduct > PlayerCharacter->LastInteractableComponent->LastDotProduct) || !PlayerCharacter->LastInteractableComponent)
		{
			PlayerCharacter->LastInteractableComponent = this;
			bIsFocusedComponent = true;

			PlayersFocusing.AddUnique(PlayerCharacter);
			
			IUseabilityInterface::Execute_OnFocusGain(UseActor, PlayerCharacter, this);
		}
		else if (PlayerCharacter->LastInteractableComponent != this)
		{
			HideActionPrompts(PlayerCharacter);

			bIsFocusedComponent = false;
			ActiveActionPromptSlot = nullptr;
			
			PlayersFocusing.Remove(PlayerCharacter);
			
			IUseabilityInterface::Execute_OnFocusLost(UseActor, PlayerCharacter, this);

			return;
		}
	}
	else
	{
		ActiveActionPromptSlot = &ActionSlot1;

		HideAllUIElements(false, false, true);
	}
	
	if (PlayerCharacter->HumanCharacterWidget_V2 && bHasLineOfSightToUseActor)
	{
		if (AnyActionSlotValid())
		{
			const bool bUsingGamepad = UReadyOrNotFunctionLibrary::IsUsingGamepad(PlayerCharacter->GetRONPlayerController());

			if (ActionSlot1.IsValid())
			{
				AssignActionSlot(PlayerCharacter->HumanCharacterWidget_V2, &ActionSlot1, 0, bUsingGamepad);
			}
			else
			{
				ClearActionSlot(PlayerCharacter->HumanCharacterWidget_V2, 0);
			}

			if (ActionSlot2.IsValid())
			{
				AssignActionSlot(PlayerCharacter->HumanCharacterWidget_V2, &ActionSlot2, 1, bUsingGamepad);
			}
			else
			{
				ClearActionSlot(PlayerCharacter->HumanCharacterWidget_V2, 1);
			}

			if (ActionSlot3.IsValid())
			{
				AssignActionSlot(PlayerCharacter->HumanCharacterWidget_V2, &ActionSlot3, 2, bUsingGamepad);
			}
			else
			{
				ClearActionSlot(PlayerCharacter->HumanCharacterWidget_V2, 2);
			}

			if (ActionSlot4.IsValid())
			{
				AssignActionSlot(PlayerCharacter->HumanCharacterWidget_V2, &ActionSlot4, 3, bUsingGamepad);
			}
			else
			{
				ClearActionSlot(PlayerCharacter->HumanCharacterWidget_V2, 3);
			}
			
			bPlayerPromptsCleared = false;
		}
		else
		{
			ActiveActionPromptSlot = &ActionSlot1;
		}
	}
}

void UInteractableComponent::HideActionPrompts(APlayerCharacter* PlayerCharacter, const bool bForce)
{
	SCOPE_CYCLE_COUNTER(STAT_HideActionPrompts);
	
	if (!PlayerCharacter)
		return;

	// Never try to clear a slot if we're not in focus. Fixes text flickering
	if (!bIsFocusedComponent && !bForce)
		return;

	if (!bPlayerPromptsCleared)
	{
		if (PlayerCharacter->HumanCharacterWidget_V2)
			PlayerCharacter->HumanCharacterWidget_V2->ClearAllPlayerActionPrompts();
		
		bPlayerPromptsCleared = true;
	}
}

void UInteractableComponent::HideAllUIElements(const bool bForce, const bool bDestroyWidget, const bool bResetLastInteractable)
{
	// Skip if UI is already hidden (unless forcing)
	if (bUIHidden && !bForce)
		return;
	
	SCOPE_CYCLE_COUNTER(STAT_HideAllUIElements);
	
	APlayerController* PlayerController = GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetFirstLocalPlayerController() : nullptr; 
	if (IsValid(PlayerController))
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetPawn()))
		{
			HideActionPrompts(PlayerCharacter, !PlayerCharacter->LastInteractableComponent);

			if (bResetLastInteractable)
			{
				if (bIsFocusedComponent)
				{
					PlayerCharacter->LastInteractableComponent = nullptr;
					bIsFocusedComponent = false;
					ActiveActionPromptSlot = nullptr;

					PlayersFocusing.Remove(PlayerCharacter);
					
					IUseabilityInterface::Execute_OnFocusLost(UseActor, PlayerCharacter, this);
				}
			}
		}
	}

	if (IconWidget)
	{
		if ((bCanHide && IconWidget->GetVisibility() != ESlateVisibility::Collapsed) || bForce)
		{
			IconWidget->SetRenderOpacity(0.0f);
			IconWidget->SetVisibility(ESlateVisibility::Collapsed);
			IconWidget->StopAllAnimations();
			IconWidget->ResetAnim();

			bPlayedFocusAnim = false;

			if (bDestroyWidget)
			{
				DestroyIconWidget();
			}

			bUIHidden = true;
		}
	}
}

void UInteractableComponent::UpdateIconVisibility(APlayerController* PlayerController, APlayerCharacter* PlayerCharacter, const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateIconVisibility);
	
	if (!PlayerController || !PlayerCharacter)
		return;

	// With worldspace action prompts, we can still have action text showing with no animated icon for things like interactables. Don't force no show if just animated icon name is none
	bool bForceNoShow = !AnyActionSlotValid() || (/*bIsFocusedComponent &&*/ !bHasLineOfSightToUseActor) || (bHideUponPlayerMovement && PlayerCharacter->IsMoving() && LastDotProduct < RequiredLookAtPercentage);
	if (!bShowActionPromptInWorld)
	{
		bForceNoShow |= AnimatedIconName.IsNone();
	}

	const FAnimatedIcon NewAnimatedIcon = DetermineAnimatedIcon();
	
	const bool bResetAnim = NewAnimatedIcon != AnimatedIcon;

	const bool bFadeOnDistanceToPlayer = bDistanceFadeIcon && (DistanceFromPlayer > ShowPromptAtDistance && DistanceFromPlayer < ShowPromptAtDistance * 1.5f);

	const float DistMultiplier = bDistanceFadeIcon ? 1.5f : 1.0f;

	if (bDisallowedItemEquipped)
	{
		AnimatedIconName = "Empty";
	}

	// Out of range to display interaction icon?
	if (bDistanceChecksEnabled && (DistanceFromPlayer < MinShowPromptAtDistance || DistanceFromPlayer > ShowPromptAtDistance * DistMultiplier))
	{
		LastDotProduct = -1.0f;
		DesiredRenderOpacity = 0.0f;
	}
	else if (bMustBeOverlapping && !bIsOverlappingPlayer)
	{
		LastDotProduct = -1.0f;
		DesiredRenderOpacity = 0.05f;
	}
	else
	{
		// Looking at interactable or navigating inside item selection menu?
		if ((bMustBeLookingAt && LastDotProduct < RequiredLookAtPercentage) || PlayerCharacter->bInDevicesMenu)
		{
			DesiredRenderOpacity = 0.05f;
		}
		else
		{
			DesiredRenderOpacity = 1.0f;
		}
	}

	// Fade out if we're not the focused interactable
	if (DesiredRenderOpacity >= 1.0f)
	{
		if (PlayerCharacter->LastInteractableComponent != this)
		{
			DesiredRenderOpacity = bShowActionPromptInWorld ? 0.0f : 0.2f;
		}
	}

	if (bForceNoShow)
	{
		DesiredRenderOpacity = 0.0f;
	}

	// Icon rendering and animation logic
	if (IconWidget)
	{
		if (DesiredRenderOpacity < 0.1f && bUIHidden)
		{
			IconWidget->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		// No animated icon widget? just collapse the icon section, not the whole thing
		IconWidget->ShowInteractIcon(!AnimatedIconName.IsNone());

		SCOPE_CYCLE_COUNTER(STAT_IconUpdate);
		
		AnimatedIcon = NewAnimatedIcon;
		
		if (bFadeOnDistanceToPlayer && DesiredRenderOpacity >= 0.1f)
			IconWidget->SetRenderOpacity(FMath::FInterpTo(IconWidget->GetRenderOpacity(), FMath::GetMappedRangeValueClamped(FVector2D(0.0f, ShowPromptAtDistance*1.5f), FVector2D(1.0f, 0.0f), DistanceFromPlayer), DeltaTime, 10.0f));
		else
			IconWidget->SetRenderOpacity(FMath::FInterpTo(IconWidget->GetRenderOpacity(), DesiredRenderOpacity, DeltaTime, 10.0f));

		IconWidget->SetVisibility(IconWidget->GetRenderOpacity() < 0.1f ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		IconWidget->SetCurrentProgress(CurrentProgress);

		if (!bDisallowedItemEquipped && bWasDisallowedItemEquipped)
		{
			SetInteractionIconState(true);
			
			bWasDisallowedItemEquipped = false;
		}
		
		if (bResetAnim)
		{
			IconWidget->ResetAnim();
		}

		if (DesiredRenderOpacity >= 0.1f)
		{
			if (!bPlayedFocusAnim)
			{
				IconWidget->ResetAnim();
				IconWidget->PlayFocusAnim();
				bPlayedFocusAnim = true;
			}
		}
		else if (DesiredRenderOpacity <= 0.0f)
		{
			IconWidget->StopInteractAnim();
			
			if (IconWidget->IsVisible() && IconWidget->GetRenderOpacity() > 0.1f && bPlayedFocusAnim)
			{
				IconWidget->PauseIconAnim();
				IconWidget->PlayFocusAnim(true);
				bPlayedFocusAnim = false;
			}
		}

		// Icon animation logic
		if (!IconWidget->bPaused)
		{
			if (CanInteract()/* && IsFocused()*/ && IconWidget->IconImages.Num() > 1)
			{
				if (IconWidget->ElapsedTime >= AnimatedIcon.FrameRate)
				{
					IconWidget->CurrentIndex++;
					if (IconWidget->CurrentIndex > IconWidget->IconImages.Num() - 1)
					{
						IconWidget->CurrentIndex = 0;
					}
				
					IconWidget->SetActiveIcon(IconWidget->CurrentIndex);

					IconWidget->ElapsedTime = 0.0f;
				}

				IconWidget->ElapsedTime += DeltaTime;
			}
			else
			{
				IconWidget->CurrentIndex = 0;
				IconWidget->ElapsedTime = 0.0f;

				IconWidget->SetActiveIcon(0);
			}
		}

		// Widget rendering optimizations
		{
			SCOPE_CYCLE_COUNTER(STAT_SetRedrawTime)

			if (CurrentProgress != 0.0f)
			{
				SetRedrawTime(0.033f);
			}
			else
			{
				SetRedrawTime(AnimatedIcon.FrameRate <= 0.0f ? PrimaryComponentTick.TickInterval : AnimatedIcon.FrameRate);
			}
		}

		IconWidget->InvalidateLayoutAndVolatility();

		bUIHidden = !IconWidget->IsVisible() || IconWidget->GetRenderOpacity() <= 0.0001f;
	}
}

void UInteractableComponent::AssignActionSlot(UHumanCharacterHUD_V2* HUD, FPlayerActionPromptSlot* InActionSlot, const int32 SlotIndex, bool bUsingGamepad)
{
	if (!InActionSlot || !HUD)
		return;
	
	SCOPE_CYCLE_COUNTER(STAT_AssignActionSlot);
	
	APlayerController* PlayerController = GetWorld()->GetGameInstance()->GetFirstLocalPlayerController(GetWorld());
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(PlayerController->GetCharacter());
	
	FText ValidActionText = LOCTEXT("ValidActionText_Default", "Interact");

	if (ActionSlot1.IsValid())
	{
		ValidActionText = ActionSlot1.bUseCustomDisallowedActionText ? ActionSlot1.DisallowedItemActionText : ActionSlot1.ActionText;
	}
	if (ActionSlot2.IsValid())
	{
		ValidActionText = ActionSlot2.bUseCustomDisallowedActionText ? ActionSlot2.DisallowedItemActionText : ActionSlot2.ActionText;
	}
	if (ActionSlot3.IsValid())
	{
		ValidActionText = ActionSlot3.bUseCustomDisallowedActionText ? ActionSlot3.DisallowedItemActionText : ActionSlot3.ActionText;
	}
	if (ActionSlot4.IsValid())
	{
		ValidActionText = ActionSlot4.bUseCustomDisallowedActionText ? ActionSlot4.DisallowedItemActionText : ActionSlot4.ActionText;
	}

	if (ValidActionText.EqualToCaseIgnored(FText::FromString("None")) ||
		ValidActionText.EqualToCaseIgnored(FText::FromString("Empty")) ||
		ValidActionText.IsEmpty())
	{
		if (!OriginalAnimatedIconName.IsNone() && OriginalAnimatedIconName != "Empty")
			ValidActionText = FText::FromName(OriginalAnimatedIconName);
		else
			ValidActionText = ActionSlot1.ActionText;
	}

	const FText ItemName = PlayerCharacter->GetEquippedItem() ? PlayerCharacter->GetEquippedItem()->ItemName : FText::GetEmpty();
	FText DisallowedText = FText::Format(LOCTEXT("DisallowedText", "Cannot <red>{0}</> with <red>{1}</>"), ValidActionText, ItemName);

	if (bShowActionPromptInWorld)
	{
		if (bDisallowedItemEquipped)
		{
			SetInteractionIconState(false);
			
			IconWidget->SetActionPromptText(DisallowedText, SlotIndex);
			IconWidget->ShowActionPrompt(true, SlotIndex);
		}
		else if (InActionSlot->IsCustomTextValid())
		{
			IconWidget->SetActionPromptText(InActionSlot->CustomActionPromptText, SlotIndex);
			IconWidget->ShowActionPrompt(true, SlotIndex);
		}
		// If we don't have any text to show, hide the action prompt
		else if (InActionSlot->ActionText.IsEmpty() || InActionSlot->ActionText.EqualToCaseIgnored(FText::FromString("None")))
		{
			IconWidget->ShowActionPrompt(false, SlotIndex);
		}
		else
		{
			FText Text = UReadyOrNotFunctionLibrary::FormatPlayerActionText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(InActionSlot->InputActionName, bUsingGamepad), InActionSlot->InputEvent, InActionSlot->ActionText, InActionSlot->ColorLabel);
			IconWidget->SetActionPromptText(Text, SlotIndex);
			IconWidget->ShowActionPrompt(true, SlotIndex);
		}
	}
	else
	{
		if (bDisallowedItemEquipped)
		{
			SetInteractionIconState(false);
			HUD->AssignPlayerActionPrompt_Custom(SlotIndex, DisallowedText, InActionSlot->bAnimate, InActionSlot->bLoopAnimation);
		}
		else if (InActionSlot->IsCustomTextValid())
		{
			HUD->AssignPlayerActionPrompt_Custom(SlotIndex, InActionSlot->CustomActionPromptText, InActionSlot->bAnimate, InActionSlot->bLoopAnimation);
		}
		else
		{
			HUD->AssignPlayerActionPrompt(SlotIndex, UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(InActionSlot->InputActionName, bUsingGamepad), InActionSlot->InputEvent, InActionSlot->ActionText, InActionSlot->ColorLabel, InActionSlot->bAnimate, false);
		}

		IconWidget->ShowActionPrompt(false, SlotIndex);
	}
	
	ActiveActionPromptSlot = InActionSlot;
}

void UInteractableComponent::ClearActionSlot(UHumanCharacterHUD_V2* HUD, const int32 SlotIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_ClearActionSlot);

	if (bShowActionPromptInWorld)
	{
		IconWidget->ShowActionPrompt(false, SlotIndex);
		return;
	}
		
	if (HUD)
	{
		HUD->RemovePlayerActionPrompt(SlotIndex);
	}	
}

FAnimatedIcon UInteractableComponent::DetermineAnimatedIcon()
{
	SCOPE_CYCLE_COUNTER(STAT_DetermineAnimatedIcon);

	if (DesiredRenderOpacity > 0.0f)
	{
		FAnimatedIcon* IconToFind = AnimatedIconMap.Find(AnimatedIconName);
		if (!IconToFind)
		{
			bool bSuccess = false;
			FAnimatedIcon FoundIcon = UBpGameplayHelperLib::GetAnimatedIconFromTable(AnimatedIconName, bSuccess);

			AnimatedIconMap.Add(AnimatedIconName, FoundIcon);

			return FoundIcon;
		}

		return *IconToFind;
	}
	
	return FAnimatedIcon();
}

void UInteractableComponent::OnInteract_Implementation(APlayerCharacter* InteractInstigator)
{
	if (bImprintIconOnHUDUponInteraction && !AnimatedIconName.IsNone() && AnimatedIconName != "Empty")
	{
		ImprintIcon();
	}
	else
	{
		if (IconWidget)
		{
			IconWidget->PlayInteractAnim();
		}		
	}
	InteractInstigator->OnInteract.Broadcast(this);
}

AActor* UInteractableComponent::GetUseActor() const
{
	return UseActor;
}

bool UInteractableComponent::IsFocused() const
{
	return bIsFocusedComponent;
}

bool UInteractableComponent::IsIconVisible() const
{
	if (IconWidget)
	{
		return IconWidget->IsVisible();
	}

	return false;
}

bool UInteractableComponent::IsBeingLookedAt(APlayerController* InPlayerController, const float MaxRange, const float LookatThreshold, const bool bUseActorLocation)
{
	if (InPlayerController)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		InPlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		const FVector TargetLocation = bUseActorLocation && UseActor ? UseActor->GetActorLocation() : GetComponentLocation();
		const float DistanceToCamera = FVector::Distance(CameraLocation, TargetLocation);
		const float DotProduct = FVector::DotProduct(CameraRotation.Vector(), UKismetMathLibrary::FindLookAtRotation(CameraLocation, TargetLocation).Vector());

		if (DistanceToCamera < MaxRange)
		{
			return DotProduct >= LookatThreshold;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
