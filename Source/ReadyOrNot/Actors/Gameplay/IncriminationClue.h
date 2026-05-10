// Void Interactive, 2020

#pragma once

#include "Actors/PickupActor.h"
#include "IncriminationClue.generated.h"

UENUM(BlueprintType)
enum class EClueState : uint8
{
	Unclaimed,
	Collected,
	Dropped
};

UCLASS(HideCategories=("Rendering", "Collision", "Input", "Actor", "HLOD", "LOD", "Cooking"))
class READYORNOT_API AIncriminationClue : public APickupActor
{
	GENERATED_BODY()
	
public:
	AIncriminationClue();

	UFUNCTION(BlueprintCallable, Category = "Clue")
	void Init(class AIncriminationClueSpawnPoint* OwningSpawn, uint8 InClueNumber, const FText& InClueName, const FText& InClueFoundMessage, float InShowObjectiveMarkerIn = 120.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Clue")
	void RevealNextClue();
	
	UFUNCTION(BlueprintPure, Category = "Clue")
	bool IsClueFound() const;
	
	void ShowClue(bool bStartCountdown = true);
	void HideClue();
	
	void SetNextClue(AIncriminationClue* NewNextClue);
	void SetClueNumber(uint8 NewClueNumber);

	virtual void ActorDropped(AActor* InDroppedInstigator) override;
	
	FORCEINLINE uint8 GetClueNumber() const { return ClueNumber; }
	FORCEINLINE AIncriminationClue* GetNextClue() const { return NextClue; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClueFound, AIncriminationClue*, ClueActor, AActor*, ClueFounder);
	UPROPERTY(BlueprintAssignable, DisplayName = "On Clue Found")
	FOnClueFound Delegate_OnClueFound;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UMapActorComponent* MapActorComponent = nullptr;
	
	friend bool operator>(const AIncriminationClue& Lhs, const AIncriminationClue& Rhs)
	{
		return Lhs.GetClueNumber() > Rhs.GetClueNumber();
	}

	friend bool operator<(const AIncriminationClue& Lhs, const AIncriminationClue& Rhs)
	{
		return Lhs.GetClueNumber() < Rhs.GetClueNumber();
	}
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ActorPickedUp(AActor* InPickupInstigator) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintNativeEvent, Category = "Clue|Events")
			void OnClueFound();
	virtual void OnClueFound_Implementation();
	
	void OnClueTimerExpired();

	UFUNCTION()
	void OnRep_OnClueFound();
	
	UFUNCTION()
	void OnRep_OnClueStateChanged();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Setup", meta = (ClampMin = 1, UIMin = 1))
	uint8 ClueNumber = 1;

	// The amount of time (in seconds) until this clue's location is revealed, if not found
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Setup")
    float ShowObjectiveMarkerIn = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Setup")
    AIncriminationClue* NextClue = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Setup|UI")
    FText ClueName = FText::FromString("Clue");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Setup|UI")
    FText ClueFoundMessage = FText::FromString("Clue Found");
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Data")
	class AIncriminationClueSpawnPoint* SpawnPointOwner = nullptr;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_OnClueFound, Category = "Data")
	uint8 bClueFound : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Data")
	uint8 bClueTimerExpired : 1;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_OnClueStateChanged, Category = "Data")
	EClueState ClueState = EClueState::Unclaimed;
	
private:
	FTimerHandle TH_ClueTimerExpiry;
};

