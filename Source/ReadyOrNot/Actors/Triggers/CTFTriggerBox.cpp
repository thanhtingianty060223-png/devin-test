// Void Interactive, 2020

#include "CTFTriggerBox.h"

#include "GameModes/CaptureTheFlagGM.h"
#include "GameModes/CaptureTheFlagGS.h"

ACTFTriggerBox::ACTFTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SetActorTickEnabled(true);

	OnActorBeginOverlap.AddDynamic(this, &ACTFTriggerBox::OnBeginOverlap);

	GetCollisionComponent()->SetCollisionProfileName("Trigger");
	GetCollisionComponent()->CanCharacterStepUpOn = ECB_No;

#if WITH_EDITOR
	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text Render"));
	TextRender->SetRelativeLocation({0.0f, 0.0f, 100.0f});
	TextRender->SetRelativeRotation({0.0f, -90.0f, 0.0f});
	TextRender->HorizontalAlignment = EHTA_Center;
	TextRender->VerticalAlignment = EVRTA_TextCenter;
	TextRender->SetWorldSize(80.0f);
	TextRender->Text = FText::FromString("CTF Trigger");
	TextRender->SetHiddenInGame(false);
	TextRender->SetupAttachment(RootComponent);

	AActor::SetActorHiddenInGame(false);
	
	if (GetCollisionComponent())
	{
		GetCollisionComponent()->SetHiddenInGame(true);
	}
	
	if (GetSpriteComponent())
	{
		GetSpriteComponent()->SetHiddenInGame(true);
	}
#endif
}

void ACTFTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	ACaptureTheFlagGM* CTFGM = Cast<ACaptureTheFlagGM>(UGameplayStatics::GetGameMode(this));
	ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this));

	// This trigger box should only work in CTF gamemodes and gamestates
	if (!CTFGM && !CTFGS)
	{
		Destroy();
		
		return;
	}
}

bool ACTFTriggerBox::FulfillsRequirements_Implementation()
{
	return true;
}

void ACTFTriggerBox::OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this)))
	{
		if (!CTFGS->bGameWon)
		{
			if (OtherActor != this && OtherActor->IsValidLowLevel() && CTFGS->FlagBearer == OtherActor && FulfillsRequirements())
			{
				if (ACaptureTheFlagGM* CTFGM = Cast<ACaptureTheFlagGM>(UGameplayStatics::GetGameMode(this)))
				{
					CTFGM->RoundWonTeam(CTFGS->FlagBearerTeam);
				}
			}
		}
	}
}
