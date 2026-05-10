// Copyright Void Interactive, 2021


#include "ReadyOrNotGauntletTestController.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Async/Async.h"
#include "ReadyOrNotGameMode.h"
 
void UReadyOrNotGauntletTestController::OnInit()
{
    UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotGauntletController started"));
    TotalTestTime -= SpinUpTime;
}
 
void UReadyOrNotGauntletTestController::StartTesting()
{
    //TODO: this is where you put your custom game code that should be run before profiling starts
 
    StartProfiling();
}
 
void UReadyOrNotGauntletTestController::StartProfiling()
{
    FCsvProfiler::Get()->BeginCapture();
 
    // set a timer for when profiling should end
    FTimerHandle dummy;
    GetWorld()->GetTimerManager().SetTimer(dummy, this, &UReadyOrNotGauntletTestController::StopProfiling, ProfilingTime, false);
}
 
void UReadyOrNotGauntletTestController::StopProfiling()
{
    UE_LOG(LogGauntlet, Display, TEXT("Stopping the profiler"));
 
    TSharedFuture<FString> future = FCsvProfiler::Get()->EndCapture();
 
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
 
void UReadyOrNotGauntletTestController::OnTick(float DeltaTime)
{
    //TODO: this is where you can put stuff that should happen on tick
    if (GetWorld())
    {
        if (!bStartedTesting)
        {
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
                    gm->StartMatch();
                }
            }
            UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotGauntletController is about to start testing..."));
            bStartedTesting = true;
            FTimerHandle dummy;
            GetWorld()->GetTimerManager().SetTimer(dummy, this, &UReadyOrNotGauntletTestController::StartTesting, SpinUpTime, false);
        }
    } 
}
 
void UReadyOrNotGauntletTestController::StopTesting()
{
    UE_LOG(LogGauntlet, Display, TEXT("ReadyOrNotGauntletController stopped"));
    EndTest(0);
}