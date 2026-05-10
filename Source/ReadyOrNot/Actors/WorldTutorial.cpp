// Copyright Void Interactive, 2023


#include "WorldTutorial.h"
#include "Blueprint/UserWidget.h"
#include "Commander/MetaGameProfile.h"
#include "HUD/Widgets/TutorialWidget.h"

AWorldTutorial::AWorldTutorial()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1;

	ActivationComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Component"));
	ActivationComponent->SetCollisionObjectType(ECC_VOLUME);
	ActivationComponent->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	ActivationComponent->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
	ActivationComponent->SetCollisionResponseToChannel(ECC_OCCLUSION, ECR_Ignore);
	ActivationComponent->SetSphereRadius(500);
	
	SetRootComponent(ActivationComponent);
}

// Called when the game starts or when spawned
void AWorldTutorial::BeginPlay()
{
	Super::BeginPlay();

	TraceDelegate.BindUObject(this,&AWorldTutorial::OnTraceCompleted);

	UMetaGameProfile* GameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if(GameProfile && GameProfile->TotalLobbyLogins != 0 && GameProfile->TotalLobbyLogins >= LoginsUntilInvalid)
	{
		Destroy();
	}

	CreateWidget();
}

// Called every frame
void AWorldTutorial::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	APlayerCharacter* Player = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if(!Player)
		return;

	if(!bIsPlayerInRange)
		return;

	// If we are waiting for the completion event to be called, no need to do any traces.
	if(bIsTutorialVisible && bWaitForCompletionClose)
		return;

	// We still need to do these traces after the UI is already open to determine if LOS has been lost.
	if(TutorialActivationType == TAT_LineOfSight)
	{
		// Ensure player is in range before we do any tracing to keep performance better.
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Player);
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActors((TArray<AActor*>)Player->GetInventoryComponent()->GetInventoryItems());
		GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, Player->GetFirstPersonCameraComponent()->GetComponentLocation(), GetActorLocation(), ECC_Visibility, QueryParams, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
	}
	else if(TutorialActivationType == TAT_DirectLook)
	{
		// Trace the look direction of the player to diameter of the activation area.
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Player);
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActors((TArray<AActor*>)Player->GetInventoryComponent()->GetInventoryItems());
		GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, Player->GetFirstPersonCameraComponent()->GetComponentLocation(), Player->GetFirstPersonCameraComponent()->GetComponentLocation() + Player->GetFirstPersonCameraComponent()->GetForwardVector()*TutorialActivationDistance, ECC_Visibility, QueryParams, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
	}
}

void AWorldTutorial::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if(Cast<APlayerCharacter>(OtherActor))
	{
		OnEnterActivationArea();
	}
}

void AWorldTutorial::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	if(Cast<APlayerCharacter>(OtherActor))
	{
		OnLeaveActivationArea();
	}
}

void AWorldTutorial::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(TutorialActivationType == TAT_LineOfSight){
		if(Data.OutHits.Num() == 0)
		{
			EnableTutorialVisibility();
			if(ScreenspaceMarkerType == SMT_TutorialClosed)
			{
				ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
				// TutorialWidget->HideScreenspaceWidget();
			}
		}
		else
		{
			DisableTutorialVisibility();
			if(ScreenspaceMarkerType == SMT_TutorialClosed)
			{
				ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
				// TutorialWidget->ShowScreenspaceWidget();
			}
		}
	}
	else if(TutorialActivationType == TAT_DirectLook && AttachedObject)
	{
		if(Data.OutHits.Num() > 0 && Data.OutHits[0].GetActor() == AttachedObject)
		{
			EnableTutorialVisibility();
			if(ScreenspaceMarkerType == SMT_TutorialClosed)
			{
				ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
				// TutorialWidget->HideScreenspaceWidget();
			}
		}
		else
		{
			DisableTutorialVisibility();
			if(ScreenspaceMarkerType == SMT_TutorialClosed)
			{
				ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
				// TutorialWidget->ShowScreenspaceWidget();
			}
		}
	}

}

void AWorldTutorial::CompletionEvent()
{
	TutorialWidget->HideMainWidget();

	if(bIsTutorialVisible && !bResetAfterCompletion)
	{
		TutorialWidget->RemoveFromViewport();
		Destroy();
	}
	bIsTutorialVisible = false;
}

void AWorldTutorial::CreateWidget()
{
	if(TutorialWidget)
		return;
	
	const FWidgetLookupData WidgetLookupData = UBpGameplayHelperLib::GetWidgetDataFromLookupData("TutorialWidget");
	TutorialWidget = Cast<UTutorialWidget>(UUserWidget::CreateWidgetInstance(*GetWorld(), WidgetLookupData.WidgetClass, "TutorialWidget"));
	if (ensureAlways(TutorialWidget))
	{
		TutorialWidget->SetData(WidgetData);
	}
}

float AWorldTutorial::GetAngleBetween(FVector A, FVector B)
{
	A.Normalize();
	B.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(A, B)));
}

void AWorldTutorial::OnEnterActivationArea()
{
	bIsPlayerInRange = true;
	TutorialWidget->AddToViewport();
	
	if(TutorialActivationType == TAT_EnterArea)
	{
		// When the player enters the range and the widget does not exist.
		if(bIsPlayerInRange && !TutorialWidget)
			EnableTutorialVisibility();
	}

	if(ScreenspaceMarkerType == SMT_InActivationArea)
	{
		ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
		// TutorialWidget->HideScreenspaceWidget();
	}
}

void AWorldTutorial::OnLeaveActivationArea()
{
	bIsPlayerInRange = false;
	TutorialWidget->RemoveFromViewport();

	// We do nothing since we want to wait for the completion even to be called.
	if(bWaitForCompletionClose)
		return;

	if(ScreenspaceMarkerType == SMT_InActivationArea)
	{
		ensureAlwaysMsgf(false, TEXT("Currently Unavailable. Screenspace marker has been removed from Tutorial Widget."));
		// TutorialWidget->HideScreenspaceWidget();
	}
	
}

void AWorldTutorial::EnableTutorialVisibility()
{
	TutorialWidget->ShowMainWidget();
	bIsTutorialVisible = true;
}

void AWorldTutorial::DisableTutorialVisibility()
{
	// We do nothing since we want to wait for the completion even to be called.
	if(bWaitForCompletionClose)
		return;
	
	TutorialWidget->HideMainWidget();
	bIsTutorialVisible = false;
}


