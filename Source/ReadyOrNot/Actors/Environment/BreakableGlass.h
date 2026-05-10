// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakableGlass.generated.h"

UCLASS()
class READYORNOT_API ABreakableGlass : public AActor
{
	GENERATED_BODY()
	
public:	
	ABreakableGlass();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void ConvertHitAndExecute(FHitResult Hit, float Damage);
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void FirstHitPositionObject(int32 FirstPositionBox, int32 TextureY, int32 TextureX, FVector HitPosition, FVector ObjectiveDirection, float DamageRadius, bool bFirstHitCanBreakIt, float CharacterVelocityToBreak);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void DestructibleHit(FVector Location);

	UPROPERTY(BlueprintReadWrite)
	bool bCanSoundPass = false;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ConvertHitAndExecute(int32 FirstPositionBox, int32 TextureX, int32 TextureY, FVector_NetQuantize HitPosition, FVector_NetQuantize Direction, float Damage);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_DestructibleHit(FVector_NetQuantize Location);
};
