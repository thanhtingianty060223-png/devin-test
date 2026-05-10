// Copyright Void Interactive, 2023

#include "BpWidgetLib.h"
#include "ReadyOrNot.h"
#include "HttpModule.h"
#include "Actors/Items/Tablet.h"

void UBpWidgetLib::DrawLineWithThickness(FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, FLinearColor Tint, bool bAntiAlias, float Thickness, FVector2D Offset)
{
	Context.MaxLayer++;

	TArray<FVector2D> Points;
	Points.Add(PositionA + Offset);
	Points.Add(PositionB + Offset);

	FSlateDrawElement::MakeLines(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		Points,
		ESlateDrawEffect::None,
		Tint,
		bAntiAlias,
		Thickness);
}

void UBpWidgetLib::DrawLinesWithThickness(FPaintContext& Context, const TArray<FVector2D>& Points, FLinearColor Tint, bool bAntiAlias, float Thickness, FVector2D Offset)
{
	Context.MaxLayer++;

	TArray<FVector2D> OffsetPoints = Points;
	for (int32 i = 0; i < OffsetPoints.Num(); i++)
	{
		OffsetPoints[i] += Offset;
	}

	FSlateDrawElement::MakeLines(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		OffsetPoints,
		ESlateDrawEffect::None,
		Tint,
		bAntiAlias,
		Thickness);
}

void UBpWidgetLib::DrawLineWithCenteredOffset(FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, FLinearColor Tint, bool bAntiAlias, float Thickness, FVector2D Offset, FVector2D Center, float Scale)
{
	Context.MaxLayer++;

	TArray<FVector2D> Points;
	FVector2D CenterDistance;
	
	CenterDistance = (PositionA - Center);
	Points.Add(Offset + (CenterDistance * Scale) + Center);

	CenterDistance = (PositionB - Center);
	Points.Add(Offset + (CenterDistance * Scale) + Center);

	FSlateDrawElement::MakeLines(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		Points,
		ESlateDrawEffect::None,
		Tint,
		bAntiAlias,
		Thickness);
}

void UBpWidgetLib::DrawLinesWithCenteredOffset(FPaintContext& Context, const TArray<FVector2D>& Points, FLinearColor Tint, bool bAntiAlias, float Thickness, FVector2D Offset, FVector2D Center, float Scale)
{
	Context.MaxLayer++;

	TArray<FVector2D> OffsetPoints = Points;
	for (int32 i = 0; i < OffsetPoints.Num(); i++)
	{
		OffsetPoints[i] -= Center;
		OffsetPoints[i] *= Scale;
		OffsetPoints[i] += Center;
		OffsetPoints[i] += Offset;
	}

	FSlateDrawElement::MakeLines(
		Context.OutDrawElements,
		Context.MaxLayer,
		Context.AllottedGeometry.ToPaintGeometry(),
		OffsetPoints,
		ESlateDrawEffect::None,
		Tint,
		bAntiAlias,
		Thickness);
}

bool UBpWidgetLib::PostBugReport(FString Summary, FString Description, FString Category)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
		return false;



	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	FString APIKey = XorString("bKnwo2zxYDMBCSRqeAGjFVDCtAV1oUyc");
	Request->SetHeader(TEXT("Authorization"), APIKey);
	Request->SetVerb("POST");
	Request->SetURL(XorString("https://bt.voidinteractive.net/api/rest/issues/"));

	JsonObject->SetStringField("summary", "Submitted from Game Client: " + Summary);
	FString ProjectVersion;
	if (GConfig)
	{
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GeneralProjectSettings"),
			TEXT("ProjectVersion"),
			ProjectVersion,
			GGameIni
		);
	}
	Description += "\r";
	Description += "Created On Version: " + ProjectVersion;
	FString OSVersionName, OSVersionLabel;
	FPlatformMisc::GetOSVersions(OSVersionName, OSVersionLabel);
	JsonObject->SetStringField("system_os", OSVersionName + " " + OSVersionLabel);
	FString BuildInformation = "Ready Or Not " + ProjectVersion;
	Description += "\r";
	Description += "\r Memory: " + FString::FromInt(FGenericPlatformMemory::GetPhysicalGBRam()) + " GB";
	Description += "\r CPU Brand: " + FPlatformMisc::GetCPUBrand();
	Description += "\r GPU: " + FPlatformMisc::GetPrimaryGPUBrand();

	Description += "\r---------\r";
	if (UBpGameplayHelperLib::GetLocalPlayerController(nullptr))
	{
		Description += UBpGameplayHelperLib::GetAdditionalBugReportInformation(UBpGameplayHelperLib::GetLocalPlayerController(nullptr));
	}


	JsonObject->SetStringField("description", Description);
	TArray< TSharedPtr<FJsonValue> > CategoryArray;
	TSharedPtr<FJsonObject> CategoryObj = MakeShareable(new FJsonObject);
	CategoryObj->SetStringField("name", Category);
	JsonObject->SetObjectField("category", CategoryObj);
	TArray< TSharedPtr<FJsonValue> > ProjectArray;
	TSharedPtr<FJsonObject> ProjectObj = MakeShareable(new FJsonObject);
	ProjectObj->SetStringField("name", "Ready Or Not Public");
	JsonObject->SetObjectField("project", ProjectObj);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(OutputString);

	if (!GEngine)
		return false;

	if (GEngine->GameViewport)
	{
		UWorld* WorldContext = GEngine->GameViewport->GetWorld();
		if (WorldContext)
		{
			for (TActorIterator<AReadyOrNotGameState> It(WorldContext); It; ++It)
			{
				Request->OnProcessRequestComplete().BindUObject(*It, &AReadyOrNotGameState::OnPostBugReportResponse);
			}
		}
	}
	return Request->ProcessRequest();
}

FText UBpWidgetLib::ChangeStringTableTextKey(const FText Target, const FString& NewKey)
{
	FName TableId = FName();
	FString OldKey = FString();
	bool bSuccess = FTextInspector::GetTableIdAndKey(Target, TableId, OldKey);

	FText Result = Target;
	if (bSuccess)
		Result = FText::FromStringTable(TableId, NewKey);

	return Result;
}

FVector2D UBpWidgetLib::GetNormalizedClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D LocalClick = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	FVector2D LocalSize = InGeometry.GetLocalSize();
	
	return LocalClick / LocalSize;
}

bool UBpWidgetLib::IsWorldTearingDown(const UObject* WorldContextObject)
{
	// If our outermost isn't a world then this doesn't apply
	if (!WorldContextObject || !WorldContextObject->GetWorld())
		return false;
	
	return WorldContextObject->GetWorld()->bIsTearingDown;
}

ATablet* UBpWidgetLib::GetPlayerTablet(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
		return nullptr;
	
	APlayerCharacter* PlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(World);
	if (!PlayerCharacter)
		return nullptr;

	UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent();
	if (!InventoryComponent)
		return nullptr;

	ATablet* Tablet = InventoryComponent->GetInventoryItemOfClass_Native<ATablet>(ATablet::StaticClass());
	return Tablet;
}

void UBpWidgetLib::PlayEventFromTablet(const UObject* WorldContextObject, UFMODEvent* Event)
{
	ATablet* Tablet = GetPlayerTablet(WorldContextObject);
	if (Tablet)
		Tablet->PlaySoundEvent(Event);
}
