// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "PopupTarget.generated.h"

UCLASS()
class READYORNOT_API APopupTarget : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* Mesh;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;


	UPROPERTY(EditAnywhere, Category = Gameplay)
	int32 MaxHealth;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	float PopupTime;

	UPROPERTY(Replicated)
	float Health;

	FTimerHandle PopupTimer;

protected:

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Animation)
	bool bFallDown;

	void FallDown();

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void Popup();
	
public:	
	// Sets default values for this actor's properties
	APopupTarget();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool IsAlive();

	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return Mesh; }
	
};
