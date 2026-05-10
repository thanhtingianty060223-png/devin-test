// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
// #include "DestructibleComponent.h"
#include "GlassActor.generated.h"

UCLASS()
class READYORNOT_API AGlassActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGlassActor();

	// UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Door, meta = (AllowPrivateAccess = "true"))
	// 	class UDestructibleComponent* DestructibleWindow;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Material that is applied to the glass on spawn
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Visuals)
		TArray<UMaterialInstance*> RandomGlassMaterial;

	// Material that is applied to the glass when it is shattered
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Visuals)
		TArray<UMaterialInstance*> RandomShatteredGlassMaterial;

	UFUNCTION(BlueprintCallable, Category = Visuals)
	UMaterialInterface* GetRandomGlassMaterial();

	UFUNCTION(BlueprintCallable, Category = Visuals)
	UMaterialInterface* GetRandomShatteredGlassMaterial();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);
	virtual void Multicast_TakeDamage_Implementation(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_ApplyDamageToWindow(float DamageAmount, FVector HitLocation, FVector ImpulseDir, float ImpulseStrength);
	virtual void Multicast_ApplyDamageToWindow_Implementation(float DamageAmount, FVector HitLocation, FVector ImpulseDir, float ImpulseStrength);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnApplyDamageToWindow, float, DamageAmount, FVector, HitLocation, FVector, ImpulseDir, float, ImpulseStrength);

	UPROPERTY(BlueprintAssignable, Category = Damage)
		FOnApplyDamageToWindow OnApplyDamageToWindow;

	// FORCEINLINE class UDestructibleComponent* GetDestructibleWindow() { return DestructibleWindow; }
	
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS