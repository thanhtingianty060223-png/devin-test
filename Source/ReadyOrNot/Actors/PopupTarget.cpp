// Copyright Void Interactive, 2017

#include "PopupTarget.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameState.h"
#include "Gameplay/ReadyOrNotPlayerState.h"
#include "Net/UnrealNetwork.h"



// Sets default values
APopupTarget::APopupTarget()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName("BlockAll");

	Health = 100.0f;
	MaxHealth = 100.0f;
}

// Called when the game starts or when spawned
void APopupTarget::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	
}

// Called every frame
void APopupTarget::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void APopupTarget::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APopupTarget, Health);
	DOREPLIFETIME(APopupTarget, bFallDown);
}

bool APopupTarget::IsAlive()
{
	return Health > 0;
}

float APopupTarget::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{

	Health -= DamageAmount;
	if (FMath::Clamp(Health, 0.0f, Health) + DamageAmount <= 0)
	{
		FallDown();
	}
	return Health;
}

void APopupTarget::FallDown()
{
	bFallDown = true;
	GetWorld()->GetTimerManager().SetTimer(PopupTimer, this, &APopupTarget::Popup, PopupTime, false);
}

void APopupTarget::Popup()
{
	bFallDown = false;
	Health = 100.0f;
}
