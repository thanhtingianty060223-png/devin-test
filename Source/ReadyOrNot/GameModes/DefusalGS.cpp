// Copyright Void Interactive, 2022


#include "GameModes/DefusalGS.h"

#include "Actors/Triggers/LoadoutPortal.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"

ADefusalGS::ADefusalGS()
{
	RoundsToPlay = 3;
}

void ADefusalGS::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADefusalGS::ChangeDefusalMatchState(EDefusalMatchSate NewMatchState)
{
	DefusalMatchState = NewMatchState;
}

void ADefusalGS::OpenBuyMenu(APlayerController* Controller)
{
	if (ElapsedRoundTime < 30)
	{
		TArray<UUserWidget*> OutWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), OutWidgets, ULoadout_V2::StaticClass());
		if (OutWidgets.Num() > 0)
		{
			ULoadout_V2* LoadoutWidget =  Cast<ULoadout_V2>( OutWidgets[0]);
			LoadoutWidget->ExitLoadout();
		}
		else
		{
			if (!LoadoutPortal)
			{
				LoadoutPortal = GetWorld()->SpawnActor<ALoadoutPortal>(FVector(0.0f, 0.0f, 10000.0f), FRotator::ZeroRotator);
			}
			LoadoutPortal->LoadLoadout();

		}		
	}
	
}

void ADefusalGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, DefusalMatchState);
	DOREPLIFETIME(ThisClass, CountdownUntilMatchStarts);
	DOREPLIFETIME(ThisClass, BombTimeRemaining);
}
