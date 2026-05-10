// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "ReadyOrNotNavQueries.generated.h"


UCLASS()
class READYORNOT_API UNavQuery_DoorTest : public UNavigationQueryFilter
{
	GENERATED_BODY()
	

	public:
	UNavQuery_DoorTest();
};
/**
 * 
 */
UCLASS()
class READYORNOT_API UNavQuery_Swat : public UNavigationQueryFilter
{
	GENERATED_BODY()

public:
	UNavQuery_Swat();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatFallIn : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatFallIn();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatBreachAndClear : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatBreachAndClear();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatAlpha : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatAlpha();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatBeta : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatBeta();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatCharlie : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatCharlie();
};

UCLASS()
class READYORNOT_API UNavQuery_SwatDelta : public UNavQuery_Swat
{
	GENERATED_BODY()

public:
	UNavQuery_SwatDelta();
};

UCLASS()
class READYORNOT_API UNavQuery_Civilian : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_Civilian();
	
};

UCLASS()
class READYORNOT_API UNavQuery_Suspect : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_Suspect();
	
};

UCLASS()
class READYORNOT_API UNavQuery_FlankingSuspect : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_FlankingSuspect();
	
};

UCLASS()
class READYORNOT_API UNavQuery_GasFleeingSuspect : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_GasFleeingSuspect();
	
};

UCLASS()
class READYORNOT_API UNavQuery_NoiseCheck : public UNavigationQueryFilter
{
	GENERATED_BODY()

	 UNavQuery_NoiseCheck();
	
};

UCLASS()
class READYORNOT_API UNavQuery_CSGas : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_CSGas();
};

UCLASS()
class READYORNOT_API UNavQuery_Awareness : public UNavigationQueryFilter
{
	GENERATED_BODY()

	UNavQuery_Awareness();
};










