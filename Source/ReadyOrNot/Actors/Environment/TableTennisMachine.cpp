// Copyright Void Interactive, 2023

#include "TableTennisMachine.h"

#include "Components/InteractableComponent.h"
#include "Math/Box2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"

const FIntPoint FTableTennisGame::GameSize = FIntPoint(256, 256);
const FIntPoint FTableTennisGame::PaddleSize = FIntPoint(6, 32);
const FIntPoint FTableTennisGame::BallSize = FIntPoint(4, 4);
const float FTableTennisGame::StartingSpeed = 90.0f;
const float FTableTennisGame::PlayerSpeed = 110.0f;
const float FTableTennisGame::AISpeed = 80.0f;
const float FTableTennisGame::DeadZone = 8.0f;
const float FTableTennisGame::PaddleDeadZone = 16.0f;
const float FTableTennisGame::PaddleOffset = 8.0f;
const float FTableTennisGame::SpeedupRate = 6.0f;

void FTableTennisGame::Update(float DeltaTime, bool bLeftAI, bool bRightAI)
{
	bLeftHit = false;
	bRightHit = false;

	if (!bLeftAI)
		bIgnoreLeftAI = true;
	if (!bRightAI)
		bIgnoreRightAI = true;

	bLeftAI &= !bIgnoreLeftAI;
	bRightAI &= !bIgnoreRightAI;

	if (GameOverPauseTime > 0.0f)
	{
		GameOverPauseTime -= DeltaTime;
		return;
	}
	
	if (TimeToStart > 0.0f)
	{
		TimeToStart -= DeltaTime;

		static const float CenterPosition = GameSize.Y / 2.0f - PaddleSize.Y / 2.0f;
		LeftPaddlePosition = CenterPosition;
		RightPaddlePosition = CenterPosition;

		return;
	}

	BallPosition += BallVelocity * DeltaTime;

	if (BallPosition.X <= 0 ||
		BallPosition.X >= GameSize.X)
	{
		if (BallPosition.X <= 0)
		{
			RightScore++;
			bServeLeft = true;
		}
		if (BallPosition.X >= GameSize.X)
		{
			LeftScore++;
			bServeLeft = false;
		}

		TimeToStart = 3.0f;
		
		BallPosition = GameSize / 2.0f - BallSize / 2.0f;
		BallPosition.Y = FMath::RandRange(GameSize.Y / 3.0f, (GameSize.Y / 3.0f) * 2.0f);

		float Angle = FMath::DegreesToRadians(FMath::RandRange(-25.0f, 25.0f));
		BallVelocity.X = (bServeLeft ? -1.0f : 1.0f) * FMath::Cos(Angle);
		BallVelocity.Y = FMath::Sin(Angle);
		BallVelocity = BallVelocity.GetSafeNormal() * StartingSpeed;
		
		bIgnoreLeftAI = false;
		bIgnoreRightAI = false;

		RandomizeTargetOffset();
		
		return;
	}
	
	if (BallPosition.Y < DeadZone)
	{
		BallPosition.Y = DeadZone;
		BallVelocity.Y *= -1;
	}
	if (BallPosition.Y > GameSize.Y - DeadZone - BallSize.Y)
	{
		BallPosition.Y = GameSize.Y - DeadZone - BallSize.Y;
		BallVelocity.Y *= -1;
	}
	
	if (bLeftAI && BallVelocity.X < 0.0f)
	{
		float Target = LeftPaddlePosition + PaddleSize.Y / 2.0f + AITargetOffset;
		float Difference = FMath::Abs(BallPosition.Y - Target);
		
		if (Target > BallPosition.Y)
			LeftPaddlePosition -=  FMath::Min(AISpeed * DeltaTime, Difference);
		else
			LeftPaddlePosition +=  FMath::Min(AISpeed * DeltaTime, Difference);
	}
	if (bRightAI && BallVelocity.X > 0.0f)
	{
		float Target = RightPaddlePosition + PaddleSize.Y / 2.0f + AITargetOffset;
		float Difference = FMath::Abs(BallPosition.Y - Target);
		
		if (Target > BallPosition.Y)
			RightPaddlePosition -= FMath::Min(AISpeed * DeltaTime, Difference);
		else
			RightPaddlePosition += FMath::Min(AISpeed * DeltaTime, Difference);
	}

	LeftPaddlePosition = ClampPaddlePosition(LeftPaddlePosition);
	RightPaddlePosition = ClampPaddlePosition(RightPaddlePosition);

	FVector2D LeftPaddle = GetLeftPaddlePosition();
	FVector2D RightPaddle = GetRightPaddlePosition();

	FBox2D LeftPaddleBox(LeftPaddle, LeftPaddle + PaddleSize);
	FBox2D RightPaddleBox(RightPaddle, RightPaddle + PaddleSize);

	FBox2D BallBox(BallPosition, BallPosition + BallSize);
	
	if (LeftPaddleBox.Intersect(BallBox))
	{
		BallPosition.X = LeftPaddle.X + PaddleSize.X;
		BallVelocity.X *= -1;

		float BallBoxCenterY = BallBox.GetCenter().Y;
		float PaddleBoxCenterY = LeftPaddleBox.GetCenter().Y;
		
		float Alpha = (PaddleBoxCenterY - BallBoxCenterY) / PaddleSize.Y;
		BallVelocity.Y = -FMath::Abs(BallVelocity.X) * FMath::Clamp(Alpha, -1.0f, 1.0f);
		
		BallVelocity += BallVelocity.GetSafeNormal() * SpeedupRate;
		
		RandomizeTargetOffset();
		
		bLeftHit = true;
	}
	if (RightPaddleBox.Intersect(BallBox))
	{
		BallPosition.X = RightPaddle.X - BallSize.X;
		BallVelocity.X *= -1;

		float BallBoxCenterY = BallBox.GetCenter().Y;
		float PaddleBoxCenterY = RightPaddleBox.GetCenter().Y;
		
		float Alpha = (PaddleBoxCenterY - BallBoxCenterY) / PaddleSize.Y;
		BallVelocity.Y = -FMath::Abs(BallVelocity.X) * FMath::Clamp(Alpha, -1.0f, 1.0f);
		
		BallVelocity += BallVelocity.GetSafeNormal() * SpeedupRate;
		
		RandomizeTargetOffset();
			
		bRightHit = true;
	}
}

void FTableTennisGame::Reset()
{
	TimeToStart = 5.0f;
	BallPosition = GameSize / 2.0f - BallSize / 2.0f;
	BallVelocity = FVector2D(StartingSpeed, 0.0f);

	LeftScore = 0;
	RightScore = 0;

	bServeLeft = false;
}

FVector2D FTableTennisGame::GetLeftPaddlePosition() const
{
	return FVector2D(PaddleOffset, LeftPaddlePosition);
}

FVector2D FTableTennisGame::GetRightPaddlePosition() const
{
	return FVector2D(GameSize.X - PaddleOffset - PaddleSize.X, RightPaddlePosition);
}

float FTableTennisGame::ClampPaddlePosition(float Position)
{
	return FMath::Clamp(Position, PaddleDeadZone, GameSize.Y - PaddleDeadZone * 1.0f - PaddleSize.Y);
}

void FTableTennisGame::RandomizeTargetOffset()
{
	AITargetOffset = FMath::RandRange(-PaddleSize.Y / 2.0f + 1.0f, PaddleSize.Y  / 2.0f - 1.0f);
}

ATableTennisPlayer::ATableTennisPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bOnlyRelevantToOwner = true;
}

void ATableTennisPlayer::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ATableTennisMachine> It(GetWorld()); It; ++It)
	{
		if (!(*It))
			continue;
		
		TargetGame = *It;
		break;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
		return;
	
	InputComponent = NewObject<UInputComponent>(this);
	InputComponent->RegisterComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("Use", IE_Pressed, this, &ATableTennisPlayer::Use);
		InputComponent->BindAxis("MoveForward", this, &ATableTennisPlayer::MoveForward);
		InputComponent->BindAxis("MoveRight", this, &ATableTennisPlayer::MoveRight);

		EnableInput(PlayerController);
	}
}

void ATableTennisPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Destroy player if disconnected
	if (HasAuthority() && !IsValid(GetOwner()))
	{
		Destroy();
	}
}

void ATableTennisPlayer::MoveForward(float Input)
{
	if (!GetWorld() || !TargetGame || TargetGame->GetGameState().TimeToStart > 0.0f)
		return;
	
	Input = FMath::Clamp(Input, -1.0f, 1.0f);
	Position -= Input * FTableTennisGame::PlayerSpeed * GetWorld()->GetDeltaSeconds();
	
	Server_SetPosition_Implementation(Position);
	Server_SetPosition(Position);
}

void ATableTennisPlayer::Use()
{
	DisableInput(nullptr);
	Server_StopPlaying();
	Destroy();
}

void ATableTennisPlayer::Server_SetPosition_Implementation(float NewPosition)
{
	Position = FTableTennisGame::ClampPaddlePosition(NewPosition);
}

void ATableTennisPlayer::Server_StopPlaying_Implementation()
{
	Destroy();
}

ATableTennisMachine::ATableTennisMachine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	LeftPlayerInteractable = CreateDefaultSubobject<UInteractableComponent>(TEXT("LeftPlayerInteractable"));
	LeftPlayerInteractable->SetupAttachment(RootComponent);
	LeftPlayerInteractable->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PlayAsLeftPaddle"));
	LeftPlayerInteractable->ActionSlot1.bCondition = false;
	
	RightPlayerInteractable = CreateDefaultSubobject<UInteractableComponent>(TEXT("RightPlayerInteractable"));
	RightPlayerInteractable->SetupAttachment(RootComponent);
	RightPlayerInteractable->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PlayAsRightPaddle"));
	RightPlayerInteractable->ActionSlot1.bCondition = false;
	
	InsertCoinInteractable = CreateDefaultSubobject<UInteractableComponent>(TEXT("InsertCoinInteractable"));
	InsertCoinInteractable->SetupAttachment(RootComponent);
	InsertCoinInteractable->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "InsertCoin"));
	InsertCoinInteractable->ActionSlot1.bCondition = true;

	bHasInsertedCoin = false;
}

void ATableTennisMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATableTennisMachine, Game);
	DOREPLIFETIME(ATableTennisMachine, LeftPlayer);
	DOREPLIFETIME(ATableTennisMachine, RightPlayer);
	DOREPLIFETIME(ATableTennisMachine, bHasInsertedCoin);
}

void ATableTennisMachine::BeginPlay()
{
	GameRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 256, 256, RTF_R8);
	if (GameRenderTarget)
	{
		GameRenderTarget->Filter = TF_Nearest;
		GameRenderTarget->AddressX = TA_Clamp;
		GameRenderTarget->AddressY = TA_Clamp;
	}
	
	Super::BeginPlay();

	InsertCoinInteractable->ActionSlot1.bCondition = !bHasInsertedCoin;
}

void ATableTennisMachine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DeltaTime = FMath::Min(0.08f, DeltaTime);
	
	LeftPlayerInteractable->ActionSlot1.bCondition = !IsValid(LeftPlayer) && bHasInsertedCoin;
	RightPlayerInteractable->ActionSlot1.bCondition = !IsValid(RightPlayer) && bHasInsertedCoin;
	
	FlashInsertCoinTime += DeltaTime;
	if (FlashInsertCoinTime > 2.0f)
		FlashInsertCoinTime = 0.0f;
	
	const float RubberbandDistance = Game.TimeToStart <= 0.0f ? 50.0f : 0.0f;
	if (IsValid(LeftPlayer))
	{
		if (FMath::Abs(LeftPlayer->Position - Game.LeftPaddlePosition) > RubberbandDistance)
		{
			LeftPlayer->Position = Game.LeftPaddlePosition;
		}
			
		Game.LeftPaddlePosition = LeftPlayer->Position;
	}
	if (IsValid(RightPlayer))
	{
		if (FMath::Abs(RightPlayer->Position - Game.RightPaddlePosition) > RubberbandDistance)
		{
			RightPlayer->Position = Game.RightPaddlePosition;
		}
			
		Game.RightPaddlePosition = RightPlayer->Position;
	}
	
	if (HasAuthority())
	{
		Game.Update(DeltaTime, !IsValid(LeftPlayer), !IsValid(RightPlayer));
	}
	else
	{
		Game.Update(DeltaTime, false, false); // Predict
	}
	
	if (Game.bLeftHit)
		OnLeftPaddleHit();
	
	if (Game.bRightHit)
		OnRightPaddleHit();

	if (HasAuthority() && (Game.LeftScore >= MaxScore || Game.RightScore >= MaxScore))
	{
		if (LeftPlayer)
		{
			LeftPlayer->Destroy();
			LeftPlayer = nullptr;
		}
		if (RightPlayer)
		{
			RightPlayer->Destroy();
			RightPlayer = nullptr;
		}

		bHasInsertedCoin = false;
		OnRep_HasInsertedCoin();

		Game.GameOverPauseTime = 3.0f;
		
		Game.Reset();
	}
	
	APlayerCharacter* PlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (PlayerCharacter)
	{
		float Distance = FVector::Distance(PlayerCharacter->GetPawnViewLocation(), GetActorLocation());

		if (Distance <= GameRenderDistance)
			DrawGame();
	}
}

void ATableTennisMachine::DrawGame()
{
	UCanvas* Canvas = nullptr;
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), GameRenderTarget, Canvas, Size, Context);

	if (!Canvas)
		return;
	
	FVector2D LeftPaddle = Game.GetLeftPaddlePosition();
	FVector2D RightPaddle = Game.GetRightPaddlePosition();

	if (BackgroundTexture)
		Canvas->K2_DrawTexture(BackgroundTexture, FVector2D(0.0f), FTableTennisGame::GameSize, FVector2D(0.0f));

	Canvas->K2_DrawTexture(nullptr, LeftPaddle, Game.PaddleSize, FVector2D::ZeroVector);
	Canvas->K2_DrawTexture(nullptr, RightPaddle, Game.PaddleSize, FVector2D::ZeroVector);
	Canvas->K2_DrawTexture(nullptr, Game.BallPosition, Game.BallSize, FVector2D::ZeroVector);

	// Render scores
	if (NumberTexture)
	{
		const FVector2D NumberTextureSize = NumberTexture->GetImportedSize();
		const float NumberStridePixels = NumberTextureSize.X / 10.0f;
		const float NumberStride = NumberStridePixels / NumberTextureSize.X; // normalized

		const float Scale = 4.0f;

		const FVector2D NumberSize = FVector2D(NumberStridePixels, NumberTextureSize.Y) * Scale;

		Canvas->K2_DrawTexture(NumberTexture, FVector2D(102, 8), NumberSize, FVector2D(NumberStride * Game.LeftScore, 0.0f), FVector2D(NumberStride, 1.0f));
		Canvas->K2_DrawTexture(NumberTexture, FVector2D(140, 8), NumberSize, FVector2D(NumberStride * Game.RightScore, 0.0f), FVector2D(NumberStride, 1.0f));
	}

	// Render ready/start/gameover texture
	if (Game.GameOverPauseTime > 0.0f && GameOverTexture)
	{
		const FIntPoint GameOverTextureSize = FIntPoint(GameOverTexture->GetSizeX(), GameOverTexture->GetSizeY()) * 3.0f;
		const FIntPoint GameOverPosition = FTableTennisGame::GameSize / 2.0f - GameOverTextureSize / 2.0f;

		Canvas->K2_DrawTexture(GameOverTexture, GameOverPosition, GameOverTextureSize, FVector2D::ZeroVector);
	}
	else if (Game.TimeToStart > 1.0f && ReadyTexture)
	{
		const FIntPoint ReadyTextureSize = FIntPoint(ReadyTexture->GetSizeX(), ReadyTexture->GetSizeY()) * 3.0f;
		const FIntPoint ReadyPosition = FTableTennisGame::GameSize / 2.0f - ReadyTextureSize / 2.0f;

		Canvas->K2_DrawTexture(ReadyTexture, ReadyPosition, ReadyTextureSize, FVector2D::ZeroVector);
	}
	else if (Game.TimeToStart > 0.0f && StartTexture)
	{
		const FIntPoint StartTextureSize = FIntPoint(StartTexture->GetSizeX(), StartTexture->GetSizeY()) * 3.0f;
		const FIntPoint StartPosition = FTableTennisGame::GameSize / 2.0f - StartTextureSize / 2.0f;

		Canvas->K2_DrawTexture(StartTexture, StartPosition, StartTextureSize, FVector2D::ZeroVector);
	}

	// Render the insert coin texture
	if (!bHasInsertedCoin && InsertCoinTexture && FlashInsertCoinTime >= 1.0f)
	{
		const FIntPoint StartTextureSize = FIntPoint(InsertCoinTexture->GetSizeX(), InsertCoinTexture->GetSizeY()) * 3.0f;
		FIntPoint StartPosition = FTableTennisGame::GameSize / 2.0f - StartTextureSize / 2.0f;
		StartPosition.Y = 36;

		Canvas->K2_DrawTexture(InsertCoinTexture, StartPosition, StartTextureSize, FVector2D::ZeroVector);
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
}

void ATableTennisMachine::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator)
		return;

	bool bCanResetGame = !IsValid(LeftPlayer) && !IsValid(RightPlayer);
	
	if (InInteractableComponent == InsertCoinInteractable)
	{
		bHasInsertedCoin = true;
		OnRep_HasInsertedCoin();
	}
	else if (InInteractableComponent == LeftPlayerInteractable && !IsValid(LeftPlayer))
	{
		AReadyOrNotPlayerController* PlayerController = InteractInstigator->GetRONPlayerController();
		LeftPlayer = CreatePlayer(PlayerController);
	}
	else if (InInteractableComponent == RightPlayerInteractable && !IsValid(RightPlayer))
	{
		AReadyOrNotPlayerController* PlayerController = InteractInstigator->GetRONPlayerController();
		RightPlayer = CreatePlayer(PlayerController);
	}

	if (bCanResetGame && (IsValid(LeftPlayer) || IsValid(RightPlayer)))
		Game.Reset();
}

ATableTennisPlayer* ATableTennisMachine::CreatePlayer(AReadyOrNotPlayerController* PlayerController)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerController;

	ATableTennisPlayer* NewPlayer = GetWorld()->SpawnActor<ATableTennisPlayer>(
		ATableTennisPlayer::StaticClass(), GetActorTransform(), SpawnParameters);

	if (NewPlayer)
		NewPlayer->TargetGame = this;
	
	return NewPlayer;
}

void ATableTennisMachine::OnRep_HasInsertedCoin()
{
	InsertCoinInteractable->ActionSlot1.bCondition = !bHasInsertedCoin;

	if (bHasInsertedCoin)
		OnCoinInserted();
}
