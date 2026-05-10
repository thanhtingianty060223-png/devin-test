// Void Interactive, 2020

#include "StackUpActor.h"

#include "Door.h"

TAutoConsoleVariable<int32> CVarRonDrawStackups(TEXT("a.RonDrawStackups"), 1, TEXT("0 = No draw stack up actors. 1 = Draw all stack up actors"));

#if WITH_EDITORONLY_DATA
static UTexture2D* NoStackUpImage = nullptr;
static UTexture2D* AlphaStackUpImage = nullptr;
static UTexture2D* BetaStackUpImage = nullptr;
static UTexture2D* CharlieStackUpImage = nullptr;
static UTexture2D* DeltaStackUpImage = nullptr;
static UTexture2D* FoxtrotStackUpImage = nullptr;
#endif

AStackUpActor::AStackUpActor()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = 1.0f;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
	#endif
	
	bCanBeInCluster = false;

	bFindCameraComponentWhenViewTarget = false;
	SetHidden(true);
	
	SetCanBeDamaged(false);

	DefaultScene = CreateDefaultSubobject<USceneComponent>("DefaultScene");
	DefaultScene->SetMobility(EComponentMobility::Static);
	SetRootComponent(DefaultScene);

	/*
	NavModifierComponent = CreateDefaultSubobject<USphereComponent>("Nav Modifier");
	NavModifierComponent->SetupAttachment(GetRootComponent());
	NavModifierComponent->SetCanEverAffectNavigation(true);
	NavModifierComponent->SetCollisionProfileName("Volume");
	NavModifierComponent->InitSphereRadius(1.0f);
	NavModifierComponent->bDynamicObstacle = true;
	*/
	
	#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComp");
	BillboardComponent->ScreenSize = 0.004f;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	BillboardComponent->SetRelativeScale3D(FVector(0.3f));
	BillboardComponent->SetupAttachment(DefaultScene);
	BillboardComponent->SetCachedMaxDrawDistance(1000.0f);
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> NoStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/RedExclamation.RedExclamation'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> AlphaStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_ALPHA.STACKING_ALPHA'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BetaStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_BETA.STACKING_BETA'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CharlieStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_CHARLIE.STACKING_CHARLIE'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DeltaStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_DELTA.STACKING_DELTA'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FoxtrotStackupImageObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/STACKING_FOXTROT.STACKING_FOXTROT'"));
	
	NoStackUpImage = NoStackupImageObj.Object;
	AlphaStackUpImage = AlphaStackupImageObj.Object;
	BetaStackUpImage = BetaStackupImageObj.Object;
	CharlieStackUpImage = CharlieStackupImageObj.Object;
	DeltaStackUpImage = DeltaStackupImageObj.Object;
	FoxtrotStackUpImage = FoxtrotStackupImageObj.Object;
	#endif
}

void AStackUpActor::SetSquadPosition(ADoor* NewLinkedDoor, ESquadPosition SquadPosition)
{
	LinkedDoor = NewLinkedDoor;
	StackUpPosition = SquadPosition;
	
	//DisableNavBlocker();

	/*
	switch (SquadPosition)
	{
		case ESquadPosition::SP_Alpha:		NavModifierComponent->AreaClass = UNavArea_SwatAlpha::StaticClass(); break;
		case ESquadPosition::SP_Beta:		NavModifierComponent->AreaClass = UNavArea_SwatBeta::StaticClass(); break;
		case ESquadPosition::SP_Charlie:	NavModifierComponent->AreaClass = UNavArea_SwatCharlie::StaticClass(); break;
		case ESquadPosition::SP_Delta:		NavModifierComponent->AreaClass = UNavArea_SwatDelta::StaticClass(); break;
		default:							NavModifierComponent->AreaClass = nullptr; break;
	}
	*/

	#if WITH_EDITORONLY_DATA
	switch (StackUpPosition)
	{
		case ESquadPosition::SP_NONE:			BillboardComponent->SetSprite(NoStackUpImage);		break;
		case ESquadPosition::SP_Alpha:			BillboardComponent->SetSprite(AlphaStackUpImage);	break;
		case ESquadPosition::SP_Beta:			BillboardComponent->SetSprite(BetaStackUpImage);	break;
		case ESquadPosition::SP_Charlie:		BillboardComponent->SetSprite(CharlieStackUpImage);	break;
		case ESquadPosition::SP_Delta:			BillboardComponent->SetSprite(DeltaStackUpImage);	break;
		case ESquadPosition::SP_Foxtrot:		BillboardComponent->SetSprite(FoxtrotStackUpImage); break;
		default: break;
	}
	#endif
}

void AStackUpActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
	#endif

	SetActorTickEnabled(false);
	PrimaryActorTick.UnRegisterTickFunction();
}

#if WITH_EDITOR
void AStackUpActor::EditorTick(float DeltaTime)
{
	if (IsSelectedInEditor())
	{
		//DrawDebugBox(GetWorld(), GetActorLocation() + FVector::UpVector * 35.0f, FVector(35.0f), GetDebugColor(), false, -1.0f, 0, 2.0f);
		
		if (LinkedDoor)
		{
			LinkedDoor->DrawStackUpPoints(LinkedDoor->FrontLeftStackUpPoints, FColor::Yellow);
			LinkedDoor->DrawStackUpPoints(LinkedDoor->FrontRightStackUpPoints, FColor::Cyan);
			LinkedDoor->DrawStackUpPoints(LinkedDoor->BackLeftStackUpPoints, FColor::Yellow);
			LinkedDoor->DrawStackUpPoints(LinkedDoor->BackRightStackUpPoints, FColor::Cyan);
		}
	}
	
	BillboardComponent->SetVisibility(CVarRonDrawStackups.GetValueOnAnyThread() > 0);
}
#endif

FColor AStackUpActor::GetDebugColor() const
{
	switch (StackUpPosition)
	{
		case ESquadPosition::SP_Alpha:		return FColor::Blue;
		case ESquadPosition::SP_Beta:		return FColor::Green;
		case ESquadPosition::SP_Charlie:	return FColor::Orange;
		case ESquadPosition::SP_Delta:		return FColor::Yellow;
		default: break;
	}
	
	return FColor::White;
}

void AStackUpActor::EnableNavBlocker()
{
	//NavModifierComponent->SetCanEverAffectNavigation(true);
}

void AStackUpActor::DisableNavBlocker()
{
	//NavModifierComponent->SetCanEverAffectNavigation(false);
}
