// Void Interactive, 2020

#include "ReadyOrNotSpinTestController.h"

#include "ReadyOrNotGameMode.h"
#include "SpinTestHeatmapVolume.h"

void UReadyOrNotSpinTestController::OnInit()
{
    UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotSpinTestController started"));
    TotalTestTime -= SpinUpTime;
}

bool SaveStringTextToFile(FString SaveDirectory, FString JoyfulFileName, FString SaveText, bool AllowOverWriting)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*SaveDirectory))
	{
		return false;
	}
	
	SaveDirectory += "\\";
	SaveDirectory += JoyfulFileName;
	
	if (!AllowOverWriting)
	{
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*SaveDirectory))
		{
			return false;
		}
	}
	
	return FFileHelper::SaveStringToFile(SaveText, * SaveDirectory);
}

void UReadyOrNotSpinTestController::StartTesting()
{
    //StartProfiling();
}

void UReadyOrNotSpinTestController::StartProfiling()
{
    //FCsvProfiler::Get()->BeginCapture();
 
    // set a timer for when profiling should end
    //FTimerHandle dummy;
    //GetWorld()->GetTimerManager().SetTimer(dummy, this, &UReadyOrNotSpinTestController::StopProfiling, ProfilingTime, false);

}

void UReadyOrNotSpinTestController::StopProfiling()
{
    UE_LOG(LogGauntlet, Display, TEXT("Stopping the profiler"));
 
    TSharedFuture<FString> future = FCsvProfiler::Get()->EndCapture();
    const FString SpintestSaveDir = UKismetSystemLibrary::GetProjectDirectory() + "../../../../PerfTests/";
    const FString SpintestSaveFile = GetWorld()->GetMapName() + ".spintest";;
    UE_LOG(LogGauntlet, Display, TEXT("Saving Spintest!! %s%s"), *SpintestSaveDir, *SpintestSaveFile);
 
    SaveStringTextToFile( SpintestSaveDir, SpintestSaveFile, PerformanceCSV, true);
    
    // launch an async task that polls the Future for completion
    // will in turn launch a task on the game thread once the CSV file is saved to disk
    AsyncTask(ENamedThreads::AnyThread, [this, future]()
        {
            while (!future.IsReady())
                FPlatformProcess::SleepNoStats(0);
 
            AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    StopTesting();
                }
            );
        }
    );
}

void UReadyOrNotSpinTestController::OnTick(float DeltaTime)
{
    //TODO: this is where you can put stuff that should happen on tick
    if (GetWorld())
    {
        if (!bStartedTesting)
        {
            PerformanceCSV += "FPS, Location, Direction\r";
            // Ready Up
            for (TActorIterator<AReadyOrNotPlayerState> It(GetWorld()); It; ++It)
            {
                AReadyOrNotPlayerState* ps = *It;
                ps->SetReady(true, ps->GetLoadout());
            }
            for (TActorIterator<AReadyOrNotGameMode> It(GetWorld()); It; ++It)
            {
                AReadyOrNotGameMode* gm = *It;
                if (gm->GetMatchState() != EMatchState::MS_Playing)
                {
                    //gm->StartMatch();
                }
            }
            ASpectatorPawn* Spectator = GetWorld()->SpawnActor<ASpectatorPawn>(ASpectatorPawn::StaticClass());
            for (TActorIterator<AReadyOrNotPlayerController> It(GetWorld()); It; ++It)
            {
                AReadyOrNotPlayerController* ps = *It;
                if (ps->GetPawn())
                {
                    ps->GetPawn()->Destroy();
                }
                ps->SetPawn(Spectator);
            }
            UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotGauntletController is about to start testing..."));
            bStartedTesting = true;
            FTimerHandle dummy, dummy2;
            GetWorld()->GetTimerManager().SetTimer(dummy, this, &UReadyOrNotSpinTestController::StartTesting, SpinUpTime, false);
            GetWorld()->GetTimerManager().SetTimer(dummy2, this, &UReadyOrNotSpinTestController::On60FPSTick, 1.0f/60.0f, true, 1.0f/60.f);
        }
    } 
}

void UReadyOrNotSpinTestController::On60FPSTick()
{
    float DeltaTime = 1.0f/60.0f;
    if (TimeAtSpot > 1.0f || LastSpot == FVector::ZeroVector)
    {
        UE_LOG(LogGauntlet, Display, TEXT("Time At Spot %s, LastSpot: %s"), *FString::SanitizeFloat(TimeAtSpot), *LastSpot.ToString());
        MoveToNewSpot();
        TimeAtSpot = 0.0f;
    }
    TimeAtSpot += DeltaTime;
    for (TActorIterator<ASpectatorPawn> It(GetWorld()); It; ++It)
    {
        ASpectatorPawn* pc = *It;
        if (pc)
        {
            PerformanceCSV += FString::SanitizeFloat(GetWorld()->GetDeltaSeconds()) + "," + LastSpot.ToString() + "," + pc->GetActorForwardVector().ToString() + "\r";
            pc->AddActorLocalRotation(FRotator(0.0f, 360.0f * DeltaTime, 0.0f));
            break;
        }
                
    }
}

void UReadyOrNotSpinTestController::MoveToNewSpot()
{
    bool bHasASpinTest = false;
    for (TActorIterator<ASpinTestHeatmapVolume> It(GetWorld()); It; ++It)
    {
        ASpinTestHeatmapVolume* sp = *It;
        if (sp)
        {
            bHasASpinTest = true;
            if (LastSpot == FVector::ZeroVector)
            {
                BeginningCorner = sp->GetBounds().Origin - sp->GetBounds().BoxExtent;
                BeginningCorner.Z = sp->GetActorLocation().Z;
                LastSpot = BeginningCorner;
                DrawDebugBox(GetWorld(), LastSpot, FVector(100.0f), FColor::Orange, false, 1.0f, 1, 1.0f);
            } else
            {
                FVector NewSpot = LastSpot + FVector(sp->GetBounds().BoxExtent.X * 0.1f, 0.0f, 0.0f);
                if (sp->EncompassesPoint(NewSpot))
                {
                    LastSpot = NewSpot;
                    DrawDebugBox(GetWorld(), LastSpot, FVector(100.0f), FColor::Orange, false, 1.0f, 1, 1.0f);
                } else
                {
                    NewSpot = LastSpot + FVector(0.0f, sp->GetBounds().BoxExtent.Y * 0.1f, 0.0f);
                    NewSpot.X = BeginningCorner.X;
                    if (sp->EncompassesPoint(NewSpot))
                    {
                        LastSpot = NewSpot;
                        DrawDebugBox(GetWorld(), LastSpot, FVector(100.0f), FColor::Orange, false, 1.0f, 1, 1.0f);
                    } else
                    {
                        StopProfiling();
                    }
                }
                
            }
        }
    }
    for (TActorIterator<ASpectatorPawn> It(GetWorld()); It; ++It)
    {
        ASpectatorPawn* pc = *It;
        if (pc)
        {
            pc->SetActorLocation(LastSpot);
        }
    }
    if (!bHasASpinTest)
    {
        UE_LOG(LogGauntlet, Display, TEXT("Add a SpinTestHeatMapVolume! Exiting..."));
        StopProfiling();
    }
}

void UReadyOrNotSpinTestController::StopTesting()
{
    UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotGauntletController stopped"));
    EndTest(0); 
}


