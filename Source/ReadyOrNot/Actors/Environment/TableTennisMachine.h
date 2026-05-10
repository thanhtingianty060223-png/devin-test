// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "TableTennisMachine.generated.h"

USTRUCT()
struct FTableTennisGame
{
	GENERATED_BODY()

	void Update(float DeltaTime, bool bLeftAI, bool bRightAI);
	void Reset();
	
	FVector2D GetLeftPaddlePosition() const;
	FVector2D GetRightPaddlePosition() const;

	static float ClampPaddlePosition(float Position);
	void RandomizeTargetOffset();
	
	UPROPERTY(EditAnywhere)
	float TimeToStart = 0.0f;

	UPROPERTY(EditAnywhere)
	float GameOverPauseTime = 0.0f;
	
	UPROPERTY(EditAnywhere)
	float LeftPaddlePosition = 0.0f;

	UPROPERTY(EditAnywhere)
	float RightPaddlePosition = 0.0f;

	UPROPERTY(EditAnywhere)
	FVector2D BallPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	FVector2D BallVelocity = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	int32 LeftScore = 0;

	UPROPERTY(EditAnywhere)
	int32 RightScore = 0;

	bool bLeftHit = false;
	bool bRightHit = false;

	bool bIgnoreLeftAI = false;
	bool bIgnoreRightAI = false;

	bool bServeLeft = false;
	float AITargetOffset = 0.0f;
	
	static const FIntPoint GameSize;
	static const FIntPoint PaddleSize;
	static const FIntPoint BallSize;
	static const float StartingSpeed;
	static const float PlayerSpeed;
	static const float AISpeed;
	static const float DeadZone;
	static const float PaddleDeadZone;
	static const float PaddleOffset;
	static const float SpeedupRate;
};

UCLASS()
class READYORNOT_API ATableTennisPlayer : public AActor
{
	GENERATED_BODY()

public:
	ATableTennisPlayer();
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY()
	class ATableTennisMachine* TargetGame;
	
	float Position = 0.0f;
	
private:
	void MoveForward(float Input);
	void MoveRight(float Input) {}
	void Use();

	UFUNCTION(Server, Reliable)
	void Server_SetPosition(float NewPosition);
	
	UFUNCTION(Server, Reliable)
	void Server_StopPlaying();
};

UCLASS()
class READYORNOT_API ATableTennisMachine : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	ATableTennisMachine();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, class UInteractableComponent* InInteractableComponent) override; 
	
	const FTableTennisGame& GetGameState() const { return Game; }

	// Distance from the player view where we stop rendering the game screen for performance
	UPROPERTY(EditAnywhere)
	float GameRenderDistance = 1200.0f;

	UPROPERTY(EditAnywhere)
	int32 MaxScore = 5;
	
protected:
	void DrawGame();
	ATableTennisPlayer* CreatePlayer(AReadyOrNotPlayerController* PlayerController);

	UPROPERTY(Replicated)
	FTableTennisGame Game;

	UPROPERTY(Replicated)
	ATableTennisPlayer* LeftPlayer;

	UPROPERTY(Replicated)
	ATableTennisPlayer* RightPlayer;

	UPROPERTY(BlueprintReadOnly)
	UTextureRenderTarget2D* GameRenderTarget;

	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 bHasInsertedCoin : 1;
	void OnRep_HasInsertedCoin();
	
	float FlashInsertCoinTime = 0.0f;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UInteractableComponent* LeftPlayerInteractable;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UInteractableComponent* RightPlayerInteractable;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UInteractableComponent* InsertCoinInteractable;
	
	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* NumberTexture;

	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* BackgroundTexture;

	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* StartTexture;

	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* ReadyTexture;

	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* InsertCoinTexture;

	UPROPERTY(EditDefaultsOnly, Category="Game Assets")
	UTexture2D* GameOverTexture;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnCoinInserted();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLeftPaddleHit();

	UFUNCTION(BlueprintImplementableEvent)
	void OnRightPaddleHit();
};
