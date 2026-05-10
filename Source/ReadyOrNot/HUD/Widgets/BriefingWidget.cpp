// Void Interactive, 2020

#include "BriefingWidget.h"

#include "PreMissionPlanning.h"

#include "Blueprint/SlateBlueprintLibrary.h"

#include "Components/WidgetComponent.h"

void UBriefingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		ACameraActor* Camera = *It;
		if (Camera->Tags.Contains("BriefingCamera_Close"))
		{
			OriginalCameraTransform = Camera->GetActorTransform();
		}
	}
}

void UBriefingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UBriefingWidget::GetWhiteboardObjectiveText(FString& OutString)
{
	int32 ObjectiveNumber = 1;
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	for (TSoftClassPtr<AObjective> Obj : Data.Objectives)
	{
		TSubclassOf<AObjective> LoadedObj = Obj.LoadSynchronous();
		if (!LoadedObj)
			continue;

		AObjective* DefaultObject = LoadedObj.GetDefaultObject();

		FString ObjectiveString = "{0}.) {1}\n{2}\n";
		OutString += FString::Format(*ObjectiveString,
			{
				FString::FromInt(ObjectiveNumber),
				DefaultObject->ObjectiveName.ToString(),
				DefaultObject->ObjectiveDescription.ToString()
			});
		ObjectiveNumber++;
	}
	if (OutString == "")
	{
		OutString += "1.) Bring Order to Chaos";
	}
}

void UBriefingWidget::GetBios(EBioType BioType,TArray<FCharacterBio>& OutBios)
{
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	switch(BioType)
	{
	case EBioType::BT_None: break;
	case EBioType::BT_Suspect:
		OutBios = Data.SuspectsBios;
		break;
	case EBioType::BT_Civilian:
		OutBios = Data.CiviliansBios;
		break;
	default: ;
	}
	
}

void UBriefingWidget::GetBioDetails(FCharacterBio Bio, TArray<FCriminalRecord>& RapSheet, UTexture2D*& ProfileImage, FString& OutDescription,
	FString& OutBioText)
{
	ProfileImage = Bio.ProfileImage.LoadSynchronous();
	OutDescription = "{6}\n{0}\n{1}\n{2}\n{3}\n{4}\n{5}\n";
	OutDescription = FString::Format(*OutDescription,{
		Bio.Sex.ToString(),
		Bio.Build.ToString(),
		Bio.Height.ToString() + " " + Bio.Weight.ToString(),
		Bio.Hair.ToString() + " Hair",
		Bio.Eyes.ToString() + " Eyes",
		Bio.DateOfBirth.ToString(),
		Bio.Name,
		Bio.IdNumber
	});
	OutBioText = Bio.Bio.ToString() + "\n";
}

void UBriefingWidget::GetBriefing(FMissionAudio& OutBriefing)
{
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	/*/OutBriefing = Data.TocBriefingTranscript.ToString();
	if (OutBriefing == "")
	{
		OutBriefing = "We don't have any intel on the situation.";
	}/*/
	OutBriefing = Data.TocBriefingAudio;

}

void UBriefingWidget::GetLevelNickname(FString& OutNickname)
{
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	OutNickname = Data.LevelNickname.ToString();
	if (OutNickname == "")
	{
		OutNickname = "\"Fear not the unknown.\"";
	}
}

void UBriefingWidget::GetWhiteboardTimelineAsString(FString& OutString)
{
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	for (FTimelineEvent Event : Data.MissionTimeline.EventsList)
	{
		FString TimelineText = "{0}		{1}\n";
		OutString += FString::Format(*TimelineText,
			{
				Event.EventTime.ToString(),
				Event.EventDescription.ToString()
			});
	}
	if (OutString == "")
	{
		OutString = "Timeline Unknown";
	}
}

void UBriefingWidget::MoveCameraToMouseCursor(FPointerEvent PointerEvent)
{
	APlayerController* PlayerController =  UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	FVector ScreenPosition, WorldDirection;
	float MouseLocX, MouseLocY;
	PlayerController->GetMousePosition(MouseLocX, MouseLocY);
	UGameplayStatics::DeprojectScreenToWorld(
		PlayerController,
        FVector2D(MouseLocX, MouseLocY),
        ScreenPosition,
        WorldDirection
        );
	FHitResult OutHit;
	GetWorld()->LineTraceSingleByChannel(OutHit,  ScreenPosition,	 ScreenPosition +
        WorldDirection * 500.0f, ECollisionChannel::ECC_Visibility);
	if (bZoomedInOnWhiteboard)
	{
		//Zoom out
		for (TObjectIterator<UPreMissionPlanning> It; It; ++It)
		{
			UPreMissionPlanning* PreMissionPlanning = *It;
			PreMissionPlanning->SetActiveCameraByTag("BriefingCamera_Close");
		}
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
		{
			ACameraActor* Camera = *It;
			if (Camera->Tags.Contains("TempZoomCamera"))
			{
				Camera->Destroy();
			}
		}
		bZoomedInOnWhiteboard = false;
	} else
	{
		if (OutHit.GetActor() && OutHit.GetActor()->Tags.Contains("Whiteboard")
        && Cast<UWidgetComponent>(OutHit.GetComponent()))
		{

			ACameraActor* TempCameraActor = GetWorld()->SpawnActor<ACameraActor>();
			TempCameraActor->SetActorTransform(OriginalCameraTransform);
			FVector NewLocation = OutHit.ImpactPoint + OutHit.ImpactNormal * 50.0f;
			TempCameraActor->SetActorLocation(FMath::Lerp(OriginalCameraTransform.GetLocation(), NewLocation, 0.7f));
			TempCameraActor->Tags.AddUnique("TempZoomCamera");
			TempCameraActor->GetCameraComponent()->FieldOfView = 40.0f;
			TempCameraActor->GetCameraComponent()->SetConstraintAspectRatio(false);
			for (TObjectIterator<UPreMissionPlanning> It; It; ++It)
			{
				UPreMissionPlanning* PreMissionPlanning = *It;
				PreMissionPlanning->SetActiveCameraByTag("TempZoomCamera");
				bZoomedInOnWhiteboard = true;
			}
		}
	}
	
}

void UBriefingWidget::PlayMissionAudio(FString AudioName)
{
	//FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	//for (FMissionAudio MissionAudio : Data.MissionAudio)
	//{
	//	if (MissionAudio.AudioName.ToString() == AudioName)
	//	{
	//		// play audio on reel
	//		for (TActorIterator<AStaticMeshActor>It(GetWorld()); It; ++It)
	//		{
	//			AStaticMeshActor* StaticMeshActor = *It;
	//			if (StaticMeshActor->Tags.Contains("AudioReel"))
	//			{
	//				UAudioComponent* AudioComp = Cast<UAudioComponent>(StaticMeshActor->GetComponentByClass(UAudioComponent::StaticClass()));
	//				if (AudioComp)
	//				{
	//					if (AudioComp->IsPlaying() && LastAudioPlayed == AudioName)
	//					{
	//						AudioComp->Stop();
	//					} else
	//					{
	//						LastAudioPlayed = AudioName;
	//						AudioComp->SetSound(MissionAudio.SoundFile);
	//						AudioComp->Play();
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
}

void UBriefingWidget::DoesLevelHaveMissionAudio(FString AudioName, bool& bHasMissionAudio)
{
	FLevelDataLookupTable Data = UBpGameplayHelperLib::GetLevelData(GetWorld());
	for (FMissionAudio MissionAudio : Data.MissionAudio)
	{
		if (MissionAudio.AudioName.ToString() == AudioName)
		{
			bHasMissionAudio = true;
		}
	}
}
