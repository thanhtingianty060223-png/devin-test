// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Volume.h"
#include "InterestOverrideZone.generated.h"

UENUM(BlueprintType)
enum class EInterestPointType : uint8
{
	Manual,
	Threat,
	Door,
	Spawner,
	CustomActor
};

USTRUCT(BlueprintType)
struct FInterestPoint
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (MakeEditWidget = true, EditCondition = "Type == EInterestPointTYpe::Manual"))
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	EInterestPointType Type = EInterestPointType::Manual;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (EditCondition = "Type == EInterestPointTYpe::Threat", EditConditionHides = true))
	TSoftObjectPtr<class AThreatAwarenessActor> Threat = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (EditCondition = "Type == EInterestPointTYpe::Door", EditConditionHides = true))
	TSoftObjectPtr<class ADoor> Door = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (EditCondition = "Type == EInterestPointTYpe::Spawner", EditConditionHides = true))
	TSoftObjectPtr<class AAISpawn> Spawner = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (EditCondition = "Type == EInterestPointTYpe::CustomActor", EditConditionHides = true))
	TSoftObjectPtr<AActor> CustomActor = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 60.0f))
	float RequiredTimeFocusing = 3.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, EditFixedSize)
	TMap<ACyberneticCharacter*, float> TimeFocusing;
};

USTRUCT(BlueprintType)
struct FInterestStationPoint
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (MakeEditWidget = true))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 60.0f))
	float RequiredTimeStationing = 15.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, EditFixedSize)
	TMap<ACyberneticCharacter*, float> TimeStationing;
};

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AInterestOverrideZone final : public AVolume
{
	GENERATED_BODY()
	
public:	
	AInterestOverrideZone();

	UFUNCTION(BlueprintPure)
	bool GetCurrentInterestInfo(ACyberneticCharacter* AI, FVector& OutLocation, AActor*& OutActor) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	#endif
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* BillboardComponent = nullptr;
	#endif
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (TitleProperty = "Type", ArrayClamp = 64))
	TArray<FInterestPoint> InterestPoints;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup", meta = (TitleProperty = "Location", ArrayClamp = 64))
	TArray<FInterestStationPoint> StationPoints;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Status (Read Only)", EditFixedSize)
	TArray<ACyberneticCharacter*> CharactersInZone;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Status (Read Only)", EditFixedSize)
	TMap<ACyberneticCharacter*, uint8> FocusIndexMap;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Status (Read Only)", EditFixedSize)
	TMap<ACyberneticCharacter*, uint8> MoveIndexMap;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditInstanceOnly, Category = "Debug")
	bool bDrawInGame = false;
	#endif

	UFUNCTION()
	void OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	
	UFUNCTION()
	void OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor);
	
	UFUNCTION()
	void OnWorldBeginPlay();

	void OnAIOverlap(ACyberneticCharacter* AI);

	void GiveMove(ACyberneticCharacter* AI);
};
