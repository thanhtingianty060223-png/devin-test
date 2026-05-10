// Copyright Void Interactive, 2021

#pragma once

#include "Components/StaticMeshComponent.h"
#include "DestructibleDoorChunkComponent.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UDestructibleDoorChunkComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

	UDestructibleDoorChunkComponent();

	UPROPERTY()
	bool bIsDoorHandle = false;
	UPROPERTY()
	bool bIsHinge = false;
	UPROPERTY()
	float Health = 1500.0f;
	UPROPERTY()
	bool bCannotKickIfDestroyed = false;
	//Supporting these chunks
	UPROPERTY()
	TArray<class UDestructibleDoorChunkComponent*> SupportChunks;

public:
	virtual void BeginPlay() override;
	virtual void OnVisibilityChanged() override;
	virtual void PostLoad() override;

	void Restore();

	void DoDamage(float Damage);
	void SetIsDoorHandle(bool bNewDoorHandle);
	void SetIsHinge(bool bNewIsHinge);
	bool IsHinge() { return bIsHinge; }
	bool IsDoorHandle() { return bIsDoorHandle; }
	bool IsDestroyed() { return Health <= 0.0f; }
	void SetCannotKickIfDestroyed(bool bNewCannotKickIfDestroyed);
	bool IsUnkickablePiece();
	void AddSupportChunk(class UDestructibleDoorChunkComponent* Chunk);
	void CheckSupportChunks();
	
	void OnChunkDestroyed();
};
