// Copyright Void Interactive, 2023


#include "Watch.h"


AWatch::AWatch()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.0f;

	WatchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WatchMesh"));
	SetRootComponent(WatchMesh);

	HourHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HourHandMesh"));
	HourHandMesh->SetupAttachment(RootComponent);

	MinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MinuteHandMesh"));
	MinuteHandMesh->SetupAttachment(RootComponent);

	SecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondHandMesh"));
	SecondHandMesh->SetupAttachment(RootComponent);

	DateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DateMesh"));
	DateMesh->SetupAttachment(RootComponent);
}

void AWatch::BeginPlay()
{
	Super::BeginPlay();

	// Get the level time
	FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(UGameplayStatics::GetCurrentLevelName(GetWorld(), true));
	FString LevelTime = LevelData.TimeOfDayText.ToString();

	ensureMsgf(LevelTime.Len() == 4, TEXT("Level time is not in the correct format!"));
	// If no level start time, don't set the time
	if (LevelTime.IsEmpty() || LevelTime.Len() < 4 || LevelTime.Len() > 4)
		return;

	const int32 LevelHour = FCString::Atoi(*LevelTime.Mid(0, 2));
	const int32 LevelMinute = FCString::Atoi(*LevelTime.Mid(2, 4));

	// Set the hour hand
	TimeElapsed += LevelHour * 3600.0f;

	// Set the minute hand
	TimeElapsed += LevelMinute * 60.0f;

	// Set the second hand to a random time
	TimeElapsed += FMath::RandRange(0, 60);
}

void AWatch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeElapsed += DeltaSeconds;

	// Rotate the second hand
	SecondHandMesh->SetRelativeRotation(FRotator(0.0f, 360.0f * (TimeElapsed / 60.0f), 0.0f));

	// Rotate the minute hand
	MinuteHandMesh->SetRelativeRotation(FRotator(0.0f, 360.0f * (TimeElapsed / 3600.0f), 0.0f));

	// Rotate the hour hand
	HourHandMesh->SetRelativeRotation(FRotator(0.0f, 360.0f * (TimeElapsed / 43200.0f), 0.0f));
}
