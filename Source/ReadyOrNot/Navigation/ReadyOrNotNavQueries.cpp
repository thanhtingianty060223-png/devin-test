// Copyright Void Interactive, 2021

#include "ReadyOrNotNavQueries.h"
#include "ReadyOrNotNavAreas.h"

UNavQuery_DoorTest::UNavQuery_DoorTest()
{
	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoor);
	
	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);
}

UNavQuery_Swat::UNavQuery_Swat()
{
	/*
	FNavigationFilterArea FilterAreaPlayer{};
	FilterAreaPlayer.AreaClass = UNavArea_Player::StaticClass();
	FilterAreaPlayer.bOverrideEnteringCost = true;
	FilterAreaPlayer.EnteringCostOverride = 100000000.0f;
	FilterAreaPlayer.bOverrideTravelCost = true;
	FilterAreaPlayer.TravelCostOverride = 10000000000.0f;
	Areas.Add(FilterAreaPlayer);
	*/
	
	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bOverrideEnteringCost = true;
	FilterAreaClosedDoor.EnteringCostOverride = 100000000.0f;
	FilterAreaClosedDoor.bOverrideTravelCost = true;
	FilterAreaClosedDoor.TravelCostOverride = 10000000000.0f;
	FilterAreaClosedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoor);
	
	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = false;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bOverrideEnteringCost = true;
	FilterAreaWallHole.EnteringCostOverride = 100000000.0f;
	FilterAreaWallHole.bOverrideTravelCost = true;
	FilterAreaWallHole.TravelCostOverride = 10000000000.0f;
	FilterAreaWallHole.bIsExcluded = true;
	Areas.Add(FilterAreaWallHole);
}

UNavQuery_SwatFallIn::UNavQuery_SwatFallIn()
{
	/*
	Areas.RemoveAll([](FNavigationFilterArea& Area)
	{
		return Area.AreaClass == UNavArea_Player::StaticClass();
	});
	*/
}

// same as UNavQuery_Swat but has HasBeenOpenedDoor nav area excluded so they dont go path through a different door lol, even tho its a shorter path
UNavQuery_SwatBreachAndClear::UNavQuery_SwatBreachAndClear()
{
	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bOverrideEnteringCost = true;
	FilterAreaClosedDoor.EnteringCostOverride = 100000000.0f;
	FilterAreaClosedDoor.bOverrideTravelCost = true;
	FilterAreaClosedDoor.TravelCostOverride = 10000000000.0f;
	FilterAreaClosedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoor);
	
	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bOverrideEnteringCost = true;
	FilterAreaClosedDoorThatHasBeenOpened.EnteringCostOverride = 100000000.0f;
	FilterAreaClosedDoorThatHasBeenOpened.bOverrideTravelCost = true;
	FilterAreaClosedDoorThatHasBeenOpened.TravelCostOverride = 10000000000.0f;
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bOverrideEnteringCost = true;
	FilterAreaWallHole.EnteringCostOverride = 100000000.0f;
	FilterAreaWallHole.bOverrideTravelCost = true;
	FilterAreaWallHole.TravelCostOverride = 10000000000.0f;
	FilterAreaWallHole.bIsExcluded = true;
	Areas.Add(FilterAreaWallHole);
}

UNavQuery_SwatAlpha::UNavQuery_SwatAlpha()
{
	FNavigationFilterArea FilterArea{};
	FilterArea.AreaClass = UNavArea_SwatBeta::StaticClass();
	FilterArea.bOverrideEnteringCost = true;
	FilterArea.EnteringCostOverride = 100000000.0f;
	FilterArea.bOverrideTravelCost = true;
	FilterArea.TravelCostOverride = 10000000000.0f;
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatCharlie::StaticClass();
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatDelta::StaticClass();
	Areas.Add(FilterArea);
}

UNavQuery_SwatBeta::UNavQuery_SwatBeta()
{
	FNavigationFilterArea FilterArea{};
	FilterArea.AreaClass = UNavArea_SwatAlpha::StaticClass();
	FilterArea.bOverrideEnteringCost = true;
	FilterArea.EnteringCostOverride = 100000000.0f;
	FilterArea.bOverrideTravelCost = true;
	FilterArea.TravelCostOverride = 10000000000.0f;
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatCharlie::StaticClass();
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatDelta::StaticClass();
	Areas.Add(FilterArea);
}

UNavQuery_SwatCharlie::UNavQuery_SwatCharlie()
{
	FNavigationFilterArea FilterArea{};
	FilterArea.AreaClass = UNavArea_SwatAlpha::StaticClass();
	FilterArea.bOverrideEnteringCost = true;
	FilterArea.EnteringCostOverride = 100000000.0f;
	FilterArea.bOverrideTravelCost = true;
	FilterArea.TravelCostOverride = 10000000000.0f;
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatBeta::StaticClass();
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatDelta::StaticClass();
	Areas.Add(FilterArea);
}

UNavQuery_SwatDelta::UNavQuery_SwatDelta()
{
	FNavigationFilterArea FilterArea{};
	FilterArea.AreaClass = UNavArea_SwatAlpha::StaticClass();
	FilterArea.bOverrideEnteringCost = true;
	FilterArea.EnteringCostOverride = 100000000.0f;
	FilterArea.bOverrideTravelCost = true;
	FilterArea.TravelCostOverride = 10000000000.0f;
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatBeta::StaticClass();
	Areas.Add(FilterArea);
	FilterArea.AreaClass = UNavArea_SwatCharlie::StaticClass();
	Areas.Add(FilterArea);
}

UNavQuery_Suspect::UNavQuery_Suspect()
{
	/*
	FNavigationFilterArea FilterAreaPlayer{};
	FilterAreaPlayer.AreaClass = UNavArea_Player::StaticClass();
	FilterAreaPlayer.bOverrideEnteringCost = true;
	FilterAreaPlayer.bOverrideTravelCost = true;
	FilterAreaPlayer.TravelCostOverride = 2.0f;
    Areas.Add(FilterAreaPlayer);
    */
	
	FNavigationFilterArea FilterAreaNoSuspects{};
	FilterAreaNoSuspects.AreaClass = UNavArea_NoSuspects::StaticClass();
	FilterAreaNoSuspects.bIsExcluded = true;
	Areas.Add(FilterAreaNoSuspects);

	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = false;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = false;
	Areas.Add(FilterAreaLockedDoorSuspect);

	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	Areas.Add(FilterAreaClosedDoor);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bIsExcluded = false;
	Areas.Add(FilterAreaWallHole);
	
	FNavigationFilterArea FilterAreaCSGas{};
	FilterAreaCSGas.AreaClass = UNavArea_CSGas::StaticClass();
	FilterAreaCSGas.bOverrideEnteringCost = true;
	FilterAreaCSGas.EnteringCostOverride = 100000000.0f;
	FilterAreaCSGas.bOverrideTravelCost = true;
	FilterAreaCSGas.TravelCostOverride = 10000000000.0f;
	FilterAreaCSGas.bIsExcluded = true;
	Areas.Add(FilterAreaCSGas);
}

UNavQuery_Civilian::UNavQuery_Civilian()
{

	/*
	FNavigationFilterArea FilterAreaPlayer{};
	FilterAreaPlayer.AreaClass = UNavArea_Player::StaticClass();
	FilterAreaPlayer.bOverrideEnteringCost = true;
	FilterAreaPlayer.bOverrideTravelCost = true;
	FilterAreaPlayer.TravelCostOverride = 100.0f;
    Areas.Add(FilterAreaPlayer);
    */
	
	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = false;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);

	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bIsExcluded = false;
	Areas.Add(FilterAreaClosedDoor);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bIsExcluded = true;
	Areas.Add(FilterAreaWallHole);

	FNavigationFilterArea FilterAreaCSGas{};
	FilterAreaCSGas.AreaClass = UNavArea_CSGas::StaticClass();
	FilterAreaCSGas.bOverrideEnteringCost = true;
	FilterAreaCSGas.EnteringCostOverride = 100000000.0f;
	FilterAreaCSGas.bOverrideTravelCost = true;
	FilterAreaCSGas.TravelCostOverride = 10000000000.0f;
	FilterAreaCSGas.bIsExcluded = true;
	Areas.Add(FilterAreaCSGas);
}

UNavQuery_FlankingSuspect::UNavQuery_FlankingSuspect()
{

	/*
	FNavigationFilterArea FilterAreaPlayer{};
	FilterAreaPlayer.AreaClass = UNavArea_Player::StaticClass();
	FilterAreaPlayer.bOverrideEnteringCost = true;
	FilterAreaPlayer.bOverrideTravelCost = true;
	FilterAreaPlayer.TravelCostOverride = 100.0f;
    Areas.Add(FilterAreaPlayer);
    */
	
	FNavigationFilterArea FlankingAvoidance{};
	FlankingAvoidance.AreaClass = UNavArea_FlankingAvoidanceArea::StaticClass();
	FlankingAvoidance.bOverrideEnteringCost = true;
	FlankingAvoidance.EnteringCostOverride = 100000000.0f;
	FlankingAvoidance.bOverrideTravelCost = true;
	FlankingAvoidance.TravelCostOverride = 10000000000.0f;
	Areas.Add(FlankingAvoidance);
	
	FNavigationFilterArea FilterAreaNoSuspects{};
	FilterAreaNoSuspects.AreaClass = UNavArea_NoSuspects::StaticClass();
	FilterAreaNoSuspects.bIsExcluded = true;
	Areas.Add(FilterAreaNoSuspects);

	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = false;
	FilterAreaClosedDoorThatHasBeenOpened.bOverrideEnteringCost = false;
	FilterAreaClosedDoorThatHasBeenOpened.EnteringCostOverride = 0.0f;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);

	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bOverrideEnteringCost = false;

	Areas.Add(FilterAreaClosedDoor);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bIsExcluded = false;
	Areas.Add(FilterAreaWallHole);

	FNavigationFilterArea FilterAreaCSGas{};
	FilterAreaCSGas.AreaClass = UNavArea_CSGas::StaticClass();
	FilterAreaCSGas.bOverrideEnteringCost = true;
	FilterAreaCSGas.EnteringCostOverride = 100000000.0f;
	FilterAreaCSGas.bOverrideTravelCost = true;
	FilterAreaCSGas.TravelCostOverride = 100000000.0f;
	FilterAreaCSGas.bIsExcluded = true;
	Areas.Add(FilterAreaCSGas);
}

UNavQuery_GasFleeingSuspect::UNavQuery_GasFleeingSuspect()
{	
	FNavigationFilterArea FilterAreaNoSuspects{};
	FilterAreaNoSuspects.AreaClass = UNavArea_NoSuspects::StaticClass();
	FilterAreaNoSuspects.bIsExcluded = true;
	Areas.Add(FilterAreaNoSuspects);

	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = false;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);

	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	Areas.Add(FilterAreaClosedDoor);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bIsExcluded = false;
	Areas.Add(FilterAreaWallHole);

	FNavigationFilterArea FilterAreaCSGas{};
	FilterAreaCSGas.AreaClass = UNavArea_CSGas::StaticClass();
	FilterAreaCSGas.bOverrideEnteringCost = true;
	FilterAreaCSGas.EnteringCostOverride = 1000.0f;
	FilterAreaCSGas.bOverrideTravelCost = false;
	//FilterAreaCSGas.TravelCostOverride = 1000.0f;
	FilterAreaCSGas.bIsExcluded = false;
	Areas.Add(FilterAreaCSGas);
}

UNavQuery_NoiseCheck::UNavQuery_NoiseCheck()
{
	/*
	FNavigationFilterArea FilterAreaPlayer{};
	FilterAreaPlayer.AreaClass = UNavArea_Player::StaticClass();	
    Areas.Add(FilterAreaPlayer);
    */
	
	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	Areas.Add(FilterAreaClosedDoor);

	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	Areas.Add(FilterAreaLockedDoor);
}

UNavQuery_CSGas::UNavQuery_CSGas()
{	
	FNavigationFilterArea FilterAreaClosedDoor{};
	FilterAreaClosedDoor.AreaClass = UNavArea_ClosedDoor::StaticClass();
	FilterAreaClosedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoor);
	
	FNavigationFilterArea FilterAreaClosedDoorThatHasBeenOpened{};
	FilterAreaClosedDoorThatHasBeenOpened.AreaClass = UNavArea_HasBeenOpenedDoor::StaticClass();
	FilterAreaClosedDoorThatHasBeenOpened.bIsExcluded = true;
	Areas.Add(FilterAreaClosedDoorThatHasBeenOpened);

	FNavigationFilterArea FilterAreaTrappedDoor{};
	FilterAreaTrappedDoor.AreaClass = UNavArea_TrappedDoor::StaticClass();
	FilterAreaTrappedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaTrappedDoor);

	FNavigationFilterArea FilterAreaLockedDoor{};
	FilterAreaLockedDoor.AreaClass = UNavArea_LockedDoor::StaticClass();
	FilterAreaLockedDoor.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoor);
	
	FNavigationFilterArea FilterAreaLockedDoorSuspect{};
	FilterAreaLockedDoorSuspect.AreaClass = UNavArea_LockedDoorSuspect::StaticClass();
	FilterAreaLockedDoorSuspect.bIsExcluded = true;
	Areas.Add(FilterAreaLockedDoorSuspect);
	
	FNavigationFilterArea FilterAreaWallHole{};
	FilterAreaWallHole.AreaClass = UNavArea_WallTraversalHole::StaticClass();
	FilterAreaWallHole.bIsExcluded = false;
	Areas.Add(FilterAreaWallHole);

	FNavigationFilterArea FilterAreaCSGas{};
	FilterAreaCSGas.AreaClass = UNavArea_CSGas::StaticClass();
	FilterAreaCSGas.bIsExcluded = false;
	Areas.Add(FilterAreaCSGas);
}

UNavQuery_Awareness::UNavQuery_Awareness()
{
	
}
