#include "UnmannedVehicle.h"
#include "ReadyOrNot.h"

#if !UE_BUILD_SHIPPING
#include "Log.h"
#endif

void AUnmannedVehicle::BeginPlay()
{
	Super::BeginPlay();
}

void AUnmannedVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AUnmannedVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUnmannedVehicle, Pilot);
}

void AUnmannedVehicle::Server_StopPiloting_Implementation(class AReadyOrNotPlayerController* CallingController)
{
	// Repossess the pilot.
	if (Pilot)
	{
		Pilot->CurrentlyPiloting = nullptr;
		CallingController->Possess(Pilot);

		// Swap in their HUD.
		if (VehicleHUD.Get())
		{
			CallingController->ClientSetHUD(PreviousHUD);
		}
	}
	Pilot = nullptr;
}

void AUnmannedVehicle::Server_StartPiloting_Implementation(class AReadyOrNotPlayerController* NewController)
{
	if (NewController)
	{
		AssumeTabletControl_Implementation(Cast<APlayerCharacter>(NewController->GetPawn()));
	}
	else
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error(CUR_CLASS_FUNC_WITH_LINE + " | NewController is null");
		#endif
	}
}

float AUnmannedVehicle::TakeDamage(const float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);

	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AUnmannedVehicle::Die(AController* EventInstigator, AActor* DamageCauser)
{
	return;
}

// IControllableByTablet implementation
bool AUnmannedVehicle::CanControlWithTablet_Implementation(class APlayerCharacter* TabletOwner)
{
	// Unmanned vehicles are always controllable with the tablet
	return true;
}

void AUnmannedVehicle::AssumeTabletControl_Implementation(class APlayerCharacter* TabletOwner)
{
	AReadyOrNotPlayerController* pc = TabletOwner->GetRONPlayerController();

	if (Pilot != nullptr)
	{
		return;
	}

	// Repossess.
	Pilot = TabletOwner;
	Pilot->CurrentlyPiloting = this;
	TabletOwner->Controller->Possess(this);

	// Swap in the new HUD, saving the previous HUD class.
	if (VehicleHUD.Get())
	{
		PreviousHUD = pc->GetHUD()->StaticClass();
		pc->ClientSetHUD(VehicleHUD);
	}
}

bool AUnmannedVehicle::CanTabletViewMe_Implementation(class APlayerCharacter* TabletOwner, class AReadyOrNotGameState* GameState)
{
	// Unmanned vehicles are always visible on the tablet
	return true;
}

USceneComponent* AUnmannedVehicle::GetTabletViewComponent_Implementation()
{
	return nullptr;
}

ETeamType AUnmannedVehicle::GetTabletViewTeamColor_Implementation()
{
	return ETeamType::TT_SQUAD;
}

void AUnmannedVehicle::HideActorsForTabletView_Implementation(class USceneCaptureComponent2D* Component)
{
	return;
}
