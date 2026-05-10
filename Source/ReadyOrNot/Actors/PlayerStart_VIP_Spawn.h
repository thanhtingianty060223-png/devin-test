// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/PlayerStart.h"
#include "PlayerStart_VIP_Spawn.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API APlayerStart_VIP_Spawn : public APlayerStart
{
	GENERATED_BODY()

public:
	APlayerStart_VIP_Spawn(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "VIP Spawn")
	FVector GetRandomSpawnPoint();
	
	UFUNCTION(BlueprintPure, Category = "VIP Spawn")
	FRotator GetSpawnDirection();

	UFUNCTION(BlueprintPure, Category = "VIP Spawn")
	FText GetVIPSpawnDescriptor() const { return VIPSpawnDescriptor; }

	#if WITH_EDITOR
	UTextRenderComponent* GetTextRender() const { return TextRender; }
	#endif
	
	UPROPERTY(BlueprintReadOnly, Category = "VIP Spawn")
	uint8 bHasVisited : 1;
	
protected:
	#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
	void PostEditMove(bool bFinished) override;
	#endif

	void BeginPlay() override;
	
	void PostLoad() override;
	void PostActorCreated() override;
	
	void Destroyed() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", DisplayName = "Descriptor")
	FText VIPSpawnDescriptor = FText::FromString("VIP Spawn");
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
	int32 SuffixNumber = 1;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VIP Spawn")
	UBoxComponent* SpawnBox;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VIP Spawn")
	UTextRenderComponent* TextRender;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VIP Spawn")
    UArrowComponent* SpawnDirection;

private:
	#if !UE_BUILD_SHIPPING
	int32 GetHighestSuffixNumber();
	#endif
};
