// Copyright Void Interactive, 2023

#include "MissionPlanWidget.h"
#include "ReadyOrNot.h"
#include "TeamViewWidget.h"
#include "Actors/Environment/MissionPortal.h"
#include "Actors/Items/Tablet.h"
#include "Info/MissionPlanManager.h"
#include "lib/BpWidgetLib.h"

void SMissionPlanLinesWidget::Construct(const FArguments& InArgs)
{
	MissionPlanManager = InArgs._MissionPlanManager;
	
	if (InArgs._LineMaterial)
	{
		Brush = MakeShareable(new FSlateMaterialBrush(*InArgs._LineMaterial, FVector2D(64.0f, 64.0f)));
		LineBrushRenderProxy = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
	}

	LineWidth = InArgs._LineWidth;
	FirstNodeRadius = InArgs._FirstNodeRadius;
	NodeRadius = InArgs._NodeRadius;

	PreviewLine = FPlanningLine();
	bOnlyDrawPreviewLine = InArgs._bOnlyDrawPreviewLine;
}

int32 SMissionPlanLinesWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Planning Line Rendering
	if (!MissionPlanManager.IsValid())
		return LayerId;

	if (!LineBrushRenderProxy.IsValid())
		return LayerId;

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	
	if (!bOnlyDrawPreviewLine)
	{
		for (const FPlanningLine& Line : MissionPlanManager->LineArray.Items)
		{
			DrawPlanningLine(Line, AllottedGeometry, Vertices, Indices);
		}
	}
	
	DrawPlanningLine(PreviewLine, AllottedGeometry, Vertices, Indices);

	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, LineBrushRenderProxy, Vertices, Indices, nullptr, 0, 0);
	return LayerId;
}

void SMissionPlanLinesWidget::DrawPlanningLine(const FPlanningLine& Line, const FGeometry& AllottedGeometry,
	TArray<FSlateVertex>& Vertices, TArray<SlateIndex>& Indices) const
{
	if (Line.Floor != CurrentFloor)
		return;

	if (Line.Points.Num() < 2)
		return;

	for (int32 i = 0; i < Line.Points.Num(); i++)
	{
		if (!Line.Points.IsValidIndex(i + 1))
			break;

		FVector2D Current = Line.Points[i];
		FVector2D Next = Line.Points[i + 1];

		FVector2D Origin = AllottedGeometry.GetAbsolutePosition();
		FVector2D Area = AllottedGeometry.GetAbsoluteSize();

		Current = Origin + Current * Area;
		Next = Origin + Next * Area;

		// Ignore lines that are too short
		if (FVector2D::Distance(Current, Next) < (i == 0 ? FirstNodeRadius : NodeRadius) + NodeRadius)
			continue;

		FVector2D Direction = Next - Current;
		Direction.Normalize();

		// Offset the start and end of our lines by the offset radius of our points
		Current += Direction * (i == 0 ? FirstNodeRadius : NodeRadius);
		Next += Direction * -NodeRadius;

		// Create a zeroed slate vertex to copy
		FSlateVertex ZeroVertex;
		FMemory::Memzero(&ZeroVertex, sizeof(FSlateVertex));

		FVector2D Right = FVector2D(Direction.Y, -Direction.X);
		Right.Normalize();

		FVector2D RightHalfLineWidth = Right * LineWidth * 0.5f;
		float LineLength = FVector2D::Distance(Current, Next);

		FSlateVertex BottomRight = ZeroVertex;
		// ##UE5UPGRADE## Lots of double float fixes below, check later
		BottomRight.SetPosition(FVector2f(Current + RightHalfLineWidth));
		BottomRight.SetTexCoords(FVector4f(1.0f, 0.0f, 1.0f, 0.0f));
		BottomRight.Color = FColor::White;
		BottomRight.MaterialTexCoords = FVector2f(1.0f, 0.0f);

		FSlateVertex BottomLeft = ZeroVertex;
		BottomLeft.SetPosition(FVector2f(Current - RightHalfLineWidth));
		BottomLeft.SetTexCoords(FVector4f(0.0f, 0.0f, 0.0f, 0.0f));
		BottomLeft.Color = FColor::White;
		BottomLeft.MaterialTexCoords = FVector2f(0.0f, 0.0f);

		FSlateVertex TopRight = ZeroVertex;
		TopRight.SetPosition(FVector2f(Next + RightHalfLineWidth));
		TopRight.SetTexCoords(FVector4f(1.0f, LineLength, 1.0f, LineLength));
		TopRight.Color = FColor::White;
		TopRight.MaterialTexCoords = FVector2f(1.0f, LineLength);

		FSlateVertex TopLeft = ZeroVertex;
		TopLeft.SetPosition(FVector2f(Next - RightHalfLineWidth));
		TopLeft.SetTexCoords(FVector4f(0.0f, LineLength, 0.0f, LineLength));
		TopLeft.Color = FColor::White;
		TopLeft.MaterialTexCoords = FVector2f(0.0f, LineLength);

		int32 BottomRightIndex = Vertices.Add(BottomRight);
		int32 BottomLeftIndex = Vertices.Add(BottomLeft);
		int32 TopRightIndex = Vertices.Add(TopRight);
		int32 TopLeftIndex = Vertices.Add(TopLeft);

		Indices.Add(TopLeftIndex);
		Indices.Add(BottomLeftIndex);
		Indices.Add(TopRightIndex);
		Indices.Add(TopRightIndex);
		Indices.Add(BottomRightIndex);
		Indices.Add(BottomLeftIndex);
	}
}

void UMissionPlanLinesWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateWidget.Reset();
}

void UMissionPlanLinesWidget::SetCurrentFloor(int32 Floor)
{
	if (SlateWidget.IsValid())
		SlateWidget->SetCurrentFloor(Floor);
}

void UMissionPlanLinesWidget::SetPreviewLine(const FPlanningLine& Line)
{
	if (SlateWidget.IsValid())
		SlateWidget->SetPreviewLine(Line);
}

TSharedRef<SWidget> UMissionPlanLinesWidget::RebuildWidget()
{
	AMissionPlanManager* MissionPlanManager = nullptr;
	for (TActorIterator<AMissionPlanManager> It(GetWorld()); It; ++It)
	{
		MissionPlanManager = *It;
		break;
	}
	
	SlateWidget = SNew(SMissionPlanLinesWidget)
		.MissionPlanManager(TWeakObjectPtr<AMissionPlanManager>(MissionPlanManager))
		.LineMaterial(LineMaterial)
		.LineWidth(LineWidth)
		.FirstNodeRadius(FirstNodeRadius)
		.NodeRadius(NodeRadius)
		.bOnlyDrawPreviewLine(bOnlyDrawPreviewLine);
	
	return SlateWidget.ToSharedRef();
}

void UMissionPlanWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateURL();
}

void UMissionPlanWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!GetWorld())
		return;

	if (!Brush.IsValid() && PlanLineMaterial)
	{
		Brush = MakeShareable(new FSlateMaterialBrush(*PlanLineMaterial, FVector2D(64.0f, 64.0f)));
		LineBrushRenderProxy = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
	}
	
	UpdateURL();
	
	for (TActorIterator<AMissionPlanManager> It(GetWorld()); It; ++It)
	{
		MissionPlanManager = *It;
		break;
	}

	if (!MissionPlanManager)
		return;

	// Remove any existing listeners
	MissionPlanManager->MarkerArray.OnPlanningMarkerAdded.RemoveAll(this);
	MissionPlanManager->MarkerArray.OnPlanningMarkerRemoved.RemoveAll(this);
	MissionPlanManager->LineArray.OnPlanningLineAdded.RemoveAll(this);
	MissionPlanManager->LineArray.OnPlanningLineRemoved.RemoveAll(this);
	
	// Setup listeners for new replicated markers
	MissionPlanManager->MarkerArray.OnPlanningMarkerAdded.AddUObject(this, &UMissionPlanWidget::HandleMarkerAdded);
	MissionPlanManager->MarkerArray.OnPlanningMarkerRemoved.AddUObject(this, &UMissionPlanWidget::HandleMarkerRemoved);

	// Setup listeners for new replicated lines
	MissionPlanManager->LineArray.OnPlanningLineAdded.AddUObject(this, &UMissionPlanWidget::HandleLineAdded);
	MissionPlanManager->LineArray.OnPlanningLineRemoved.AddUObject(this, &UMissionPlanWidget::HandleLineRemoved);

	// Add all existing markers
	for (FPlanningMarker& Marker : MissionPlanManager->MarkerArray.Items)
	{
		HandleMarkerAdded(Marker);
	}

	// Add all existing lines
	for (FPlanningLine& Line : MissionPlanManager->LineArray.Items)
	{
		HandleLineAdded(Line);
	}
}

int32 UMissionPlanWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!DrawingTargetWidget)
		return LayerId;
	
	if (!GetPlayerContext().IsValid())
		return LayerId;
	
	AGameStateBase* GameState = GetPlayerContext().GetGameState();
	if (!GameState)
		return LayerId;

	FGeometry TargetGeometry = AllottedGeometry;
	if (DrawingTargetWidget)
		TargetGeometry = DrawingTargetWidget->GetCachedGeometry();
	
	for (APlayerState* Player : GameState->PlayerArray)
	{
		AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(Player);
		FVector2D LocalSize = TargetGeometry.GetLocalSize();

		// Render existing free draw lines
		for (const FPlanningDrawing& Drawing : PlayerState->DrawingArray.Items)
		{
			if (Drawing.Floor != DrawingFloor)
				continue;
			
			if (Drawing.Points.Num() < 2)
				continue;
		
			FLinearColor Color = LineColor;
			Color.A = FMath::Clamp(1.0f - (Drawing.Time / FadeTime), 0.0f, 1.0f);
			
			TArray<FVector2D> PointsScaled = Drawing.Points;
			for (FVector2D& Point : PointsScaled)
			{
				Point *= LocalSize;
			}

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, TargetGeometry.ToPaintGeometry(), PointsScaled,
				ESlateDrawEffect::None, Color, true, LineThickness);
		}

		// Render the current free drawing line if needed
		if (PlayerState->CurrentDrawing.Floor == DrawingFloor &&
			PlayerState->CurrentDrawing.Points.Num() >= 2)
		{
			TArray<FVector2D> PointsScaled = PlayerState->CurrentDrawing.Points;
			for (FVector2D& Point : PointsScaled)
			{
				Point *= LocalSize;
			}
			
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, TargetGeometry.ToPaintGeometry(), PointsScaled,
				ESlateDrawEffect::None, ActiveLineColor, true, LineThickness);
		}
	}

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void UMissionPlanWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	FinishDrawing();
}

void UMissionPlanWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	FinishDrawing();
}

FReply UMissionPlanWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DrawingAudioComponent)
	{
		DrawingAudioComponent->Stop();
		DrawingAudioComponent = nullptr;
	}
	
	if (!GetPlayerContext().IsValid())
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	
	AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(GetPlayerContext().GetPlayerState());
	if (!PlayerState)
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (DrawingTargetWidget && InMouseEvent.GetModifierKeys().IsControlDown())
		{
			FGeometry TargetGeometry = InGeometry;
			if (DrawingTargetWidget)
				TargetGeometry = DrawingTargetWidget->GetCachedGeometry();
			
			FVector2D NormalizedClick = UBpWidgetLib::GetNormalizedClick(TargetGeometry, InMouseEvent);

			if (!PlayerState->HasAuthority())
			{
				PlayerState->CurrentDrawing = FPlanningDrawing();
				PlayerState->CurrentDrawing.Points.Add(NormalizedClick);
				PlayerState->CurrentDrawing.Floor = DrawingFloor;
			}
			PlayerState->Server_StartDrawing(DrawingFloor, NormalizedClick);
			
			bIsDrawing = true;
			
			ATablet* Tablet = UBpWidgetLib::GetPlayerTablet(GetOwningPlayer());
			if (Tablet)
			{
				DrawingAudioComponent = UFMODBlueprintStatics::PlayEventAttached(DrawingSoundEvent, Tablet->ItemMesh,
					NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
			}
			
			return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
		}
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMissionPlanWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!GetPlayerContext().IsValid())
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	
	if (bIsDrawing)
	{

		return FReply::Handled().ReleaseMouseCapture();
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMissionPlanWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!GetPlayerContext().IsValid())
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	// Tablet exists in the world so mouse movement might occur even if the actual mouse has not occurred
	// Ignore mouse events if the actual cursor hasn't moved
	if (InMouseEvent.GetCursorDelta().IsZero())
	{
		if (DrawingAudioComponent)
		{
			DrawingAudioComponent->SetParameter("mouseSpeed", 0.0f);
		}
		
		return FReply::Handled();
	}
	
	if (bIsDrawing)
	{		
		AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(GetPlayerContext().GetPlayerState());
		if (!PlayerState)
			return FReply::Handled();

		ensure(PlayerState->CurrentDrawing.Points.Num() > 0);
		if (PlayerState->CurrentDrawing.Points.Num() <= 0)
			return FReply::Handled();
		
		FGeometry TargetGeometry = InGeometry;
		if (DrawingTargetWidget)
			TargetGeometry = DrawingTargetWidget->GetCachedGeometry();
		
		FVector2D CurrentPosition = UBpWidgetLib::GetNormalizedClick(TargetGeometry, InMouseEvent);
		FVector2D LastPosition = PlayerState->CurrentDrawing.Points.Last();

		if (DrawingAudioComponent)
		{
			float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / 60.0f);
			float Speed = InMouseEvent.GetCursorDelta().Size() * DeltaTime * (1.0f / 0.5f);
			DrawingAudioComponent->SetParameter("mouseSpeed", Speed);
			
			FVector2D MouseLocation = InMouseEvent.GetScreenSpacePosition();
			FVector2D MouseArea = FVector2D(1440.0f, 1080.0f);
			FVector2D PanningScale = FVector2D(60.0f, 32.0f);

			FVector2D RelativeLocation2D = (2.0f * MouseLocation - MouseArea) * (FVector2D(1.0f) / MouseArea) * PanningScale;
			FVector RelativeLocation = FVector(RelativeLocation2D.Y, 0.0f, -RelativeLocation2D.X);
			
			DrawingAudioComponent->SetRelativeLocation(RelativeLocation);
		}
		
		if (PlayerState->CurrentDrawing.Points.Num() >= AReadyOrNotPlayerState::MaxDrawingPoints)
			return FReply::Handled();

		float Tolerance = 1.0f / TargetGeometry.GetAbsoluteSize().GetMin();
		if (!CurrentPosition.Equals(LastPosition, Tolerance))
		{
			if (!PlayerState->HasAuthority())
			{
				PlayerState->CurrentDrawing.Points.Add(CurrentPosition);
				PlayerState->CurrentDrawing.Floor = DrawingFloor;
			}
			PlayerState->Server_UpdateDrawing(CurrentPosition);
		}

		return FReply::Handled();
	}
	
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMissionPlanWidget::FinishDrawing()
{
	bIsDrawing = false;

	if (DrawingAudioComponent)
	{
		DrawingAudioComponent->Stop();
		DrawingAudioComponent = nullptr;
	}
		
	AReadyOrNotPlayerState* PlayerState = Cast<AReadyOrNotPlayerState>(GetPlayerContext().GetPlayerState());
	if (!PlayerState)
		return;
		
	PlayerState->CurrentDrawing.Floor = DrawingFloor;
		
	if (!PlayerState->HasAuthority())
		PlayerState->DrawingArray.Items.Add(PlayerState->CurrentDrawing);
		
	PlayerState->Server_FinishDrawing();
	PlayerState->CurrentDrawing = FPlanningDrawing();
}

void UMissionPlanWidget::UpdateURL()
{
	if (!GetWorld())
		return;
	
	if (UReadyOrNotFunctionLibrary::IsInLobby())
	{
		// TODO(killo): don't do this lol
		for (TActorIterator<AMissionPortal> It(GetWorld()); It; ++It)
		{
			FString NewURL = It->GetFormattedMissionURL();
			if (CurrentURL != NewURL)
			{
				CurrentURL = NewURL;
				
				FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(CurrentURL);
				OnMissionChanged(CurrentURL, LevelData);
			}

			if (CurrentEntryPoint != It->SelectedEntryPoint)
			{
				CurrentEntryPoint = It->SelectedEntryPoint;
				
				FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(CurrentURL);
				for (const FEntryPoint& EntryPoint : LevelData.EntryPoints)
				{
					if (EntryPoint.Tag == CurrentEntryPoint)
					{
						OnEntryPointChanged(EntryPoint);
						break;
					}
				}
			}
			break;
		}
	}
	else
	{
		FString NewURL = GetWorld()->URL.ToString();
		if (CurrentURL != NewURL)
		{
			CurrentURL = NewURL;
			OnMissionChanged(GetWorld()->URL.ToString(), UBpGameplayHelperLib::GetLevelData(GetWorld()));
		}
	}
}

void UMissionPlanWidget::SetEntryPoint(FName EntryPoint)
{
	// NOTE(killo): the tablet cant change entry points anymore
	//AMissionPortal::SetSelectedEntryPoint(EntryPoint);
}

void UMissionPlanWidget::AddMarker(const FPlanningMarker& Marker)
{
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (!PlayerController)
		return;
	
	PlayerController->Server_AddMarker(Marker);
}

void UMissionPlanWidget::RemoveMarker(int32 ID)
{
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (!PlayerController)
		return;
	
	PlayerController->Server_RemoveMarker(ID);
}

void UMissionPlanWidget::AddLine(const FPlanningLine& Line)
{
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (!PlayerController)
		return;
	
	PlayerController->Server_AddLine(Line);
}

void UMissionPlanWidget::RemoveLine(int32 ID)
{
	AReadyOrNotPlayerController* PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (!PlayerController)
		return;

	PlayerController->Server_RemoveLine(ID);
}
