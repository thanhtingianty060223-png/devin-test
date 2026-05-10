// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "ThrownItem.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class READYORNOT_API AThrownItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AThrownItem();
	
	FORCEINLINE UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	FORCEINLINE class UAIPerceptionStimuliSourceComponent* GetPerceptionStimuliComponent() const { return PerceptionStimuliComp; }

	void FullySimulateThrowPath(FVector ThrowDirection, float DistMultiplier ,FVector ForcedStartPoint = FVector::ZeroVector);

	UPROPERTY(BlueprintReadOnly)
	AReadyOrNotCharacter* ThrowInstigator = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FName ThrownInRoom = NAME_None;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMesh = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliComp = nullptr;

	UPROPERTY()
	float TurnPhysicsOffDelay;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float ThrowDistance = 1300.0f;
	
	UPROPERTY()
	TArray<FVector> FirstBouncePath;
	UPROPERTY()
	FHitResult FirstBounceHit;
	bool bAppliedFirstBounce = false;
	UPROPERTY()
	TArray<FVector> SecondBouncePath;
	UPROPERTY()
	FHitResult SecondBounceHit;
	bool bAppliedSecondBounce = false;
	UPROPERTY()
	TArray<FVector> ThirdBouncePath;
	UPROPERTY()
	FHitResult ThirdBounceHit;
	bool bAppliedThirdBounce = false;

	UPROPERTY(ReplicatedUsing = OnRep_ThrowPath)
	TArray<FVector_NetQuantize> CompletePath;

	UFUNCTION()
	void OnRep_ThrowPath();
	
	UPROPERTY(Replicated)
	int32 BouncePt1;
	UPROPERTY(Replicated)
	int32 BouncePt2;
	UPROPERTY(Replicated)
	int32 BouncePt3;

	int32 pathIdx = 0;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float ThrowSpeed = 1500.0f;
	float DefaultThrowSpeed = 0.0f;
	float MaxThrowSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float ThrowBounciness = 1.0f;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit );

	//Path Prediction Functions
	
	UFUNCTION(Server, Reliable, WithValidation)
			void UpdateServerPath(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3);
	virtual void UpdateServerPath_Implementation(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3);
	virtual bool UpdateServerPath_Validate(const TArray<FVector_NetQuantize>& Path, int32 Bounce1, int32 Bounce2, int32 Bounce3) { return true; }
	
	void TravelAlongSimulatedPath(float DeltaTime);

	bool IsLocallyControlled() const;

	TArray<AActor*> GetIgnoredActorsForThrow();
};
