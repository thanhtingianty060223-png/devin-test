// Void Interactive, 2020


#include "SpinTestHeatmapVolume.h"

#include "Kismet/KismetStringLibrary.h"

ASpinTestHeatmapVolume::ASpinTestHeatmapVolume()
{
    SetActorEnableCollision(ECollisionEnabled::NoCollision);
}

void ASpinTestHeatmapVolume::VisualizeHeatMapIfExists()
{
    DrawDebugLine(GetWorld(), FVector::ZeroVector, FVector(0,0,1000.0f), FColor::Purple, false, 60.0f, 0, 1);
    FString PerformanceCSV, FilePath;
    FilePath = UKismetSystemLibrary::GetProjectDirectory() + "/PerfTests/" + GetWorld()->GetMapName() + ".spintest";
    FFileHelper::LoadFileToString(PerformanceCSV, *FilePath);

    int32 previousLocIdx = 0;
    FVector PreviousLocation = FVector::ZeroVector;
    TArray<FString> PerformanceCSVLines = UKismetStringLibrary::ParseIntoArray(PerformanceCSV, "\r");
    for (int32 i = 1; i < PerformanceCSVLines.Num(); i++)
    {
        TArray<FString> data = UKismetStringLibrary::ParseIntoArray(PerformanceCSVLines[i], ",");
        FString deltaTime, location, rotation;
        deltaTime = data[0];
        location = data[1];
        rotation = data[2];
        FVector locationAsVector, rotationAsVector;
        bool bNewLoc = false;
        if (PreviousLocation != locationAsVector)
        {
            previousLocIdx = i;
            PreviousLocation = locationAsVector;
            bNewLoc = true;
        }
        locationAsVector.InitFromString(location);
        FRotator rotationAsRotator;
        rotationAsVector.InitFromString(rotation);
        rotationAsRotator = rotationAsVector.Rotation();
		// ##UE5UPGRADE
		float deltaTimeFloat = 1.0f;
		LexFromString(deltaTimeFloat, *deltaTime);
        float FPS = 1.0f/ deltaTimeFloat;
        if (bNewLoc)
        {
            float MinFPS = GetMinFPSAtSpot(locationAsVector, PerformanceCSVLines, previousLocIdx);
            FColor DisplayColor = FColor::White;
            if (MinFPS == -1.0f)
            {
                DisplayColor = FColor::White;
            }
            else if (MinFPS < 30)
            {
                DisplayColor = FColor::Red;
            } else if (MinFPS < 60)
            {
                DisplayColor = FColor::Orange;
            } else if (MinFPS < 120)
            {
                DisplayColor = FColor::Yellow;
            } else
            {
                DisplayColor = FColor::Green;
            }

            DrawDebugLine(GetWorld(), locationAsVector + FVector(0, 0, -1000.0f), locationAsVector + FVector(0,0,1000.0f),
        DisplayColor,
        true, -1.0f, 0, 10);
        }
       
        
        FColor VectorDisplayColor = FColor::White;
        if (FPS < 30)
        {
            VectorDisplayColor = FColor::Red;
        } else if (FPS < 60)
        {
            VectorDisplayColor = FColor::Orange;
        } else if (FPS < 120)
        {
            VectorDisplayColor = FColor::Yellow;
        } else
        {
           VectorDisplayColor = FColor::Green;
        }
        
        DrawDebugLine(GetWorld(), locationAsVector, locationAsVector  + FVector(0,0,-100) + rotationAsRotator.Vector() * 110.0f,
            VectorDisplayColor,
            true, -1.0f, 0, 1);
        //DrawDebugLine(GetWorld(), locationAsVector  + FVector(0,0,1000.0f), locationAsVector + FVector(0,0,1100.0f) + rotationAsRotator.Vector() * 70.0f,
         //  VectorDisplayColor,
         //  true, -1.0f, 0, 1);
    }
}

void ASpinTestHeatmapVolume::FlushVisualization()
{
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "FlushPersistentDebugLines" );
}

float ASpinTestHeatmapVolume::GetMinFPSAtSpot(FVector Location, TArray<FString> Data, int32 index)
{
    float MinFPS = -1.0f;
    // add soem limtis for we're not doing 15000^2.. we know there are only a max of 62 data points per location..
    for (int32 i = index; i < index + 62 && i < Data.Num(); i++)
    {
        TArray<FString> data = UKismetStringLibrary::ParseIntoArray(Data[i], ",");
        FString deltaTime, location, rotation;
        location = data[1];
        FVector locationAsVector;
        locationAsVector.InitFromString(location);
        if (locationAsVector != Location)
            continue;
        
        deltaTime = data[0];
		// ##UE5UPGRADE
		float deltaTimeFloat = 1.0f;
		LexFromString(deltaTimeFloat, *deltaTime);
		float FPS = 1.0f / deltaTimeFloat;
        if (MinFPS == -1.0f || FPS < MinFPS)
        {
            MinFPS = FPS;
        }
    }
    return MinFPS;
}
