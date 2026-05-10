// Copyright Void Interactive, 2021

#include "PlayerStart_VIP_Spawn.h"

#include "GameModes/VIPEscortGM.h"
#include "GameModes/VIPEscortGS.h"

APlayerStart_VIP_Spawn::APlayerStart_VIP_Spawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Box"));
	SpawnBox->InitBoxExtent({256.0f, 256.0f, 10.0f});
	SpawnBox->SetMobility(EComponentMobility::Static);
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SpawnBox->SetGenerateOverlapEvents(false);
	SpawnBox->SetupAttachment(RootComponent);

#if WITH_EDITOR
	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text Render"));
	TextRender->SetRelativeLocation({0.0f, 0.0f, 130.0f});
	TextRender->HorizontalAlignment = EHTA_Center;
	TextRender->VerticalAlignment = EVRTA_TextCenter;
	TextRender->WorldSize = 100.0f;
	TextRender->Text = FText::FromString("VIP Spawn 1");
	TextRender->SetupAttachment(RootComponent);
	
	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
#endif
	
	PlayerStartTag = "VIP_SPAWN";
	bHasVisited = false;
}

FVector APlayerStart_VIP_Spawn::GetRandomSpawnPoint()
{
	if (!SpawnBox)
	{
		#if WITH_EDITOR
		ULog::Error(CUR_CLASS_FUNC + " | SpawnBox is null. Returning " + GetName() + "'s location");
		#endif
		
		return GetActorLocation();
	}
	
	return UKismetMathLibrary::RandomPointInBoundingBox(SpawnBox->GetComponentLocation(), SpawnBox->GetUnscaledBoxExtent());
}

FRotator APlayerStart_VIP_Spawn::GetSpawnDirection()
{
	if (!SpawnDirection)
	{
		#if WITH_EDITOR
		ULog::Error(CUR_CLASS_FUNC + " | ArrowComponent is null. Returning " + GetName() + "'s rotation");
		#endif
		
		return GetActorRotation();
	}
	
	return SpawnDirection->GetComponentRotation();
}

#if WITH_EDITOR
void APlayerStart_VIP_Spawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyThatChanged = PropertyChangedEvent.GetPropertyName();
	if (PropertyThatChanged == "VIPSpawnDescriptor")
	{
		if (!TextRender)
		{
			ULog::Error(CUR_CLASS_FUNC + " | TextRender is null");
		
			return;
		}

		TextRender->SetText(FText::FromString(VIPSpawnDescriptor.ToString() + " " + FString::FromInt(SuffixNumber)));
	}
}

void APlayerStart_VIP_Spawn::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (!TextRender)
	{
		ULog::Error(CUR_CLASS_FUNC + " | TextRender is null");
		
		return;
	}

	TextRender->SetText(FText::FromString(VIPSpawnDescriptor.ToString() + " " + FString::FromInt(GetHighestSuffixNumber())));
}
#endif

void APlayerStart_VIP_Spawn::BeginPlay()
{
	Super::BeginPlay();
	
	AVIPEscortGM* VIPGM = Cast<AVIPEscortGM>(UGameplayStatics::GetGameMode(this));
	AVIPEscortGS* VIPGS = Cast<AVIPEscortGS>(UGameplayStatics::GetGameState(this));

	// This trigger box should only work in VIP gamemodes and gamestates
	if (!VIPGM && !VIPGS)
	{
		Destroy();
		
		return;
	}
}

void APlayerStart_VIP_Spawn::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!TextRender)
	{
		ULog::Error(CUR_CLASS_FUNC + " | TextRender is null");
		
		return;
	}
	
	SuffixNumber = FCString::Atoi(&TextRender->Text.ToString()[GetNum(TextRender->Text.ToString()) - 1]);
#endif
}

void APlayerStart_VIP_Spawn::PostActorCreated()
{
	Super::PostActorCreated();

#if WITH_EDITOR
	if (!TextRender)
	{
		ULog::Error(CUR_CLASS_FUNC + " | TextRender is null");
		
		return;
	}
	
	TArray<APlayerStart_VIP_Spawn*> VIPSpawns;
	for (TActorIterator<APlayerStart_VIP_Spawn> It(GetWorld()); It; ++It)
	{
		VIPSpawns.Add(*It);
	}

	SuffixNumber = VIPSpawns.Num();

	TextRender->SetText(FText::FromString(VIPSpawnDescriptor.ToString() + " " + FString::FromInt(SuffixNumber)));
#endif
}

void APlayerStart_VIP_Spawn::Destroyed()
{
	Super::Destroyed();

#if WITH_EDITOR
	int32 i = 0;
	for (TActorIterator<APlayerStart_VIP_Spawn> It(GetWorld()); It; ++It)
	{
		APlayerStart_VIP_Spawn* VIPSpawn = *It;
		if (VIPSpawn != this)
		{
			VIPSpawn->SuffixNumber = i+1;
			VIPSpawn->GetTextRender()->SetText(FText::FromString(VIPSpawn->GetVIPSpawnDescriptor().ToString() + " " + FString::FromInt(i+1)));
			i++;
		}
	}
#endif
}

#if !UE_BUILD_SHIPPING
int32 APlayerStart_VIP_Spawn::GetHighestSuffixNumber()
{	
	TArray<APlayerStart_VIP_Spawn*> VIPSpawns;
	for (TActorIterator<APlayerStart_VIP_Spawn> It(GetWorld()); It; ++It)
	{
		VIPSpawns.Add(*It);
	}

	const int32 HighestSuffixNumber = VIPSpawns.Num();

	if (SuffixNumber < HighestSuffixNumber)
		return SuffixNumber;

	return HighestSuffixNumber;
}
#endif
