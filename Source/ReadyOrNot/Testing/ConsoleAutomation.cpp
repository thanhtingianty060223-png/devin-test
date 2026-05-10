#include "ConsoleAutomation.h"
#include <Tests/AutomationCommon.h>

#include "CoreMinimal.h"
#include "Stats/StatsData.h"
#if !UE_BUILD_SHIPPING
#include "IAutomationControllerModule.h"

READYORNOT_API DECLARE_LOG_CATEGORY_EXTERN(LogConsoleAutomation, Log, All);
DEFINE_LOG_CATEGORY(LogConsoleAutomation);

namespace TestHelpers
{
	UWorld* GetWorld()
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			return GWorld;
		}
#endif // WITH_EDITOR
		return GEngine->GetWorldContexts()[0].World();
	}
}

class READYORNOT_API FConsoleAutomationLoadMap : public IAutomationLatentCommand
{
public:
	FConsoleAutomationLoadMap(const FString& InMapName) : MapName(InMapName), bTravelStarted(false)
	{}

	virtual bool Update() override
	{
		if (!bTravelStarted)
		{
			StartTime = FDateTime::Now();
			UE_LOG(LogConsoleAutomation, Log, TEXT("Open %s"), *MapName);
			FString OpenCommand = FString::Printf(TEXT("Open %s"), *MapName);
			GEngine->Exec(TestHelpers::GetWorld(), *OpenCommand);
			bTravelStarted = true;
		}
		else
		{
			auto TravelUrl = GEngine->GetWorldContexts()[0].TravelURL;
			auto LevelsLoaded = TestHelpers::GetWorld()->AreAlwaysLoadedLevelsLoaded();
			
			if (TravelUrl.IsEmpty() && LevelsLoaded && UReadyOrNotFunctionLibrary::HasStartedMatch(TestHelpers::GetWorld()))
			{
				UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("MapLoadTime"), (FDateTime::Now() - StartTime).GetTotalMilliseconds());
				return true;
			}
		}
		return false;
	}

private:
	FString MapName;
	bool bTravelStarted;
	FDateTime StartTime;
};

class READYORNOT_API FConsoleAutomationStatStartFile : public IAutomationLatentCommand
{
public:
	FConsoleAutomationStatStartFile()
	{}

	virtual bool Update() override
	{
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("stat startfile")));
		return true;
	}
};

class READYORNOT_API FConsoleAutomationStatStopFile : public IAutomationLatentCommand
{
public:
	FConsoleAutomationStatStopFile()
	{}

	virtual bool Update() override
	{
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("stat stopfile")));
		return true;
	}
};

class READYORNOT_API FConsoleAutomationPause : public IAutomationLatentCommand
{
public:
	FConsoleAutomationPause()
	{}

	virtual bool Update() override
	{
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("pause")));
		return true;
	}
};

class READYORNOT_API FConsoleAutomationMemory : public IAutomationLatentCommand
{
public:
	FConsoleAutomationMemory()
	{}

	virtual bool Update() override
	{
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("memreport -full")));
		return true;
	}
};

class READYORNOT_API FConsoleStatUnit : public IAutomationLatentCommand
{
public:
	FConsoleStatUnit()
	{}

	virtual bool Update() override
	{
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("stat none")));
		GEngine->Exec(TestHelpers::GetWorld(), *FString::Printf(TEXT("stat unit")));
		return true;
	}
};

class READYORNOT_API FConsoleAutomationWriteStatsToLog : public IAutomationLatentCommand
{
public:
	FConsoleAutomationWriteStatsToLog(const FString& InMapName) : MapName(InMapName)
	{}

	virtual bool Update() override
	{
		const FStatUnitData* StatUnitData = TestHelpers::GetWorld()->GetGameViewport()->GetStatUnitData();

		// testresult to the log to be gathered by external script, ; separated
		// TestResult;Level;MetricName;MetricValue
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("RenderThreadTime"), StatUnitData->RenderThreadTime);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("GameThreadTime"), StatUnitData->GameThreadTime);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("GPUFrameTime"), StatUnitData->GPUFrameTime[0]);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("FrameTime"), StatUnitData->FrameTime);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("RHITTime"), StatUnitData->RHITTime);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%f"), *MapName, TEXT("InputLatencyTime"), StatUnitData->InputLatencyTime);

		const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();

		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("TotalPhysical"), Stats.TotalPhysical);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("TotalVirtual"), Stats.TotalVirtual);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("UsedPhysical"), Stats.UsedPhysical);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("UsedVirtual"), Stats.UsedVirtual);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("PeakUsedPhysical"), Stats.PeakUsedPhysical);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("PeakUsedVirtual"), Stats.PeakUsedVirtual);

		FTextureMemoryStats TexMemStats;
		RHIGetTextureMemoryStats(TexMemStats);

		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("DedicatedVideoMemory"), TexMemStats.DedicatedVideoMemory);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("DedicatedSystemMemory"), TexMemStats.DedicatedSystemMemory);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("SharedSystemMemory"), TexMemStats.SharedSystemMemory);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("TotalGraphicsMemory"), TexMemStats.TotalGraphicsMemory);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("AllocatedMemorySize"), TexMemStats.AllocatedMemorySize);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("LargestContiguousAllocation"), TexMemStats.LargestContiguousAllocation);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("TexturePoolSize"), TexMemStats.TexturePoolSize);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%lld"), *MapName, TEXT("PendingMemoryAdjustment"), TexMemStats.PendingMemoryAdjustment);
		
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%ld"), *MapName, TEXT("NumDrawCalls"), GNumDrawCallsRHI[0]);
		UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%ld"), *MapName, TEXT("NumPrimitives"), GNumPrimitivesDrawnRHI[0]);

		// log staticmesh memory use

		TArray<FStatMessage> StatMessages;
		GetPermanentStats(StatMessages);

		for(const FStatMessage& Item : StatMessages)
		{
			FString Result(TEXT("Invalid"));

			bool bIsOk = false;
			
			switch (Item.NameAndInfo.GetField<EStatDataType>())
			{
				case EStatDataType::ST_int64:
					if (Item.NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration))
					{
						if (Item.GetValue_int64()!=0) // ignore 0
						{ 
							Result = FString::Printf(TEXT("%.3fms (%4d)"), FPlatformTime::ToMilliseconds(FromPackedCallCountDuration_Duration(Item.GetValue_int64())), FromPackedCallCountDuration_CallCount(Item.GetValue_int64()));
							bIsOk = true;
						}
					}
					else if (Item.NameAndInfo.GetFlag(EStatMetaFlags::IsCycle))
					{
						if (Item.GetValue_int64()!=0) // ignore 0
						{
							
							Result = FString::Printf(TEXT("%.3fms"), FPlatformTime::ToMilliseconds64(Item.GetValue_int64()));
							bIsOk = true;
						}
					}
					else
					{
						if (Item.GetValue_int64() != 0) // ignore 0
						{
							Result = FString::Printf(TEXT("%llu"), Item.GetValue_int64());
							bIsOk = true;
						}
					}
					break;
				case EStatDataType::ST_double:
					if (Item.GetValue_double()>0.0) // ignore 0.0
					{
						Result = FString::Printf(TEXT("%.1f"), Item.GetValue_double());
						bIsOk = true;
					}
					break;
				case EStatDataType::ST_FName:
					// Enable below to get FNames valuess
					// Result = Item.GetValue_FName().ToString();
					break;
			}

			const FString ShortName = Item.NameAndInfo.GetShortName().ToString();

			if (bIsOk)
			{
				UE_LOG(LogConsoleAutomation, Log, TEXT("TestResult;%s;%s;%s"), *MapName, *ShortName, *Result);
			}
		}
		
		return true;
	}

private:
	FString MapName;
};



IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMapLoadTest, "ReadyOrNot.LoadLevelTimings", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

void FMapLoadTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	// add more maps and make it controllable from outside of the test later
	
	OutBeautifiedNames.Add(TEXT("Station"));
	OutBeautifiedNames.Add(TEXT("Datacenter"));
	OutBeautifiedNames.Add(TEXT("Gas"));
	OutBeautifiedNames.Add(TEXT("Sins"));
	OutBeautifiedNames.Add(TEXT("Meth"));
	OutBeautifiedNames.Add(TEXT("Coyote"));
	OutBeautifiedNames.Add(TEXT("Streamer"));
	OutBeautifiedNames.Add(TEXT("Agency"));
	OutBeautifiedNames.Add(TEXT("Valley"));
	OutBeautifiedNames.Add(TEXT("Club"));
	OutBeautifiedNames.Add(TEXT("Campus"));
	OutBeautifiedNames.Add(TEXT("Hospital"));
	OutBeautifiedNames.Add(TEXT("Ridgeline"));
	OutBeautifiedNames.Add(TEXT("Penthouse"));
	OutBeautifiedNames.Add(TEXT("Farm"));
	OutBeautifiedNames.Add(TEXT("Importer"));
	OutBeautifiedNames.Add(TEXT("Beachfront"));
	OutBeautifiedNames.Add(TEXT("Dealer"));
	OutBeautifiedNames.Add(TEXT("Port"));


	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Station/Levels/Editable/RoN_Station_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Datacenter_New/Levels/Editable/Missions/RoN_Datacenter_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Gas/Levels/Editable/Missions/RoN_Gas_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Sins/Levels/Editable/Missions/RoN_Sins_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Meth/Levels/Editable/Missions/RoN_Meth_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Coyote/Levels/Editable/Missions/RoN_Coyote_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Streamer/Levels/Editable/Missions/RoN_Streamer_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Agency/Level/Editable/Missions/RoN_Agency_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Valley/Levels/Editable/Missions/RoN_Valley_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Club/Levels/Editable/Missions/RoN_Club_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_School/Levels/Editable/Missions/RoN_Campus_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Hospital/Levels/Editable/Missions/RoN_Hospital_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Ridgeline/Level/Missions/RoN_Ridgeline_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Penthouse/Level/Missions/RoN_Penthouse_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Farm/Levels/Editable/Missions/RoN_Farm_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Importer/Levels/Editable/Missions/RoN_Importer_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Beachfront/Levels/Editable/Missions/RoN_Beachfront_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Dealer/Levels/Editable/Missions/RoN_Dealer_BarricadedSuspects_Core"));
	OutTestCommands.Add(TEXT("/Game/ReadyOrNot/Level/RoN_Port/Editable/Missions/RoN_Port_BarricadedSuspects_Core"));
}

bool FMapLoadTest::RunTest(const FString& MapName)
{
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationLoadMap(MapName));
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleStatUnit());
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(10.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationWriteStatsToLog(MapName)); // dump stats to log
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationStatStartFile());	
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(5.0f));		// 5s profile
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationPause());			// pause Gamethread
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(5.0f));		// 5s profile
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationStatStopFile());
	ADD_LATENT_AUTOMATION_COMMAND(FConsoleAutomationMemory());			// MemReport to file
	return true;
}
#endif